//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _HEAP_H_
#define _HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "apsprofile.h"

typedef enum _BTR_LOOKASIDE_TYPE {
	LOOKASIDE_HEAP,
	LOOKASIDE_PAGE,
	LOOKASIDE_HANDLE,
	LOOKASIDE_GDI,
	LOOKASIDE_NET,
	LOOKASIDE_FILE,
	BTR_MAX_LOOKASIDE, 
} BTR_LOOKASIDE_TYPE;

typedef struct DECLSPEC_CACHEALIGN _BTR_LOOKASIDE {
	SLIST_HEADER ListHead;
	ULONG BlockSize;
	ULONG MaximumDepth;
	ULONG TotalAllocates;
	ULONG AllocateMisses;
	ULONG TotalFrees;
	ULONG FreeMisses;
} BTR_LOOKASIDE, *PBTR_LOOKASIDE;

ULONG
BtrInitializeHeap(
	VOID	
	);

VOID
BtrUninitializeHeap(
	VOID
	);

PVOID 
BtrMalloc(
	__in ULONG ByteCount
	);

VOID 
BtrFree(
	__in PVOID Address
	);

PVOID
BtrAlignedMalloc(
	__in ULONG ByteCount,
	__in ULONG Alignment
	);

VOID
BtrAlignedFree(
	__in PVOID Address
	);

BOOLEAN
BtrIsPrivateHeap(
	__in HANDLE Heap
	);

VOID
BtrInitializeLookaside(
	VOID
	);

PVOID
BtrMallocLookaside(
	__in BTR_LOOKASIDE_TYPE Type
	);

VOID
BtrFreeLookaside(
	__in BTR_LOOKASIDE_TYPE Type,
	__in PVOID Ptr
	);

#ifdef __cplusplus
}
#endif
#endif