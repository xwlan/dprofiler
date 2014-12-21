//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _DPF_H_
#define _DPF_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#include "sdk.h"
#include "apsprofile.h"
#include "dprofiler.h"
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <assert.h>
#include "ntapi.h"
#include "list.h"

#define PROFILE_MAX_PC 60

typedef struct _CPU_PROFILE_ENTRY {
	LIST_ENTRY ListEntry;
	ULONG_PTR ThreadPc;
	PCHAR PcName;
	ULONG NumberOfSamples;
} CPU_PROFILE_ENTRY, *PCPU_PROFILE_ENTRY;

typedef struct _CPU_PROFILE_SAMPLE {
	LARGE_INTEGER TimeStamp;
	FILETIME KernelTime;
	FILETIME UserTime;
	ULONG ThreadId;
	ULONG Hash;
	ULONG Depth;
	ULONG_PTR Pc[PROFILE_MAX_PC];
} CPU_PROFILE_SAMPLE, *PCPU_PROFILE_SAMPLE;

typedef struct _CPU_PROFILE_HISTORY {
	HANDLE Object;
	PVOID BaseVa;
	SIZE_T Size;
	ULONG SampleCount;
	PCPU_PROFILE_SAMPLE Sample[ANYSIZE_ARRAY];
} CPU_PROFILE_HISTORY, *PCPU_PROFILE_HISTORY;

typedef struct _CPU_PROFILE_INFO {
	ULONG ThreadCount;
	ULONG SampleCount;
	ULONG PcCount;
	ULONG StackCount;
	ULONG ReportSize;
	ULONG SkipCount;
	LARGE_INTEGER TotalTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} CPU_PROFILE_INFO, *PCPU_PROFILE_INFO;

//
// Profile Callback
//

typedef ULONG
(CALLBACK *DPF_PROFILE_CALLBACK)(
	IN PVOID Context,
	IN struct _DPF_PROFILE_INFO *Info
	);

//
// Analyze Callback
//

typedef VOID
(CALLBACK *DPF_ANALYZE_CALLBACK)(
	IN PVOID Context,
	IN ULONG Percent
	);

typedef struct _CPU_PROFILE_OBJECT {

	ULONG ProcessId;
	HANDLE ProcessHandle;
	WCHAR BaseName[MAX_PATH];
	WCHAR FullPath[MAX_PATH];

	LARGE_INTEGER CreationTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER TotalTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;

	ULONG SamplingPeriod;
	ULONG StackDepth;
	BOOLEAN IncludeWait;

	HANDLE StartEvent;
	HANDLE StopEvent;
	HANDLE ThreadHandle;
	ULONG ExitStatus;
	BOOLEAN Paused;

	//
	// N.B. To elimiate scan complexity, we should ensure
	// we only scan the active thread list once in O(n)
	//

	ULONG SamplesDepth;
	ULONG FuzzyThreshold;

	PPC_DATABASE Pc;
	PDLL_DATABASE Dll;
	PTHREAD_DATABASE Thread;

	DPF_PROFILE_CALLBACK Callback;
	PVOID CallbackContext;
	struct _DPF_PROFILE_INFO *ProfileInfo;

} CPU_PROFILE_OBJECT, *PCPU_PROFILE_OBJECT;

typedef struct _MM_PROFILE_INFO {

	ULONG PrivateBytes;
	ULONG VirtualBytes;
	ULONG KernelHandles;
	ULONG GdiHandles;

	ULONG NumberOfHeapAllocs;
	ULONG NumberOfHeapFrees;
	ULONG64 SizeOfHeapAllocs;
	ULONG64 SizeOfHeapFrees;

	ULONG NumberOfPageAllocs;
	ULONG NumberOfPageFrees;
	ULONG64 SizeOfPageAllocs;
	ULONG64 SizeOfPageFrees;

	ULONG NumberOfHandleAllocs;
	ULONG NumberOfHandleFrees;

	ULONG NumberOfGdiAllocs;
	ULONG NumberOfGdiFrees;

} MM_PROFILE_INFO, *PMM_PROFILE_INFO;

typedef struct _MM_PROFILE_OBJECT {

	PBTR_PROFILE_OBJECT BtrObject;

	ULONG ProcessId;
	HANDLE ProcessHandle;
	WCHAR BaseName[MAX_PATH];
	WCHAR FullPath[MAX_PATH];

	LARGE_INTEGER CreationTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER TotalTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;

	HANDLE StartEvent;
	HANDLE StopEvent;
	ULONG ExitStatus;

	//
	// Profile Callback
	//

	DPF_PROFILE_CALLBACK Callback;
	PVOID CallbackContext;
	PULONG AnalyzePercent;

} MM_PROFILE_OBJECT, *PMM_PROFILE_OBJECT;

typedef struct _IO_PROFILE_INFO {
	ULONG NumberOfIoRead;
	ULONG NumberOfIoWrite;
	ULONG NumberOfIoCtl;
	ULONG64 SizeOfIoRead;
	ULONG64 SizeOfIoWrite;
	ULONG64 SizeOfIoCtl;
} IO_PROFILE_INFO, *PIO_PROFILE_INFO;

typedef struct _IO_PROFILE_OBJECT {

	ULONG ProcessId;
	HANDLE ProcessHandle;
	WCHAR BaseName[MAX_PATH];
	WCHAR FullPath[MAX_PATH];

	LARGE_INTEGER CreationTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER TotalTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;

	HANDLE StartEvent;
	HANDLE StopEvent;
	HANDLE ThreadHandle;
	ULONG ExitStatus;

	ULONG NumberOfIoRead;
	ULONG NumberOfIoWrite;
	ULONG NumberOfIoCtl;

	ULONG64 SizeOfIoRead;
	ULONG64 SizeOfIoWrite;
	ULONG64 SizeOfIoCtl;

	//
	// File mapping of target process,
	// mapped as a DPF_PROFILE_INFO structure
	//

	HANDLE CounterObject;
	union _BTR_PROFILER_INFO *MappedInfo;

	DPF_PROFILE_CALLBACK Callback;
	PVOID CallbackContext;
	struct _DPF_PROFILE_INFO *ProfileInfo;

	PULONG AnalyzePercent;

} IO_PROFILE_OBJECT, *PIO_PROFILE_OBJECT;

typedef struct _DPF_PROFILE_INFO {

	BTR_PROFILE_TYPE Type;

	ULONG64 Overhead;
	ULONG64 KernelTime;
	ULONG64 UserTime;

	ULONG64 TotalOverhead;
	ULONG64 TotalKernelTime;
	ULONG64 TotalUserTime;

	double OverheadPercent;
	double TotalOverheadPercent;

	union {
		CPU_PROFILE_INFO CpuInfo;
		MM_PROFILE_INFO  MmInfo;
		IO_PROFILE_INFO  IoInfo;
	};

} DPF_PROFILE_INFO, *PDFP_PROFILE_INFO;

typedef struct _DPF_PROFILE_CONFIG {

	BTR_PROFILE_TYPE Type;

	union {

		struct {
			ULONG SamplingPeriod;
			ULONG StackDepth;
			BOOLEAN IncludeWait;
		} Cpu;

		struct {
			ULONG EnableHeap   : 1;
			ULONG EnablePage   : 1;
			ULONG EnableHandle : 1;
			ULONG EnableGdi    : 1;
			ULONG Measure      : 1;
			ULONG Distribution : 1;
		} Mm;

		struct {
			ULONG EnableNet    : 1;
			ULONG EnableDisk   : 1;
			ULONG EnablePipe   : 1;
			ULONG EnableDevice : 1;
		} Io;
	};

	BOOLEAN InitialPause;
	BOOLEAN Attach;
	PBSP_PROCESS Process;

	WCHAR ImagePath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];

} DPF_PROFILE_CONFIG, *PDPF_PROFILE_CONFIG;

//
// CPU profile routines
//

ULONG
CpuCreateProfile(
	IN ULONG ProcessId,
	IN PWSTR Path,
	IN PWSTR BaseName,
	IN ULONG Period,
	IN ULONG StackDepth,
	IN BOOLEAN IncludeWait,
	OUT PCPU_PROFILE_OBJECT *Object
	);

ULONG
CpuStartProfile(
	IN PCPU_PROFILE_OBJECT Object
	);

ULONG
CpuStopProfile(
	IN PCPU_PROFILE_OBJECT Object
	);

VOID
CpuRegisterCallback(
	IN PCPU_PROFILE_OBJECT Object,
	IN DPF_PROFILE_CALLBACK Callback,
	IN PVOID Context,
	IN struct _DPF_PROFILE_INFO *Info
	);

ULONG CALLBACK
CpuProfileProcedure(
	IN PVOID Context
	);

ULONG
CpuSampleTarget(
	IN PCPU_PROFILE_OBJECT Object
	);

ULONG
CpuCaptureStackTrace(
	IN PCPU_PROFILE_OBJECT Object,
	IN PSYSTEM_THREAD_INFORMATION Info
	);

PTHREAD_ENTRY
CpuDeQueueThread(
	IN PCPU_PROFILE_OBJECT Object,
	IN PSYSTEM_THREAD_INFORMATION Info,
	OUT BOOLEAN *First
	);

ULONG
CpuInsertStackTrace(
	IN PCPU_PROFILE_OBJECT Object,
	IN PCPU_PROFILE_SAMPLE Sample
	);

ULONG
CpuInsertPc(
	IN PCPU_PROFILE_OBJECT Object,
	IN PCPU_PROFILE_SAMPLE Sample
	);

//
// MM profile routines
//

ULONG
DpfCreateMmProfile(
	IN struct _WIZARD_CONTEXT *Context,
	IN PWSTR BaseName,
	IN PWSTR FullPath,
	IN ULONG ProcessId,
	OUT PMM_PROFILE_OBJECT *Object
	);

ULONG
DpfStopMmProfile(
	IN PMM_PROFILE_OBJECT Object
	);

VOID
DpfRegisterMmCallback(
	IN PMM_PROFILE_OBJECT Object,
	IN DPF_PROFILE_CALLBACK Callback,
	IN PVOID Context,
	IN struct _DPF_PROFILE_INFO *Info
	);

ULONG
DpfStartMmProfile(
	IN PMM_PROFILE_OBJECT Object
	);

VOID
DpfMmAnalyzeReport(
	IN PMM_PROFILE_OBJECT Profile
	);

ULONG
DpfUpdateMmCounter(
	IN PMM_PROFILE_OBJECT Object
	);

ULONG CALLBACK
DpfMmProfileProcedure(
	IN PVOID Context
	);

ULONG CALLBACK
DpfMmAnalyzeProcedure(
	IN PVOID Context
	);

//
// DPF shared routines
//

VOID
DpfBuildConfigFromWizard(
	IN struct _WIZARD_CONTEXT *Context,
	IN PDPF_PROFILE_CONFIG Config
	);

ULONG
DpfInitDbghelp(
	IN HANDLE Handle	
	);

VOID
DpfUninitDbghelp(
	IN HANDLE Handle	
	);

#ifdef __cplusplus
}
#endif
#endif