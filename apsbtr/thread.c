//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "thread.h"
#include "heap.h"
#include "util.h"
#include "hal.h"
#include "trap.h"
#include "cache.h"
#include "btr.h"
#include "message.h"
#include <tlhelp32.h>

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
	InitializeListHead(&Thread->ApcList);
	
	BtrInitSpinLock(&Thread->IrpLock, 100);
	InitializeListHead(&Thread->IrpList);
	InitializeSListHead(&Thread->IrpLookaside);

	InitializeListHead(&Thread->StackPageRetireList);
	InitializeListHead(&Thread->TracePageRetireList);

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
	
		if (BtrProfileObject->Attribute.Mode == RECORD_MODE||
			BtrProfileObject->Attribute.Type == PROFILE_IO_TYPE||
			BtrProfileObject->Attribute.Type == PROFILE_CCR_TYPE ) {
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
BtrClearThreadStackByTeb(
	IN PTEB Teb 
	)
{
	__try {
		*((PULONG_PTR)Teb->Tib.StackBase - 1) = 0;
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
	PULONG Buffer;

	Length = (ULONG)(BtrAllocationGranularity - FIELD_OFFSET(BTR_THREAD_PAGE, Object));
	Number = Length / sizeof(BTR_THREAD_OBJECT);

	//
	// Initialize bitmap object which track usage of trap object
	// allocation, it's aligned with 32 bits.
	//

	Length = (ULONG)BtrUlongPtrRoundUp((ULONG_PTR)Number, 32) / 8;
	Buffer = (PULONG)BtrMalloc(Length);
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

ULONG
BtrSuspendProcess(
	_Out_ PLIST_ENTRY SuspendList,
	_Out_ PULONG Count
	)
{
	ULONG Status;
	ULONG CurrentProcessId;
	ULONG CurrentThreadId;
	HANDLE Toolhelp;
	HANDLE ThreadHandle;
	THREADENTRY32 ThreadEntry;
	PBTR_SUSPENDEE Thread;
	HANDLE Current;
	int Priority;

	Current = GetCurrentThread();
	Priority = GetThreadPriority(Current);
	SetThreadPriority(Current, THREAD_PRIORITY_HIGHEST);

	CurrentProcessId = GetCurrentProcessId();
	CurrentThreadId = GetCurrentThreadId();

    Toolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, CurrentProcessId);
	if (Toolhelp == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	ThreadEntry.dwSize = sizeof(ThreadEntry);
    Status = Thread32First(Toolhelp, &ThreadEntry);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	Status = S_OK;
	InitializeListHead(SuspendList);
	*Count = 0;

	do {

		//
		// Enumerate all threads of current process except current runtime thread
		//

		if (ThreadEntry.th32OwnerProcessID == CurrentProcessId && 
			ThreadEntry.th32ThreadID != CurrentThreadId && 
			BtrIsRuntimeThread(ThreadEntry.th32ThreadID) != TRUE ) {

				ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadEntry.th32ThreadID);
				if (ThreadHandle != NULL) {
					Status = SuspendThread(ThreadHandle);
					if (Status != -1) {

						Thread = (PBTR_SUSPENDEE)BtrMalloc(sizeof(BTR_SUSPENDEE));
						Thread->ThreadId = ThreadEntry.th32ThreadID;
						Thread->ThreadHandle = ThreadHandle;
						Thread->SuspendCount = 1;
						Thread->Context = NULL;
						Thread->Object = NULL;
						Thread->Teb = (PTEB)BtrQueryTebAddress(Thread->ThreadHandle);
						ASSERT(Thread->Teb != NULL);

						InsertTailList(SuspendList, &Thread->ListEntry);
						*Count += 1;

					} else {

						//
						// N.B. if failed to suspend thread, we have nothing to do but
						// close its handle, and ignore, this should not happen, if 
						// it did happen, we can do nothing.
						//

						CloseHandle(ThreadHandle);
					}
				} 
		} 

    } while (Thread32Next(Toolhelp, &ThreadEntry));
 
	CloseHandle(Toolhelp);
	SetThreadPriority(Current, Priority);
	return S_OK;
}

VOID
BtrResumeProcess(
	_Inout_ PLIST_ENTRY SuspendList,
	_In_ BOOLEAN Clean
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_SUSPENDEE Suspendee;
	ULONG i;

	while (!IsListEmpty(SuspendList)) {

		ListEntry = RemoveHeadList(SuspendList);
		Suspendee = CONTAINING_RECORD(ListEntry, BTR_SUSPENDEE, ListEntry);

		ASSERT(Suspendee->SuspendCount != 0);
		ASSERT(Suspendee->ThreadHandle != NULL);

		for(i = 0; i < Suspendee->SuspendCount; i++) {
			ResumeThread(Suspendee->ThreadHandle);
		}

		CloseHandle(Suspendee->ThreadHandle);

		if (Suspendee->Context) {
			BtrFree(Suspendee->Context);
		}

		if (Clean){
			BtrClearThreadStackByTeb(Suspendee->Teb);
		}

		BtrFree(Suspendee);
	}
}

VOID
BtrWalkSuspendeeStack(
	_In_ PLIST_ENTRY SuspendeeList
	)
{
	PLIST_ENTRY ListEntry;
	PCONTEXT Context;
	PBTR_SUSPENDEE Suspendee;
	PTEB Teb;
	ULONG_PTR StackBase;
	ULONG_PTR StackLimit;
	ULONG Hash;

	ListEntry = SuspendeeList->Flink;
	while (ListEntry != SuspendeeList) {

		Suspendee = CONTAINING_RECORD(ListEntry, BTR_SUSPENDEE, ListEntry);
		Context = (PCONTEXT)BtrMalloc(sizeof(CONTEXT));
		Context->ContextFlags = CONTEXT_FULL;
retry:
		if (!GetThreadContext(Suspendee->ThreadHandle, Context)){

			//
			// If the thread was terminated, unchain from suspendee list, chain into
			// retired list.
			//

			if (WAIT_OBJECT_0 == WaitForSingleObject(Suspendee->ThreadHandle, 0)) {
				ListEntry = ListEntry->Flink;
				RemoveEntryList(&Suspendee->ListEntry);
				BtrFree(Context);
				BtrFree(Suspendee);
				continue;
			}
			else {

				//
				// N.B. Because thread suspension is an ASYNCHRONOUS operation, it may
				// fail due to its suspension APC is not yet completed in kernel mode,
				// we have to wait it to complete. If we're in very bad lucky, the thread
				// get stuck in kernel, we're done...
				//

				//SwitchToThread();
				Sleep(1000);
				goto retry;
			}
		}

		//
		// N.B. Thread's StackLimit can dynamically change, we must
		// fetch its current value after suspend it.
		//

		Teb = Suspendee->Teb;
		ASSERT(Teb != NULL);

		StackBase = (ULONG_PTR)((PULONG_PTR)Teb->Tib.StackBase - 1);
		StackLimit = (ULONG_PTR)Teb->Tib.StackLimit;

		//
		// N.B. RtlVirtualUnwind may modify the CONTEXT structure, so we need
		// record the target RIP in Callers[0].
		//
		
#if defined(_M_IX86)
				Suspendee->Frame[0] = (PVOID)(ULONG_PTR)Context->Eip;
#elif defined(_M_X64)
				Suspendee->Frame[0] = (PVOID)(ULONG_PTR)Context->Rip;
#endif
		Suspendee->Depth = BtrRemoteWalkStack(Suspendee->ThreadId, StackBase, StackLimit,
								Context, MAX_STACK_DEPTH - 1, &Suspendee->Frame[1], &Hash);

		ListEntry = ListEntry->Flink;
	}
}

BOOLEAN
BtrScanFrameFromSuspendeeList(
	_In_ PLIST_ENTRY SuspendeeList
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_SUSPENDEE Suspendee;
	ULONG i;

	ASSERT(IsListEmpty(SuspendeeList) != TRUE);

	//
	// Walk the suspended thread list, if there's any frame pointer fall into
	// runtime image range, return TRUE, this indicate it's not safe to unload
	// from host process.
	//

	ListEntry = SuspendeeList->Flink;
	while (ListEntry != SuspendeeList) {
		Suspendee = CONTAINING_RECORD(ListEntry, BTR_SUSPENDEE, ListEntry);
		for(i = 0; i < Suspendee->Depth; i++) {
			if ((ULONG_PTR)Suspendee->Frame[i] >= BtrDllBase && 
				(ULONG_PTR)Suspendee->Frame[i] < (BtrDllBase + BtrDllSize)) {
				return TRUE;
			}
		}
		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}