//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_STAT_H_
#define _MM_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsprofile.h"

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#define DPF_HEAPLIST_BUCKET 233 
#define DPF_HEAP_BUCKET    4093
#define DPF_PAGE_BUCKET    4093
#define DPF_MMAP_BUCKET    4093
#define DPF_HANDLE_BUCKET  4093
#define DPF_GDI_BUCKET     4093

#define DPF_DLL_BUCKET 233  

#if defined (_M_IX86)
#ifndef GOLDEN_RATIO
#define GOLDEN_RATIO 0x9E3779B9
#endif

#elif defined (_M_X64)
#ifndef GOLDEN_RATIO
#define GOLDEN_RATIO 0x9E3779B97F4A7C13
#endif
#endif

typedef enum _DPF_DLL_FLAG {
	DPF_DLL_SYSTEM = 0x00000000,
	DPF_DLL_TARGET = 0x00000001,
	DPF_DLL_LOADED = 0x00000010,
	DPF_DLL_UNLOAD = 0x00000020,
} DPF_DLL_FLAG, *PDPF_DLL_FLAG;

//
// Heap Hash Table
//

typedef struct _DPF_HEAP_TABLE {
	LIST_ENTRY ListEntry;
	PVOID CallerCreate;
	PVOID CallerDestroy; 
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	HANDLE HeapHandle;
#if defined (_M_X64)
	ULONG64 Spare0;
#endif
	LIST_ENTRY ListHead[DPF_HEAP_BUCKET];
} DPF_HEAP_TABLE, *PDPF_HEAP_TABLE;

typedef struct _DPF_HEAPLIST_TABLE {
	ULONG64 NumberOfAllocs;
	ULONG64 NumberOfFrees;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	ULONG NumberOfHeaps;
	ULONG NumberOfTrackedHeaps;
	ULONG64 Spare0;
	LIST_ENTRY ActiveListHead[DPF_HEAPLIST_BUCKET];	
	LIST_ENTRY RetireListHead;	
} DPF_HEAPLIST_TABLE, *PDPF_HEAPLIST_TABLE;

//
// Page Table
//

typedef struct _DPF_PAGE_TABLE {
	ULONG64 NumberOfAllocs;
	ULONG64 NumberOfFrees;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	LIST_ENTRY ListHead[DPF_PAGE_BUCKET];
} DPF_PAGE_TABLE, *PDPF_PAGE_TABLE;

//
// Kernel Handle Table
//

typedef struct _DPF_HANDLE_TYPE_ENTRY {
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
} DPF_HANDLE_TYPE_ENTRY, *PDPF_HANDLE_TYPE_ENTRY;

typedef struct _DPF_HANDLE_TABLE {
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	ULONG64 Spare0;
	DPF_HANDLE_TYPE_ENTRY TypeEntry[HANDLE_LAST];
	LIST_ENTRY ListHead[DPF_HANDLE_BUCKET];
} DPF_HANDLE_TABLE, *PDPF_HANDLE_TABLE;

//
// GDI Handle Table, note that TypeEntry start from
// 1, because GDI object type start from 1, it's OBJ_PEN
//

typedef struct _DPF_GDI_TABLE {
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	ULONG Spare[2];
	DPF_HANDLE_TYPE_ENTRY TypeEntry[GDI_LAST];
	LIST_ENTRY ListHead[DPF_GDI_BUCKET];
} DPF_GDI_TABLE, *PDPF_GDI_TABLE;

//
// Dll Table
//

typedef struct _DPF_DLL_ENTRY {

	LIST_ENTRY ListEntry;
	PVOID BaseVa;
	ULONG Size;
	ULONG CheckSum;
	ULONG Flag;
	ULONG64 LinkerTime;
    VS_FIXEDFILEINFO Version;
	LARGE_INTEGER LoadTime;
	LARGE_INTEGER UnloadTime;
	
	//
	// MM Profile
	//

	ULONG NumberOfHeapAllocs;
	ULONG NumberOfHeapFrees;
	ULONG64 SizeOfHeapAllocs;
	ULONG64 SizeOfHeapFrees;
	DPF_HEAPLIST_TABLE HeapListTable;

	ULONG NumberOfPageAllocs;
	ULONG NumberOfPageFrees;
	ULONG64 SizeOfPageAllocs;
	ULONG64 SizeOfPageFrees;

	ULONG NumberOfHandleAllocs;
	ULONG NumberOfHandleFrees;
	DPF_HANDLE_TYPE_ENTRY HandleType[HANDLE_LAST];

	ULONG NumberOfGdiAllocs;
	ULONG NumberOfGdiFrees;
	DPF_HANDLE_TYPE_ENTRY GdiType[GDI_LAST];
	
	WCHAR Path[MAX_PATH];

} DPF_DLL_ENTRY, *PDPF_DLL_ENTRY;


typedef struct _DPF_DLL_TABLE {
	ULONG ProcessId;
	ULONG NumberOfDlls;
	HANDLE ProcessHandle;
	LIST_ENTRY ListHead[DPF_DLL_BUCKET];
} DPF_DLL_TABLE, *PDPF_DLL_TABLE;

typedef struct _DPF_PC_ENTRY {
	LIST_ENTRY ListEntry;
	PVOID Pc;
	PDPF_DLL_ENTRY DllEntry;
} DPF_PC_ENTRY, *PDPF_PC_ENTRY;

//
// N.B. Thread alloc/free counters are tracked
// in profile target address space, because it's
// thread local, won't incur lock contention,
// we just need to read the thread counters after
// profile.
//

//
// Global MM Profile Tables
//

extern DPF_HEAPLIST_TABLE DpfHeapListTable;
extern DPF_HANDLE_TABLE   DpfHandleTable;
extern DPF_PAGE_TABLE     DpfPageTable;
extern DPF_GDI_TABLE      DpfGdiTable;

//
// Global DLL Table
//

extern DPF_DLL_TABLE DpfDllTable;

#ifdef __cplusplus
}
#endif
#endif