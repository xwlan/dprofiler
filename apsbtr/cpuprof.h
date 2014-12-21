//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
//

#ifndef _CPU_PROF_H_
#define _CPU_PROF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"

#ifdef _DFKERNEL
#include "callback.h"
#endif

//
// Counter Value Order
//

typedef enum _BTR_CV_ORDER {
    CV_ENTER,
    CV_LAST,
    CV_CURRENT,
    CV_NUMBER,
} BTR_CV_ORDER;

#pragma pack(push, 4)

typedef struct _BTR_CPU_SAMPLE {
	ULONG ThreadId;
	ULONG StackId;
	ULONG KernelTime;
	ULONG UserTime;
    ULONG Cycles;
    ULONG WaitReason;
} BTR_CPU_SAMPLE, *PBTR_CPU_SAMPLE;

typedef struct _BTR_CPU_THREAD {

	union {
		SLIST_ENTRY SingleListEntry;
		LIST_ENTRY ListEntry;
	};

	ULONG ThreadId;
	HANDLE ThreadHandle;
	struct _TEB *Teb;

	FILETIME EnterTime;
	FILETIME ExitTime;

    FILETIME KernelTime[CV_NUMBER];
    FILETIME UserTime[CV_NUMBER];
    ULONG64 Cycles[CV_NUMBER]; 

	ULONG Priority;

	BOOLEAN FirstRun;
	BOOLEAN Retired;
	BOOLEAN Spare[2];

	ULONG NumberOfSamples;
	ULONG FirstSample;

} BTR_CPU_THREAD, *PBTR_CPU_THREAD;

typedef enum _BTR_CPU_FLAG {
    BTR_FLAG_CPU_SAMPLE,
    BTR_FLAG_CPU_RETIRE,
    BTR_FLAG_CPU_MARKER,
} BTR_CPU_FLAG;

typedef struct _BTR_CPU_RECORD {

	FILETIME Timestamp;
    
    double CpuUsage;
	ULONG64 KernelTime;
	ULONG64 UserTime;
    ULONG64 Cycles;

    ULONG PageFault;
	ULONG ActiveCount;
    ULONG RetireCount;
    ULONG ProfileTime;

	BTR_CPU_FLAG Flag;
	BTR_CPU_SAMPLE Sample[ANYSIZE_ARRAY];

} BTR_CPU_RECORD, *PBTR_CPU_RECORD;

//
// CPU process object to track global performance counters
//

typedef struct _BTR_CPU_PROCESS {

    ULONG ProcessId;
    HANDLE ProcessHandle;

    FILETIME EnterTime;
    FILETIME ExitTime;

    FILETIME IdleTime[CV_NUMBER];
    FILETIME KernelTime[CV_NUMBER];
    FILETIME UserTime[CV_NUMBER];

    ULONG64 CycleTime[CV_NUMBER];
    ULONG PageFault[CV_NUMBER];

    ULONG64 ProfileTime;

} BTR_CPU_PROCESS, *PBTR_CPU_PROCESS;

#pragma pack(pop)

//
// CPU Thread Stream to track sample range of each thread
// it's an array of BTR_CPU_THREAD objects
//

typedef struct _PF_STREAM_CPU_THREAD {
	BTR_CPU_THREAD Thread[ANYSIZE_ARRAY];
} PF_STREAM_CPU_THREAD, *PPF_STREAM_CPU_THREAD;

ULONG
CpuInitialize(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	);

ULONG
CpuStartProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuStopProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuUnload(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CpuQueryHotpatch(
	__in PBTR_PROFILE_OBJECT Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	);

BOOLEAN
CpuIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	);

VOID WINAPI 
CpuExitProcessCallback(
	__in UINT ExitCode
	);

//
// CPU profie internal routines
//

ULONG CALLBACK
CpuProfileProcedure(
	__in PVOID Context
	);

BOOLEAN
CpuDllCanUnload(
	VOID
	);

ULONG
CpuGenerateSample(
	__in BOOLEAN Extended	
	);

ULONG
CpuSetThreadAffinity(
	__in ULONG Cpu
	);

ULONG
CpuBuildActiveThreadList(
	VOID
	);

VOID
CpuUpdateThreadList(
	VOID
	);

VOID
CpuRemoveRetireThread(
	__in ULONG ThreadId
	);

VOID
CpuInsertFreshThread(
	__in PBTR_CPU_THREAD Thread 
	);

ULONG
CpuQueryThreadInformation(
	__in PBTR_CPU_THREAD Thread
	);

ULONG
CpuInitializeThread(
    __in PBTR_CPU_THREAD Thread,
    __in ULONG ThreadId,
    __in HANDLE ThreadHandle,
    __in BOOLEAN CurrentThread
    );

PBTR_CPU_THREAD
CpuAllocateThread(
    VOID
    );

VOID
CpuFreeThread(
    __in PBTR_CPU_THREAD Thread
    );

PBTR_CPU_RECORD
CpuAllocateRecord(
	__in ULONG Length 
	);

ULONG
CpuWriteRecord(
	__in PBTR_CPU_RECORD Record
	);

ULONG
CpuQueryThreadCycles(
    __in PBTR_CPU_THREAD Thread,
    __out PULONG64 Cycles
    );

PBTR_CPU_PROCESS
CpuAllocateProcess(
    VOID
    );

ULONG
CpuInitializeProcess(
    __in PBTR_CPU_PROCESS Object 
    );

VOID
CpuFreeProcess(
    __in PBTR_CPU_PROCESS Object
    );

//
// CPU Counters
//

VOID
CpuInitializeCounters(
	VOID
	);

VOID
CpuUpdateThreadCounters(
	__in PBTR_CPU_THREAD Thread
	);

VOID
CpuGetThreadCounters(
	__in PBTR_CPU_THREAD Thread,
	__out PULONG KernelTime, 
	__out PULONG UserTime,
	__out PULONG Cycles 
	);

VOID
CpuUpdateProcessPageFault(
    __in PBTR_CPU_PROCESS Object 
    );

VOID
CpuUpdateProcessCounters(
    __in PBTR_CPU_PROCESS Object 
    );

VOID
CpuUpdateSystemCounters(
	VOID
    );

VOID
CpuUpdateProfileCounters(
	VOID
    );

double
CpuComputeUsage(
	__in BOOLEAN IncludeOverhead
	);

ULONG
CpuFillAddressTable(
	VOID
	);

extern LIST_ENTRY CpuRetireList;
extern LIST_ENTRY CpuActiveList;

extern ULONG CpuRetireCount;
extern ULONG CpuActiveCount;

#ifdef __cplusplus
}
#endif
#endif
