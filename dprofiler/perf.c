//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "perf.h"
#include "ntapi.h"

ULONG PerfNumberOfTasks;
LIST_ENTRY PerfListHead;
LIST_ENTRY PerfCallbackListHead;

SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PerfCpuTotal;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *PerfCpuInformation;
ULONG PerfCpuInformationLength;

ULONG64 PerfTotalTime;
FLOAT PerfKernelUsage;
FLOAT PerfUserUsage;

PERF_DELTA PerfCpuKernelDelta;
PERF_DELTA PerfCpuUserDelta;
PERF_DELTA PerfCpuOtherDelta;

PPERF_DELTA PerfCpusKernelDelta;
PPERF_DELTA PerfCpusUserDelta;
PPERF_DELTA PerfCpusOtherDelta;

FLOAT PerfCpuKernelUsage;
FLOAT PerfCpuUserUsage;
FLOAT PerfCpuOtherUsage;

PFLOAT PerfCpusKernelUsage;
PFLOAT PerfCpusUserUsage;
PFLOAT PerfCpusOtherUsage;

BOOLEAN PerfFirstRun;
HANDLE PerfStopEvent;
HANDLE PerfThreadHandle;

ULONG CALLBACK
PerfWorkItem(
	IN PVOID Context
	)
{
	ULONG Status;
	HANDLE TimerHandle;
	LARGE_INTEGER DueTime;
	HANDLE WaitHandle[2];

	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);

	//
	// N.B. DueTime is set at 100ns granularity, check
	// MSDN for further details.
	//

	DueTime.QuadPart = -10000 * 1000;
	Status = SetWaitableTimer(TimerHandle, &DueTime, 1000, NULL, NULL, FALSE);	
	ASSERT(Status);
	
	if (Status != TRUE) {
		Status = GetLastError();
		CancelWaitableTimer(TimerHandle);
		CloseHandle(TimerHandle);
		return Status;
	}

	WaitHandle[0] = TimerHandle;
	WaitHandle[1] = PerfStopEvent;

	__try {

		while (TRUE) {

			Status = WaitForMultipleObjects(2, WaitHandle, FALSE, INFINITE);

			if (Status == WAIT_OBJECT_0) {
				PerfUpdateInformation();
			}
			if (Status == WAIT_OBJECT_0 + 1) {
				break;
			}
		}
	}

	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	CancelWaitableTimer(TimerHandle);
	CloseHandle(TimerHandle);
	return 0;
}

ULONG
PerfInitialize(
	IN PVOID Reserved
	)
{
	PerfNumberOfTasks = 0;
	InitializeListHead(&PerfListHead);
	InitializeListHead(&PerfCallbackListHead);
	
	PerfCpuInformationLength = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * BspProcessorNumber;
	PerfCpuInformation = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)BspMalloc(PerfCpuInformationLength);

	PerfCpusKernelDelta = BspMalloc(sizeof(PERF_DELTA) * BspProcessorNumber);
	PerfCpusUserDelta = BspMalloc(sizeof(PERF_DELTA) * BspProcessorNumber);
	PerfCpusOtherDelta = BspMalloc(sizeof(PERF_DELTA) * BspProcessorNumber);

	PerfCpusKernelUsage = BspMalloc(sizeof(FLOAT) * BspProcessorNumber);
	PerfCpusUserUsage = BspMalloc(sizeof(FLOAT) * BspProcessorNumber);
	PerfCpusOtherUsage = BspMalloc(sizeof(FLOAT) * BspProcessorNumber);

	PerfFirstRun = TRUE;
	PerfStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	return ERROR_SUCCESS;
}

VOID
PerfUninitialize(
	VOID
	)
{
	
}

ULONG
PerfStartWorkItem(
	VOID
	)
{
	ResetEvent(PerfStopEvent);

	PerfThreadHandle = CreateThread(NULL, 0, PerfWorkItem, NULL, 0, NULL);
	if (PerfThreadHandle) {
		return ERROR_SUCCESS;
	}

	return GetLastError();
}

VOID
PerfStopWorkItem(
	VOID
	)
{
	SetEvent(PerfStopEvent);
	WaitForSingleObject(PerfThreadHandle, INFINITE);
	CloseHandle(PerfThreadHandle);
	PerfThreadHandle = NULL;
}

ULONG
PerfInsertTask(
	IN ULONG ProcessId,
	IN PWSTR Name 
	)
{
	PPERF_INFORMATION Information;

	//
	// N.B. First lookup whether it's already there, avoid multiple
	// instances exist in the perf list
	//

	Information = PerfLookupTask(ProcessId);
	if (Information != NULL) {
		return ERROR_SUCCESS;
	}

	Information = (PPERF_INFORMATION)BspMalloc(sizeof(PERF_INFORMATION));
	ZeroMemory(Information, sizeof(PERF_INFORMATION));

	Information->ProcessId = ProcessId;
	StringCchCopy(Information->Name, MAX_PATH - 1, Name);

	InsertTailList(&PerfListHead, &Information->ListEntry);
	PerfNumberOfTasks += 1;
	return ERROR_SUCCESS;
}

PPERF_INFORMATION
PerfLookupTask(
	IN ULONG ProcessId
	)
{
	PPERF_INFORMATION Information;
	PLIST_ENTRY ListEntry;

	ListEntry = PerfListHead.Flink;
	while (ListEntry != &PerfListHead) {
		Information = CONTAINING_RECORD(ListEntry, PERF_INFORMATION, ListEntry);
		if (Information->ProcessId == ProcessId) {
			return Information;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

BOOLEAN
PerfIsInformationValid(
	IN PPERF_INFORMATION Information
	)
{
	PPERF_INFORMATION Object;

	__try {
		Object = PerfLookupTask(Information->ProcessId);		
		if (Object == Information) {
			return TRUE;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return FALSE;
	}

	return FALSE;
}

ULONG
PerfDeleteTask(
	IN ULONG ProcessId 
	)
{
	PPERF_INFORMATION Information;

	Information = PerfLookupTask(ProcessId);
	if (Information != NULL) {
		RemoveEntryList(&Information->ListEntry);
		BspFree(Information);
		PerfNumberOfTasks -= 1;
	}

	return ERROR_SUCCESS;
}

VOID
PerfDeleteAll(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PPERF_INFORMATION Information;

	while (IsListEmpty(&PerfListHead) != TRUE) {
		ListEntry = RemoveHeadList(&PerfListHead);
		Information = CONTAINING_RECORD(ListEntry, PERF_INFORMATION, ListEntry);
		BspFree(Information);
	}

	PerfNumberOfTasks = 0;
	PerfFirstRun = TRUE;
}

ULONG64 __inline
PerfUpdateDelta(
	IN PPERF_DELTA Delta, 
	IN ULONG64 Current
	)
{
    Delta->Delta = Current - Delta->Value;
    Delta->Value = Current;
	return Delta->Delta;
}

ULONG
PerfUpdateInformation(
	VOID
	)
{
	ULONG i;
    ULONG64 TotalTime;
	PLIST_ENTRY ListEntry;
	LIST_ENTRY ListHead;
	PBSP_PROCESS Process;
	PPERF_INFORMATION Information;
	NTSTATUS Status;
	LIST_ENTRY FreeListHead;

    RtlZeroMemory(&PerfCpuTotal, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    Status = (*NtQuerySystemInformation)(SystemProcessorPerformanceInformation,
									     PerfCpuInformation,
									     PerfCpuInformationLength,
									     NULL);
	ASSERT(Status == STATUS_SUCCESS);

    for (i = 0; i < BspProcessorNumber; i++) {

        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION Cpu = &PerfCpuInformation[i];

        Cpu->KernelTime.QuadPart -= Cpu->IdleTime.QuadPart + Cpu->DpcTime.QuadPart + Cpu->InterruptTime.QuadPart;

        PerfCpuTotal.DpcTime.QuadPart += Cpu->DpcTime.QuadPart;
        PerfCpuTotal.IdleTime.QuadPart += Cpu->IdleTime.QuadPart;
        PerfCpuTotal.InterruptCount += Cpu->InterruptCount;
        PerfCpuTotal.InterruptTime.QuadPart += Cpu->InterruptTime.QuadPart;
        PerfCpuTotal.KernelTime.QuadPart += Cpu->KernelTime.QuadPart;
        PerfCpuTotal.UserTime.QuadPart += Cpu->UserTime.QuadPart;

        PerfUpdateDelta(&PerfCpusKernelDelta[i], Cpu->KernelTime.QuadPart);
        PerfUpdateDelta(&PerfCpusUserDelta[i], Cpu->UserTime.QuadPart);
        PerfUpdateDelta(&PerfCpusOtherDelta[i],
			             Cpu->IdleTime.QuadPart + 
						 Cpu->DpcTime.QuadPart + 
						 Cpu->InterruptTime.QuadPart);

        TotalTime = PerfCpusKernelDelta[i].Delta + PerfCpusUserDelta[i].Delta + PerfCpusOtherDelta[i].Delta;

        if (TotalTime != 0) {
            PerfCpusKernelUsage[i] = (FLOAT)PerfCpusKernelDelta[i].Delta / TotalTime;
            PerfCpusUserUsage[i] = (FLOAT)PerfCpusUserDelta[i].Delta / TotalTime;
        }
        else {
            PerfCpusKernelUsage[i] = 0;
            PerfCpusUserUsage[i] = 0;
        }
	}

    PerfUpdateDelta(&PerfCpuKernelDelta, PerfCpuTotal.KernelTime.QuadPart);
    PerfUpdateDelta(&PerfCpuUserDelta, PerfCpuTotal.UserTime.QuadPart);
	PerfUpdateDelta(&PerfCpuOtherDelta, 
		             PerfCpuTotal.IdleTime.QuadPart + 
					 PerfCpuTotal.DpcTime.QuadPart  + 
					 PerfCpuTotal.InterruptTime.QuadPart);

    TotalTime = PerfCpuKernelDelta.Delta + PerfCpuUserDelta.Delta + PerfCpuOtherDelta.Delta;

    if (TotalTime) {
        PerfCpuKernelUsage = (FLOAT)PerfCpuKernelDelta.Delta / TotalTime;
        PerfCpuUserUsage = (FLOAT)PerfCpuUserDelta.Delta / TotalTime;
    }
    else {
        PerfCpuKernelUsage = 0;
        PerfCpuUserUsage = 0;
	}

    PerfTotalTime = TotalTime;
	
	InitializeListHead(&ListHead);
	InitializeListHead(&FreeListHead);

	BspQueryProcessList(&ListHead);

	ListEntry = PerfListHead.Flink;
	while (ListEntry != &PerfListHead) {

		HANDLE ProcessHandle;
		PROCESS_MEMORY_COUNTERS_EX MemoryCounters;
		IO_COUNTERS IoCounters;

		Information = CONTAINING_RECORD(ListEntry, PERF_INFORMATION, ListEntry);
		Process = BspQueryProcessById(&ListHead, Information->ProcessId);

		if (!Process) {

			ASSERT(Information->ProcessId != 0);

			ListEntry = ListEntry->Flink;
			Information->Terminated = TRUE;
			RemoveEntryList(&Information->ListEntry);	
			InsertTailList(&FreeListHead, &Information->ListEntry);
			
			if (!PerfFirstRun) {
				PerfPushUpdateToConsumers(Information);
			}

			continue;
		}

		PerfUpdateDelta(&Information->CpuKernelDelta, Process->KernelTime.QuadPart);
		PerfUpdateDelta(&Information->CpuUserDelta, Process->UserTime.QuadPart);

		if (PerfTotalTime != 0) {
			Information->CpuKernelUsage = (FLOAT)Information->CpuKernelDelta.Delta / PerfTotalTime;
			Information->CpuUserUsage = (FLOAT)Information->CpuUserDelta.Delta / PerfTotalTime;
			Information->CpuTotalUsage = (FLOAT)Information->CpuKernelUsage + Information->CpuUserUsage;
		} else {
			Information->CpuKernelUsage = 0;
			Information->CpuUserUsage = 0;
			Information->CpuTotalUsage = 0;
		}

		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Process->ProcessId);
		GetProcessMemoryInfo(ProcessHandle, (PPROCESS_MEMORY_COUNTERS)&MemoryCounters, 
			                 sizeof(MemoryCounters));

		GetProcessIoCounters(ProcessHandle, &IoCounters);

		PerfUpdateDelta(&Information->PrivateDelta, MemoryCounters.PrivateUsage);
		PerfUpdateDelta(&Information->WorkingSetDelta, MemoryCounters.WorkingSetSize);
		Information->WorkingSetPeak = MemoryCounters.PeakWorkingSetSize;

		PerfUpdateDelta(&Information->VirtualDelta, Process->VirtualBytes);
		PerfUpdateDelta(&Information->IoReadDelta, IoCounters.ReadTransferCount);
		PerfUpdateDelta(&Information->IoWriteDelta, IoCounters.WriteTransferCount);

		Information->KernelHandles = Process->KernelHandleCount;
		
		Information->UserHandles = GetGuiResources(ProcessHandle, GR_USEROBJECTS);
		Information->GdiHandles = GetGuiResources(ProcessHandle, GR_GDIOBJECTS);
		CloseHandle(ProcessHandle);

		if (Information->KernelHandlesPeak < Information->KernelHandles) {
			Information->KernelHandlesPeak = Information->KernelHandles;
		}

		if (Information->UserHandlesPeak < Information->UserHandles) {
			Information->UserHandlesPeak = Information->UserHandles;
		}

		if (Information->GdiHandlesPeak < Information->GdiHandles) {
			Information->GdiHandlesPeak = Information->GdiHandles;
		}

		//
		// Notify all performance data consumers via registered callback objects
		//
		
		if (!PerfFirstRun) {
			PerfPushUpdateToConsumers(Information);
		}

		ListEntry = ListEntry->Flink;
	}
	
	//
	// Deallocate died PERF_INFORMATION structures
	//

	while (IsListEmpty(&FreeListHead) != TRUE) {
		ListEntry = RemoveHeadList(&FreeListHead);
		Information = CONTAINING_RECORD(ListEntry, PERF_INFORMATION, ListEntry);
		BspFree(Information);
	}
	
	BspFreeProcessList(&ListHead);

	if (PerfFirstRun) {
		PerfFirstRun = FALSE;
	}

	return 0;
}

ULONG
PerfRegisterCallback(
	IN PERF_CONSUMER_CALLBACK Callback,
	IN PVOID Context
	)
{
	PBSP_CALLBACK CallbackObject;

	CallbackObject = (PBSP_CALLBACK)BspMalloc(sizeof(BSP_CALLBACK));
	CallbackObject->Callback = Callback;
	CallbackObject->Context = Context;
	
	InsertTailList(&PerfCallbackListHead, &CallbackObject->ListEntry);
	return ERROR_SUCCESS;
}

ULONG
PerfUnregisterCallback(
	IN PERF_CONSUMER_CALLBACK Callback
	)
{
	PBSP_CALLBACK CallbackObject;
	PLIST_ENTRY ListEntry;

	ListEntry = PerfCallbackListHead.Flink;

	while (ListEntry != &PerfCallbackListHead) {

		CallbackObject = CONTAINING_RECORD(ListEntry, BSP_CALLBACK, ListEntry);

		if (CallbackObject->Callback == (PVOID)Callback) {
			RemoveEntryList(&CallbackObject->ListEntry);	
			BspFree(CallbackObject);
			break;
		}

		ListEntry = ListEntry->Flink;
	}
	
	return ERROR_SUCCESS;
}

ULONG
PerfPushUpdateToConsumers(
	IN PPERF_INFORMATION Information	
	)
{
	PLIST_ENTRY ListEntry;
	PBSP_CALLBACK Callback;
	PERF_CONSUMER_CALLBACK ConsumerRoutine;

	ListEntry = PerfCallbackListHead.Flink;

	while (ListEntry != &PerfCallbackListHead) {

		__try {

			Callback = CONTAINING_RECORD(ListEntry, BSP_CALLBACK, ListEntry);
			ConsumerRoutine = (PERF_CONSUMER_CALLBACK)Callback->Callback;
			(*ConsumerRoutine)(Information, Callback->Context);

		} 
		__except(EXCEPTION_EXECUTE_HANDLER) {
		}

		ListEntry = ListEntry->Flink;
	}
	
	return ERROR_SUCCESS;
}