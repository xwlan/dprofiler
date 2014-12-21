//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _APS_ANALYSIS_H_
#define _APS_ANALYSIS_H_

#include "aps.h"
#include "apspdb.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Analysis Option
//

typedef enum _APS_ANALYSIS_OPTION {
    ANALYSIS_CREATE_COUNTERS = 0x00000001, 
    ANALYSIS_RELOAD_SYMBOL   = 0x00000002,
} APS_ANALYSIS_OPTION;

//
// Analysis Context
//

typedef struct _APS_ANALYSIS_CONTEXT {
    BTR_PROFILE_TYPE Type;
    APS_ANALYSIS_OPTION Option;
    APS_CALLBACK Callback;
    PWSTR ReportPath;
    PWSTR SymbolPath;
} APS_ANALYSIS_CONTEXT, *PAPS_ANALYSIS_CONTEXT;

//
// Analysis Procedure
//

ULONG CALLBACK
ApsAnalysisProcedure(
    __in PVOID Argument 
    );

ULONG
ApsCreatePcStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *Table
	);

VOID
ApsNullDuplicatedPc(
    __in PVOID *Source,
    __in LONG Depth,
    __in PVOID *Destine
    );

ULONG
ApsDeduplicateFunction(
	__in PBTR_STACK_RECORD Record,
	__in PAPS_PC_METRICS Metrics,
	__in PBTR_PC_TABLE PcTable,
	__in PBTR_FUNCTION_ENTRY FuncTable,
	__in PBTR_TEXT_TABLE TextTable,
	__out PULONG_PTR Pc0,
	__out PULONG_PTR Pc1
	);

ULONG
ApsWriteAnalysisStreams(
	__in PWSTR Path,
	__in HANDLE ProcessHandle,
	__in PWSTR SymbolPath
	);

ULONG
ApsWriteAnalysisStreamsEx(
	__in PWSTR Path,
	__in HANDLE ProcessHandle,
	__in PWSTR SymbolPath,
	__in BTR_PROFILE_TYPE Type
	);

ULONG
ApsMapReportFile(
	__in PWSTR Path,
	__out PPF_REPORT_HEAD *Head,
	__out PHANDLE MappingObject,
	__out PHANDLE FileObject
	);

VOID
ApsUnmapReportFile(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE MappingObject,
	__in HANDLE FileObject
	);

ULONG
ApsWriteCpuCounterStreams(
	__in PWSTR Path
	);

ULONG
ApsWriteMmCounterStreams(
	__in PWSTR Path
	);

#ifdef __cplusplus
}
#endif

#endif  // _APS_ANALYSIS_H_