//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "trap.h"
#include "util.h"
#include "thread.h"
#include "heap.h"
#include "hal.h"
#include "user.h"
#include "queue.h"
#include "stktrace.h"

#define STACK_ARGUMENT_LIMIT  16
#define RIP_RANGE (ULONG_PTR)0x80000000

BTR_TRAP BtrTrapTemplate = {
	0x0,                                        // trap flag
	0x0,                                        // probe object
	0x0,                                        // trap procedure
	0x0,                                        // backward address
	0x0,										// hijacked length
	{0xcc }, 								    // hijacked code (32 bytes)
	0x55,                                       // push ebp
	0x68,                                       // push probe 
	0x0,                                        // probe object address
	0x60,                                       // pushad
	0x9c,                                       // pushfd
	0x54,                                       // push esp
	0x15ff,                                     // call dword ptr[m32] 
	0x0,                                        // address of trap procedure field
	{ 0x83, 0xc4, 0x04 },                       // add esp, 4
	0x9D,                                       // popfd
	0x61,                                       // popad
	0x5c,                                       // pop esp
	0xc3,                                       // ret
};

//
// Trap page allocation list
//

LIST_ENTRY BtrTrapPageList;

VOID
BtrInitializeTrapPage(
	IN PBTR_TRAP_PAGE Page 
	)
{
	ULONG Length;
	ULONG Number;
	PVOID Buffer;

	Length = (ULONG)(BtrAllocationGranularity - FIELD_OFFSET(BTR_TRAP_PAGE, Object));
	Number = Length / sizeof(BTR_TRAP);

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

VOID
BtrReleaseTrapPageList(
	IN PLIST_ENTRY ListHead	
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_TRAP_PAGE Page;

	while (IsListEmpty(ListHead) != TRUE) {
	
		ListEntry = RemoveHeadList(ListHead);
		Page = CONTAINING_RECORD(ListEntry, BTR_TRAP_PAGE, ListEntry);

		BtrFree(Page->BitMap.Buffer);
		VirtualFree(Page, 0, MEM_RELEASE);
	}

	return;
}

PBTR_TRAP
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
			RtlZeroMemory(&Page->Object[BitNumber], sizeof(BTR_TRAP));
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

	RtlZeroMemory(&Page->Object[0], sizeof(BTR_TRAP));
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

ULONG
BtrFuseTrap(
	IN PVOID Address, 
	IN PBTR_DECODE_CONTEXT Decode,
	IN PBTR_TRAP Trap
	)
{
	PUCHAR Buffer;

	//
	// Fill the trap object with trap template, and copy hijacked
	// code to back up buffer
	//

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

	Trap->TrapProcedureRip = PtrToUlong(&Trap->TrapProcedure);
	Trap->TrapProcedure = &BtrTrapProcedure;

	return S_OK;
}


ULONG
BtrFuseTrapLite(
	IN PVOID Address, 
	IN PVOID Destine,
	IN PBTR_TRAP Trap
	)
{
	PUCHAR Buffer;

	ASSERT(!FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_UNSUPPORTED));

	switch (Trap->TrapFlag) {

		case BTR_FLAG_JMP_RM32:
			Trap->HijackedLength = 6;
			break;

		case BTR_FLAG_JMP_REL32:
			Trap->HijackedLength = 5;
			break;

		default:
			ASSERT(0);
	}

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

	Trap->TrapProcedureRip = PtrToUlong(&Trap->TrapProcedure);
	Trap->TrapProcedure = &BtrTrapProcedure;

	return S_OK;
}

BOOLEAN
BtrGetFpuReturn(
	OUT double *Result
	)
{
	BOOLEAN Status = FALSE;

	__asm {
		fstsw   ax
        and     ax, 3800h		// check TOP bits of FPU state word
        cmp     ax, 0			// If non-zero, require to save FPU return 
        jz      NoFpuReturn 
		mov     ecx, dword ptr[Result]
        fstp    qword ptr [ecx]
		mov     byte ptr[Status], 1
	}

NoFpuReturn:
	return Status;
}

VOID
BtrRestoreFpuReturn(
	IN double Result
	)
{
	__asm {
		fld qword ptr[Result]
	}
}

ULONG
BtrExecuteRoutine(
	IN PBTR_TRAPFRAME TrapFrame,
	IN PBTR_THREAD Thread,
	IN PBTR_PROBE Probe,
	IN BOOLEAN ExecuteTarget,
	IN PBTR_REGISTER Register,
	OUT PULONG_PTR EspDelta,
	OUT PBTR_RECORD_HEADER *Record
	)
{
	PULONG_PTR StackLimit;
	PULONG_PTR StackFrame;
	ULONG_PTR CookieBefore;
	ULONG_PTR CookieAfter;
	PULONG_PTR CallerEsp;
	BTR_EXECUTE_ROUTINE Routine;
	ULONG_PTR Return;
	PBTR_TRAP Trap;
	BTR_ACTIVEFRAME ActiveFrame;
	ULONG_PTR ArgumentCount;
	ULONG_PTR i;
	double FpuReturn;
	BOOLEAN IsFpuReturn = FALSE;
	BOOLEAN ContextSaved = TRUE;
	LARGE_INTEGER Time;
	LARGE_INTEGER EnterTime;
	PBTR_CALLFRAME Frame;
	ULONG_PTR SecurityCookie;

	//
	// N.B. _alloca allocate stack space below the saved registers,
	// so it's safe to tune the ESP without corruptting the registers.
	//

	SecurityCookie = 0xBB40E64E;

	if (ExecuteTarget) {
		Trap = Probe->Trap;
		Routine = (BTR_EXECUTE_ROUTINE)Trap->HijackedCode;
	} else {
		ASSERT(Record != NULL);
		Routine = (BTR_EXECUTE_ROUTINE)Probe->FilterCallback;
	}
	
	//
	// Esp/Rsp include caller's return address, we need skip
	// the caller's return address to locate to the first stack
	// parameter
	//

	CallerEsp = (PULONG_PTR)UlongToPtr(Register->Esp + sizeof(ULONG_PTR));

	StackLimit = HalGetCurrentStackBase();
	ArgumentCount = (StackLimit - CallerEsp) / sizeof(ULONG_PTR);
	if (ArgumentCount > STACK_ARGUMENT_LIMIT) {
		ArgumentCount = STACK_ARGUMENT_LIMIT;
	}
	
	//
	// Allocate shadown stack space, it's always a fixed space, however,
	// valid parameter count is calculated to avoid crash because some
	// early stage call have very few stack space, e.g. some thread start
	// routine, we need take care to not read beyond its stack limits,
	// see above code.
	//

	Frame = (PBTR_CALLFRAME)_alloca(sizeof(BTR_CALLFRAME));

	//
	// Push current execution frame to virtual stack
	//

	ActiveFrame.TrapFrame = TrapFrame;
	ActiveFrame.CallFrame = Frame;
	BtrPushActiveFrame(Thread, &ActiveFrame);

	for(i = 0; i < ArgumentCount; i++) {
		Frame->Argument[i] = (ULONG)CallerEsp[i];
	}

	for ( ; i < STACK_ARGUMENT_LIMIT; i++) {
		Frame->Argument[i] = 0;
	}

	StackFrame = BtrGetStackFrame();
	CookieBefore = (ULONG_PTR)((ULONG_PTR)StackFrame ^ SecurityCookie);
	Frame->Cookie = (ULONG)CookieBefore;

	//
	// Filter execution requires to put probe object on current shadown stack
	//

	Frame->Record = 0;
	Frame->LastError = 0;

	if (ExecuteTarget != TRUE) {
		Frame->Probe = Probe;
	}
	else {
		
		if (Record != NULL) { 
			
			//
			// Get PerfMon counter here for user probe, we only call this api
			// when record ptr is available, because BtrCaptureArgument can
			// fail, and pass us a null ptr, so we need check this carefully.
			//

			QueryPerformanceCounter(&EnterTime);
		}
	}

	//
	// N.B. BtrGetResults MUST be immediately follow the call to routine,
	// because we need immediately save the register values after the call.
	//

	__try {

		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

		Return = (*Routine)(Register->Ecx, Register->Edx);
		BtrGetResults(Frame);
		
		if (!ExecuteTarget) {
			if (!FlagOn(Frame->Flags, BTR_FLAG_CONTEXT_SET)) {
				ContextSaved = FALSE;
			}
		}

		if (Probe->ReturnType == ReturnFloat || Probe->ReturnType == ReturnDouble) {
			IsFpuReturn = BtrGetFpuReturn(&FpuReturn);
		}

		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	}
	__finally {

		//
		// This is where to pop up the frame either when exception occured,
		// or under the condition of normal execution flow
		//
	
		BtrPopActiveFrame(Thread, &ActiveFrame);
	}

	//
	// Check the stack frame pointer, ensure it's not corrupted
	//

	CookieAfter = Frame->Cookie ^ SecurityCookie;
	StackFrame = BtrGetStackFrame();
	if (CookieAfter != (ULONG_PTR)StackFrame) {
		DebugBreak();
	}

	//
	// Ensure the stack pointer is appropriately balanced after the call,
	// AMD64 is same as __cdecl, caller clean up stack, so the EspDelta
	// should be always 0
	//

	*EspDelta = Frame->EspValue - (ULONG_PTR)Frame;

	if (*EspDelta < 0) {
		DebugBreak();
	}

	Register->Eax = (ULONG)Frame->EaxValue;
	Register->Edx = (ULONG)Frame->EdxValue;

	//
	// Only filter execution require to get the record pointer
	//

	if (ExecuteTarget != TRUE) {
		*Record = (PBTR_RECORD_HEADER)Frame->Record;
	} 

	if (*Record != NULL) {

		if (ContextSaved) { 

			(*Record)->LastError = (ULONG)Frame->LastError;
			(*Record)->Duration = Frame->Time.LowPart;

		} else {

			//
			// 2 cases here, 1, it's a user probe. 2, for a filter probe,
			// filter callback did not call BtrFltSetContext, we need save
			// the last error status here, note that the last error
			// may already be dirty.
			//

			(*Record)->LastError = GetLastError();
			QueryPerformanceCounter(&Time);

			if (ExecuteTarget) {
				Time.QuadPart -= EnterTime.QuadPart;
			} else {
				Time.QuadPart -= Frame->Time.QuadPart;
			}

			(*Record)->Duration = Time.LowPart;
		}

		(*Record)->ReturnType = Probe->ReturnType;

		if (IsFpuReturn) {
			(*Record)->FpuReturn = FpuReturn;
			SetFlag((*Record)->RecordFlag, BTR_FLAG_FPU_RETURN);
		} 
		
		//
		// N.B. Special case for return 64 bits integer.
		//

		else if (Probe->ReturnType == ReturnUInt64 || Probe->ReturnType == ReturnInt64) {
			(*Record)->Return64 = ((ULONG64)Register->Eax) | (((ULONG64)Register->Edx) << 32);
		}

		else {
			(*Record)->Return32 = Register->Eax;
		}
	}

	return S_OK;
}

PBTR_RECORD_HEADER
BtrCaptureUserRecord(
	IN PBTR_TRAPFRAME Frame,
	IN PBTR_REGISTER Registers,
	IN PBTR_THREAD Thread,
	OUT PULONG_PTR EspDelta
	)
{
	ULONG Status;
	PBTR_PROBE Object;
	PBTR_USER_RECORD Record;
	SYSTEMTIME SystemTime;
	FILETIME FileTime;
	Object = Frame->Object;
	Record = NULL;

	//
	// Get record time stamp
	//

	GetLocalTime(&SystemTime);
	SystemTimeToFileTime(&SystemTime, &FileTime);

	//
	// Capture arguments and allocate record
	//

	Status = BtrCaptureArgument(Object, Registers, &Record);
	if (Status != S_OK) {

		//
		// N.B. Even if argument capture failed, we still need call
		// the target routine to get esp delta, and the target call
		// may succeed anyway.
		//

		BtrExecuteRoutine(Frame, Thread, Object, TRUE, Registers, EspDelta, NULL);
		return NULL;
	}

	BtrExecuteRoutine(Frame, Thread, Object, TRUE, Registers, EspDelta, (PBTR_RECORD_HEADER *)&Record);

	Record->Version = (ULONG)Object->Version;
	Record->ArgumentCount = Object->ArgumentCount;
	Record->Base.FileTime.dwLowDateTime = FileTime.dwLowDateTime;
	Record->Base.FileTime.dwHighDateTime = FileTime.dwHighDateTime;
	Record->Base.StackHash = 0;

	return &Record->Base;	
}

PBTR_RECORD_HEADER
BtrCaptureFilterRecord(
	IN PBTR_TRAPFRAME Frame,
	IN PBTR_REGISTER Registers,
	IN PBTR_THREAD Thread,
	OUT PULONG_PTR EspDelta
	)
{
	PBTR_PROBE Object;
	PBTR_FILTER_RECORD Record;
	SYSTEMTIME SystemTime;
	FILETIME FileTime;

	Object = Frame->Object;
	Record = NULL;

	//
	// Get record time stamp
	//

	GetLocalTime(&SystemTime);
	SystemTimeToFileTime(&SystemTime, &FileTime);

	BtrExecuteRoutine(Frame, Thread, Object, FALSE, Registers, EspDelta, (PBTR_RECORD_HEADER *)&Record);

	if (!Record) {
		return NULL;
	}

	Record->Base.Address = Object->Address;
	Record->Base.ProcessId = GetCurrentProcessId();
	Record->Base.ThreadId = Thread->ThreadId;
	Record->Base.FileTime.dwLowDateTime = FileTime.dwLowDateTime;
	Record->Base.FileTime.dwHighDateTime = FileTime.dwHighDateTime;
	Record->Base.StackHash = 0;

	return &Record->Base;
}

VOID 
BtrTrapProcedure(
	IN PBTR_TRAPFRAME Frame 
	)
{
	PBTR_THREAD Thread;
	PBTR_PROBE Object;
	PULONG_PTR AdjustedEsp;
	BTR_REGISTER Registers;
	ULONG_PTR EspDelta;
	PBTR_TRAP Trap;
	PBTR_RECORD_HEADER Record;
	ULONG_PTR CallerReturn;
	ULONG Status;
	BOOLEAN RequireExemption; 
	double FpuReturn;
	ULONG Depth, Hash;

	ASSERT(Frame->Object != NULL);

	Object = Frame->Object;
	Trap = Object->Trap;

	//
	// If btr traps itself, no probing
	//

	CallerReturn = *(PULONG_PTR)(Frame + 1);
	if (BtrIsExemptedAddress((PVOID)CallerReturn)) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->ExemptionReturn;
		Frame->ExemptionReturn = (ULONG_PTR)Trap->HijackedCode;
		return;
	}
	
	//
	// If current thread is runtime thread, no probing
	//

	if (BtrIsRuntimeThread(HalCurrentThreadId())) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->ExemptionReturn;
		Frame->ExemptionReturn = (ULONG_PTR)Trap->HijackedCode;
		return;
	}

	//
	// If we're in process of allocate thread object, no probing
	//

	if (BtrIsExemptionCookieSet(TRAP_EXEMPTION_COOKIE)) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->ExemptionReturn;
		Frame->ExemptionReturn = (ULONG_PTR)Trap->HijackedCode;
		return;
	}

	//
	// If btr is unloading, or null activation context pointer, no probing
	//

	if (BtrIsUnloading() || !HalIsAcspValid()) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->ExemptionReturn;
		Frame->ExemptionReturn = (ULONG_PTR)Trap->HijackedCode;
		return;
	}
	
	//
	// If thread object is marked as exempted, no probing
	//

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);

	if (FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->ExemptionReturn;
		Frame->ExemptionReturn = (ULONG_PTR)Trap->HijackedCode;
		return;
	}

	//
	// Mark current thread's probe exemption flag and increase reference count
	//

	RequireExemption = FALSE;
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	_InterlockedIncrement(&Object->References);

	//
	// Clone saved registers
	//

	RtlCopyMemory(&Registers, &Frame->Registers, sizeof(Registers));

	//
	// N.B. Esp must be adjusted to track original esp when entering target routine
	// without our trampoline
	//

	Registers.Esp = (ULONG)PtrToUlong(Frame + 1);

	EspDelta = 0;
	Record = NULL;

	{
		PVOID Callers[MAX_STACK_DEPTH];

		if (FlagOn(Object->Flags, BTR_FLAG_STACKTRACE)) {

			PBTR_STACKTRACE_ENTRY StackTrace;

			BtrCaptureStackTrace(Registers.Ebp, Object->Address, 
				MAX_STACK_DEPTH, &Depth, &Callers[0], &Hash);

			//
			// First lookup whether it's already at stack trace database
			//

			StackTrace = BtrLookupStackTrace(Object->Address, Hash, Depth);
			if (!StackTrace) {

				//
				// It's a new stack trace, insert it
				//

				StackTrace = (PBTR_STACKTRACE_ENTRY)BtrAllocateStackTraceLookaside();
				StackTrace->Probe = Object->Address;
				StackTrace->Hash = Hash;
				StackTrace->Depth = (USHORT)Depth;
				RtlCopyMemory(StackTrace->Frame, Callers, sizeof(PVOID) * Depth);
				BtrQueueStackTrace(StackTrace);
			}
		}
	}

	//
	// under any case, we should get a record here, else the sequence number will
	// be broken here, it's impossible to fix any subsequent record's sequence number.
	// for exception case, we can allocate a dummy record and report the exception.
	//

	if (FlagOn(Object->Flags, BTR_FLAG_USER)) {
		Record = BtrCaptureUserRecord(Frame, &Registers, Thread, &EspDelta);
	}  
	else if (FlagOn(Object->Flags, BTR_FLAG_FILTER)) {
		Record = BtrCaptureFilterRecord(Frame, &Registers, Thread, &EspDelta);
	} 
	else {
		ASSERT(0);	
	}

	if (Record != NULL) {

		Record->Caller = (PVOID)CallerReturn;
		Record->StackDepth = Depth;
		Record->StackHash = Hash;
		
		if (FlagOn(Record->RecordFlag, BTR_FLAG_FPU_RETURN)) {
			FpuReturn = Record->FpuReturn;
		}

		//
		// Write record to shared cache
		//

		Status = BtrQueueSharedCacheRecord(Thread, Record, Object);
		if (Status != S_OK) {
			RequireExemption = TRUE;
		}

		if ((PBTR_RECORD_HEADER)Thread->Buffer == Record) {

			Thread->BufferBusy = FALSE;			
			InterlockedIncrement(&BtrFltTotalFrees);
			InterlockedIncrement(&BtrFltTotalThreadFrees);

		} else {

			BtrFreeLookaside(RecordHeap, Record);
			InterlockedIncrement(&BtrFltTotalFrees);
		}
	}

	//
	// N.B. Even if there's no record available, it's required that we
	// update the result registers, because the target routine was already
	// executed, we need adjust esp and execute caller's return address
	//

	if ((LONG)EspDelta < 0) {
		DebugBreak();
	}

	CallerReturn = *(PULONG_PTR)(Frame + 1);
	AdjustedEsp = (PULONG_PTR)(Frame + 1);
	AdjustedEsp = (PULONG_PTR)((ULONG_PTR)AdjustedEsp + EspDelta);

	//
	// Make POP ESP/RSP to update ESP/RSP to point to adjusted ESP/RSP 
	//

	*(PULONG_PTR)&Frame->Object = (ULONG_PTR)AdjustedEsp; 
	*AdjustedEsp = CallerReturn;

	Frame->Registers.Eax = Registers.Eax;
	Frame->Registers.Edx = Registers.Edx;

	//
	// Restore FPU return, this is only for x86.
	//

	if (FlagOn(Record->RecordFlag, BTR_FLAG_FPU_RETURN)) {
		BtrRestoreFpuReturn(FpuReturn);
	}

	//
	// Clear current thread's probe exemption flag and decrease reference count
	//

	InterlockedDecrement(&Object->References);

	if (!RequireExemption) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}

	return;
}

VOID
BtrFreeTrap(
	IN PBTR_TRAP Trap
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
	BitNumber = Distance / sizeof(BTR_TRAP);

	BtrClearBit(&Page->BitMap, BitNumber);
	RtlZeroMemory(Trap, sizeof(BTR_TRAP));
}

VOID
BtrPushActiveFrame(
	IN PBTR_THREAD Thread,
	IN PBTR_ACTIVEFRAME Frame
	)
{
	ASSERT(Thread != NULL);

	if (IsListEmpty(&Thread->FrameList)) {
		Frame->CurrentDepth = 0;

	} else {

		PBTR_ACTIVEFRAME TopFrame;

		TopFrame = CONTAINING_RECORD(Thread->FrameList.Flink, BTR_ACTIVEFRAME, ListEntry);
		Frame->CurrentDepth = TopFrame->CurrentDepth + 1;
	}

	InterlockedIncrement(&Thread->FrameDepth);
	InsertHeadList(&Thread->FrameList, &Frame->ListEntry);
}

VOID
BtrPopActiveFrame(
	IN PBTR_THREAD Thread,
	IN PBTR_ACTIVEFRAME Frame
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_ACTIVEFRAME TopFrame;
	PBTR_CALLFRAME CallFrame;

	ASSERT(Thread != NULL);	

	ListEntry = RemoveHeadList(&Thread->FrameList);
	TopFrame = CONTAINING_RECORD(ListEntry, BTR_ACTIVEFRAME, ListEntry);

	if (TopFrame != Frame) {
		
		CallFrame = TopFrame->CallFrame;
		if (CallFrame->Record) {

			//
			// Don't know whether the thread flag is corrupted by
			// filters, we use BtrFltFreeRecord instead of BtrFree
			//

			BtrFltFreeRecord((PVOID)CallFrame->Record);
		}

		//
		// Stack was unwinded by exception handling process, adjust
		// the active frame list to its appropriate position
		//

		while (IsListEmpty(&Thread->FrameList) != TRUE) {
			
			ListEntry = RemoveHeadList(&Thread->FrameList);
			TopFrame = CONTAINING_RECORD(ListEntry, BTR_ACTIVEFRAME, ListEntry);

			InterlockedDecrement(&Thread->FrameDepth);

			if (TopFrame == Frame) {
				CallFrame = TopFrame->CallFrame;
				if (CallFrame->Record) {
					((PBTR_RECORD_HEADER)CallFrame->Record)->CallDepth = TopFrame->CurrentDepth;
				}
				return;
			}
			else {
				CallFrame = TopFrame->CallFrame;
				if (CallFrame->Record) {
					BtrFltFreeRecord((PVOID)CallFrame->Record);
				}
			}
		}	

		//
		// N.B. This indicates there's stack corruption
		//

		ASSERT(0);
	} 

	else {
	
		CallFrame = TopFrame->CallFrame;
		if (CallFrame->Record != 0) {
			((PBTR_RECORD_HEADER)CallFrame->Record)->CallDepth = TopFrame->CurrentDepth;
		}

		InterlockedDecrement(&Thread->FrameDepth);
	}
	
}

ULONG
BtrInitializeTrap(
	VOID
	)
{
	InitializeListHead(&BtrTrapPageList);
	return S_OK;
}

VOID
BtrSetExemptionCookie(
	IN ULONG_PTR Cookie,
	OUT PULONG_PTR Value
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	*Value = *StackBase;
	*StackBase = Cookie;
}

VOID
BtrClearExemptionCookie(
	IN ULONG_PTR Value 
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	*StackBase = Value;
}

BOOLEAN
BtrIsExemptionCookieSet(
	IN ULONG_PTR Cookie
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	return (*StackBase == Cookie) ? TRUE : FALSE;
}