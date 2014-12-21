//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "bsp.h"
#include <stdio.h>
#include <psapi.h>
#include <strsafe.h>
#include <time.h>
#include "ntapi.h"
#include "dprofiler.h"
#include "pcdb.h"
#include <dbghelp.h>

ULONG
PcCreateDatabase(
	IN PPC_DATABASE *Database,
	IN ULONG Buckets
	)
{
	ULONG Status;
	ULONG Size;
	HANDLE Handle;
	ULONG Number;
	PPC_DATABASE Object;

	Size = FIELD_OFFSET(PC_DATABASE, ListHead[Buckets]);
	Size = BspRoundUp(Size, BspPageSize);

	Handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, NULL);
	if (!Handle) {
		Status = GetLastError();
		return Status;
	}

	Object = (PPC_DATABASE)MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Object) {
		Status = GetLastError();
		CloseHandle(Handle);
		return Status;
	}
	
	Object->Object = Handle;
	Object->BaseVa = Object;
	Object->Size = Size;
	Object->Buckets = Buckets;
	Object->PcCount = 0;

	for(Number = 0; Number < Buckets; Number += 1) {
		InitializeListHead(&Object->ListHead[Number]);
	}

	*Database = Object;
	return 0;
}

PPC_ENTRY
PcLookupDatabase(
	IN PPC_DATABASE Database,
	IN ULONG_PTR Pc
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PPC_ENTRY Entry;

	Bucket = PC_HASH_BUCKET(Database,Pc);

	ListHead = &Database->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, PC_ENTRY, ListEntry);
		if (Entry->Pc == Pc) {
			return Entry;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PPC_ENTRY
PcInsertDatabase(
	IN PPC_DATABASE Database,
	IN ULONG_PTR Pc
	)
{
	ULONG Bucket;
	PPC_ENTRY Entry;

	Entry = PcLookupDatabase(Database, Pc);
	if (Entry != NULL) {
		return Entry;
	}

	Entry = (PPC_ENTRY)BspMalloc(sizeof(PC_ENTRY));
	Entry->Name = NULL;
	Entry->Module = 0;
	Entry->Pc = Pc;

	Bucket = PC_HASH_BUCKET(Database, Pc);
	InsertTailList(&Database->ListHead[Bucket], &Entry->ListEntry);

	Database->PcCount += 1;
	return Entry;
}

VOID
PcDecodePc(
	IN HANDLE Handle,
	IN PPC_DATABASE Database,
	IN PPC_ENTRY Entry
	)
{
	ULONG Status;
	SIZE_T Length;
	ULONG64 Displacement;
	ULONG64 ModuleBase;
	PIMAGEHLP_SYMBOL64 Symbol;
	CHAR ModuleName[64];
	CHAR FullName[4096];

	Length = FIELD_OFFSET(IMAGEHLP_SYMBOL64, Name[128]);
	Symbol = (PIMAGEHLP_SYMBOL64)_alloca(Length);
	Symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	Symbol->MaxNameLength = 128;

	Status = SymGetSymFromAddr64(Handle, 
								 Entry->Pc, 
		                         &Displacement, 
		                         Symbol);

	if (Status != TRUE) {
		Symbol->Name[0] = '\0';
		ModuleBase = SymGetModuleBase64(Handle, Entry->Pc);
		Displacement = Entry->Pc - ModuleBase;
	} 

	Status = PcDecodeDll(Handle, Entry->Pc, ModuleName, 64);

	if (Status != PF_STATUS_OK) {

		StringCchPrintfA(FullName, 4096, "0x%x", Entry->Pc);
		StringCchLengthA(FullName, 4096, &Length);
		Entry->Name = (PCHAR)BspMalloc((ULONG)Length + 1);
		strcpy(Entry->Name, FullName);
		return;

	} 

	StringCchPrintfA(FullName, 4096, "%s!%s+0x%x", 
					 ModuleName, Symbol->Name, Displacement);

	StringCchLengthA(FullName, 4096, &Length);
	Entry->Name = (PCHAR)BspMalloc((ULONG)Length + 1);
	strcpy(Entry->Name, FullName);
	return;
}

ULONG
PcDecodeDll(
	IN HANDLE Handle,
	IN ULONG64 Pc,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	)
{
	PIMAGEHLP_MODULE64 Module;
	ULONG64 Buffer[sizeof(IMAGEHLP_MODULE64) / sizeof(ULONG64) + 1];
	ULONG Status;

	Module = (PIMAGEHLP_MODULE64)Buffer;
	Module->SizeOfStruct = BspRoundUp(sizeof(IMAGEHLP_MODULE64), 8);

	Status = SymGetModuleInfo64(Handle, Pc, Module);

	if (Status != TRUE) {
		//strncpy(NameBuffer, "<Unknown>", BufferLength);
		return PF_STATUS_ERROR;
	}
	else {
		strncpy(NameBuffer, Module->ModuleName, BufferLength);
	}

	strlwr(NameBuffer);
	return PF_STATUS_OK;
}