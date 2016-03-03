//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include <psapi.h>
#include "ccrprof.h"
#include "buildin.h"
#include "lock.h"
#include "heap.h"
#include "btr.h"
#include "util.h"
#include "cache.h"
#include "stream.h"
#include "marker.h"
#include "thread.h"
#include <tlhelp32.h>

//
// Ccr worker thread
//

HANDLE CcrThreadHandle;
ULONG CcrThreadId;

SLIST_HEADER CcrRetiredThreadList;

BOOLEAN
CcrCloseFiles(
	VOID
	)
{	
	return TRUE;
}

ULONG CALLBACK
CcrProfileProcedure(
	__in PVOID Context
	)
{
	ULONG Status;
	HANDLE TimerHandle;
	LARGE_INTEGER DueTime;
	HANDLE Handles[3];
	ULONG_PTR Value;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value); 

	DueTime.QuadPart = -10000 * 1000;
	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	if (!TimerHandle) {
		return S_FALSE;
	}

	//
	// Insert start mark
	//

	BtrInsertMark(0, MARK_START);
	SetWaitableTimer(TimerHandle, &DueTime, 1000, NULL, NULL, FALSE);

	Handles[0] = TimerHandle;
	Handles[1] = BtrProfileObject->StopEvent;
	Handles[2] = BtrProfileObject->ControlEnd;

	__try {

		while (TRUE) {

			Status = WaitForMultipleObjects(3, Handles, FALSE, INFINITE);

			//
			// Timer expired, flush stack record if any
			//

			if (Status == WAIT_OBJECT_0) {
				BtrFlushStackRecord();
				Status = S_OK;
			}

			//
			// Control end issue a stop request
			//

			else if (Status == WAIT_OBJECT_0 + 1) {
				BtrFlushStackRecord();
				Status = S_OK;
				break;
			}

			//
			// Control end exit unexpectedly, we need stop profile,
			// otherwise the process will crash sooner or later,
			// under this circumstance, we get stuck in the host
			// process, we just don't capture any new records, this 
			// at least keep host resource usage stable as possible.
			//

			else if (Status == WAIT_OBJECT_0 + 2) {
				BtrFlushStackRecord();
				InterlockedExchange(&BtrStopped, (LONG)TRUE);
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

	CancelWaitableTimer(TimerHandle);
	CloseHandle(TimerHandle);
	return S_OK;
}

BOOLEAN
CcrDllCanUnload(
	VOID
	)
{
	ULONG i;
	PBTR_CALLBACK Callback;

	//
	// This routine is called with all other threads suspended
	//

	for(i = 0; i < CcrCallbackCount; i++) {
		Callback = &CcrCallback[i];
		if (Callback->References != 0) {
			return FALSE;
		}
	}

	//
	// Check runtime dll references
	//

	if (BtrDllReferences != 0) {
		return FALSE;
	}

	return TRUE;
}

ULONG
CcrValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	ASSERT(Attr->Type == PROFILE_CCR_TYPE);
	return S_OK;
}

ULONG
CcrInitialize(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	//
	// Fill profile object callbacks
	//

	Object->Start = CcrStartProfile;
	Object->Stop = CcrStopProfile;
	Object->Pause = CcrPauseProfile;
	Object->Resume = CcrResumeProfile;
	Object->Unload = CcrUnload;
	Object->QueryHotpatch = CcrQueryHotpatch;
	Object->ThreadAttach = CcrThreadAttach;
	Object->ThreadDetach = CcrThreadDetach;
	Object->IsRuntimeThread = CcrIsRuntimeThread;

	//
	// Initialize mark list
	//

	BtrInitializeMarkList();

	//
	// Initialize retired thread list
	//

	InitializeSListHead(&CcrRetiredThreadList);

	if (Object->Attribute.TrackSystemLock){
		CcrTrackSystemLock = TRUE;
	} else {
		CcrTrackSystemLock = FALSE;
	}

	return S_OK;
}

ULONG
CcrStartProfile(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	HANDLE StopEvent;

	//
	// Create stop event object
	//

	StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!StopEvent) {
		return GetLastError();
	}

	Object->StopEvent = StopEvent;

	//
	// Create sampling worker thread 
	//

	CcrThreadHandle = CreateThread(NULL, 0, CcrProfileProcedure, Object, 0, &CcrThreadId);
	if (!CcrThreadHandle) {
		return GetLastError();
	}

	return S_OK;
}

ULONG
CcrStopProfile(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	InterlockedExchange(&BtrStopped, (LONG)TRUE);

	//
	// Signal stop event and wait for profile thread terminated
	//

	SetEvent(Object->StopEvent);
	WaitForSingleObject(CcrThreadHandle, INFINITE);
	CloseHandle(CcrThreadHandle);
	CcrThreadHandle = NULL;

	return S_OK;
}

ULONG
CcrPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return S_OK;
}

ULONG
CcrResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return S_OK;
}

ULONG
CcrThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return TRUE;
}

ULONG
CcrThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	PBTR_THREAD_OBJECT Thread;
	ULONG_PTR Value;
	ULONG ThreadId;

	ThreadId = GetCurrentThreadId();

	if (BtrIsRuntimeThread(ThreadId)) {
		return TRUE;
	}

	if (BtrIsExemptionCookieSet(GOLDEN_RATIO)) {
		return TRUE;
	}

	Thread = (PBTR_THREAD_OBJECT)BtrGetTlsValue();
	if (!Thread) {
		BtrSetExemptionCookie(GOLDEN_RATIO, &Value);
		return TRUE;
	}

	if (Thread->Type == NullThread) {
		BtrSetExemptionCookie(GOLDEN_RATIO, &Value);
		return TRUE;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	BtrSetExemptionCookie(GOLDEN_RATIO, &Value);

	//
	// N.B. We don't remove thread object from BtrThreadObjectList,
	// since we will walk the whole list when stop profiling
	//

	return TRUE;
}

//
// This routine can only be called in unloading stage, after the profile
// thread stopped, and before the runtime unloaded, this routine is called
// to merge all threads' stack trace into stack trace table, prepare to
// write to CCR stack stream.
//

ULONG
CcrInsertStackRecord(
	_In_ PBTR_STACK_RECORD Record
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_STACK_RECORD Entry;
	PBTR_SPIN_HASH Hash;
	BOOLEAN Found = FALSE;

	Bucket = STACK_ENTRY_BUCKET(Record->Hash);
	Hash = &BtrStackTable.Hash[Bucket];

	ListHead = &Hash->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, BTR_STACK_RECORD, ListEntry);
		if (Entry->Hash == Record->Hash && Entry->Depth == Record->Depth &&
			Entry->Frame[0] == Record->Frame[0] && Entry->Frame[1] == Record->Frame[1]) {
			Entry->Count += 1;
			Found = TRUE;
			break;
		}
		ListEntry = ListEntry->Flink;
	}

	if (Found) {
		return Entry->StackId;
	}

	//
	// Generate stack record id
	//

	Record->StackId = BtrAcquireStackId();
	InsertHeadList(ListHead, &Record->ListEntry2);
	BtrStackTable.Count += 1;
	return Record->StackId;
}

//
// N.B. This routine must be called after all per thread 
// stack record has been inserted into BtrStackTable to
// free allocated pages.
//

VOID
CcrFreeStackPagePerThread(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	PBTR_STACK_RECORD_PAGE Page;
	PLIST_ENTRY ListEntry;

	//
	// Free current page it's not on retired page list
	//

	Page = Thread->StackPage;
	if (Page) {
		VirtualFree(Page, 0, MEM_RELEASE);
		Page = NULL;
	}

	//
	// Free retired page list
	//

	while (!IsListEmpty(&Thread->StackPageRetireList)) {
		ListEntry = RemoveHeadList(&Thread->StackPageRetireList);
		Page = CONTAINING_RECORD(ListEntry, BTR_STACK_RECORD_PAGE, ListEntry);
		VirtualFree(Page, 0, MEM_RELEASE);
	}

	//
	// Free lookaside header, note that we're not free its entries since its entries
	// are allocated from current page or a retired page. lookaside header must be
	// aligned freed, since it's aligned malloced.
	//

	if (Thread->StackRecordLookaside) {
		BtrAlignedFree(Thread->StackRecordLookaside);
		Thread->StackRecordLookaside = NULL;
	}

	if (Thread->StackTable){
		BtrFree(Thread->StackTable);
		Thread->StackTable = NULL;
	}
}

VOID
CcrMergeStackTraceByThread(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListEntry2;
	PBTR_THREAD_OBJECT Thread;
	PBTR_STACK_TABLE_PER_THREAD Table;
	PBTR_STACK_RECORD Record;
	ULONG StackId;
	ULONG i;

	//
	// N.B. This is a tedious, and time consuming procedure, we may need
	// optimization to reduce the overhead of this procedure
	//

	ListEntry = BtrThreadObjectList.Flink;
	while (ListEntry != &BtrThreadObjectList) {
		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD_OBJECT, ListEntry);

		//
		// If the thread ever acquired a lock, insert its stack record into global 
		// stack record table
		//

		if (Thread->StackTable) {
			Table = Thread->StackTable;

			for(i = 0; i < STACK_RECORD_BUCKET_PER_THREAD; i++) {

				while (!IsListEmpty(&Table->ListHead[i])) {
					ListEntry2 = RemoveHeadList(&Table->ListHead[i]);
					Record = CONTAINING_RECORD(ListEntry2, BTR_STACK_RECORD, ListEntry);
					StackId = CcrInsertStackRecord(Record);

					//
					// Scan the lock cache to update stack record pointer to be stack id
					//

					CcrUpdateLockTrackByStackId(Thread, Record, StackId);
				}
			}
		}
		
		if (BtrProfileObject->Attribute.Mode == RECORD_MODE) {
			BtrCloseMappingPerThread(Thread);
		}

		if (Thread->RundownHandle != NULL) {
			CloseHandle(Thread->RundownHandle);
			Thread->RundownHandle = NULL;
		}

		if (Thread->Buffer) {
			VirtualFree(Thread->Buffer, 0, MEM_RELEASE);
			Thread->Buffer = NULL;
		}

		ListEntry = ListEntry->Flink;
	}
}

VOID
CcrFreePerThreadResource(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListEntry2;
	PBTR_THREAD_OBJECT Thread;

	//
	// N.B. This is a tedious, and time consuming procedure, we may need
	// optimization to reduce the overhead of this procedure
	//

	ListEntry = BtrThreadObjectList.Flink;
	while (ListEntry != &BtrThreadObjectList) {

		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD_OBJECT, ListEntry);

		if (Thread->StackRecordLookaside) {
			BtrAlignedFree(Thread->StackRecordLookaside);
			Thread->StackRecordLookaside = NULL;
		}

		if (Thread->StackPage) {
			VirtualFree(Thread->StackPage, 0, MEM_RELEASE);
			Thread->StackPage = NULL;
		}

		while (!IsListEmpty(&Thread->StackPageRetireList)) {
			PBTR_STACK_RECORD_PAGE StackPage;
			ListEntry2 = RemoveHeadList(&Thread->StackPageRetireList);
			StackPage = CONTAINING_RECORD(ListEntry2, BTR_STACK_RECORD_PAGE, ListEntry);
			VirtualFree(StackPage, 0, MEM_RELEASE);
		}

		if (Thread->TracePage){
			VirtualFree(Thread->TracePage, 0, MEM_RELEASE);
			Thread->TracePage = NULL;
		}

		while (!IsListEmpty(&Thread->TracePageRetireList)) {
			PCCR_STACKTRACE_PAGE TracePage;
			ListEntry2 = RemoveHeadList(&Thread->StackPageRetireList);
			TracePage = CONTAINING_RECORD(ListEntry2, CCR_STACKTRACE_PAGE, ListEntry);
			VirtualFree(TracePage, 0, MEM_RELEASE);
		}

		ListEntry = ListEntry->Flink;
	}
}

ULONG
CcrPrepareProfileStream(
	VOID
	)
{
	ULONG Status;

	Status = CcrRetireAllThreads();
	if (Status != S_OK) {
		return Status;
	}

	CcrMergeStackTraceByThread();
	return S_OK;
}

ULONG
CcrRetireAllThreads(
	VOID
	)
{
	LIST_ENTRY SuspendeeList;
	ULONG Count;
	BOOLEAN Exist;

retry:
	Count = 0;
	InitializeListHead(&SuspendeeList);

	BtrSuspendProcess(&SuspendeeList, &Count);
	BtrWalkSuspendeeStack(&SuspendeeList);

	Exist = BtrScanFrameFromSuspendeeList(&SuspendeeList);
	if (Exist) {

		//
		// If there's any pending runtime frame, yield the CPU and
		// retry again.
		//

		BtrResumeProcess(&SuspendeeList, FALSE);
		Sleep(1000);
		goto retry;
	}

	//
	// Resume other threads
	//

	BtrResumeProcess(&SuspendeeList, TRUE);
	return S_OK;
}

ULONG
CcrUnload(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	BOOLEAN Unload;
	BOOLEAN Terminate = FALSE;

	//
	// N.B. Disable thread attach and detach notification 
	//

	DisableThreadLibraryCalls((HMODULE)BtrDllBase);

	//
	// Prepare profile stream because we use a per thread
	// strategy to capture profile information, we need
	// walk the thread list to merge them before write to
	// profile report.
	//

	Status = CcrPrepareProfileStream();
	if (Status != S_OK) {

		LIST_ENTRY SuspendeeList;
		ULONG Count;

		//
		// N.B. If preparation failed, suspend the whole process,
		// and terminate later after write CCR profile stream, we
		// have no better choice ??
		//

		BtrSuspendProcess(&SuspendeeList, &Count);
		CcrMergeStackTraceByThread();
		Terminate = TRUE;
	}

	//
	// Close index and data file
	//

	BtrCloseCache();
	BtrCloseStackFile();

	//
	// Write profile report file
	//

	Status = BtrWriteProfileStream(BtrProfileObject->ReportFileObject);
	CcrCloseFiles();
	CcrFreePerThreadResource();

	CloseHandle(BtrProfileObject->ReportFileObject);
	BtrProfileObject->ReportFileObject = NULL;

	CloseHandle(BtrProfileObject->StopEvent);
	BtrProfileObject->StopEvent = NULL;

	CloseHandle(BtrProfileObject->ControlEnd);
	BtrProfileObject->ControlEnd = NULL;

	if (Terminate) {

		//
		// If we can not unload, just terminate host process
		//

		SetEvent(BtrProfileObject->UnloadEvent);
		CloseHandle(BtrProfileObject->UnloadEvent);
		ExitProcess(0);

		//
		// never arrive here
		//
	}

	//
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

BOOLEAN
CcrIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	)
{
	if (ThreadId == CcrThreadId) {
		return TRUE;
	}
	return FALSE;
}

ULONG
CcrQueryHotpatch(
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
	ULONG CallbackCount;
	PBTR_HOTPATCH_ENTRY Hotpatch;

	Valid = 0;

	CallbackCount = CcrCallbackCount;
	Hotpatch = (PBTR_HOTPATCH_ENTRY)BtrMalloc(sizeof(BTR_HOTPATCH_ENTRY) * CallbackCount);

	if (Object->Attribute.EnableFile) {
	}

	for(Number = 0; Number < CcrCallbackCount; Number += 1) {

		Callback = &CcrCallback[Number];
		DllHandle = GetModuleHandleA(Callback->DllName);
		Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

		if (Callback->Address != NULL) {

			if (Action == HOTPATCH_COMMIT) {
				Status = BtrRegisterCallbackEx(Callback);
			} else {
				Status = BtrUnregisterCallbackEx(Callback);
			}

			if (Status == S_OK) {
				RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
					sizeof(BTR_HOTPATCH_ENTRY));
				Valid += 1;
			}
		}

	}

	*Entry = Hotpatch;
	*Count = Valid;
	return S_OK;
}

VOID WINAPI 
CcrExitProcessCallback(
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

PCCR_LOCK_CACHE
CcrGetLockCache(
	_In_ PBTR_THREAD_OBJECT Thread	
	)
{
	ULONG Number;
	PCCR_LOCK_CACHE Cache;

	ASSERT(Thread != NULL);

	if (Thread->CcrLockCache) {
		return Thread->CcrLockCache;
	}

	BtrEnterExemptionRegion(Thread);
	Cache = (PCCR_LOCK_CACHE)BtrMalloc(sizeof(CCR_LOCK_CACHE));
	Cache->LockTrackCount = 0;
	Cache->StackTraceCount = 0;
	Cache->Current = NULL;
	Cache->StackInHeapAlloc = 0;
	Cache->StackInHeapFree = 0;
	Cache->StackInAcquire = 0;
	Cache->StackInKernelWait = 0;

	for(Number = 0; Number < CCR_LOCK_CACHE_BUCKET; Number += 1) {
		InitializeListHead(&Cache->LockList[Number]);
	}

	Thread->CcrLockCache = Cache;
	BtrLeaveExemptionRegion(Thread);
	return Cache;
}

#define CCR_LOCKTRACK_BUCKET(_P) \
	((ULONG)((ULONG_PTR)_P % CCR_LOCK_CACHE_BUCKET))

PCCR_LOCK_TRACK
CcrLookupLockTrack(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PVOID LockPtr,
	_In_ BOOLEAN Allocate
	)
{
	PCCR_LOCK_CACHE Cache;	
	ULONG Bucket;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PCCR_LOCK_TRACK Track;
	BOOLEAN Found = FALSE;

	Cache = CcrGetLockCache(Thread);
	ASSERT(Cache != NULL);

	Bucket = CCR_LOCKTRACK_BUCKET(LockPtr);
	ListHead = &Cache->LockList[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Track = CONTAINING_RECORD(ListEntry, CCR_LOCK_TRACK, ListEntry);
		if (Track->LockPtr == LockPtr) {
			Found = TRUE;
			break;
		}
		ListEntry = ListEntry->Flink;
	}

	if (Found) {
		return Track;
	}

	if (!Allocate) {
		return NULL;
	}
	
	BtrEnterExemptionRegion(Thread);
	Track = CcrAllocateLockTrack();
	InsertHeadList(ListHead, &Track->ListEntry);
	Cache->LockTrackCount += 1;
	BtrLeaveExemptionRegion(Thread);
	return Track;
}

VOID
CcrTrackLockAcquire(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PCCR_LOCK_TRACK Track,
	_In_ CCR_PROBE_TYPE Type,
	_In_ PLARGE_INTEGER Start,
	_In_ PLARGE_INTEGER End,
	_In_ BOOLEAN Owner
	)
{
	PCCR_LOCK_CACHE Cache;	

	//
	// Clear bit of StackInAcquire
	//

	switch (Type) {
	case CCR_PROBE_ENTER_CS:
		Track->Acquire += 1;
		break;

	case CCR_PROBE_TRY_ENTER_CS:
		Track->TryAcquire += 1;
		break;

	case CCR_PROBE_ACQUIRE_SRW_EXCLUSIVE:
		Track->AcquireExclusive += 1;
		break;

	case CCR_PROBE_ACQUIRE_SRW_SHARED:
		Track->AcquireShared += 1;
		break;

	case CCR_PROBE_TRY_ACQUIRE_SRW_EXCLUSIVE:
		Track->TryAcquireExclusive += 1;
		break;

	case CCR_PROBE_TRY_ACQUIRE_SRW_SHARED:
		Track->TryAcquireShared += 1;
		break;
	default:
		ASSERT(0);
		return;
	}

	if (Owner){
		Track->LockOwner = Thread->ThreadId;
		Track->RecursiveCount += 1;

		//
		// N.B. Only track latency if it's not a recusive acquisition, since
		// there's no significant cost to acquired a holding lock, meanwhile,
		// we track the start time to measure the holding duration when lock
		// is released.
		//

		if (Track->RecursiveCount == 1) {
			Track->Start = *Start;
			if (End->QuadPart - Start->QuadPart > Track->MaximumAcquireLatency.QuadPart){
				Track->MaximumAcquireLatency.QuadPart = End->QuadPart - Start->QuadPart;
			}
		}

		//
		// Check whether it's a heap lock acquisition
		//

		Cache = Thread->CcrLockCache;
		if (Cache->StackInHeapAlloc) {
			Track->HeapAllocAcquire += 1;
		}
		if (Cache->StackInHeapFree) {
			Track->HeapFreeAcquire += 1;
		}

		//
		// Check whether it ever waited in kernel
		//

		if (Cache->StackInKernelWait) {
			Track->KernelWait += 1;
		}

		//
		// Clear stack bits
		//

		Cache->StackInHeapAlloc = 0;
		Cache->StackInHeapFree = 0;
		Cache->StackInAcquire = 0;
		Cache->StackInKernelWait = 0;
	}
	else {
		Track->TryFailure += 1;
	}
}

VOID
CcrTrackLockRelease(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PCCR_LOCK_TRACK Track,
	_In_ CCR_PROBE_TYPE Type,
	_In_ PLARGE_INTEGER End
	)
{
	if (!Track->LockOwner) {
		return;
	}

	Track->Release += 1;

	//
	// If current thread release all references to this lock,
	// clear thread owner state
	//

	Track->RecursiveCount -= 1;

	ASSERT(Track->RecursiveCount != -1);
	ASSERT(Track->LockOwner == Thread->ThreadId);

	if (!Track->RecursiveCount) {
		Track->LockOwner = 0;
	}

	//
	// Mark largest holding duration
	//

	if ((ULONG64)End->QuadPart - (ULONG64)Track->Start.QuadPart > (ULONG64)Track->MaximumHoldingDuration.QuadPart){
		Track->MaximumHoldingDuration.QuadPart = (ULONG64)End->QuadPart - (ULONG64)Track->Start.QuadPart;
	}
}

PCCR_LOCK_TRACK
CcrAllocateLockTrack(
	VOID
	)
{
	PCCR_LOCK_TRACK Track;

	Track = (PCCR_LOCK_TRACK)BtrMalloc(sizeof(CCR_LOCK_TRACK));
	RtlZeroMemory(Track, sizeof(CCR_LOCK_TRACK));
	return Track;
}

VOID
CcrFreeLockTrack(
	_In_ PCCR_LOCK_TRACK Track 
	)
{
	BtrFree(Track);
}

PCCR_STACKTRACE_PAGE
CcrAllocateStackTracePage(
	VOID
	)
{
	PCCR_STACKTRACE_PAGE Page;
	SIZE_T Size;

	Size = FIELD_OFFSET(CCR_STACKTRACE_PAGE, Entries[CCR_COUNT_OF_STACKTRACE_PER_PAGE]);
	Page = (PCCR_STACKTRACE_PAGE)VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE);
	if (!Page) {
		return NULL;
	}
	Page->Count = CCR_COUNT_OF_STACKTRACE_PER_PAGE;
	Page->Next = 0;
	return Page;
}

PCCR_STACKTRACE
CcrAllocateStackTrace(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	PCCR_STACKTRACE_PAGE Page;
	PCCR_STACKTRACE_PAGE StackPage;
	PCCR_STACKTRACE Trace;

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);

	StackPage = Thread->TracePage;
	if (!StackPage) {

		//
		// First time usage initialization of per thread stack record data structure
		//

		InitializeListHead(&Thread->TracePageRetireList);
		StackPage = CcrAllocateStackTracePage();
		Thread->TracePage = StackPage;
	}

	if (StackPage->Next < StackPage->Count) {
		Trace = &StackPage->Entries[StackPage->Next];
		StackPage->Next += 1;

	} else {

		//
		// Current page is full, retire it and allocate new page
		//

		InsertHeadList(&Thread->StackPageRetireList, &StackPage->ListEntry);
		Page = CcrAllocateStackTracePage();
		if (Page != NULL) {
			ASSERT(Page->Next == 0);
			Trace = &Page->Entries[Page->Next];
			Page->Next += 1;
			Thread->TracePage = Page;

		} else {
			Trace = NULL;
		}

	}
	return Trace;
}

VOID
CcrInsertStackTrace(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PCCR_LOCK_TRACK Track,
	_In_ PBTR_STACK_RECORD Record
	)
{
	PCCR_STACKTRACE Trace;
	PCCR_LOCK_CACHE Cache;

	Cache = Thread->CcrLockCache;
	ASSERT(Cache != NULL);

	Trace = Track->StackTrace;
	while (Trace) {
		if (Trace->u.Record == Record) {
			Trace->Count += 1;
			return;
		}
		Trace = Trace->Next;
	}

	//
	// It's a new stack trace for this lock, insert it
	//

	BtrEnterExemptionRegion(Thread);
	Trace = CcrAllocateStackTrace(Thread);
	RtlZeroMemory(Trace, sizeof(*Trace));
	ASSERT(Trace != NULL);
	BtrLeaveExemptionRegion(Thread);

	Trace->Count = 1;
	Trace->u.Record = Record;
	Track->StackTraceCount += 1;

	//
	// We track stack trace count here primarily for easy calculation
	// of write buffer size when serialize lock track to disk file.
	//

	Cache->StackTraceCount += 1;
	Trace->Next = Track->StackTrace;
	Track->StackTrace = Trace;
}

VOID
CcrUpdateLockTrackStackTraceId(
	_In_ PCCR_LOCK_TRACK Track,
	_In_ PBTR_STACK_RECORD Record,
	_In_ ULONG StackId
	)
{
	PCCR_STACKTRACE Trace;

	if (!Track->StackTraceCount){
		return;
	}

	Trace = Track->StackTrace;
	ASSERT(Trace != NULL);

	//
	// Update the record ptr as acquired stack id
	//

	while (Trace) {
		if (Trace->u.Record == Record) {
			Trace->u.Record = NULL;
			Trace->u.StackId = StackId;
			break;
		}
		Trace = Trace->Next;
	}
}

VOID
CcrUpdateLockTrackByStackId(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PBTR_STACK_RECORD Record,
	_In_ ULONG StackId
	)
{
	PCCR_LOCK_CACHE Cache;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PCCR_LOCK_TRACK Track;
	ULONG i;

	Cache = Thread->CcrLockCache;
	if (!Cache) {
		return;
	}
	for(i = 0; i < CCR_LOCK_CACHE_BUCKET; i++) {
        ListHead = &Cache->LockList[i];
		ListEntry = ListHead->Flink;
		while (ListEntry != ListHead) {
			Track = CONTAINING_RECORD(ListEntry, CCR_LOCK_TRACK, ListEntry);
			CcrUpdateLockTrackStackTraceId(Track, Record, StackId);
			ListEntry = ListEntry->Flink;
		}
	}
}

VOID
CcrPrintStatistics(
	VOID
	)
{
	
}