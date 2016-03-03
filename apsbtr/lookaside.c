//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "apsprofile.h"
#include "btr.h"
#include "heap.h"
#include "ioprof.h"

DECLSPEC_CACHEALIGN
BTR_LOOKASIDE BtrLookaside[BTR_MAX_LOOKASIDE];

VOID
BtrInitializeLookaside(
	VOID
	)
{
	PBTR_LOOKASIDE Lookaside;

	if (BtrProfileObject->Attribute.Type == PROFILE_MM_TYPE) {

		Lookaside = &BtrLookaside[LOOKASIDE_HEAP];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 4096;
		Lookaside->BlockSize = sizeof(PF_HEAP_RECORD);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

		Lookaside = &BtrLookaside[LOOKASIDE_PAGE];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 64;
		Lookaside->BlockSize = sizeof(PF_PAGE_RECORD);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

		Lookaside = &BtrLookaside[LOOKASIDE_HANDLE];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 64;
		Lookaside->BlockSize = sizeof(PF_HANDLE_RECORD);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

		Lookaside = &BtrLookaside[LOOKASIDE_GDI];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 64;
		Lookaside->BlockSize = sizeof(PF_GDI_RECORD);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

	}

	if (BtrProfileObject->Attribute.Type == PROFILE_IO_TYPE) {

		Lookaside = &BtrLookaside[LOOKASIDE_IO_IRP];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 4096;
		Lookaside->BlockSize = sizeof(IO_IRP);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

		Lookaside = &BtrLookaside[LOOKASIDE_IO_IRP_TRACK];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 4096;
		Lookaside->BlockSize = sizeof(IO_COMPLETION_PACKET);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

		Lookaside = &BtrLookaside[LOOKASIDE_IO_COMPLETION];
		InitializeSListHead(&Lookaside->ListHead);
		Lookaside->MaximumDepth = 4096;
		Lookaside->BlockSize = sizeof(IO_COMPLETION_PACKET);
		Lookaside->TotalAllocates = 0;
		Lookaside->AllocateMisses = 0;
		Lookaside->TotalFrees = 0;
		Lookaside->FreeMisses = 0;

	}

}

PVOID
BtrMallocLookaside(
	__in BTR_LOOKASIDE_TYPE Type
	)
{
	PBTR_LOOKASIDE Lookaside;
	PUCHAR Ptr;
	size_t Length;

	Lookaside = &BtrLookaside[Type];
	Length = Lookaside->BlockSize + MEMORY_ALLOCATION_ALIGNMENT;

	InterlockedIncrement(&Lookaside->TotalAllocates);

	Ptr = (PUCHAR)InterlockedPopEntrySList(&Lookaside->ListHead);
	if (Ptr != NULL) {
		RtlZeroMemory(Ptr, Length);
		return (Ptr + MEMORY_ALLOCATION_ALIGNMENT);
	}

	Ptr = (PUCHAR)BtrAlignedMalloc((ULONG)Length, MEMORY_ALLOCATION_ALIGNMENT);
	if (!Ptr) {
		return NULL;
	}

	InterlockedIncrement(&Lookaside->AllocateMisses);
	return (Ptr + MEMORY_ALLOCATION_ALIGNMENT);
}

VOID
BtrFreeLookaside(
	__in BTR_LOOKASIDE_TYPE Type,
	__in PVOID Ptr
	)
{
	PBTR_LOOKASIDE Lookaside;
	PSLIST_ENTRY ListEntry;
	USHORT Depth;

	if (!Ptr) {
		return;
	}

	Lookaside = &BtrLookaside[Type];
	InterlockedIncrement(&Lookaside->TotalFrees);

	Depth = QueryDepthSList(&Lookaside->ListHead);
	ListEntry = (PSLIST_ENTRY)((PUCHAR)Ptr - MEMORY_ALLOCATION_ALIGNMENT);

	if (Depth < Lookaside->MaximumDepth) {
		InterlockedPushEntrySList(&Lookaside->ListHead, ListEntry);
	} else {
		InterlockedIncrement(&Lookaside->FreeMisses);
		BtrAlignedFree(ListEntry);
	}
}
