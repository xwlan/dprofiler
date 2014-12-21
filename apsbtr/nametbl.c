//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "nametbl.h"
#include "util.h"
#include "heap.h"
#include "hal.h"
#include "thread.h"

POBJECT_NAME_TABLE BtrObjectNameTable;
NTQUERYOBJECT NtQueryObject;
NTQUERYKEY NtQueryKey;

ULONG
BtrCreateObjectNameTable(
	IN ULONG FixedBuckets
	)
{
	HANDLE Handle;
	ULONG Size;
	PVOID Address;
	ULONG Status;
	ULONG Number;

	Handle = GetModuleHandleA("ntdll.dll");
	NtQueryObject = (NTQUERYOBJECT)GetProcAddress(Handle, "NtQueryObject");
	if (!NtQueryObject) {
		return BTR_E_NO_API;
	}

	NtQueryKey = (NTQUERYKEY)GetProcAddress(Handle, "NtQueryKey");
	if (!NtQueryKey) {
		return BTR_E_NO_API;
	}

	Size = FIELD_OFFSET(OBJECT_NAME_TABLE, Entry[FixedBuckets]);
	Size = BtrUlongRoundUp(Size, (ULONG)BtrPageSize);

	Handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, NULL);
	if (!Handle) {
		return GetLastError();
	}

	Address = MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Address) {
		Status = GetLastError();
		CloseHandle(Handle);
		return Status;
	}

	BtrObjectNameTable = (POBJECT_NAME_TABLE)Address;
	BtrObjectNameTable->Object = Handle;
	BtrObjectNameTable->BaseVa = Address;
	BtrObjectNameTable->Size = Size;
	BtrObjectNameTable->NumberOfBuckets = FixedBuckets;
	BtrObjectNameTable->NumberOfFixedBuckets = FixedBuckets;

	for(Number = 0; Number < FixedBuckets; Number += 1) {
		BtrInitSpinLock(&BtrObjectNameTable->Entry[Number].SpinLock, 100);
		InitializeListHead(&BtrObjectNameTable->Entry[Number].ListHead);
	}

	return S_OK;
}

VOID
BtrDestroyNameTable(
	VOID
	)
{
	HANDLE Object;

	Object = BtrObjectNameTable->Object;
	UnmapViewOfFile(BtrObjectNameTable);

	CloseHandle(Object);
	BtrObjectNameTable = NULL;
}

ULONG FORCEINLINE
BtrHashStringBKDR(
	IN PWSTR Buffer
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

ULONG FORCEINLINE 
BtrComputeNameBucket(
	IN ULONG Hash
	)
{
	return Hash % BtrObjectNameTable->NumberOfFixedBuckets;
}

ULONG
BtrInsertObjectName(
	IN HANDLE Object,
	IN HANDLE_TYPE Type,
	IN PWSTR Name,
	IN ULONG Length
	)
{
	ULONG Bucket;
	ULONG Hash;
	PHASH_ENTRY HashEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	POBJECT_NAME_ENTRY NameEntry;

	Hash = BtrHashStringBKDR(Name);
	Bucket = BtrComputeNameBucket(Hash);
	HashEntry = &BtrObjectNameTable->Entry[Bucket];
	ListHead = &HashEntry->ListHead;

	BtrAcquireSpinLock(&HashEntry->SpinLock);

	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		NameEntry = CONTAINING_RECORD(ListEntry, OBJECT_NAME_ENTRY, ListEntry);
		if (NameEntry->Hash == Hash && NameEntry->Length == Length &&
			!wcsicmp(Name, NameEntry->Name)) {
			BtrReleaseSpinLock(&HashEntry->SpinLock);
			return Hash;
		}
		ListEntry = ListEntry->Flink;
	}

	NameEntry = (POBJECT_NAME_ENTRY)BtrMalloc(sizeof(OBJECT_NAME_ENTRY));
	NameEntry->Object = Object;
	NameEntry->Type = Type;
	NameEntry->Hash = Hash;
	NameEntry->Length = Length;
	wcscpy_s(NameEntry->Name, MAX_PATH, Name);

	InsertHeadList(ListHead, &NameEntry->ListEntry);

	BtrReleaseSpinLock(&HashEntry->SpinLock);

	return Hash;
}

ULONG
BtrQueryObjectName(
	IN HANDLE Object,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	NTSTATUS Status;
	ULONG ReturnLength;
	POBJECT_NAME_INFORMATION Info;

	if (!NtQueryObject) {
		return BTR_E_NO_API;
	}

	Info = (POBJECT_NAME_INFORMATION)_alloca(BtrPageSize);
	Status = NtQueryObject(Object, ObjectNameInformation, 
		                   Info, (ULONG)BtrPageSize, &ReturnLength);

	if (NT_SUCCESS(Status)) {

		StringCchCopy(Buffer, Length, Info->Name.Buffer);
		return S_OK;

	} else {
		return S_FALSE;
	}
}

ULONG
BtrQueryKnownKeyName(
	IN HKEY hKey, 
	OUT PWCHAR Buffer, 
	IN  ULONG BufferLength 
	)
{
	switch ((ULONG_PTR)hKey) {

		case HKEY_CLASSES_ROOT:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_CLASS_ROOT");
			break;

		case HKEY_CURRENT_USER:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_CURRENT_USER");
			break;

		case HKEY_LOCAL_MACHINE:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_LOCAL_MACHINE");
			break;

		case HKEY_USERS:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_USER");
			break;

		case HKEY_CURRENT_CONFIG:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_CURRENT_CONFIG");
			break;

			//
			// Non well known keys
			//

		default:
			Buffer[0] = 0;
			return 0;
	}

	return (ULONG)wcslen(Buffer);
}

ULONG
BtrQueryKeyFullPath(
	IN HKEY Key,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	NTSTATUS Status;
	ULONG ResultLength;
	PKEY_NAME_INFORMATION Info;

	Info = (PKEY_NAME_INFORMATION)_alloca(BtrPageSize);
	Status = NtQueryKey(Key, KeyNameInformation, Info, (ULONG)BtrPageSize, &ResultLength);
	if (Status == STATUS_SUCCESS) {
		StringCchCopy(Buffer, BufferLength, Info->Name);
		return (ULONG)wcslen(Buffer);
	}

	return 0;
}

PNAME_LOOKUP_CACHE
BtrCreateNameLookupCache(
	IN PBTR_THREAD Thread
	)
{
	PNAME_LOOKUP_CACHE Cache;
	ULONG Number;
	
	Cache = Thread->NameLookupCache;
	if (Cache != NULL) {
		return Cache;
	}

	Cache = (PNAME_LOOKUP_CACHE)VirtualAlloc(NULL, sizeof(NAME_LOOKUP_CACHE), 
		                                     MEM_COMMIT, PAGE_READWRITE);
	Cache->ActiveCount = 0;
	InitializeListHead(&Cache->ActiveListHead);
	InitializeListHead(&Cache->FreeListHead);

	for(Number = 0; Number < NAME_CACHE_LENGTH; Number += 1) {
		InsertTailList(&Cache->FreeListHead, 
			           &Cache->Entry[Number].ListEntry);
	}

	Thread->NameLookupCache = Cache;
	return Cache;
}

VOID
BtrDestroyNameCache(
	IN PBTR_THREAD Thread
	)
{
	PVOID Cache;

	Cache = Thread->NameLookupCache;
	if (Cache) {
		VirtualFree(Cache, 0, MEM_RELEASE);
		Thread->NameLookupCache = NULL;
	}
}

POBJECT_NAME_ENTRY
BtrLookupNameCache(
	IN PBTR_THREAD Thread,
	IN HANDLE Object
	)
{
	ULONG Number; 
	PNAME_LOOKUP_CACHE Cache;
	POBJECT_NAME_ENTRY Entry;
	PLIST_ENTRY ListEntry;

	Cache = (PNAME_LOOKUP_CACHE)Thread->NameLookupCache;
	if (!Cache) {

		BtrCreateNameLookupCache(Thread);
		Cache = (PNAME_LOOKUP_CACHE)Thread->NameLookupCache;
		return NULL;
	}

	ListEntry = Cache->ActiveListHead.Flink;

	for(Number = 0; Number <  Cache->ActiveCount; Number += 1) {

		Entry = CONTAINING_RECORD(ListEntry, OBJECT_NAME_ENTRY, ListEntry);
		if (Entry->Object == Object) {

			//
			// If cache hit, and it's not at list head,
			// insert it into head of ActiveListHead
			//

			if (ListEntry != Cache->ActiveListHead.Flink) {
				RemoveEntryList(&Entry->ListEntry);
				InsertHeadList(&Cache->ActiveListHead, &Entry->ListEntry);
			}

			return Entry;

		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

VOID
BtrAddNameCache(
	IN PBTR_THREAD Thread,
	IN POBJECT_NAME_ENTRY New 
	)
{
	PNAME_LOOKUP_CACHE Cache;
	POBJECT_NAME_ENTRY Entry;
	PLIST_ENTRY ListEntry;

	Cache = (PNAME_LOOKUP_CACHE)Thread->NameLookupCache;
	
	//
	// Cache is full, evict the active list tail, insert
	// the new entry at head of ActiveListHead
	//

	if (Cache->ActiveCount == NAME_CACHE_LENGTH) {

		ASSERT(IsListEmpty(&Cache->FreeListHead) == TRUE);

		ListEntry = RemoveTailList(&Cache->ActiveListHead);
		Entry = CONTAINING_RECORD(ListEntry, OBJECT_NAME_ENTRY, ListEntry);
		RtlCopyMemory(Entry, New, sizeof(OBJECT_NAME_ENTRY));

		InsertHeadList(&Cache->ActiveListHead, &Entry->ListEntry);
		return;
	}

	ASSERT(IsListEmpty(&Cache->FreeListHead) != TRUE);

	ListEntry = RemoveHeadList(&Cache->FreeListHead);
	Entry = CONTAINING_RECORD(ListEntry, OBJECT_NAME_ENTRY, ListEntry);
	RtlCopyMemory(Entry, New, sizeof(OBJECT_NAME_ENTRY));

	Cache->ActiveCount += 1;
	InsertHeadList(&Cache->ActiveListHead, &Entry->ListEntry);
}

VOID
BtrEvictNameCache(
	IN PBTR_THREAD Thread,
	IN HANDLE Object
	)
{
	POBJECT_NAME_ENTRY Entry;
	PNAME_LOOKUP_CACHE Cache;

	Entry = BtrLookupNameCache(Thread, Object);
	if (Entry != NULL) {

		RemoveEntryList(&Entry->ListEntry);
		Cache = (PNAME_LOOKUP_CACHE)Thread->NameLookupCache;
		Cache->ActiveCount -= 1;

		InsertHeadList(&Cache->FreeListHead, &Entry->ListEntry);
	}
}