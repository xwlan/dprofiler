//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "heap.h"
#include <malloc.h>

HANDLE MmHeapHandle;

ULONG
BtrInitializeHeap(
	VOID	
	) 
{
	ULONG Flag = 2;
 
	//
	// Get private crt heap handle
	//

	MmHeapHandle = (HANDLE)_get_heap_handle();
    HeapSetInformation(MmHeapHandle, HeapCompatibilityInformation, 
		               &Flag, sizeof(Flag));

	return S_OK;
}

VOID
BtrUninitializeHeap(
	VOID
	)
{
}

PVOID 
BtrMalloc(
	IN ULONG ByteCount
	)
{
	PVOID Ptr;

	Ptr = malloc(ByteCount);
	return Ptr;
}

VOID 
BtrFree(
	IN PVOID Address
	)
{
	free(Address);
}

BOOLEAN
BtrIsPrivateHeap(
	IN HANDLE Heap
	)
{
	return (Heap == MmHeapHandle);
}

PVOID
BtrAlignedMalloc(
	__in ULONG ByteCount,
	__in ULONG Alignment
	)
{
	return _aligned_malloc(ByteCount, Alignment);
}

VOID
BtrAlignedFree(
	__in PVOID Address
	)
{
	_aligned_free(Address);
}