//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
//

//
// N.B. This file is from dprobe project, tailor it to be as minimum as possible
// we only retain CPU time related code
//

#ifndef _PERF_H_
#define _PERF_H_

#include "apsbtr.h"
#include "apsprofile.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _PERF_PROCESS {

	LIST_ENTRY ListEntry;
	PVOID PebAddress;
	ULONG ProcessId;
	ULONG ParentId;
	ULONG SessionId;
	PWSTR Name;
	PWSTR FullPath;
	PWSTR CommandLine;
	
	ULONG Priority;
	ULONG ThreadCount;
	FILETIME CreateTime; 
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	ULONG KernelHandleCount;
	ULONG GdiHandleCount;
	ULONG UserHandleCount;
	ULONG_PTR VirtualBytes;
	ULONG_PTR PrivateBytes;
	ULONG_PTR WorkingSetBytes;
	LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;

} PERF_PROCESS, *PPERF_PROCESS;

typedef struct _PERF_DELTA {
    ULONG64 Value;
    ULONG64 Delta;
} PERF_DELTA, *PPERF_DELTA;

typedef struct _PERF_INFORMATION {

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

	ULONG StackEntryBuckets;
	ULONG StackTextBuckets;

	ULONG64 SamplesCount;

} PERF_INFORMATION, *PPERF_INFORMATION;


ULONG
PerfInitialize(
    VOID
	);

BOOLEAN
PerfIsFirstRun(
	VOID
	);

ULONG
PerfUpdateCpuTime(
	__in PPERF_PROCESS Process
	);

VOID
PerfUpdateProcessTime(
    __in ULONG64 KernelTime, 
    __in ULONG64 UserTime 
    );

FLOAT
PerfGetCpuUsage(
    VOID
    );


#ifdef __cplusplus
}
#endif

#endif