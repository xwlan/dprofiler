//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "apsdefs.h"
#include "apspri.h"
#include "apsctl.h"
#include "apsbtr.h"
#include "aps.h"
#include "apsfile.h"
#include "aps.h"
#include "ntapi.h"
#include "apspdb.h"
#include "apsrpt.h"
#include "apscpu.h"
#include "cpuprof.h"
#include "apsanalysis.h"


ULONG CALLBACK
ApsAnalysisProcedure(
    __in PVOID Argument 
    )
{
    ULONG Status;
    PAPS_ANALYSIS_CONTEXT Context;
    PAPS_CALLBACK Callback;
    
    Status = APS_STATUS_OK;
    Context = (PAPS_ANALYSIS_CONTEXT)Argument;

    ASSERT(Context->ReportPath != NULL);
    ASSERT(Context->SymbolPath != NULL);

    //
    // N.B. Currently CPU, MM use different analysis routines
    // Is it necessary to unify them into a single one ?
    //

    if (Context->Type == PROFILE_CPU_TYPE) {
		Status = ApsWriteCpuCounterStreams(Context->ReportPath);
    }

    else if (Context->Type == PROFILE_MM_TYPE) {
		Status = ApsWriteMmCounterStreams(Context->ReportPath);
    }

    else {
        ASSERT(0);
    } 
    
	if (Status != APS_STATUS_OK) {
		
		//
		// Log the error
		//

	}

	//
	// Load symbols to generate analysis streams
	//

	Status = ApsWriteAnalysisStreamsEx(Context->ReportPath,
		                               ApsPsuedoHandle, 
                                       Context->SymbolPath,
									   Context->Type);
	if (Status != APS_STATUS_OK) {

		//
		// Log the error
		//
	}

    //
    // Pass callback status and execute callback routine
    //

    Callback = &Context->Callback;
    if (Callback->Callback != NULL) { 

        __try {

            Callback->Status = Status;
            (*Callback->Callback)(Callback->Context);

        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
			
			//
			// Ingore exception from callback
			//

        }
    }

    return Status;
}

VOID
ApsNullDuplicatedPc(
    __in PVOID *Source,
    __in LONG Depth,
    __in PVOID *Destine
    )
{
    LONG i;
    LONG j;

    for(i = Depth - 1; i >= 0; i -= 1) {

        Destine[i] = Source[i];

        //
        // Scan the copied Pc to check whether it's duplicated
        //

        for(j = i + 1; j < Depth; j += 1) {
            if (Destine[j] == Source[i]) {
                Destine[i] = NULL;
                break;
            }
        }

    } 
}

int __cdecl
ApsDeduplicateFunctionCallback(
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
		// High depth precede low depth
		//

		Result = -((int)M1->Depth - (int)M2->Depth);
	}

	return Result;
}

//
// Deduplicate fuction pc to avoid inclusive
// percent overflow 100%, note that the PcTable
// is built from STREAM_PC, DllId, FunctionId,
// and LineId is already filled
//

ULONG
ApsDeduplicateFunction(
	__in PBTR_STACK_RECORD Record,
	__in PAPS_PC_METRICS Metrics,
	__in PBTR_PC_TABLE PcTable,
	__in PBTR_FUNCTION_ENTRY FuncTable,
	__in PBTR_TEXT_TABLE TextTable,
	__out PULONG_PTR Pc0,
	__out PULONG_PTR Pc1
	)
{
	ULONG Number;
	PBTR_PC_ENTRY Pc;
	PBTR_TEXT_ENTRY TextEntry;

	//RtlZeroMemory(Metrics, sizeof(APS_PC_METRICS) * Record->Depth);

	for(Number = 0; Number < Record->Depth; Number += 1) {

		Metrics[Number].Depth = Number;

		//
		// Lookup the Pc entry
		//

		Pc = ApsLookupPcEntry(Record->Frame[Number], PcTable);
		ASSERT(Pc != NULL);

		Metrics[Number].FunctionId = Pc->FunctionId;

		if (Pc->FunctionId != -1) {
			Metrics[Number].FunctionAddress = (PVOID)FuncTable[Pc->FunctionId].Address;
		} else {
			Metrics[Number].FunctionAddress = Pc->Address;
		}
		
		Metrics[Number].DllId = Pc->DllId;	

		if (Pc->FunctionId != -1) {
			TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Metrics[Number].FunctionAddress);
			Metrics[Number].FunctionLineId = TextEntry->LineId;
		}
		else {
			Metrics[Number].FunctionLineId = -1;
		}

		if (Number == 0) {
			Metrics[Number].FunctionInclusive = 0;
			Metrics[Number].FunctionExclusive = 1;
		} else {
			Metrics[Number].FunctionInclusive = 1;
			Metrics[Number].FunctionExclusive = 0;
		}
	}

	//
	// Output Pc0 and Pc1 the top 2 stack frames
	//

	*Pc0 = (ULONG_PTR)Metrics[0].FunctionAddress;
	*Pc1 = (ULONG_PTR)Metrics[1].FunctionAddress;

	//
	// Sort the metrics entries and clear duplicated functions
	//

	qsort(Metrics, Record->Depth, sizeof(APS_PC_METRICS), ApsDeduplicateFunctionCallback);
	for(Number = 0; Number < Record->Depth - 1; Number += 1) {
		if (Metrics[Number].FunctionId == Metrics[Number + 1].FunctionId) {
			Metrics[Number + 1].FunctionInclusive = 0;
		}
	}

	return APS_STATUS_OK;
}

ULONG
ApsCreatePcStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *Table
	)
{
	PBTR_STACK_RECORD Stack;
	PBTR_STACK_RECORD Record;
	ULONG Count;
	PBTR_PC_TABLE PcTable;
	ULONG Number;
	LONG Depth;
	ULONG Inclusive;
	ULONG Exclusive;
	PBTR_PC_ENTRY Pc;
    PVOID Frame[MAX_STACK_DEPTH];

	Inclusive = 0;
	Exclusive = 0;

	Stack = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

	ApsCreatePcTable(&PcTable);

	for(Number = 0; Number < Count; Number += 1) {
		
		Record = &Stack[Number];

        //
        // N.B. Null duplicated PC to avoid recusive calls overflow counters,
        // every duplicated PC only reserve at its maximum depth at stack trace 
        //

        ApsNullDuplicatedPc(&Record->Frame[0], MAX_STACK_DEPTH, &Frame[0]);

        //
        // N.B. Scan the frame pointer from stack bottom to top
        //

        for(Depth = Record->Depth - 1; Depth >= 0; Depth -= 1) {

            //
            // Inclusive and exclusive counters are applicable only to CPU profile
            //

            if (Head->IncludeCpu) {
                ApsInsertPcEntry(PcTable, Frame[Depth], Record->Count, 
                                 Record->Count * (Depth + 1), 0);
            }
            else if (Head->IncludeMm) {
                ApsInsertPcEntry(PcTable, Frame[Depth], 0, 0, 0);
            }
            else {
                ASSERT(0);
            }

		}

        if (Head->IncludeCpu) {
            Exclusive += Record->Count * Record->Depth;
            Inclusive += Record->Count * Record->Depth;
        }

	}

	//
	// Insert a fake PC entry to track total inclusive and exclusive counters,
	// chain it in pc table's slot 0, position 0.
	//

	Pc = (PBTR_PC_ENTRY)ApsMalloc(sizeof(BTR_PC_ENTRY));
	RtlZeroMemory(Pc, sizeof(BTR_PC_ENTRY));

	Pc->Inclusive = Inclusive;
	Pc->Exclusive = Exclusive;
	Pc->Address = (PVOID)-1;

	InsertHeadList(&PcTable->Hash[0].ListHead, &Pc->ListEntry);
	PcTable->Count += 1;

	*Table = PcTable;
	return APS_STATUS_OK;
}

ULONG
ApsWriteAnalysisStreams(
	__in PWSTR Path,
	__in HANDLE ProcessHandle,
	__in PWSTR SymbolPath
	)
{
	ULONG i;
	ULONG Status;
	PPF_REPORT_HEAD Head;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PBTR_PC_ENTRY PcEntry;
	ULONG PcCount;
	PBTR_DLL_FILE DllFile;
	PBTR_TEXT_TABLE TextTable;
	PBTR_FUNCTION_TABLE FuncTable;
	PBTR_LINE_TABLE LineTable;
	WCHAR BackPath[MAX_PATH * 2];

	//
	// Map the profile report
	//

	Status = ApsMapReportFile(Path, &Head, &MappingHandle, &FileHandle);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	if (!Head->Complete) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_BAD_FILE;
	}

	//
	// N.B. This routine requires that the counters are already created
	//

	if (!Head->Counters) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_NO_COUNTERS;
	}

	//
	// Backup report file
	//

	StringCchPrintf(BackPath, MAX_PATH * 2, L"%s.bak", Path);
	CopyFile(Path, BackPath, FALSE);

	__try {

		//
		// N.B. Skip first Pc entry which contains summary 
		//

		PcEntry = (PBTR_PC_ENTRY)ApsGetStreamPointer(Head, STREAM_PC);
		PcEntry += 1;

		PcCount = (ULONG)Head->Streams[STREAM_PC].Length / sizeof(BTR_PC_ENTRY);
		PcCount -= 1;

		//
		// Get dllfile, create text table, function table, line table
		//

		DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
		ApsCreateTextTable(ApsPsuedoHandle, 4093, &TextTable);
		ApsCreateFunctionTable(&FuncTable);
		ApsCreateLineTable(&LineTable);

		//
		// Initialize dbghelp and load all symbols
		//

		ApsLoadSymbols(Head, ProcessHandle, SymbolPath);

		//
		// Decode Pc to get distribution of Pcs in function and dlls.
		//

		for(i = 0; i < PcCount; i++) {
			ApsDecodePc(ApsPsuedoHandle, PcEntry, FuncTable, DllFile, TextTable, LineTable);
			PcEntry += 1;
		}

		//
		// Position file pointer to its end
		//

		GetFileSizeEx(FileHandle, &Start);
		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

		//
		// Write function stream and symbol stream
		//

		Status = ApsWriteFunctionStream(Head, FileHandle, FuncTable, TextTable, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Head->Streams[STREAM_FUNCTION].Offset = Start.QuadPart;
		Head->Streams[STREAM_FUNCTION].Length = End.QuadPart - Start.QuadPart;
		Start = End;

		//
		// Write symbol stream
		//

		Status = ApsWriteSymbolStream(Head, FileHandle, TextTable, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Head->Streams[STREAM_SYMBOL].Offset = Start.QuadPart;
		Head->Streams[STREAM_SYMBOL].Length = End.QuadPart - Start.QuadPart;
		Start = End;

		//
		// Write line stream 
		//

		Status = ApsWriteLineStream(Head, FileHandle, LineTable, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Head->Streams[STREAM_LINE].Offset = Start.QuadPart;
		Head->Streams[STREAM_LINE].Length = End.QuadPart - Start.QuadPart;

		//
		// Mark as analyzed and update checksum
		//

		Head->Analyzed = 1;
		ApsComputeStreamCheckSum(Head);

		SetEndOfFile(FileHandle);
	}

	__finally {

		ApsDestroyFunctionTable(FuncTable);
		ApsDestroyLineTable(LineTable);
		ApsDestroySymbolTable(TextTable);

		ApsUnloadAllSymbols(ProcessHandle);
	}

	ApsUnmapReportFile(Head, MappingHandle, FileHandle);

	if (Status != APS_STATUS_OK) {
		DeleteFile(Path);
		CopyFile(BackPath, Path, FALSE);

	} else {
		DeleteFile(BackPath);
	}
	
	return Status;
}

VOID
ApsCheckStackRecords(
    __in PBTR_STACK_RECORD Record,
    __in ULONG Count
    )
{
    ULONG Number;
    ULONG i;

    for(Number = 0; Number < Count; Number += 1) {
        ASSERT(Record[Number].Committed == 1);
        ASSERT(Record[Number].Depth <= MAX_STACK_DEPTH);
        for(i = 0; i < Record[Number].Depth; i++) {
            ASSERT(Record[Number].Frame[i] != NULL);
        }
    }
}

ULONG
ApsWriteAnalysisStreamsEx(
	__in PWSTR Path,
	__in HANDLE ProcessHandle,
	__in PWSTR SymbolPath,
	__in BTR_PROFILE_TYPE Type
	)
{
	ULONG i;
	ULONG Status;
	PPF_REPORT_HEAD Head;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	ULONG Count;
	PBTR_STACK_RECORD Record;
	PBTR_PC_TABLE PcTable;
	PBTR_DLL_FILE DllFile;
	PBTR_TEXT_TABLE TextTable;
	PBTR_FUNCTION_TABLE FuncTable;
	PBTR_LINE_TABLE LineTable;
	WCHAR BackPath[MAX_PATH * 2];

	//
	// Map the profile report
	//

	Status = ApsMapReportFile(Path, &Head, &MappingHandle, &FileHandle);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	if (!Head->Complete) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_BAD_FILE;
	}

	//
	// N.B. This routine requires that the counters are already created
	//

	if (!Head->Counters) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_NO_COUNTERS;
	}

	//
	// Backup report file
	//

	StringCchPrintf(BackPath, MAX_PATH * 2, L"%s.bak", Path);
	CopyFile(Path, BackPath, FALSE);

	__try {

		//
		// Create pc, text, function and line tables
		//

		DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
		Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
		Count = ApsGetStreamLength(Head, STREAM_STACK) / sizeof(BTR_STACK_RECORD);

        ApsCheckStackRecords(Record, Count);

		ApsCreatePcTable(&PcTable);
		ApsCreateTextTable(ApsPsuedoHandle, 4093, &TextTable);
		ApsCreateFunctionTable(&FuncTable);
		ApsCreateLineTable(&LineTable);

		//
		// Initialize dbghelp and load all symbols
		//

		ApsLoadSymbols(Head, ProcessHandle, SymbolPath);

		//
		// Decode Pc to get distribution of Pcs in function and dlls.
		//

		for(i = 0; i < Count; i++) {
			ApsDecodePcByStackTrace(ApsPsuedoHandle, Record, PcTable, FuncTable, 
				                    DllFile, TextTable, LineTable, Type);
			Record += 1;
		}

		//
		// Position file pointer to its end
		//

		GetFileSizeEx(FileHandle, &Start);
		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

		//
		// Write PC streams
		//

		Status = ApsWritePcStreamEx(Head, PcTable, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}
		
		Head->Streams[STREAM_PC].Offset = Start.QuadPart;
		Head->Streams[STREAM_PC].Length = End.QuadPart - Start.QuadPart;
		Start = End;

		//
		// Write function stream and symbol stream
		//

		Status = ApsWriteFunctionStream(Head, FileHandle, FuncTable, TextTable, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Head->Streams[STREAM_FUNCTION].Offset = Start.QuadPart;
		Head->Streams[STREAM_FUNCTION].Length = End.QuadPart - Start.QuadPart;
		Start = End;

		//
		// Write symbol stream
		//

		Status = ApsWriteSymbolStream(Head, FileHandle, TextTable, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Head->Streams[STREAM_SYMBOL].Offset = Start.QuadPart;
		Head->Streams[STREAM_SYMBOL].Length = End.QuadPart - Start.QuadPart;
		Start = End;

		//
		// Write line stream 
		//

		Status = ApsWriteLineStream(Head, FileHandle, LineTable, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Head->Streams[STREAM_LINE].Offset = Start.QuadPart;
		Head->Streams[STREAM_LINE].Length = End.QuadPart - Start.QuadPart;

		//
		// Mark as analyzed and update checksum
		//

		Head->Analyzed = 1;
		ApsComputeStreamCheckSum(Head);

		SetEndOfFile(FileHandle);
	}

	__finally {

		ApsDestroyPcTable(PcTable);
		ApsDestroyFunctionTable(FuncTable);
		ApsDestroyLineTable(LineTable);
		ApsDestroySymbolTable(TextTable);

		ApsUnloadAllSymbols(ProcessHandle);
	}

	ApsUnmapReportFile(Head, MappingHandle, FileHandle);

	if (Status != APS_STATUS_OK) {
		DeleteFile(Path);
		CopyFile(BackPath, Path, FALSE);

	} else {
		DeleteFile(BackPath);
	}
	
	return Status;
}

ULONG
ApsMapReportFile(
	__in PWSTR Path,
	__out PPF_REPORT_HEAD *Head,
	__out PHANDLE MappingObject,
	__out PHANDLE FileObject
	)
{
	ULONG Status;
	PPF_REPORT_HEAD Report;
	HANDLE FileHandle;
	HANDLE MappingHandle;

	FileHandle = NULL;
	MappingHandle = NULL;
	Report = NULL;

	*MappingObject = NULL;
	*FileObject = NULL;
	*Head = NULL;

	__try {

		Status = APS_STATUS_OK;

		//
		// Enable shared read 
		//

		FileHandle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, 
			                    FILE_SHARE_READ, NULL, OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL, NULL);

		if (FileHandle == INVALID_HANDLE_VALUE) {
			Status = GetLastError();
			__leave;
		}

		MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (!MappingHandle) {
			Status = GetLastError();
			__leave;
		}

		Report = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		if (!Report) {
			Status = GetLastError();
			__leave;
		}

	}
	__finally {

		if (Status != APS_STATUS_OK) {

			if (Head) {
				UnmapViewOfFile(Head);
			}

			if (MappingHandle) {
				CloseHandle(MappingHandle);
			}

			if (FileHandle) {
				CloseHandle(FileHandle);
			}
		}
		else {
		
			*MappingObject = MappingHandle;
			*FileObject = FileHandle;
			*Head = Report;
		}
	}

	return Status;
}

VOID
ApsUnmapReportFile(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE MappingObject,
	__in HANDLE FileObject
	)
{
	UnmapViewOfFile(Head);
	CloseHandle(MappingObject);
	CloseHandle(FileObject);
}

ULONG
ApsWriteCpuCounterStreams(
	__in PWSTR Path
	)
{
	ULONG Status;
	PPF_REPORT_HEAD Head;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PBTR_PC_TABLE PcTable = NULL;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	WCHAR BackPath[MAX_PATH * 2];

	//
	// Map profile report file
	//

	Status = ApsMapReportFile(Path, &Head, &MappingHandle, &FileHandle);
	if (Status != APS_STATUS_OK) {
		return Status;
	}
	
    //
    // Check whether counters already created 
    //

    ASSERT(Head->Complete != 0);

	if (!Head->Complete) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_BAD_FILE;
	}

    if (Head->Counters != 0) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_OK;
	}

	//
	// Back report file, if it exists, force a overwrite
	//

	StringCchPrintf(BackPath, MAX_PATH * 2, L"%s.bak", Path);
	CopyFile(Path, BackPath, FALSE);

	__try {

		//
		// Set file pointer to its end
		//

		GetFileSizeEx(FileHandle, &Start);
		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
        End = Start;

		//
		// Mark counter bit and update checksum
		// FileSize is indeed file size does not
		// include symbol streams
		//

	    Head->Counters = 1;
		Head->FileSize = End.QuadPart;
	    ApsComputeStreamCheckSum(Head);
		
		SetEndOfFile(FileHandle);
	}

	__finally {

	}

	ApsUnmapReportFile(Head, MappingHandle, FileHandle);

	if (Status != APS_STATUS_OK) {
		DeleteFile(Path);
		CopyFile(BackPath, Path, FALSE);
	}

	DeleteFile(BackPath);
	return APS_STATUS_OK;
}

ULONG
ApsWriteMmCounterStreams(
	__in PWSTR Path
	)
{
	ULONG Status;
	PPF_REPORT_HEAD Head;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PBTR_PC_TABLE PcTable;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	WCHAR BackPath[MAX_PATH * 2];

	//
	// Map profile report file
	//

	Status = ApsMapReportFile(Path, &Head, &MappingHandle, &FileHandle);
	if (Status != APS_STATUS_OK) {
		return Status;
	}
	
    //
    // Check whether counters already created 
    //

    ASSERT(Head->Complete != 0);

	if (!Head->Complete) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_BAD_FILE;
	}

    if (Head->Counters != 0) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_OK;
	}

	//
	// Back report file, if it exists, force a overwrite
	//

	StringCchPrintf(BackPath, MAX_PATH * 2, L"%s.bak", Path);
	CopyFile(Path, BackPath, FALSE);

	__try {

		//
		// Set file pointer to its end
		//

		GetFileSizeEx(FileHandle, &Start);
		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

		//
		// Write the caller stream this is a special PC stream which contains only
		// directly caller of probe callback.
		//

		ApsCreateMmPcStream(Head, &PcTable);

		Status = ApsWriteCallerStream(Head, PcTable, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		} 

		ApsDestroyPcTable(PcTable);
		PcTable = NULL;

		Head->Streams[STREAM_CALLER].Offset = Start.QuadPart;
		Head->Streams[STREAM_CALLER].Length = End.QuadPart - Start.QuadPart;
		Start = End;

		//
		// Heap leak 
		//

		Status = ApsWriteHeapLeakStream(Head, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Start = End;

		//
		// Page leak
		//

		Status = ApsWritePageLeakStream(Head, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Start = End;

		//
		// Handle leak
		//

		Status = ApsWriteHandleLeakStream(Head, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		Start = End;

		//
		// GDI leak
		//

		Status = ApsWriteGdiLeakStream(Head, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		}

		//
		// Hot heap by module
		//

		Status = ApsUpdateHotHeapCounters(Head);
		if (Status != APS_STATUS_OK) {
			__leave;
		}
		
		//
		// Mark counter bit and update checksum
		// FileSize is indeed size does not include
		// symbol streams.
		//

	    Head->Counters = 1;
		Head->FileSize = End.QuadPart;
	    ApsComputeStreamCheckSum(Head);
		
		SetEndOfFile(FileHandle);
	}
	__finally {

		if (Status != APS_STATUS_OK) {
			if (PcTable != NULL) {
				ApsDestroyPcTable(PcTable);
			}
		}
	}

	ApsUnmapReportFile(Head, MappingHandle, FileHandle);

	if (Status != APS_STATUS_OK) {
		DeleteFile(Path);
		CopyFile(BackPath, Path, FALSE);
	}

	DeleteFile(BackPath);
	return APS_STATUS_OK;
}