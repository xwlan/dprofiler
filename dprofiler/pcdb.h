//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PCDB_H_
#define _PCDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

struct _MODULE_ENTRY;

typedef struct _PC_ENTRY {
	LIST_ENTRY ListEntry;
	ULONG64 Pc;
	PSTR Name;
	struct _MODULE_ENTRY *Module;  
	ULONG Inclusive;
	ULONG Exclusive;
} PC_ENTRY, *PPC_ENTRY;
	
typedef struct _PC_DATABASE {
	HANDLE Object;
	PVOID BaseVa;
	SIZE_T Size;
	ULONG Buckets;
	ULONG PcCount;
	ULONG Inclusive;
	ULONG Exclusive;
	LIST_ENTRY ListHead[ANYSIZE_ARRAY];
} PC_DATABASE, *PPC_DATABASE;


#define PC_BUCKET_NUMBER  4093

#define PC_HASH_BUCKET(_D, _P) \
	((_P) % (_D->Buckets))

ULONG
PcCreateDatabase(
	IN PPC_DATABASE *Database,
	IN ULONG Buckets
	);

PPC_ENTRY
PcLookupDatabase(
	IN PPC_DATABASE Database,
	IN ULONG_PTR Pc
	);

PPC_ENTRY
PcInsertDatabase(
	IN PPC_DATABASE Database,
	IN ULONG_PTR Pc
	);

VOID
PcDecodePc(
	IN HANDLE Handle,
	IN PPC_DATABASE Database,
	IN PPC_ENTRY Entry
	);

ULONG
PcDecodeDll(
	IN HANDLE Handle,
	IN ULONG64 Pc,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	);

#ifdef __cplusplus
}
#endif

#endif