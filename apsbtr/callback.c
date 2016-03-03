//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//


#include "apsbtr.h"
#include "callback.h"
#include "btr.h"
#include "trap.h"
#include "heap.h"
#include "thread.h"
#include "hal.h"
#include "cache.h"
#include "stacktrace.h"
#include "util.h"
#include "mmheap.h"
#include "mmpage.h"
#include "mmgdi.h"
#include "mmhandle.h"
#include "buildin.h"
#include "cpuprof.h"
#include "mmprof.h"

ULONG
BtrRegisterCallback(
	IN PBTR_CALLBACK Callback
	)
{
	ULONG Status;
	PBTR_TRAP_OBJECT Trap;
	BTR_DECODE_CONTEXT Decode;
	PVOID Address;
	LONG_PTR Destine;
	BTR_DECODE_FLAG Type;
	LONG_PTR PatchAddress = 0;

	if (FlagOn(Callback->Flag, CALLBACK_ON)) {
		return S_OK;
	}

	Address = Callback->Address;
	if (!Address) {
		return BTR_E_INVALID_ARGUMENT;
	}

	//
	// Ensure it's executable address
	//

	if (BtrIsExecutableAddress(Address) != TRUE) {
		return BTR_E_CANNOT_PROBE;
	}

	//
	// Check whether the first instruction is a jump opcode,
	// return it's destine absolute address. If Destine != NULL,
	// we can simply patch the offset to redirect it to BTR,
	// no instruction copy involved.
	//

	Type = BtrComputeDestineAddress(Address, &Destine, &PatchAddress);

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) { 

		//
		// Decode target routine to check whether it's probable
		//

		RtlZeroMemory(&Decode, sizeof(Decode));
		Decode.RequiredLength = 5;
		
		//
		// Decode use patch address if it's jmp rel8
		//

		if (PatchAddress != (LONG_PTR)Address) {
			Status = BtrDecodeRoutine((PVOID)PatchAddress, &Decode);
		} else {
			Status = BtrDecodeRoutine(Address, &Decode);
		}

		if (Status != S_OK || Decode.ResultLength < Decode.RequiredLength ||
			FlagOn(Decode.DecodeFlag, BTR_FLAG_BRANCH)) {
			return BTR_E_CANNOT_PROBE;
		}
	}

	//
	// Allocate trap object to fuse target and runtime
	//

	Trap = BtrAllocateTrap((ULONG_PTR)Address); 
	if (!Trap) {
		return BTR_E_CANNOT_PROBE;
	}

	RtlCopyMemory(Trap, &BtrTrapTemplate, sizeof(BTR_TRAP_OBJECT));
	Trap->TrapFlag = Type | BTR_FLAG_CALLBACK;

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrap((PVOID)PatchAddress, &Decode, Trap);
		} else {
			BtrFuseTrap(Address, &Decode, Trap);
		}

	} else {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrapLite((PVOID)PatchAddress, (PVOID)Destine, Trap);
		} else {
			BtrFuseTrapLite(Address, (PVOID)Destine, Trap);
		}
	}

	Trap->Callback = Callback->Callback;
	Callback->PatchAddress = (PVOID)PatchAddress;
	Callback->Trap = Trap;

	//
	// Create hotpatch entry to be committed by control end
	//

	Status = BtrApplyCallback(Callback);
	return Status;
}

ULONG
BtrApplyCallback(
	IN PBTR_CALLBACK Callback 
	)
{
	ULONG Protect;
	PBTR_TRAP_OBJECT Trap;

	if (!Callback->Address || !Callback->Trap) {
		return S_OK;
	}

	if (!VirtualProtect(Callback->PatchAddress, BtrPageSize, PAGE_EXECUTE_READWRITE, &Protect)) {
		return BTR_E_ACCESSDENIED;
	}

	Trap = Callback->Trap;
	ASSERT(Trap != NULL);

	//
	// Connect to trap object via a jmp rel32
	//

	*(PUCHAR)Callback->PatchAddress = 0xe9;
	*(PLONG)((PUCHAR)Callback->PatchAddress + 1) = (LONG)((LONG_PTR)&Trap->Jmp[0] - ((LONG_PTR)Callback->PatchAddress + 5));

#if defined (_M_IX86)
	Trap->Procedure = &Trap->Callback;

#elif defined (_M_X64)

	Trap->Procedure = Trap->Callback;

	//
	// AMD64 use RIP for call/jmp qword[xxx], so RIP is 0
	//

	Trap->Rip = 0;

#endif

	if (Trap->HijackedLength > 5) {
		RtlFillMemory((PUCHAR)Callback->PatchAddress + 5, Trap->HijackedLength - 5, 0xcc);
	}

	SetFlag(Callback->Flag, CALLBACK_ON);

	VirtualProtect(Callback->PatchAddress, BtrPageSize, Protect, &Protect);
	return S_OK;
}

ULONG
BtrUnregisterCallback(
	IN PBTR_CALLBACK Callback
	)
{
	ULONG Status;
	ULONG Protect;
	PBTR_TRAP_OBJECT Trap;

	if (!Callback->Address || !Callback->Trap) {
		return S_OK;
	}

	Status = VirtualProtect(Callback->PatchAddress, BtrPageSize, PAGE_EXECUTE_READWRITE, &Protect);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	Trap = Callback->Trap;

	__try {
		BtrCopySmallMemory(Callback->PatchAddress, Trap->OriginalCopy, Trap->HijackedLength);
	}__except(EXCEPTION_EXECUTE_HANDLER) {
		VirtualProtect(Callback->PatchAddress, BtrPageSize, Protect, &Protect);
	}
	
	//
	// N.B. This is to avoid repeatly free callback object
	//

	Callback->Address = NULL;
	Callback->PatchAddress = NULL;

	BtrFreeTrap(Callback->Trap);
	Callback->Trap = NULL;

	ClearFlag(Callback->Flag, CALLBACK_ON);
	SetFlag(Callback->Flag, CALLBACK_OFF);

	return S_OK;
}

PVOID 
BtrGetCallbackDestine(
	IN PBTR_CALLBACK Callback
	)
{
	PBTR_TRAP_OBJECT Trap;

	Trap = Callback->Trap;
	return (PVOID)Trap->HijackedCode;
}

BOOLEAN
BtrIsCallbackComplete(
	IN PBTR_CALLBACK Callback,
	IN ULONG Count
	)
{
	ULONG i;
	ULONG References;

	for(i = 0; i < Count; i++) {
		References = InterlockedCompareExchange(&Callback[i].References, 0, 0);
		if (References != 0) {
			return FALSE;
		}
	}

	return TRUE;
}

ULONG
BtrCreateHotpatch(
	__in PBTR_CALLBACK Callback,
	__in BTR_HOTPATCH_ACTION Action
	)
{
	PBTR_TRAP_OBJECT Trap;

	if (!Callback->Address || !Callback->Trap) {
		return S_FALSE;
	}

	Trap = Callback->Trap;

	//
	// Fill hotpatch entry based on action to be taken
	//

	RtlZeroMemory(&Callback->Hotpatch, sizeof(BTR_HOTPATCH_ENTRY));
	Callback->Hotpatch.Address = Callback->PatchAddress;

	if (Action == HOTPATCH_COMMIT) {

#if defined (_M_IX86)
		Trap->Procedure = &Trap->Callback;

#elif defined (_M_X64)

		Trap->Procedure = Trap->Callback;

		//
		// AMD64 use RIP for call/jmp qword[xxx], so RIP is 0
		//

		Trap->Rip = 0;

#endif

		Callback->Hotpatch.Code[0] = 0xe9;
		*(PULONG)&Callback->Hotpatch.Code[1] = (LONG)((LONG_PTR)&Trap->Jmp[0] - ((LONG_PTR)Callback->PatchAddress + 5));

		if (Trap->HijackedLength > 5) {
			RtlFillMemory(&Callback->Hotpatch.Code[5], Trap->HijackedLength - 5, 0xcc);
		}

	}

	else if (Action == HOTPATCH_DECOMMIT) {

		Callback->Hotpatch.Address = Callback->PatchAddress;
		RtlCopyMemory(&Callback->Hotpatch.Code[0], Trap->OriginalCopy, Trap->HijackedLength);

	}

	Callback->Hotpatch.Length = Trap->HijackedLength;
	return S_OK;
}

ULONG
BtrRegisterCallbackEx(
	__in PBTR_CALLBACK Callback
	)
{
	ULONG Status;
	PBTR_TRAP_OBJECT Trap;
	BTR_DECODE_CONTEXT Decode;
	PVOID Address;
	LONG_PTR Destine;
	BTR_DECODE_FLAG Type;
	LONG_PTR PatchAddress;

	if (FlagOn(Callback->Flag, CALLBACK_ON)) {
		return S_OK;
	}

	Address = Callback->Address;
	if (!Address) {
		return BTR_E_INVALID_ARGUMENT;
	}

	//
	// Ensure it's executable address
	//

	if (BtrIsExecutableAddress(Address) != TRUE) {
		return BTR_E_CANNOT_PROBE;
	}

	//
	// Check whether the first instruction is a jump opcode,
	// return it's destine absolute address. If Destine != NULL,
	// we can simply patch the offset to redirect it to BTR,
	// no instruction copy involved.
	//

	Type = BtrComputeDestineAddress(Address, &Destine, &PatchAddress);

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) { 

		//
		// Decode target routine to check whether it's probable
		//

		RtlZeroMemory(&Decode, sizeof(Decode));
		Decode.RequiredLength = 5;
		
		//
		// Decode use patch address if it's jmp rel8
		//

		if (PatchAddress != (LONG_PTR)Address) {
			Status = BtrDecodeRoutine((PVOID)PatchAddress, &Decode);
		} else {
			Status = BtrDecodeRoutine(Address, &Decode);
		}

		if (Status != S_OK || Decode.ResultLength < Decode.RequiredLength ||
			FlagOn(Decode.DecodeFlag, BTR_FLAG_BRANCH)) {
			DebugTrace("BtrRegisterCallbackEx: %s!%s", Callback->DllName, Callback->ApiName);
			return BTR_E_CANNOT_PROBE;
		}
	}

	//
	// Allocate trap object to fuse target and runtime
	//

	Trap = BtrAllocateTrap((ULONG_PTR)Address); 
	if (!Trap) {
		return BTR_E_CANNOT_PROBE;
	}

	RtlCopyMemory(Trap, &BtrTrapTemplate, sizeof(BTR_TRAP_OBJECT));
	Trap->TrapFlag = Type | BTR_FLAG_CALLBACK;

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrap((PVOID)PatchAddress, &Decode, Trap);
		} else {
			BtrFuseTrap(Address, &Decode, Trap);
		}

	} else {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrapLite((PVOID)PatchAddress, (PVOID)Destine, Trap);
		} else {
			BtrFuseTrapLite(Address, (PVOID)Destine, Trap);
		}
	}

	Trap->Callback = Callback->Callback;
	Callback->PatchAddress = (PVOID)PatchAddress;
	Callback->Trap = Trap;

	//
	// Create hotpatch entry to be committed by control end
	//

	Status = BtrCreateHotpatch(Callback, HOTPATCH_COMMIT);
	return Status;
}

ULONG
BtrUnregisterCallbackEx(
	__in PBTR_CALLBACK Callback
	)
{
	return BtrCreateHotpatch(Callback, HOTPATCH_DECOMMIT);
}