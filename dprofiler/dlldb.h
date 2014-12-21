//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _DLLDB_H_
#define _DLLDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#define MAX_DLL_BUCKET 257 

typedef enum _DLL_FLAG {
	DLL_SYSTEM = 0x00000000,
	DLL_TARGET = 0x00000001,
	DLL_LOADED = 0x00000010,
	DLL_UNLOAD = 0x00000020,
} DLL_FLAG, *PDLL_FLAG;

//
// N.B. DLL_ENTRY basically map MINIDUMP_DLL
// in dbghelp.h
//

typedef struct _DLL_ENTRY {

	LIST_ENTRY ListEntry;
	PVOID BaseVa;
	ULONG Size;
	ULONG CheckSum;
	ULONG Flag;
	ULONG64 LinkerTime;
    VS_FIXEDFILEINFO Version;
	LARGE_INTEGER LoadTime;
	LARGE_INTEGER UnloadTime;

	ULONG PcCount;
	ULONG SampleCount;
	WCHAR Path[MAX_PATH];

} DLL_ENTRY, *PDLL_ENTRY;
 
//
// N.B. DLL_DATABASE is allocated from heap,
// or simply a global object, we never use it
// via memory map, since it's very small.
//

typedef struct _DLL_DATABASE {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	ULONG DllCount;
	HANDLE ProcessHandle;
	LIST_ENTRY ListHead[MAX_DLL_BUCKET];
} DLL_DATABASE, *PDLL_DATABASE;

ULONG
DllCreateDatabase(
	IN ULONG ProcessId,
	IN PDLL_DATABASE *Database
	);

ULONG
DllComputeAddressHash(
	IN PVOID Address,
	IN ULONG Limit
	);

PDLL_ENTRY
DllLookupDatabase(
	IN PDLL_DATABASE Database,
	IN PVOID BaseVa
	);

PDLL_ENTRY
DllInsertDatabase(
	IN PDLL_DATABASE Database,
	IN PVOID Pc 
	);
	
#ifdef __cplusplus
}
#endif

#endif