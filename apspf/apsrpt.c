//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "apsprofile.h"
#include "apsrpt.h"
#include "aps.h"
#include "apsanalysis.h"
#include "apscpu.h"
#include <stdlib.h>
#include "list.h"

#define HASH_BUCKET(_V, _C) \
	((ULONG_PTR)_V % _C)

ULONG
ApsWriteReport(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	PPF_REPORT_HEAD Head;
	HANDLE Handle;
	PVOID MappedVa;
	ULONG Status;
	LARGE_INTEGER Start, End;
	PPF_STREAM_SYSTEM System;
	HANDLE IndexHandle;
	HANDLE DataHandle;

	Profile->ReportFileHandle = CreateFile(Profile->ReportPath,
		                                   GENERIC_READ|GENERIC_WRITE,
									       0, NULL, OPEN_EXISTING,
									       FILE_ATTRIBUTE_NORMAL, NULL);

	if (Profile->ReportFileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	Handle = CreateFileMapping(Profile->ReportFileHandle, NULL, 
		                       PAGE_READWRITE, 0, 0, NULL);
	if (!Handle) {
		CloseHandle(Profile->ReportFileHandle);
		Profile->ReportFileHandle = NULL;
		return GetLastError();
	}
	
	MappedVa = MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!MappedVa) {
		Status = GetLastError();
		CloseHandle(Handle);
		CloseHandle(Profile->ReportFileHandle);
		Profile->ReportFileHandle = NULL;
		return Status;
	}

	//
	// Write system stream, system stream immediately follows report head
	//

	Head = (PPF_REPORT_HEAD)MappedVa;
	System = (PPF_STREAM_SYSTEM)(Head + 1);

	ApsWriteSystemStream(Profile, System);

	//
	// Write stack record stream and delete stack record file
	//

	if (Profile->Type != PROFILE_CCR_TYPE) {

		Profile->StackFileHandle = CreateFile(Profile->StackPath,
			GENERIC_READ|GENERIC_WRITE,
			0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);

		if (Profile->StackFileHandle == INVALID_HANDLE_VALUE) {
			Status = GetLastError();
			CloseHandle(Profile->ReportFileHandle);
			Profile->ReportFileHandle = NULL;
			return Status;
		}

		Start.QuadPart = Head->Streams[STREAM_STACK].Offset;
		SetFilePointerEx(Profile->ReportFileHandle, Start, NULL, FILE_BEGIN);

		Status = ApsWriteStackStream(Profile, Head, Profile->ReportFileHandle, 
			Profile->StackFileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			UnmapViewOfFile(Head);
			CloseHandle(Profile->ReportFileHandle);
			Profile->ReportFileHandle = NULL;
			return Status;
		}

		Start.QuadPart = End.QuadPart;

		CloseHandle(Profile->StackFileHandle);
		Profile->StackFileHandle = NULL;
	}
	else {

		//
		// CCR profiling complete all in host side
		//

		Start.QuadPart = Head->Streams[STREAM_STACK].Offset;
		Start.QuadPart += Head->Streams[STREAM_STACK].Length;
		SetFilePointerEx(Profile->ReportFileHandle, Start, NULL, FILE_BEGIN);
	}

	DeleteFile(Profile->StackPath);

	//
	// Write dll stream
	//

	Status = ApsWriteDllStream(Profile, Head, Profile->ReportFileHandle, Start, &End);
	if (Status != APS_STATUS_OK) {
		CloseHandle(Profile->ReportFileHandle);
		Profile->ReportFileHandle = NULL;
		return Status;
	}

	Start.QuadPart = End.QuadPart;

	//
	// Write index and record stream if it's RECORD_MODE or CPU profie type
	// note that CPU profiling is always RECORD_MODE default
	//

	if (Profile->Type == PROFILE_CPU_TYPE || Profile->Mode == RECORD_MODE) {

		//
		// Write index stream
		//

		IndexHandle = CreateFile(Profile->IndexPath, GENERIC_READ|GENERIC_WRITE,
							     0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (IndexHandle == INVALID_HANDLE_VALUE) {
			return GetLastError();
		}

		Status = ApsWriteIndexStream(Profile, Head, Profile->ReportFileHandle, 
			                         IndexHandle, Start, &End);
		CloseHandle(IndexHandle);
		DeleteFile(Profile->IndexPath);

		if (Status != APS_STATUS_OK) {
			return Status;
		}

		//
		// Update the record start position
		//

		Start.QuadPart = End.QuadPart;

		//
		// Write record stream
		//

		DataHandle = CreateFile(Profile->DataPath, GENERIC_READ|GENERIC_WRITE,
							    0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (DataHandle == INVALID_HANDLE_VALUE) {
			return GetLastError();
		}

		Status = ApsWriteRecordStream(Profile, Head, Profile->ReportFileHandle, 
			                          DataHandle, Start, &End);
		CloseHandle(DataHandle);
		DeleteFile(Profile->DataPath);

		if (Status != APS_STATUS_OK) {
			return Status;
		}
		
	}

	//
	// Mark as complete and update checksum
	//

    Head->Complete = 1;
	ApsComputeStreamCheckSum(Head);

	UnmapViewOfFile(MappedVa);
	CloseHandle(Handle);

	SetFilePointerEx(Profile->ReportFileHandle, End, NULL, FILE_BEGIN);
	SetEndOfFile(Profile->ReportFileHandle);
	CloseHandle(Profile->ReportFileHandle);
	Profile->ReportFileHandle = NULL;

	return APS_STATUS_OK;
}

ULONG
ApsWriteSystemStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_STREAM_SYSTEM System
	)
{
	ULONG64 RamSize;
	double GigaBytes;
	
	//
	// Fill the system stream
	//

	System->ProcessId = Profile->ProcessId;
	System->SessionId = Profile->SessionId;

	StringCchCopy(System->UserName, MAX_PATH, Profile->UserName);
	StringCchCopy(System->ImagePath, MAX_PATH, Profile->ImagePath);
	StringCchCopy(System->Argument, MAX_PATH, Profile->Argument);
	StringCchCopy(System->WorkPath, MAX_PATH, Profile->WorkPath);

	ApsGetComputerName(System->Computer, 64);
	ApsQuerySystemVersion(System->System, 64);
	ApsQueryCpuModel(System->Processor, 64);

	//
	// RAM size
	//

	RamSize = ApsGetPhysicalMemorySize();

	GigaBytes = (RamSize * 1.0) / (1024 * 1024 * 1024);
	if (GigaBytes > 1.0) {
		StringCchPrintf(System->Memory, 64, L"%.2f GB", GigaBytes);
	}
	else {
		StringCchPrintf(System->Memory, 64, L"% MB", 
			            (ULONG)(RamSize / (1024 * 1024)));
	}

	return APS_STATUS_OK;
}

//
// N.B. Whenever report file is modified,
// ApsComputeStreamCheckSum must be called
// to re-compute the new checksum and update
// the report header
//

VOID
ApsComputeStreamCheckSum(
	__in PPF_REPORT_HEAD Head
	)
{
	ULONG i;
	ULONG CheckSum;
	PUCHAR Buffer;

	CheckSum = 0;
	Buffer = (PUCHAR)Head;

    //
    // N.B. CheckSum field is not included in
    // checksum computation
    //

    Head->CheckSum = 0;
    Head->Context = NULL;

	for(i = 0; i < sizeof(*Head); i++) {
		CheckSum += (ULONG)Buffer[i];		
	}	

    //
    // Assign the new checksum
    //

	Head->CheckSum = CheckSum;
}


#define MINIDUMP_FULL  MiniDumpWithFullMemory        | \
                       MiniDumpWithFullMemoryInfo    | \
					   MiniDumpWithHandleData        | \
					   MiniDumpWithThreadInfo        | \
					   MiniDumpWithUnloadedModules   |\
					   MiniDumpWithFullAuxiliaryState

#define MINIDUMP_DLL  (MiniDumpNormal | MiniDumpWithUnloadedModules)

ULONG
ApsWriteMinidump(
	IN HANDLE ProcessHandle,
	IN ULONG ProcessId,
	IN PWSTR Path,
	IN BOOLEAN Full,
	OUT PHANDLE DumpHandle 
	)
{
	HANDLE FileHandle;
	MINIDUMP_TYPE Type;
	ULONG Status;

	*DumpHandle = NULL;

	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
		                    FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	ApsSuspendProcess(ProcessHandle);

	__try {

		Type = (MINIDUMP_TYPE)(Full ? (MINIDUMP_FULL) : MINIDUMP_DLL);
		Status = MiniDumpWriteDump(ProcessHandle, ProcessId, FileHandle, 
			                       Type, NULL, NULL, NULL);

	}__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = FALSE;
	}

	ApsResumeProcess(ProcessHandle);

	if (!Status) {
		Status = GetLastError();
		CloseHandle(FileHandle);
	} else {
		*DumpHandle = FileHandle;
		Status = APS_STATUS_OK;
	}

	return Status;
}

ULONG
ApsWriteDllStream(
	IN PAPS_PROFILE_OBJECT Profile,
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	WCHAR Path[MAX_PATH];
	ULONG Length;
	HANDLE DumpHandle;
	ULONG Status;
	ULONG Complete;
	ULONG Size;
	PVOID Buffer;
	HANDLE MappingHandle;
	PMINIDUMP_MODULE_LIST ModuleList;
	ULONG ModuleListSize;
	PMINIDUMP_DIRECTORY Dir;
	PBTR_DLL_ENTRY Dll;
	PBTR_DLL_FILE DllFile;
	PMINIDUMP_MODULE Module;
	ULONG DllFileSize;
	PMINIDUMP_STRING NameString;
	PCV_INFO_PDB70 PdbRecord;
	CHAR PdbName[64];
	CHAR Pdb[16];
	PBTR_CV_RECORD CvRecord;
	ULONG i;
	SYSTEMTIME Time;

	//
	// Write a minidump to system temporary folder
	//

	Length = GetTempPath(MAX_PATH, Path);
	if (!Length) {
		return GetLastError();
	}

	GetLocalTime(&Time);
	StringCchPrintf(Path, MAX_PATH, L"%2d%2d%2d-%d-dpf.dmp", 
					Time.wHour, Time.wMinute, Time.wSecond, Profile->ProcessId);

	Status = ApsWriteMinidump(Profile->ProcessHandle, Profile->ProcessId, 
		                      Path, FALSE, &DumpHandle);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	Size = GetFileSize(DumpHandle, NULL);
	SetFilePointer(DumpHandle, 0, NULL, FILE_BEGIN);

	__try {

		MappingHandle = CreateFileMapping(DumpHandle, NULL, PAGE_READONLY, 0, 0, NULL);
		if (!MappingHandle) {
			Status = GetLastError();
			__leave;
		}

		Buffer = MapViewOfFile(MappingHandle, FILE_MAP_READ, 0, 0, 0);
		if (!Buffer) {
			Status = GetLastError();
			__leave;
		}

		MiniDumpReadDumpStream(Buffer, ModuleListStream, &Dir, (PVOID *)&ModuleList, &ModuleListSize);

		DllFileSize = FIELD_OFFSET(BTR_DLL_FILE, Dll[ModuleList->NumberOfModules]);
		DllFile = (PBTR_DLL_FILE)ApsMalloc(DllFileSize);

		for(i = 0; i < ModuleList->NumberOfModules; i++) {

			Module = &ModuleList->Modules[i];	
			Dll = &DllFile->Dll[i];

			Dll->BaseVa = (ULONG_PTR)Module->BaseOfImage;
			Dll->Size = Module->SizeOfImage;
			Dll->DllId = i;
			Dll->Count = 0;
			Dll->Timestamp = Module->TimeDateStamp;
			Dll->CheckSum = Module->CheckSum;
			RtlCopyMemory(&Dll->VersionInfo, &Module->VersionInfo, sizeof(VS_FIXEDFILEINFO));

			NameString = (PMINIDUMP_STRING)((PUCHAR)Buffer + Module->ModuleNameRva);
			StringCchCopy(Dll->Path, MAX_PATH, NameString->Buffer);

			//
			// Copy CvRecord, note that the pdb name may contains a full path, we
			// need split the path and reserve only its short file name 
			//

			CvRecord = &Dll->CvRecord;

			if (Module->CvRecord.Rva != 0) {

				PdbRecord = (PCV_INFO_PDB70)((PUCHAR)Buffer + Module->CvRecord.Rva);
				_splitpath(PdbRecord->PdbName, NULL, NULL, PdbName, Pdb); 

				StringCchPrintfA(CvRecord->PdbName, 64, "%s%s", PdbName, Pdb);
				CvRecord->CvRecordOffset = FIELD_OFFSET(BTR_CV_RECORD, CvSignature);
				CvRecord->CvSignature = PdbRecord->CvSignature;
				CvRecord->Signature = PdbRecord->Signature;
				CvRecord->Age = PdbRecord->Age;

				//
				// Compute size of CV_INFO_PDB70
				//

				Length = (ULONG)strlen(CvRecord->PdbName) + 1; 
				Length = FIELD_OFFSET(BTR_CV_RECORD, PdbName[Length]);
				CvRecord->SizeOfCvRecord = Length - CvRecord->CvRecordOffset;
				CvRecord->Reserved0 = 0;
				CvRecord->Reserved1 = 0;
				CvRecord->Timestamp = Dll->Timestamp;
				CvRecord->SizeOfImage = (ULONG)Dll->Size;

			} else {
				
				CvRecord->CvRecordOffset = FIELD_OFFSET(BTR_CV_RECORD, CvSignature);
				CvRecord->SizeOfCvRecord = 0;
				CvRecord->Reserved0 = 0;
				CvRecord->Reserved1 = 0;
				CvRecord->Timestamp = Dll->Timestamp;
				CvRecord->SizeOfImage = (ULONG)Dll->Size;
			}
		} 

		DllFile->Count = ModuleList->NumberOfModules;

		//
		// Write dll stream into report file
		//

		Status = WriteFile(FileHandle, DllFile, DllFileSize, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		} 

		Head->Streams[STREAM_DLL].Offset = Start.QuadPart;
		Head->Streams[STREAM_DLL].Length = DllFileSize;

		End->QuadPart = Start.QuadPart + DllFileSize;
		Status = APS_STATUS_OK;

	}
	__finally {

		if (Buffer != NULL) {
			UnmapViewOfFile(Buffer);
		}

		if (MappingHandle != NULL) {
			CloseHandle(MappingHandle);
		}

		if (DumpHandle != NULL) {
			CloseHandle(DumpHandle);
		}

		DeleteFile(Path);

		if (DllFile != NULL) {
			ApsFree(DllFile);
		}
	}

	if (Status != APS_STATUS_OK) {
		__debugbreak();
	}

	return Status;
}

ULONG
ApsWriteCoreDllStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	return 0;	
}

ULONG
ApsWriteStackStream(
	__in PAPS_PROFILE_OBJECT Profile,
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in HANDLE StackHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG Status;
	HANDLE MappingHandle;
	PBTR_STACK_RECORD Record;
	PBTR_STACK_ENTRY Entry;
	ULONG Number;
	ULONG StackId;
	ULONG EntryCount;
	ULONG RecordCount;
	ULONG Complete;
	ULONG Size;

	__try {

		//
		// Create file mapping for stack record file
		//

		MappingHandle = CreateFileMapping(StackHandle, NULL, 
			                              PAGE_READWRITE, 0, 0, 0);
		if (!MappingHandle) {

			//
			// N.B. It can fail due to zero sized file length,
			// this means that there's no stack trace collected
			//

			Status = GetLastError();
			__leave;
		}

		Record = (PBTR_STACK_RECORD)MapViewOfFile(MappingHandle, 
			                                      FILE_MAP_READ|FILE_MAP_WRITE, 
												  0, 0, 0);
		if (!Record) {
			Status = GetLastError();
			__leave;
		}

		//
		// Update stack record's counter based on its stack entry
		//

		Entry = (PBTR_STACK_ENTRY)ApsGetStreamPointer(Head, STREAM_STACK);
		EntryCount = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_ENTRY);
		RecordCount = GetFileSize(StackHandle, NULL) / sizeof(BTR_STACK_RECORD);
		ASSERT(EntryCount == RecordCount);

		for(Number = 0; Number < EntryCount; Number += 1) {
			StackId = Entry[Number].StackId;
			Record[StackId].Count = Entry[Number].Count;
			Record[StackId].SizeOfAllocs = Entry[Number].SizeOfAllocs;
		}
		
		//
		// Write the stack record stream to replace stack entries 
		//

		Size = sizeof(BTR_STACK_RECORD) * EntryCount;
		Start.QuadPart = Head->Streams[STREAM_STACK].Offset;
		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

		Status = WriteFile(FileHandle, Record, Size, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}

		//
		// Update only the stream length since it's offset don't change
		//

		Head->Streams[STREAM_STACK].Length = Size;
		End->QuadPart = Start.QuadPart + Size;
		Status = APS_STATUS_OK;

	}
	__finally {

		if (MappingHandle != NULL) {
			if (Record != NULL) {
				UnmapViewOfFile(Record);
			}
			CloseHandle(MappingHandle);
		}
	}

	return Status;
}

ULONG
ApsWriteIndexStream(
	IN PAPS_PROFILE_OBJECT Profile,
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN HANDLE IndexHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Status;
	ULONG Complete;
	ULONG Size;
	PVOID Buffer;
	HANDLE MappingHandle;

    Status = APS_STATUS_OK;
    Complete = 0;
    Size = 0;
    Buffer = NULL;
    MappingHandle = NULL;

	__try {

		Size = GetFileSize(IndexHandle, NULL);
		MappingHandle = CreateFileMapping(IndexHandle, NULL, PAGE_READWRITE, 0, 0, 0);
		if (!MappingHandle) {
			Status = GetLastError();
			__leave;
		}

		Buffer = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		if (!Buffer) {
			Status = GetLastError();
			__leave;
		}

		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
		Status = WriteFile(FileHandle, Buffer, Size, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}

		Head->Streams[STREAM_INDEX].Offset = Start.QuadPart;
		Head->Streams[STREAM_INDEX].Length = Size;

		End->QuadPart = Start.QuadPart + Size;
		SetFilePointerEx(FileHandle, *End, NULL, FILE_BEGIN);

		Status = APS_STATUS_OK;
	}
	__finally {

		if (Buffer != NULL) {
			UnmapViewOfFile(Buffer);
		}

		if (MappingHandle != NULL) {
			CloseHandle(MappingHandle);
		}
	}

	return Status;
}

ULONG
ApsWriteRecordStream(
	IN PAPS_PROFILE_OBJECT Profile,
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN HANDLE DataHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Status;
	ULONG Complete;
	ULONG Size;
	PVOID Buffer;
	HANDLE MappingHandle;

    Status = APS_STATUS_OK;
    Complete = 0;
    Size = 0;
    Buffer = NULL;
    MappingHandle = NULL;

	__try {

		Size = GetFileSize(DataHandle, NULL);
		MappingHandle = CreateFileMapping(DataHandle, NULL, PAGE_READWRITE, 0, 0, 0);
		if (!MappingHandle) {
			Status = GetLastError();
			__leave;
		}

		Buffer = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		if (!Buffer) {
			Status = GetLastError();
			__leave;
		}

		SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
		Status = WriteFile(FileHandle, Buffer, Size, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}

		Head->Streams[STREAM_RECORD].Offset = Start.QuadPart;
		Head->Streams[STREAM_RECORD].Length = Size;

		End->QuadPart = Start.QuadPart + Size;
		SetFilePointerEx(FileHandle, *End, NULL, FILE_BEGIN);

		Status = APS_STATUS_OK;
	}
	__finally {

		if (Buffer != NULL) {
			UnmapViewOfFile(Buffer);
		}

		if (MappingHandle != NULL) {
			CloseHandle(MappingHandle);
		}
	}

	return Status;
}

ULONG
ApsCreateCounterStreams(
	__in PWSTR Path
	)
{
	PBTR_PC_TABLE PcTable;
	ULONG Size;
	ULONG Status;
	PPF_REPORT_HEAD Head;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	HANDLE FileHandle;
	HANDLE MappingHandle;

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
    // Check whether counters already created 
    //

    ASSERT(Head->Complete != 0);
    if (Head->Counters != 0) {
		UnmapViewOfFile(Head);
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return APS_STATUS_OK;
	}

	//
	// Create PC streams based on profile types
	//

	if (Head->IncludeCpu) {
		ApsCreateCpuPcStream(Head, &PcTable);
	}
	
	if (Head->IncludeMm) {
		ApsCreateMmPcStream(Head, &PcTable);
	}

	//
	// Write the caller stream this is a special PC stream which contains only
	// directly caller of probe callback.
	//

	Size = GetFileSize(FileHandle, NULL);
	Start.QuadPart = Size;
	SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);

	__try {

		Status = ApsWritePcStream(Head, PcTable, FileHandle, Start, &End);
		if (Status != APS_STATUS_OK) {
			__leave;
		} 

		//
		// N.B. CPU/MM use different stream type to store pc stream, we may consider
		// to re-design this, for MM, STREAM_PC should be allocator's PC.
		//

		if (Head->IncludeCpu) {
			Head->Streams[STREAM_PC].Offset = Start.QuadPart;
			Head->Streams[STREAM_PC].Length = End.QuadPart - Start.QuadPart;
		}

		if (Head->IncludeMm) {
			Head->Streams[STREAM_CALLER].Offset = Start.QuadPart;
			Head->Streams[STREAM_CALLER].Length = End.QuadPart - Start.QuadPart;

   //         //
   //         // N.B. For MM profile, we also need decode all PCs for stack trace
   //         //

   //         ApsDestroyPcTable(PcTable);
   //         ApsCreatePcTable(&PcTable);

   //         ApsCreateCpuPcStream(Head, &PcTable);
   //         
   //         //
   //         // Update file position to write whole PC stream
   //         //

   //         Start.QuadPart = End.QuadPart;
   //         Status = ApsWritePcStream(Head, PcTable, FileHandle, Start, &End);
   //         if (Status != APS_STATUS_OK) {
   //             __leave;
   //         } 

			//Head->Streams[STREAM_PC].Offset = Start.QuadPart;
			//Head->Streams[STREAM_PC].Length = End.QuadPart - Start.QuadPart;
		}

		//
		// Generate leak streams
		//

		Start.QuadPart = End.QuadPart;

		if (Head->IncludeMm) {

			Status = ApsWriteHeapLeakStream(Head, FileHandle, Start, &End);
			if (Status != APS_STATUS_OK) {
				__leave;
			}

			Start.QuadPart = End.QuadPart;

			Status = ApsWritePageLeakStream(Head, FileHandle, Start, &End);
			if (Status != APS_STATUS_OK) {
				__leave;
			}

			Start.QuadPart = End.QuadPart;

			Status = ApsWriteHandleLeakStream(Head, FileHandle, Start, &End);
			if (Status != APS_STATUS_OK) {
				__leave;
			}

			Start.QuadPart = End.QuadPart;

			Status = ApsWriteGdiLeakStream(Head, FileHandle, Start, &End);
			if (Status != APS_STATUS_OK) {
				__leave;
			}

            //
            // Update counters by module for hot heap
            //

            Status = ApsUpdateHotHeapCounters(Head);
			if (Status != APS_STATUS_OK) {
				__leave;
			}

		}

		//
		// Decode all stack record to generate symbol stream
		//

	}
	__finally {

        //
        // Mark as analyzed and update checksum
        //

        if (Status == APS_STATUS_OK) {
            Head->Counters = 1;
            ApsComputeStreamCheckSum(Head);
        }

        if (Head) {
            UnmapViewOfFile(Head);
        }

        if (MappingHandle) {
            CloseHandle(MappingHandle);
        }

		SetEndOfFile(FileHandle);
		CloseHandle(FileHandle);
	}

	return Status;
}

ULONG
ApsCreatePcTable(
	__out PBTR_PC_TABLE *PcTable
	)
{
	ULONG i;
	PBTR_PC_TABLE Table;

	Table = (PBTR_PC_TABLE)ApsMalloc(sizeof(BTR_PC_TABLE));

	for(i = 0; i < STACK_PC_BUCKET; i++) {
		ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
		InitializeListHead(&Table->Hash[i].ListHead);
	}

	*PcTable = Table;
	return APS_STATUS_OK;
}

ULONG
ApsCreatePcTableFromStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *PcTable
	)
{
	ULONG Status;
	ULONG Bucket;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	PBTR_PC_ENTRY PcEntry;
	PBTR_PC_TABLE Table;
	PVOID Address;
	ULONG Count;
	ULONG Number;

	Status = APS_STATUS_OK;
	PcEntry = (PBTR_PC_ENTRY)ApsGetStreamPointer(Head, STREAM_PC);
	Count = ApsGetStreamRecordCount(Head, STREAM_PC, BTR_PC_ENTRY);

	//
	// Create Pc table
	//

	ApsCreatePcTable(&Table);

	for(Number = 0; Number < Count; Number += 1) {

		Address = PcEntry->Address;
		Bucket = HASH_BUCKET(Address, STACK_PC_BUCKET);

		Hash = &Table->Hash[Bucket];
		ListHead = &Hash->ListHead;

#if defined(_DEBUG)
		ApsHasDuplicatedPc(ListHead, Address);
#endif

		//
		// Insert tail list
		//

		InsertTailList(ListHead, &PcEntry->ListEntry);

		PcEntry += 1;
	}

	Table->Count = Count;
	*PcTable = Table;
	return Status;
}

BOOLEAN
ApsHasDuplicatedPc(
	_In_ PLIST_ENTRY ListHead,
	_In_ PVOID Address
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_PC_ENTRY Pc;

	ListEntry = ListHead->Flink;
	while (ListHead != ListEntry) {
		Pc = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
		if (Pc->Address == Address) {

			//
			// N.B. address should not have duplicated entries
			//

			ASSERT(0);
			return TRUE;
		}
		ListEntry = ListEntry->Flink;
	}
	return FALSE;
}

ULONG
ApsCreateCallerTableFromStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *PcTable
	)
{
	ULONG Status;
	ULONG Bucket;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	PBTR_PC_ENTRY PcEntry;
	PBTR_PC_TABLE Table;
	PVOID Address;
	ULONG Count;
	ULONG Number;

	Status = APS_STATUS_OK;
	PcEntry = (PBTR_PC_ENTRY)ApsGetStreamPointer(Head, STREAM_CALLER);
	ASSERT(PcEntry != NULL);

	Count = ApsGetStreamRecordCount(Head, STREAM_CALLER, BTR_PC_ENTRY);
	ASSERT(Count != 0);

	//
	// Create Pc table
	//

	ApsCreatePcTable(&Table);

	for(Number = 0; Number < Count; Number += 1) {

		Address = PcEntry->Address;
		Bucket = HASH_BUCKET(Address, STACK_PC_BUCKET);

		Hash = &Table->Hash[Bucket];
		ListHead = &Hash->ListHead;

#if defined(_DEBUG)
		ApsHasDuplicatedPc(ListHead, Address);
#endif

		//
		// Insert tail list
		//

		InsertTailList(ListHead, &PcEntry->ListEntry);

		PcEntry += 1;
	}

	Table->Count = Count;
	*PcTable = Table;
	return Status;
}

PBTR_PC_ENTRY
ApsLookupPcEntry(
	__in PVOID Address,
	__in PBTR_PC_TABLE PcTable
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	PBTR_PC_ENTRY Pc;

	Bucket = HASH_BUCKET(Address, STACK_PC_BUCKET);

	Hash = &PcTable->Hash[Bucket];
	ListHead = &Hash->ListHead;

	ListEntry = ListHead->Flink;
	while (ListHead != ListEntry) {
		Pc = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
		if (Pc->Address == Address) {
			return Pc;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
ApsCreateCpuPcStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *Table
	)
{
	PBTR_STACK_RECORD Stack;
	PBTR_STACK_RECORD Record;
	ULONG Count;
	PBTR_PC_TABLE PcTable;
	ULONG Number;
	ULONG Depth;
	ULONG Inclusive;
	ULONG Exclusive;
	PBTR_PC_ENTRY Pc;

	Inclusive = 0;
	Exclusive = 0;

	Stack = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

	ApsCreatePcTable(&PcTable);

	for(Number = 0; Number < Count; Number += 1) {
		
		Record = &Stack[Number];
		for(Depth = 0; Depth < Record->Depth; Depth += 1) {

            ASSERT(Record->Frame[Depth] != NULL);

			ApsInsertPcEntry(PcTable, Record->Frame[Depth], Record->Count, 
							 Record->Count * (Depth + 1), 0);

		}

		Exclusive += Record->Count * Record->Depth;
        Inclusive += Record->Count * Record->Depth;

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
ApsCreateMmPcStream(
	__in PPF_REPORT_HEAD Head,
	__out PBTR_PC_TABLE *Table
	)
{
	PBTR_STACK_RECORD Record;
	ULONG Count;
	PBTR_PC_TABLE PcTable;
	ULONG Number;

	Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

	ApsCreatePcTable(&PcTable);

    //
    // N.B. For MM profile, allocator's caller is gathered
    // to generate PC counter
    //

	for(Number = 0; Number < Count; Number += 1) {
		ApsInsertPcEntry(PcTable, Record[Number].Frame[1],
						 Record[Number].Count, 0,
						 Record[Number].SizeOfAllocs);
	}

	*Table = PcTable;
	return APS_STATUS_OK;
}

VOID
ApsDestroyPcTableFromStream(
	__in PBTR_PC_TABLE PcTable
	)
{
	ApsFree(PcTable);
}

ULONG
ApsDestroyPcTable(
	__in PBTR_PC_TABLE PcTable
	)
{
	ULONG i;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_PC_ENTRY PcEntry;

	for(i = 0; i < STACK_PC_BUCKET; i++) {
		ListHead = &PcTable->Hash[i].ListHead;
		while (IsListEmpty(ListHead) != TRUE) {
			ListEntry = RemoveHeadList(ListHead);
			PcEntry = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
			ApsFree(PcEntry);
		}
	}

	ApsFree(PcTable);
	return APS_STATUS_OK;
}

VOID
ApsInsertPcEntry(
	__in PBTR_PC_TABLE Table,
	__in PVOID Address,
	__in ULONG Count,
	__in ULONG Inclusive,
	__in ULONG64 Size
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	PBTR_PC_ENTRY Pc;

    if (!Address) {
        return;
    }
    
	Bucket = HASH_BUCKET(Address, STACK_PC_BUCKET);

	Hash = &Table->Hash[Bucket];
	ListHead = &Hash->ListHead;
	ListEntry = ListHead->Flink;

	while (ListHead != ListEntry) {
		Pc = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
		if (Pc->Address == Address) {

			//
			// Increase reference count for this Pc
			//

			Pc->Count += Count;
			Pc->Inclusive += Inclusive;
			Pc->SizeOfAllocs = Size;
			return;
		}

		ListEntry = ListEntry->Flink;
	}

	//
	// It's a new Pc, allocate and insert into hash table
	//

	Pc = (PBTR_PC_ENTRY)ApsMalloc(sizeof(BTR_PC_ENTRY));
	Pc->Address = Address;
	Pc->SizeOfAllocs = (ULONG64)Size;
	Pc->Count = Count;
	Pc->Inclusive = Inclusive;

    Pc->UseAddress = FALSE;
    Pc->DllId = -1;
	Pc->FunctionId = -1;
	Pc->LineId = -1;

	InsertHeadList(ListHead, &Pc->ListEntry);
	Table->Count += 1;
}

VOID
ApsInsertPcEntryEx(
	__in PBTR_PC_TABLE Table,
	__in PBTR_PC_ENTRY PcEntry,
    __in ULONG Times,
	__in ULONG Depth,
	__in BTR_PROFILE_TYPE Type
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	PBTR_PC_ENTRY Pc;
	ULONG64 Address;

	ASSERT(PcEntry->Address != NULL);

	Address = (ULONG64)PcEntry->Address;
	Bucket = HASH_BUCKET(Address, STACK_PC_BUCKET);

	Hash = &Table->Hash[Bucket];
	ListHead = &Hash->ListHead;
	ListEntry = ListHead->Flink;

	while (ListHead != ListEntry) {
		Pc = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
		if (Pc->Address == (PVOID)Address) {

			//
			// Found, update its count and free passed PcEntry
			//

			if (Type == PROFILE_CPU_TYPE) {
				if (Depth == 0) {
					Pc->Inclusive += 1 * Times;
					Pc->Exclusive += 1 * Times;
				}
				else {
					Pc->Inclusive += 1 * Times;
				}
			}
			if (Type == PROFILE_MM_TYPE) {
				Pc->SizeOfAllocs += PcEntry->SizeOfAllocs;
			}

			Pc->Count += 1 * Times;
			ApsFree(PcEntry);
			return;
		}
		ListEntry = ListEntry->Flink;
	}

	//
	// It's new entry, insert it
	//

	if (Type == PROFILE_CPU_TYPE) {
		if (Depth == 0) {
			PcEntry->Inclusive = 1 * Times;
			PcEntry->Exclusive = 1 * Times;
		}
		else {
			PcEntry->Inclusive = 1 * Times;
			PcEntry->Exclusive = 0;
		}
	}

	PcEntry->Count = 1 * Times;
	InsertHeadList(ListHead, &PcEntry->ListEntry);
	Table->Count += 1;
}

ULONG
ApsWriteCallerStream(
	__in PPF_REPORT_HEAD Head,
	__in PBTR_PC_TABLE Table,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG Status;
	ULONG Size;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_PC_ENTRY Source;
	PBTR_PC_ENTRY Destine;
	ULONG Complete;
	ULONG Number;
	PBTR_DLL_FILE DllFile;
	ULONG i;

	//
	// Compute PC file size and allocate from page file
	//

	Size = Table->Count * sizeof(BTR_PC_ENTRY);
    Destine = (PBTR_PC_ENTRY)ApsMalloc(Size);

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);

	//
	// Serialize all PC buckets into PC file
	//

	Number = 0;
	for(i = 0; i < STACK_PC_BUCKET; i++) {

		ListHead = &Table->Hash[i].ListHead;
		ListEntry = ListHead->Flink;

		while (ListEntry != ListHead) {

			Source = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
			Destine[Number] = *Source;
			Destine[Number].DllId = (USHORT)ApsGetDllIdByAddress(DllFile, (ULONG64)Source->Address);

			ListEntry = ListEntry->Flink;
			Number += 1;
		}
	}

	ASSERT(Number == Table->Count);

    //
    // Sort the PCs in place
    //

	qsort(&Destine[0], Number, sizeof(BTR_PC_ENTRY), ApsPcCompareCallback);

	Status = WriteFile(FileHandle, Destine, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		End->QuadPart = Start.QuadPart;
	} else {
		Status = APS_STATUS_OK;
		End->QuadPart = Start.QuadPart + Size;
	}

    ApsFree(Destine);
	return Status;
}

ULONG
ApsWritePcStream(
	__in PPF_REPORT_HEAD Head,
	__in PBTR_PC_TABLE Table,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG Status;
	ULONG Size;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_PC_ENTRY Source;
	PBTR_PC_ENTRY Destine;
	HANDLE Mapping;
	ULONG Complete;
	ULONG Number;
	PBTR_DLL_FILE DllFile;
	ULONG i;

	//
	// Compute PC file size and allocate from page file
	//

	Size = Table->Count * sizeof(BTR_PC_ENTRY);
	Mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, NULL);
	if (!Mapping) {
		return GetLastError();
	}

	Destine = (PBTR_PC_ENTRY)MapViewOfFile(Mapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Destine) {
		Status = GetLastError();
		CloseHandle(Mapping);
		return Status;
	}

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);

	//
	// Serialize all PC buckets into PC file
	//

	Number = 0;
	for(i = 0; i < STACK_PC_BUCKET; i++) {

		ListHead = &Table->Hash[i].ListHead;
		ListEntry = ListHead->Flink;

		while (ListEntry != ListHead) {

			Source = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
			Destine[Number] = *Source;
			Destine[Number].DllId = (USHORT)ApsGetDllIdByAddress(DllFile, (ULONG64)Source->Address);
			
			ListEntry = ListEntry->Flink;
			Number += 1;
		}
	}

	ASSERT(Number == Table->Count);

	//
	// Sort the Pc entries in decreasing order by its Count field
	// note that CPU pc entry 0 tracks counters, it's always at position 0
	//
	
	if (Head->IncludeCpu) {
		qsort(&Destine[1], Number - 1, sizeof(BTR_PC_ENTRY), ApsPcCompareCallback);
	} 
   
    //
    // MM profile does not need sorting
    //
    
	else {
		qsort(&Destine[0], Number, sizeof(BTR_PC_ENTRY), ApsPcCompareCallback);
	}

	Status = WriteFile(FileHandle, Destine, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		End->QuadPart = Start.QuadPart;
	} else {
		Status = APS_STATUS_OK;
		End->QuadPart = Start.QuadPart + Size;
	}

	UnmapViewOfFile(Destine);
	CloseHandle(Mapping);

	return Status;
}

ULONG
ApsWritePcStreamEx(
	__in PPF_REPORT_HEAD Head,
	__in PBTR_PC_TABLE Table,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG Status;
	ULONG Size;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_PC_ENTRY Source;
	PBTR_PC_ENTRY Destine;
	ULONG Complete;
	ULONG Number;
	ULONG i;

	//
	// N.B. The root entry tracks total counters for CPU profile,
    // for MM profile, currently it's useless 
    //

    Size = Table->Count * sizeof(BTR_PC_ENTRY);
	Destine = (PBTR_PC_ENTRY)ApsMalloc(Size);

	//
	// Serialize all PC buckets into PC file
	//

	Number = 0;

	for(i = 0; i < STACK_PC_BUCKET; i++) {

		ListHead = &Table->Hash[i].ListHead;
		ListEntry = ListHead->Flink;

		while (ListEntry != ListHead) {
			Source = CONTAINING_RECORD(ListEntry, BTR_PC_ENTRY, ListEntry);
			Destine[Number] = *Source;
			ListEntry = ListEntry->Flink;
			Number += 1;
		}
	}

	ASSERT(Number == Table->Count);

	//
	// Sort the Pc entries in decreasing order by its Count field
	//
	
    qsort(&Destine[0], Number, sizeof(BTR_PC_ENTRY), ApsPcCompareCallback);

	Status = WriteFile(FileHandle, Destine, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		End->QuadPart = Start.QuadPart;
	} else {
		Status = APS_STATUS_OK;
		End->QuadPart = Start.QuadPart + Size;
	}

	ApsFree(Destine);
	return Status;
}

ULONG
ApsGetDllIdByAddress(
	__in PBTR_DLL_FILE DllFile,
	__in ULONG64 Address
	)
{
	ULONG i;
	PBTR_DLL_ENTRY DllEntry;

	for(i = 0; i < DllFile->Count; i++) {

		DllEntry = &DllFile->Dll[i];

		if (Address >= (ULONG64)DllEntry->BaseVa && 
			Address < (ULONG64)DllEntry->BaseVa + DllEntry->Size) {
			return DllEntry->DllId;
		}
	}

	return -1;
}

int __cdecl
ApsPcCompareCallback(
	__in const void *Ptr1,
	__in const void *Ptr2 
	)
{
	PBTR_PC_ENTRY Pc1;
	PBTR_PC_ENTRY Pc2;

	Pc1 = (PBTR_PC_ENTRY)Ptr1;
	Pc2 = (PBTR_PC_ENTRY)Ptr2;

	//
	// N.B. Pc is sorted in decreasing order
	//

	return Pc2->Count - Pc1->Count;
}

int __cdecl
ApsStackCompareCallback(
	__in const void *Ptr1,
	__in const void *Ptr2 
	)
{
	PBTR_STACK_RECORD Stack1;
	PBTR_STACK_RECORD Stack2;

	Stack1 = (PBTR_STACK_RECORD)Ptr1;
	Stack2 = (PBTR_STACK_RECORD)Ptr2;
	
	return Stack2->Count - Stack1->Count;
}
	
int __cdecl
ApsStackIdCompareCallback(
	__in const void *Ptr1,
	__in const void *Ptr2 
	)
{
	PBTR_STACK_RECORD Stack1;
	PBTR_STACK_RECORD Stack2;

	Stack1 = (PBTR_STACK_RECORD)Ptr1;
	Stack2 = (PBTR_STACK_RECORD)Ptr2;
	
	return Stack1->StackId - Stack2->StackId;
}

PVOID
ApsGetStreamPointer(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
	)
{
	if (Head->Streams[Stream].Offset != 0 && Head->Streams[Stream].Length != 0) {
		return (PUCHAR)Head + Head->Streams[Stream].Offset;
	}

	return NULL;
}

ULONG
ApsGetStreamLength(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
	)
{
	if (Head->Streams[Stream].Offset != 0) {
		return (ULONG)Head->Streams[Stream].Length;
	}

	return 0;
}

BOOLEAN 
ApsIsStreamValid(
	__in PPF_REPORT_HEAD Head,
	__in PF_STREAM_TYPE Stream
    )
{
    if (Head->Streams[Stream].Offset != 0 && Head->Streams[Stream].Length != 0) {
        return TRUE;
    }

    return FALSE;
}

ULONG
ApsWriteHeapLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG i, j;
	PPF_STREAM_HEAP Heap;
	PPF_STREAM_HEAPLIST HeapList;
	PBTR_LEAK_TABLE Table;
	PBTR_LEAK_FILE HeapFile;
	LIST_ENTRY LeakHeapList;
	PLIST_ENTRY ListEntry;
	PPF_STREAM_HEAP_LEAK HeapLeak;
	ULONG Count;
	ULONG Complete;
	ULONG HeapHeadSize;
	ULONG Size;
	LARGE_INTEGER Offset;
    ULONG64 TotalBytes;
    ULONG64 HeapBytes;
    ULONG64 TotalCount;
	ULONG64 TotalLeakCount;

	if (Head->Streams[STREAM_MM_HEAP].Offset == 0) {
		Head->Streams[STREAM_LEAK_HEAP].Offset = 0;
		Head->Streams[STREAM_LEAK_HEAP].Length = 0;
		End->QuadPart = Start.QuadPart;
		return APS_STATUS_OK;
	}

	Count = 0;
    TotalBytes = 0;
    TotalCount = 0;
    TotalLeakCount = 0;

	InitializeListHead(&LeakHeapList);
	Offset.QuadPart = Start.QuadPart;

	//
	// Scan heap tables to build heap leak list
	//

	HeapList = (PPF_STREAM_HEAPLIST)ApsGetStreamPointer(Head, STREAM_MM_HEAP);

	//
	// N.B. Currently we don't scan retired heaps, but it's better to
	// configure whether we should scan retired heap.
	//

	for(i = 0; i < HeapList->NumberOfActiveHeaps; i++) {

		Heap = (PPF_STREAM_HEAP)((PUCHAR)Head + HeapList->Heaps[i].Offset);	

		if (!Heap->NumberOfRecords) {
			continue;
		}

		//
		// Insert outstanding allocation record into leak table,
		// it's hashed by record's stack id
		//

		Table = ApsCreateLeakTable();
        HeapBytes = 0;

		for(j = 0; j < Heap->NumberOfRecords; j++) {

			ApsInsertLeakEntry(Table, Heap->Records[j].Address, 
							   Heap->Records[j].Size, 
				               Heap->Records[j].CallbackType,
				               Heap->Records[j].StackHash);

            HeapBytes += Heap->Records[j].Size;
		}

		//
		// N.B. Leak table is freed after ApsCreateLeakFile, don't
		// touch it anymore, use returned leak file instead
		//

		HeapFile = ApsCreateLeakFile(Table);
		HeapFile->Context = Heap->HeapHandle;

        HeapFile->Bytes = HeapBytes;
        TotalBytes += HeapBytes;
        TotalCount += HeapFile->Count;
        TotalLeakCount += HeapFile->LeakCount;

		InsertTailList(&LeakHeapList, &HeapFile->ListEntry);
		Count += 1;
	}

	HeapHeadSize = FIELD_OFFSET(PF_STREAM_HEAP_LEAK, Info[Count]);
	HeapLeak = (PPF_STREAM_HEAP_LEAK)ApsMalloc(HeapHeadSize);
    HeapLeak->Count = TotalCount;
	HeapLeak->LeakCount = TotalLeakCount;
    HeapLeak->Bytes = TotalBytes;

	//
	// Skip the heap head, it's written after all heap streams written
	//

	Offset.QuadPart = Start.QuadPart + HeapHeadSize;	
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);

	//
	// Write heap leak stream
	//

	HeapLeak->NumberOfHeaps = Count;

	i = 0;
	while (IsListEmpty(&LeakHeapList) != TRUE) {

		ListEntry = RemoveHeadList(&LeakHeapList);
		HeapFile = CONTAINING_RECORD(ListEntry, BTR_LEAK_FILE, ListEntry);

		Size = FIELD_OFFSET(BTR_LEAK_FILE, Leak[HeapFile->Count]);
		WriteFile(FileHandle, HeapFile, Size, &Complete, NULL);
		ApsFree(HeapFile);

		HeapLeak->Info[i].Offset = Offset.QuadPart;
		HeapLeak->Info[i].Length = Size;
		Offset.QuadPart = Offset.QuadPart + Size;

		i += 1;
	}

	//
	// Write heap leak head
	//

	SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
	WriteFile(FileHandle, HeapLeak, HeapHeadSize, &Complete, NULL);

	//
	// Fill the stream information in report head
	//

	Head->Streams[STREAM_LEAK_HEAP].Offset = Start.QuadPart;
	Head->Streams[STREAM_LEAK_HEAP].Length = Offset.QuadPart - Start.QuadPart;
	End->QuadPart = Offset.QuadPart;

	SetFilePointerEx(FileHandle, *End, NULL, FILE_BEGIN);
	return APS_STATUS_OK;
}

ULONG
ApsWritePageLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	PPF_STREAM_PAGE Page;
	PPF_STREAM_PAGE_LEAK PageFile;
	PBTR_LEAK_TABLE Table;
	ULONG i;
	ULONG Size;
	ULONG Complete;  
	ULONG64 TotalBytes;

	if (Head->Streams[STREAM_MM_PAGE].Length == 0) {

		Head->Streams[STREAM_LEAK_PAGE].Offset = 0;
		Head->Streams[STREAM_LEAK_PAGE].Length = 0;

		End->QuadPart = Start.QuadPart;
		return APS_STATUS_OK;
	}

	Page = (PPF_STREAM_PAGE)ApsGetStreamPointer(Head, STREAM_MM_PAGE);
	Table = ApsCreateLeakTable();
	TotalBytes = 0;

	for(i = 0; i < Page->NumberOfRecords; i++) {

		ApsInsertLeakEntry(Table, Page->Records[i].Address, 
						   Page->Records[i].Size, 
						   Page->Records[i].CallbackType,
						   Page->Records[i].StackHash);

		TotalBytes += Page->Records[i].Size;
	}

	PageFile = ApsCreateLeakFile(Table);
	PageFile->Bytes = TotalBytes;

	Size = FIELD_OFFSET(BTR_LEAK_FILE, Leak[PageFile->Count]);

	SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
	WriteFile(FileHandle, PageFile, Size, &Complete, NULL);
	ApsFree(PageFile);

	Head->Streams[STREAM_LEAK_PAGE].Offset = Start.QuadPart;
	Head->Streams[STREAM_LEAK_PAGE].Length = Size;
	End->QuadPart = Start.QuadPart + Size;
			
	return APS_STATUS_OK;
}


ULONG
ApsWriteHandleLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	PPF_STREAM_HANDLE Handle;
	PPF_STREAM_HANDLE_LEAK HandleFile;
	PBTR_LEAK_TABLE Table;
	ULONG i;
	ULONG Size;
	ULONG Complete;

	if (Head->Streams[STREAM_MM_HANDLE].Length == 0) {
		
		Head->Streams[STREAM_LEAK_HANDLE].Offset = 0;
		Head->Streams[STREAM_LEAK_HANDLE].Length = 0;

		End->QuadPart = Start.QuadPart;
		return APS_STATUS_OK;	
	}

	Handle = (PPF_STREAM_HANDLE)ApsGetStreamPointer(Head, STREAM_MM_HANDLE);
	Table = ApsCreateLeakTable();

	for(i = 0; i < Handle->NumberOfRecords; i++) {
		ApsInsertLeakEntry(Table, Handle->Records[i].Value, 0,
			               Handle->Records[i].CallbackType,
			               Handle->Records[i].StackHash);
	}

	HandleFile = ApsCreateLeakFile(Table);
	Size = FIELD_OFFSET(BTR_LEAK_FILE, Leak[HandleFile->Count]);

	SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
	WriteFile(FileHandle, HandleFile, Size, &Complete, NULL);
	ApsFree(HandleFile);

	Head->Streams[STREAM_LEAK_HANDLE].Offset = Start.QuadPart;
	Head->Streams[STREAM_LEAK_HANDLE].Length = Size;
	End->QuadPart = Start.QuadPart + Size;

	return APS_STATUS_OK;
}

ULONG
ApsWriteGdiLeakStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	PPF_STREAM_GDI Gdi;
	PPF_STREAM_GDI_LEAK GdiFile;
	PBTR_LEAK_TABLE Table;
	ULONG i;
	ULONG Size;
	ULONG Complete;

	if (Head->Streams[STREAM_MM_GDI].Length == 0) {
		
		Head->Streams[STREAM_LEAK_GDI].Offset = 0;
		Head->Streams[STREAM_LEAK_GDI].Length = 0;

		End->QuadPart = Start.QuadPart;
		return APS_STATUS_OK;
	}

	Gdi = (PPF_STREAM_GDI)ApsGetStreamPointer(Head, STREAM_MM_GDI);
	Table = ApsCreateLeakTable();

	for(i = 0; i < Gdi->NumberOfRecords; i++) {
		ApsInsertLeakEntry(Table, Gdi->Records[i].Value, 0,
						   Gdi->Records[i].CallbackType,
						   Gdi->Records[i].StackHash);
	}

	GdiFile = ApsCreateLeakFile(Table);
	Size = FIELD_OFFSET(BTR_LEAK_FILE, Leak[GdiFile->Count]);

	SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
	WriteFile(FileHandle, GdiFile, Size, &Complete, NULL);
	ApsFree(GdiFile);

	Head->Streams[STREAM_LEAK_GDI].Offset = Start.QuadPart;
	Head->Streams[STREAM_LEAK_GDI].Length = Size;
	End->QuadPart = Start.QuadPart + Size;

	return APS_STATUS_OK;
}

PBTR_LEAK_TABLE
ApsCreateLeakTable(
	VOID
	)
{
	PBTR_LEAK_TABLE Table;
	ULONG i;

	Table = (PBTR_LEAK_TABLE)ApsMalloc(sizeof(BTR_LEAK_TABLE));
	Table->Count = 0;

	for(i = 0; i < STACK_LEAK_BUCKET; i++) {
		ApsInitSpinLock(&Table->Hash[i].SpinLock, 100);
		InitializeListHead(&Table->Hash[i].ListHead);
	}

	return Table;
}

PBTR_LEAK_FILE
ApsCreateLeakFile(
	__in PBTR_LEAK_TABLE Table
	)
{
	ULONG Size;
	PBTR_LEAK_FILE File;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_LEAK_ENTRY Entry;
	ULONG i, j;
	ULONG Count;

	Count = Table->Count;
	Size = FIELD_OFFSET(BTR_LEAK_FILE, Leak[Count]);

	File = (PBTR_LEAK_FILE)ApsMalloc(Size);
	File->Count = Count;
	File->LeakCount = 0;

	for(i = 0, j = 0; i < STACK_LEAK_BUCKET; i++) {

		ListHead = &Table->Hash[i].ListHead;

		while (IsListEmpty(ListHead) != TRUE) {

			ListEntry = RemoveHeadList(ListHead);
			Entry = CONTAINING_RECORD(ListEntry, BTR_LEAK_ENTRY, ListEntry);

			File->Leak[j] = *Entry;
			File->LeakCount += Entry->Count;
			j += 1;

			ApsFree(Entry);
		}
	}

	ApsFree(Table);
	return File;
}

VOID
ApsInsertLeakEntry(
	__in PBTR_LEAK_TABLE Table,
	__in PVOID Ptr,
	__in ULONG Size,
	__in ULONG Allocator,
	__in ULONG StackId
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	PBTR_LEAK_ENTRY Leak;

	//
	// N.B. We use stack id as key to hash bucket,
	// because leak analysis is stack id central, for
	// same stack id records, their other attributes
	// should be also identical.
	//

	Bucket = HASH_BUCKET(StackId, STACK_LEAK_BUCKET);

	Hash = &Table->Hash[Bucket];
	ListHead = &Hash->ListHead;
	ListEntry = ListHead->Flink;

	while (ListHead != ListEntry) {
		Leak = CONTAINING_RECORD(ListEntry, BTR_LEAK_ENTRY, ListEntry);
		if (Leak->StackId == StackId) {
			Leak->Count += 1;
			Leak->Size += Size;
			return;
		}

		ListEntry = ListEntry->Flink;
	}

	//
	// It's a new leak stack id, allocate and insert into hash table
	//

	Leak = (PBTR_LEAK_ENTRY)ApsMalloc(sizeof(BTR_LEAK_ENTRY));
	Leak->Allocator = Allocator;
	Leak->StackId = StackId;
	Leak->Count = 1;
	Leak->Size = Size;

	InsertHeadList(ListHead, &Leak->ListEntry);
	Table->Count += 1;
}

ULONG
ApsCreateHotStackStream(
	IN PPF_REPORT_HEAD Head
	)
{
    return APS_STATUS_OK;
}

ULONG
ApsUpdateHotHeapCounters(
	IN PPF_REPORT_HEAD Head
	)
{
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_STACK_RECORD BackTrace;
	ULONG DllId;
	ULONG Number;
	ULONG Count;

	//
	// Initialize dll file
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);

	for(Number = 0; Number < DllFile->Count; Number += 1) {
		InitializeListHead(&DllFile->Dll[Number].ListHead);
		DllFile->Dll[Number].Count = 0;
        DllFile->Dll[Number].CountOfAllocs = 0;
        DllFile->Dll[Number].SizeOfAllocs = 0;
	}

	BackTrace = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

	//
	// Scan all stack traces and chain each record into dll entry by its caller
	//

	for(Number = 0; Number < Count; Number += 1) {

		if (!BackTrace->Heap) {

            //
            // Skip non-heap stack trace record
            //

			BackTrace += 1;
			continue;
		}

		DllId = ApsGetDllIdByAddress(DllFile, (ULONG64)BackTrace->Frame[1]);

		if (DllId != (ULONG)-1) {
			DllEntry = &DllFile->Dll[DllId];
			DllEntry->Count += 1;
			DllEntry->CountOfAllocs += BackTrace->Count;
			DllEntry->SizeOfAllocs += BackTrace->SizeOfAllocs;
		}

		BackTrace += 1;
	}

    return APS_STATUS_OK;
}

ULONG
ApsOpenReport(
	__in PWSTR ReportPath,
	__out PHANDLE FileObject,
	__out PHANDLE MappingObject,
	__out PPF_REPORT_HEAD *Report
	)
{
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PPF_REPORT_HEAD Head;
	ULONG Size;
	ULONG Status;

	*FileObject = NULL;
	*MappingObject = NULL;
	*Report = NULL;

	FileHandle = CreateFile(ReportPath, GENERIC_READ|GENERIC_WRITE, 
		                    FILE_SHARE_READ|FILE_SHARE_WRITE,
			                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	__try {

		MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (!MappingHandle) {
			Status = GetLastError();
			__leave;
		}

		Size = GetFileSize(FileHandle, NULL);
		Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		if (!Head) {
			Status = GetLastError();
			__leave;
		}

		Status = APS_STATUS_OK;

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

		} else {

			*FileObject = FileHandle;
			*MappingObject = MappingHandle;
			*Report = Head;
		}
	}

	return Status;
}

ULONG
ApsReadReportHead(
	__in PWSTR ReportPath,
	__out PPF_REPORT_HEAD Head 
	)
{
	HANDLE FileHandle;
	ULONG Complete;
	ULONG Status;

	FileHandle = CreateFile(ReportPath, GENERIC_READ|GENERIC_WRITE, 
		                    FILE_SHARE_READ|FILE_SHARE_WRITE,
			                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}
    
    // 
    // N.B. From MSDN
    // When a synchronous read operation reaches the end of a file, 
    // ReadFile returns TRUE and sets *lpNumberOfBytesRead to zero
    //
    // If the file has a zero size, nNumberOfBytesToRead > 0, 
    // ReadFile will return TRUE !, So we should always to check
    // lpNumberOfBytesRead to determine whether the I/O is successfully
    // completed.
    //

	Status = ReadFile(FileHandle, Head, sizeof(*Head), &Complete, NULL);
	if (!Status) {
		Status = GetLastError();

	} else {

        if (Complete == 0) {
            Status = APS_STATUS_BAD_FILE; 

        } else {
            Status = APS_STATUS_OK;
        }
	}

    //
    // Verfiy report head
    //

    if (Status == APS_STATUS_OK) {
        Status = ApsVerifyReportHead(Head);
    }

	CloseHandle(FileHandle);
	return Status;
}

ULONG
ApsVerifyReportHead(
    __in PPF_REPORT_HEAD Head
    )
{
    ULONG Status;
    ULONG Number;
	ULONG NewCheckSum;
	ULONG OldCheckSum;
	PUCHAR Buffer;

    //
    // Verify whether it was a completed report
    //

    if (!Head->Complete) {
        return APS_STATUS_BAD_FILE;
    }

    //
    // Save old checksum
    //

    NewCheckSum = 0;
    OldCheckSum = Head->CheckSum;

    //
    // N.B. Clear head's checksum, Context field
    // is never computed since it's used after the
    // report is read into memory it's not part of
    // the report itself, only a facility to associate
    // the report and its callers
    //

    Head->CheckSum = 0;
    Head->Context = NULL;

	Buffer = (PUCHAR)Head;

	for(Number = 0; Number < sizeof(PF_REPORT_HEAD); Number += 1) {
		NewCheckSum += (ULONG)Buffer[Number];		
	}	

    if (OldCheckSum != NewCheckSum) {
        Status = APS_STATUS_BAD_CHECKSUM;
    } else {
        Status = APS_STATUS_OK;
    }

    //
    // Restore old checksum
    //

	Head->CheckSum = OldCheckSum;
    return Status;
}

ULONG
ApsPrepareForAnalysis(
	__in PWSTR ReportPath
	)
{
	ULONG Status;
	PPF_REPORT_HEAD Head;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	LARGE_INTEGER End;
    PBTR_DLL_FILE DllFile;

	Status = ApsMapReportFile(ReportPath, &Head, 
		                      &MappingHandle, &FileHandle);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	if (!Head->Complete) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_BAD_FILE;
	}

	//
	// If the report has not yet create counters,
	// or not yet analyzed, just return here
	//

	if (!Head->Counters || !Head->Analyzed) {
		ApsUnmapReportFile(Head, MappingHandle, FileHandle);
		return APS_STATUS_OK;
	}

    if (Head->IncludeCpu) {
        DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
        ApsClearDllFileCounters(DllFile);
    }

	//
	// The report file must have been analyzed 
	// truncate the symbol streams reserve the 
	// counter streams
	//

	Head->Streams[STREAM_PC].Offset = 0;
	Head->Streams[STREAM_PC].Length = 0;
	Head->Streams[STREAM_FUNCTION].Offset = 0;
	Head->Streams[STREAM_FUNCTION].Length = 0;
	Head->Streams[STREAM_SYMBOL].Offset = 0;
	Head->Streams[STREAM_SYMBOL].Length = 0;
	Head->Streams[STREAM_LINE].Offset = 0;
	Head->Streams[STREAM_LINE].Length = 0;

	//
	// Clear flags and update head checksum
	//

	Head->Analyzed = 0;
	ApsComputeStreamCheckSum(Head);

	//
	// Set file size which does not include symbol streams
	//

	End.QuadPart = Head->FileSize;
	SetFilePointerEx(FileHandle, End, NULL, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	ApsUnmapReportFile(Head, MappingHandle, FileHandle);
	return APS_STATUS_OK;
}

VOID
ApsClearHotHeapCounters(
	__in PPF_REPORT_HEAD Head
	)
{
	PBTR_DLL_FILE DllFile;
	ULONG Number;

	//
	// Initialize dll file
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);

	for(Number = 0; Number < DllFile->Count; Number += 1) {
		InitializeListHead(&DllFile->Dll[Number].ListHead);
		DllFile->Dll[Number].Count = 0;
        DllFile->Dll[Number].CountOfAllocs = 0;
        DllFile->Dll[Number].SizeOfAllocs = 0;
	}
}

PBTR_DLL_ENTRY
ApsGetDllEntryById(
    __in PPF_REPORT_HEAD Head,
    __in ULONG Id
    )
{
    PBTR_DLL_FILE DllFile;
    PBTR_DLL_ENTRY DllEntry;

    DllFile = (PBTR_DLL_FILE)((PUCHAR)Head + Head->Streams[STREAM_DLL].Offset);
    if (Id < DllFile->Count) {
        DllEntry = (PBTR_DLL_ENTRY)&DllFile->Dll[Id]; 
    } else {
        DllEntry = NULL;
    }

    return DllEntry;
}

VOID
ApsGetDllBaseNameById(
    __in PPF_REPORT_HEAD Head,
    __in ULONG Id,
    __out PWSTR Buffer,
    __in ULONG Length
    )
{
    PBTR_DLL_FILE DllFile;
    PBTR_DLL_ENTRY DllEntry;
    WCHAR Name[MAX_PATH];
    WCHAR Ext[MAX_PATH];

    DllFile = (PBTR_DLL_FILE)((PUCHAR)Head + Head->Streams[STREAM_DLL].Offset);
    if (Id < DllFile->Count) {

        DllEntry = (PBTR_DLL_ENTRY)&DllFile->Dll[Id]; 
        _wsplitpath(DllEntry->Path, NULL, NULL, Name, Ext);
		_wcslwr(Name);

        if (Ext[0] != 0) {
            swprintf_s(Buffer, Length, L"%s%s", Name, Ext);
        } else {
            swprintf_s(Buffer, Length, L"%s", Name);
        }

    } else {
        wcscpy_s(Buffer, Length, L"N/A");
    }

}

VOID
ApsClearDllFileCounters(
    __in PBTR_DLL_FILE DllFile
    )
{
    ULONG Number;

	for(Number = 0; Number < DllFile->Count; Number += 1) {
		InitializeListHead(&DllFile->Dll[Number].ListHead);
		DllFile->Dll[Number].Inclusive = 0;
		DllFile->Dll[Number].Exclusive = 0;
        DllFile->Dll[Number].SizeOfAllocs = 0;
	}
}

VOID
ApsInitializeDllFile(
    __in PBTR_DLL_FILE DllFile
    )
{
    ULONG Number;
	for(Number = 0; Number < DllFile->Count; Number += 1) {
		InitializeListHead(&DllFile->Dll[Number].ListHead);
	}
}

ULONG
ApsGetSampleCount(
	__in PPF_REPORT_HEAD Head
	)
{
	PBTR_STACK_RECORD Record;
	ULONG Count;
	ULONG Number;
	ULONG Total = 0;

	Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

	for(Number = 0; Number < Count; Number += 1) {
		Total += Record->Count;
		Record += 1;
	}

	return Total;
}

VOID
ApsGetCpuSampleCounters(
	__in PPF_REPORT_HEAD Head,
	__out PULONG Inclusive,
	__out PULONG Exclusive
	)
{
	PBTR_STACK_RECORD Record;
	ULONG Count;
	ULONG Number;

	*Inclusive = 0;
	*Exclusive = 0;

	Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

	for(Number = 0; Number < Count; Number += 1) {
		*Exclusive += Record->Count;
		*Inclusive += Record->Count;
		Record += 1;
	}

}

VOID
ApsInitReportContext(
	__in PPF_REPORT_HEAD Head
	)
{
	if (Head->IncludeCpu) {
		CpuBuildAddressTable(Head);
	}
}
