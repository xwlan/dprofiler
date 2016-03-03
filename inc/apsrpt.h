//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _APS_RPT_H_
#define _APS_RPT_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "apsbtr.h"
#include "apsdefs.h"
#include "apsprofile.h"
#include "apsctl.h"

//
// Profiler report file context
//

typedef struct _APS_FILE_CONTEXT {
	HANDLE FileObject;
	HANDLE MappingObject;
	PPF_REPORT_HEAD Head;
	WCHAR Path[MAX_PATH];
} APS_FILE_CONTEXT, *PAPS_FILE_CONTEXT;

ULONG
ApsWriteReport(
	__in PAPS_PROFILE_OBJECT Profile
	);

ULONG
ApsWriteSystemStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_STREAM_SYSTEM System
	);

VOID
ApsComputeStreamCheckSum(
	__in PPF_REPORT_HEAD Head
	);

ULONG
ApsWriteMinidump(
	__in HANDLE ProcessHandle,
	__in ULONG ProcessId,
	__in PWSTR Path,
	__in BOOLEAN Full,
	__out PHANDLE DumpHandle 
	);

ULONG
ApsWriteStackStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in HANDLE StackHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteDllStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteIndexStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in HANDLE IndexHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteRecordStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in HANDLE DataHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteCoreDllStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsCreateCounterStreams(
	__in PWSTR Path
	);

ULONG
ApsCreatePcTable(
	__out PBTR_PC_TABLE *PcTable
	);

ULONG
ApsDestroyPcTable(
	__in PBTR_PC_TABLE PcTable
	);

ULONG
ApsCreatePcTableFromStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *PcTable
	);

BOOLEAN
ApsHasDuplicatedPc(
	_In_ PLIST_ENTRY ListHead,
	_In_ PVOID Address
	);

//
// N.B. This routine is required by MM profile
//

ULONG
ApsCreateCallerTableFromStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *PcTable
	);

VOID
ApsDestroyPcTableFromStream(
	__in PBTR_PC_TABLE PcTable
	);

PBTR_PC_ENTRY
ApsLookupPcEntry(
	__in PVOID Address,
	__in PBTR_PC_TABLE PcTable
	);

ULONG
ApsCreateCpuPcStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *Table
	);

ULONG
ApsCreateMmPcStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *Table
	);

VOID
ApsInsertPcEntry(
	__in PBTR_PC_TABLE Table,
	__in PVOID Address,
	__in ULONG Count,
	__in ULONG Inclusive,
	__in ULONG64 Size
	);

VOID
ApsInsertPcEntryEx(
	__in PBTR_PC_TABLE Table,
	__in PBTR_PC_ENTRY PcEntry,
    __in ULONG Times,
	__in ULONG Depth,
	__in BTR_PROFILE_TYPE Type
	);

ULONG
ApsWriteCallerStream(
	__in PPF_REPORT_HEAD Head,
	__in PBTR_PC_TABLE Table,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWritePcStream(
	__in PPF_REPORT_HEAD Head,
	__in PBTR_PC_TABLE Table,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWritePcStreamEx(
	__in PPF_REPORT_HEAD Head,
	__in PBTR_PC_TABLE Table,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsGetDllIdByAddress(
	__in PBTR_DLL_FILE DllFile,
	__in ULONG64 Address
	);

int __cdecl
ApsPcCompareCallback(
	__in const void *Ptr1,
	__in const void *Ptr2 
	);

int __cdecl
ApsStackCompareCallback(
	__in const void *Ptr1,
	__in const void *Ptr2 
	);

int __cdecl
ApsStackIdCompareCallback(
	__in const void *Ptr1,
	__in const void *Ptr2 
	);

PVOID
ApsGetStreamPointer(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
	);

ULONG
ApsGetStreamLength(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
	);

BOOLEAN
ApsIsStreamValid(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
	);

#define ApsGetStreamRecordCount(_H, _S, _T) \
	(ULONG)(ApsGetStreamLength(_H, _S) / sizeof(_T))


PBTR_LEAK_TABLE
ApsCreateLeakTable(
	VOID
	);

PBTR_LEAK_FILE
ApsCreateLeakFile(
	__in PBTR_LEAK_TABLE Table
	);

VOID
ApsInsertLeakEntry(
	__in PBTR_LEAK_TABLE Table,
	__in PVOID Ptr,
	__in ULONG Size,
	__in ULONG Allocator,
	__in ULONG StackId
	);

PVOID
ApsGetStreamPointer(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
	);

ULONG
ApsWriteHeapLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWritePageLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteHandleLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteGdiLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsCreateHotStackStream(
	IN PPF_REPORT_HEAD Head
	);

ULONG
ApsUpdateHotHeapCounters(
	IN PPF_REPORT_HEAD Head
	);

VOID
ApsClearHotHeapCounters(
	__in PPF_REPORT_HEAD Head
	);

ULONG
ApsCreateHeapLeakStream(
	IN PPF_REPORT_HEAD Head
	);

ULONG
ApsCreatePageLeakStream(
	IN PPF_REPORT_HEAD Head
	);

ULONG
ApsCreateHandleLeakStream(
	IN PPF_REPORT_HEAD Head
	);

ULONG
ApsCreateGdiLeakStream(
	IN PPF_REPORT_HEAD Head
	);

ULONG
ApsOpenReport(
	__in PWSTR ReportPath,
	__out PHANDLE FileObject,
	__out PHANDLE MappingObject,
	__out PPF_REPORT_HEAD *Report
	);

ULONG
ApsReadReportHead(
	__in PWSTR ReportPath,
	__out PPF_REPORT_HEAD Head 
	);

ULONG
ApsVerifyReportHead(
    __in PPF_REPORT_HEAD Head
    );

ULONG
ApsPrepareForAnalysis(
	__in PWSTR ReportPath
	);

//
// Debug Routines
//

ULONG
ApsDebugValidateReport(
	__in PWSTR Path
	);

ULONG
ApsDebugValidateReportEx(
	__in PPF_REPORT_HEAD Head,
	__in PWSTR Path
	);

ULONG
ApsDebugDumpDll(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	);

VOID
ApsDebugDumpBackTrace(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	);

ULONG
ApsDebugDumpHeapLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	);

ULONG
ApsDebugDumpPageLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	);

ULONG
ApsDebugDumpHandleLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	);

ULONG
ApsDebugDumpGdiLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	);

//
// Helper Routines
//

SIZE_T
ApsBuildReportFullName(
	__in PWSTR ProcessName,
	__in ULONG ProcessId,
	__in BTR_PROFILE_TYPE Type,
	__out PWCHAR Name
	);

VOID
ApsSetReportName(
	__in PWSTR Name
	);

PWSTR
ApsGetCurrentReportBaseName(
	VOID
	);

PWSTR
ApsGetCurrentReportFullName(
	VOID
	);

VOID
ApsCleanReportBaseName(
	VOID
	);

VOID
ApsCleanReportFullName(
	VOID
	);

VOID
ApsCleanReport(
	VOID
	);

PBTR_DLL_ENTRY
ApsGetDllEntryById(
    __in PPF_REPORT_HEAD Head,
    __in ULONG Id
    );

VOID
ApsGetDllBaseNameById(
    __in PPF_REPORT_HEAD Head,
    __in ULONG Id,
    __out PWSTR Buffer,
    __in ULONG Length
    );

VOID
ApsClearDllFileCounters(
    __in PBTR_DLL_FILE DllFile
    );

VOID
ApsInitializeDllFile(
    __in PBTR_DLL_FILE DllFile
    );

ULONG
ApsGetSampleCount(
	__in PPF_REPORT_HEAD Head
	);

VOID
ApsGetCpuSampleCounters(
	__in PPF_REPORT_HEAD Head,
	__out PULONG Inclusive,
	__out PULONG Exclusive
	);

SIZE_T
ApsGetBaseName(
	__in PWSTR FullPath,
	__out PWSTR BaseName,
	__in SIZE_T Length
	);

HANDLE
ApsOpenSourceFile(
	__in PWSTR SourceFolder,
	__in PWSTR SpecifiedPath,
	__out PWSTR *RealPath
	);

BOOLEAN
ApsIsProcessLive(
	__in HANDLE ProcessHandle
	);

VOID
ApsInitReportContext(
	__in PPF_REPORT_HEAD Head
	);


extern PWSTR ApsCurrentReportBaseName;
extern PWSTR ApsCurrentReportFullName;

#ifdef __cplusplus 
}
#endif
#endif
