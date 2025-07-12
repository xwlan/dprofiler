//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _APS_CPU_H_
#define _APS_CPU_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "apsctl.h"
#include "cpuprof.h"
#include "apspdb.h"
    
typedef struct _CPU_THREAD_PC {
	LIST_ENTRY ListEntry;
	PVOID Pc;
	ULONG FunctionId;
	ULONG DllId;
	ULONG LineId;
	ULONG Inclusive;
	ULONG Exclusive;
	ULONG Depth;
} CPU_THREAD_PC, *PCPU_THREAD_PC;

#define CPU_PC_BUCKET 233

typedef struct _CPU_THREAD_PC_TABLE {
    ULONG Inclusive;
    ULONG Exclusive;
	ULONG Count;
	LIST_ENTRY ListHead[CPU_PC_BUCKET];
} CPU_THREAD_PC_TABLE, *PCPU_THREAD_PC_TABLE;

//
// N.B. This is a temporary definition, we need
// map the thread state to kernel's thread state
// include wait, e.g. UserRequest, IoPending etc.
//

#define CPU_THREAD_STATE_RUNNING  0
#define CPU_THREAD_STATE_RETIRED  1

//
// The maximum thread number for thread counter list
//

#define CPU_MAX_THREAD 1024

//
// The maximum stack trace list for a specified pc
//

#define CPU_MAX_PC_STACKTRACE 64

//
// Stack record per thread
//

typedef struct _CPU_STACK_ENTRY {
    ULONG StackId;
    ULONG Count;
    LIST_ENTRY ListEntry;
} CPU_STACK_ENTRY, *PCPU_STACK_ENTRY;

//
// N.B. The ListHead is used during table build up,
// the Table is valid after build up, it's expected
// that CPU_STACK_ENTRY is sorted in descending order
//

#define CPU_STACK_BUCKET  CPU_PC_BUCKET

typedef struct _CPU_STACK_TABLE {
    ULONG ThreadId;
    ULONG Count;
    PCPU_STACK_ENTRY Table;
    LIST_ENTRY ListHead[CPU_STACK_BUCKET];
} CPU_STACK_TABLE, *PCPU_STACK_TABLE;

//
// Per thread CPU state during sample period
//

typedef struct _CPU_THREAD_STATE {

	ULONG ThreadId;
	ULONG Count;

	//
	// Valid only when build thread state array to
	// point to current write location
	//

	ULONG Current;

	//
	// The first sample index of current thread
	// which track the (start, end) range
	//

	ULONG First;  

	BTR_CPU_STATE State[ANYSIZE_ARRAY];

} CPU_THREAD_STATE, *PCPU_THREAD_STATE;

//
// CPU Thread Object
//

typedef struct _CPU_THREAD {

    LIST_ENTRY ListEntry;

	LIST_ENTRY ListHead;
	PCPU_THREAD_PC_TABLE Table;
	PCPU_THREAD_PC PcStream;
    PCPU_STACK_TABLE StackTable;
	PCPU_THREAD_STATE SampleState;

    ULONG ThreadId;
    ULONG IdealCpu;
    ULONG Priority;
    ULONG State;

    ULONG Inclusive;
    ULONG Exclusive;

    ULONG KernelTime;
    ULONG UserTime;
    ULONG64 Cycles;
    PVOID Procedure;

	struct _CALL_GRAPH *Graph;
	PVOID Context;

} CPU_THREAD, *PCPU_THREAD;

//
// CPU Utilization History
//

typedef struct _CPU_HISTORY{
	FLOAT *Value;
	FLOAT MinimumValue;
	FLOAT MaximumValue;
	FLOAT Limits; 
    ULONG Count;
} CPU_HISTORY, *PCPU_HISTORY;

#define CPU_THREAD_BUCKET 233

typedef struct _CPU_THREAD_TABLE {

    ULONG64 KernelTime;
    ULONG64 UserTime;

	PCPU_HISTORY History;

    ULONG Count; 
    PCPU_THREAD *Thread;  

    LIST_ENTRY ListHead[CPU_THREAD_BUCKET];

} CPU_THREAD_TABLE, *PCPU_THREAD_TABLE;

typedef enum _CPU_COUNTER_TYPE {
	CPU_COUNTER_ONCPU,
	CPU_COUNTER_ONCPU_THREADED,
	CPU_COUNTER_OFFCPU,
	CPU_COUNTER_OFFCPU_THREADED,
	CPU_COUNTER_MIXED_THREADED,
} CPU_COUNTER_TYPE;

typedef struct _CPU_PC_ENTRY {
	BTR_PC_ENTRY Pc;
	ULONG Count;
	ULONG KernelTime;
	ULONG UserTime;
} CPU_PC_ENTRY, *PCPU_PC_ENTRY;

typedef struct _CPU_COUNTERS {
	CPU_COUNTER_TYPE Type;
	ULONG TotalKernelTime;
	ULONG TotalUserTime;
	ULONG TotalCount;
	ULONG ThreadId;
	ULONG PcCount;
	ULONG AllocationCount;
	ULONG ThreadCount; 
	CPU_PC_ENTRY Pc[ANYSIZE_ARRAY];
} CPU_COUNTERS, *PCPU_COUNTERS;

typedef struct _CPU_PC_STACKTRACE {
	ULONG StackId;
	union {
		ULONG Count;
		ULONG StackTraceCount;
	};
	ULONG KernelTime;
	ULONG UserTime;
} CPU_PC_STACKTRACE, *PCPU_PC_STACKTRACE;

typedef struct _CPU_FUNCTION_ENTRY {
	BTR_FUNCTION_ENTRY Function;
	ULONG Count;
	ULONG KernelTime;
	ULONG UserTime;
} CPU_FUNCTION_ENTRY, *PCPU_FUNCTION_ENTRY;

typedef struct _CPU_FUNCTION_COUNTERS {
	CPU_COUNTER_TYPE Type;
	ULONG TotalKernelTime;
	ULONG TotalUserTime;
	ULONG TotalCount;
	ULONG ThreadId;
	ULONG FunctionCount;
	ULONG AllocationCount;
	CPU_FUNCTION_ENTRY Function[ANYSIZE_ARRAY];
} CPU_FUNCTION_COUNTERS, *PCPU_FUNCTION_COUNTERS;

ULONG
ApsCreateCpuProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	);

ULONG
CpuCreateCounterStreams(
	__in PWSTR Path,
	__in HANDLE ProcessHandle,
	__in PWSTR SymbolPath
	);

ULONG
CpuCheckAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	);

//
// N.B. CpuScanThreadedPc require PC stream available
//

PCPU_THREAD_TABLE
CpuCreateThreadTable(
	VOID
	);

ULONG
CpuScanThreadedPc(
    __in PPF_REPORT_HEAD Head,
    __inout PCPU_THREAD_TABLE Table
    );

PCPU_THREAD
CpuLookupCpuThread(
    __in PCPU_THREAD_TABLE Table,
    __in ULONG ThreadId
    );

VOID
CpuInsertCpuThread(
	__in PCPU_THREAD_TABLE Table,
	__in PCPU_THREAD Thread
	);

PCPU_THREAD
CpuAllocateCpuThread(
	__in ULONG ThreadId
	);

VOID
CpuInsertThreadPc(
	__in PCPU_THREAD Thread,
	__in PVOID Pc,
	__in ULONG Depth
	);

VOID
CpuInsertStackRecord(
    __in PCPU_THREAD Thread,
    __in PBTR_STACK_RECORD Record
    );

ULONG
CpuSortThreads(
    __in PCPU_THREAD_TABLE Table
    );

int __cdecl 
CpuStackTableSortCallback(
	__in const void *Entry1,
	__in const void *Entry2 
	);

VOID
CpuSortStackTable(
    __in PCPU_THREAD Thread
    );

VOID
CpuInsertThreadPcEx(
	__in PCPU_THREAD Thread,
	__in PVOID Pc,
	__in ULONG Depth,
	__in PBTR_PC_TABLE PcTable,
	__in PBTR_DLL_ENTRY DllEntry,
	__in PBTR_FUNCTION_ENTRY FuncEntry,
	__in PBTR_LINE_ENTRY LineEntry
	);

VOID
CpuInsertThreadFunction(
	__in PCPU_THREAD Thread,
	__in PAPS_PC_METRICS Metrics,
	__in ULONG Depth
	);

PBTR_CPU_RECORD
CpuGetNextCpuRecord(
	__in PBTR_CPU_RECORD Record,
	__in ULONG Index
	);

PCPU_THREAD_PC_TABLE
CpuCreateThreadPcTable(
    VOID
    );

PBTR_CPU_RECORD
CpuGetRecordByNumber(
    __in PPF_REPORT_HEAD Head,
    __in ULONG Number
    );

PCPU_STACK_TABLE
CpuCreateThreadStackTable(
    VOID
    );

PBTR_CPU_THREAD
CpuGetBtrThreadObject(
    __in PPF_REPORT_HEAD Head,
	__in ULONG ThreadId
    );

ULONG
CpuGetThreadPriority(
	PPF_REPORT_HEAD Head,
	ULONG ThreadId
	);

BOOLEAN
CpuIsThreadRetired(
	PPF_REPORT_HEAD Head,
	ULONG ThreadId
	);

VOID
CpuCreateThreadStateHistory(
	__in PCPU_THREAD Thread, 
	__in PBTR_CPU_THREAD BtrThread
	);

ULONG
CpuBuildThreadListByRange(
	__in PCPU_THREAD *Thread,
	__in ULONG NumberOfThreads,
	__in ULONG First,
	__in ULONG Last,
	__out PULONG Index
	);

VOID
CpuSetThreadSampleState(
	__in PCPU_THREAD Thread,
	__in ULONG_PTR Pc0,
	__in ULONG_PTR Pc1
	);

ULONG
CpuGetSampleDepth(
	__in PPF_REPORT_HEAD Head
	);

BOOLEAN
CpuComputeUsageByRange(
	__in PPF_REPORT_HEAD Head,
	__in ULONG First,
	__in ULONG Last,
	__out double *CpuMin,
	__out double *CpuMax,
	__out double *Average
	);

double
CpuGetUsageByIndex(
	__in PPF_REPORT_HEAD Head,
	__in ULONG Number
	);

VOID
CpuBuildAddressTable(
	__in PPF_REPORT_HEAD Head
	);

BTR_CPU_STATE_TYPE
CpuPcToThreadState(
	__in ULONG_PTR Pc0,
	__in ULONG_PTR Pc1
	);

VOID
CpuUpdateOnCpuCounter(
	IN PCPU_COUNTERS OnCpu,
	IN PBTR_CPU_SAMPLE Sample,
	IN PBTR_PC_ENTRY PcEntry
	);

ULONG
CpuBuildOnCpuStatistics(
	IN PPF_REPORT_HEAD Head,
	OUT PCPU_COUNTERS* Stat
	);

VOID
CpuUpdateOnFunctionCounter(
	IN PCPU_FUNCTION_COUNTERS OnFunc,
	IN PCPU_PC_ENTRY PcEntry
	);

ULONG
CpuBuildOnCpuStatisticsEx(
	IN PPF_REPORT_HEAD Head,
	IN PCPU_COUNTERS OnCpu,
	OUT PCPU_FUNCTION_COUNTERS* Func
	);

VOID
CpuDumpOnCpuFunction(
	IN PPF_REPORT_HEAD Head,
	IN PCPU_FUNCTION_COUNTERS Func,
	IN PBTR_TEXT_TABLE Table
	);

BOOLEAN
CpuIsMarkRecord(
	IN PBTR_CPU_RECORD Record
	);

VOID
CpuDebugOnCpuStatistics(
	IN PCPU_COUNTERS OnCpu,
	IN PBTR_TEXT_TABLE TextTable
	);

int __cdecl
CpuOnCpuFunctionSortCallback(
	IN const void* Entry1,
	IN const void* Entry2
	);

int __cdecl
CpuOnCpuPcSortCallback(
	IN const void* Entry1,
	IN const void* Entry2
	);

int __cdecl
CpuOnCpuCounterSortCallback(
	IN const void* Entry1,
	IN const void* Entry2
	);

float
CpuComputeOnCpuPercent(
	IN PCPU_COUNTERS OnCpu,
	IN PCPU_PC_ENTRY Pc,
	IN BOOLEAN ByTime
	);

float
CpuComputeFunctionPercent(
	IN PCPU_FUNCTION_COUNTERS OnFunc,
	IN PCPU_FUNCTION_ENTRY Func,
	IN BOOLEAN ByTime
	);

int
CpuGetMarkStep(
	IN PPF_REPORT_HEAD Head
	);

PCPU_COUNTERS
CpuGetCounterPerThread(
	IN PCPU_COUNTERS Counters,
	IN ULONG CounterSize,
	IN ULONG ThreadId
	);

VOID
CpuUpdateOffCpuCounter(
	IN PCPU_COUNTERS Counter,
	IN PBTR_CPU_SAMPLE Sample,
	IN PBTR_PC_ENTRY PcEntry
	);

ULONG
CpuScanOnCpuThreaded(
	IN PPF_REPORT_HEAD Head,
	IN CPU_COUNTER_TYPE Type,
	OUT PCPU_COUNTERS* Stat
	);

PCPU_COUNTERS
CpuGetFirstCounter(
	IN PCPU_COUNTERS Counter
	);

PCPU_COUNTERS
CpuGetNextCounter(
	IN PCPU_COUNTERS Counter,
	IN PCPU_COUNTERS Current
	);

BOOLEAN
CpuIsPcInFunction(
	IN PCPU_FUNCTION_ENTRY Func,
	IN PVOID Pc
	);

PCPU_PC_STACKTRACE
CpuBuildStackTraceList(
	IN PPF_REPORT_HEAD Head,
	IN ULONG ThreadId,
	IN PCPU_FUNCTION_ENTRY Func,
	IN BOOLEAN OnCpu
	);

VOID
CpuInsertPcStackTrace(
	IN PCPU_PC_STACKTRACE StackTrace,
	IN PBTR_STACK_RECORD StackRecord,
	IN PBTR_CPU_SAMPLE Sample
	);

VOID
CpuUpdateOnCpuStackTraceCounter(
	IN PCPU_PC_STACKTRACE StackTrace,
	IN ULONG Limit,
	IN PBTR_CPU_SAMPLE Sample
	);

ULONG
CpuPickOnCpuStackRecord(
	IN PPF_REPORT_HEAD Head,
	OUT PULONG OnCpuCount,
	OUT PBTR_STACK_RECORD* OnCpuRecord
	);

#ifdef __cplusplus 
}
#endif
#endif