//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2011
//

#ifndef _APS_PDB_H_
#define _APS_PDB_H_

#include "apsbtr.h"
#include "apsprofile.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "aps.h"
#include <rpc.h>
#include "list.h"
#include "bitmap.h"

#define STACK_FLAG_DECODED 1

typedef enum _APS_PDB_STREAM {
	PDB_STREAM_RECORD,
	PDB_STREAM_PC,
	PDB_STREAM_FUNCTION,
	PDB_STREAM_DLL,
	PDB_STREAM_TEXT,
	PDB_STREAM_LINE,
	PDB_STREAM_FILE,
	PDB_STREAM_COUNT,
} APS_PDB_STREAM, *PAPS_PDB_STREAM;

typedef struct _APS_STREAM_INFO {
	ULONG64 Offset;
	ULONG64 Length;
} APS_STREAM_INFO, *PAPS_STREAM_INFO;

typedef struct _APS_STACK_FILE {
	UUID Uuid;
	APS_STREAM_INFO Info[PDB_STREAM_COUNT];
} APS_STACK_FILE, *PAPS_STACK_FILE;

typedef struct _STACK_CONTEXT {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	HANDLE ProcessHandle;
} STACK_CONTEXT, *PSTACK_CONTEXT;

typedef struct _APS_PDB_OBJECT {

	HANDLE ProcessHandle;
	ULONG ProcessId;
	ULONG Options;

	HANDLE ReportHandle;
	HANDLE MappingHandle;
	PPF_REPORT_HEAD Head;

	PBTR_DLL_TABLE DllTable;
	PBTR_DLL_FILE DllFile;
	PBTR_STACK_RECORD StackFile;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;

	WCHAR ReportPath[MAX_PATH];
	WCHAR SymbolPath[1024];

} APS_PDB_OBJECT, *PAPS_PDB_OBJECT;

typedef struct _APS_SYMBOL_PATH {
	LIST_ENTRY ListEntry;
	CHAR Path[MAX_PATH];
} APS_SYMBOL_PATH, *PAPS_SYMBOL_PATH;

typedef struct _APS_PDB_ENUM_CONTEXT {
	HANDLE ProcessHandle;
	BTR_DLL_ENTRY Dll;
	MODLOAD_DATA Data;
	BOOLEAN Matched;
	CHAR Path[MAX_PATH];
} APS_PDB_ENUM_CONTEXT, *PAPS_PDB_ENUM_CONTEXT;

//
// Psuedo handle for symbol parse
//

#define ApsPsuedoHandle (HANDLE)('dpfs')

extern PBTR_DLL_TABLE ApsDllTable;
extern APS_PDB_OBJECT ApsPdbObject;
extern LIST_ENTRY ApsSymbolPathList;

//
// Function metrics
//

typedef struct _APS_FUNCTION_METRICS {
    PVOID Address;
    ULONG FunctionId;
    ULONG Exclusive;
    ULONG Inclusive;
} APS_FUNCTION_METRICS, *PAPS_FUNCTION_METRICS;

//
// DLL metrics
//

typedef struct _APS_DLL_METRICS {
	ULONG DllId;
	ULONG Exclusive;
	ULONG Inclusive;
} APS_DLL_METRICS, *PAPS_DLL_METRICS;

//
// PC metrics
//

typedef struct _APS_PC_METRICS {

	PVOID PcAddress;
	ULONG PcLineId;
	ULONG PcInclusive;
	ULONG PcExclusive;
	ULONG Depth;

	PVOID FunctionAddress;
	ULONG FunctionId;
	ULONG FunctionLineId;
	ULONG FunctionInclusive;
	ULONG FunctionExclusive;

	ULONG DllId;
	ULONG DllInclusive;
	ULONG DllExclusive;

} APS_PC_METRICS, *PAPS_PC_METRICS;

ULONG
ApsCreatePdbObject(
	__in HANDLE ProcessHandle,
	__in ULONG ProcessId,
	__in PWSTR SymbolPath,
	__in ULONG Options,
	__in PWSTR ReportPath,
	__out PAPS_PDB_OBJECT *PdbObject
	);

ULONG
ApsInitDbghelp(
	__in HANDLE ProcessHandle,
	__in PWSTR SymbolPath
	);

ULONG
ApsCleanDbghelp(
	__in HANDLE ProcessHandle
	);

ULONG
ApsLoadSymbols(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE ProcessHandle, 
	__in PWSTR SymbolPath
	);

VOID
ApsUnloadAllSymbols(
	__in HANDLE ProcessHandle
	);

ULONG
ApsLoadAllSymbols(
	__in HANDLE ProcessHandle,
	__in PBTR_DLL_FILE DllFile
	);

ULONG
ApsLoadSymbol(
	__in HANDLE ProcessHandle,
	__in PBTR_DLL_ENTRY Dll,
	__in PMODLOAD_DATA Data
	);

VOID
ApsGetSymbolPath(
	__in PWCHAR Buffer,
	__in ULONG Length
	);

PWSTR 
ApsGetDefaultSymbolPath(
	VOID
	);

PWSTR
ApsGetReportPath(
	VOID
	);

VOID
ApsGetCachePath(
	__in PWCHAR Buffer, 
	__in ULONG Length
	);

BOOL CALLBACK 
ApsPdbEnumCallback(
	__in PCSTR FilePath,
	__in PVOID CallerData
	);

ULONG
ApsBuildSymbolPathList(
	__in HANDLE ProcessHandle,
	__out PLIST_ENTRY ListHead
	);

BOOLEAN
ApsOnSymLoadStart(
	IN PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 Image
	);

BOOLEAN
ApsOnSymReadMemory(
	IN PIMAGEHLP_CBA_READ_MEMORY Read
	);

PBTR_DLL_TABLE
ApsCreateDllTable(
	IN PBTR_DLL_FILE File
	);

PBTR_DLL_ENTRY
ApsLookupDllEntry(
	IN PBTR_DLL_TABLE Table,
	IN PVOID BaseVa
	);

VOID
ApsDestroyDllTable(
	IN PBTR_DLL_TABLE Table
	);

BOOL CALLBACK 
ApsDbghelpCallback64(
	IN HANDLE hProcess,
	ULONG ActionCode,
	ULONG64 CallbackData,
	ULONG64 UserContext
	);

VOID
ApsDecodeStackTrace(
	__in PBTR_TEXT_TABLE Table,
	__in PBTR_STACK_RECORD Trace
	);

ULONG
ApsDecodeModuleName(
	IN HANDLE ProcessHandle,
	IN ULONG64 Address,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	);

VOID
ApsAddressToText(
	__in HANDLE ProcessHandle,
	__in ULONG64 Address,
	__out PCHAR Buffer,
	__out ULONG Length,
	__out PULONG ResultLength
	);

VOID
ApsAddressToTextEx(
	__in HANDLE ProcessHandle,
	__in ULONG64 Address,
	__out PCHAR Buffer,
	__out ULONG Length,
	__out PULONG ResultLength
	);

ULONG 
ApsHashStringBKDR(
	IN PSTR Buffer
	);

ULONG
ApsCreateTextTable(
	__in HANDLE ProcessHandle,
	__in ULONG Buckets,
	__out PBTR_TEXT_TABLE *SymbolTable
	);

VOID
ApsDestroySymbolTable(
	__in PBTR_TEXT_TABLE Table
	);

PBTR_TEXT_ENTRY
ApsInsertTextEntry(
	__in PBTR_TEXT_TABLE Table,
	__in ULONG64 Address
	);

VOID
ApsInsertTextEntryEx(
	__in PBTR_TEXT_TABLE Table,
	__in PBTR_TEXT_ENTRY Entry 
	);

PBTR_TEXT_ENTRY
ApsLookupSymbol(
	__in PBTR_TEXT_TABLE Table,
	__in ULONG64 Address
	);

PBTR_TEXT_TABLE
ApsBuildSymbolTable(
	__in PBTR_TEXT_FILE File,
	__in ULONG Buckets
	);

PBTR_TEXT_ENTRY
ApsInsertPseudoTextEntry(
    __in PBTR_TEXT_TABLE Table,
    __in ULONG64 Address 
    );

ULONG
ApsWriteSymbolStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in PBTR_TEXT_TABLE Table,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsDecodeStackStream(
	__in HANDLE ProcessHandle,
	__in PWSTR ReportPath,
	__in PWSTR ImagePath,
	__in PWSTR SymbolPath
	);

ULONG
ApsCreateFunctionTable(
	__out PBTR_FUNCTION_TABLE *FuncTable
	);

ULONG
ApsDestroyFunctionTable(
	__in PBTR_FUNCTION_TABLE Table
	);

ULONG
ApsInsertFunction(
	__in PBTR_FUNCTION_TABLE Table,
	__in ULONG64 Address,
	__in ULONG Inclusive,
	__in ULONG Exclusive,
	__in ULONG DllId
	);

VOID
ApsComputeFunctionMetrics(
    __in PPF_REPORT_HEAD Head,
    __in PBTR_TEXT_TABLE TextTable,
    __in PBTR_FUNCTION_ENTRY Cache 
    );

VOID
ApsUpdateFunctionMetrics(
    __in PBTR_FUNCTION_ENTRY Cache,
    __in PAPS_FUNCTION_METRICS Metrics,
    __in ULONG Maximum
    );

VOID
ApsComputeFunctionMetricsPerPc(
    __in PAPS_FUNCTION_METRICS Metrics,
    __in LONG Maximum,
    __in LONG Depth,
    __in PVOID FunctionPc,
    __in ULONG FunctionId,
    __in ULONG Times
    );

VOID
ApsComputeDllMetrics(
    __in PPF_REPORT_HEAD Head,
    __in PBTR_TEXT_TABLE TextTable,
    __in PBTR_DLL_ENTRY Cache 
    );

VOID
ApsComputeDllMetricsPerPc(
    __in PAPS_DLL_METRICS Metrics,
    __in LONG Maximum,
    __in LONG Depth,
    __in PVOID Pc,
    __in ULONG DllId,
    __in ULONG Times
    );

VOID
ApsUpdateDllMetrics(
    __in PBTR_DLL_ENTRY Cache,
    __in PAPS_DLL_METRICS Metrics,
    __in ULONG Maximum
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
ApsCreateLineTable(
	__out PBTR_LINE_TABLE *LineTable
	);

VOID
ApsDestroyLineTable(
	__in PBTR_LINE_TABLE Table
	);

ULONG
ApsInsertLine(
	__in PBTR_LINE_TABLE Table,
	__in ULONG64 Address,
	__in ULONG Line,
	__in PSTR FileName
	);

ULONG
ApsWriteFunctionStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in PBTR_FUNCTION_TABLE FuncTable,
    __in PBTR_TEXT_TABLE TextTable,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

ULONG
ApsWriteLineStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in PBTR_LINE_TABLE Table,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

PBTR_DLL_ENTRY
ApsGetDllEntryByPc(
	__in PBTR_DLL_FILE DllFile,
	__in PVOID Pc
	);

VOID
ApsDecodePc(
	__in HANDLE ProcessHandle,
	__in PBTR_PC_ENTRY PcEntry,
	__in PBTR_FUNCTION_TABLE FuncTable,
	__in PBTR_DLL_FILE DllFile,
	__in PBTR_TEXT_TABLE TextTable,
	__in PBTR_LINE_TABLE LineTable
	);

VOID
ApsDecodePcByStackTrace(
    __in HANDLE ProcessHandle,
    __in PBTR_STACK_RECORD Record,
	__in PBTR_PC_TABLE PcTable,
    __in PBTR_FUNCTION_TABLE FuncTable,
    __in PBTR_DLL_FILE DllFile,
    __in PBTR_TEXT_TABLE TextTable,
    __in PBTR_LINE_TABLE LineTable,
	__in BTR_PROFILE_TYPE Type
    );

int __cdecl
ApsMetricsSortFunctionCallback(
    __in const void *Ptr1,
    __in const void *Ptr2 
    );

BOOLEAN
ApsIsValidPc(
	__in PVOID Pc
	);

ULONG
ApsCreateStackTable(
	__out PBTR_STACK_TABLE *StackTable
	);

VOID
ApsDestroyStackTable(
	__out PBTR_STACK_TABLE StackTable
	);

PBTR_PC_ENTRY
ApsAllocatePcEntry(
	VOID
	);

#ifdef __cplusplus
}
#endif
#endif