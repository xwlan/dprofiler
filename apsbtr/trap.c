//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "trap.h"
#include "util.h"
#include "thread.h"
#include "heap.h"
#include "hal.h"
#include "stacktrace.h"

#define RIP_RANGE (ULONG_PTR)0x80000000

#if defined(_M_IX86) 

BTR_TRAP_OBJECT BtrTrapTemplate = {
	0x0,                                        // trap flag
	0x0,                                        // probe object
	0x0,                                        // trap procedure
	0x0,                                        // backward address
	0x0,										// hijacked length
	{0},                                        // original copy
	{0xcc }, 								    // hijacked code (32 bytes)
	
	//
	// short stub to set up probe object and jmp to enter procedure.
	//

	{ 0xc7, 0x44, 0x24, 0xfc },                 // mov dword ptr[esp - 4], 0
	0,
	{ 0xc7, 0x44, 0x24, 0xf8 },                 // mov dword ptr[esp - 8], object 
	0,
	{ 0xff, 0x25 },                             // jmp dword ptr[BtrTrapProcedure]
	0,                                          // (address of probe's trap procedure)
	0xc3,                                       // ret (for beauty)
};

#elif defined(_M_X64) 

BTR_TRAP_OBJECT BtrTrapTemplate = {
	0x0,                                        // trap flag
	0x0,                                        // probe object
	0x0,                                        // trap procedure
	0x0,                                        // backward address
	0x0,                                        // hijacked length
	{0},                                        // original copy
    { 0xcc },                                   // hijacked code (32 bytes)

	//
	// short stub to set up probe object and jmp to enter procedure.
	//

	{ 0x48, 0xc7, 0x44, 0x24, 0xf8 },           // mov qword[rsp-8], 0
	{ 0x0 },                                    //
	{ 0x48, 0xc7, 0x44, 0x24, 0xf0 },           // mov qword[rsp-0x10], 0
	{ 0x0 },                                    //
	{ 0xc7, 0x44, 0x24, 0xe8 },                 // mov dword ptr[rsp-0x18], low 
	{ 0x0 },
	{ 0xc7, 0x44, 0x24, 0xec },                 // mov dword ptr[rsp-0x1c], high 
	{ 0x0 },
	{ 0xff, 0x25 },                             // jmp qword[BtrTrapProcedure]
	{ 0x0 },                                    // (rip)
	{ 0x0 },                                    // (absolute address)
	{ 0xc3 }                                    // ret (for beauty)
};

#endif

//
// Trap page allocation list
//

DECLSPEC_CACHEALIGN
LIST_ENTRY BtrTrapPageList;

ULONG BtrTrapPageCount;

ULONG
BtrInitializeTrap(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	InitializeListHead(&BtrTrapPageList);
	BtrTrapPageCount = 0;
	return S_OK;
}

VOID
BtrUninitializeTrap(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_TRAP_PAGE Page;

	__try {
		while (IsListEmpty(&BtrTrapPageList) != TRUE) {

			ListEntry = RemoveHeadList(&BtrTrapPageList);
			Page = CONTAINING_RECORD(ListEntry, BTR_TRAP_PAGE, ListEntry);

			BtrFree(Page->BitMap.Buffer);
			VirtualFree(Page, 0, MEM_RELEASE);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}

VOID
BtrInitializeTrapPage(
	IN PBTR_TRAP_PAGE Page 
	)
{
	ULONG Length;
	ULONG Number;
	PVOID Buffer;

	Length = (ULONG)(BtrAllocationGranularity - FIELD_OFFSET(BTR_TRAP_PAGE, Object));
	Number = Length / sizeof(BTR_TRAP_OBJECT);

	//
	// Initialize bitmap object which track usage of trap object
	// allocation, it's aligned with 32 bits.
	//

	Length = (ULONG)BtrUlongPtrRoundUp((ULONG_PTR)Number, 32) / 8;
	Buffer = BtrMalloc(Length);
	RtlZeroMemory(Buffer, Length);

	BtrInitializeBitMap(&Page->BitMap, Buffer, Number);

	Page->StartVa = &Page->Object[0];
	Page->EndVa = &Page->Object[Number - 1];

	return;
}

PBTR_TRAP_OBJECT
BtrAllocateTrap(
	IN ULONG_PTR SourceAddress
	)
{
	PBTR_TRAP_PAGE Page;
	ULONG BitNumber;

	//
	// First scan the fusion page list to check whether there's
	// any available page fall into the rip range
	//

	Page = BtrScanTrapPageList((ULONG_PTR)SourceAddress);

	if (Page != NULL) {
	
		BitNumber = BtrFindFirstClearBit(&Page->BitMap, 0);
		if (BitNumber != (ULONG)-1) {
			RtlZeroMemory(&Page->Object[BitNumber], sizeof(BTR_TRAP_OBJECT));
			BtrSetBit(&Page->BitMap, BitNumber);
			return &Page->Object[BitNumber];
		}
	}

	//
	// Either there's no usable page, or the page is full, now allocate
	// a new region if possible
	//

	Page = BtrAllocateTrapPage(SourceAddress);
	if (!Page) {
		return NULL;
	}

	//
	// Insert trap page into global trap page list, note that because
	// trap allocation and free are always performed by a unique runtime
	// thread, so no lock acquired to protect its state
	//
	
	BtrInitializeTrapPage(Page);
	InsertHeadList(&BtrTrapPageList, &Page->ListEntry);
	BtrTrapPageCount += 1;

	RtlZeroMemory(&Page->Object[0], sizeof(BTR_TRAP_OBJECT));
	BtrSetBit(&Page->BitMap, 0);
	return &Page->Object[0];
}

PBTR_TRAP_PAGE
BtrScanTrapPageList(
	IN ULONG_PTR SourceAddress
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_TRAP_PAGE Page;

	ListEntry = BtrTrapPageList.Flink;
	while (ListEntry != &BtrTrapPageList) {
	
		Page = CONTAINING_RECORD(ListEntry, BTR_TRAP_PAGE, ListEntry);
		
		if (SourceAddress > (ULONG_PTR)Page->EndVa) {
			if (SourceAddress - (ULONG_PTR)Page->StartVa < RIP_RANGE - BtrAllocationGranularity) {
				return Page;
			}
		} 

		if (SourceAddress < (ULONG_PTR)Page->StartVa) {
			if ((ULONG_PTR)Page->EndVa - SourceAddress < RIP_RANGE - BtrAllocationGranularity) {
				return Page;
			}
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

BOOLEAN
BtrIsTrapPage(
	IN PVOID Address
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_TRAP_PAGE Page;

	ListEntry = BtrTrapPageList.Flink;
	while (ListEntry != &BtrTrapPageList) {
	
		Page = CONTAINING_RECORD(ListEntry, BTR_TRAP_PAGE, ListEntry);
		
		if ((ULONG_PTR)Address >= (ULONG_PTR)Page->StartVa &&
			(ULONG_PTR)Address < (ULONG_PTR)Page->EndVa ) {
			return TRUE;
		} 

		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

PBTR_TRAP_PAGE
BtrAllocateTrapPage(
	IN ULONG_PTR SourceAddress
	)
{
	ULONG_PTR Address;
    ULONG_PTR Region;
    ULONG_PTR LowLimit;
    ULONG_PTR HighLimit;
	ULONG_PTR Granularity;
	PBTR_TRAP_PAGE Page;
	MEMORY_BASIC_INFORMATION Information;

	Granularity = BtrAllocationGranularity;
	Address = BtrUlongPtrRoundDown(SourceAddress, Granularity);

	//
	// Compute the legal allocation range source address can safely reach
	//

	if (Address < RIP_RANGE) {
		LowLimit = BtrMinimumUserAddress + Granularity;
	} else {
	    LowLimit = Address - RIP_RANGE + Granularity;
	}

    HighLimit = Address + RIP_RANGE - Granularity;
	
	if(HighLimit > BtrMaximumUserAddress) {
       HighLimit = BtrMaximumUserAddress - Granularity;
	}

	//
	// Scan higher range of the addressable memory space
	// first skip the region contain specified source address
	//

	Region = BtrUlongPtrRoundUp(SourceAddress, Granularity);
	while (Region < HighLimit) {
	
		VirtualQuery((PVOID)Region, &Information, sizeof(Information));

		if (Information.State & MEM_FREE) {

			Page = (PBTR_TRAP_PAGE)VirtualAlloc((PVOID)Region, 
				                                 Granularity, 
												 MEM_RESERVE | MEM_COMMIT, 
												 PAGE_EXECUTE_READWRITE);
			if (Page != NULL) {
				return Page;
			}
			else {
			
				//
				// N.B. This can be caused by a race condition in multithreaded environment
				// other thread already allocate the specified region before current thread
				//

				VirtualQuery((PVOID)Region, &Information, sizeof(Information));
			}
		}
	
		Region += Information.RegionSize;
		Region = BtrUlongPtrRoundUp(Region, Granularity);
	}

	//
	// Scan the lower range of addressable memory space, note that this scan
	// is in a decremental direction, this keep the allocation as near as possible
	// to the source address, and make rip code to safely address its destine data.
	//

	Region = BtrUlongPtrRoundDown(SourceAddress, Granularity);
	while (Region > LowLimit) {
	
		VirtualQuery((PVOID)Region, &Information, sizeof(Information));

		if (Information.State & MEM_FREE) {

			Page = (PBTR_TRAP_PAGE)VirtualAlloc((PVOID)Region, 
				                                 Granularity, 
												 MEM_RESERVE | MEM_COMMIT, 
												 PAGE_EXECUTE_READWRITE);
			if (Page != NULL) {
				return Page;
			}

			else {
			
				VirtualQuery((PVOID)Region, &Information, sizeof(Information));
			}
		}
	
		Region -= Information.RegionSize;
		BtrUlongPtrRoundDown(Region, Granularity);
	}

	return NULL;
}

BOOLEAN
BtrIsTrapPossible(
	IN ULONG_PTR DestineAddress,
	IN ULONG_PTR SourceAddress
	)
{
	ULONG_PTR Distance;

	if (DestineAddress >= SourceAddress) {
		Distance = DestineAddress - SourceAddress;
	} else {
		Distance = SourceAddress - DestineAddress;
	}

	if (Distance > RIP_RANGE - BtrAllocationGranularity) {
		return FALSE;
	}

	return TRUE;
}

#if defined (_M_IX86)

ULONG
BtrFuseTrap(
	IN PVOID Address, 
	IN PBTR_DECODE_CONTEXT Decode,
	IN PBTR_TRAP_OBJECT Trap
	)
{
	PUCHAR Buffer;

	//
	// Fill the trap object with trap template, and copy hijacked
	// code to back up buffer
	//

	RtlCopyMemory(Trap->OriginalCopy, (PVOID)Address, Decode->ResultLength); 
	RtlCopyMemory(Trap->HijackedCode, (PVOID)Address, Decode->ResultLength); 
	Trap->HijackedLength = Decode->ResultLength; 

	//
	// Connect trap back to target via jmp dword ptr[m32] m32
	//

	Buffer = (PUCHAR)&Trap->HijackedCode[0] + Trap->HijackedLength;
	Buffer[0] = (UCHAR)0xff;
	Buffer[1] = (UCHAR)0x25;
	*(PULONG)&Buffer[2] = PtrToUlong(&Trap->BackwardAddress);
	Trap->BackwardAddress = (PCHAR)Address + Decode->ResultLength;

	//
	// Connect trap to runtime 
	//

	if (!FlagOn(Trap->TrapFlag, BTR_FLAG_CALLBACK)) {
		Trap->TrapProcedure = &BtrTrapProcedure;
		Trap->Procedure = &Trap->TrapProcedure;
	}

	return S_OK;
}

ULONG
BtrFuseTrapLite(
	IN PVOID Address, 
	IN PVOID Destine,
	IN PBTR_TRAP_OBJECT Trap
	)
{
	PUCHAR Buffer;

	ASSERT(!FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_UNSUPPORTED));

	if (FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_RM32)) {
		Trap->HijackedLength = 6;
	}
	else if (FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_REL32)) {
		Trap->HijackedLength = 5;
	}
	else {
		ASSERT(0);
	}

	RtlCopyMemory(Trap->OriginalCopy, Address, Trap->HijackedLength);

	//
	// Connect trap back to target via jmp dword ptr[m32] m32
	//

	Buffer = (PUCHAR)&Trap->HijackedCode[0];
	Buffer[0] = (UCHAR)0xff;
	Buffer[1] = (UCHAR)0x25;
	*(PULONG)&Buffer[2] = PtrToUlong(&Trap->BackwardAddress);

	//
	// N.B. for BTR_FLAG_FUSELITE, we just jump to previous hooked handler (hooked),
	// or stub's function body (not hooked)
	//

	Trap->BackwardAddress = Destine;

	//
	// Copy original jmp instruction in the end.
	//

	RtlCopyMemory(&Buffer[6], Address, Trap->HijackedLength);

	//
	// Connect trap to runtime 
	//

	if (!FlagOn(Trap->TrapFlag, BTR_FLAG_CALLBACK)) {
		Trap->TrapProcedure = &BtrTrapProcedure;
		Trap->Procedure = &Trap->TrapProcedure;
	}

	return S_OK;
} 

#endif

#if defined (_M_X64)

ULONG
BtrFuseTrap(
	IN PVOID Address, 
	IN PBTR_DECODE_CONTEXT Decode,
	IN PBTR_TRAP_OBJECT Trap
	)
{
	PUCHAR Buffer;

	//
	// Fill the trap object with trap template, and copy hijacked
	// code to back up buffer
	//
	
	RtlCopyMemory(Trap->OriginalCopy, (PVOID)Address, Decode->ResultLength); 
	RtlCopyMemory(Trap->HijackedCode, (PVOID)Address, Decode->ResultLength); 
	Trap->HijackedLength = Decode->ResultLength; 

	if (Decode->RipDisplacementOffset != 0) {
		
		//
		// Adjust RIP displacement if there's RIP relative addressing 
		//

		ULONG64 Rip;
		ULONG64 Absolute;
		LONG Displacement;
		
		Rip = (ULONG_PTR)Address + Decode->ResultLength;
		Displacement = *(PLONG)((ULONG_PTR)Address + Decode->RipDisplacementOffset);
		Absolute = (ULONG64)((LONG64)Rip + (LONG64)Displacement);

		Rip = (ULONG64)Trap->HijackedCode + (ULONG64)Trap->HijackedLength;
		*(PLONG)(Rip - 4) = (LONG)(Absolute - Rip);

	}

	//
	// Connect trap back to target jmp qword[rip] m64
	//

	Buffer = (PUCHAR)&Trap->HijackedCode[0] + Trap->HijackedLength;
	Buffer[0] = (UCHAR)0xff;
	Buffer[1] = (UCHAR)0x25;

	//
	// N.B. 0xff25 in x64 use RIP relative addressing
	//

	*(PLONG)&Buffer[2] = (LONG)((LONG_PTR)&Trap->BackwardAddress - (LONG_PTR)(Buffer + 6));
	Trap->BackwardAddress = (PVOID)((ULONG64)Address + (ULONG64)Decode->ResultLength);

	Trap->TrapProcedure = &BtrTrapProcedure;
	Trap->Rip = 0;
	Trap->Procedure = &BtrTrapProcedure;

	return S_OK;
}

ULONG
BtrFuseTrapLite(
	IN PVOID Address, 
	IN PVOID Destine,
	IN PBTR_TRAP_OBJECT Trap
	)
{
	PUCHAR Buffer;

	ASSERT(!FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_UNSUPPORTED));

	if (FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_RM64)) {
		Trap->HijackedLength = 6;
	}
	else if (FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_REL32)) {
		Trap->HijackedLength = 5;
	}
	else {
		ASSERT(0);
	}

	RtlCopyMemory(Trap->OriginalCopy, Address, Trap->HijackedLength);

	//
	// Connect trap back to target jmp qword[rip] m64
	//

	Buffer = (PUCHAR)&Trap->HijackedCode[0];
	Buffer[0] = (UCHAR)0xff;
	Buffer[1] = (UCHAR)0x25;

	//
	// N.B. 0xff25 in x64 use RIP relative addressing
	//

	*(PLONG)&Buffer[2] = (LONG)((LONG_PTR)&Trap->BackwardAddress - (LONG_PTR)(Buffer + 6));
	Trap->BackwardAddress = Destine;

	Trap->TrapProcedure = &BtrTrapProcedure;
	Trap->Rip = 0;
	Trap->Procedure = &BtrTrapProcedure;

	//
	// Copy original jmp instruction in the end.
	//

	RtlCopyMemory(&Buffer[6], Address, Trap->HijackedLength);
	return S_OK;
}

#endif

VOID
BtrFreeTrap(
	IN PBTR_TRAP_OBJECT Trap
	)
{
	PBTR_TRAP_PAGE Page;
	ULONG Distance;
	ULONG BitNumber;

	//
	// N.B. This routine assume that either the trap object is
	// not valid due to probe failure, or it's already defused
	// from runtime, else it can crash if any thread is executing
	// in the range of trap object
	//

	Page = (PBTR_TRAP_PAGE)BtrUlongPtrRoundDown((ULONG_PTR)Trap, 
		                                        BtrAllocationGranularity);

	Distance = (ULONG)((ULONG_PTR)Trap - (ULONG_PTR)&Page->Object[0]);
	BitNumber = Distance / sizeof(BTR_TRAP_OBJECT);

	BtrClearBit(&Page->BitMap, BitNumber);
	BtrZeroSmallMemory((PUCHAR)Trap, sizeof(BTR_TRAP_OBJECT));
}

PBTR_ADDRESS_RANGE
BtrGetTrapRange(
	OUT PULONG Count
	)
{
	PBTR_ADDRESS_RANGE Range;
	PBTR_TRAP_PAGE Page;
	PLIST_ENTRY ListEntry;
	ULONG Size; 
	ULONG i;
	
	Size = sizeof(BTR_ADDRESS_RANGE) * BtrTrapPageCount;
	Range = (PBTR_ADDRESS_RANGE)BtrMalloc(Size);

	i = 0;
	ListEntry = BtrTrapPageList.Flink;

	while (ListEntry != &BtrTrapPageList) {

		Page = CONTAINING_RECORD(ListEntry, BTR_TRAP_PAGE, ListEntry);
		Range[i].Address = (ULONG_PTR)&Page->Object[0];
		Range[i].Size = (ULONG_PTR)Page->EndVa -  (ULONG_PTR)Page->StartVa;
		i += 1;

		ListEntry = ListEntry->Flink;
	}

	*Count = BtrTrapPageCount;
	return Range;
}