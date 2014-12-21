//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "bsp.h"
#include "dlldb.h"
#include "list.h"
#include <psapi.h>
#include "dprofiler.h"

#if defined (_M_IX86)
#define GOLDEN_RATIO 0x9E3779B9

#elif defined (_M_X64)
#define GOLDEN_RATIO 0x9E3779B97F4A7C13
#endif

ULONG
DllCreateDatabase(
	IN ULONG ProcessId,
	IN PDLL_DATABASE *Database
	)
{
	PDLL_DATABASE Dll;
	LIST_ENTRY ListHead;
	PDLL_ENTRY Entry;
	PLIST_ENTRY ListEntry;
	PBSP_MODULE Module;
	ULONG Number;
	ULONG Bucket;
	ULONG Status;
	HANDLE Handle;

	Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!Handle) {
		Status = GetLastError();
		return Status;
	}

	InitializeListHead(&ListHead);
	Status = BspQueryModule(ProcessId, FALSE, &ListHead);
	if (Status != ERROR_SUCCESS) {
		CloseHandle(Handle);
		return Status;
	}

	Dll = (PDLL_DATABASE)BspMalloc(sizeof(DLL_DATABASE));
	Dll->ProcessId = ProcessId;
	Dll->ProcessHandle = Handle;
	
	for(Number = 0; Number < MAX_DLL_BUCKET; Number += 1) {
		InitializeListHead(&Dll->ListHead[Number]);
	}

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);
		
		//
		// Get base, size, path and linker time of dll entry
		//

		Entry = (PDLL_ENTRY)BspMalloc(sizeof(DLL_ENTRY));
		ZeroMemory(Entry, sizeof(DLL_ENTRY));

		Entry->BaseVa = Module->BaseVa;
		Entry->Size = Module->Size;

		//
		// Convert string to be lower case
		//

		wcslwr(Module->FullPath);
		wcscpy_s(Entry->Path, MAX_PATH, Module->FullPath);

		BspGetModuleTimeStamp(Module, &Entry->LinkerTime);

		Entry->Flag = DLL_LOADED;
		QueryPerformanceCounter(&Entry->LoadTime);

		//
		// Hash the dll base address and insert into its slot
		//

		Bucket = DllComputeAddressHash(Entry->BaseVa, MAX_DLL_BUCKET);
		InsertTailList(&Dll->ListHead[Bucket], &Entry->ListEntry);

		BspFreeModule(Module);
	}

	*Database = Dll;
	return PF_STATUS_OK;
}

ULONG
DllComputeAddressHash(
	IN PVOID Address,
	IN ULONG Limit
	)
{
	ULONG Hash;
	ULONG_PTR Value;

	//
	// Mask off its lower 16 bits, because dll are always mapped
	// with 64KB aligned
	//

	Value = (ULONG_PTR)Address >> 16;

	Hash = (ULONG)((ULONG_PTR)Value ^ (ULONG_PTR)GOLDEN_RATIO);
	Hash = (Hash >> 24) ^ (Hash >> 16) ^ (Hash >> 8) ^ (Hash);

	return Hash % Limit;
}

PDLL_ENTRY
DllLookupDatabase(
	IN PDLL_DATABASE Database,
	IN PVOID BaseVa
	)
{
	PDLL_ENTRY Entry;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	ULONG Bucket;

	Bucket = DllComputeAddressHash(BaseVa, MAX_DLL_BUCKET);
	ListHead = &Database->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, DLL_ENTRY, ListEntry);
		if (Entry->BaseVa == BaseVa) {
			return Entry;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PDLL_ENTRY
DllInsertDatabase(
	IN PDLL_DATABASE Database,
	IN PVOID Pc 
	)
{
	PDLL_ENTRY Entry;
	ULONG Bucket;
	ULONG Status;
	MODULEINFO Info;
	PVOID BaseVa;
	
	//
	// N.B. We expect that the specified Pc is not available in 
	// Pc database, the profiler work loop should always first 
	// lookup pc database to check whether it's tracked one.
	//

	BaseVa = (PVOID)SymGetModuleBase64(Database->ProcessHandle, (DWORD64)Pc);

	Entry = DllLookupDatabase(Database, BaseVa);
	if (Entry != NULL) {
		return Entry;
	}

	Status = GetModuleInformation(Database->ProcessHandle, BaseVa, 
		                          &Info, sizeof(MODULEINFO));
	if (!Status) {
		return NULL;
	}

	Entry = (PDLL_ENTRY)BspMalloc(sizeof(DLL_ENTRY));
	ZeroMemory(Entry, sizeof(DLL_ENTRY));
	Entry->BaseVa = Info.lpBaseOfDll;
	Entry->Size = Info.SizeOfImage;
	QueryPerformanceCounter(&Entry->LoadTime);

	GetModuleFileNameEx(Database->ProcessHandle, Info.lpBaseOfDll, 
		                Entry->Path, MAX_PATH);

	Bucket = DllComputeAddressHash(Info.lpBaseOfDll, MAX_DLL_BUCKET);
	InsertTailList(&Database->ListHead[Bucket], &Entry->ListEntry);

	return Entry;
}
	