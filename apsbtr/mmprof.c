//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//


#include "apsbtr.h"
#include <psapi.h>
#include "mmprof.h"
#include "mmheap.h"
#include "mmpage.h"
#include "mmgdi.h"
#include "mmhandle.h"
#include "buildin.h"
#include "lock.h"
#include "heap.h"
#include "btr.h"
#include "util.h"
#include "cache.h"
#include "stream.h"
#include "marker.h"

//
// N.B. MmSkipAllocators is used to when profile session
// is to be stopped, we first skip allocators, however, continue
// capture free records, this will help to largely reduce noisy
// data, we pause for sometime to let resource freed as many as possible.
//

DECLSPEC_CACHEALIGN
ULONG MmSkipAllocators = 0;

//
// Default skip duration set to be 2 seconds.
//

DECLSPEC_CACHEALIGN
ULONG MmSkipDuration = 1000 * 2;

DECLSPEC_CACHEALIGN
MM_HEAPLIST_TABLE MmHeapListTable;

DECLSPEC_CACHEALIGN
MM_PAGE_TABLE MmPageTable;

DECLSPEC_CACHEALIGN
MM_HANDLE_TABLE MmHandleTable;

DECLSPEC_CACHEALIGN
MM_GDI_TABLE MmGdiTable;

DECLSPEC_CACHEALIGN
BTR_PC_TABLE MmPcTable;

HANDLE MmThreadHandle;
ULONG MmThreadId;

#define HEAPLIST_BUCKET(_V) \
	((ULONG_PTR)_V % MAX_HEAPLIST_BUCKET)

#define HEAP_BUCKET(_V) \
	((ULONG_PTR)_V % MAX_HEAP_BUCKET)

#define HANDLE_BUCKET(_V) \
	((ULONG_PTR)_V % MAX_HANDLE_BUCKET)

#define PAGE_BUCKET(_V) \
	((ULONG_PTR)_V % MAX_PAGE_BUCKET)

#define GDI_BUCKET(_V) \
	((ULONG_PTR)_V % MAX_GDI_BUCKET)

#define PC_BUCKET(_V) \
	((ULONG_PTR)_V % STACK_PC_BUCKET)

//
// Callback to monitor process termination
//

BTR_CALLBACK MmCallback[] = {
	{  MmCallbackType, 0,0,0,0, MmExitProcessCallback, "kernel32.dll", "ExitProcess", 0 },
};

ULONG MmCallbackCount = ARRAYSIZE(MmCallback);

HANDLE MmProcessHeap;
HANDLE MmMsvcrtHeap;
HANDLE MmBstrHeap;

ULONG
MmGetHeapHandles(
	VOID
	)
{
	HMODULE Handle;
	GETHEAPHANDLE GetHeapHandlePtr;

	//
	// GlobalAlloc/LocalAlloc allocate from process heap
	//

	MmProcessHeap = GetProcessHeap();

	//
	// Track only msvcrt.dll's heap, other version crt go to Rtl routines,
	// we can classify crt heaps via "dll!_crtheap" symbol after profiling.
	//

	Handle = LoadLibraryA("msvcrt.dll");
	if (!Handle) {
		return BTR_E_LOADLIBRARY;
	}

	GetHeapHandlePtr = (GETHEAPHANDLE)GetProcAddress(Handle, "_get_heap_handle");
	ASSERT(GetHeapHandlePtr != NULL);

	if (!GetHeapHandlePtr) {
		return BTR_E_GETPROCADDRESS;
	}

	MmMsvcrtHeap = (HANDLE)(*GetHeapHandlePtr)();

	//
	// BSTR cache or not cached use process heap, however, we separate it
	// from process heap, use a psuedo handle.
	//

	MmBstrHeap = (HANDLE)((ULONG_PTR)-2);

	return S_OK;
}

PMM_HEAP_TABLE
MmInsertHeapTable(
	IN HANDLE HeapHandle
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMM_HEAP_TABLE Table;
	ULONG Number;
	PBTR_SPIN_HASH Entry;

	Bucket = HEAPLIST_BUCKET(HeapHandle);
	Entry = &MmHeapListTable.Active[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Table = CONTAINING_RECORD(ListEntry, MM_HEAP_TABLE, ListEntry);	
		if (Table->HeapHandle == HeapHandle) {
			BtrReleaseSpinLock(&Entry->SpinLock);
			return Table;
		}
		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);

	//
	// Allocate new heap table object
	//

	Table = (PMM_HEAP_TABLE)BtrMalloc(sizeof(MM_HEAP_TABLE));
	RtlZeroMemory(Table, sizeof(MM_HEAP_TABLE));
	Table->HeapHandle = HeapHandle;

	for(Number = 0; Number < MAX_HEAP_BUCKET; Number += 1) {
		BtrInitSpinLock(&Table->ListHead[Number].SpinLock, 100);
		InitializeListHead(&Table->ListHead[Number].ListHead);
	}

	//
	// Insert heap list table
	//

	BtrAcquireSpinLock(&Entry->SpinLock);
	ListHead = &Entry->ListHead;
	InsertHeadList(ListHead, &Table->ListEntry);
	BtrReleaseSpinLock(&Entry->SpinLock);

	InterlockedIncrement(&MmHeapListTable.NumberOfActiveHeaps);
	InterlockedIncrement(&MmHeapListTable.NumberOfHeaps);
	return Table;
	
}

PMM_HEAP_TABLE
MmLookupHeapTable(
	IN HANDLE HeapHandle
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMM_HEAP_TABLE Table;
	PBTR_SPIN_HASH Entry;

	Bucket = HEAPLIST_BUCKET(HeapHandle);
	Entry = &MmHeapListTable.Active[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Table = CONTAINING_RECORD(ListEntry, MM_HEAP_TABLE, ListEntry);	
		if (Table->HeapHandle == HeapHandle) {
			BtrReleaseSpinLock(&Entry->SpinLock);
			return Table;
		}
		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
	return NULL;
}

PPF_HEAP_RECORD
MmLookupHeapRecord(
	IN PMM_HEAP_TABLE Table,
	IN PVOID Address
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Entry;
	PPF_HEAP_RECORD Record;

	Bucket = HEAP_BUCKET(Address);
	Entry = &Table->ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Record = CONTAINING_RECORD(ListEntry, PF_HEAP_RECORD, ListEntry);	
		if (Record->Address == Address) {
			BtrReleaseSpinLock(&Entry->SpinLock);
			return Record;
		}
		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
	return NULL;
}

PMM_HEAP_TABLE
MmInsertHeapRecord(
	IN HANDLE HeapHandle,
	IN PVOID Address,
	IN ULONG Size,
	IN ULONG Duration,
	IN ULONG StackHash,
	IN USHORT StackDepth,
	IN USHORT CallbackType
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PMM_HEAP_TABLE Table;
	PPF_HEAP_RECORD Record;

	Table = MmInsertHeapTable(HeapHandle);
	ASSERT(Table != NULL);

	if (!Table) {
		return NULL;
	}

	Record = (PPF_HEAP_RECORD)BtrMallocLookaside(LOOKASIDE_HEAP);
	Record->Address = Address;
	Record->Size = Size;
	Record->OldAddr = NULL;
	Record->Duration = Duration;
	Record->StackHash = StackHash;
	Record->StackDepth = StackDepth;
	Record->CallbackType = CallbackType;
	Record->HeapHandle = HeapHandle;

	Bucket = HEAP_BUCKET(Record->Address);
	Entry = &Table->ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);
	InsertHeadList(&Entry->ListHead, &Record->ListEntry);
	BtrReleaseSpinLock(&Entry->SpinLock);

	InterlockedIncrement(&Table->NumberOfAllocs); 
	Table->SizeOfAllocs += Size;
	
	InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfHeapAllocs);
	return Table;
}

BOOLEAN
MmRemoveHeapRecord(
	IN HANDLE HeapHandle,
	IN PVOID Address
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Entry;
	PPF_HEAP_RECORD Record;
	PMM_HEAP_TABLE Table;
	ULONG Size;

	Table = MmLookupHeapTable(HeapHandle);
	if (!Table) {

		//
		// N.B. This indicates that we never track this heap's allocation,
		// so we just ignore free from this heap, this is different from
		// MmInsertHeapRecord, which call MmInsertHeapTable, if the table
		// is not available, a new heap table will be inserted into heap list.
		//

		return FALSE;
	}

	Bucket = HEAP_BUCKET(Address);
	Entry = &Table->ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Record = CONTAINING_RECORD(ListEntry, PF_HEAP_RECORD, ListEntry);	
		ASSERT(Record->HeapHandle == HeapHandle);

		if (Record->Address == Address) {

			Size = Record->Size;
			RemoveEntryList(&Record->ListEntry);
			BtrReleaseSpinLock(&Entry->SpinLock);

			BtrFreeLookaside(LOOKASIDE_HEAP, Record);

			InterlockedIncrement(&Table->NumberOfFrees);
			Table->SizeOfFrees += Size;
			
			InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfHeapFrees);
			return TRUE;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
	return FALSE;
}

VOID
MmRetireHeapTable(
	IN HANDLE HeapHandle
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMM_HEAP_TABLE Table;
	PBTR_SPIN_HASH Entry;

	Bucket = HEAPLIST_BUCKET(HeapHandle);
	Entry = &MmHeapListTable.Active[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Table = CONTAINING_RECORD(ListEntry, MM_HEAP_TABLE, ListEntry);	
		if (Table->HeapHandle == HeapHandle) {

			//
			// Remove the heap table from active list
			//

			RemoveEntryList(&Table->ListEntry);
			BtrReleaseSpinLock(&Entry->SpinLock);

			//
			// Insert the heap table into retire list
			//

			BtrAcquireSpinLock(&MmHeapListTable.Retire.SpinLock);
			InsertHeadList(&MmHeapListTable.Retire.ListHead, &Table->ListEntry);
			BtrReleaseSpinLock(&MmHeapListTable.Retire.SpinLock);

			//
			// Decrease only tracked heap number, we never decrease heap number,
			// which represent all heaps we have ever tracked.
			//

			InterlockedDecrement(&MmHeapListTable.NumberOfActiveHeaps);
			return;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
}

PPF_PAGE_RECORD
MmLookupPageRecord(
	IN PVOID Address
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Entry;
	PPF_PAGE_RECORD Record;

	Bucket = PAGE_BUCKET(Address);
	Entry = &MmPageTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Record = CONTAINING_RECORD(ListEntry, PF_PAGE_RECORD, ListEntry);	
		if (Record->Address == Address) {
			BtrReleaseSpinLock(&Entry->SpinLock);
			return Record;
		}
		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
	return NULL;
}

VOID
MmInsertPageRecord(
	IN PVOID Address,
	IN ULONG Size,
	IN ULONG Duration,
	IN ULONG StackHash,
	IN USHORT StackDepth,
	IN USHORT CallbackType
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_PAGE_RECORD Record;

	Record = (PPF_PAGE_RECORD)BtrMallocLookaside(LOOKASIDE_PAGE);
	Record->Address = Address;
	Record->Size = Size;
	Record->Duration = Duration;
	Record->StackHash = StackHash;
	Record->StackDepth = StackDepth;
	Record->CallbackType = CallbackType;

	Bucket = PAGE_BUCKET(Record->Address);
	Entry = &MmPageTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);
	InsertHeadList(&Entry->ListHead, &Record->ListEntry);
	BtrReleaseSpinLock(&Entry->SpinLock);

	InterlockedIncrement(&MmPageTable.NumberOfAllocs); 
	MmPageTable.SizeOfAllocs += Size;
	
	InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfPageAllocs);
}

VOID
MmRemovePageRecord(
	IN PVOID Address
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_SPIN_HASH Entry;
	PPF_PAGE_RECORD Record;
	ULONG Size;

	Bucket = PAGE_BUCKET(Address);
	Entry = &MmPageTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Record = CONTAINING_RECORD(ListEntry, PF_PAGE_RECORD, ListEntry);	

		if (Record->Address == Address) {

			Size = Record->Size;
			RemoveEntryList(&Record->ListEntry);
			BtrReleaseSpinLock(&Entry->SpinLock);

			BtrFreeLookaside(LOOKASIDE_PAGE, Record);
			InterlockedIncrement(&MmPageTable.NumberOfFrees);
			MmPageTable.SizeOfFrees += Size;
			
			InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfPageFrees);
			return;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
}

VOID
MmInsertHandleRecord(
	IN HANDLE Handle,
	IN HANDLE_TYPE Type,
	IN ULONG Duration,
	IN ULONG StackHash,
	IN USHORT StackDepth,
	IN USHORT CallbackType
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_HANDLE_RECORD Record;
	PPF_TYPE_ENTRY TypeEntry;

	Record = (PPF_HANDLE_RECORD)BtrMallocLookaside(LOOKASIDE_HANDLE);
	Record->Value = Handle;
	Record->Type = Type;
	Record->Duration = Duration;
	Record->StackHash = StackHash;
	Record->StackDepth = StackDepth;
	Record->CallbackType = CallbackType;

	ASSERT(Type < HANDLE_LAST);

	TypeEntry = &MmHandleTable.Types[Type];
	InterlockedIncrement(&TypeEntry->Allocs);

	Bucket = HANDLE_BUCKET(Handle);
	Entry = &MmHandleTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);
	InsertHeadList(&Entry->ListHead, &Record->ListEntry);
	BtrReleaseSpinLock(&Entry->SpinLock);

	InterlockedIncrement(&MmHandleTable.NumberOfAllocs); 
	InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfHandleAllocs);
}

VOID
MmRemoveHandleRecord(
	IN HANDLE Handle
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_HANDLE_RECORD Record;
	PPF_TYPE_ENTRY TypeEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	HANDLE_TYPE Type;

	Bucket = HANDLE_BUCKET(Handle);
	Entry = &MmHandleTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Record = CONTAINING_RECORD(ListEntry, PF_HANDLE_RECORD, ListEntry);	
		if (Record->Value == Handle) {

			Type = Record->Type;
			RemoveEntryList(&Record->ListEntry);
			BtrReleaseSpinLock(&Entry->SpinLock);

			BtrFreeLookaside(LOOKASIDE_HANDLE, Record);
			InterlockedIncrement(&MmHandleTable.NumberOfFrees); 
			
			TypeEntry = &MmHandleTable.Types[Type];
			InterlockedIncrement(&TypeEntry->Frees);

			InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfHandleFrees);
			return;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
}

PPF_HANDLE_RECORD
MmLookupHandleRecord(
	IN HANDLE Handle
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_HANDLE_RECORD Record;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	Bucket = HANDLE_BUCKET(Handle);
	Entry = &MmHandleTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Record = CONTAINING_RECORD(ListEntry, PF_HANDLE_RECORD, ListEntry);	
		if (Record->Value == Handle) {
			BtrReleaseSpinLock(&Entry->SpinLock);
			return Record;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
	return NULL;
}

VOID
MmInsertGdiRecord(
	IN HANDLE Handle,
	IN ULONG Type,
	IN ULONG Duration,
	IN ULONG StackHash,
	IN USHORT StackDepth,
	IN USHORT CallbackType
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_GDI_RECORD Record;
	PPF_TYPE_ENTRY TypeEntry;

	Record = (PPF_GDI_RECORD)BtrMallocLookaside(LOOKASIDE_GDI);
	Record->Value = Handle;
	Record->Type = Type;
	Record->Duration = Duration;
	Record->StackHash = StackHash;
	Record->StackDepth = StackDepth;
	Record->CallbackType = CallbackType;

	ASSERT(Type < GDI_OBJ_LAST);

	TypeEntry = &MmGdiTable.Types[Type];
	InterlockedIncrement(&TypeEntry->Allocs);

	Bucket = GDI_BUCKET(Handle);
	Entry = &MmGdiTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);
	InsertHeadList(&Entry->ListHead, &Record->ListEntry);
	BtrReleaseSpinLock(&Entry->SpinLock);

	InterlockedIncrement(&MmGdiTable.NumberOfAllocs); 
	InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfGdiAllocs);
}

VOID
MmRemoveGdiRecord(
	IN HANDLE Handle
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_GDI_RECORD Record;
	PPF_TYPE_ENTRY TypeEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	HANDLE_TYPE Type;

	Bucket = GDI_BUCKET(Handle);
	Entry = &MmGdiTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Record = CONTAINING_RECORD(ListEntry, PF_GDI_RECORD, ListEntry);	
		if (Record->Value == Handle) {

			Type = Record->Type;
			RemoveEntryList(&Record->ListEntry);
			BtrReleaseSpinLock(&Entry->SpinLock);

			BtrFreeLookaside(LOOKASIDE_GDI, Record);
			InterlockedIncrement(&MmGdiTable.NumberOfFrees); 
			
			TypeEntry = &MmGdiTable.Types[Type];
			InterlockedIncrement(&TypeEntry->Frees);

			InterlockedIncrement(&BtrProfileObject->Attribute.NumberOfGdiFrees);
			return;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
}

PPF_GDI_RECORD
MmLookupGdiRecord(
	IN HANDLE Handle
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH Entry;
	PPF_GDI_RECORD Record;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	Bucket = GDI_BUCKET(Handle);
	Entry = &MmGdiTable.ListHead[Bucket];

	BtrAcquireSpinLock(&Entry->SpinLock);

	ListHead = &Entry->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Record = CONTAINING_RECORD(ListEntry, PF_GDI_RECORD, ListEntry);	
		if (Record->Value == Handle) {
			BtrReleaseSpinLock(&Entry->SpinLock);
			return Record;
		}

		ListEntry = ListEntry->Flink;
	}

	BtrReleaseSpinLock(&Entry->SpinLock);
	return NULL;
}

//
// Flag indicate whether skip allocators when unloading
//

BOOLEAN
MmIsSkipAllocators(
	VOID
	)
{
	BOOLEAN Skip;

	Skip = (BOOLEAN)ReadForWriteAccess(&MmSkipAllocators);
	return Skip;
}

ULONG CALLBACK
MmProfileProcedure(
	__in PVOID Context
	)
{
	ULONG Status;
	HANDLE TimerHandle;
	LARGE_INTEGER DueTime;
	HANDLE Handles[3];
	ULONG_PTR Value;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value); 

	DueTime.QuadPart = -10000 * 1000;
	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	if (!TimerHandle) {
		return S_FALSE;
	}

	//
	// Insert start mark
	//

	BtrInsertMark(0, MARK_START);

	SetWaitableTimer(TimerHandle, &DueTime, 1000, NULL, NULL, FALSE);

	Handles[0] = TimerHandle;
	Handles[1] = BtrProfileObject->StopEvent;
	Handles[2] = BtrProfileObject->ControlEnd;

	__try {

		while (TRUE) {

			Status = WaitForMultipleObjects(3, Handles, FALSE, INFINITE);

			//
			// Timer expired, flush stack record if any
			//

			if (Status == WAIT_OBJECT_0) {
				BtrFlushStackRecord();
				Status = S_OK;
			}

			//
			// Control end issue a stop request
			//

			else if (Status == WAIT_OBJECT_0 + 1) {
				BtrFlushStackRecord();
				Status = S_OK;
				break;
			}

			//
			// Control end exit unexpectedly, we need stop profile,
			// otherwise the process will crash sooner or later,
			// under this circumstance, we get stuck in the host
			// process, we just don't capture any new records, this 
			// at least keep host resource usage stable as possible.
			//

			else if (Status == WAIT_OBJECT_0 + 2) {
				InterlockedExchange(&BtrStopped, (LONG)TRUE);
				Status = S_OK;
				break;
			}
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = BTR_E_EXCEPTION;
	}

	if (Status != S_OK) {
		BtrInsertMark(BtrCurrentSequence(), MARK_ABORT);
	}
	else {
		BtrInsertMark(BtrCurrentSequence(), MARK_STOP);
	}

	CancelWaitableTimer(TimerHandle);
	CloseHandle(TimerHandle);
	return S_OK;
}

BOOLEAN
MmDllCanUnload(
	VOID
	)
{
	ULONG i;
	PBTR_CALLBACK Callback;

	//
	// This routine is called with all other threads suspended
	//

	if (BtrProfileObject->Attribute.EnableHeap) {

		for(i = 0; i < MmHeapCallbackCount; i++) {
			Callback = &MmHeapCallback[i];
			if (Callback->References != 0) {
				return FALSE;
			}
		}

	}

	if (BtrProfileObject->Attribute.EnablePage) {

		for(i = 0; i < MmPageCallbackCount; i++) {
			Callback = &MmPageCallback[i];
			if (Callback->References != 0) {
				return FALSE;
			}
		}

	}

	if (BtrProfileObject->Attribute.EnableHandle) {

		for(i = 0; i < MmHandleCallbackCount; i++) {
			Callback = &MmHandleCallback[i];
			if (Callback->References != 0) {
				return FALSE;
			}
		}

	}

	if (BtrProfileObject->Attribute.EnableGdi) {

		for(i = 0; i < MmGdiCallbackCount; i++) {
			Callback = &MmGdiCallback[i];
			if (Callback->References != 0) {
				return FALSE;
			}
		}

	}

	//
	// Also check MmExitProcessCallback references
	//

	for(i = 0; i < MmCallbackCount; i++) {
		Callback = &MmCallback[i];
		if (Callback->References != 0) {
			return FALSE;
		}
	}

	//
	// Check runtime dll references
	//

	if (BtrDllReferences != 0) {
		return FALSE;
	}

	return TRUE;
}

ULONG
MmValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	ASSERT(Attr->Type == PROFILE_MM_TYPE);

	if (!Attr->EnableHeap && !Attr->EnablePage && 
		!Attr->EnableHandle && !Attr->EnableGdi) {

		return BTR_E_INVALID_PARAMETER;
	}

	return S_OK;
}

ULONG
MmInitialize(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG i;
	ULONG Count;
	HANDLE Heaps[1024];

	BtrInitLock(&MmHeapListTable.Lock);

	//
	// Initialize heap list entries
	//

	for(i = 0; i < MAX_HEAPLIST_BUCKET; i++) {
		BtrInitSpinLock(&MmHeapListTable.Active[i].SpinLock, 100);
		InitializeListHead(&MmHeapListTable.Active[i].ListHead);
	}

	BtrInitSpinLock(&MmHeapListTable.Retire.SpinLock, 100);
	InitializeListHead(&MmHeapListTable.Retire.ListHead);

	//
	// Enumerate available heaps, fill into heap list table
	//

	Count = GetProcessHeaps(1024, Heaps);
	if (!Count) {
		return GetLastError();
	}
	
	for(i = 0; i < Count; i += 1) {
		MmInsertHeapTable(Heaps[i]);
	}

	MmHeapListTable.NumberOfHeaps = Count;
	MmHeapListTable.NumberOfActiveHeaps = Count;

	//
	// Initialize heap handles to be tracked
	//

	MmGetHeapHandles();

	//
	// Initialize page table
	//

	for(i = 0; i < MAX_PAGE_BUCKET; i += 1) {
		BtrInitSpinLock(&MmPageTable.ListHead[i].SpinLock, 100);
		InitializeListHead(&MmPageTable.ListHead[i].ListHead);
	}

	//
	// Initialize handle table
	//

	for(i = 0; i < MAX_HANDLE_BUCKET; i += 1) {
		BtrInitSpinLock(&MmHandleTable.ListHead[i].SpinLock, 100);
		InitializeListHead(&MmHandleTable.ListHead[i].ListHead);
	}

	//
	// Initialize GDI table
	//

	for(i = 0; i < MAX_GDI_BUCKET; i += 1) {
		BtrInitSpinLock(&MmGdiTable.ListHead[i].SpinLock, 100);
		InitializeListHead(&MmGdiTable.ListHead[i].ListHead);
	}

	//
	// Initialize heap caller table, only heap use this table,
	// this is for performance monitoring of heap allocation
	//

	for(i = 0; i < STACK_PC_BUCKET; i++) {
		BtrInitSpinLock(&MmPcTable.Hash[i].SpinLock, 100);
		InitializeListHead(&MmPcTable.Hash[i].ListHead);
	}

	//
	// Fill profile object callbacks
	//

	Object->Start = MmStartProfile;
	Object->Stop = MmStopProfile;
	Object->Pause = MmPauseProfile;
	Object->Resume = MmResumeProfile;
	Object->Unload = MmUnload;
	Object->QueryHotpatch = MmQueryHotpatch;
	Object->ThreadAttach = MmThreadAttach;
	Object->ThreadDetach = MmThreadDetach;
	Object->IsRuntimeThread = MmIsRuntimeThread;
	
	//
	// Initialize mark list
	//

	BtrInitializeMarkList();
	return S_OK;
}

ULONG
MmStartProfile(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	HANDLE StopEvent;

	//
	// Create stop event object
	//

	StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!StopEvent) {
		return GetLastError();
	}

	Object->StopEvent = StopEvent;

	//
	// Create sampling worker thread 
	//

	MmThreadHandle = CreateThread(NULL, 0, MmProfileProcedure, Object, 0, &MmThreadId);
	if (!MmThreadHandle) {
		return GetLastError();
	}

	return S_OK;
}

ULONG
MmStopProfile(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	//
	// Mark skip allocator flag
	//
	
	InterlockedExchange(&MmSkipAllocators, (ULONG)1);
	Sleep(MmSkipDuration);

	InterlockedExchange(&BtrStopped, (LONG)TRUE);

	//
	// Signal stop event and wait for profile thread terminated
	//

	SetEvent(Object->StopEvent);
	WaitForSingleObject(MmThreadHandle, INFINITE);
	CloseHandle(MmThreadHandle);
	MmThreadHandle = NULL;

	return S_OK;
}

ULONG
MmPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return S_OK;
}

ULONG
MmResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return S_OK;
}

ULONG
MmThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return TRUE;
}

ULONG
MmThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	PBTR_THREAD_OBJECT Thread;
	ULONG_PTR Value;
	ULONG ThreadId;

	//
	// N.B. Memory profiling don't use thread management infrastructure
	//

	ThreadId = GetCurrentThreadId();

	if (BtrIsRuntimeThread(ThreadId)) {
		return TRUE;
	}

	if (BtrIsExemptionCookieSet(GOLDEN_RATIO)) {
		return TRUE;
	}

	Thread = (PBTR_THREAD_OBJECT)BtrGetTlsValue();
	if (!Thread) {
		BtrSetExemptionCookie(GOLDEN_RATIO, &Value);
		return TRUE;
	}

	if (Thread->Type == NullThread) {
		BtrSetExemptionCookie(GOLDEN_RATIO, &Value);
		return TRUE;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	__try {

		BtrAcquireLock(&BtrThreadListLock);
		RemoveEntryList(&Thread->ListEntry);
		BtrReleaseLock(&BtrThreadListLock);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		BtrReleaseLock(&BtrThreadListLock);
	}

	if (BtrProfileObject->Attribute.Mode == RECORD_MODE) {
		BtrCloseMappingPerThread(Thread);
	}

	if (Thread->RundownHandle != NULL) {
		CloseHandle(Thread->RundownHandle);
		Thread->RundownHandle = NULL;
	}

	if (Thread->Buffer) {
		VirtualFree(Thread->Buffer, 0, MEM_RELEASE);
	}

	//
	// N.B. Must set exemption cookie first, because BtrFree will trap
	// into runtime, and however, the thread object is being freed!
	//

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value);
	BtrFreeThread(Thread);

	//
	// For normal thread, we always make its thread object live,
	// even it's terminating, because thread shutdown process still
	// can trap into runtime, we just set its thread object as
	// null thread object to exempt all probes.
	//

	return TRUE;
}

ULONG
MmUnload(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	BOOLEAN Unload;

	//
	// Wait until it's safe to unload dll
	//

	while(MmDllCanUnload() != TRUE) {
		Sleep(2000);
	}

	//
	// Close index and data file
	//

	BtrCloseCache();

	//
	// Close stack record file
	//

	BtrCloseStackFile();

	//
	// Write profile report file
	//

	Status = BtrWriteProfileStream(BtrProfileObject->ReportFileObject);
	CloseHandle(BtrProfileObject->ReportFileObject);
	BtrProfileObject->ReportFileObject = NULL;

	CloseHandle(BtrProfileObject->StopEvent);
	BtrProfileObject->StopEvent = NULL;

	CloseHandle(BtrProfileObject->ControlEnd);
	BtrProfileObject->ControlEnd = NULL;

	//
	// This indicate a detach stop, need free up resources
	//

	BtrUninitializeStack();
	BtrUninitializeCache();
	BtrUninitializeTrap();
	BtrUninitializeThread();
	BtrUninitializeHeap();

	if (BtrProfileObject->ExitStatus == BTR_S_USERSTOP) {
		Unload = TRUE;

	} else {

		//
		// ExitProcess is called, we should not free runtime library
		//

		Unload = FALSE;
	}

	//
	// Close shared data object 
	//

	UnmapViewOfFile(BtrSharedData);
	CloseHandle(BtrProfileObject->SharedDataObject);

	BtrUninitializeHal();

	__try {

		//
		// Signal unload event to notify control end that unload is completed
		//

		SetEvent(BtrProfileObject->UnloadEvent);
		CloseHandle(BtrProfileObject->UnloadEvent);

		if (Unload) {
			FreeLibraryAndExitThread((HMODULE)BtrDllBase, 0);
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}

	return S_OK;
}

BOOLEAN
MmIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	)
{
	if (ThreadId == MmThreadId) {
		return TRUE;
	}

	return FALSE;
}

ULONG
MmQueryHotpatch(
	__in PBTR_PROFILE_OBJECT Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	)
{
	ULONG Number;
	PBTR_CALLBACK Callback;
	HMODULE DllHandle;
	ULONG Valid;
	ULONG Status;
	ULONG CallbackCount;
	PBTR_HOTPATCH_ENTRY Hotpatch;

	Valid = 0;

	//
	// N.B. We allocate maximum possible slots here, MmExitProcessCallback is extra
	// callback to monitor process termination
	//

	CallbackCount = MmHeapCallbackCount + MmPageCallbackCount + 
					MmHandleCallbackCount + MmGdiCallbackCount + MmCallbackCount;

	Hotpatch = (PBTR_HOTPATCH_ENTRY)BtrMalloc(sizeof(BTR_HOTPATCH_ENTRY) * CallbackCount);

	if (Object->Attribute.EnableHeap) {

		for(Number = 0; Number < MmHeapCallbackCount; Number += 1) {

			Callback = &MmHeapCallback[Number];
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

			if (Callback->Address != NULL) {

				if (Action == HOTPATCH_COMMIT) {
					Status = BtrRegisterCallbackEx(Callback);
				} else {
					Status = BtrUnregisterCallbackEx(Callback);
				}

				if (Status == S_OK) {
					RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
								  sizeof(BTR_HOTPATCH_ENTRY));
					Valid += 1;
				}
			}
		}
	}

	if (Object->Attribute.EnablePage) {

		for(Number = 0; Number < MmPageCallbackCount; Number += 1) {

			Callback = &MmPageCallback[Number];
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

			if (Callback->Address != NULL) {

				if (Action == HOTPATCH_COMMIT) {
					Status = BtrRegisterCallbackEx(Callback);
				} else {
					Status = BtrUnregisterCallbackEx(Callback);
				}

				if (Status == S_OK) {
					RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
								  sizeof(BTR_HOTPATCH_ENTRY));
					Valid += 1;
				}
			}
		}
	}

	if (Object->Attribute.EnableHandle) {

		for(Number = 0; Number < MmHandleCallbackCount; Number += 1) {

			Callback = &MmHandleCallback[Number];
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

			if (Callback->Address != NULL) {

				if (Action == HOTPATCH_COMMIT) {
					Status = BtrRegisterCallbackEx(Callback);
				} else {
					Status = BtrUnregisterCallbackEx(Callback);
				}

				if (Status == S_OK) {
					RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
								  sizeof(BTR_HOTPATCH_ENTRY));
					Valid += 1;
				}
			}
		}
	}

	if (Object->Attribute.EnableGdi) {

		for(Number = 0; Number < MmGdiCallbackCount; Number += 1) {

			Callback = &MmGdiCallback[Number];
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

			if (Callback->Address != NULL) {

				if (Action == HOTPATCH_COMMIT) {
					Status = BtrRegisterCallbackEx(Callback);
				} else {
					Status = BtrUnregisterCallbackEx(Callback);
				}

				if (Status == S_OK) {
					RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
								  sizeof(BTR_HOTPATCH_ENTRY));
					Valid += 1;
				}
			}
		}
	}

	for(Number = 0; Number < MmCallbackCount; Number += 1) {

		Callback = &MmCallback[Number];
		DllHandle = GetModuleHandleA(Callback->DllName);
		Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

		if (Callback->Address != NULL) {

			if (Action == HOTPATCH_COMMIT) {
				Status = BtrRegisterCallbackEx(Callback);
			} else {
				Status = BtrUnregisterCallbackEx(Callback);
			}

			if (Status == S_OK) {
				RtlCopyMemory(&Hotpatch[Valid], &Callback->Hotpatch, 
					          sizeof(BTR_HOTPATCH_ENTRY));
				Valid += 1;
			}
		}

	}

	*Entry = Hotpatch;
	*Count = Valid;
	return S_OK;
}

VOID WINAPI 
MmExitProcessCallback(
	__in UINT ExitCode
	)
{
	ULONG_PTR Value;
	ULONG Current;
	static ULONG Concurrency = 0;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value);

	Current = InterlockedCompareExchange(&Concurrency, 1, 0);
	if (Current != 0) {

		//
		// Another thread acquires lock to call ExitProcess
		//

		return;
	}
	
	__try {

		BtrProfileObject->ExitStatus = BTR_S_EXITPROCESS;
		SignalObjectAndWait(BtrProfileObject->ExitProcessEvent,
							BtrProfileObject->ExitProcessAckEvent, 
							INFINITE, FALSE);
		//
		// N.B. The following code won't enter runtime anymore since all
		// callbacks are unregistered by control end.
		//

		ExitProcess(ExitCode);

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}