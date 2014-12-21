//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "stream.h"
#include "mmprof.h"
#include "mmheap.h"
#include "lock.h"
#include "heap.h"
#include <psapi.h>
#include <dbghelp.h>
#include "util.h"
#include "stacktrace.h"
#include "cache.h"
#include "btr.h"
#include "stream.h"
#include "marker.h"
#include "cpuprof.h"


ULONG
BtrWriteHeapStream(
	IN PMM_HEAP_TABLE Table,
	IN HANDLE FileHandle,
	OUT PULONG Length
	)
{
	ULONG Count;
	ULONG Size;
	PPF_STREAM_HEAP Heap;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PPF_HEAP_RECORD Record;
	ULONG Complete;
	ULONG Status;
	ULONG i, j;
	LIST_ENTRY Queue;

	Count = Table->NumberOfAllocs - Table->NumberOfFrees;
	ASSERT(Count >= 0);
	
	//
	// N.B. Be careful to handle 0 case, this indicate that
	// no outstanding allocation for this heap, however, we
	// still need its other information
	//

	Size = FIELD_OFFSET(PF_STREAM_HEAP, Records[Count]);
	Heap = BtrMalloc(Size);

	Heap->HeapHandle = Table->HeapHandle;
	Heap->Flags = 0;
	Heap->OwnerName[0] = 0;
	Heap->NumberOfRecords = Count;

	Heap->ReservedBytes = Table->ReservedBytes;
	Heap->CommittedBytes = Table->CommittedBytes;
	Heap->AllocatedBytes = Table->AllocatedBytes;
	Heap->SizeOfAllocs = Table->SizeOfAllocs;
	Heap->SizeOfFrees = Table->SizeOfFrees;
	Heap->NumberOfAllocs = Table->NumberOfAllocs;
	Heap->NumberOfFrees = Table->NumberOfFrees;
	Heap->CreateStackHash = Table->CreateStackHash;
	Heap->CreateStackDepth = Table->CreateStackDepth;
	Heap->CreatePc = Table->CreatePc;
	Heap->DestroyStackHash = Table->DestroyStackHash;
	Heap->DestroyStackDepth = Table->DestroyStackDepth;
	Heap->DestroyPc = Table->DestroyPc;

	//
	// If there's no outstanding allocation, go to return
	//

	if (Count > 0) {

		j = 0;

		for(i = 0; i < MAX_HEAP_BUCKET; i++) {

			BtrAcquireSpinLock(&Table->ListHead[i].SpinLock);

			ListHead = &Table->ListHead[i].ListHead;

			if (IsListEmpty(ListHead)) {
				BtrReleaseSpinLock(&Table->ListHead[i].SpinLock);
				continue;
			}

			Queue.Flink = ListHead->Flink;
			Queue.Blink = ListHead->Blink;
			ListHead->Flink->Blink = &Queue;
			ListHead->Blink->Flink = &Queue;

			BtrReleaseSpinLock(&Table->ListHead[i].SpinLock);

			while (IsListEmpty(&Queue) != TRUE) {

				ListEntry = RemoveHeadList(&Queue);
				Record = CONTAINING_RECORD(ListEntry, PF_HEAP_RECORD, ListEntry);
				Heap->Records[j] = *Record;

				BtrFreeLookaside(LOOKASIDE_HEAP, Record);
				j += 1;
			}

		}

	}

	*Length = Size;

	Status = WriteFile(FileHandle, Heap, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
	} else {
		Status = S_OK;
	}

	BtrFree(Heap);
	return Status;
}

ULONG
BtrWriteHeapListStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	PPF_STREAM_HEAPLIST HeapList;
	PPF_STREAM_INFO Info;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMM_HEAP_TABLE Heap;
	LARGE_INTEGER Offset;
	ULONG HeapListSize;
	ULONG Length;
	ULONG Count;
	ULONG Complete;
	ULONG i;
	ULONG j;
	ULONG Status;
	LIST_ENTRY Queue;

	//
	// Compute heaplist stream size by number of heaps
	//

	Count = MmHeapListTable.NumberOfHeaps;
	HeapListSize = FIELD_OFFSET(PF_STREAM_HEAPLIST, Heaps[Count]);
	HeapList = (PPF_STREAM_HEAPLIST)BtrMalloc(HeapListSize);

	HeapList->ReservedBytes = MmHeapListTable.ReservedBytes;
	HeapList->CommittedBytes = MmHeapListTable.CommittedBytes;

	HeapList->NumberOfAllocs = MmHeapListTable.NumberOfAllocs;
	HeapList->NumberOfFrees = MmHeapListTable.NumberOfFrees;
	HeapList->SizeOfAllocs = MmHeapListTable.SizeOfAllocs;
	HeapList->SizeOfFrees = MmHeapListTable.SizeOfFrees;

	HeapList->NumberOfHeaps = MmHeapListTable.NumberOfHeaps;
	HeapList->NumberOfActiveHeaps = MmHeapListTable.NumberOfActiveHeaps;
	HeapList->NumberOfRetiredHeaps = HeapList->NumberOfHeaps - HeapList->NumberOfActiveHeaps;

	Info = &HeapList->Heaps[0];

	//
	// Skip the heaplist stream size
	//

	__try {

		Offset.QuadPart = Start.QuadPart + HeapListSize;
		Status = SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}

		j = 0;

		//
		// Serialize all active heap tables
		//

		for(i = 0; i < MAX_HEAPLIST_BUCKET; i++) {

			BtrAcquireSpinLock(&MmHeapListTable.Active[i].SpinLock);

			ListHead = &MmHeapListTable.Active[i].ListHead;

			if (IsListEmpty(ListHead)) {
				BtrReleaseSpinLock(&MmHeapListTable.Active[i].SpinLock);
				continue;
			}

			Queue.Flink = ListHead->Flink;
			Queue.Blink = ListHead->Blink;
			ListHead->Flink->Blink = &Queue;
			ListHead->Blink->Flink = &Queue;

			BtrReleaseSpinLock(&MmHeapListTable.Active[i].SpinLock);

			while (IsListEmpty(&Queue) != TRUE) {

				ListEntry = RemoveHeadList(&Queue);
				Heap = CONTAINING_RECORD(ListEntry, MM_HEAP_TABLE, ListEntry);

				Status = BtrWriteHeapStream(Heap, FileHandle, &Length);
				BtrFree(Heap);

				if (Status != S_OK) {
					__leave;
				}

				Info[j].Offset = Offset.QuadPart;
				Info[j].Length = Length;

				Offset.QuadPart += Length;
				j += 1;

				ASSERT(j <= Count);
			}
		}

		//
		// Serialize all retired heap tables
		//

		BtrAcquireSpinLock(&MmHeapListTable.Retire.SpinLock);

		ListHead = &MmHeapListTable.Active[i].ListHead;
		Queue.Flink = ListHead->Flink;
		Queue.Blink = ListHead->Blink;
		ListHead->Flink->Blink = &Queue;
		ListHead->Blink->Flink = &Queue;

		BtrReleaseSpinLock(&MmHeapListTable.Retire.SpinLock);

		while (IsListEmpty(&Queue) != TRUE) {

			ListEntry = RemoveHeadList(&Queue);
			Heap = CONTAINING_RECORD(ListEntry, MM_HEAP_TABLE, ListEntry);

			Status = BtrWriteHeapStream(Heap, FileHandle, &Length);
			BtrFree(Heap);

			if (Status != S_OK) {
				__leave;
			}

			Info[j].Length = Length;
			Info[j].Offset = Offset.QuadPart;

			Offset.QuadPart += Length;
			j += 1;

			ASSERT(j <= Count);
		}

		//
		// Track the end position
		//

		End->QuadPart = Offset.QuadPart;

		//
		// Adjust file pointer to heap list stream position
		//

		Status = SetFilePointerEx(FileHandle, Start, NULL, FILE_BEGIN);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		} 
		
		Status = S_OK;

		Status = WriteFile(FileHandle, HeapList, HeapListSize, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}
		
		Status = S_OK;

		Status = SetFilePointerEx(FileHandle, *End, NULL, FILE_BEGIN);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		} 

		Status = S_OK;
	}
	__finally {
		BtrFree(HeapList);
	}

	return Status;
}

ULONG
BtrWritePageStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Count;
	ULONG Size;
	PPF_STREAM_PAGE Page;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PPF_PAGE_RECORD Record;
	ULONG Complete;
	ULONG Status;
	ULONG i, j;
	LIST_ENTRY Queue;

	Count = MmPageTable.NumberOfAllocs - MmPageTable.NumberOfFrees;
	ASSERT(Count >= 0);

	Size = FIELD_OFFSET(PF_STREAM_PAGE, Records[Count]);

	Page = BtrMalloc(Size);
	Page->NumberOfRecords = Count;

	Page->ReservedBytes = MmPageTable.ReservedBytes;
	Page->CommittedBytes = MmPageTable.CommittedBytes;
	Page->SizeOfAllocs = MmPageTable.SizeOfAllocs;
	Page->SizeOfFrees = MmPageTable.SizeOfFrees;
	Page->NumberOfAllocs = MmPageTable.NumberOfAllocs;
	Page->NumberOfFrees = MmPageTable.NumberOfFrees;

	if (Count > 0) {

		for(i = 0, j = 0; i < MAX_PAGE_BUCKET; i++) {

			BtrAcquireSpinLock(&MmPageTable.ListHead[i].SpinLock);

			ListHead = &MmPageTable.ListHead[i].ListHead;

			if (IsListEmpty(ListHead)) {
				BtrReleaseSpinLock(&MmPageTable.ListHead[i].SpinLock);
				continue;
			}

			Queue.Flink = ListHead->Flink;
			Queue.Blink = ListHead->Blink;
			ListHead->Flink->Blink = &Queue;
			ListHead->Blink->Flink = &Queue;

			BtrReleaseSpinLock(&MmPageTable.ListHead[i].SpinLock);

			while (IsListEmpty(&Queue) != TRUE) {

				ListEntry = RemoveHeadList(&Queue);
				Record = CONTAINING_RECORD(ListEntry, PF_PAGE_RECORD, ListEntry);
				Page->Records[j] = *Record;

				BtrFreeLookaside(LOOKASIDE_PAGE, Record);
				j += 1;
			}
		}

	}

	Status = WriteFile(FileHandle, Page, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
	} else {
		End->QuadPart = Start.QuadPart + Size;
		Status = S_OK;
	}

	BtrFree(Page);
	return Status;
}

ULONG
BtrWriteHandleStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Count;
	ULONG Size;
	PPF_STREAM_HANDLE Handle;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PPF_HANDLE_RECORD Record;
	ULONG Complete;
	ULONG Status;
	ULONG i, j;
	LIST_ENTRY Queue;

	Count = MmHandleTable.NumberOfAllocs - MmHandleTable.NumberOfFrees;
	ASSERT(Count >= 0);

	Size = FIELD_OFFSET(PF_STREAM_HANDLE, Records[Count]);

	Handle = BtrMalloc(Size);
	Handle->NumberOfRecords = Count;
	Handle->NumberOfAllocs = MmHandleTable.NumberOfAllocs;
	Handle->NumberOfFrees = MmHandleTable.NumberOfFrees;
	GetProcessHandleCount(GetCurrentProcess(), &Handle->HandleCount);

	//
	// Copy type entries
	//

	RtlCopyMemory(Handle->Types, MmHandleTable.Types, sizeof(Handle->Types));
	
	if (Count > 0) {

		for(i = 0, j = 0; i < MAX_HANDLE_BUCKET; i++) {

			BtrAcquireSpinLock(&MmHandleTable.ListHead[i].SpinLock);

			ListHead = &MmHandleTable.ListHead[i].ListHead;

			if (IsListEmpty(ListHead)) {
				BtrReleaseSpinLock(&MmHandleTable.ListHead[i].SpinLock);
				continue;
			}

			Queue.Flink = ListHead->Flink;
			Queue.Blink = ListHead->Blink;
			ListHead->Flink->Blink = &Queue;
			ListHead->Blink->Flink = &Queue;

			BtrReleaseSpinLock(&MmHandleTable.ListHead[i].SpinLock);

			while (IsListEmpty(&Queue) != TRUE) {

				ListEntry = RemoveHeadList(&Queue);
				Record = CONTAINING_RECORD(ListEntry, PF_HANDLE_RECORD, ListEntry);
				Handle->Records[j] = *Record;

				BtrFreeLookaside(LOOKASIDE_HANDLE, Record);
				j += 1;
			}
		}

	}

	Status = WriteFile(FileHandle, Handle, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
	} else {
		End->QuadPart = Start.QuadPart + Size;
		Status = S_OK;
	}

	BtrFree(Handle);
	return Status;
}

ULONG
BtrWriteGdiStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Count;
	ULONG Size;
	PPF_STREAM_GDI Gdi;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PPF_GDI_RECORD Record;
	ULONG Complete;
	ULONG Status;
	ULONG i, j;
	LIST_ENTRY Queue;

	Count = MmGdiTable.NumberOfAllocs - MmGdiTable.NumberOfFrees;
	ASSERT(Count >= 0);

	Size = FIELD_OFFSET(PF_STREAM_GDI, Records[Count]);

	Gdi = BtrMalloc(Size);
	Gdi->NumberOfRecords = Count;
	Gdi->NumberOfAllocs = MmGdiTable.NumberOfAllocs;
	Gdi->NumberOfFrees = MmGdiTable.NumberOfFrees;

	//
	// Copy type entries
	//

	RtlCopyMemory(Gdi->Types, MmGdiTable.Types, sizeof(Gdi->Types));
	
	if (Count > 0) {

		for(i = 0, j = 0; i < MAX_GDI_BUCKET; i++) {

			BtrAcquireSpinLock(&MmGdiTable.ListHead[i].SpinLock);

			ListHead = &MmGdiTable.ListHead[i].ListHead;

			if (IsListEmpty(ListHead)) {
				BtrReleaseSpinLock(&MmGdiTable.ListHead[i].SpinLock);
				continue;
			}

			Queue.Flink = ListHead->Flink;
			Queue.Blink = ListHead->Blink;
			ListHead->Flink->Blink = &Queue;
			ListHead->Blink->Flink = &Queue;

			BtrReleaseSpinLock(&MmGdiTable.ListHead[i].SpinLock);

			while (IsListEmpty(&Queue) != TRUE) {

				ListEntry = RemoveHeadList(&Queue);
				Record = CONTAINING_RECORD(ListEntry, PF_GDI_RECORD, ListEntry);
				Gdi->Records[j] = *Record;

				BtrFreeLookaside(LOOKASIDE_GDI, Record);
				j += 1;
			}
		}

	}

	Status = WriteFile(FileHandle, Gdi, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
	} else {
		End->QuadPart = Start.QuadPart + Size;
		Status = S_OK;
	}

	BtrFree(Gdi);
	return Status;
}

ULONG
BtrWriteDllStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Required;
	ULONG Status;
	ULONG Count;
	ULONG i;
	HMODULE *Modules;
	PPF_STREAM_DLL Dlls;
	PBTR_DLL_ENTRY Dll;
	ULONG Size;
	ULONG Complete;
	HANDLE ProcessHandle;
	MODULEINFO Info;
	ULONG ProcessId;

	Modules = BtrMalloc(sizeof(HMODULE) * 1024);
	if (!Modules) {
		return BTR_E_OUTOFMEMORY;
	}
	
	ProcessId = GetCurrentProcessId();
	ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}

	Status = EnumProcessModules(ProcessHandle, Modules, 
		                        sizeof(HMODULE) * 1024, &Required);
	if (!Status) {
		Status = GetLastError();
		BtrFree(Modules);
		CloseHandle(ProcessHandle);
		return Status;
	}

	Count = Required / sizeof(HMODULE);
	Size = FIELD_OFFSET(PF_STREAM_DLL, Dll[Count]);

	Dlls = (PPF_STREAM_DLL)BtrMalloc(Size);
	if (!Dlls) {
		BtrFree(Modules);
		CloseHandle(ProcessHandle);
		return BTR_E_OUTOFMEMORY;
	}

	Dlls->Count = Count;

	for(i = 0; i < Count; i++) {

		Dll = &Dlls->Dll[i];
		Dll->DllId = i;
		Dll->Count = 0;

		//
		// Dll base address and its size
		//

		GetModuleInformation(ProcessHandle, Modules[i], &Info, sizeof(Info));
		Dll->BaseVa = (ULONG_PTR)Info.lpBaseOfDll;
		Dll->Size = Info.SizeOfImage;

		//
		// Dll path
		//

		GetModuleFileName(Modules[i], Dll->Path, MAX_PATH);
		BtrGetImageInformation(Modules[i], &Dll->Timestamp, &Dll->CheckSum);

		//
		// Copy code review record
		//

		BtrCreateCvRecord((ULONG_PTR)Modules[i], (ULONG)Dll->Size, 
			               Dll->Timestamp, &Dll->CvRecord);
	}

	Status = WriteFile(FileHandle, Dlls, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
	} else {
		End->QuadPart = Start.QuadPart + Size;
		Status = S_OK;
	}

	BtrFree(Modules);
	BtrFree(Dlls);
	CloseHandle(ProcessHandle);
	return Status;
}

#define PtrFromRva(_B, _R) (((PUCHAR)_B) + _R)
#define CV_SIGNATURE  'SDSR'

ULONG
BtrCopyCodeViewRecord(
	__in PVOID Base,
	__out PCV_INFO_PDB70 CvRecord,
	__out PULONG NameLength
	)
{
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_DATA_DIRECTORY Directory;
	PIMAGE_DEBUG_DIRECTORY Debug;
	ULONG Count;
	PCV_INFO_PDB70 Info;
	ULONG i;

	NtHeader = ImageNtHeader(Base);
	Directory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
	Debug = (PIMAGE_DEBUG_DIRECTORY)PtrFromRva(Base, Directory->VirtualAddress);
	
	Count = Directory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

	for (i = 0; i < Count; i += 1) {
		if (Debug[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
			Info = (PCV_INFO_PDB70)PtrFromRva(Base, Debug[i].AddressOfRawData);
			CvRecord->CvSignature = Info->CvSignature;
			CvRecord->Signature = Info->Signature;
			CvRecord->Age = Info->Age;
			StringCchCopyA(CvRecord->PdbName, MAX_PATH, Info->PdbName);
			*NameLength = (ULONG)strlen(Info->PdbName) + 1;
			return S_OK;
		}
	}

	return S_FALSE;
}

ULONG
BtrCreateCvRecord(
	__in ULONG_PTR Base,
	__in ULONG Size,
	__in ULONG Timestamp,
	__out PBTR_CV_RECORD CvRecord
	)
{
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_DATA_DIRECTORY Directory;
	PIMAGE_DEBUG_DIRECTORY Debug;
	ULONG Count;
	PCV_INFO_PDB70 Info;
	CHAR Name[64];
	CHAR Pdb[16];
	ULONG Length;
	ULONG i;

	NtHeader = ImageNtHeader((PVOID)Base);
	Directory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
	Debug = (PIMAGE_DEBUG_DIRECTORY)PtrFromRva(Base, Directory->VirtualAddress);
	
	Count = Directory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

	for (i = 0; i < Count; i += 1) {

		if (Debug[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW) {

			Info = (PCV_INFO_PDB70)PtrFromRva(Base, Debug[i].AddressOfRawData);

			CvRecord->CvSignature = Info->CvSignature;
			CvRecord->Signature = Info->Signature;
			CvRecord->Age = Info->Age;

			//
			// N.B. Some pdb string include a full path, we only reserved its
			// short name, it's ok without problem.
			//

			_splitpath_s(Info->PdbName, NULL, 0, NULL, 0, Name, 64, Pdb, 16);
			StringCchPrintfA(CvRecord->PdbName, 64, "%s%s", Name, Pdb);
			
			//
			// Compute CV_INFO_PDB70 offset
			//
			
			CvRecord->CvRecordOffset = FIELD_OFFSET(BTR_CV_RECORD, CvSignature);

			//
			// Compute size of CV_INFO_PDB70
			//

			Length = (ULONG)strlen(CvRecord->PdbName) + 1; 
			Length = FIELD_OFFSET(BTR_CV_RECORD, PdbName[Length]);
			CvRecord->SizeOfCvRecord = Length - CvRecord->CvRecordOffset;

			CvRecord->Reserved0 = 0;
			CvRecord->Reserved1 = 0;
			CvRecord->Timestamp = Timestamp;
			CvRecord->SizeOfImage = Size;

			return S_OK;
		}
	}

	return S_FALSE;
}

ULONG
BtrWriteMinidumpStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	WCHAR Path[MAX_PATH];
	ULONG Length;
	HANDLE DumpHandle;
	HANDLE MappingHandle;
	PVOID MappedVa;
	ULONG Status;
	ULONG Complete;
	ULONG Size;

	Length = GetTempPath(MAX_PATH, Path);
	if (!Length) {
		return GetLastError();
	}

	StringCchCat(Path, MAX_PATH, L"dpf.dmp");
	Status = BtrWriteMinidump(Path, FALSE, &DumpHandle);
	if (Status != S_OK) {
		return Status;
	}

	ASSERT(DumpHandle != NULL);
	Size = GetFileSize(DumpHandle, NULL);

	__try {

		MappingHandle = CreateFileMapping(DumpHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (!MappingHandle) {
			Status = GetLastError();
			__leave;
		}

		MappedVa = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		if (!MappedVa) {
			Status = GetLastError();
			__leave;
		}

		Status = WriteFile(FileHandle, MappedVa, Size, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		} 

		End->QuadPart = Start.QuadPart + Size;
		Status = S_OK;

	}
	__finally {

		if (MappedVa) {
			UnmapViewOfFile(MappedVa);
		}

		if (MappingHandle) {
			CloseHandle(MappingHandle);
		}

		CloseHandle(DumpHandle);
		DeleteFile(Path);
	}

	return Status;
}

ULONG
BtrWriteStackStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{ 
	ULONG Count;
	PBTR_STACK_ENTRY Record;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Hash;
	ULONG i, j;
	ULONG Complete;
	ULONG Status;

	Count = BtrStackTable.Count;

	__try {

		for(i = 0, j = 0; i < STACK_RECORD_BUCKET; i++) {

			Hash = &BtrStackTable.Hash[i];

			BtrAcquireSpinLock(&Hash->SpinLock);

			ListHead = &Hash->ListHead;
			while (IsListEmpty(ListHead) != TRUE) {
				ListEntry = RemoveHeadList(ListHead);

				Record = CONTAINING_RECORD(ListEntry, BTR_STACK_ENTRY, ListEntry);
				Status = WriteFile(FileHandle, Record, sizeof(BTR_STACK_ENTRY), &Complete, NULL);

				j += 1;

				if (!Status) {
					Status = GetLastError();
					BtrReleaseSpinLock(&Hash->SpinLock);
					__leave;
				}
			}
			
			BtrReleaseSpinLock(&Hash->SpinLock);
		}

		ASSERT(j == Count);

		End->QuadPart = Start.QuadPart + j * sizeof(BTR_STACK_ENTRY);
		Status = S_OK;

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = BTR_E_EXCEPTION;
	}

	return Status;
}

ULONG
BtrWriteStackFileStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	HANDLE MappingHandle;
	PVOID MappedVa;
	ULONG Status;
	ULONG Complete;
	ULONG ValidLength;
	ULONG Count;
	ULONG Round;
	ULONG Remain;
	ULONG Unit;
	ULONG i;

	Count = BtrStackTable.Count;
	Status = WriteFile(FileHandle, &Count, sizeof(ULONG), &Complete, NULL);
	if (!Status) {
		return GetLastError();
	}

	MappingHandle = CreateFileMapping(BtrProfileObject->StackFileObject, 
		                              NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		return GetLastError();
	}

	Unit = 1024 * 1024;
	ValidLength = Count * sizeof(BTR_STACK_RECORD);

	Round = ValidLength / Unit;
	Remain = ValidLength % Unit;

	__try {

		for(i = 0; i < Round; i++) {

			MappedVa = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 
				                     0, i * Unit, Unit);
			if (!MappedVa) {
				Status = GetLastError();
				__leave;
			}

			Status = WriteFile(FileHandle, MappedVa, Unit, &Complete, NULL);
			if (Status != TRUE) {
				Status = GetLastError();
				__leave;
			} 

			UnmapViewOfFile(MappedVa);
		}

		MappedVa = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 
			                     0, Round * Unit, Remain);
		if (!MappedVa) {
			Status = GetLastError();
			__leave;
		}

		Status = WriteFile(FileHandle, MappedVa, Unit, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}

		End->QuadPart = Start.QuadPart + ValidLength;
		Status = S_OK;

	}
	__finally {

		if (MappedVa) {
			UnmapViewOfFile(MappedVa);
		}

		if (MappingHandle) {
			CloseHandle(MappingHandle);
		}
	}

	return Status;
}

VOID
BtrComputeStreamCheckSum(
	IN PPF_REPORT_HEAD Head
	)
{
	ULONG i;
	ULONG CheckSum;
	PUCHAR Buffer;

	CheckSum = 0;
	Buffer = (PUCHAR)Head;

	for(i = 0; i < sizeof(*Head); i++) {
		CheckSum += (ULONG)Buffer[i];		
	}	

	Head->CheckSum = CheckSum;
}

ULONG
BtrWriteProfileStream(
	__in HANDLE Handle
	)
{
	PPF_REPORT_HEAD Head;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	ULONG Complete;
	ULONG Status;
	PPF_STREAM_SYSTEM System;

	Head = (PPF_REPORT_HEAD)BtrMalloc(sizeof(PF_REPORT_HEAD));
	RtlZeroMemory(Head, sizeof(*Head));

	//
	// Skip system stream which is filled by control end
	//

	Head->Streams[STREAM_SYSTEM].Offset = sizeof(PF_REPORT_HEAD);
	Head->Streams[STREAM_SYSTEM].Length = sizeof(PF_STREAM_SYSTEM);

	Start.QuadPart = sizeof(PF_REPORT_HEAD);
	SetFilePointerEx(Handle, Start, NULL, FILE_BEGIN);
	System = (PPF_STREAM_SYSTEM)BtrMalloc(sizeof(PF_STREAM_SYSTEM));
	RtlZeroMemory(System, sizeof(*System));

	//
	// N.B. Profile attribute is written by runtime end
	//

	System->Attr = BtrProfileObject->Attribute;
	WriteFile(Handle, System, sizeof(*System), &Complete, NULL);
	BtrFree(System);

	Start.QuadPart += sizeof(*System);

	if (BtrProfileObject->Attribute.Type == PROFILE_CPU_TYPE) {

		//
		// Write CPU thread stream
		//

		Status = BtrWriteCpuThreadStream(Head, Handle, Start, &End);
		if (Status != S_OK) {
			return Status;
		}
		
		Head->Streams[STREAM_CPU_THREAD].Offset = Start.QuadPart;
		Head->Streams[STREAM_CPU_THREAD].Length = End.QuadPart - Start.QuadPart;
		Start.QuadPart = End.QuadPart;

		//
		// Write CPU address stream
		//

		Status = BtrWriteCpuAddressStream(Head, Handle, Start, &End);
		if (Status != S_OK) {
			return Status;
		}
		
		Head->Streams[STREAM_CPU_ADDRESS].Offset = Start.QuadPart;
		Head->Streams[STREAM_CPU_ADDRESS].Length = End.QuadPart - Start.QuadPart;
		Start.QuadPart = End.QuadPart;
	}

	if (BtrProfileObject->Attribute.Type == PROFILE_MM_TYPE) {

		//
		// Write heap stream
		//

		if (BtrProfileObject->Attribute.EnableHeap) {

			Status = BtrWriteHeapListStream(Head, Handle, Start, &End);
			if (Status != S_OK) {
				return Status;
			}

			Head->Streams[STREAM_MM_HEAP].Offset = Start.QuadPart;
			Head->Streams[STREAM_MM_HEAP].Length = End.QuadPart - Start.QuadPart;

			Start.QuadPart = End.QuadPart;
		}

		//
		// Write page stream
		//

		if (BtrProfileObject->Attribute.EnablePage) {

			Status = BtrWritePageStream(Head, Handle, Start, &End);
			if (Status != S_OK) {
				return Status;
			}

			Head->Streams[STREAM_MM_PAGE].Offset = Start.QuadPart;
			Head->Streams[STREAM_MM_PAGE].Length = End.QuadPart - Start.QuadPart;

			Start.QuadPart = End.QuadPart;
		}

		//
		// Write handle stream
		//

		if (BtrProfileObject->Attribute.EnableHandle) {

			Status = BtrWriteHandleStream(Head, Handle, Start, &End);
			if (Status != S_OK) {
				return Status;
			}

			Head->Streams[STREAM_MM_HANDLE].Offset = Start.QuadPart;
			Head->Streams[STREAM_MM_HANDLE].Length = End.QuadPart - Start.QuadPart;

			Start.QuadPart = End.QuadPart;
		}

		//
		// Write GDI stream
		//

		if (BtrProfileObject->Attribute.EnableGdi) {

			Status = BtrWriteGdiStream(Head, Handle, Start, &End);
			if (Status != S_OK) {
				return Status;
			}

			Head->Streams[STREAM_MM_GDI].Offset = Start.QuadPart;
			Head->Streams[STREAM_MM_GDI].Length = End.QuadPart - Start.QuadPart;

			Start.QuadPart = End.QuadPart;
		}

	}

	//
	// Write mark stream
	//

	if (BtrMarkCount > 0) {
		
		Status = BtrWriteMarkStream(Head, Handle, Start, &End);
		if (Status != S_OK) {
			return Status;
		}

		Head->Streams[STREAM_MARK].Offset = Start.QuadPart;
		Head->Streams[STREAM_MARK].Length = End.QuadPart - Start.QuadPart;
		Start.QuadPart = End.QuadPart;
	}

	//
	// N.B. Because stack stream will be rewritten by control end,
	// we need always write it as the last step, otherwise, other
	// streams can be corrupted!
	//

	//
	// Write stack hash stream, which will be replaced by stack record stream
	// after fill counters into each record.
	//

	Status = BtrWriteStackStream(Head, Handle, Start, &End);
	if (Status != S_OK) {
		return Status;
	}

	Head->Streams[STREAM_STACK].Offset = Start.QuadPart;
	Head->Streams[STREAM_STACK].Length = End.QuadPart - Start.QuadPart;
	Start.QuadPart = End.QuadPart;

	//
	// Write report header
	//

	Head->Signature = DPF_SIGNATURE;
	Head->MajorVersion = 1;
	Head->MinorVersion = 0;
	Head->FileSize = End.QuadPart;

	if (BtrProfileObject->Attribute.Type == PROFILE_CPU_TYPE) {
		Head->IncludeCpu = 1;
	}

	else if (BtrProfileObject->Attribute.Type == PROFILE_MM_TYPE) {
		Head->IncludeMm = 1;
	}

	else if (BtrProfileObject->Attribute.Type == PROFILE_IO_TYPE) {
		Head->IncludeIo = 1;
	}

	else if (BtrProfileObject->Attribute.Type == PROFILE_CCR_TYPE) {
		Head->IncludeCcr = 1;
	}

	else {
		ASSERT(0);
	}

	Head->CpuType = HalGetCpuType();
	Head->TargetType = HalGetTargetType();

	Head->Reserved = 0;
	Head->CheckSum = 0;

	//
	// Compute head checksum of stream file
	//

	BtrComputeStreamCheckSum(Head);

	//
	// Write head finally
	//

	Start.QuadPart = 0;
	SetFilePointerEx(Handle, Start, NULL, FILE_BEGIN);

	Status = S_OK;
	Status = WriteFile(Handle, Head, sizeof(*Head), &Complete, NULL);
	if (!Status) {
		Status = GetLastError();
	}
	
	BtrFree(Head);

	SetFilePointerEx(Handle, End, NULL, FILE_BEGIN);
	SetEndOfFile(Handle);
	return Status;
}

ULONG
BtrWriteDllStreamEx(
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

	Length = GetTempPath(MAX_PATH, Path);
	if (!Length) {
		return GetLastError();
	}

	GetLocalTime(&Time);
	StringCchPrintf(Path, MAX_PATH, L"%2d%2d%2d-dpf.dmp", 
					Time.wHour, Time.wMinute, Time.wSecond);

	Status = BtrWriteMinidump(Path, FALSE, &DumpHandle);
	if (Status != S_OK) {
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
		DllFile = (PBTR_DLL_FILE)BtrMalloc(DllFileSize);

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
		Status = S_OK;

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
			BtrFree(DllFile);
		}
	}

	return Status;
}

//
// Write CPU thread list into STREAM_CPU_THREAD
//

ULONG
BtrWriteCpuThreadStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	ULONG Count;
	PBTR_CPU_THREAD Thread;
	PBTR_CPU_THREAD Threads;
	PLIST_ENTRY ListEntry;
	ULONG Complete;
	ULONG Size;
	ULONG Status;
    ULONG Dropped;

	Size = sizeof(BTR_CPU_THREAD) * (CpuActiveCount + CpuRetireCount);
	Threads = (PBTR_CPU_THREAD)BtrMalloc(Size);

	__try {

        //
        // N.B. If thread's sample count is zero, it's dropped
        //

		Count = 0;
        Dropped = 0;

		while (IsListEmpty(&CpuActiveList) != TRUE) {

			ListEntry = RemoveHeadList(&CpuActiveList);
			Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, ListEntry);

            if (Thread->ThreadHandle) {
                CloseHandle(Thread->ThreadHandle);
                Thread->ThreadHandle = NULL;
            }

            if (Thread->NumberOfSamples != 0) {
                Threads[Count] = *Thread;
                Count += 1;
            }
            else {
                Dropped += 1;
            }
		}
		
		while (IsListEmpty(&CpuRetireList) != TRUE) {

			ListEntry = RemoveHeadList(&CpuRetireList);
			Thread = CONTAINING_RECORD(ListEntry, BTR_CPU_THREAD, ListEntry);
            
            if (Thread->ThreadHandle) {
                CloseHandle(Thread->ThreadHandle);
                Thread->ThreadHandle = NULL;
            }

            if (Thread->NumberOfSamples != 0) {
                Threads[Count] = *Thread;
                Count += 1;
            }
            else {
                Dropped += 1;
            }
		}

		//
		// Sanity debugging
		//

		ASSERT((Count + Dropped) == (CpuActiveCount + CpuRetireCount));
		
		//
		// Adjust the block size according to dropped count
        //

        Size -= sizeof(BTR_CPU_THREAD) * Dropped;
		Status = WriteFile(FileHandle, Threads, Size, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		} 

		Head->Streams[STREAM_CPU_THREAD].Offset = Start.QuadPart;
		Head->Streams[STREAM_CPU_THREAD].Length = Size;
		End->QuadPart = Start.QuadPart + Size;

		Status = S_OK;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = BTR_E_EXCEPTION;
	}

	return Status;
}

ULONG
BtrWriteCpuAddressStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	)
{
	PBTR_CPU_ADDRESS Address;
	ULONG Complete;
	ULONG Size;
	ULONG Status;

	__try {

		Address = &BtrCpuAddressTable[0];
		Size = sizeof(BtrCpuAddressTable);

		//
		// Write stream into report file
		//

		Status = WriteFile(FileHandle, Address, Size, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		} 

		Head->Streams[STREAM_CPU_ADDRESS].Offset = Start.QuadPart;
		Head->Streams[STREAM_CPU_ADDRESS].Length = Size;
		End->QuadPart = Start.QuadPart + Size;

		Status = S_OK;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = BTR_E_EXCEPTION;
	}

	return Status;
}