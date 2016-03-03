//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//
//

#ifndef _MM_PROF_H_
#define _MM_PROF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "callback.h"
#include <oleauto.h>

#define MAX_HEAPLIST_BUCKET 233 
#define MAX_HEAP_BUCKET    4093
#define MAX_PAGE_BUCKET    4093
#define MAX_MMAP_BUCKET    4093
#define MAX_HANDLE_BUCKET  4093
#define MAX_GDI_BUCKET     4093

//
// Heap Hash Table
//

typedef struct _MM_HEAP_TABLE {
	LIST_ENTRY ListEntry;
	ULONG64 ReservedBytes;
	ULONG64 CommittedBytes;
	ULONG64 AllocatedBytes;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	ULONG CreateStackHash;
	ULONG CreateStackDepth;
	PVOID CreatePc;
	ULONG DestroyStackHash;
	ULONG DestroyStackDepth;
	PVOID DestroyPc;
	HANDLE HeapHandle;
	BTR_SPIN_HASH ListHead[MAX_HEAP_BUCKET];
} MM_HEAP_TABLE, *PMM_HEAP_TABLE;

typedef struct _MM_HEAPLIST_TABLE {
	ULONG64 ReservedBytes;
	ULONG64 CommittedBytes;
	ULONG64 AllocatedBytes;
	ULONG64 NumberOfAllocs;
	ULONG64 NumberOfFrees;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	ULONG NumberOfHeaps;
	ULONG NumberOfActiveHeaps;
	ULONG64 Spare0;
	BTR_LOCK Lock;
	BTR_SPIN_HASH Active[MAX_HEAPLIST_BUCKET];	
	BTR_SPIN_HASH Retire;	
} MM_HEAPLIST_TABLE, *PMM_HEAPLIST_TABLE;

//
// Page Table
//

typedef struct _MM_PAGE_TABLE {
	ULONG64 ReservedBytes;
	ULONG64 CommittedBytes;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	BTR_SPIN_HASH ListHead[MAX_PAGE_BUCKET];
} MM_PAGE_TABLE, *PMM_PAGE_TABLE;

//
// Kernel Handle Table
//

typedef struct _MM_HANDLE_TABLE {
	ULONG HandleCount;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	PF_TYPE_ENTRY Types[HANDLE_LAST];
	BTR_SPIN_HASH ListHead[MAX_HEAP_BUCKET];
} MM_HANDLE_TABLE, *PMM_HANDLE_TABLE;

//
// GDI Handle Table, note that GDI object
// type starts from OBJ_PEN it's 1, not 0,
// so first entry of Types is ignored.
//

typedef struct _MM_GDI_TABLE {
	ULONG HandleCount;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	PF_TYPE_ENTRY Types[GDI_OBJ_LAST];
	BTR_SPIN_HASH ListHead[MAX_HEAP_BUCKET];
} MM_GDI_TABLE, *PMM_GDI_TABLE;


ULONG
MmGetHeapHandles(
	VOID
	);

BOOLEAN
MmDllCanUnload(
	VOID
	);

PMM_HEAP_TABLE
MmInsertHeapTable(
	__in HANDLE HeapHandle
	);

VOID
MmRetireHeapTable(
	__in HANDLE HeapHandle
	);

PMM_HEAP_TABLE
MmLookupHeapTable(
	__in HANDLE HeapHandle
	);

PMM_HEAP_TABLE
MmInsertHeapRecord(
	__in HANDLE HeapHandle,
	__in PVOID Address,
	__in ULONG Size,
	__in ULONG Duration,
	__in ULONG StackHash,
	__in USHORT StackDepth,
	__in USHORT CallbackType
	);

BOOLEAN
MmRemoveHeapRecord(
	__in HANDLE HeapHandle,
	__in PVOID Address
	);

PPF_HEAP_RECORD
MmLookupHeapRecord(
	__in PMM_HEAP_TABLE Table,
	__in PVOID Address
	);

VOID
MmInsertPageRecord(
	__in PVOID Address,
	__in ULONG Size,
	__in ULONG Duration,
	__in ULONG StackHash,
	__in USHORT StackDepth,
	__in USHORT CallbackType
	);

VOID
MmRemovePageRecord(
	__in PVOID Address
	);

PPF_PAGE_RECORD
MmLookupPageRecord(
	__in PVOID Address
	);

VOID
MmInsertHandleRecord(
	__in HANDLE Handle,
	__in HANDLE_TYPE Type,
	__in ULONG Duration,
	__in ULONG StackHash,
	__in USHORT StackDepth,
	__in USHORT CallbackType
	);

VOID
MmRemoveHandleRecord(
	__in HANDLE Handle
	);

PPF_HANDLE_RECORD
MmLookupHandleRecord(
	__in HANDLE Handle
	);

VOID
MmInsertGdiRecord(
	__in HANDLE Handle,
	__in ULONG Type,
	__in ULONG Duration,
	__in ULONG StackHash,
	__in USHORT StackDepth,
	__in USHORT CallbackType
	);

VOID
MmRemoveGdiRecord(
	__in HANDLE Handle
	);

PPF_GDI_RECORD
MmLookupGdiRecord(
	__in HANDLE Handle
	);

BOOLEAN
MmIsSkipAllocators(
	VOID
	);

ULONG
MmValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	);

ULONG
MmInitialize(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
MmStartProfile(
	__in PBTR_PROFILE_OBJECT Object	
	);

ULONG
MmStopProfile(
	__in PBTR_PROFILE_OBJECT Object	
	);

ULONG
MmPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
MmResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
MmThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
MmThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
MmUnload(
	__in PBTR_PROFILE_OBJECT Object
	);

BOOLEAN
MmIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	);

ULONG
MmQueryHotpatch(
	__in PBTR_PROFILE_OBJECT Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	);

ULONG CALLBACK
MmProfileProcedure(
	__in PVOID Context
	);

VOID WINAPI 
MmExitProcessCallback(
	__in UINT ExitCode
	);

//
// Callback structures
//

extern BTR_CALLBACK MmHeapCallback[];
extern ULONG MmHeapCallbackCount;

extern DECLSPEC_CACHEALIGN
MM_HEAPLIST_TABLE MmHeapListTable;

extern DECLSPEC_CACHEALIGN
MM_PAGE_TABLE MmPageTable;

extern DECLSPEC_CACHEALIGN
MM_HANDLE_TABLE MmHandleTable;

extern DECLSPEC_CACHEALIGN
MM_GDI_TABLE MmGdiTable;

extern DECLSPEC_CACHEALIGN
BTR_PC_TABLE MmPcTable;

extern HANDLE MmProcessHeap;
extern HANDLE MmBstrHeap;
extern HANDLE MmMsvcrtHeap;

#ifdef __cplusplus
}
#endif
#endif