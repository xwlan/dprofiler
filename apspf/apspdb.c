//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#include "apsdefs.h"
#include "apspri.h"
#include "apsbtr.h"
#include "apspdb.h"
#include "apsfile.h"
#include "apsprofile.h"
#include "aps.h"
#include "status.h"
#include <dbghelp.h>
#include "apsrpt.h"
#include "aps.h"
#include <stdlib.h>

APS_PDB_OBJECT ApsPdbObject;
PBTR_DLL_FILE ApsDllFile;
PBTR_DLL_TABLE ApsDllTable;
LIST_ENTRY ApsSymbolPathList;
PWSTR ApsSymbolPath;

PWSTR ApsDefaultSymbolPath = NULL;
PWSTR ApsReportPath = NULL;
PWSTR ApsCurrentReportBaseName = NULL; 
PWSTR ApsCurrentReportFullName = NULL;

ULONG
ApsCreatePdbObject(
    __in HANDLE ProcessHandle,
    __in ULONG ProcessId,
    __in PWSTR SymbolPath,
    __in ULONG Options,
    __in PWSTR ReportPath,
    __out PAPS_PDB_OBJECT *PdbObject
    )
{
    ULONG Status;
    PAPS_PDB_OBJECT Object;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PPF_REPORT_HEAD Head;

    InitializeListHead(&ApsSymbolPathList);

    Status = ApsInitDbghelp(ProcessHandle, SymbolPath);
    if (Status != APS_STATUS_OK) {
        return Status;
    }

    Object = (PAPS_PDB_OBJECT)ApsMalloc(sizeof(APS_PDB_OBJECT));
    RtlZeroMemory(Object, sizeof(*Object));

    Object->ProcessHandle = ProcessHandle;
    Object->ProcessId = ProcessId;
    Object->Options = Options;
    StringCchCopy(Object->ReportPath, MAX_PATH, ReportPath);
    StringCchCopy(Object->SymbolPath, 1024, SymbolPath);

    //
    // Open profile report file and map stack record, generate symbol
    // if necessary
    //

    __try {

        FileHandle = CreateFile(ReportPath, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (FileHandle == INVALID_HANDLE_VALUE) {
            Status = GetLastError();
            __leave;
        }	

        MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, 0);
        if (!MappingHandle) {
            Status = GetLastError();
            __leave;
        }

        Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE,
                                              0, 0, 0); 
        if (!Head) {
            Status = GetLastError();
            __leave;
        }

        Object->StackFile = (PBTR_STACK_RECORD)((PUCHAR)Head + Head->Streams[STREAM_STACK].Offset);

        //
        // Create dll table, dll table is for fast hash lookup, dll file is for
        // slow path dll lookup, e.g. when pdb for dll is missing we have to walk
        // the dll file to determine whether a given address falls into dll range.
        //

        Object->DllFile = (PBTR_DLL_FILE)((PUCHAR)Head + Head->Streams[STREAM_DLL].Offset);
        Object->DllTable = ApsCreateDllTable(Object->DllFile);

        if (Head->Streams[STREAM_SYMBOL].Offset != 0 && Head->Streams[STREAM_SYMBOL].Length != 0) {
            Object->TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);
            Object->TextTable = ApsBuildSymbolTable(Object->TextFile, 4093);
        }

        Object->ReportHandle = FileHandle;
        Object->MappingHandle = MappingHandle;
        Object->Head = Head;
    }
    __finally {

        if (Status != APS_STATUS_OK) {

            if (Head != NULL) {
                UnmapViewOfFile(Head);
            }

            if (MappingHandle != NULL) {
                CloseHandle(MappingHandle);
            }

            if (FileHandle != NULL) {
                CloseHandle(FileHandle);
            }

            if (Object != NULL) {
                ApsFree(Object);
            }

            *PdbObject = NULL;

        } else {

            *PdbObject = Object;
        }
    }

    return Status;
}

ULONG
ApsInitDbghelp(
    __in HANDLE ProcessHandle,
    __in PWSTR SymbolPath
    )
{
    ULONG Status;
    SIZE_T Size;

    InitializeListHead(&ApsSymbolPathList);

    //
    // N.B. If ProcessHandle is ApsPsuedoHandle, it's postmortem debugging,
    // if it's a valid process handle, it's live debugging.
    //

    Status = SymInitialize(ProcessHandle, NULL, FALSE);
    if (!Status) {
        return GetLastError();
    }

    //
    // The following options are copied from dbgeng.dll
    //

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | 
                  SYMOPT_OMAP_FIND_NEAREST | SYMOPT_DEFERRED_LOADS);

    SymSetSearchPathW(ProcessHandle, SymbolPath);

    Size = sizeof(WCHAR) * (wcslen(SymbolPath) + 1);
    ApsSymbolPath = (PWSTR)ApsMalloc(Size);
    memcpy(ApsSymbolPath, SymbolPath, Size);

    ApsBuildSymbolPathList(ProcessHandle, &ApsSymbolPathList);
    return APS_STATUS_OK;
}

VOID
ApsGetSymbolPath(
    __in PWCHAR Buffer,
    __in ULONG Length
    )
{
    WCHAR Path[MAX_PATH];
    WCHAR SymbolPath[MAX_PATH * 2];

    Path[0] = 0;
    SymbolPath[0] = 0;

    GetEnvironmentVariable(L"_NT_SYMBOL_PATH", SymbolPath, MAX_PATH);
    if (!wcslen(SymbolPath)) {

        //
        // _NT_SYMBOL_PATH is not set, use dprobe private symbol path instead
        //

        GetCurrentDirectory(MAX_PATH, Path);
        StringCchPrintf(SymbolPath, MAX_PATH * 2, 
                        L"%s\\sym;SRV*%s\\sym*http://msdl.microsoft.com/download/symbols", 
                        Path, Path);
    }

    StringCchCopy(Buffer, Length, SymbolPath);
}

//
// N.B. Symbol path can be much longer than MAX_PATH, dynamically allocated
// string is required to hold its value
//

PWSTR
ApsGetDefaultSymbolPath(
    VOID
    )
{
    WCHAR Buffer[MAX_PATH];
    ULONG Length;

    if (ApsDefaultSymbolPath != NULL) {
        return ApsDefaultSymbolPath;
    }

    //
    // Check whether _NT_SYMBOL_PATH is available
    //

    Length = GetEnvironmentVariable(L"_NT_SYMBOL_PATH", Buffer, MAX_PATH);
    if (!Length || GetLastError() == ERROR_ENVVAR_NOT_FOUND) {

        //
        // If failed or _NT_SYMBOL_PATH not found in environment variables, use
        // profiler own's local symbol store
        //

        GetCurrentDirectory(MAX_PATH, Buffer);
        ApsDefaultSymbolPath = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR) * 3);

        StringCchPrintf(ApsDefaultSymbolPath, MAX_PATH * 3, 
                        L"%s;SRV*%s\\sym*http://msdl.microsoft.com/download/symbols", 
                        Buffer, Buffer);

        return ApsDefaultSymbolPath;
    } 

    if (Length < MAX_PATH) { 
        Length += 1;	
    }

    ApsDefaultSymbolPath = (PWSTR)ApsMalloc(Length * sizeof(WCHAR));
    GetEnvironmentVariable(L"_NT_SYMBOL_PATH", ApsDefaultSymbolPath, Length);

    return ApsDefaultSymbolPath;
}

VOID
ApsGetCachePath(
    __in PWCHAR Buffer, 
    __in ULONG Length
    )
{
    WCHAR Path[MAX_PATH];

    GetCurrentDirectory(MAX_PATH, Path);
    StringCchPrintf(Buffer, Length, L"%s\\log", Path);
}

PWSTR
ApsGetReportPath(
    VOID
    )
{
    WCHAR Path[MAX_PATH];

    if (ApsReportPath != NULL) {
        return ApsReportPath;
    }

    GetCurrentDirectory(MAX_PATH, Path);
    ApsReportPath = (PWSTR)ApsMalloc(MAX_PATH);
    StringCchPrintf(ApsReportPath, MAX_PATH, L"%s\\log", Path);

    return ApsReportPath;
}

ULONG
ApsLoadSymbols(
    __in PPF_REPORT_HEAD Head,
    __in HANDLE ProcessHandle, 
    __in PWSTR SymbolPath
    )
{
    ULONG Status;
    PBTR_DLL_FILE DllFile;

    Status = ApsInitDbghelp(ProcessHandle, SymbolPath); 
    if (Status != APS_STATUS_OK) {
        return Status;
    }

    DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
    ApsLoadAllSymbols(ProcessHandle, DllFile);

    return APS_STATUS_OK;
}

VOID
ApsUnloadAllSymbols(
    __in HANDLE ProcessHandle
    )
{
    ApsCleanDbghelp(ProcessHandle);
}

ULONG
ApsLoadAllSymbols(
    IN HANDLE ProcessHandle,
    IN PBTR_DLL_FILE DllFile
    )
{
    PBTR_DLL_ENTRY Dll;
    WCHAR Buffer[MAX_PATH];
    ULONG i;
    DWORD64 Address;
    MODLOAD_DATA Data;
    IMAGEHLP_MODULE64 Info;
    ULONG Status;

    ApsDllFile = DllFile;

    for(i = 0; i < DllFile->Count; i++) {

        Dll = &DllFile->Dll[i];
        _wsplitpath(Dll->Path, NULL, NULL, Buffer, NULL);

        Data.ssize = sizeof(Data);
        Data.ssig = 2; // DBHHEADER_CVMISC
        Data.data = &Dll->CvRecord;
        Data.size = (DWORD)(Dll->CvRecord.CvRecordOffset + Dll->CvRecord.SizeOfCvRecord);
        Data.flags = 0;

        Address = SymLoadModuleExW(ProcessHandle, NULL, Buffer, Buffer, 
                                   (DWORD64)Dll->BaseVa, (DWORD)Dll->Size, &Data, 0);

        Info.SizeOfStruct = sizeof(Info);
        SymGetModuleInfo64(ProcessHandle, Address, &Info);

        if (strlen(Info.LoadedPdbName) == 0) {

            //
            // N.B. We must unload the module
            //

            SymUnloadModule64(ProcessHandle, Dll->BaseVa);

            //
            // Enumerate all possible symbol path to find pdb, note that
            // this is because typically users do not have an indexed
            // symbol store for their own private pdb files, we have to
            // enumerate the path to find them.
            //

            Status = ApsLoadSymbol(ProcessHandle, Dll, &Data);
            if (Status != APS_STATUS_OK) {

                SymLoadModuleExW(ProcessHandle, NULL, Buffer, Buffer, 
                                (DWORD64)Dll->BaseVa, (DWORD)Dll->Size, &Data, 0);

                ApsDebugTrace("failed to load symbol for %S", Dll->Path);

            } else {

                Info.SizeOfStruct = sizeof(Info);
                SymGetModuleInfo64(ProcessHandle, Address, &Info);

                ApsDebugTrace("loaded symbol for %s, %S", Dll->Path, Info.LoadedPdbName);
            }
        }
    }

    return 0;
}

PBTR_DLL_TABLE
ApsCreateDllTable(
    IN PBTR_DLL_FILE DllFile
    )
{
    PBTR_DLL_TABLE Table;
    PBTR_DLL_ENTRY Dll;
    ULONG i;
    ULONG Bucket;

    Table = (PBTR_DLL_TABLE)ApsMalloc(sizeof(BTR_DLL_TABLE));
    for(i = 0; i < STACK_DLL_BUCKET; i++) {
        ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
        InitializeListHead(&Table->Hash[i].ListHead);
    }

    //
    // Insert all dll entry into hash table
    //

    for(i = 0; i < DllFile->Count; i++) {

        Dll = (PBTR_DLL_ENTRY)ApsMalloc(sizeof(BTR_DLL_ENTRY));
        RtlCopyMemory(Dll, &DllFile->Dll[i], sizeof(BTR_DLL_ENTRY));

        Bucket = STACK_DLL_HASH(Dll->BaseVa);
        InsertHeadList(&Table->Hash[Bucket].ListHead, &Dll->ListEntry);

    }

    Table->Count = DllFile->Count;
    return Table;
}

VOID
ApsDestroyDllTable(
    IN PBTR_DLL_TABLE Table
    )
{
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    PBTR_DLL_ENTRY Dll;
    ULONG i;

    for(i = 0; i < STACK_DLL_BUCKET; i++) {
        ListHead = &Table->Hash[i].ListHead;
        while (IsListEmpty(ListHead) != TRUE) {
            ListEntry = RemoveHeadList(ListHead);
            Dll = CONTAINING_RECORD(ListEntry, BTR_DLL_ENTRY, ListEntry);
            ApsFree(Dll);
        }
    }
}

BOOL CALLBACK 
ApsDbghelpCallback64(
    IN HANDLE hProcess,
    ULONG ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    )
{
    BOOL Status = FALSE;

    switch (ActionCode) {

    case CBA_DEBUG_INFO:
        ApsDebugTrace("CBA_DEBUG_INFO: %s", (PSTR)CallbackData);
        Status = TRUE;
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        ApsDebugTrace("CBA_DEFERRED_SYMBOL_LOAD_CANCEL"); 
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
        ApsDebugTrace("CBA_DEFERRED_SYMBOL_LOAD_COMPLETE: %s",  
            ((PIMAGEHLP_DEFERRED_SYMBOL_LOAD64)CallbackData)->FileName);
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
        ApsDebugTrace("CBA_DEFERRED_SYMBOL_LOAD_FAILURE: %s", 
            ((PIMAGEHLP_DEFERRED_SYMBOL_LOAD64)CallbackData)->FileName);
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_PARTIAL:
        ApsDebugTrace("CBA_DEFERRED_SYMBOL_LOAD_PARTIAL: %s",
            ((PIMAGEHLP_DEFERRED_SYMBOL_LOAD64)CallbackData)->FileName);
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_START:
        ApsDebugTrace("CBA_DEFERRED_SYMBOL_LOAD_START: %s",
            ((PIMAGEHLP_DEFERRED_SYMBOL_LOAD64)CallbackData)->FileName);
        Status = ApsOnSymLoadStart((PIMAGEHLP_DEFERRED_SYMBOL_LOAD64)CallbackData);
        break;

    case CBA_EVENT:
        ApsDebugTrace("CBA_EVENT");
        break;

    case CBA_READ_MEMORY:
        ApsDebugTrace("CBA_READ_MEMORY");
        Status = ApsOnSymReadMemory((PIMAGEHLP_CBA_READ_MEMORY)CallbackData);
        break;

    case CBA_SET_OPTIONS:
        ApsDebugTrace("CBA_SET_OPTIONS");
        break;

    case CBA_SYMBOLS_UNLOADED:
        ApsDebugTrace("CBA_SYMBOLS_UNLOADED");
        break;

    }

    return Status;
}

BOOLEAN
ApsOnSymLoadStart(
    IN PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 Image
    )
{
    PBTR_DLL_ENTRY Dll;

    Dll = ApsLookupDllEntry(ApsDllTable, (PVOID)Image->BaseOfImage);
    if (!Dll) {
        Image->Reparse = FALSE;
        return FALSE;
    }

    if (Dll->CheckSum == Image->CheckSum && Dll->Timestamp == Image->TimeDateStamp) {
        Image->Reparse = FALSE;
        return TRUE;
    }

    Image->Reparse = TRUE;
    return FALSE;
}

BOOLEAN
ApsOnSymReadMemory(
    IN PIMAGEHLP_CBA_READ_MEMORY Read
    )
{
    return FALSE;
}

PBTR_DLL_ENTRY
ApsLookupDllEntry(
    IN PBTR_DLL_TABLE Table,
    IN PVOID BaseVa
    )
{
    ULONG Bucket;
    PBTR_DLL_ENTRY Dll;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;

    Bucket = STACK_DLL_HASH((ULONG_PTR)BaseVa);
    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {
        Dll = CONTAINING_RECORD(ListEntry, BTR_DLL_ENTRY, ListEntry);
        if (Dll->BaseVa == (ULONG_PTR)BaseVa) {
            return Dll;
        }
        ListEntry = ListEntry->Flink;
    }

    return NULL;
}

ULONG 
ApsCompareMemoryPointer(
    IN PVOID Source,
    IN PVOID Destine,
    IN ULONG NumberOfPointers
    )
{
    ULONG Number;

    for(Number = 0; Number < NumberOfPointers; Number += 1) {
        if (*(PULONG_PTR)Source != *(PULONG_PTR)Destine) {
            return Number;
        }
    }

    return Number;
}

ULONG
ApsDecodeModuleName(
    IN HANDLE ProcessHandle,
    IN ULONG64 Address,
    IN PCHAR NameBuffer,
    IN ULONG BufferLength
    )
{
    PIMAGEHLP_MODULE64 Module;
    ULONG64 Buffer[sizeof(IMAGEHLP_MODULE64) / sizeof(ULONG64) + 1];
    ULONG Status;

    Module = (PIMAGEHLP_MODULE64)Buffer;
    Module->SizeOfStruct = ApsUlongRoundUp(sizeof(IMAGEHLP_MODULE64), 8);
    Status = SymGetModuleInfo64(ProcessHandle, 
        Address,
        Module);
    if (Status != TRUE) {
        strncpy(NameBuffer, "<Unknown>", BufferLength);
    }
    else {
        strncpy(NameBuffer, Module->ModuleName, BufferLength);
    }

    strlwr(NameBuffer);
    return APS_STATUS_OK;
}

VOID
ApsDecodeStackTrace(
    __in PBTR_TEXT_TABLE Table,
    __in PBTR_STACK_RECORD Trace
    )
{
    ULONG Depth;
    ULONG64 Address;
    PBTR_TEXT_ENTRY Entry;

    for(Depth = 0; Depth < Trace->Depth; Depth += 1) {
        Address = (ULONG64) Trace->Frame[Depth];
        Entry = ApsInsertTextEntry(Table, Address);
    }
}

VOID
ApsAddressToText(
    __in HANDLE ProcessHandle,
    __in ULONG64 Address,
    __out PCHAR Buffer,
    __out ULONG Length,
    __out PULONG ResultLength
    )
{
    ULONG Status;
    ULONG64 Displacement;
    SYMBOL_INFO_PACKAGE Info;

    Info.si.SizeOfStruct = sizeof(SYMBOL_INFO);
    Info.si.Address = 0;
    Info.si.MaxNameLen = MAX_SYM_NAME;
    Status = SymFromAddr(ProcessHandle, Address, &Displacement, &Info.si);

    if (Status != TRUE) {
        *ResultLength = _snprintf(Buffer, Length - 1, "0x%p", (PVOID)Address);
    } else {

        if (Displacement != 0) {
            *ResultLength = _snprintf(Buffer, Length - 1, "%s+0x%I64x", 
                Info.si.Name, Displacement);
        } else {
            *ResultLength = _snprintf(Buffer, Length - 1, "%s", Info.si.Name);
        }
    }

    Buffer[Length - 1] = 0;
}

VOID
ApsAddressToTextEx(
    __in HANDLE ProcessHandle,
    __in ULONG64 Address,
    __out PCHAR Buffer,
    __out ULONG Length,
    __out PULONG ResultLength
    )
{
    ULONG Status;
    ULONG64 Displacement;
    ULONG64 ModuleBase;
    CHAR ModuleName[64];
    SYMBOL_INFO_PACKAGE Info;

    Info.si.SizeOfStruct = sizeof(SYMBOL_INFO);
    Info.si.Address = 0;
    Info.si.MaxNameLen = MAX_SYM_NAME;
    Status = SymFromAddr(ProcessHandle, Address, &Displacement, &Info.si);

    ApsDecodeModuleName(ProcessHandle, Address, ModuleName, 64);

    if (Status != TRUE) {
        ModuleBase = SymGetModuleBase64(ProcessHandle, Address);
        Displacement = Address - ModuleBase;
        *ResultLength = _snprintf(Buffer, Length - 1, "%s!+0x%I64x", 
            ModuleName, Displacement);
    } else {

        if (Displacement != 0) {
            *ResultLength = _snprintf(Buffer, Length - 1, "%s!%s+0x%I64x", 
                ModuleName, Info.si.Name, Displacement);
        } else {
            *ResultLength = _snprintf(Buffer, Length - 1, "%s!%s", 
                ModuleName, Info.si.Name);
        }
    }
}

ULONG 
ApsHashStringBKDR(
    IN PSTR Buffer
    )
{
    ULONG Hash = 0;
    ULONG Seed = 131;

    while (*Buffer) {
        Hash = Hash * Seed + *Buffer;
        Buffer += 1;
    }
    return Hash & 0x7fffffff;
}

ULONG
ApsCreateTextTable(
    __in HANDLE ProcessHandle,
    __in ULONG Buckets,
    __out PBTR_TEXT_TABLE *SymbolTable
    )
{
    PBTR_TEXT_TABLE Table;
    ULONG Size;
    HANDLE Handle;
    ULONG Status;
    ULONG i;

    Size = FIELD_OFFSET(BTR_TEXT_TABLE, Hash[Buckets]);

    Handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, NULL);
    if (!Handle) {
        return GetLastError();
    }

    Table = (PBTR_TEXT_TABLE)MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE,
        0, 0, 0);
    if (!Table) {
        Status = GetLastError();
        CloseHandle(Handle);
    }

    Table->Object = Handle;
    Table->Buckets = Buckets;
    Table->Count = 0;
    Table->Process = ProcessHandle;

    for(i = 0; i < Buckets; i++) {
        InitializeListHead(&Table->Hash[i].ListHead);
        ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
    }

    *SymbolTable = Table;
    return APS_STATUS_OK;
}

PBTR_TEXT_ENTRY
ApsInsertTextEntry(
    __in PBTR_TEXT_TABLE Table,
    __in ULONG64 Address
    )
{
    ULONG Bucket;
    PBTR_TEXT_ENTRY Text;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    ULONG Length;

    Bucket = (ULONG)(Address % Table->Buckets);

    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListHead != ListEntry) {
        Text = CONTAINING_RECORD(ListEntry, BTR_TEXT_ENTRY, ListEntry);
        if (Text->Address == Address) {
            return Text;
        }
        ListEntry = ListEntry->Flink;
    }

    Text = (PBTR_TEXT_ENTRY)ApsMalloc(sizeof(BTR_TEXT_ENTRY));
    Text->Address = Address;

    //
    // N.B. Maximum symbol name length is STACK_TEXT_LIMIT
    //

    ApsAddressToText(Table->Process, Address, Text->Text, STACK_TEXT_LIMIT, &Length);

    InsertHeadList(ListHead, &Text->ListEntry);
    Table->Count += 1;

    return Text;
}

VOID
ApsInsertTextEntryEx(
    __in PBTR_TEXT_TABLE Table,
    __in PBTR_TEXT_ENTRY Entry 
    )
{
    ULONG Bucket;
    PBTR_TEXT_ENTRY Text;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    ULONG64 Address;

    Address = Entry->Address;
    Bucket = (ULONG)(Address % Table->Buckets);

    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListHead != ListEntry) {
        Text = CONTAINING_RECORD(ListEntry, BTR_TEXT_ENTRY, ListEntry);
        if (Text->Address == Address) {
            return;
        }
        ListEntry = ListEntry->Flink;
    }

    InsertHeadList(ListHead, &Entry->ListEntry);
    Table->Count += 1;
}

PBTR_TEXT_ENTRY
ApsLookupSymbol(
    __in PBTR_TEXT_TABLE Table,
    __in ULONG64 Address
    )
{
    ULONG Bucket;
    PBTR_TEXT_ENTRY Text;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;

    Bucket = (ULONG)(Address % Table->Buckets);

    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListHead != ListEntry) {
        Text = CONTAINING_RECORD(ListEntry, BTR_TEXT_ENTRY, ListEntry);
        if (Text->Address == Address) {
            return Text;
        }
        ListEntry = ListEntry->Flink;
    }

    return NULL;
}

PBTR_TEXT_ENTRY
ApsInsertPseudoTextEntry(
    __in PBTR_TEXT_TABLE Table,
    __in ULONG64 Address 
    )
{
    ULONG Bucket;
    PBTR_TEXT_ENTRY Text;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;

    Bucket = (ULONG)(Address % Table->Buckets);

    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListHead != ListEntry) {
        Text = CONTAINING_RECORD(ListEntry, BTR_TEXT_ENTRY, ListEntry);
        if (Text->Address == Address) {
            return Text;
        }
        ListEntry = ListEntry->Flink;
    }

    //
    // Insert the pseudo text entry into list tail of hash bucket
    // the text field is formatted as its address string
    //

    Text = (PBTR_TEXT_ENTRY)ApsMalloc(sizeof(BTR_TEXT_ENTRY));
    Text->Address = Address;
    Text->FunctionId = -1;
    Text->LineId = -1;
    StringCchPrintfA(Text->Text, STACK_TEXT_LIMIT, "0x%p", (PVOID)Address);

    InsertTailList(ListHead, &Text->ListEntry);
    Table->Count += 1;
    return Text;
}

VOID
ApsDestroySymbolTable(
    __in PBTR_TEXT_TABLE Table
    )
{
    HANDLE Object;
    PBTR_TEXT_ENTRY Text;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    ULONG i;

    for(i = 0; i < Table->Buckets; i++) {
        ListHead = &Table->Hash[i].ListHead;
        while (IsListEmpty(ListHead) != TRUE) {
            ListEntry = RemoveHeadList(ListHead);
            Text = CONTAINING_RECORD(ListEntry, BTR_TEXT_ENTRY, ListEntry);
            ApsFree(Text);
        }
    }

    if (Table->Object != NULL) {
        Object = Table->Object;
        UnmapViewOfFile(Table);
        CloseHandle(Object);
    } else {
        ApsFree(Table);
    }
}

ULONG
ApsWriteSymbolStream(
    __in PPF_REPORT_HEAD Head,
    __in HANDLE FileHandle,
    __in PBTR_TEXT_TABLE Table,
    __in LARGE_INTEGER Start,
    __out PLARGE_INTEGER End
    )
{
    ULONG Size;
    ULONG i;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PBTR_TEXT_ENTRY Entry;
    ULONG Status;
    HANDLE Mapping;
    PBTR_TEXT_FILE TextFile;
    ULONG Complete;

    //
    // Compute text file size on disk
    //

    Size = FIELD_OFFSET(BTR_TEXT_FILE, Text[Table->Count]);
    Mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, NULL);
    if (!Mapping) {
        return GetLastError();
    }

    TextFile = (PBTR_TEXT_FILE)MapViewOfFile(Mapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
    if (!TextFile) {
        Status = GetLastError();
        CloseHandle(Mapping);
        return Status;
    }

    TextFile->Count = 0;

    //
    // Serialize all text buckets to text file cache
    //

    for(i = 0; i < Table->Buckets; i++) {

        ListHead = &Table->Hash[i].ListHead;
        ListEntry = ListHead->Flink;

        while (ListEntry != ListHead) {

            Entry = CONTAINING_RECORD(ListEntry, BTR_TEXT_ENTRY, ListEntry);
            TextFile->Text[TextFile->Count] = *Entry;

            ListEntry = ListEntry->Flink;
            TextFile->Count += 1;
        }
    }

    ASSERT(TextFile->Count == Table->Count);

    //
    // Commit text file cache to disk
    //

    Status = WriteFile(FileHandle, TextFile, Size, &Complete, NULL);
    if (Status != TRUE) {

        Status = GetLastError();
        End->QuadPart = Start.QuadPart;

    } else {

        Status = APS_STATUS_OK;
        End->QuadPart = Start.QuadPart + Size;

        //
        // Mark stream information in report head
        //

        Head->Streams[STREAM_SYMBOL].Offset = Start.QuadPart;
        Head->Streams[STREAM_SYMBOL].Length = Size;
    }

    UnmapViewOfFile(TextFile);
    CloseHandle(Mapping);

    return Status;
}

PBTR_TEXT_TABLE
ApsBuildSymbolTable(
    __in PBTR_TEXT_FILE File,
    __in ULONG Buckets
    )
{
    PBTR_TEXT_TABLE Table;
    PBTR_TEXT_ENTRY Entry;
    ULONG i;
    ULONG Bucket;
    ULONG Size;

    Size = FIELD_OFFSET(BTR_TEXT_TABLE, Hash[Buckets]);
    Table = (PBTR_TEXT_TABLE)ApsMalloc(Size);
    Table->Buckets = Buckets;
    Table->Count = File->Count;
    Table->Object = NULL;
    Table->Process = NULL;

    for(i = 0; i < Table->Buckets; i++) {
        InitializeListHead(&Table->Hash[i].ListHead);
        ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
    }

    for(i = 0; i < File->Count; i++) {

        Entry = (PBTR_TEXT_ENTRY)ApsMalloc(sizeof(BTR_TEXT_ENTRY));
        *Entry = File->Text[i];

        Bucket = (ULONG)(Entry->Address % Buckets);
        InsertHeadList(&Table->Hash[Bucket].ListHead, &Entry->ListEntry);
    }

    return Table;
}

ULONG
ApsDecodeStackStream(
    __in HANDLE ProcessHandle,
    __in PWSTR ReportPath,
    __in PWSTR ImagePath,
    __in PWSTR SymbolPath
    )
{
    ULONG Status;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PPF_REPORT_HEAD Head;
    PBTR_STACK_RECORD Record;
    PBTR_TEXT_TABLE Table;
    LARGE_INTEGER Start, End;
    PBTR_DLL_FILE DllFile;
    ULONG Count;
    ULONG i;

    Status = ApsInitDbghelp(ProcessHandle, SymbolPath); 
    if (Status != APS_STATUS_OK) {
        return Status;
    }

    FileHandle = CreateFile(ReportPath, GENERIC_READ|GENERIC_WRITE, 
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    Status = APS_STATUS_ERROR;
    MappingHandle = NULL;
    Head = NULL;

    __try {

        MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
        if (!MappingHandle) {
            Status = GetLastError();
            __leave;
        }

        Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
        if (!Head) {
            Status = GetLastError();
            __leave;
        }

        //
        // Check whether it's already analyzed
        //

        ASSERT(Head->Complete != 0);
        ASSERT(Head->Counters != 0);

        if (Head->Analyzed) {
            Status = APS_STATUS_OK;
            __leave;
        }

        //
        // Load symbol file based on dll stream
        //

        DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
        ApsLoadAllSymbols(ProcessHandle, DllFile);

        Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
        Count = ApsGetStreamLength(Head, STREAM_STACK) / sizeof(BTR_STACK_RECORD);

        //
        // Create symbol text table
        //

        ApsCreateTextTable(ProcessHandle, 4093, &Table);

        //
        // Decode all stack record embedded in report file
        //

        for(i = 0; i < Count; i++) {
            ApsDecodeStackTrace(Table, &Record[i]);
        }

        GetFileSizeEx(FileHandle, &Start);
        SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

        Status = ApsWriteSymbolStream(Head, FileHandle, Table, Start, &End);
        if (Status != APS_STATUS_OK) {
            __leave;
        }

        ApsDestroySymbolTable(Table);
        Status = APS_STATUS_OK;
    }
    __finally {

        if (Status == APS_STATUS_OK) {

            //
            // Mark as analyzed and update checksum
            //

            if (!Head->Analyzed) { 
                Head->Analyzed = 1;
                ApsComputeStreamCheckSum(Head);
            }
        }

        if (Head != NULL) {
            UnmapViewOfFile(Head);
        }

        if (MappingHandle != NULL) {
            CloseHandle(MappingHandle);
        }

        if (FileHandle != NULL) {
            CloseHandle(FileHandle);
        }

        ApsCleanDbghelp(ProcessHandle);
    }

    return Status;
}

ULONG
ApsCleanDbghelp(
    __in HANDLE ProcessHandle
    )
{
    SymCleanup(ProcessHandle);
    return APS_STATUS_OK;
}

ULONG
ApsBuildSymbolPathList(
    __in HANDLE ProcessHandle,
    __out PLIST_ENTRY ListHead
    )
{
    PSTR Path;
    PCHAR StartPtr;
    PCHAR EndPtr;
    PCHAR SrvPtr;
    ULONG Size;
    PAPS_SYMBOL_PATH PathEntry;

    InitializeListHead(ListHead);

    Path = (PSTR)ApsMalloc(ApsPageSize);
    SymGetSearchPath(ProcessHandle, Path, ApsPageSize);

    //
    // N.B. Ensure the whole path ends with a ';' we use it
    // as indicator to split each path
    //

    Size = (ULONG)strlen(Path);
    if (Path[Size - 1] != ';') {
        Path[Size] = ';';
        Path[Size + 1] = 0;
    }

    StartPtr = (PCHAR)Path;
    EndPtr = NULL;

    while (EndPtr = strchr(StartPtr, ';')) {

        *EndPtr = 0;
        SrvPtr = strchr(StartPtr, '*');
        if (SrvPtr) {

            //
            // This is a symbol server path, just skip it, we only need
            // local, or lanman symbol path
            //

            *EndPtr = ';';
            StartPtr = EndPtr + 1;
            continue;
        }

        PathEntry = (PAPS_SYMBOL_PATH)ApsMalloc(sizeof(APS_SYMBOL_PATH));
        strcpy_s(PathEntry->Path, MAX_PATH, StartPtr); 
        *EndPtr = ';';

        InsertHeadList(&ApsSymbolPathList, &PathEntry->ListEntry);
        StartPtr = EndPtr + 1;
    }

    ApsFree(Path);
    return APS_STATUS_OK;
}

BOOL CALLBACK 
ApsPdbEnumCallback(
    __in PCSTR FilePath,
    __in PVOID CallerData
    )
{
    PAPS_PDB_ENUM_CONTEXT Context;
    DWORD64 Address;	
    IMAGEHLP_MODULE64 Info;

    Context = (PAPS_PDB_ENUM_CONTEXT)CallerData;
    ASSERT(Context != NULL);

    Address = SymLoadModuleEx(Context->ProcessHandle, NULL, FilePath, NULL, 
        (DWORD64)Context->Dll.BaseVa, (DWORD)Context->Dll.Size, 
        &Context->Data, 0);
    if (!Address) {
        return FALSE;
    }

    Info.SizeOfStruct = sizeof(Info);
    SymGetModuleInfo64(Context->ProcessHandle, Address, &Info);

    if (Info.LoadedPdbName[0] != 0) {
        Context->Matched = TRUE;
        return TRUE;

    } else {
        SymUnloadModule64(Context->ProcessHandle, (DWORD64)Context->Dll.BaseVa);
        return FALSE;
    }
}

ULONG
ApsLoadSymbol(
    __in HANDLE ProcessHandle,
    __in PBTR_DLL_ENTRY Dll,
    __in PMODLOAD_DATA Data
    )
{
    PLIST_ENTRY ListEntry;
    PAPS_SYMBOL_PATH PathEntry;
    CHAR Buffer[MAX_PATH];
    ULONG Status;
    APS_PDB_ENUM_CONTEXT Context;

    Status = APS_STATUS_ERROR;
    ListEntry = ApsSymbolPathList.Flink;

    Context.ProcessHandle = ProcessHandle;
    memcpy(&Context.Dll, Dll, sizeof(*Dll));
    memcpy(&Context.Data, Data, Data->ssize);
    Context.Matched = FALSE;
    Context.Path[0] = 0;

    while (ListEntry != &ApsSymbolPathList) {

        PathEntry = CONTAINING_RECORD(ListEntry, APS_SYMBOL_PATH, ListEntry);

        do {
            Status = EnumDirTree(ProcessHandle, (PCSTR)PathEntry->Path, 
                                 (PCSTR)Dll->CvRecord.PdbName, Buffer, 
                                 ApsPdbEnumCallback, &Context);
        } while (Status);

        ListEntry = ListEntry->Flink;
    }

    if (!Context.Matched) {
        return APS_STATUS_ERROR;
    }

    return APS_STATUS_OK;
}

ULONG
ApsCreateFunctionTable(
    __out PBTR_FUNCTION_TABLE *FuncTable
    )
{
    ULONG i;
    PBTR_FUNCTION_TABLE Table;

    Table = (PBTR_FUNCTION_TABLE)ApsMalloc(sizeof(BTR_FUNCTION_TABLE));
    for(i = 0; i < STACK_FUNCTION_BUCKET; i++) {
        ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
        InitializeListHead(&Table->Hash[i].ListHead);
    }

    *FuncTable = Table;
    return APS_STATUS_OK;
}

ULONG
ApsDestroyFunctionTable(
    __in PBTR_FUNCTION_TABLE Table
    )
{
    ULONG i;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    PBTR_FUNCTION_ENTRY FuncEntry;

    for(i = 0; i < STACK_FUNCTION_BUCKET; i++) {
        ListHead = &Table->Hash[i].ListHead;
        while (IsListEmpty(ListHead) != TRUE) {
            ListEntry = RemoveHeadList(ListHead);
            FuncEntry = CONTAINING_RECORD(ListEntry, BTR_FUNCTION_ENTRY, ListEntry);
            ApsFree(FuncEntry);
        }
    }

    return APS_STATUS_OK;
}

ULONG
ApsInsertFunction(
    __in PBTR_FUNCTION_TABLE Table,
    __in ULONG64 Address,
    __in ULONG Inclusive,
    __in ULONG Exclusive,
    __in ULONG DllId
    )
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PBTR_FUNCTION_ENTRY Function;

    Bucket = Address % STACK_FUNCTION_BUCKET;

    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {
        Function = CONTAINING_RECORD(ListEntry, BTR_FUNCTION_ENTRY, ListEntry);
        if (Function->Address == Address) {
            return Function->FunctionId;
        }
        ListEntry = ListEntry->Flink;
    }

    Function = (PBTR_FUNCTION_ENTRY)ApsMalloc(sizeof(BTR_FUNCTION_ENTRY));
    Function->Address = Address;
    Function->FunctionId = Table->Count;
    Function->DllId = DllId;

    InsertHeadList(ListHead, &Function->ListEntry);
    Table->Count += 1;

    return Table->Count - 1;
}

VOID
ApsComputeFunctionMetricsPerPc(
    __in PAPS_FUNCTION_METRICS Metrics,
    __in LONG Maximum,
    __in LONG Depth,
    __in PVOID FunctionPc,
    __in ULONG FunctionId,
    __in ULONG Times
    )
{
    LONG i;
    BOOLEAN Duplicated;

    //
    // N.B. If the FunctionPc is NULL, this indicate there's no
    // way to determine whether it's a named function and have to
    // ignore this Pc.
    //

    if (!FunctionPc || FunctionId == -1) {
        Metrics[Depth].Address = NULL;
        Metrics[Depth].FunctionId = -1;
        return;
    }

    Duplicated = FALSE;

    //
    // Scan the FunctionId to check whether it's duplicated
    //

    for(i = Depth + 1; i < Maximum; i += 1) {
        if (Metrics[i].FunctionId == FunctionId) {
            Duplicated = TRUE;
            break;
        }
    }

    if (!Duplicated) {
        Metrics[Depth].Address = FunctionPc;
        Metrics[Depth].Inclusive = (Depth + 1) * Times;
        Metrics[Depth].Exclusive = 1 * Times;
    } else {
        Metrics[Depth].Address = FunctionPc;
        Metrics[Depth].Inclusive = 0 * Times;
        Metrics[Depth].Exclusive = 1 * Times;
    }

    Metrics[Depth].FunctionId = FunctionId;
}

VOID
ApsUpdateFunctionMetrics(
    __in PBTR_FUNCTION_ENTRY Cache,
    __in PAPS_FUNCTION_METRICS Metrics,
    __in ULONG Maximum
    )
{
    ULONG Number;
    PBTR_FUNCTION_ENTRY Function;
    ULONG64 Address;
    ULONG FunctionId;

    for(Number = 0; Number < Maximum; Number += 1) {

        Address = (ULONG64)Metrics[Number].Address;
        FunctionId = Metrics[Number].FunctionId;

        if (!Address || FunctionId == -1) {
            continue;
        }
        
        Function = &Cache[FunctionId];
        Function->Inclusive += Metrics[Number].Inclusive;
        Function->Exclusive += Metrics[Number].Exclusive;
    }
}

VOID
ApsComputeFunctionMetrics(
    __in PPF_REPORT_HEAD Head,
    __in PBTR_TEXT_TABLE TextTable,
    __in PBTR_FUNCTION_ENTRY Cache
    )
{
    PBTR_STACK_RECORD Record;
    ULONG Count;
    ULONG Number;
    LONG Depth;
    LONG ScanDepth;
    PAPS_FUNCTION_METRICS Metrics;
    PBTR_TEXT_ENTRY TextEntry;

    Metrics = (PAPS_FUNCTION_METRICS)ApsMalloc(sizeof(APS_FUNCTION_METRICS) * MAX_STACK_DEPTH);
    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
    Count = (ULONG)Head->Streams[STREAM_STACK].Length / sizeof(BTR_STACK_RECORD);
    
    for(Number = 0; Number < Count; Number += 1) {

        //
        // Scan each stack trace from bottom to top
        //

        ScanDepth = Record->Depth;
        for (Depth = ScanDepth - 1; Depth >= 0; Depth -= 1) {

            //
            // Lookup the FunctionPc by frame Pc
            //

            TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Record->Frame[Depth]);

            if (TextEntry && TextEntry->FunctionId != -1) {
                ApsComputeFunctionMetricsPerPc(Metrics, ScanDepth, Depth, 
                                               Record->Frame[Depth],
                                               TextEntry->FunctionId,
                                               Record->Count);
            }
            else {

                //
                // If the Pc can not be grouped into named function, just ignore it
                //

                ApsComputeFunctionMetricsPerPc(Metrics, ScanDepth, Depth, 
                                               NULL, -1, Record->Count);
            }
        }

        ApsUpdateFunctionMetrics(Cache, Metrics, ScanDepth);
        Record += 1;
    }

    ApsFree(Metrics);
}

VOID
ApsComputeDllMetrics(
    __in PPF_REPORT_HEAD Head,
    __in PBTR_TEXT_TABLE TextTable,
    __in PBTR_DLL_ENTRY Cache 
    )
{
}

VOID
ApsComputeDllMetricsPerPc(
    __in PAPS_DLL_METRICS Metrics,
    __in LONG Maximum,
    __in LONG Depth,
    __in PVOID Pc,
    __in ULONG DllId,
    __in ULONG Times
    )
{
}

VOID
ApsUpdateDllMetrics(
    __in PBTR_DLL_ENTRY Cache,
    __in PAPS_DLL_METRICS Metrics,
    __in ULONG Maximum
    )
{
}

ULONG
ApsCreateLineTable(
    __out PBTR_LINE_TABLE *LineTable
    )
{
    ULONG i;
    PBTR_LINE_TABLE Table;

    Table = (PBTR_LINE_TABLE)ApsMalloc(sizeof(BTR_LINE_TABLE));
    for(i = 0; i < STACK_LINE_BUCKET; i++) {
        InitializeListHead(&Table->Hash[i].ListHead);
        ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
    }

    *LineTable = Table;
    return APS_STATUS_OK;
}

VOID
ApsDestroyLineTable(
    __in PBTR_LINE_TABLE Table
    )
{
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    PBTR_LINE_ENTRY Entry;
    ULONG i;

    for(i = 0; i < STACK_LINE_BUCKET; i++) {
        ListHead = &Table->Hash[i].ListHead;
        while(IsListEmpty(ListHead) != TRUE) {
            ListEntry = RemoveHeadList(ListHead);
            Entry = CONTAINING_RECORD(ListEntry, BTR_LINE_ENTRY, ListEntry);
            ApsFree(Entry);
        }
    }

    ApsFree(Table);
}

ULONG
ApsInsertLine(
    __in PBTR_LINE_TABLE Table,
    __in ULONG64 Address,
    __in ULONG Line,
    __in PSTR FileName
    )
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PBTR_LINE_ENTRY Entry;

    Bucket = (ULONG)(Address % STACK_LINE_BUCKET);

    ListHead = &Table->Hash[Bucket].ListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {
        Entry = CONTAINING_RECORD(ListEntry, BTR_LINE_ENTRY, ListEntry);
        if (Entry->Address == Address) {
            return Entry->Id;
        }
        ListEntry = ListEntry->Flink;
    }

    Entry = (PBTR_LINE_ENTRY)ApsMalloc(sizeof(BTR_LINE_ENTRY));
    Entry->Address = Address;
    Entry->Line = Line;
    Entry->Id = Table->Count;
    StringCchCopyA(Entry->File, MAX_PATH, FileName);

    InsertHeadList(ListHead, &Entry->ListEntry);
    Table->Count += 1;
    return Entry->Id;
}

int __cdecl
ApsFunctionIdCompareCallback(
    __in const void *Ptr1,
    __in const void *Ptr2 
    )
{
    PBTR_FUNCTION_ENTRY F1;
    PBTR_FUNCTION_ENTRY F2;

    F1 = (PBTR_FUNCTION_ENTRY)Ptr1;
    F2 = (PBTR_FUNCTION_ENTRY)Ptr2;

    //
    // N.B. Pc is sorted in decreasing order
    //

    ASSERT(F1->FunctionId != -1);
    ASSERT(F2->FunctionId != -1);

    return (int)F1->FunctionId - F2->FunctionId;
}

ULONG
ApsWriteFunctionStream(
    __in PPF_REPORT_HEAD Head,
    __in HANDLE FileHandle,
    __in PBTR_FUNCTION_TABLE FuncTable,
    __in PBTR_TEXT_TABLE TextTable,
    __in LARGE_INTEGER Start,
    __out PLARGE_INTEGER End
    )
{
    PBTR_FUNCTION_ENTRY FuncEntry;
    PBTR_FUNCTION_ENTRY Cache;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG Complete;
    ULONG i, j;
    ULONG Length;

    SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

    if (!FuncTable->Count) {
        *End = Start;
        return APS_STATUS_OK;
    }

    Length = sizeof(BTR_FUNCTION_ENTRY) * FuncTable->Count;
    Cache = (PBTR_FUNCTION_ENTRY)ApsMalloc(Length);
    j = 0;

    for(i = 0; i < STACK_FUNCTION_BUCKET; i++) {

        ListHead = &FuncTable->Hash[i].ListHead;
        while (IsListEmpty(ListHead) != TRUE) {

            ListEntry = RemoveHeadList(ListHead);
            FuncEntry = CONTAINING_RECORD(ListEntry, BTR_FUNCTION_ENTRY, ListEntry);

            Cache[j] = *FuncEntry;
            j += 1;

            ApsFree(FuncEntry);
        }
    }

    ASSERT(j == FuncTable->Count);

    //
    // Sort the function entries in ascendant order
    //

    qsort(Cache, j, sizeof(BTR_FUNCTION_ENTRY), ApsFunctionIdCompareCallback);

    //
    // Update the function metrics
    //

    //ApsComputeFunctionMetrics(Head, TextTable, Cache);

    WriteFile(FileHandle, Cache, Length, &Complete, NULL);
    ApsFree(Cache);

    End->QuadPart = Start.QuadPart + Length;
    return APS_STATUS_OK;
}

int __cdecl
ApsLineIdCompareCallback(
    __in const void *Ptr1,
    __in const void *Ptr2 
    )
{
    PBTR_LINE_ENTRY Line1;
    PBTR_LINE_ENTRY Line2;

    Line1 = (PBTR_LINE_ENTRY)Ptr1;
    Line2 = (PBTR_LINE_ENTRY)Ptr2;

    //
    // N.B. Pc is sorted in decreasing order
    //

    ASSERT(Line1->Id != -1);
    ASSERT(Line2->Id != -1);

    return (int)Line1->Id - Line2->Id;
}

ULONG
ApsWriteLineStream(
    __in PPF_REPORT_HEAD Head,
    __in HANDLE FileHandle,
    __in PBTR_LINE_TABLE LineTable,
    __in LARGE_INTEGER Start,
    __out PLARGE_INTEGER End
    )
{
    PBTR_LINE_ENTRY LineEntry;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG Complete;
    ULONG i, j;
    PBTR_LINE_ENTRY Buffer;

    SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

    //
    // If there's no line information, return
    //

    if (!LineTable->Count) {
        *End = Start;
        return APS_STATUS_OK;
    }

    Buffer = (PBTR_LINE_ENTRY)ApsMalloc(sizeof(BTR_LINE_ENTRY) * LineTable->Count);
    j = 0;

    for(i = 0; i < STACK_LINE_BUCKET; i++) {

        ListHead = &LineTable->Hash[i].ListHead;

        while (IsListEmpty(ListHead) != TRUE) {

            ListEntry = RemoveHeadList(ListHead);
            LineEntry = CONTAINING_RECORD(ListEntry, BTR_LINE_ENTRY, ListEntry);

            Buffer[j] = *LineEntry;
            j += 1;

            ApsFree(LineEntry);
        }
    }

    //
    // N.B. Line id must be sorted before write to disk
    //

    ASSERT(LineTable->Count == j);
    qsort(Buffer, LineTable->Count, sizeof(BTR_LINE_ENTRY), ApsLineIdCompareCallback);

    //
    // Write line entries to disk
    //

    WriteFile(FileHandle, Buffer, sizeof(BTR_LINE_ENTRY) * LineTable->Count, 
              &Complete, NULL);
    ApsFree(Buffer);

    End->QuadPart = Start.QuadPart + sizeof(BTR_LINE_ENTRY) * LineTable->Count;
    return APS_STATUS_OK;
}

PBTR_DLL_ENTRY
ApsGetDllEntryByPc(
	__in PBTR_DLL_FILE DllFile,
	__in PVOID Pc
	)
{
	ULONG Number;
	PBTR_DLL_ENTRY DllEntry;

	//
	// N.B. This algorithm may be optimized by using BST to speed up
	// currently it's a basic linear scan
	//

    for(Number = 0; Number < DllFile->Count; Number += 1) {
        DllEntry = &DllFile->Dll[Number];
        if ((ULONG_PTR)Pc >= (ULONG_PTR)DllEntry->BaseVa && 
			(ULONG_PTR)Pc < (ULONG_PTR)DllEntry->BaseVa + DllEntry->Size) {
			break;
        }
    }

	if (Number < DllFile->Count) {
		return DllEntry;		
	}

	return NULL;
}

VOID
ApsDecodePc(
    __in HANDLE ProcessHandle,
    __in PBTR_PC_ENTRY PcEntry,
    __in PBTR_FUNCTION_TABLE FuncTable,
    __in PBTR_DLL_FILE DllFile,
    __in PBTR_TEXT_TABLE TextTable,
    __in PBTR_LINE_TABLE LineTable
    )
{
    ULONG Status;
    ULONG64 Displacement;
    SYMBOL_INFO_PACKAGE Info;
    PBTR_DLL_ENTRY DllEntry;
    ULONG64 Address;
    ULONG64 FuncAddress;
    ULONG FunctionId;
	ULONG FuncLineId;
    PBTR_TEXT_ENTRY TextEntry;
    IMAGEHLP_LINE64 Line64 = {0};

    //
    // Scan dll file to identify dll
    //

    Address = (ULONG64)PcEntry->Address;
    PcEntry->DllId = -1;
    PcEntry->UseAddress = TRUE;

	DllEntry = ApsGetDllEntryByPc(DllFile, (PVOID)Address);

    //
    // N.B. If the address does not fall into any dll address range, 
    // it's either a fuzzy address, or it's in dynamically allocated
    // code address, just ignore it.
    //

	if (DllEntry != NULL) {
		PcEntry->DllId = (USHORT)DllEntry->DllId;
	}
	else {
		return;
	}

    Info.si.SizeOfStruct = sizeof(SYMBOL_INFO);
    Info.si.Address = 0;
    Info.si.MaxNameLen = MAX_SYM_NAME;
    Status = SymFromAddr(ProcessHandle, Address, &Displacement, &Info.si);

    if (Status != TRUE) {

        //
        // N.B. UseAddress is FALSE, indicate that we will use it's DLL to represent
        // the pc entry.
        //

        PcEntry->UseAddress = FALSE;
        return;
    }

    FuncAddress = Info.si.Address;
    FunctionId = ApsInsertFunction(FuncTable, FuncAddress, 
                                   PcEntry->Inclusive, PcEntry->Exclusive, 
                                   (ULONG)PcEntry->DllId);
    PcEntry->FunctionId = FunctionId;

    //
    // Get line information
    //

    Line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    Status = SymGetLineFromAddr64(ProcessHandle, Address, 
		                          (PDWORD)&Displacement, &Line64);
    if (Status != TRUE) {
        PcEntry->LineId = -1;
    } else {
        PcEntry->LineId = ApsInsertLine(LineTable, Address, Line64.LineNumber, Line64.FileName);
    }

    TextEntry = ApsInsertTextEntry(TextTable, Address);
    TextEntry->FunctionId = PcEntry->FunctionId;
    TextEntry->LineId = PcEntry->LineId;

    //
    // Get function line information
	//

    Status = SymGetLineFromAddr64(ProcessHandle, FuncAddress, 
		                         (PDWORD)&Displacement, &Line64);
    if (Status != TRUE) {
        FuncLineId = -1;
    } else {
        FuncLineId = ApsInsertLine(LineTable, FuncAddress, 
			                       Line64.LineNumber, 
								   Line64.FileName);
    }

    TextEntry = ApsInsertTextEntry(TextTable, FuncAddress);
    TextEntry->FunctionId = FunctionId;
    TextEntry->LineId = FuncLineId;
}

int __cdecl
ApsMetricsSortFunctionCallback(
    __in const void *Ptr1,
    __in const void *Ptr2 
    )
{
	int Result;
	PAPS_PC_METRICS M1;
	PAPS_PC_METRICS M2;

    M1 = (PAPS_PC_METRICS)Ptr1;
    M2 = (PAPS_PC_METRICS)Ptr2;

    Result = (int)M1->FunctionId - M2->FunctionId;
	if (Result == 0) {

		//
		// N.B. If they're same function, put greater inclusive ahead
		// so that we don't scan the array to select the maximum one,
		// the first one of same function has the maximum depth
		//

		Result = -((int)M1->Depth - (int)M2->Depth);
	}

	return Result;
}

int __cdecl
ApsMetricsSortModuleCallback(
    __in const void *Ptr1,
    __in const void *Ptr2 
    )
{
	int Result;
	PAPS_PC_METRICS M1;
	PAPS_PC_METRICS M2;

    M1 = (PAPS_PC_METRICS)Ptr1;
    M2 = (PAPS_PC_METRICS)Ptr2;

    Result = (int)M1->DllId - M2->DllId;
	if (Result == 0) {

		//
		// N.B. If they're same function, put greater inclusive ahead
		// so that we don't scan the array to select the maximum one,
		// the first one of same function has the maximum depth
		//

		Result = -((int)M1->DllInclusive - (int)M2->DllInclusive);
	}

	return Result;
}

VOID
ApsInsertFunctionMetrics(
    __in PBTR_FUNCTION_TABLE Table,
    __in PAPS_PC_METRICS Metrics,
	__in ULONG Depth,
	__in ULONG Times
    )
{
    ULONG Number;
    ULONG64 Address;
	ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PBTR_FUNCTION_ENTRY Function;

    for(Number = 0; Number < Depth; Number += 1) {

		Address = (ULONG64)Metrics[Number].FunctionAddress;
		if (!Address || Metrics[Number].FunctionId == -1) {
			continue;
		}

		//
		// Lookup the hash table and update its counters
		//

		Bucket = Address % STACK_FUNCTION_BUCKET;
		ListHead = &Table->Hash[Bucket].ListHead;
		ListEntry = ListHead->Flink;

		while (ListEntry != ListHead) {

			Function = CONTAINING_RECORD(ListEntry, BTR_FUNCTION_ENTRY, ListEntry);

			if (Function->Address == Address) {
		        Function->Inclusive += Metrics[Number].FunctionInclusive * Times;
		        Function->Exclusive += Metrics[Number].FunctionExclusive * Times;
				break;
			}

			ListEntry = ListEntry->Flink;
		}
    }
}

VOID
ApsInsertDllMetrics(
    __in PBTR_DLL_FILE DllFile,
    __in PAPS_PC_METRICS Metrics,
	__in ULONG Depth,
	__in ULONG Times
    )
{
    ULONG Number;
    ULONG64 Address;
    PBTR_DLL_ENTRY Dll;

    for(Number = 0; Number < Depth; Number += 1) {

		if (Metrics[Number].DllId == -1) {
			continue;
		}

		Address = (ULONG64)Metrics[Number].PcAddress;
		ASSERT(Address != 0);

		Dll = ApsGetDllEntryByPc(DllFile, (PVOID)Address);
		if (Dll != NULL) {
			Dll->Inclusive += Metrics[Number].DllInclusive * Times;
			Dll->Exclusive += Metrics[Number].DllExclusive * Times;
		}
    }
}

PBTR_PC_ENTRY
ApsAllocatePcEntry(
	VOID
	)
{
	PBTR_PC_ENTRY PcEntry;

	PcEntry = (PBTR_PC_ENTRY)ApsMalloc(sizeof(BTR_PC_ENTRY));
	PcEntry->DllId = -1;
	PcEntry->FunctionId = -1;
	PcEntry->LineId = -1;
	PcEntry->UseAddress = TRUE;
	InitializeListHead(&PcEntry->ListEntry);

	return PcEntry;
}

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
    )
{
    ULONG Status;
    ULONG64 Displacement;
    SYMBOL_INFO_PACKAGE Info;
    PBTR_DLL_ENTRY DllEntry;
    ULONG FunctionId;
    ULONG64 FuncAddress;
    ULONG64 Address;
    PBTR_TEXT_ENTRY TextEntry;
    IMAGEHLP_LINE64 Line64 = {0};
	LONG Depth;
	LONG ScanDepth;
	PBTR_PC_ENTRY PcEntry;
	LONG Number;

	PAPS_PC_METRICS Metrics;
	Metrics = (PAPS_PC_METRICS)ApsMalloc(sizeof(APS_PC_METRICS) * MAX_STACK_DEPTH);

	ScanDepth = Record->Depth;
	for(Depth = ScanDepth - 1; Depth >= 0; Depth -= 1) {

		PcEntry = ApsAllocatePcEntry();

		//
		// Fill Address
		//
		
		Address = (ULONG64)Record->Frame[Depth];
		Metrics[Depth].PcAddress = (PVOID)Address;
		PcEntry->Address = Metrics[Depth].PcAddress;

		//
		// Fill DllId
		//

		DllEntry = ApsGetDllEntryByPc(DllFile, (PVOID)Address);
		if (DllEntry != NULL) {
			Metrics[Depth].DllId  = (USHORT)DllEntry->DllId;
		}
		else {
			Metrics[Depth].DllId  = -1;
		}
		
		PcEntry->DllId = Metrics[Depth].DllId;

		//
		// Fill FunctionId and insert function into function table
		//

		Info.si.SizeOfStruct = sizeof(SYMBOL_INFO);
		Info.si.Address = 0;
		Info.si.MaxNameLen = MAX_SYM_NAME;
		Status = SymFromAddr(ProcessHandle, Address, &Displacement, &Info.si);

		if (Status != TRUE) {
			FuncAddress = (ULONG64)NULL;
			Metrics[Depth].FunctionAddress = (PVOID)FuncAddress;
			Metrics[Depth].FunctionId = -1;
		} else {
			FuncAddress = Info.si.Address;
			Metrics[Depth].FunctionAddress = (PVOID)FuncAddress;
			FunctionId = ApsInsertFunction(FuncTable, FuncAddress, 0, 0, PcEntry->DllId);
			Metrics[Depth].FunctionId = FunctionId;
		}

		PcEntry->FunctionId = Metrics[Depth].FunctionId;

		//
		// Fill PcLineId and insert line into line table
		//

		Line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		Status = SymGetLineFromAddr64(ProcessHandle, Address, (PDWORD)&Displacement, &Line64);

		if (Status != TRUE) {
			Metrics[Depth].PcLineId = -1;
		} else {
			Metrics[Depth].PcLineId = ApsInsertLine(LineTable, Address, 
				                                    Line64.LineNumber, 
													Line64.FileName);
		}
		
		PcEntry->LineId = Metrics[Depth].PcLineId;

		//
		// Fill FunctionLineId 
		//

		if (FuncAddress) {
			Status = SymGetLineFromAddr64(ProcessHandle, FuncAddress, 
				                         (PDWORD)&Displacement, &Line64);
			if (Status != TRUE) {
				Metrics[Depth].FunctionLineId = -1;
			} else {
				Metrics[Depth].FunctionLineId = ApsInsertLine(LineTable, FuncAddress, 
					                                          Line64.LineNumber, 
															  Line64.FileName);
			}
		}

		//
		// Insert Pc symbol and function symbol into tables
		//

		if (Metrics[Depth].FunctionId != -1) {

			TextEntry = ApsInsertTextEntry(TextTable, FuncAddress);
			TextEntry->FunctionId = Metrics[Depth].FunctionId;
			TextEntry->LineId = Metrics[Depth].FunctionLineId;

			TextEntry = ApsInsertTextEntry(TextTable, Address);
			TextEntry->FunctionId = PcEntry->FunctionId;
			TextEntry->LineId = PcEntry->LineId;
		}

		//
		// Insert PC into PC table, the PcEntry can not be
		// touched after ApsInsertPcEntryEx() since it's possible
		// that it's already freed
		// 

	    ApsInsertPcEntryEx(PcTable, PcEntry, Record->Count, Depth, Type);

		if (Type == PROFILE_CPU_TYPE) {

			//
			// Set temporary function and dll counters
			//

			if (Metrics[Depth].FunctionId != -1) {

				if (Depth == 0) {
					Metrics[Depth].FunctionExclusive = 1;
					Metrics[Depth].FunctionInclusive = 1;
				} else {
					Metrics[Depth].FunctionExclusive = 0;
					Metrics[Depth].FunctionInclusive = 1;
				}

			} else {

				//
				// N.B. If the function has no symbol, we may need have a deviation
				// counter to fix the inclusive and exclusive ratio
				//

				Metrics[Depth].FunctionExclusive = 0;
				Metrics[Depth].FunctionInclusive = 0;
			}

			if (Metrics[Depth].DllId != -1) {

				if (Depth == 0) {
					Metrics[Depth].DllExclusive = 1;
					Metrics[Depth].DllInclusive = 1;
				} else {
					Metrics[Depth].DllExclusive = 0;
					Metrics[Depth].DllInclusive = 1;
				}

			} else {
				
				//
				// N.B. If the function has no symbol, we may need have a deviation
				// counter to fix the inclusive and exclusive ratio
				//

				Metrics[Depth].DllExclusive = 0;
				Metrics[Depth].DllInclusive = 0;
			}
		}
	}

	//
	// Sort by FunctionId and ModuleId and then scan duplicated entries
	//

	if (Type == PROFILE_CPU_TYPE) {

		qsort(Metrics, ScanDepth, sizeof(APS_PC_METRICS), ApsMetricsSortFunctionCallback);
		for(Number = 0; Number < ScanDepth - 1; Number += 1) {
			if (Metrics[Number].FunctionId == Metrics[Number + 1].FunctionId) {
				Metrics[Number + 1].FunctionInclusive = 0;
			}
		}
		ApsInsertFunctionMetrics(FuncTable, Metrics, ScanDepth, Record->Count);

		qsort(Metrics, ScanDepth, sizeof(APS_PC_METRICS), ApsMetricsSortModuleCallback);
		for(Number = 0; Number < ScanDepth - 1; Number += 1) {
			if (Metrics[Number].DllId == Metrics[Number + 1].DllId) {
				Metrics[Number + 1].DllInclusive = 0;
			}
		}
		ApsInsertDllMetrics(DllFile, Metrics, ScanDepth, Record->Count);

	}

	ApsFree(Metrics);
	Metrics = NULL;
}

BOOLEAN
ApsIsValidPc(
    __in PVOID Pc
    )
{
#define PTR_64KB  0x10000

    if ((ULONG_PTR)Pc < PTR_64KB) {
        return FALSE;
    }

    return TRUE;
}

ULONG
ApsCreateStackTable(
    __out PBTR_STACK_TABLE *StackTable
    )
{
    ULONG i;
    PBTR_STACK_TABLE Table;

    Table = (PBTR_STACK_TABLE)ApsMalloc(sizeof(BTR_STACK_TABLE));
    Table->Count = 0;

    for(i = 0; i < STACK_RECORD_BUCKET; i++) {
        ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
        InitializeListHead(&Table->Hash[i].ListHead);
    }

    *StackTable = Table;
    return APS_STATUS_OK;
}

VOID
ApsDestroyStackTable(
    __out PBTR_STACK_TABLE StackTable
    )
{
    ULONG i;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    PBTR_STACK_ENTRY Entry;

    for(i = 0; i < STACK_RECORD_BUCKET; i++) {
        ListHead = &StackTable->Hash[i].ListHead;
        while (IsListEmpty(ListHead) != TRUE) {
            ListEntry = RemoveHeadList(ListHead);
            Entry = CONTAINING_RECORD(ListEntry, BTR_STACK_ENTRY, ListEntry);
            ApsFree(Entry);
        }
    }

    ApsFree(StackTable);
}