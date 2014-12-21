//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PERF_H_
#define _PERF_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PERF_DELTA {
    ULONG64 Value;
    ULONG64 Delta;
} PERF_DELTA, *PPERF_DELTA;

typedef struct _PERF_INFORMATION {

	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	WCHAR Name[MAX_PATH];
	BOOLEAN Terminated;

    FLOAT CpuTotalUsage;
	FLOAT CpuKernelUsage;
    FLOAT CpuUserUsage;
	FLOAT CpuTotalPeak;

    PERF_DELTA CpuKernelDelta;
    PERF_DELTA CpuUserDelta;

    PERF_DELTA PrivateDelta; 
    PERF_DELTA VirtualDelta; 
    PERF_DELTA WorkingSetDelta; 

	ULONG64 PrivateBytesPeak;
	ULONG64 VirtualBytesPeak;
	ULONG64 WorkingSetPeak;

    PERF_DELTA IoReadDelta;
    PERF_DELTA IoWriteDelta;
    PERF_DELTA IoOtherDelta;
	ULONG IoReadPeak;
	ULONG IoWritePeak;

    PERF_DELTA BtrIoDelta;
	ULONG BtrIoPeak;

	ULONG KernelHandles;
	ULONG KernelHandlesPeak;
	ULONG UserHandles;
	ULONG UserHandlesPeak;
	ULONG GdiHandles;
	ULONG GdiHandlesPeak;

	ULONG64 SamplesCount;

} PERF_INFORMATION, *PPERF_INFORMATION;

typedef ULONG
(CALLBACK *PERF_CONSUMER_CALLBACK)(
	IN PPERF_INFORMATION Information,
	IN PVOID Context
	);

ULONG
PerfInitialize(
	IN PVOID Reserved
	);

ULONG
PerfStartWorkItem(
	VOID
	);

VOID
PerfStopWorkItem(
	VOID
	);

ULONG
PerfInsertTask(
	IN ULONG ProcessId,
	IN PWSTR Name 
	);

ULONG
PerfDeleteTask(
	IN ULONG ProcessId
	);

VOID
PerfDeleteAll(
	VOID
	);

PPERF_INFORMATION
PerfLookupTask(
	IN ULONG ProcessId
	);

BOOLEAN
PerfIsInformationValid(
	IN PPERF_INFORMATION Information
	);

ULONG
PerfUpdateInformation(
	VOID
	);

ULONG
PerfRegisterCallback(
	IN PERF_CONSUMER_CALLBACK Callback,
	IN PVOID Context
	);

ULONG
PerfUnregisterCallback(
	IN PERF_CONSUMER_CALLBACK Callback
	);

ULONG
PerfPushUpdateToConsumers(
	IN PPERF_INFORMATION Information	
	);

ULONG CALLBACK
PerfWorkItem(
	IN PVOID Context
	);

extern LIST_ENTRY PerfListHead;

#ifdef __cplusplus
}
#endif

#endif