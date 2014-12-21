//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _NAMETBL_H_
#define _NAMETBL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mmprof.h"
#include "hal.h"

typedef struct _OBJECT_NAME_ENTRY {
	LIST_ENTRY ListEntry;
	HANDLE Object;
	HANDLE_TYPE Type;
	ULONG Hash;
	ULONG Length;
	WCHAR Name[MAX_PATH];
} OBJECT_NAME_ENTRY, *POBJECT_NAME_ENTRY;

//
// Object Name Table is backed by page file
//

typedef struct _OBJECT_NAME_TABLE {

	HANDLE Object;
	PVOID BaseVa;
	ULONG Size;

	ULONG NumberOfBuckets;
	ULONG NumberOfFixedBuckets;

	HASH_ENTRY Entry[ANYSIZE_ARRAY];

} OBJECT_NAME_TABLE, *POBJECT_NAME_TABLE;

//
// Thread local name lookup cache
//

#define NAME_CACHE_LENGTH 16

typedef struct _NAME_LOOKUP_CACHE {
	ULONG ActiveCount;
	LIST_ENTRY ActiveListHead;
	LIST_ENTRY FreeListHead;
	OBJECT_NAME_ENTRY Entry[NAME_CACHE_LENGTH];
} NAME_LOOKUP_CACHE, *PNAME_LOOKUP_CACHE;

ULONG
BtrCreateObjectNameTable(
	IN ULONG FixedBuckets
	);

VOID
BtrDestroyNameTable(
	VOID
	);

ULONG FORCEINLINE
BtrHashStringBKDR(
	IN PWSTR Buffer
	);

ULONG FORCEINLINE
BtrComputeNameBucket(
	IN ULONG Hash
	);

ULONG
BtrInsertObjectName(
	IN HANDLE Object,
	IN HANDLE_TYPE Type,
	IN PWSTR Name,
	IN ULONG Length
	);

ULONG
BtrQueryObjectName(
	IN HANDLE Object,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
BtrQueryKnownKeyName(
	IN HKEY hKey, 
	OUT PWCHAR Buffer, 
	IN  ULONG BufferLength 
	);

ULONG
BtrQueryKeyFullPath(
	IN HKEY Key,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

//
// N.B. 
// 
// Add name when:
//
// 1, in object create routine
// 2, in object open routine
//
// Evict name when:
//
// 1, in object close routine
// 2, if cache is full
//

PNAME_LOOKUP_CACHE
BtrCreateNameLookupCache(
	IN PBTR_THREAD Thread
	);

VOID
BtrDestroyNameCache(
	IN PBTR_THREAD Thread
	);

POBJECT_NAME_ENTRY
BtrLookupNameCache(
	IN PBTR_THREAD Thread,
	IN HANDLE Object
	);

VOID
BtrAddNameCache(
	IN PBTR_THREAD Thread,
	IN POBJECT_NAME_ENTRY Entry
	);

VOID
BtrEvictNameCache(
	IN PBTR_THREAD Thread,
	IN HANDLE Object
	);

extern POBJECT_NAME_TABLE BtrObjectNameTable;

#ifdef __cplusplus
}
#endif
#endif
