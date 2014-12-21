#include "apsrpt.h"
#include "apspdb.h"
#include "aps.h"

ULONG
ApsDebugDumpDll(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	)
{
	PBTR_DLL_FILE DllFile;
	ULONG i;
	PBTR_DLL_ENTRY Dll;
	CHAR Buffer[MAX_PATH];

	DllFile = (PBTR_DLL_FILE)((PUCHAR)Head + (ULONG)Head->Streams[STREAM_DLL].Offset);

	fprintf(fp, "# DLL STREAM: %d of dlls loaded \n", DllFile->Count);
	
	for(i = 0; i < DllFile->Count; i++) {

		Dll = &DllFile->Dll[i];

		//
		// dump dll path, note that fprintf does not support UNICODE
		//

		StringCchPrintfA(Buffer, MAX_PATH, "%S", Dll->Path);
		fprintf(fp, "%s\n", Buffer);

		//
		// dump code view record
		//

		ApsConvertGuidToString((UUID *)&Dll->CvRecord.Signature, Buffer, MAX_PATH);
		fprintf(fp, "base: 0x%x, size: 0x%x, cvsig %08x  GUID: %s, Age: %d PDB: %s \n",
					Dll->BaseVa, Dll->Size, Dll->CvRecord.CvSignature, Buffer, Dll->CvRecord.Age, 
					Dll->CvRecord.PdbName);
	}
	
	fprintf(fp, "\n");
	return APS_STATUS_OK;
}

VOID
ApsDebugDumpBackTrace(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	)
{
	ULONG i, j;
	ULONG Count;
	PBTR_STACK_RECORD Record;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY Text;
	PBTR_TEXT_TABLE Table;

	Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamLength(Head, STREAM_STACK) / sizeof(BTR_STACK_RECORD);

	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	Table = ApsBuildSymbolTable(TextFile, 4093);

	fprintf(fp, "# BACK TRACE STREAM: %u of back trace records\n", Count);

	for (i = 0; i < Count; i++) {

		fprintf(fp, "%#08u: depth %u count %u size %I64u \n", 
			    i, Record->Depth, Record->Count, Record->SizeOfAllocs);

		for(j = 0; j < Record->Depth; j++) {
			Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[j]);
			ASSERT(Text != NULL);
			fprintf(fp, "#%02u: 0x%I64x %s\n", j, Record->Frame[j], Text->Text);
		}

		fprintf(fp, "\n");
		Record += 1;
	}

	fprintf(fp, "\n");
	ApsDestroySymbolTable(Table);
}

ULONG
ApsDebugDumpHeapLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	)
{
	ULONG i, j, k;
	PPF_STREAM_HEAP_LEAK Leak;
	PBTR_LEAK_FILE LeakFile;
	PBTR_LEAK_ENTRY Entry;
	PBTR_STACK_RECORD StackRecord;
	PBTR_STACK_RECORD Record;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY Text;
	PBTR_TEXT_TABLE Table;

	Leak = (PPF_STREAM_HEAP_LEAK)ApsGetStreamPointer(Head, STREAM_LEAK_HEAP);
	if (!Leak) {
		return 0;
	}

	fprintf(fp, "# STREAM HEAP LEAK, total %d heaps\n", Leak->NumberOfHeaps);
	StackRecord = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);

	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	Table = ApsBuildSymbolTable(TextFile, 4093);

	for(i = 0; i < Leak->NumberOfHeaps; i++) {

		LeakFile = (PBTR_LEAK_FILE)((PUCHAR)Head + Leak->Info[i].Offset);
		fprintf(fp, "# HEAP 0x%0I64x\n", (HANDLE)LeakFile->Context);

		for(j = 0; j < LeakFile->Count; j++) {

			Entry = &LeakFile->Leak[j];
			Record = &StackRecord[Entry->StackId];

			fprintf(fp, "stackid %u count %u size %I64d: \n",  Entry->StackId, Entry->Count, Entry->Size);

			for(k = 0; k < Record->Depth; k++) {
				Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[k]);
				ASSERT(Text != NULL);
				fprintf(fp, "#%02u: 0x%I64x %s\n", k, Record->Frame[k], Text->Text);
			}
			
			fprintf(fp, "\n");
		}

	    fprintf(fp, "\n");
	}

	fprintf(fp, "\n");

	ApsDestroySymbolTable(Table);
	return 0;
}

ULONG
ApsDebugDumpPageLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	)
{
	ULONG i, j;
	PBTR_LEAK_FILE LeakFile;
	PBTR_LEAK_ENTRY Entry;
	PBTR_STACK_RECORD StackRecord;
	PBTR_STACK_RECORD Record;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY Text;
	PBTR_TEXT_TABLE Table;

	LeakFile = (PBTR_LEAK_FILE)ApsGetStreamPointer(Head, STREAM_LEAK_PAGE);
	if (!LeakFile) {
		return 0;
	}

	StackRecord = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	Table = ApsBuildSymbolTable(TextFile, 4093);

	fprintf(fp, "# STREAM PAGE LEAK, total %d outstanding allocation \n", LeakFile->Count);

	for(i = 0; i < LeakFile->Count; i++) {

		Entry = &LeakFile->Leak[i];
		Record = &StackRecord[Entry->StackId];

		fprintf(fp, "stackid %u count %u \n",  Entry->StackId, Entry->Count);

		for(j = 0; j < Record->Depth; j++) {
			Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[j]);
			ASSERT(Text != NULL);
			fprintf(fp, "#%02u: 0x%I64x %s\n", j, Record->Frame[j], Text->Text);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");

	ApsDestroySymbolTable(Table);
	return 0;
}

ULONG
ApsDebugDumpHandleLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	)
{
	ULONG i, j;
	PBTR_LEAK_FILE LeakFile;
	PBTR_LEAK_ENTRY Entry;
	PBTR_STACK_RECORD StackRecord;
	PBTR_STACK_RECORD Record;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY Text;
	PBTR_TEXT_TABLE Table;

	LeakFile = (PBTR_LEAK_FILE)ApsGetStreamPointer(Head, STREAM_LEAK_HANDLE);
	if (!LeakFile) {
		return 0;
	}

	StackRecord = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	Table = ApsBuildSymbolTable(TextFile, 4093);

	fprintf(fp, "# STREAM HANDLE LEAK, total %d outstanding allocation \n", LeakFile->Count);

	for(i = 0; i < LeakFile->Count; i++) {

		Entry = &LeakFile->Leak[i];
		Record = &StackRecord[Entry->StackId];

		fprintf(fp, "stackid %u count %u \n",  Entry->StackId, Entry->Count);

		for(j = 0; j < Record->Depth; j++) {
			Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[j]);
			ASSERT(Text != NULL);
			fprintf(fp, "#%02u: 0x%I64x %s\n", j, Record->Frame[j], Text->Text);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");

	ApsDestroySymbolTable(Table);
	return 0;
}

ULONG
ApsDebugDumpGdiLeak(
	__in PPF_REPORT_HEAD Head,
	__in FILE *fp
	)
{
	ULONG i, j;
	PBTR_LEAK_FILE LeakFile;
	PBTR_LEAK_ENTRY Entry;
	PBTR_STACK_RECORD StackRecord;
	PBTR_STACK_RECORD Record;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY Text;
	PBTR_TEXT_TABLE Table;

	LeakFile = (PBTR_LEAK_FILE)ApsGetStreamPointer(Head, STREAM_LEAK_GDI);
	if (!LeakFile) {
		return 0;
	}

	StackRecord = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);

	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	Table = ApsBuildSymbolTable(TextFile, 4093);

	fprintf(fp, "# STREAM GDI LEAK, total %d outstanding allocation \n", LeakFile->Count);

	for(i = 0; i < LeakFile->Count; i++) {

		Entry = &LeakFile->Leak[i];
		Record = &StackRecord[Entry->StackId];

		fprintf(fp, "stackid %u count %u \n",  Entry->StackId, Entry->Count);

		for(j = 0; j < Record->Depth; j++) {
			Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[j]);
			ASSERT(Text != NULL);
			fprintf(fp, "#%02u: 0x%I64x %s\n", j, Record->Frame[j], Text->Text);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");

	ApsDestroySymbolTable(Table);
	return 0;
}

ULONG
ApsDebugValidateReport(
	__in PWSTR Path
	)
{
	ULONG Status;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PPF_REPORT_HEAD Head;
	FILE *fp;
	WCHAR Buffer[MAX_PATH];

	Status = APS_STATUS_OK;

	FileHandle = CreateFile(Path, GENERIC_READ, 0, NULL, 
		                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, 0);
	if (!MappingHandle) {
		Status = GetLastError();
		CloseHandle(FileHandle);
		return Status;
	}

	Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ, 0, 0, 0);
	if (!Head) {
		Status = GetLastError();
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return Status;
	}

	_wsplitpath(Path, NULL, NULL, Buffer, NULL);
	wcscat_s(Buffer, MAX_PATH, L".vrf.txt");

	fp = _wfopen(Buffer, L"w");
	if (!fp) {
		Status = GetLastError();	
		UnmapViewOfFile(Head);
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return Status;
	}

	//
	// Dump dll stream
	//

	ApsDebugDumpDll(Head, fp);

	//
	// Dump all back traces
	//

	ApsDebugDumpBackTrace(Head, fp);

	ApsDebugDumpHeapLeak(Head, fp);
	ApsDebugDumpPageLeak(Head, fp);
	ApsDebugDumpHandleLeak(Head, fp);
	ApsDebugDumpGdiLeak(Head, fp);

	fclose(fp);
	
	UnmapViewOfFile(Head);
	CloseHandle(MappingHandle);
	CloseHandle(FileHandle);
	return Status;
}

ULONG
ApsDebugValidateReportEx(
	__in PPF_REPORT_HEAD Head,
	__in PWSTR Path
	)
{
	ULONG Status;
	FILE *fp;
	WCHAR Buffer[MAX_PATH];

	Status = APS_STATUS_OK;

	_wsplitpath(Path, NULL, NULL, Buffer, NULL);
	wcscat_s(Buffer, MAX_PATH, L".vrf.txt");

	fp = _wfopen(Buffer, L"w");
	if (!fp) {
		Status = GetLastError();	
		return Status;
	}

	//
	// Dump dll stream
	//

	ApsDebugDumpDll(Head, fp);

	//
	// Dump all back traces
	//

	ApsDebugDumpBackTrace(Head, fp);

	ApsDebugDumpHeapLeak(Head, fp);
	ApsDebugDumpPageLeak(Head, fp);
	ApsDebugDumpHandleLeak(Head, fp);
	ApsDebugDumpGdiLeak(Head, fp);

	fclose(fp);
	return Status;
}

SIZE_T
ApsBuildReportFullName(
	__in PWSTR ProcessName,
	__in ULONG ProcessId,
	__in BTR_PROFILE_TYPE Type,
	__out PWCHAR Name 
	)
{
    SYSTEMTIME Time;
    WCHAR ProfileType[MAX_PATH];

	ApsCleanReport();

    //if (ApsCurrentReportFullName) {
    //    StringCchCopy(Name, MAX_PATH, ApsCurrentReportFullName);
    //    return wcslen(Name);
    //}

    switch (Type) {
    case PROFILE_CPU_TYPE:
        StringCchCopy(ProfileType, 16, L"CPU");
        break;
    case PROFILE_MM_TYPE:
        StringCchCopy(ProfileType, 16, L"MM");
        break;
    case PROFILE_IO_TYPE:
        StringCchCopy(ProfileType, 16, L"IO");
        break;
    case PROFILE_CCR_TYPE:
        StringCchCopy(ProfileType, 16, L"CCR");
        break;
    default:
        StringCchCopy(ProfileType, 16, L"ANY");
        break;
    }

    GetLocalTime(&Time);

    if (!ApsCurrentReportBaseName) {
        ApsCurrentReportBaseName = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
        StringCchPrintf(ApsCurrentReportBaseName, MAX_PATH,
                        L"%s_PID-%u_%s_%04d-%02d-%02d_%02d-%02d-%02d.dpf",
                        ProcessName, ProcessId, ProfileType, Time.wYear, Time.wMonth,
                        Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);
    }

    StringCchPrintf(Name, MAX_PATH, L"%s\\%s", ApsGetReportPath(), ApsCurrentReportBaseName); 

    //
    // Cache the full name in ApsCurrentReportFullName
    //

    ApsCurrentReportFullName = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
    StringCchCopy(ApsCurrentReportFullName, MAX_PATH, Name);

    return wcslen(Name);
}

VOID
ApsSetReportName(
	__in PWSTR Name
	)
{
	WCHAR BaseName[MAX_PATH];
	WCHAR ExtName[MAX_PATH];

	_wsplitpath(Name, NULL, NULL, BaseName, ExtName);

	ApsCurrentReportBaseName = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
	ApsCurrentReportFullName = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));

	//
	// Copy report base name
	//

	StringCchCopy(ApsCurrentReportBaseName, MAX_PATH, BaseName);
	if (ExtName[0] != 0) {
		StringCchCat(ApsCurrentReportBaseName, MAX_PATH, ExtName);
	}

	//
	// Copy report full name
	//

	StringCchCopy(ApsCurrentReportFullName, MAX_PATH, Name);
}

PWSTR
ApsGetCurrentReportBaseName(
	VOID
	)
{
	return ApsCurrentReportBaseName;
}

PWSTR
ApsGetCurrentReportFullName(
	VOID
	)
{
	return ApsCurrentReportFullName;
}

VOID
ApsCleanReportBaseName(
	VOID
	)
{
	if (!ApsCurrentReportBaseName) {
		return;
	} 
	
	ApsFree(ApsCurrentReportBaseName);
	ApsCurrentReportBaseName = NULL;
}

VOID
ApsCleanReportFullName(
	VOID
	)
{
	if (!ApsCurrentReportFullName) {
		return;
	} 
	
	ApsFree(ApsCurrentReportFullName);
	ApsCurrentReportFullName = NULL;
}
VOID
ApsCleanReport(
	VOID
	)
{
	ApsCleanReportBaseName();
	ApsCleanReportFullName();
}
