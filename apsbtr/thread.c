//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "thread.h"
#include "heap.h"
#include "util.h"
#include "hal.h"
#include "trap.h"
#include "cache.h"
#include "global.h"
#include "btr.h"
#include "message.h"

LIST_ENTRY BtrThreadObjectList;
BTR_LOCK BtrThreadListLock;

LIST_ENTRY BtrThreadPageList;
BTR_LOCK BtrThreadPageLock;

PBTR_THREAD_OBJECT BtrNullThread;

DECLSPEC_CACHEALIGN
ULONG BtrDllReferences;

ULONG 
BtrInitializeThread(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	BtrInitLock(&BtrThreadListLock);
	InitializeListHead(&BtrThreadObjectList);
	
	//
	// Paged thread allocation
	//

	BtrInitLock(&BtrThreadPageLock);
	InitializeListHead(&BtrThreadPageList);

	//
	// Fill system dlls address
	//

	BtrInitSystemDllAddress();

	//
	// Initialize the shared thread object, it's used for threads without a
	// proper thread object, all threads attached to shared thread object
	// are exempted from probes.
	//
	
	BtrNullThread = BtrAllocateThread(NullThread);
	if (!BtrNullThread) {
		return S_FALSE;
	}

	return S_OK;
}

VOID
BtrUninitializeThread(
	VOID
	)
{
	PBTR_THREAD_OBJECT Thread;
	PLIST_ENTRY ListEntry;

	__try {

		while (IsListEmpty(&BtrThreadObjectList) != TRUE) {

			ListEntry = RemoveHeadList(&BtrThreadObjectList);
			Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD_OBJECT, ListEntry);

			if (BtrProfileObject->Attribute.Mode == RECORD_MODE) {
				BtrCloseMappingPerThread(Thread);
			}

			if (Thread->RundownHandle != NULL) {

				if (BtrIsThreadTerminated(Thread) != TRUE) {
					BtrClearThreadStack(Thread);
				} 

				CloseHandle(Thread->RundownHandle);
			} 

			if (Thread->Buffer != NULL) {
				VirtualFree(Thread->Buffer, 0, MEM_RELEASE);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}

	BtrFreeThreadPageList();
}

PBTR_TEB 
BtrGetCurrentTeb(
	VOID
	)
{
	return (PBTR_TEB)NtCurrentTeb();
}

PBTR_THREAD_OBJECT
BtrAllocateThread(
	IN BTR_THREAD_TYPE Type	
	)
{
	ULONG Status;
	PBTR_THREAD_OBJECT Thread;

	Thread = (PBTR_THREAD_OBJECT)BtrAllocateThreadEx();
	if (!Thread) {
		return NULL;
	}

	RtlZeroMemory(Thread, sizeof(BTR_THREAD_OBJECT));
	Thread->Type = Type;
	Thread->ThreadId = BtrCurrentThreadId();
	Thread->StackBase = BtrGetCurrentStackBase();
	InitializeListHead(&Thread->FrameList);
	
	if (Type == NullThread || Type == SystemThread) {
		Thread->ThreadFlag = BTR_FLAG_EXEMPTION;
		return Thread;
	}

	//
	// N.B. The following is for normal thread can be probed as usual
	//

	Status = S_OK;

	Thread->RundownHandle = OpenThread(THREAD_QUERY_INFORMATION|SYNCHRONIZE, 
		                              FALSE, Thread->ThreadId);
	if (!Thread->RundownHandle) {

		Status = S_FALSE;

	} else {

		//
		// Create per thread file and mapping objects
		//

		if (BtrProfileObject->Attribute.Mode == RECORD_MODE) {
			Status = BtrCreateMappingPerThread(Thread);
		}
	}

	//
	// Even it failed, we still make a live thread object
	// attach to current thread and set its exemption flag
	//

	if (Status != S_OK) {
		if (Thread->RundownHandle) {
			CloseHandle(Thread->RundownHandle);
		}
		Thread->ThreadFlag = BTR_FLAG_EXEMPTION;
	} 

	else {
	
		if (BtrProfileObject->Attribute.Mode == RECORD_MODE) {
			Thread->Buffer = VirtualAlloc(NULL, BtrPageSize, MEM_COMMIT, PAGE_READWRITE);
			Thread->BufferBusy = FALSE;
		}
	}

	//
	// Insert thread object into global thread list.
	//

	BtrAcquireLock(&BtrThreadListLock);
	InsertHeadList(&BtrThreadObjectList, &Thread->ListEntry);
	BtrReleaseLock(&BtrThreadListLock);

	return Thread;
}

PBTR_THREAD_OBJECT
BtrGetCurrentThread(
	VOID
	)
{
	PBTR_THREAD_OBJECT Thread;

	//
	// N.B. When enter callbacks, thread's Pc has three possible cases:
	// 1, Pc falls into runtime code region
	// 2, Pc falls into other modules, runtime issue API call to that module
	// 3, Pc falls into trap page
	//
	// for case 1, we can determinte it from runtime dll base and size
	// for case 2, we can determine from BTR_INFORMATION.DllRefereences
	// for case 3, we can determine from BTR_INFORMATION.TrapRanges
	//
	// this is crucial for safe unregister and unloading. 
	//
	// if both dll references and all callbacks' references are 0, it's safe to unloading.
	// for safe unregister, only whether thread's Pc falls into jmp instruction
	// range of its function body, which is much more simple.
	//

	BtrReferenceDll();

	Thread = BtrGetTlsValue();
	if (Thread != NULL) {
		BtrDereferenceDll();
		return Thread;
	}

	//
	// Temporarily set null thread as current thread object
	// to protect thread allocation process.
	//

	BtrSetTlsValue(BtrNullThread);

	Thread = BtrAllocateThread(NormalThread);
	if (Thread == NULL) {
		BtrDereferenceDll();
		return BtrNullThread;
	}

	BtrSetTlsValue(Thread);
	BtrDereferenceDll();

	return Thread;
}

BOOLEAN
BtrIsExecutingAddress(
	IN PLIST_ENTRY ContextList,
	IN PLIST_ENTRY AddressList
	)
{
	PLIST_ENTRY ContextEntry;
	PLIST_ENTRY AddressEntry;
	PBTR_CONTEXT Context;
	PBTR_ADDRESS_RANGE Range;
	ULONG_PTR ThreadPc;

	//
	// N.B. This routine presume all threads except current thread
	//      are already suspended.
	//

	ASSERT(IsListEmpty(ContextList) != TRUE);
	ASSERT(IsListEmpty(AddressList) != TRUE);

	ContextEntry = ContextList->Flink;
	while (ContextEntry != ContextList) {
		
		Context = CONTAINING_RECORD(ContextEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)
		ThreadPc = (ULONG_PTR)Context->Registers.Eip;
#elif defined (_M_X64)
		ThreadPc = (ULONG_PTR)Context->Registers.Rip;
#endif

		//
		// Check ThreadPc whether fall into specified address range
		//

		AddressEntry = AddressList->Flink;	
		while (AddressEntry != AddressList) {
			Range = CONTAINING_RECORD(AddressEntry, BTR_ADDRESS_RANGE, ListEntry);
			if (ThreadPc >= Range->Address && ThreadPc < (Range->Address + Range->Size)) {
				return TRUE;
			}
			AddressEntry = AddressEntry->Flink;
		}

		ContextEntry = ContextEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
BtrIsExecutingRuntime(
	IN PLIST_ENTRY ContextList 
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;
	ULONG_PTR ThreadPc;

	ListEntry = ContextList->Flink;

	while (ListEntry != ContextList) {
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)
		ThreadPc = Context->Registers.Eip;
#elif defined (_M_X64)
		ThreadPc = Context->Registers.Rip;
#endif

		if (ThreadPc >= BtrDllBase && ThreadPc < (BtrDllBase + BtrDllSize)) {
			return TRUE;
		}

		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
BtrIsExecutingTrap(
	IN PLIST_ENTRY ContextList
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;
	ULONG_PTR ThreadPc;

	ListEntry = ContextList->Flink;

	while (ListEntry != ContextList) {
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)
		ThreadPc = Context->Registers.Eip;
#elif defined (_M_X64)
		ThreadPc = Context->Registers.Rip;
#endif

		if (BtrIsTrapPage((PVOID)ThreadPc)) {
			return TRUE;
		}

		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
BtrIsPendingFrameExist(
	IN PBTR_THREAD_OBJECT Thread
	)
{
	HANDLE ThreadHandle;
	CONTEXT Context;
	ULONG_PTR ThreadPc;
	BOOLEAN Pending = FALSE;

	//
	// N.B. This routine assume target thread is not runtime thread, and it's
	// thread object from global active thread list.
	//

	ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, 
							  FALSE, Thread->ThreadId);
	//
	// N.B. Not sure it's because security issue, or thread terminated,
	// anyway, we can not access this thread, return FALSE
	//

	if (!ThreadHandle) {
		return FALSE;
	}

	while (TRUE) {

		if (SuspendThread(ThreadHandle) == (ULONG)-1) {

			//
			// Thread is probably terminated
			//

			CloseHandle(ThreadHandle);
			return FALSE;
		}

		Context.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(ThreadHandle, &Context)) {

			//
			// Thread is probably terminated, whatever we need
			// add a ResumeThread, it's safe to do so.
			//
				
			ResumeThread(ThreadHandle);
			CloseHandle(ThreadHandle);
			return FALSE;
		}

#if defined (_M_IX86)
		ThreadPc = Context.Eip;
#endif

#if defined (_M_X64)
		ThreadPc = Context.Rip;
#endif

		if (ThreadPc >= BtrDllBase && ThreadPc < (BtrDllBase + BtrDllSize)) {

			//
			// Executing inside runtime, may be operating on frame list, 
			// repeat again to avoid corrupt the thread's frame list
			//

			ResumeThread(ThreadHandle);
			SwitchToThread();

		} else {

			if (IsListEmpty(&Thread->FrameList) != TRUE) {
				Pending = TRUE;
			}

			break;
		}
	}

	ResumeThread(ThreadHandle);
	CloseHandle(ThreadHandle);
	return Pending;
}

BOOLEAN
BtrIsThreadTerminated(
	IN PBTR_THREAD_OBJECT Thread 
	)
{
	ULONG Status;

	//
	// N.B. Wait rundown handle to test whether it's terminated,
	// the rundown handle is opened when first build its thread object,
	// so it's guranteed that it's a valid handle. The handle will be
	// closed when thread object is freed, or runtime unloaded.
	//

	Status = WaitForSingleObject(Thread->RundownHandle, 0);
	if (Status == WAIT_OBJECT_0) {
		return TRUE;
	}

	return FALSE;
}

VOID
BtrClearThreadStack(
	IN PBTR_THREAD_OBJECT Thread
	)
{
	__try {
		*Thread->StackBase = 0;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}
}

VOID
BtrSetTlsValue(
	IN PBTR_THREAD_OBJECT Thread
	)
{
	PULONG_PTR StackBase;

	StackBase = BtrGetCurrentStackBase();
	*(PBTR_THREAD_OBJECT *)StackBase = Thread;
}

PBTR_THREAD_OBJECT
BtrGetTlsValue(
	VOID
	)
{
	PULONG_PTR StackBase;
	PBTR_THREAD_OBJECT Thread;

	StackBase = BtrGetCurrentStackBase();
	Thread = *(PBTR_THREAD_OBJECT *)StackBase;
	return Thread;
}

VOID
BtrInitializeThreadPage(
	IN PBTR_THREAD_PAGE Page 
	)
{
	ULONG Length;
	ULONG Number;
	PVOID Buffer;

	Length = (ULONG)(BtrAllocationGranularity - FIELD_OFFSET(BTR_THREAD_PAGE, Object));
	Number = Length / sizeof(BTR_THREAD_OBJECT);

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
}

PBTR_THREAD_OBJECT
BtrAllocateThreadEx(
	VOID
	)
{
	PBTR_THREAD_PAGE Page;
	ULONG BitNumber;
	PLIST_ENTRY ListEntry;

	BtrAcquireLock(&BtrThreadPageLock);

	ListEntry = BtrThreadPageList.Flink;

	while (ListEntry != &BtrThreadPageList) {
		Page = CONTAINING_RECORD(ListEntry, BTR_THREAD_PAGE, ListEntry);	
		BitNumber = BtrFindFirstClearBit(&Page->BitMap, 0);
		if (BitNumber != (ULONG)-1) {
			RtlZeroMemory(&Page->Object[BitNumber], sizeof(BTR_THREAD_OBJECT));
			BtrSetBit(&Page->BitMap, BitNumber);
			BtrReleaseLock(&BtrThreadPageLock);
			return &Page->Object[BitNumber];
		}
		ListEntry = ListEntry->Flink;	
	}

	BtrReleaseLock(&BtrThreadPageLock);

	Page = (PBTR_THREAD_PAGE)VirtualAlloc(NULL, BtrAllocationGranularity, 
										  MEM_COMMIT, PAGE_READWRITE);
	if (!Page) {
		return NULL;
	}

	//
	// Allocate from first object slot
	//

	BtrInitializeThreadPage(Page);
	RtlZeroMemory(&Page->Object[0], sizeof(BTR_THREAD_OBJECT));
	BtrSetBit(&Page->BitMap, 0);
	
	BtrAcquireLock(&BtrThreadPageLock);
	InsertHeadList(&BtrThreadPageList, &Page->ListEntry);
	BtrReleaseLock(&BtrThreadPageLock);

	return &Page->Object[0];
}

VOID
BtrFreeThread(
	IN PBTR_THREAD_OBJECT Thread
	)
{
	PBTR_THREAD_PAGE Page;
	ULONG Distance;
	ULONG BitNumber;

	Page = (PBTR_THREAD_PAGE)BtrUlongPtrRoundDown((ULONG_PTR)Thread, 
		                                          BtrAllocationGranularity);

	Distance = (ULONG)((ULONG_PTR)Thread - (ULONG_PTR)&Page->Object[0]);
	BitNumber = Distance / sizeof(BTR_THREAD_OBJECT);

	BtrAcquireLock(&BtrThreadPageLock);

	BtrClearBit(&Page->BitMap, BitNumber);
	RtlZeroMemory(Thread, sizeof(BTR_THREAD_OBJECT));

	BtrReleaseLock(&BtrThreadPageLock);
}

VOID
BtrFreeThreadPageList(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_THREAD_PAGE Page;

	__try {

		BtrAcquireLock(&BtrThreadPageLock);

		while (IsListEmpty(&BtrThreadPageList) != TRUE) {

			ListEntry = RemoveHeadList(&BtrThreadPageList);
			Page = CONTAINING_RECORD(ListEntry, BTR_THREAD_PAGE, ListEntry);

			BtrFree(Page->BitMap.Buffer);
			VirtualFree(Page, 0, MEM_RELEASE);
		}

		BtrReleaseLock(&BtrThreadPageLock);
		
		BtrDeleteLock(&BtrThreadPageLock);
		BtrDeleteLock(&BtrThreadListLock);

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}

BOOLEAN
BtrIsRuntimeThread(
	IN ULONG ThreadId
	)
{
	BOOLEAN Status;

	//
	// If profile object is not ready, just fake current thread as
	// runtime thread to skip all profiling
	//

	if (!BtrProfileObject) {
		return TRUE;
	}

	if (ThreadId == BtrMessageThreadId) {
		return TRUE;
	}

	ASSERT(BtrProfileObject->IsRuntimeThread != NULL);
	Status = (*BtrProfileObject->IsRuntimeThread)(BtrProfileObject, 
		                                          ThreadId);
	return Status;
}

#if defined (_M_IX86)

PVOID __declspec(naked)
BtrGetFramePointer(
	VOID
	)
{
	__asm { 
		mov eax, ebp
		ret
	}
}

#elif defined(_M_X64)

PVOID
BtrGetFramePointer(
	VOID
	)
{
	return NULL;
}

#endif

VOID
BtrTrapProcedure(
	VOID
	)
{
	ASSERT(0);
}