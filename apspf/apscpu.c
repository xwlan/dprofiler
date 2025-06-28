//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
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
#include "apsprofile.h"

ULONG
CpuCreateCounterStreams(
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
	ULONG Size;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PBTR_PC_TABLE PcTable;
	PBTR_PC_ENTRY PcEntry;
	ULONG PcCount;
	PBTR_DLL_FILE DllFile;
	PBTR_TEXT_TABLE TextTable;
	PBTR_FUNCTION_TABLE FuncTable;
	PBTR_LINE_TABLE LineTable;

	FileHandle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
	                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		Status = GetLastError();
		CloseHandle(FileHandle);
		return Status;
	}
	
	Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Head) {
		Status = GetLastError();
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return Status;
	}

    //
    // Check whether it's already analyzed
    //

    ASSERT(Head->Complete != 0);
    if (Head->Analyzed) {
        UnmapViewOfFile(Head);
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);
        return APS_STATUS_OK;
    }
	
	if (Head->Streams[STREAM_FUNCTION].Offset != 0 && 
		Head->Streams[STREAM_FUNCTION].Length != 0) {

		//
		// N.B. This indicate that the counter streams are already generated,
		// do nothing and return
		//

		UnmapViewOfFile(Head);
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return APS_STATUS_OK;
	}

	//
	// Create pc stream based on stack records
	//
	
	Status = ApsCreateCpuPcStream(Head, &PcTable);
	if (Status != APS_STATUS_OK) {
        UnmapViewOfFile(Head);
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);
		return Status;
	}
	
	Size = GetFileSize(FileHandle, NULL);
	Start.QuadPart = Size;
	SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

	Status = ApsWritePcStream(Head, PcTable, FileHandle, Start, &End);
	if (Status != APS_STATUS_OK) {
        UnmapViewOfFile(Head);
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);
		return Status;
	} 

	Head->Streams[STREAM_PC].Offset = Start.QuadPart;
	Head->Streams[STREAM_PC].Length = End.QuadPart - Start.QuadPart;

	//
	// Close and remap again
	//

	UnmapViewOfFile(Head);
	CloseHandle(MappingHandle);

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		Status = GetLastError();
		CloseHandle(FileHandle);
		return Status;
	}
	
	Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Head) {
		Status = GetLastError();
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return Status;
	}

	//
	// N.B. Skip root pc entry which contains counters
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
	// Write function stream and symbol stream
	//
	
	GetFileSizeEx(FileHandle, &Start);

	ApsWriteFunctionStream(Head, FileHandle, FuncTable, TextTable, Start, &End);

	Head->Streams[STREAM_FUNCTION].Offset = Start.QuadPart;
	Head->Streams[STREAM_FUNCTION].Length = End.QuadPart - Start.QuadPart;

	Start.QuadPart = End.QuadPart;
	ApsWriteSymbolStream(Head, FileHandle, TextTable, Start, &End);
	
	//
	// Write line stream if available
	//

	if (LineTable->Count != 0) {

		Start.QuadPart = End.QuadPart;
		ApsWriteLineStream(Head, FileHandle, LineTable, Start, &End);

		Head->Streams[STREAM_LINE].Offset = Start.QuadPart;
		Head->Streams[STREAM_LINE].Length = End.QuadPart - Start.QuadPart;
	}

	SetEndOfFile(FileHandle);

    //
    // Mark as analyzed and update checksum
    //

    Head->Counters = 1;
    Head->Analyzed = 1;
    ApsComputeStreamCheckSum(Head);

	//
	// Destroy function table and text table
	//

	ApsDestroyFunctionTable(FuncTable);
	ApsDestroyLineTable(LineTable);
	ApsDestroySymbolTable(TextTable);

	UnmapViewOfFile(Head);
    CloseHandle(MappingHandle);
	CloseHandle(FileHandle);

	ApsCleanDbghelp(ProcessHandle);
	return APS_STATUS_OK;
}

ULONG
ApsCreateCpuProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	PAPS_PROFILE_OBJECT Profile;

	*Object = NULL;

	//
	// Validate profiling attributes
	//

	Attribute->Type = PROFILE_CPU_TYPE;
	Attribute->Mode = RECORD_MODE;

	Status = CpuCheckAttribute(Attribute);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Open target process object
	//

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}

	//
	// Create profile object
	//

	Status = ApsCreateProfile(&Profile, RECORD_MODE, PROFILE_CPU_TYPE, 
		                      ProcessId, ProcessHandle, ReportPath);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Copy attribute to local profile object
	//

	ASSERT(Profile != NULL);
	memcpy(&Profile->Attribute, Attribute, sizeof(BTR_PROFILE_ATTRIBUTE));

	*Object = Profile;
	return Status;
}

ULONG
CpuCheckAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	if (Attr->StackDepth > MAX_STACK_DEPTH || Attr->StackDepth < 1) {
		return APS_STATUS_INVALID_PARAMETER;
	}

	if (Attr->SamplingPeriod < MINIMUM_SAMPLE_PERIOD) {
		return APS_STATUS_INVALID_PARAMETER;
	}

	return APS_STATUS_OK;
}

PCPU_THREAD_TABLE
CpuCreateThreadTable(
	VOID
	)
{
	PCPU_THREAD_TABLE Table;
	ULONG Number;

	Table = (PCPU_THREAD_TABLE)ApsMalloc(sizeof(CPU_THREAD_TABLE));
	for(Number = 0; Number < CPU_THREAD_BUCKET; Number++) {
		InitializeListHead(&Table->ListHead[Number]);
	}	

    Table->KernelTime = 0;
    Table->UserTime = 0;
	Table->Count = 0;
    Table->Thread = NULL;
	return Table;
}

ULONG
CpuScanThreadedPc(
    __in PPF_REPORT_HEAD Head,
    __in PCPU_THREAD_TABLE Table
    )
{
    PBTR_FILE_INDEX Index;
    PBTR_CPU_RECORD Record;
    PBTR_CPU_SAMPLE Sample;
    ULONG Number;
    ULONG Count;
	ULONG ThreadCount;
	ULONG i;
	PCPU_THREAD Thread;
	PBTR_STACK_RECORD StackTrace;
	PBTR_STACK_RECORD StackFile;
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_PC_TABLE PcTable;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_CPU_THREAD BtrThread;
	ULONG_PTR Pc0, Pc1;
	PCPU_HISTORY History;
	APS_PC_METRICS Metrics[MAX_STACK_DEPTH];

    //
    // Require PC stream is available
    //

    if (!Head->Streams[STREAM_PC].Offset) {  
		return APS_STATUS_ERROR;
	}

    if (!Head->Streams[STREAM_INDEX].Offset) {
		return APS_STATUS_ERROR;
    }

    if (!Head->Streams[STREAM_RECORD].Offset) {
		return APS_STATUS_ERROR;
    }

    Index = (PBTR_FILE_INDEX)ApsGetStreamPointer(Head, STREAM_INDEX);
    Record = (PBTR_CPU_RECORD)ApsGetStreamPointer(Head, STREAM_RECORD);
    Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));
    
	StackFile = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	ASSERT(StackFile != NULL);

	//
	// Build symbol table
	//

	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	//
	// Get DLL entry base address 
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
	DllEntry = &DllFile->Dll[0];

	//
	// Get function entry base address
	//

	FuncEntry = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);

	//
	// Get line entry base address
	//

	LineEntry = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE);

	ApsCreatePcTableFromStream(Head, &PcTable);

	//
	// Create CPU history
	//

	History = (PCPU_HISTORY)ApsMalloc(sizeof(CPU_HISTORY));
	History->MaximumValue = 100.0;
	History->MinimumValue = 0.0;
	History->Limits = 100.0;
	History->Value = (PFLOAT)ApsMalloc(sizeof(FLOAT)*Count);
	History->Count = Count;
	Table->History = History;

    for(Number = 0; Number < Count; Number += 1) {

        //
        // Get next CPU profiling record
        //

		Record = CpuGetNextCpuRecord(Record, Number);

		//
		// N.B. We temporarily skip mark record to avoid break comptability
		// with old implementation of threaded PC statistics.
		//

		if (Record->Flag & BTR_FLAG_CPU_MARKER) {
			continue;
		}

        //ApsDebugTrace("cpur #%u: %.03f", Number, Record->CpuUsage);
        Table->KernelTime += Record->KernelTime;
        Table->UserTime += Record->UserTime;

		//
		// Set CPU usage of current record
		//

		History->Value[Number] = (float)Record->CpuUsage;
		//ApsDebugTrace("#%d CPU %: %.2f", Number, Record->CpuUsage);

        //
        // Get number of total threads include retired ones
        //

		ThreadCount = Record->ActiveCount + Record->RetireCount;

		for(i = 0; i < ThreadCount; i++) {

            //
            // For each thread, update its time counters
            //

			Sample = &Record->Sample[i];
			Thread = CpuLookupCpuThread(Table, Sample->ThreadId);

			if (!Thread) {

                PCPU_THREAD_PC_TABLE ThreadPcTable;
                PCPU_STACK_TABLE StackTable;

                //
                // Lookup the BTR_CPU_THREAD object, if not found,
                // then the sample is invalid, dropped it.
                //

				BtrThread = CpuGetBtrThreadObject(Head, Sample->ThreadId);
                if (!BtrThread) {
                    continue;
                }

				Thread = CpuAllocateCpuThread(Sample->ThreadId);
                ThreadPcTable = CpuCreateThreadPcTable();
                Thread->Table = ThreadPcTable;

                StackTable = CpuCreateThreadStackTable();
                Thread->StackTable = StackTable;
                StackTable->ThreadId = Sample->ThreadId;

				//
				// Create per thread state history
				//

				CpuCreateThreadStateHistory(Thread, BtrThread);

                if (i < Record->ActiveCount) {
                    Thread->State = CPU_THREAD_STATE_RUNNING;
                } else {
                    Thread->State = CPU_THREAD_STATE_RETIRED;
                }

				CpuInsertCpuThread(Table, Thread);

            }

			Thread->KernelTime += Sample->KernelTime;
			Thread->UserTime += Sample->UserTime;
			Thread->Cycles += Sample->Cycles;

            //
            // For each thread, update its PC counters
            //
			
            StackTrace = &StackFile[Sample->StackId];

			ApsDeduplicateFunction(StackTrace, Metrics, PcTable, 
				                   FuncEntry,TextTable, &Pc0, &Pc1);

			//for(j = 0; j < StackTrace->Depth; j++) {
			//	//CpuInsertThreadPc(Thread, StackTrace->Frame[j], j + 1);
			//	CpuInsertThreadPcEx(Thread, StackTrace->Frame[j], j + 1,
			//		                PcTable, DllEntry, FuncEntry, LineEntry);
			//}

			CpuInsertThreadFunction(Thread, Metrics, StackTrace->Depth);

			//
			// Track the thread's state for current sample
			// note that Metrics are sorted in bottom down order,
			// we need skip to its first stack top frames
			//
			
			CpuSetThreadSampleState(Thread, Pc0, Pc1);

            Thread->Exclusive += 1;

            //
            // Track the stack trace per thread
            //

            CpuInsertStackRecord(Thread, StackTrace);
		}
	}

    CpuSortThreads(Table);

	ApsDestroySymbolTable(TextTable);
	ApsDestroyPcTableFromStream(PcTable);
    return APS_STATUS_OK;
}


int __cdecl 
CpuThreadTableSortCallback(
	__in const void *Entry1,
	__in const void *Entry2 
	)
{
    PCPU_THREAD T1, T2;

    T1 = *(PCPU_THREAD *)Entry1;
    T2 = *(PCPU_THREAD *)Entry2;

    //
    // N.B. we need a decreasing order
    //

    //return T2->Inclusive - T1->Inclusive;
    return (T2->KernelTime + T2->UserTime) - (T1->KernelTime + T1->UserTime);
}

ULONG
CpuSortThreads(
    __in PCPU_THREAD_TABLE Table
    )
{ 
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_THREAD Thread;
    ULONG Number;
    ULONG Index;

    Table->Thread = (PCPU_THREAD *)ApsMalloc(sizeof(PCPU_THREAD) * Table->Count);

    for(Number = 0, Index = 0; Number < CPU_THREAD_BUCKET; Number += 1) {

        ListHead = &Table->ListHead[Number];
        ListEntry = ListHead->Flink;

        while (ListEntry != ListHead) {
            Thread = CONTAINING_RECORD(ListEntry, CPU_THREAD, ListEntry);
            Table->Thread[Index] = Thread;

            //
            // Meanwhile sort the stack table here 
            //

            CpuSortStackTable(Thread);
            
            ListEntry = ListEntry->Flink;
            Index += 1;
        }
    }
    
    qsort(Table->Thread, Table->Count, sizeof(PCPU_THREAD), CpuThreadTableSortCallback);
    return 0;
}

VOID
CpuInsertStackRecord(
    __in PCPU_THREAD Thread,
    __in PBTR_STACK_RECORD Record
    )
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_STACK_ENTRY Entry;
    PCPU_STACK_TABLE Table;

    Table = Thread->StackTable;
    ASSERT(Table != NULL);

    Bucket = Record->StackId % CPU_THREAD_BUCKET;
    ListHead = &Table->ListHead[Bucket];

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {
        Entry = CONTAINING_RECORD(ListEntry, CPU_STACK_ENTRY, ListEntry);
        if (Entry->StackId == Record->StackId) {
            Entry->Count += 1;
            return;
        }
        ListEntry = ListEntry->Flink;
    }

    //
    // It's a new record, allocate a new entry and insert into table
    //
    
    Entry = (PCPU_STACK_ENTRY)ApsMalloc(sizeof(CPU_STACK_ENTRY));
    Entry->Count = 1;
    Entry->StackId = Record->StackId;

    InsertHeadList(ListHead, &Entry->ListEntry);
    Table->Count += 1;
}

int __cdecl 
CpuStackTableSortCallback(
	__in const void *Entry1,
	__in const void *Entry2 
	)
{
    //
    // N.B. we need a decreasing order
    //

    return ((PCPU_STACK_ENTRY)Entry2)->Count - ((PCPU_STACK_ENTRY)Entry1)->Count;
}

VOID
CpuSortStackTable(
    __in PCPU_THREAD Thread
    )
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_STACK_ENTRY Entry;
    PCPU_STACK_TABLE Table;
    ULONG Number;
    ULONG Index;

    Table = Thread->StackTable;
    ASSERT(Table != NULL);

    if (!Table->Count)
        return;

    Index = 0;
    Table->Table = (PCPU_STACK_ENTRY)ApsMalloc(sizeof(CPU_STACK_ENTRY) * Table->Count);

    //
    // Remove entries from hash table and copy into array
    //

    for(Number = 0; Number < CPU_STACK_BUCKET; Number += 1) { 
        ListHead = &Table->ListHead[Number];
        while (IsListEmpty(ListHead) != TRUE) {
            ListEntry = RemoveHeadList(ListHead);
            Entry = CONTAINING_RECORD(ListEntry, CPU_STACK_ENTRY, ListEntry);
            Table->Table[Index] = *Entry;
            Index += 1;
        }
    }

    //
    // Sort the entries in descending order
    //

    qsort(Table->Table, Table->Count, sizeof(CPU_STACK_ENTRY), CpuStackTableSortCallback);
}

VOID
CpuInsertThreadPc(
	__in PCPU_THREAD Thread,
	__in PVOID Pc,
	__in ULONG Depth
	)
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_THREAD_PC_TABLE Table;
    PCPU_THREAD_PC PcEntry;

    Table = Thread->Table;
    ASSERT(Table != NULL);

    Bucket = ((ULONG_PTR)Pc) % CPU_PC_BUCKET;
    ListHead = &Table->ListHead[Bucket];

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {

        PcEntry = CONTAINING_RECORD(ListEntry, CPU_THREAD_PC, ListEntry);

        if (PcEntry->Pc == Pc) {

            if (Depth == 1) {
                PcEntry->Inclusive += 0;
                PcEntry->Exclusive += 1;
            } else {
                PcEntry->Inclusive += 1;
                PcEntry->Exclusive += 0;
            }

			PcEntry->Depth += Depth;

            //Table->Inclusive += Depth;
            //Table->Exclusive += 1;

            //Thread->Exclusive = Table->Exclusive;
            //Thread->Inclusive = Table->Inclusive;

            return;
        }

        ListEntry = ListEntry->Flink;

    }

    PcEntry = (PCPU_THREAD_PC)ApsMalloc(sizeof(CPU_THREAD_PC));

    PcEntry->Pc = Pc;

    if (Depth == 1) {
        PcEntry->Inclusive = 0;
        PcEntry->Exclusive = 1;
    } else {
        PcEntry->Inclusive = 1;
        PcEntry->Exclusive = 0;
    }

	PcEntry->Depth = Depth;

    //Table->Inclusive += Depth;
    //Table->Exclusive += 1;

    //Thread->Exclusive = Table->Exclusive;
    //Thread->Inclusive = Table->Inclusive;

    InsertTailList(ListHead, &PcEntry->ListEntry);
    Table->Count += 1;
}

VOID
CpuInsertThreadPcEx(
	__in PCPU_THREAD Thread,
	__in PVOID Address,
	__in ULONG Depth,
	__in PBTR_PC_TABLE PcTable,
	__in PBTR_DLL_ENTRY DllEntry,
	__in PBTR_FUNCTION_ENTRY FuncEntry,
	__in PBTR_LINE_ENTRY LineEntry
	)
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_THREAD_PC_TABLE Table;
    PCPU_THREAD_PC CpuPc;
    PBTR_PC_ENTRY Pc;
	PVOID FuncAddress;

	//
	// Decode the Pc as function
	//

	Pc = ApsLookupPcEntry(Address, PcTable);
	ASSERT(Pc != NULL);
	ASSERT(Pc->Address != NULL);

	if (Pc->FunctionId != -1) {
		FuncAddress = (PVOID)FuncEntry[Pc->FunctionId].Address;
	} else {
		FuncAddress = Address;
	}

    Table = Thread->Table;
    ASSERT(Table != NULL);

    Bucket = ((ULONG_PTR)FuncAddress) % CPU_PC_BUCKET;
    ListHead = &Table->ListHead[Bucket];

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {

        CpuPc = CONTAINING_RECORD(ListEntry, CPU_THREAD_PC, ListEntry);

		if (CpuPc->Pc == FuncAddress) {

            if (Depth == 1) {
                CpuPc->Inclusive += 0;
                CpuPc->Exclusive += 1;
            } else {
                CpuPc->Inclusive += 1;
                CpuPc->Exclusive += 0;
            }

			CpuPc->Depth += Depth;
            return;
        }

        ListEntry = ListEntry->Flink;

    }

	//
	// Insert Pc into thread Pc table
	//

    CpuPc = (PCPU_THREAD_PC)ApsMalloc(sizeof(CPU_THREAD_PC));
    CpuPc->Pc = FuncAddress;

    if (Depth == 1) {
        CpuPc->Inclusive = 0;
        CpuPc->Exclusive = 1;
    } else {
        CpuPc->Inclusive = 1;
        CpuPc->Exclusive = 0;
    }

	CpuPc->Depth = Depth;
	CpuPc->DllId = Pc->DllId;
	CpuPc->FunctionId = Pc->FunctionId;
	CpuPc->LineId = Pc->LineId;

    InsertTailList(ListHead, &CpuPc->ListEntry);
    Table->Count += 1;
}

VOID
CpuInsertThreadFunction(
	__in PCPU_THREAD Thread,
	__in PAPS_PC_METRICS Metrics,
	__in ULONG Depth
	)
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_THREAD_PC_TABLE Table;
    PCPU_THREAD_PC CpuPc;
	PVOID FuncAddress;
	ULONG Number;
	BOOLEAN Found;

    Table = Thread->Table;
    ASSERT(Table != NULL);

	for(Number = 0; Number < Depth; Number += 1) {

		FuncAddress = Metrics[Number].FunctionAddress;
		Bucket = ((ULONG_PTR)FuncAddress) % CPU_PC_BUCKET;
		ListHead = &Table->ListHead[Bucket];

		Found = FALSE;
		ListEntry = ListHead->Flink;

		while (ListEntry != ListHead) {

			CpuPc = CONTAINING_RECORD(ListEntry, CPU_THREAD_PC, ListEntry);

			if (CpuPc->Pc == FuncAddress) {
				CpuPc->Inclusive += Metrics[Number].FunctionInclusive;
				CpuPc->Exclusive += Metrics[Number].FunctionExclusive;
				CpuPc->Depth += Depth;
				Found = TRUE;
				break;
			}

			ListEntry = ListEntry->Flink;

		}

		if (Found) {
			continue;
		}

		//
		// Insert Pc into thread Pc table
		//

		CpuPc = (PCPU_THREAD_PC)ApsMalloc(sizeof(CPU_THREAD_PC));
		CpuPc->Pc = FuncAddress;

		CpuPc->Inclusive = Metrics[Number].FunctionInclusive;
		CpuPc->Exclusive = Metrics[Number].FunctionExclusive;

		CpuPc->Depth = Metrics[Number].Depth;
		CpuPc->DllId = Metrics[Number].DllId;
		CpuPc->FunctionId = Metrics[Number].FunctionId;
		CpuPc->LineId = Metrics[Number].FunctionLineId;

		InsertTailList(ListHead, &CpuPc->ListEntry);
		Table->Count += 1;

	}
}

PBTR_CPU_RECORD
CpuGetNextCpuRecord(
	__in PBTR_CPU_RECORD Record,
	__in ULONG Index
	)
{
	ULONG Count;

	if(Index != 0) {
		Count = Record->ActiveCount + Record->RetireCount;
		Record = (PBTR_CPU_RECORD)((PUCHAR)Record + FIELD_OFFSET(BTR_CPU_RECORD, Sample[Count]));
		return Record;
	}

	return Record;
}

PCPU_THREAD
CpuLookupCpuThread(
    __in PCPU_THREAD_TABLE Table,
    __in ULONG ThreadId
    )
{
    ULONG Bucket;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCPU_THREAD Thread;

    Bucket = ThreadId % CPU_THREAD_BUCKET;
    ListHead = &Table->ListHead[Bucket];

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {
        Thread = CONTAINING_RECORD(ListEntry, CPU_THREAD, ListEntry);
        if (Thread->ThreadId == ThreadId) {
            return Thread;
        }
        ListEntry = ListEntry->Flink;
    }

    return NULL;
}

PCPU_THREAD
CpuAllocateCpuThread(
	__in ULONG ThreadId
	)
{
	PCPU_THREAD Thread;

	Thread = (PCPU_THREAD)ApsMalloc(sizeof(CPU_THREAD));
	RtlZeroMemory(Thread, sizeof(CPU_THREAD));

	Thread->ThreadId = ThreadId;
	InitializeListHead(&Thread->ListHead);

	return Thread;
}

VOID
CpuInsertCpuThread(
	__in PCPU_THREAD_TABLE Table,
	__in PCPU_THREAD Thread
	)
{
    ULONG Bucket;
    PLIST_ENTRY ListHead;
    Bucket = Thread->ThreadId % CPU_THREAD_BUCKET;
    ListHead = &Table->ListHead[Bucket];

#ifdef _DEBUG
	ASSERT(CpuLookupCpuThread(Table, Thread->ThreadId) == NULL);
#endif
	
    InsertHeadList(ListHead, &Thread->ListEntry);
    Table->Count += 1;
}

PCPU_THREAD_PC_TABLE
CpuCreateThreadPcTable(
    VOID
    )
{
	PCPU_THREAD_PC_TABLE Table;
	ULONG Number;

	Table = (PCPU_THREAD_PC_TABLE)ApsMalloc(sizeof(CPU_THREAD_PC_TABLE));
	for(Number = 0; Number < CPU_PC_BUCKET; Number++) {
		InitializeListHead(&Table->ListHead[Number]);
	}	

    Table->Inclusive = 0;
    Table->Exclusive = 0;
	Table->Count = 0;
	return Table;
}

PCPU_STACK_TABLE
CpuCreateThreadStackTable(
    VOID
    )
{
	PCPU_STACK_TABLE Table;
	ULONG Number;

	Table = (PCPU_STACK_TABLE)ApsMalloc(sizeof(CPU_STACK_TABLE));
	for(Number = 0; Number < CPU_STACK_BUCKET; Number++) {
		InitializeListHead(&Table->ListHead[Number]);
	}	

    Table->ThreadId = 0;
	Table->Count = 0;
    Table->Table = 0;

	return Table;
}

PBTR_CPU_RECORD
CpuGetRecordByNumber(
    __in PPF_REPORT_HEAD Head,
    __in ULONG Number
    )
{
    PBTR_FILE_INDEX Index;
    PBTR_CPU_RECORD Record;
    ULONG Count;
    ULONG Length;

    if (!Head->Streams[STREAM_PC].Offset) {  
		return NULL;
	}

    if (!Head->Streams[STREAM_INDEX].Offset) {
		return NULL;
    }

    if (!Head->Streams[STREAM_RECORD].Offset) {
		return NULL;
    }

    Index = (PBTR_FILE_INDEX)ApsGetStreamPointer(Head, STREAM_INDEX);
    Record = (PBTR_CPU_RECORD)ApsGetStreamPointer(Head, STREAM_RECORD);
    Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));

    if (Number >= Count) {
        return NULL; 
    } 

    Index = Index + Number;
    Record = (PBTR_CPU_RECORD)((PUCHAR)Record + (ULONG)Index->Offset);
    
    //
    // Verify that the record's length equal index specify 
    //

    Length = FIELD_OFFSET(BTR_CPU_RECORD, Sample[Record->ActiveCount + Record->RetireCount]);
    if (Length != (ULONG)Index->Length) {
        ApsFailFast();
    }

    return Record;
 } 

PBTR_CPU_THREAD
CpuGetBtrThreadObject(
    __in PPF_REPORT_HEAD Head,
	__in ULONG ThreadId
    )
{
	PPF_STREAM_CPU_THREAD Threads;
	ULONG Number;
    ULONG Count;

	Threads = (PPF_STREAM_CPU_THREAD)ApsGetStreamPointer(Head, STREAM_CPU_THREAD);
    Count = ApsGetStreamRecordCount(Head, STREAM_CPU_THREAD, BTR_CPU_THREAD);

	if (!Threads) {
		return NULL;
	}

	for(Number = 0; Number < Count; Number += 1) {
		if (Threads->Thread[Number].ThreadId == ThreadId) {
			return &Threads->Thread[Number];
		}
	}

	return NULL;
}

ULONG
CpuGetThreadPriority(
    PPF_REPORT_HEAD Head,
	ULONG ThreadId
)
{
	PBTR_CPU_THREAD Thread;
	Thread = CpuGetBtrThreadObject(Head, ThreadId);
	return Thread->Priority;
}

BOOLEAN
CpuIsThreadRetired(
	PPF_REPORT_HEAD Head,
	ULONG ThreadId
	)
{
	PBTR_CPU_THREAD Thread;
	Thread = CpuGetBtrThreadObject(Head, ThreadId);
	return Thread->Retired;
}

VOID
CpuCreateThreadStateHistory(
	__in PCPU_THREAD Thread, 
	__in PBTR_CPU_THREAD BtrThread
	)
{
	ULONG Size;
	PCPU_THREAD_STATE State;

	Size = FIELD_OFFSET(CPU_THREAD_STATE, State[BtrThread->NumberOfSamples]);
	State = (PCPU_THREAD_STATE)ApsMalloc(Size);

	State->ThreadId = Thread->ThreadId;
	State->Count = BtrThread->NumberOfSamples;
    ASSERT(State->Count != 0);
	State->Current = 0;
	State->First = BtrThread->FirstSample;

	Thread->SampleState = State;

	ApsDebugTrace("tid: %d, first: %d, count: %d", Thread->ThreadId,
				  State->First,State->Count);
}

//
// N.B CpuAddress table points to address table in report
//

PBTR_CPU_ADDRESS CpuAddressTable;
ULONG CpuAddressDelta;

ULONG_PTR KiFastSystemCallRetPtr;
ULONG_PTR CpuAddressMinimum;
ULONG_PTR CpuAddressMaximum;
ULONG CpuAddressCount;

BTR_TARGET_TYPE CpuTarget;
BTR_CPU_TYPE CpuType;

int __cdecl
CpuAddressSortCallback(
    __in const void *Ptr1,
    __in const void *Ptr2 
    )
{
	int Result;
	PBTR_CPU_ADDRESS A1,A2;

    A1 = (PBTR_CPU_ADDRESS)Ptr1;
    A2 = (PBTR_CPU_ADDRESS)Ptr2;

    Result = (int)((ULONG_PTR)A1->Address - (ULONG_PTR)A2->Address);
	return Result;
}

//
// N.B. Whenever a CPU profile report is opened, this routine must
// be called to initialize the address table
//

VOID
CpuBuildAddressTable(
	__in PPF_REPORT_HEAD Head
	)
{
	PVOID Destine;
	PVOID Source;
	ULONG Size;
	ULONG Number;

	CpuType = (BTR_CPU_TYPE)Head->CpuType;
	CpuTarget = (BTR_TARGET_TYPE)Head->TargetType;

	Source = ApsGetStreamPointer(Head, STREAM_CPU_ADDRESS);
	Size = ApsGetStreamLength(Head, STREAM_CPU_ADDRESS);

	if (CpuAddressTable) {

		//
		// N.B. CpuAddressTable may be adjusted due to
		// zero entries, we need backward to its origin
		// position to appropriately free it
		//

		ApsFree(CpuAddressTable - CpuAddressDelta);
	}

	Destine = ApsMalloc(Size);
	memcpy_s(Destine, Size, Source, Size);
	CpuAddressTable = (PBTR_CPU_ADDRESS)Destine;

	CpuAddressCount = Size / sizeof(BTR_CPU_ADDRESS);

	//
	// Save KiFastSystemCallRet ptr, it's valid only for win32
	// profile report
	//

	KiFastSystemCallRetPtr = (ULONG_PTR)CpuAddressTable[0].Address;
	CpuAddressTable[0].Address = 0;

	//
	// Sort the address entries in ascending order
	//

	qsort(CpuAddressTable, CpuAddressCount, sizeof(BTR_CPU_ADDRESS), CpuAddressSortCallback); 

	//
	// Filter out zero address
	//

	Number = 0;
	while (CpuAddressTable[Number].Address == 0) {
		Number += 1;
	}

	CpuAddressDelta = Number;
	CpuAddressTable += Number;
	CpuAddressCount -= Number;

	CpuAddressMinimum = (ULONG_PTR)CpuAddressTable[0].Address;
	CpuAddressMaximum = (ULONG_PTR)CpuAddressTable[CpuAddressCount - 1].Address;
}

#define CPU_COMPARE_PTR(_P1, _P2) \
	((int)((LONG_PTR)_P1 - (LONG_PTR)_P2))

//
// N.B. This routine map a pc on top of stack to thread's
// execution state, run, wait or io, we need speed up this
// procedure. A tuned hash table may be more performant.
//

BTR_CPU_STATE_TYPE
CpuPcToThreadState(
	__in ULONG_PTR Pc0,
	__in ULONG_PTR Pc1
	)
{
	int Result;
	ULONG Number;
	ULONG_PTR Pc;

	ASSERT(CpuAddressTable != NULL);
	ASSERT(CpuAddressCount != 0);

	Pc = Pc0;
	if (CpuTarget == TARGET_WIN32 && CpuType == CPU_X86_32) {

		//
		// If it's not doing system call, we consider it's running
		//

		if (Pc != KiFastSystemCallRetPtr) {
			return CPU_STATE_RUN;
		}
		else {

			//
			// It's doing system call, fetch next pc to check
			//

			Pc = Pc1;
		}
	}

	//
	// If the ptr fall out of address table range, it's running
	//

	if (Pc < CpuAddressMinimum)
		return CPU_STATE_RUN;

	if (Pc > CpuAddressMaximum)
		return CPU_STATE_RUN;

	//
	// Slow path to match address through whole table
	//

	for(Number = 0; Number < CpuAddressCount; Number += 1) {
		Result = CPU_COMPARE_PTR(Pc, CpuAddressTable[Number].Address);
		if (Result == 0) {
			return CpuAddressTable[Number].State;
		}
	}

	//
	// All left are considered to be running
	//

	return CPU_STATE_RUN;
}

VOID
CpuSetThreadSampleState(
	__in PCPU_THREAD Thread,
	__in ULONG_PTR Pc0,
	__in ULONG_PTR Pc1
	)
{
	PCPU_THREAD_STATE SampleState;

	SampleState = Thread->SampleState;
	if (SampleState->Current >= SampleState->Count){

		//
		// N.B. CPU state array overflow, this should never happen
		//

		//ASSERT(0);
		return;
	}

	SampleState->State[SampleState->Current].Type = CpuPcToThreadState(Pc0, Pc1);
	SampleState->Current += 1;
}

//
// N.B. We don't pass CPU_THREAD_TABLE ptr, because each caller
// may maintain its own PCPU_THREAD array, and may change the
// order of thread object in the array, so we need caller provide
// its ptr to CPU_THREAD array.
//

ULONG
CpuBuildThreadListByRange(
	__in PCPU_THREAD *Thread,
	__in ULONG NumberOfThreads,
	__in ULONG First,
	__in ULONG Last,
	__out PULONG Index
	)
{
	PCPU_THREAD_STATE State;
	ULONG Number;
	ULONG Count;

	Count = 0;

	for(Number = 0; Number < NumberOfThreads; Number += 1) {

		State = Thread[Number]->SampleState;
		ASSERT(State != NULL);

        if (State->First + State->Count - 1 < First || State->First > Last) {
            continue;
		}

        Index[Count] = Number;
        Count += 1;
	}

#ifdef _DEBUG
    for(Number = 0; Number < Count; Number += 1) {
		State = Thread[Index[Number]]->SampleState;
        ApsDebugTrace("cpu picked threads: tid=%u, range (%u, %u)", 
                       State->ThreadId, State->First, State->First + State->Count - 1);
    }
#endif

	return Count;
} 

ULONG
CpuGetSampleDepth(
	__in PPF_REPORT_HEAD Head
	)
{
	ULONG Count;
	Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));
	return Count;
}

BOOLEAN
CpuComputeUsageByRange(
	__in PPF_REPORT_HEAD Head,
	__in ULONG First,
	__in ULONG Last,
	__out double *CpuMin,
	__out double *CpuMax,
	__out double *Average
	)
{
	PBTR_FILE_INDEX Index;
    PBTR_CPU_RECORD Record;
    ULONG Count;
	ULONG Number;
	double Total;

	*CpuMin = 0.0;
	*CpuMax = 0.0;
	*Average = 0.0;
	Total = 0.0;

    Index = (PBTR_FILE_INDEX)ApsGetStreamPointer(Head, STREAM_INDEX);
    Record = (PBTR_CPU_RECORD)ApsGetStreamPointer(Head, STREAM_RECORD);
    Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));

    if (Last >= Count) {
		ASSERT(0);
        return FALSE; 
    } 

	for(Number = First; Number <= Last; Number += 1) {
	    Record = (PBTR_CPU_RECORD)((PUCHAR)Record + (ULONG)Index[Number].Offset);

		//
		// N.B. We temporarily skip mark record to avoid break compatibilty with
		// old implementation.
		//

		if (Record->Flag & BTR_FLAG_CPU_MARKER) {
			continue;
		}

		*CpuMin = min(*CpuMin, Record->CpuUsage);
		*CpuMax = max(*CpuMax, Record->CpuUsage);
		Total += Record->CpuUsage;
	}
    
	*Average = Total / (Last - First + 1);
    return TRUE;
}

double
CpuGetUsageByIndex(
	__in PPF_REPORT_HEAD Head,
	__in ULONG Number
	)
{
	PBTR_FILE_INDEX Index;
    PBTR_CPU_RECORD Record;
    ULONG Count;

    Index = (PBTR_FILE_INDEX)ApsGetStreamPointer(Head, STREAM_INDEX);
    Record = (PBTR_CPU_RECORD)ApsGetStreamPointer(Head, STREAM_RECORD);
    Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));

    if (Number >= Count) {
		ASSERT(0);
        return 0.0; 
    } 

	Record = (PBTR_CPU_RECORD)((PUCHAR)Record + (ULONG)Index[Number].Offset);
	return Record->CpuUsage;
}

BOOLEAN
CpuIsMarkRecord(
	IN PBTR_CPU_RECORD Record
	)
{
	return (Record->Flag & BTR_FLAG_CPU_MARKER);
}

VOID
CpuUpdateOnCpuCounter(
	IN PCPU_COUNTERS OnCpu,
	IN PBTR_CPU_SAMPLE Sample,
	IN PBTR_PC_ENTRY PcEntry
	)
{
	ULONG i;
	PCPU_PC_ENTRY CpuPc;

	for (i = 0; i < OnCpu->AllocationCount; i++) {
		CpuPc = &OnCpu->Pc[i];
		if (!CpuPc->Pc.Address) {

			//
			// Scan an empty slot and allocate for current sample
			//

			CpuPc->Pc = *PcEntry;
			CpuPc->Count = 1;
			CpuPc->KernelTime = Sample->KernelTime;
			CpuPc->UserTime = Sample->UserTime;

			OnCpu->PcCount += 1;
			OnCpu->TotalCount += 1;
			OnCpu->TotalKernelTime += Sample->KernelTime;
			OnCpu->TotalUserTime += Sample->UserTime;
			break;
		}
		else if ((LONG_PTR)CpuPc->Pc.Address == (LONG_PTR)PcEntry->Address) {

			//
			// Find the match PcEntry, update its counters
			//

			CpuPc->Count += 1;
			CpuPc->KernelTime += Sample->KernelTime;
			CpuPc->UserTime += Sample->UserTime;
			
			OnCpu->TotalCount += 1;
			OnCpu->TotalKernelTime += Sample->KernelTime;
			OnCpu->TotalUserTime += Sample->UserTime;
			break;
		}
	}
}

ULONG
CpuBuildOnCpuStatistics(
	IN PPF_REPORT_HEAD Head,
	OUT PCPU_COUNTERS *Stat 
	)
{
	PBTR_FILE_INDEX Index;
	PBTR_CPU_RECORD Record;
	PBTR_CPU_SAMPLE Sample;
	ULONG Number;
	ULONG Count;
	ULONG ThreadCount;
	ULONG i;
	PBTR_STACK_RECORD StackTrace;
	PBTR_STACK_RECORD StackFile;
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_PC_TABLE PcTable;
	PBTR_PC_ENTRY PcEntry;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;

	int MaxPcCount;
	PCPU_COUNTERS OnCpu;

	//
	// Require PC stream is available
	//

	if (!Head->Streams[STREAM_PC].Offset) {
		return APS_STATUS_ERROR;
	}

	if (!Head->Streams[STREAM_INDEX].Offset) {
		return APS_STATUS_ERROR;
	}

	if (!Head->Streams[STREAM_RECORD].Offset) {
		return APS_STATUS_ERROR;
	}

	Index = (PBTR_FILE_INDEX)ApsGetStreamPointer(Head, STREAM_INDEX);
	Record = (PBTR_CPU_RECORD)ApsGetStreamPointer(Head, STREAM_RECORD);
	Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));

	StackFile = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	ASSERT(StackFile != NULL);

	MaxPcCount = ApsGetStreamLength(Head, STREAM_STACK) / sizeof(BTR_STACK_RECORD);
	OnCpu = (PCPU_COUNTERS)ApsMalloc(FIELD_OFFSET(CPU_COUNTERS, Pc[MaxPcCount]));
	OnCpu->AllocationCount = MaxPcCount;

	//
	// Build symbol table
	//

	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	//
	// Get DLL entry base address 
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
	DllEntry = &DllFile->Dll[0];

	//
	// Get function entry base address
	//

	FuncEntry = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);

	//
	// Get line entry base address
	//

	LineEntry = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE);

	//
	// Create Pc table from STREAM_PC
	//
 
	ApsCreatePcTableFromStream(Head, &PcTable);

	int MarkIndex = 0;
	for (Number = 0; Number < Count; Number += 1) {

		//
		// Get next CPU profiling record
		//

		Record = CpuGetNextCpuRecord(Record, Number);
		if (CpuIsMarkRecord(Record)) {
			ApsDebugTrace("CPU Mark #%u: K %u U %u Usage: %.2f %%", MarkIndex,
						(ULONG)Record->KernelTime, (ULONG)Record->UserTime, Record->CpuUsage);
			MarkIndex += 1;
			continue;
		}
		else {
			ApsDebugTrace("CPU Record #%u: Active %u Retire %u K %u U %u", Number,
						Record->ActiveCount, Record->RetireCount,
						(ULONG)Record->KernelTime, (ULONG)Record->UserTime);
		}

		//
		// Get number of total threads include retired ones
		//

		ThreadCount = Record->ActiveCount + Record->RetireCount;

		for (i = 0; i < ThreadCount; i++) {

			CHAR Name[MAX_PATH];
			PSTR Ptr;

			Sample = &Record->Sample[i];
			if (Sample->KernelTime == 0 && Sample->UserTime == 0) {

				//
				// The thread is off CPU, just ignore it
				//

				continue;
			}

			StackTrace = &StackFile[Sample->StackId];
			PcEntry = ApsLookupPcEntry(StackTrace->Frame[0], PcTable);
			ASSERT(PcEntry != NULL);
			TextEntry = ApsLookupSymbol(TextTable, (ULONG64)PcEntry->Address);
			if (!TextEntry) {
				sprintf_s(Name, MAX_PATH, "0x%I64x", (ULONG64)PcEntry->Address);
				Ptr = Name;
			}
			else {
				Ptr = TextEntry->Text;
			}

			CpuUpdateOnCpuCounter(OnCpu, Sample, PcEntry);

			ApsDebugTrace("TID %u: K %u U %u IP %s", Sample->ThreadId, 
						(ULONG)Sample->KernelTime, (ULONG)Sample->UserTime, Ptr);
		}
	}

	//
	// Sort the On CPU Pc in decrease order by its time percent
	//

	qsort(&OnCpu->Pc[0], OnCpu->PcCount, sizeof(CPU_PC_ENTRY), CpuOnCpuPcSortCallback);

#ifdef _DEBUG
	CpuDebugOnCpuStatistics(OnCpu, TextTable);
#endif

	*Stat = OnCpu;
	
	ApsDestroySymbolTable(TextTable);
	ApsDestroyPcTableFromStream(PcTable);
	return APS_STATUS_OK;
}

int __cdecl
CpuOnCpuPcSortCallback(
	IN const void* Entry1,
	IN const void* Entry2
	)
{
	PCPU_PC_ENTRY T1, T2;

	T1 = (PCPU_PC_ENTRY)Entry1;
	T2 = (PCPU_PC_ENTRY)Entry2;

	//
	// N.B. we need a decreasing order
	//

	return (T2->KernelTime + T2->UserTime) - (T1->KernelTime + T1->UserTime);
}

int __cdecl
CpuOnCpuCounterSortCallback(
	IN const void* Entry1,
	IN const void* Entry2
	)
{
	PCPU_COUNTERS T1, T2;

	T1 = (PCPU_COUNTERS)Entry1;
	T2 = (PCPU_COUNTERS)Entry2;

	//
	// N.B. we need a decreasing order
	//

	return (T1->TotalKernelTime + T1->TotalUserTime) - (T2->TotalKernelTime + T2->TotalUserTime);
}

VOID
CpuDebugOnCpuStatistics(
	IN PCPU_COUNTERS OnCpu,
	IN PBTR_TEXT_TABLE TextTable
	)
{
	ULONG i;
	PCPU_PC_ENTRY Pc;
	PBTR_TEXT_ENTRY TextEntry;
	CHAR Name[MAX_PATH];
	PCSTR Ptr;
	double TotalTime;
	double Percent;

	if (OnCpu->ThreadId != 0) {
		ApsDebugTrace("OnCpu Statistics: TID =%u", OnCpu->ThreadId);
	}

	TotalTime = (OnCpu->TotalKernelTime + OnCpu->TotalUserTime) * 1.0;

	for (i = 0; i < OnCpu->PcCount; i++) {
		Pc = &OnCpu->Pc[i];
		TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Pc->Pc.Address);
		if (!TextEntry) {
			sprintf_s(Name, MAX_PATH, "0x%I64x", (ULONG64)Pc->Pc.Address);
			Ptr = Name;
		}
		else {
			Ptr = TextEntry->Text;
		}

		Percent = (Pc->KernelTime + Pc->UserTime) * 100.0 / TotalTime;
		ApsDebugTrace("OnCpu Statistics: #%u IP %s , Percent %.2f %% , Count %u", 
						i, Ptr, Percent, Pc->Count);
	}
}

float
CpuComputeOnCpuPercent(
	IN PCPU_COUNTERS OnCpu,
	IN PCPU_PC_ENTRY Pc,
	IN BOOLEAN ByTime
	)
{
	float Percent;

	if (ByTime) {
		Percent = (Pc->KernelTime + Pc->UserTime) * 100.0f / (OnCpu->TotalKernelTime + OnCpu->TotalUserTime);
	}
	else {
		Percent = Pc->Count * 100.0f / OnCpu->TotalCount;
	}
	return Percent;
}

int
CpuGetMarkStep(
	IN PPF_REPORT_HEAD Head
	)
{
	PPF_STREAM_SYSTEM Info;

	Info = (PPF_STREAM_SYSTEM)ApsGetStreamPointer(Head, STREAM_SYSTEM);
	ASSERT(Info != NULL);
	ASSERT(Info->Attr.SamplingPeriod != 0);
	ASSERT(Info->Attr.SamplingPeriod < 1000);
	return (1000 / Info->Attr.SamplingPeriod);
}

PCPU_COUNTERS
CpuAllocateThreadTable(
	IN PPF_REPORT_HEAD Head
	)
{
	PCPU_COUNTERS Counter;
	return NULL;
}

VOID
CpuFreeThreadTable(
	IN PCPU_COUNTERS Counters
)
{
	ApsFree(Counters);
}

PCPU_COUNTERS
CpuGetCounterPerThread(
	IN PCPU_COUNTERS Counters,
	IN ULONG CounterSize,
	IN ULONG ThreadId
	)
{
	int i;
	PCPU_COUNTERS Entry;

	ASSERT(ThreadId != 0);

	//
	// Skip first entry which is reserved for global counter
	//

	Entry = (PCPU_COUNTERS)((PUCHAR)Counters + CounterSize);

	for (i = 0; i < CPU_MAX_THREAD - 1; i++) {
		if (Entry->ThreadId == 0) {

			//
			// Allocate new thread entry for ThreadId
			//

			Entry->ThreadId = ThreadId;
			Counters->ThreadCount += 1;
			return Entry;
		}
		else if (Entry->ThreadId == ThreadId) {
			return Entry;
		}
		Entry = (PCPU_COUNTERS)((PUCHAR)Entry + CounterSize);
	}

	ASSERT(0);
	return NULL;
}

VOID
CpuUpdateOffCpuCounter(
	IN PCPU_COUNTERS Counter,
	IN PBTR_CPU_SAMPLE Sample,
	IN PBTR_PC_ENTRY PcEntry
	)
{
	ULONG i;
	PCPU_PC_ENTRY CpuPc;

	for (i = 0; i < Counter->AllocationCount; i++) {
		CpuPc = &Counter->Pc[i];
		if (!CpuPc->Pc.Address) {

			//
			// Scan an empty slot and allocate for current sample
			//

			CpuPc->Pc = *PcEntry;
			CpuPc->Count = 1;

			Counter->PcCount += 1;
			Counter->TotalCount += 1;
			break;
		}
		else if ((LONG_PTR)CpuPc->Pc.Address == (LONG_PTR)PcEntry->Address) {

			//
			// Find the match PcEntry, update its counters
			//

			CpuPc->Count += 1;
			Counter->TotalCount += 1;
			break;
		}
	}
}

ULONG
CpuScanOnCpuThreaded(
	IN PPF_REPORT_HEAD Head,
	IN CPU_COUNTER_TYPE Type,
	OUT PCPU_COUNTERS* Stat
	)
{
	PBTR_FILE_INDEX Index;
	PBTR_CPU_RECORD Record;
	PBTR_CPU_SAMPLE Sample;
	ULONG Number;
	ULONG Count;
	ULONG ThreadCount;
	ULONG i;
	PBTR_STACK_RECORD StackTrace;
	PBTR_STACK_RECORD StackFile;
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_PC_TABLE PcTable;
	PBTR_PC_ENTRY PcEntry;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;

	int MaxPcCount;
	int AllocationCount;
	int CounterSize;
	int MarkStep;
	PCPU_COUNTERS Counter;
	PCPU_COUNTERS Thread;
	PCPU_COUNTERS Entry;

	//
	// Require PC stream is available
	//

	if (!Head->Streams[STREAM_PC].Offset) {
		return APS_STATUS_ERROR;
	}

	if (!Head->Streams[STREAM_INDEX].Offset) {
		return APS_STATUS_ERROR;
	}

	if (!Head->Streams[STREAM_RECORD].Offset) {
		return APS_STATUS_ERROR;
	}

	Index = (PBTR_FILE_INDEX)ApsGetStreamPointer(Head, STREAM_INDEX);
	Record = (PBTR_CPU_RECORD)ApsGetStreamPointer(Head, STREAM_RECORD);
	Count = (ULONG)(Head->Streams[STREAM_INDEX].Length / sizeof(BTR_FILE_INDEX));

	StackFile = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	ASSERT(StackFile != NULL);

	//
	// Allocated CPU_COUNTER structure by fixed number of Pc entries by
	// compare minimum of number of stack records and number of index
	//

	MaxPcCount = ApsGetStreamLength(Head, STREAM_STACK) / sizeof(BTR_STACK_RECORD);
	//MarkStep = CpuGetMarkStep(Head);
	//MaxPcCount = min(Count / (MarkStep + 1), MaxPcCount);

	//
	// Support up to 1024 threads limit
	//

	CounterSize = FIELD_OFFSET(CPU_COUNTERS, Pc[MaxPcCount]);
	Counter = (PCPU_COUNTERS)ApsMalloc(CounterSize * CPU_MAX_THREAD);
	for (Number = 0; Number < CPU_MAX_THREAD; Number += 1) {
		Entry = (PCPU_COUNTERS)((PUCHAR)Counter + Number * CounterSize);
		Entry->AllocationCount = MaxPcCount;
		Entry->Type = Type;
	}

	//
	// First counter entry is reserved for total summary
	//

	Counter[0].ThreadCount = 0;

	//
	// Build symbol table
	//

	TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	//
	// Get DLL entry base address 
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
	DllEntry = &DllFile->Dll[0];

	//
	// Get function entry base address
	//

	FuncEntry = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);

	//
	// Get line entry base address
	//

	LineEntry = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE);

	//
	// Create Pc table from STREAM_PC
	//

	ApsCreatePcTableFromStream(Head, &PcTable);

	int MarkIndex = 0;
	for (Number = 0; Number < Count; Number += 1) {

		//
		// Get next CPU profiling record
		//

		Record = CpuGetNextCpuRecord(Record, Number);
		if (CpuIsMarkRecord(Record)) {
			ApsDebugTrace("CPU Mark #%u: K %u U %u Usage: %.2f %%", MarkIndex,
				(ULONG)Record->KernelTime, (ULONG)Record->UserTime, Record->CpuUsage);
			MarkIndex += 1;
			continue;
		}
		else {
			ApsDebugTrace("CPU Record #%u: Active %u Retire %u K %u U %u", Number,
				Record->ActiveCount, Record->RetireCount,
				(ULONG)Record->KernelTime, (ULONG)Record->UserTime);
		}

		//
		// Get number of total threads include retired ones
		//

		ThreadCount = Record->ActiveCount + Record->RetireCount;

		for (i = 0; i < ThreadCount; i++) {

			CHAR Name[MAX_PATH];
			PSTR Ptr;

			Sample = &Record->Sample[i];
			if (Sample->KernelTime == 0 && Sample->UserTime == 0) {
				continue;
			}

			//
			// Lookup thread
			//

			Thread = CpuGetCounterPerThread(Counter, CounterSize, Sample->ThreadId);
			if (!Thread) {

				//
				// This indicate thread number overflow and we need ignore this thread
				//

				continue;
			}

			StackTrace = &StackFile[Sample->StackId];
			PcEntry = ApsLookupPcEntry(StackTrace->Frame[0], PcTable);
			ASSERT(PcEntry != NULL);
			TextEntry = ApsLookupSymbol(TextTable, (ULONG64)PcEntry->Address);
			if (!TextEntry) {
				sprintf_s(Name, MAX_PATH, "0x%I64x", (ULONG64)PcEntry->Address);
				Ptr = Name;
			}
			else {
				Ptr = TextEntry->Text;
			}

			CpuUpdateOnCpuCounter(Thread, Sample, PcEntry);
			CpuUpdateOnCpuCounter(Counter, Sample, PcEntry);
		}
	}

	//
	// Sort the On CPU Pc in decrease order by its time percent
	// Sort each counter for each thread
	//

	Entry = CpuGetFirstCounter(Counter);
	qsort(Entry, Counter->ThreadCount, CounterSize, CpuOnCpuCounterSortCallback);
	
	for (i = 1; i < Counter->ThreadCount + 1; i++) {
		Entry = (PCPU_COUNTERS)((PUCHAR)Counter + i * CounterSize);
		if (Entry->PcCount > 1) {
			qsort(&Entry->Pc[0], Entry->PcCount, sizeof(CPU_PC_ENTRY), CpuOnCpuPcSortCallback);
		}
	}

#ifdef _DEBUG
	for (i = 1; i < Counter->ThreadCount + 1; i++) {
		Entry = (PCPU_COUNTERS)((PUCHAR)Counter + i * CounterSize);
		CpuDebugOnCpuStatistics(Entry, TextTable);
	}
#endif

	*Stat = Counter;

	ApsDestroySymbolTable(TextTable);
	ApsDestroyPcTableFromStream(PcTable);
	return APS_STATUS_OK;
}

PCPU_COUNTERS
CpuGetFirstCounter(
	IN PCPU_COUNTERS Counter
	)
{
	int Size;
	Size = FIELD_OFFSET(CPU_COUNTERS, Pc[Counter->AllocationCount]);
	return (PCPU_COUNTERS)((PUCHAR)Counter + Size);
}

PCPU_COUNTERS
CpuGetNextCounter(
	IN PCPU_COUNTERS Counter,
	IN PCPU_COUNTERS Current
	)
{
	int Size;
	Size = FIELD_OFFSET(CPU_COUNTERS, Pc[Counter->AllocationCount]);
	if (((ULONG_PTR)Current - (ULONG_PTR)Counter) / Size < Counter->ThreadCount) {
		return (PCPU_COUNTERS)((PUCHAR)Current + Size);
	}
	return NULL;
} 