//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "cpuprof.h"
#include "btr.h"
#include "heap.h"
#include "thread.h"
#include "hal.h"
#include "cache.h"
#include "stacktrace.h"
#include "util.h"
#include "buildin.h"
#include "hal.h"
#include "stream.h"
#include <tlhelp32.h>
#include "perf.h"
#include "marker.h"
#include "timer.h"


DECLSPEC_CACHEALIGN
SLIST_HEADER CpuAttachList;

DECLSPEC_CACHEALIGN
SLIST_HEADER CpuDetachList;

LIST_ENTRY CpuRetireList;
LIST_ENTRY CpuActiveList;

ULONG CpuRetireCount;
ULONG CpuActiveCount;

PBTR_CPU_RECORD CpuRecordBuffer;
ULONG CpuRecordBufferLength;

HANDLE CpuThreadHandle;
ULONG CpuThreadId;

//
// CPU profiling support pause/resume operations
//

DECLSPEC_CACHEALIGN
ULONG CpuPaused;

//
// CPU callback types
//

BTR_CALLBACK CpuCallback[] = {
	{ CpuCallbackType, 0,0,0,0, CpuExitProcessCallback, "kernel32.dll", "ExitProcess", 0 },
};

ULONG CpuCallbackCount = ARRAYSIZE(CpuCallback);

//
// Flags for OpenThread() API, it can fail on some version of Windows if
// we simply require THREAD_ALL_ACCESS.
//

#define CPU_OPENTHREAD_FLAG (THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |\
	                         THREAD_QUERY_INFORMATION | SYNCHRONIZE)

#define CPU_OPENPROCESS_FLAG (PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|\
                              PROCESS_VM_READ|PROCESS_VM_WRITE | SYNCHRONIZE)


//
// CPU process object
//

PBTR_CPU_PROCESS CpuProcess;


ULONG
CpuInitialize(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	//
	// Fill control callback routines
	//

	Object->Start = CpuStartProfile;
	Object->Stop = CpuStopProfile;
	Object->Pause = CpuPauseProfile;
	Object->Resume = CpuResumeProfile;
	Object->Unload = CpuUnload;
	Object->QueryHotpatch = CpuQueryHotpatch;
	Object->ThreadAttach = CpuThreadAttach;
	Object->ThreadDetach = CpuThreadDetach;
	Object->IsRuntimeThread = CpuIsRuntimeThread;

	//
	// Initialize various thread list
	//

	InitializeSListHead(&CpuAttachList);
	InitializeSListHead(&CpuDetachList);

	InitializeListHead(&CpuRetireList);
	InitializeListHead(&CpuActiveList);

	CpuRetireCount = 0;
	CpuActiveCount = 0;

	CpuRecordBuffer = NULL;
	CpuRecordBufferLength = 0;

	CpuPaused = FALSE;

	//
	// Initialize CPU performance counters
	//

	PerfInitialize();

	//
	// Initialize mark list
	//

	BtrInitializeMarkList();

	//
	// Initialize address table
	//

	CpuFillAddressTable();

	return S_OK;
}

ULONG
CpuValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	ASSERT(Attr->Type == PROFILE_CPU_TYPE);

	//
	// CPU profiling must be record mode
	//

	if (Attr->Mode != RECORD_MODE) {
		return BTR_E_INVALID_PARAMETER;
	}

	if (Attr->StackDepth > MAX_STACK_DEPTH) {
		return BTR_E_INVALID_PARAMETER;
	}
	
	if (Attr->SamplingPeriod < MINIMUM_SAMPLE_PERIOD) {
		return BTR_E_INVALID_PARAMETER;
	}

	return S_OK;
}

ULONG
CpuStartProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	HANDLE EventHandle;

	//
	// Create stop event object
	//

	EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!EventHandle) {
		return GetLastError();
	}

	Object->StopEvent = EventHandle;

    //
    // Create pause event object
    //

	EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!EventHandle) {
		return GetLastError();
	}

    Object->PauseEvent = EventHandle;

	//
	// Create sampling worker thread 
	//

	CpuThreadHandle = CreateThread(NULL, 0, CpuProfileProcedure, Object, 0, &CpuThreadId);
	if (!CpuThreadHandle) {
		return GetLastError();
	}

	return S_OK;
}


ULONG
CpuStopProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	//
	// If the profile is paused, resume it before stop
	//

	CpuResumeProfile(Object);
	
	//
	// Mark global stop flag and wait profile thread exit
	//

	InterlockedExchange(&BtrStopped, (LONG)TRUE);

	SetEvent(Object->StopEvent);
	WaitForSingleObject(CpuThreadHandle, INFINITE);
	CloseHandle(CpuThreadHandle);
	CpuThreadHandle = NULL;

	return S_OK;
}

ULONG
CpuPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	InterlockedCompareExchange(&CpuPaused, (LONG)TRUE, FALSE);
	return S_OK;
}

ULONG
CpuResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	InterlockedCompareExchange(&CpuPaused, (LONG)FALSE, TRUE);

    //
    // Wake up profile procedure
    //

    SetEvent(Object->PauseEvent);
	return S_OK;
}

ULONG
CpuUnload(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	BOOLEAN Unload;

	//
	// Wait until it's safe to unload dll
	//

	while(CpuDllCanUnload() != TRUE) {
		Sleep(2000);
	}

	//
	// Close index and data file
	//

	BtrCloseCache();

	//
	// Close stack record file
	//

	BtrCloseStackFile();

	//
	// Write profile report file
	//

	Status = BtrWriteProfileStream(BtrProfileObject->ReportFileObject);
	CloseHandle(BtrProfileObject->ReportFileObject);
	BtrProfileObject->ReportFileObject = NULL;

	CloseHandle(BtrProfileObject->StopEvent);
	BtrProfileObject->StopEvent = NULL;

	CloseHandle(BtrProfileObject->ControlEnd);
	BtrProfileObject->ControlEnd = NULL;

	//
	// This indicate a detach stop, need free up resources
	//

	BtrUninitializeStack();
	BtrUninitializeCache();
	BtrUninitializeTrap();
	BtrUninitializeThread();
	BtrUninitializeHeap();

	if (BtrProfileObject->ExitStatus == BTR_S_USERSTOP) {
		Unload = TRUE;

	} else {

		//
		// ExitProcess is called, we should not free runtime library
		//

		Unload = FALSE;
	}

	//
	// Close shared data object 
	//

	UnmapViewOfFile(BtrSharedData);
	CloseHandle(BtrProfileObject->SharedDataObject);

	BtrUninitializeHal();

	__try {

		//
		// Signal unload event to notify control end that unload is completed
		//

		SetEvent(BtrProfileObject->UnloadEvent);
		CloseHandle(BtrProfileObject->UnloadEvent);

		if (Unload) {
			FreeLibraryAndExitThread((HMODULE)BtrDllBase, 0);
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}

	return S_OK;
}

ULONG
CpuQueryHotpatch(
	__in PBTR_PROFILE_OBJECT Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	)
{
	ULONG Number;
	PBTR_CALLBACK Callback;
	HMODULE DllHandle;
	ULONG Valid;
	ULONG Status;
	PBTR_HOTPATCH_ENTRY Hotpatch;

	Valid = 0;

	Hotpatch = (PBTR_HOTPATCH_ENTRY)BtrMalloc(sizeof(BTR_HOTPATCH_ENTRY) * CpuCallbackCount);

	if (Action == HOTPATCH_COMMIT) {

		for(Number = 0; Number < CpuCallbackCount; Number += 1) {

			Callback = &CpuCallback[Number];
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

			if (Callback->Address != NULL) {

				Status = BtrRegisterCallbackEx(Callback);
				if (Status == S_OK) {
					
					//
					// Copy hotpatch entry
					//

					RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
						          sizeof(BTR_HOTPATCH_ENTRY));
					Valid += 1;
				}
			}
		}

	}
	else if (Action == HOTPATCH_DECOMMIT) {

		for(Number = 0; Number < CpuCallbackCount; Number += 1) {

			Callback = &CpuCallback[Number];
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

			if (Callback->Address != NULL) {

				Status = BtrUnregisterCallbackEx(Callback);
				if (Status == S_OK) {

					//
					// Copy hotpatch entry
					//

					RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
						          sizeof(BTR_HOTPATCH_ENTRY));
					Valid += 1;
				}
			}
		}
	}

	else {
		ASSERT(0);
	}

	*Entry = Hotpatch;
	*Count = Valid;
	return S_OK;
}

ULONG
CpuThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	PBTR_CPU_THREAD Thread;
	HANDLE ThreadHandle;
	ULONG ThreadId;

	ThreadId = GetCurrentThreadId();

	//
	// Skip runtime thread
	//

	if (BtrIsRuntimeThread(ThreadId)) {
		return TRUE;
	}

    //
    // N.B. Even if OpenThread failed, we still return TRUE, otherwise,
    // target process can't function as expected
    //

	ThreadHandle = OpenThread(CPU_OPENTHREAD_FLAG, FALSE, ThreadId);
	if (!ThreadHandle) {
		return TRUE;
	}

    //
    // Allocate and initialize current thread object
    //

    Thread = CpuAllocateThread();
    Thread->ThreadId = GetCurrentThreadId();
    Thread->ThreadHandle = ThreadHandle;

	//
	// Enqueue to thread attach list
	//

	InterlockedPushEntrySList(&CpuAttachList, &Thread->SingleListEntry);
	return TRUE;
}

ULONG
CpuThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	PBTR_CPU_THREAD Thread;

    Thread = CpuAllocateThread();
	Thread->ThreadId = GetCurrentThreadId();

	//
	// Enqueue to thread detach list
	//

	InterlockedPushEntrySList(&CpuDetachList, &Thread->SingleListEntry);
	return TRUE;
}

BOOLEAN
CpuIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	)
{
	if (ThreadId == CpuThreadId) {
		return TRUE;
	}

	return FALSE;
}

VOID WINAPI 
CpuExitProcessCallback(
	__in UINT ExitCode
	)
{
	ULONG_PTR Value;
	ULONG Current;
	static ULONG Concurrency = 0;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value);

	Current = InterlockedCompareExchange(&Concurrency, 1, 0);
	if (Current != 0) {

		//
		// Another thread acquires lock to call ExitProcess
		//

		return;
	}
	
	__try {

		BtrProfileObject->ExitStatus = BTR_S_EXITPROCESS;
		SignalObjectAndWait(BtrProfileObject->ExitProcessEvent,
							BtrProfileObject->ExitProcessAckEvent, 
							INFINITE, FALSE);
		//
		// N.B. The following code won't enter runtime anymore since all
		// callbacks are unregistered by control end.
		//

		ExitProcess(ExitCode);

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}

BOOLEAN
CpuDllCanUnload(
	VOID
	)
{
	ULONG Number;
	PBTR_CALLBACK Callback;

	for(Number = 0; Number < CpuCallbackCount; Number += 1) {
		Callback = &CpuCallback[Number];
		if (Callback->References != 0) {
			return FALSE;
		}
	}

	if (BtrDllReferences != 0) {
		return FALSE;
	}

	return TRUE;
}

ULONG CALLBACK
CpuProfileProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	ULONG TimerPeriod;
	HANDLE TimerHandle;
	LARGE_INTEGER DueTime;
	HANDLE Handles[3];
	ULONG_PTR Value;
    BOOLEAN IsPaused;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value); 
	
	//
	// Insert start mark
	//

	BtrInsertMark(0, MARK_START);

    //
    // Allocate and initialize CPU process object
    //

    CpuProcess = CpuAllocateProcess();
    CpuInitializeProcess(CpuProcess);

	//
	// Enumerate all active threads
	//

	CpuBuildActiveThreadList();

	//
	// Fill initial time value
	//

	CpuInitializeCounters();

	//
	// Set profile thread to work on CPU 0 to make time measurement consistent,
    // we may adjust this behavior by hardware resources and utilization, it's
    // best to affinite our profiler thread to a low utilization CPU.
	//

#if defined(_DF_CPU_AFFINITY)
	SetThreadIdealProcessor(GetCurrentThread(), 0);
	CpuSetThreadAffinity(0);
#endif

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    //
    // N.B. Initialize high resolution timer
    //

    BtrInitializeTimerList();

	//
	// Create profiling timer based on sample period
	//

	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	TimerPeriod = BtrProfileObject->Attribute.SamplingPeriod;

	DueTime.QuadPart = -10000 * TimerPeriod;
	SetWaitableTimer(TimerHandle, &DueTime, TimerPeriod, 
		             NULL, NULL, FALSE);	

	Handles[0] = TimerHandle;
	Handles[1] = BtrProfileObject->StopEvent;
	Handles[2] = BtrProfileObject->ControlEnd;

	__try {

		while (TRUE) {

            //
            // First check whether it's paused by user
            //

            IsPaused = (BOOLEAN)ReadForWriteAccess(&CpuPaused);
            if (IsPaused) {

				BtrInsertMark(BtrCurrentSequence(), MARK_PAUSE);
                Status = WaitForSingleObject(BtrProfileObject->PauseEvent, INFINITE);
                if (Status != WAIT_OBJECT_0) {
                    return GetLastError();
                }

                ResetEvent(BtrProfileObject->PauseEvent);
				BtrInsertMark(BtrCurrentSequence(), MARK_RESUME);
            }
             
			//
			// N.B. This is to reduce CPU bias due to our sampling
			//

			CpuUpdateSystemCounters();
			CpuUpdateProcessCounters(CpuProcess);
			CpuUpdateProfileCounters();

			Status = WaitForMultipleObjects(3, Handles, FALSE, INFINITE);

			//
			// Timer expired, collect CPU samples
			//

			if (Status == WAIT_OBJECT_0) {
				CpuGenerateSample(TRUE);
				BtrProfileObject->Attribute.SamplesDepth += 1;
			}

			//
			// Control end issue a stop request
			//

			else if (Status == WAIT_OBJECT_0 + 1) {
				Status = S_OK;
				break;
			}

			//
			// Control end is closed unexpectedly
			//

			else if (Status == WAIT_OBJECT_0 + 2) {
				Status = S_OK;
				break;
			}
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = BTR_E_EXCEPTION;
	}
	
	if (Status != S_OK) {
		BtrInsertMark(BtrCurrentSequence(), MARK_ABORT);
	}
	else {
		BtrInsertMark(BtrCurrentSequence(), MARK_STOP);
	}
    
    //
    // Clean timers and restore old resolution
    //

    BtrDestroyTimerList(); 
	return S_OK;
}

ULONG
CpuSetThreadAffinity(
	__in ULONG Cpu
	)
{
	DWORD_PTR Mask = 1;

	Mask = Mask << Cpu;
	SetThreadAffinityMask(GetCurrentThread(), Mask);
	return S_OK;
}

ULONG
CpuGenerateSample(
	__in BOOLEAN Extended	
	)
{
	PBTR_CPU_RECORD Record;
	PLIST_ENTRY ListEntry;
	PBTR_CPU_THREAD Thread;
	ULONG Length;
	ULONG Status;
	ULONG_PTR StackBase;
	ULONG_PTR StackLimit;
	ULONG Hash;
	ULONG Depth;
	ULONG StackId;
	ULONG Number;
	LIST_ENTRY StackRecordList;
	LIST_ENTRY FailedThreadList;
	PTEB Teb;
	PBTR_STACK_RECORD StackRecord;
	CONTEXT Context;
	PVOID Callers[MAX_STACK_DEPTH];
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER EnterTime;

	QueryPerformanceCounter(&EnterTime);

	//
	// Update current active thread list
	//

	Number = 0;
	Context.ContextFlags = CONTEXT_FULL;

	InitializeListHead(&StackRecordList);
	InitializeListHead(&FailedThreadList);

    //
    // Allocate CPU profile record
    //

	Length = FIELD_OFFSET(BTR_CPU_RECORD, Sample[CpuActiveCount]);
	Record = CpuAllocateRecord(Length);
    RtlZeroMemory(Record, sizeof(BTR_CPU_RECORD));

    //
    // Mark timestamp of the record
    //

	GetSystemTimeAsFileTime(&Record->Timestamp);
    Record->Flag = BTR_FLAG_CPU_SAMPLE;

	//
	// N.B. Get pagefault here to avoid our sampling incur more pagefaults
	//

    CpuUpdateProcessPageFault(CpuProcess);
    Record->PageFault = CpuProcess->PageFault[CV_CURRENT] - CpuProcess->PageFault[CV_LAST];

	//
	// Walk each active thread to collect stack records
	//

	ListEntry = CpuActiveList.Flink;
	while (ListEntry != &CpuActiveList) {
	
		//
		// Suspend target thread
		//

		Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, ListEntry);
		Status = SuspendThread(Thread->ThreadHandle);

		//
		// N.B. Status is suspend count, if it's -1, it means failed to suspend
		// the target thread.
		//

		if (Status != -1) {

			//
			// Get target thread's context record
			//

			Status = GetThreadContext(Thread->ThreadHandle, &Context);
			if (Status != FALSE) {
                
#if defined(_M_IX86)
				Callers[0] = (PVOID)(ULONG_PTR)Context.Eip;
#elif defined(_M_X64)
				Callers[0] = (PVOID)(ULONG_PTR)Context.Rip;
#endif
                //
                // N.B. Ensure the caller's PC is valid
                //

                ASSERT(Callers[0] != NULL);

				//
				// N.B. Thread's StackLimit can dynamically change, we must
				// fetch its current value after suspend it.
				//

				Teb = Thread->Teb;
				ASSERT(Teb != NULL);

				StackBase = (ULONG_PTR)((PULONG_PTR)Teb->Tib.StackBase - 1);
				StackLimit = (ULONG_PTR)Teb->Tib.StackLimit;

                //
                // N.B. RtlVirtualUnwind may modify the CONTEXT structure, so we need
                // record the target RIP in Callers[0]
                //

				Depth = BtrRemoteWalkStack(Thread->ThreadId, StackBase, StackLimit,
									       &Context, MAX_STACK_DEPTH - 1, &Callers[1], &Hash);
				Depth = BtrValidateStackTrace((PULONG_PTR)&Callers[1], Depth);
				Depth += 1;

				//
				// Update thread's CPU time
				//

				CpuUpdateThreadCounters(Thread);
				ResumeThread(Thread->ThreadHandle);

				//
				// Track the thread's first sample index if this is the first sample
				// for target thread
				//

				if (!Thread->NumberOfSamples) {

					//
					// N.B. SamplesDepth is increased after return of this function
					//

					Thread->FirstSample = BtrProfileObject->Attribute.SamplesDepth;
				}

			    Thread->NumberOfSamples += 1;

				//
				// Insert stack record
				//

				StackRecord = BtrAllocateStackRecord();
				StackRecord->Committed = 0;
				StackRecord->Heap = 0;
				StackRecord->Depth = Depth;
				StackRecord->Count = 0;
				StackRecord->StackId = -1;
				StackRecord->Hash = Hash;
				StackRecord->SizeOfAllocs = 0;
				RtlCopyMemory(StackRecord->Frame, Callers, Depth * sizeof(PVOID));	

				//
				// Insert stack record into temporary list, all records are
				// processed after stack walking of all target threads. Again,
				// this is to do as less as possible work in sampling work loop
				//

				InsertTailList(&StackRecordList, &StackRecord->ListEntry2);

				//
				// Fill CPU sample record per thread and increment sample count
				//

				Record->Sample[Number].ThreadId = Thread->ThreadId;
				CpuGetThreadCounters(Thread, &Record->Sample[Number].KernelTime,
									&Record->Sample[Number].UserTime, 
									&Record->Sample[Number].Cycles);

                //
                // Accumulate to process's counter
                //

                Record->KernelTime += Record->Sample[Number].KernelTime;
                Record->UserTime += Record->Sample[Number].UserTime;
                Record->Cycles += Record->Sample[Number].Cycles;

                Record->ActiveCount += 1;
                Number += 1;

				ListEntry = ListEntry->Flink;
				
			}
			else {

				//
				// Failed to get thread context, just skip it and finally check its state
				// after sampling, the thread may be terminating or terminated
				//

				ListEntry = ListEntry->Flink;
				RemoveEntryList(&Thread->ListEntry);
				InsertTailList(&FailedThreadList, &Thread->ListEntry);

				ResumeThread(Thread->ThreadHandle);
			}
		} 

		else {

			//
			// Failed to suspend thread, just skip it and finally check its state
			// after sampling, the thread may be terminating or terminated
			//
			
			ListEntry = ListEntry->Flink;
			RemoveEntryList(&Thread->ListEntry);
			InsertTailList(&FailedThreadList, &Thread->ListEntry);
		}

	}
	
	//
	// Insert stack record to stack table in sampling order
	// and update CPU sample's stack record id
	//

	Number = 0;
	while (IsListEmpty(&StackRecordList) != TRUE) {
		ListEntry = RemoveHeadList(&StackRecordList);
		StackRecord = CONTAINING_RECORD(ListEntry, BTR_STACK_RECORD, ListEntry2);
		StackId = BtrInsertStackEntry(StackRecord);
		Record->Sample[Number].StackId = StackId;
		Number += 1;
	}

	//
	// Check failed thread state
	//

	while (IsListEmpty(&FailedThreadList) != TRUE) {

		ListEntry = RemoveHeadList(&FailedThreadList);
		Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, ListEntry);

		//
		// If the thread is terminated, insert into retire thread list
		//

		if (WAIT_OBJECT_0 == WaitForSingleObject(Thread->ThreadHandle, 0)) {

            ULONG Index;

            //
            // Append retire thread into CPU record
            //

            Index = Record->ActiveCount + Record->RetireCount;
            Record->Sample[Index].ThreadId = Thread->ThreadId;
            Record->RetireCount += 1;
			
            CloseHandle(Thread->ThreadHandle);
            Thread->ThreadHandle = NULL;

            InsertTailList(&CpuRetireList, &Thread->ListEntry);
			CpuRetireCount += 1;
			CpuActiveCount -= 1;

		}
		else {

			//
			// Insert into tail of active thread list
			//

			InsertTailList(&CpuActiveList, &Thread->ListEntry);
		}
	}

	//
	// Compute CPU usage of current process
	//

	Record->CpuUsage = CpuComputeUsage(TRUE);

	//
	// Track profiling cost by ticks
	//

	QueryPerformanceCounter(&ExitTime);
	Record->ProfileTime = (ULONG)(ExitTime.QuadPart - EnterTime.QuadPart);

	//
	// Write CPU sampling record and flush stack record
	//

	CpuWriteRecord(Record);

    //
    // Update thread list
    //

    CpuUpdateThreadList();

	return S_OK;
}

PBTR_CPU_RECORD
CpuAllocateRecord(
	__in ULONG Length 
	)
{
	if (CpuRecordBuffer != NULL) {

		if (Length < CpuRecordBufferLength) {
			return CpuRecordBuffer;
		}

		VirtualFree(CpuRecordBuffer, 0, MEM_RELEASE);
	}

	CpuRecordBufferLength = BtrUlongRoundUp(Length, (ULONG)BtrPageSize);
	CpuRecordBuffer = (PBTR_CPU_RECORD)VirtualAlloc(NULL, CpuRecordBufferLength, 
		                                            MEM_COMMIT, PAGE_READWRITE);
	return CpuRecordBuffer;
}

ULONG
CpuWriteRecord(
	__in PBTR_CPU_RECORD Record
	)
{
	ULONG Status;
	ULONG Sequence;
	ULONG Complete;
	ULONG Length;
    ULONG Count;
	BTR_FILE_INDEX Index;

	//
	// Write index record
	//

	Sequence = InterlockedIncrement(&BtrSharedData->Sequence);

    //
    // N.B. CPU record also include retired threads as notification
    //

    Count = Record->ActiveCount + Record->RetireCount;
	Length = FIELD_OFFSET(BTR_CPU_RECORD, Sample[Count]);

	Index.Committed = 1;
	Index.Excluded = 0;
	Index.Length = Length;
	Index.Reserved = 0;
	Index.Offset = BtrSharedData->DataValidLength;

	Status = WriteFile(BtrProfileObject->IndexFileObject, &Index, sizeof(Index), &Complete, NULL);
	if (Status != TRUE) {
		return GetLastError();
	}

	BtrSharedData->IndexValidLength += sizeof(BTR_FILE_INDEX);

	//
	// Write data record
	//

	Status = WriteFile(BtrProfileObject->DataFileObject, Record, Length, &Complete, NULL);
	if (Status != TRUE) {
		return GetLastError();
	}

	BtrSharedData->DataValidLength += Length;

	//
	// Flush stack record
	//

	BtrFlushStackRecord();
	return S_OK;
}

//
// CpuBuildActiveThreadList is called only once for CPU profiling
//

ULONG
CpuBuildActiveThreadList(
	VOID
	)
{
	ULONG Status;
	ULONG CurrentProcessId;
	ULONG CurrentThreadId;
	HANDLE Toolhelp;
	HANDLE ThreadHandle;
	THREADENTRY32 ThreadEntry;
	PBTR_CPU_THREAD Thread;

	CurrentProcessId = GetCurrentProcessId();
	CurrentThreadId = GetCurrentThreadId();

    Toolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, CurrentProcessId);
	if (Toolhelp == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	ThreadEntry.dwSize = sizeof(ThreadEntry);
    Status = Thread32First(Toolhelp, &ThreadEntry);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	Status = S_OK;

	do {

		//
		// Enumerate all threads of current process except current runtime thread
		//

		if (ThreadEntry.th32OwnerProcessID == CurrentProcessId && 
			ThreadEntry.th32ThreadID != CurrentThreadId && 
			BtrIsRuntimeThread(ThreadEntry.th32ThreadID) != TRUE ) {

				ThreadHandle = OpenThread(CPU_OPENTHREAD_FLAG, FALSE, ThreadEntry.th32ThreadID);
				if (ThreadHandle != NULL) {

                    Thread = CpuAllocateThread();
                    CpuInitializeThread(Thread, ThreadEntry.th32ThreadID, 
                                        ThreadHandle, FALSE);

					//
					// Chain thread to active thread list and increment active count
					//

					InsertTailList(&CpuActiveList, &Thread->ListEntry);
					CpuActiveCount += 1;
				} 
		} 

    } while (Thread32Next(Toolhelp, &ThreadEntry));
 
	CloseHandle(Toolhelp);
	return S_OK;
}

VOID
CpuUpdateThreadList(
	VOID
	)
{
	PSLIST_ENTRY ListEntry;
	PBTR_CPU_THREAD Thread;

	//
	// Remove retire thread from active thread list
	//

	ListEntry = InterlockedFlushSList(&CpuDetachList);
	while (ListEntry != NULL) {
		Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, SingleListEntry);
		ListEntry = ListEntry->Next;
		CpuRemoveRetireThread(Thread->ThreadId);
		BtrAlignedFree(Thread);
	}

	//
	// Add fresh thread into active list thread
	//

	ListEntry = InterlockedFlushSList(&CpuAttachList);
	while (ListEntry != NULL) {
		Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, SingleListEntry);
		ListEntry = ListEntry->Next;
		CpuInsertFreshThread(Thread);
	}
}

VOID
CpuRemoveRetireThread(
	__in ULONG ThreadId
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CPU_THREAD Thread;

	ListEntry = CpuActiveList.Flink;

	while (ListEntry != &CpuActiveList) {

		//
		// Remove retire thread from active thread list
		//

		Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, ListEntry);
		if (Thread->ThreadId == ThreadId) {

			//
			// If found the retire thread, unchained from active thread list
			// and enqueue to retire thread list
			//

			RemoveEntryList(&Thread->ListEntry);

            if (Thread->ThreadHandle) {
                CloseHandle(Thread->ThreadHandle);
                Thread->ThreadHandle = NULL;
            }

			Thread->Retired = TRUE;
			GetSystemTimeAsFileTime(&Thread->ExitTime);

			InsertHeadList(&CpuRetireList, &Thread->ListEntry);

			CpuActiveCount -= 1;
			CpuRetireCount += 1;
			break;
		}

		ListEntry = ListEntry->Flink;
	}
}

VOID
CpuInsertFreshThread(
	__in PBTR_CPU_THREAD Thread 
	)
{
	//
	// N.B. Insert fresh thread into tail of active thread list
	// this postpones sampling of fresh thread and make the sampling
	// more effective.
	//

    CpuInitializeThread(Thread, Thread->ThreadId, Thread->ThreadHandle, FALSE);

	InsertTailList(&CpuActiveList, &Thread->ListEntry);
	CpuActiveCount += 1;
}


ULONG
CpuQueryThreadInformation(
	__in PBTR_CPU_THREAD Thread
	)
{
    ASSERT(0);
	return S_OK;
}

//
// N.B. Thread object, ThreadId, ThreadHandle, must be valid value 
// when CpuInitializeThread is called, CurrentThread specify whether
// the specified ThreadId is current thread, this help to get TEB address
//

ULONG
CpuInitializeThread(
    __in PBTR_CPU_THREAD Thread,
    __in ULONG ThreadId,
    __in HANDLE ThreadHandle,
    __in BOOLEAN CurrentThread
    )
{
    FILETIME Enter;
    FILETIME Exit;
    ULONG64 Cycles;

    ASSERT(Thread != NULL);
    ASSERT(ThreadHandle != NULL);

    Thread->ThreadId = ThreadId;
    Thread->ThreadHandle = ThreadHandle;

    if (CurrentThread) {
        Thread->Teb = NtCurrentTeb();
    } else {
        Thread->Teb = (PTEB)BtrQueryTebAddress(ThreadHandle);
    }

	//
	// Track thread timestamp when enter profile session
    //

    GetSystemTimeAsFileTime(&Thread->EnterTime);

    //
    // N.B. Enter, Exit must be passed to GetThreadTimes, it's mandatory parameters,
    // otherwise it will raise access violation exception
    //

    GetThreadTimes(Thread->ThreadHandle, &Enter, &Exit, 
                   (FILETIME *)&Thread->KernelTime[CV_ENTER], 
                   (FILETIME *)&Thread->UserTime[CV_ENTER]); 

    Thread->KernelTime[CV_LAST] = Thread->KernelTime[CV_ENTER];
    Thread->UserTime[CV_LAST] = Thread->UserTime[CV_ENTER];
    Thread->KernelTime[CV_CURRENT] = Thread->KernelTime[CV_ENTER];
    Thread->UserTime[CV_CURRENT] = Thread->UserTime[CV_ENTER];

	//
	// N.B. Query CPU cycles, this may require at least NT 6, need further check
	//

    CpuQueryThreadCycles(Thread, &Cycles);
	Thread->Cycles[CV_ENTER] = Cycles;
	Thread->Cycles[CV_LAST] = Cycles;
	Thread->Cycles[CV_CURRENT] = Cycles;
	
    Thread->Priority = (ULONG)GetThreadPriority(Thread->ThreadHandle);
    Thread->FirstRun = TRUE;

    return S_OK;
}

PBTR_CPU_THREAD
CpuAllocateThread(
    VOID
    )
{
    PBTR_CPU_THREAD Object;

	Object = (PBTR_CPU_THREAD)BtrAlignedMalloc(sizeof(BTR_CPU_THREAD),
		                                       MEMORY_ALLOCATION_ALIGNMENT);
	RtlZeroMemory(Object, sizeof(BTR_CPU_THREAD));
    return Object;
}

VOID 
CpuFreeThread(
    __in PBTR_CPU_THREAD Object 
    )
{
    ASSERT(Object != NULL);

    if (Object) {
        BtrAlignedFree(Object);
    }
}

ULONG
CpuQueryThreadCycles(
    __in PBTR_CPU_THREAD Thread,
    __out PULONG64 Cycles
    )
{
    NTSTATUS NtStatus;
    THREAD_CYCLE_TIME_INFORMATION Information;

    NtStatus = (*NtQueryInformationThread)(Thread->ThreadHandle, ThreadCycleTime,
                                           &Information, sizeof(Information), NULL);

    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    *Cycles = Information.AccumulatedCycles.QuadPart;
    return S_OK;
}

ULONG
CpuInitializeProcess(
    __in PBTR_CPU_PROCESS Object 
    )
{
    PROCESS_MEMORY_COUNTERS Counters;
    ULONG ProcessId;
    HANDLE ProcessHandle;

    ProcessId = GetCurrentProcessId();
    ProcessHandle = OpenProcess(CPU_OPENPROCESS_FLAG, FALSE, ProcessId);
    if (!ProcessHandle){
        return GetLastError();
    }

    GetProcessMemoryInfo(ProcessHandle, &Counters, sizeof(Counters));
    Object->PageFault[CV_ENTER] = Counters.PageFaultCount;
    Object->PageFault[CV_LAST] = Counters.PageFaultCount;
    Object->PageFault[CV_CURRENT] = Counters.PageFaultCount;

    Object->ProcessId = ProcessId;
    Object->ProcessHandle = ProcessHandle;
    return S_OK;
}

PBTR_CPU_PROCESS
CpuAllocateProcess(
    VOID
    )
{
    PBTR_CPU_PROCESS Object;

    Object = (PBTR_CPU_PROCESS)BtrAlignedMalloc(sizeof(BTR_CPU_PROCESS), 
                                                MEMORY_ALLOCATION_ALIGNMENT);
    RtlZeroMemory(Object, sizeof(BTR_CPU_PROCESS));
    return Object;
}

VOID
CpuFreeProcess(
    __in PBTR_CPU_PROCESS Object
    )
{
    if (Object) {
        if (Object->ProcessHandle) {
            CloseHandle(Object->ProcessHandle);
        }
        BtrAlignedFree(Object);
    }
}

//
// CPU Time Counters
//

FILETIME CpuTotalIdleTime[CV_NUMBER];
FILETIME CpuTotalKernelTime[CV_NUMBER];
FILETIME CpuTotalUserTime[CV_NUMBER];

FILETIME CpuProcessKernelTime[CV_NUMBER];
FILETIME CpuProcessUserTime[CV_NUMBER];

FILETIME CpuProfileKernelTime[CV_NUMBER];
FILETIME CpuProfileUserTime[CV_NUMBER];

LARGE_INTEGER CpuTotalDelta;
LARGE_INTEGER CpuTotalIdleDelta;
LARGE_INTEGER CpuTotalKernelDelta;
LARGE_INTEGER CpuTotalUserDelta;

LARGE_INTEGER CpuProcessDelta;
LARGE_INTEGER CpuProcessKernelDelta;
LARGE_INTEGER CpuProcessUserDelta;

LARGE_INTEGER CpuProfileDelta;
LARGE_INTEGER CpuProfileKernelDelta;
LARGE_INTEGER CpuProfileUserDelta;

VOID
CpuInitializeCounters(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CPU_THREAD Thread;

	//
	// Fill each thread's initial time value
	//

	CpuUpdateProcessPageFault(CpuProcess);
	CpuUpdateSystemCounters();
	CpuUpdateProcessCounters(CpuProcess);

	ListEntry = CpuActiveList.Flink;
	while (ListEntry != &CpuActiveList) {
		Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, ListEntry);
		CpuUpdateThreadCounters(Thread);
		ListEntry = ListEntry->Flink;
	}

	CpuUpdateProfileCounters();
}

VOID 
CpuComputeTimeDelta(
	__in PFILETIME T1,
	__in PFILETIME T2,
	__out PLARGE_INTEGER Delta
	)
{
	LARGE_INTEGER L1, L2;

	L1.LowPart = T1->dwLowDateTime;
	L1.HighPart = T1->dwHighDateTime;
	L2.LowPart = T2->dwLowDateTime;
	L2.HighPart = T2->dwHighDateTime;

	Delta->QuadPart = L2.QuadPart - L1.QuadPart;
}

VOID
CpuUpdateThreadCounters(
	__in PBTR_CPU_THREAD Object 
	)
{
	FILETIME Create;
	FILETIME Exit;
	FILETIME Kernel;
	FILETIME User;
    ULONG64 Cycles;

	GetThreadTimes(Object->ThreadHandle, &Create, &Exit, &Kernel, &User);

    Object->KernelTime[CV_LAST] = Object->KernelTime[CV_CURRENT];
    Object->KernelTime[CV_CURRENT] = Kernel;
    Object->UserTime[CV_LAST] = Object->UserTime[CV_CURRENT];
    Object->UserTime[CV_CURRENT] = User;

    CpuQueryThreadCycles(Object, &Cycles);
    Object->Cycles[CV_LAST] = Object->Cycles[CV_CURRENT];
    Object->Cycles[CV_CURRENT] = Cycles;
}

VOID
CpuGetThreadCounters(
	__in PBTR_CPU_THREAD Thread,
	__out PULONG KernelTime, 
	__out PULONG UserTime,
	__out PULONG Cycles 
	)
{
	LARGE_INTEGER Delta;

	//
	// N.B. Origin time is in 100 nanoseconds unit,
	//      1 ms = 1000 us = 1000*1000 ns
	//

	CpuComputeTimeDelta(&Thread->KernelTime[CV_LAST],&Thread->KernelTime[CV_CURRENT],&Delta);
	*KernelTime = (ULONG)(Delta.QuadPart * 10 / 1000);
	CpuComputeTimeDelta(&Thread->UserTime[CV_LAST],&Thread->UserTime[CV_CURRENT],&Delta);
	*UserTime = (ULONG)(Delta.QuadPart * 10 / 1000);
	*Cycles = (ULONG)(Thread->Cycles[CV_CURRENT] - Thread->Cycles[CV_LAST]);
}

VOID
CpuUpdateProcessPageFault(
    __in PBTR_CPU_PROCESS Object 
    )
{
    PROCESS_MEMORY_COUNTERS Counters;

    //
    // PageFault[CV_CURRENT] - PageFault[CV_LAST] is the pagefault delta
    // in last sampling period
    //

    GetProcessMemoryInfo(Object->ProcessHandle, &Counters, sizeof(Counters));
    Object->PageFault[CV_LAST] = Object->PageFault[CV_CURRENT];
    Object->PageFault[CV_CURRENT] = Counters.PageFaultCount;
}

VOID
CpuUpdateProcessCounters(
    __in PBTR_CPU_PROCESS Object 
    )
{
	FILETIME Create, Exit, Kernel, User;

	GetProcessTimes(Object->ProcessHandle, &Create, &Exit, &Kernel, &User);
	CpuProcessKernelTime[CV_LAST] = CpuProcessKernelTime[CV_CURRENT];
	CpuProcessKernelTime[CV_CURRENT] = Kernel;
	CpuProcessUserTime[CV_LAST] = CpuProcessUserTime[CV_CURRENT];
	CpuProcessUserTime[CV_CURRENT] = User;
}

VOID
CpuUpdateSystemCounters(
	VOID
    )
{
	FILETIME Idle, Kernel, User;

	GetSystemTimes(&Idle, &Kernel, &User);
	CpuTotalIdleTime[CV_LAST] = CpuTotalIdleTime[CV_CURRENT];
	CpuTotalIdleTime[CV_CURRENT] = Idle;
	CpuTotalKernelTime[CV_LAST] = CpuTotalKernelTime[CV_CURRENT];
	CpuTotalKernelTime[CV_CURRENT] = Kernel;
	CpuTotalUserTime[CV_LAST] = CpuTotalUserTime[CV_CURRENT];
	CpuTotalUserTime[CV_CURRENT] = User;
}

VOID
CpuUpdateProfileCounters(
	VOID
    )
{
	FILETIME Create, Exit, Kernel, User;

	GetThreadTimes(GetCurrentThread(), &Create, &Exit, &Kernel, &User);
	CpuProfileKernelTime[CV_LAST] = CpuProfileKernelTime[CV_CURRENT];
	CpuProfileKernelTime[CV_CURRENT] = Kernel;
	CpuProfileUserTime[CV_LAST] = CpuProfileUserTime[CV_CURRENT];
	CpuProfileUserTime[CV_CURRENT] = User;
}

double
CpuComputeUsage(
	__in BOOLEAN IncludeOverhead
	)
{
	DOUBLE Percent;
	LONGLONG TotalDelta;
	LONGLONG ProcessDelta;

	//
	// Compute system delta time
	//

	CpuUpdateSystemCounters();

	CpuComputeTimeDelta(&CpuTotalIdleTime[CV_LAST], &CpuTotalIdleTime[CV_CURRENT], &CpuTotalIdleDelta);
	CpuComputeTimeDelta(&CpuTotalKernelTime[CV_LAST], &CpuTotalKernelTime[CV_CURRENT], &CpuTotalKernelDelta);
	CpuComputeTimeDelta(&CpuTotalUserTime[CV_LAST],&CpuTotalUserTime[CV_CURRENT], &CpuTotalUserDelta);
	
	CpuTotalDelta.QuadPart = CpuTotalKernelDelta.QuadPart + CpuTotalUserDelta.QuadPart;

	//
	// Compute process delta time
	//

	CpuUpdateProcessCounters(CpuProcess);
	CpuComputeTimeDelta(&CpuProcessKernelTime[CV_LAST],&CpuProcessKernelTime[CV_CURRENT],&CpuProcessKernelDelta);
	CpuComputeTimeDelta(&CpuProcessUserTime[CV_LAST],&CpuProcessUserTime[CV_CURRENT],&CpuProcessUserDelta);
	CpuProcessDelta.QuadPart = CpuProcessKernelDelta.QuadPart + CpuProcessUserDelta.QuadPart;
	
	//
	// Compute profile delta time
	//

	CpuUpdateProfileCounters();
	CpuComputeTimeDelta(&CpuProfileKernelTime[CV_LAST], &CpuProfileKernelTime[CV_CURRENT],&CpuProfileKernelDelta);
	CpuComputeTimeDelta(&CpuProfileUserTime[CV_LAST],&CpuProfileUserTime[CV_CURRENT],&CpuProfileUserDelta);
	CpuProfileDelta.QuadPart = CpuProfileKernelDelta.QuadPart + CpuProfileUserDelta.QuadPart;
		
	//
	// Compute the process's CPU usage 
	//

	if (IncludeOverhead) {
		TotalDelta = CpuTotalDelta.QuadPart;
		ProcessDelta = CpuProcessDelta.QuadPart;
	} else {
		TotalDelta = CpuTotalDelta.QuadPart - CpuProfileDelta.QuadPart;
		ProcessDelta = CpuProcessDelta.QuadPart - CpuProfileDelta.QuadPart;
	}

	//
	// Ensure no divide by zero
	//

	TotalDelta = max(TotalDelta, 1);
	Percent = ProcessDelta * 100.0 / TotalDelta;

    DebugTrace("CPU usage: %.2f", Percent);
	return Percent;
}

//
// CPU PC state table, used to map PC to thread's CPU state
//

BTR_CPU_ADDRESS BtrCpuAddressTable[BTR_CPU_ADDRESS_COUNT] = {

//
// KiFastSystemCallRet (win32 only)
//

{ _KiFastSystemCallRet, 0, CPU_STATE_RUN, "KiFastSystemCallRet" },

//
// Yield/Sleep
//

{ _NtYieldExecution, 0, CPU_STATE_SLEEP, "NtYieldExecution" },
{ _NtDelayExecution, 0, CPU_STATE_SLEEP, "NtDelayExecution" },

//
// Synchronization
//

{ _NtWaitForSingleObject,			0, CPU_STATE_WAIT, "NtWaitForSingleObject" },
{ _NtWaitForMultipleObjects,		0, CPU_STATE_WAIT, "NtWaitForMultipleObjects" },
{ _NtSignalAndWaitForSingleObject,	0, CPU_STATE_WAIT, "NtSignalAndWaitForSingleObject" },
{ _NtWaitForKeyedEvent,				0, CPU_STATE_WAIT, "NtWaitForKeyedEvent" },
{ _NtWaitForWorkViaWorkerFactory,	0, CPU_STATE_WAIT, "NtWaitForWorkViaWorkerFactory" },

//
// I/O
//

{ _NtCreateFile,				0, CPU_STATE_IO, "NtCreateFile" },
{ _NtOpenFile,					0, CPU_STATE_IO, "NtOpenFile" },
{ _NtReadFile,					0, CPU_STATE_IO, "NtReadFile" },
{ _NtWriteFile,					0, CPU_STATE_IO, "NtWriteFile" },
{ _NtReadFileScatter,			0, CPU_STATE_IO, "NtReadFileScatter" },
{ _NtWriteFileGather,			0, CPU_STATE_IO, "NtWriteFileGather" },
{ _NtDeleteFile,				0, CPU_STATE_IO, "NtDeleteFile" },
{ _NtFsControlFile,				0, CPU_STATE_IO, "NtFsControlFile" },
{ _NtDeviceIoControlFile,		0, CPU_STATE_IO, "NtDeviceIoControlFile"},
{ _NtQueryInformationFile,		0, CPU_STATE_IO, "NtQueryInformationFile"},
{ _NtQueryDirectoryFile,		0, CPU_STATE_IO, "NtQueryDirectoryFile"},
{ _NtQueryAttributesFile,		0, CPU_STATE_IO, "NtQueryAttributesFile"},
{ _NtQueryFullAttributesFile,	0, CPU_STATE_IO, "NtQueryFullAttributesFile"},
{ _NtQueryEaFile,				0, CPU_STATE_IO, "NtQueryEaFile"},
{ _NtSetInformationFile,		0, CPU_STATE_IO, "NtSetInformationFile"},
{ _NtSetEaFile,					0, CPU_STATE_IO, "NtSetEaFile" },
};

ULONG
CpuFillAddressTable(
	VOID
	)
{
	ULONG Number;
	ULONG Count;
	PBTR_CPU_ADDRESS Entry;
	PVOID NtdllPtr;

	NtdllPtr = HalGetModulePtr("ntdll.dll");
	if (!NtdllPtr) {
		return 0;
	}

	Count = 0;

	for(Number = 0; Number < BTR_CPU_ADDRESS_COUNT; Number += 1) {
		Entry = &BtrCpuAddressTable[Number];
		Entry->Address = (ULONG64)HalGetProcedureAddress(NtdllPtr, Entry->Name); 
		if (Entry->Address) {
			Count += 1;
		}
	}

	return Count;
}