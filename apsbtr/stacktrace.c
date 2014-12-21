//
// lan.john@gmail.com 
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "stacktrace.h"
#include "util.h"
#include "cache.h"
#include "heap.h"
#include "list.h"
#include "hal.h"
#include <windows.h>
#include "trap.h"
#include "status.h"

//
// Global stack entry hash table 
//

DECLSPEC_CACHEALIGN
BTR_STACK_TABLE BtrStackTable;

//
// Global stack record queue
//

DECLSPEC_CACHEALIGN
SLIST_HEADER BtrStackRecordQueue;

//
// Global lookaside list for stack record allocation
//

DECLSPEC_CACHEALIGN
SLIST_HEADER BtrStackRecordLookaside;

//
// Threshold of record allocation lookaside
//

ULONG BtrStackLookasideThreshold = 1024;

//
// Lock object to protect allocation of stack entry pages
//

DECLSPEC_CACHEALIGN
BTR_LOCK BtrStackPageLock;

//
// Current stack page for entry allocation
//

DECLSPEC_CACHEALIGN
PBTR_STACK_PAGE BtrStackPageFree;

//
// Retire list chains all full stack page
//

LIST_ENTRY BtrStackPageRetireList;

//
// Mapping object to support stack record write
//

BTR_MAPPING_OBJECT BtrStackMapping;

//
// Hash a stack hash to its bucket in stack table
//

#define STACK_ENTRY_BUCKET(_H) \
 (_H % STACK_RECORD_BUCKET)

//
// Determine whether a file offset is mapped by mapping object's view
//

#define STACK_IS_MAPPED_OFFSET(_M, _S, _E) \
	(_S >= _M->MappedOffset && _E <= _M->MappedOffset + _M->MappedLength)

//
// Determine whether a file offset is within size of mapping object
//

#define STACK_IS_FILE_OFFSET(_M, _E) \
	(_E <= _M->FileLength)

//
// Determine whether an address falls into range of runtime 
//

#define IS_RUNTIME_DLL(_A) \
	(_A >= BtrDllBase && _A <= BtrDllBase + BtrDllSize)

FORCEINLINE
BOOLEAN
BtrFilterStackTrace(
	__in ULONG_PTR Address
	)
{
	if (IS_RUNTIME_DLL((ULONG_PTR)Address)) {
		return FALSE;
	}

	return TRUE;
}

FORCEINLINE
BOOLEAN
BtrIsSameStackEntry(
	__in PBTR_STACK_ENTRY S0,
	__in PBTR_STACK_ENTRY S1
	)
{
	if (S0->Hash == S1->Hash && S0->Depth == S1->Depth && 
		S0->Frame[0] == S1->Frame[0] && S0->Frame[1] == S1->Frame[1]) {
		return TRUE;
	}

	return FALSE;
}

ULONG
BtrInitializeStack(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	ULONG Number;
	ULONG Size;

	//
	// Initialize stack record queue and its lookaside
	//

	InitializeSListHead(&BtrStackRecordQueue);
	InitializeSListHead(&BtrStackRecordLookaside);

	//
	// Initialize stack page list
	//

	BtrInitLock(&BtrStackPageLock);
	InitializeListHead(&BtrStackPageRetireList);

	BtrStackPageFree = BtrAllocateStackPage();
	if (!BtrStackPageFree) {
		return S_FALSE;
	}

	//
	// Initialize stack entry table
	//

	for(Number = 0; Number < STACK_RECORD_BUCKET; Number += 1) {
		BtrInitSpinLock(&BtrStackTable.Hash[Number].SpinLock, 100);
		InitializeListHead(&BtrStackTable.Hash[Number].ListHead);
	}

	BtrStackTable.Count = 0;

	//
	// Initialize mapping object for stack record file
	//

	Size = GetFileSize(Object->StackFileObject, NULL);
	if (Size < STACK_FILE_INCREMENT) {

		Status = BtrExtendFileLength(Object->StackFileObject, STACK_FILE_INCREMENT);
		if (Status != S_OK) {
			return Status;
		}

		SetFilePointer(Object->StackFileObject, 0, NULL, FILE_BEGIN);
		BtrSharedData->StackFileLength = STACK_FILE_INCREMENT;
	}
	
	BtrSharedData->StackValidLength = 0;

	RtlZeroMemory(&BtrStackMapping, sizeof(BTR_MAPPING_OBJECT));
	BtrStackMapping.Object = CreateFileMapping(Object->StackFileObject, NULL, 
		                                       PAGE_READWRITE, 0, 0, NULL);
	if (!BtrStackMapping.Object) {
		return GetLastError();
	}

	BtrStackMapping.FileLength = BtrSharedData->StackFileLength;
	return S_OK;
}

VOID
BtrUninitializeStack(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_STACK_PAGE Page;

	//
	// Close stack file
	//

	BtrCloseStackFile();

	//
	// N.B. BtrFlushStackRecord() can not be called after this routine
	//

	ListHead = &BtrStackPageRetireList;

	__try {

		while (IsListEmpty(ListHead) != TRUE) {
			ListEntry = RemoveHeadList(ListHead);
			Page = CONTAINING_RECORD(ListEntry, BTR_STACK_PAGE, ListEntry);
			VirtualFree(Page, 0, MEM_RELEASE);
		}

		if (BtrStackPageFree != NULL) {
			VirtualFree(BtrStackPageFree, 0, MEM_RELEASE);
		}
	}

	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	__try {
		BtrDeleteLock(&BtrStackPageLock);
	}

	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

}

VOID
BtrCloseStackFile(
	VOID
	)
{
	LARGE_INTEGER Size;

	//
	// Flush left stack records if any
	//

	BtrFlushStackRecord();

	if (BtrStackMapping.MappedVa != NULL) {
		UnmapViewOfFile(BtrStackMapping.MappedVa);
	}

	if (BtrStackMapping.Object != NULL) {
        __try {

            //
            // N.B. First chance exception occur if debugger is attached,
            // BtrStackMapping.Object is an invalid file mapping object handle,
            // need investigate into this issue.
            //

            CloseHandle(BtrStackMapping.Object);
            BtrStackMapping.Object = NULL;

        }__except(EXCEPTION_EXECUTE_HANDLER) {
            BtrStackMapping.Object = NULL;
        }
	}

	Size.QuadPart = BtrStackTable.Count * sizeof(BTR_STACK_RECORD);

#ifdef _DEBUG
	{
		ULONG64 CurrentLength;
		CurrentLength = (ULONG64)Size.QuadPart;
		ASSERT(CurrentLength == BtrSharedData->StackValidLength);
	}
#endif

	SetFilePointerEx(BtrProfileObject->StackFileObject, Size, NULL, FILE_BEGIN);
	SetEndOfFile(BtrProfileObject->StackFileObject);

	CloseHandle(BtrProfileObject->StackFileObject);
	BtrProfileObject->StackFileObject = NULL;
}

VOID
BtrCaptureStackTrace(
	__in PVOID Frame, 
	__in PVOID Address,
	__out PULONG Hash,
	__out PULONG Depth
	)
{
	PVOID Callers[MAX_STACK_DEPTH];

	//
	// N.B. Only x86 require start frame pointer, x64 has different
	// stack walk algorithm, don't use it.
	//

	BtrWalkCallStack(Frame, Address, MAX_STACK_DEPTH, Depth, &Callers[0], Hash);
}

ULONG
BtrCaptureStackTraceEx(
	__in PBTR_THREAD_OBJECT Thread,
	__in PVOID Callers[],
	__in ULONG MaxDepth,
	__in PVOID Frame, 
	__in PVOID Address,
	__out PULONG Hash,
	__out PULONG Depth
	)
{
	ULONG i;
	ULONG Id;
	PBTR_STACK_RECORD Record;
	PVOID Caller;
	SIZE_T Size;
	BOOLEAN Heap = FALSE;

	Caller = Callers[0];
	Size = (SIZE_T)PtrToUlong(Callers[1]);

	if (PtrToUlong(Callers[2]) == (ULONG)1) {
		Heap = TRUE;
	}

	//
	// N.B. Only x86 require start frame pointer, x64 has different
	// stack walk algorithm, don't use it.
	//

	BtrWalkCallStack(Frame, Address, MaxDepth, Depth, &Callers[0], Hash);

	Record = BtrAllocateStackRecord();
	if (!Record) {
		*Hash = STACK_INVALID_ID;
		return STACK_INVALID_ID;
	}

	//
	// N.B. Must clear Committed bit, since record is allocated from lookaside, 
	// whose state must be dirty currently.
	//

	Record->Committed = 0;
	Record->Heap = Heap;
	Record->StackId = 0;
	Record->Count = 0;
	Record->SizeOfAllocs = (ULONG64)Size;

	if (*Depth >= 2) {

		Record->Depth = *Depth;
		Record->Hash = *Hash;

		for(i = 0; i < *Depth; i++) {
			Record->Frame[i] = Callers[i];
		}

	} else {

		//
		// N.B. Some FPO frame may have only 1 or 2 frames, we need null Frame[1],
		// because we rely on Frame[1] to compare stack entry, the bottom line is
		// Frame[0] tracks callback, Frame[1] tracks callback's caller.
		//

		Record->Depth = 2;
		Record->Frame[0] = Address;
		Record->Frame[1] = Caller;
		Record->Hash = (ULONG)((ULONG_PTR)Record->Frame[0] + (ULONG_PTR)Record->Frame[1]);
	}

	//
	// N.B. Current version assign a stack id to Hash, not a real stack trace hash,
	// this is convenient for fast linear stack record lookup.
	//

	Id = BtrInsertStackEntry(Record);
	*Hash = Id;

	return Id;
}

BOOLEAN
BtrWalkCallStack(
	__in PVOID Fp,
	__in PVOID Address,
	__in ULONG Count,
	__out PULONG CapturedCount,
	__out PVOID  *Frame,
	__out PULONG Hash
	)
{
	ULONG Depth;
	ULONG CapturedDepth;
	ULONG ValidDepth;
	ULONG_PTR Pc;
	PVOID Buffer[MAX_STACK_DEPTH];

	Count = min(Count, MAX_STACK_DEPTH);

#if defined (_M_IX86)
	CapturedDepth = BtrWalkFrameChain((ULONG_PTR)Fp, Count, Buffer, Hash);
#elif defined (_M_X64)
	CapturedDepth = CaptureStackBackTrace(3, Count, Buffer, Hash);
#endif

	if (CapturedDepth == MAX_STACK_DEPTH) {
		CapturedDepth -= 1;
	}
	
	if (CapturedDepth < 3) {
		CapturedDepth = 0; 
	}

	ValidDepth = 1;
	Frame[0] = Address;

	for(Depth = 0; Depth < CapturedDepth; Depth += 1) {
		Pc = (ULONG_PTR)Buffer[Depth];			
		if (BtrFilterStackTrace(Pc)) {
			Frame[ValidDepth] = (PVOID)Pc;
			ValidDepth += 1;
		}
	}

	*CapturedCount = ValidDepth;
	return TRUE;
}

ULONG
BtrWalkFrameChain(
	__in ULONG_PTR Fp,
    __in ULONG Count, 
    __in PVOID *Callers,
	__out PULONG Hash
    )
{
	ULONG_PTR StackHigh;
	ULONG_PTR StackLow;
    ULONG_PTR BackwardFp;
	ULONG_PTR ReturnAddress;
	ULONG Number;

	*Hash = 0;
	StackLow = (ULONG_PTR)&Fp;
	StackHigh = (ULONG_PTR)BtrGetCurrentStackBase();

	for (Number = 0; Number < Count; Number += 1) {

		if (Fp < StackLow || Fp > StackHigh) {
			break;
		}

		BackwardFp = *(PULONG_PTR)Fp;
		ReturnAddress = *((PULONG_PTR)(Fp + sizeof(ULONG_PTR)));

		if (Fp >= BackwardFp || BackwardFp > StackHigh) {
			break;
		}

		if (StackLow < ReturnAddress && ReturnAddress < StackHigh) {
			break;
		}

		Callers[Number] = (PVOID)ReturnAddress;
		*Hash += PtrToUlong(Callers[Number]);
		Fp = BackwardFp;
	}

	return Number;
}

PBTR_STACK_PAGE
BtrAllocateStackPage(
	VOID
	)
{
	PBTR_STACK_PAGE Page;
	SIZE_T Size;

	Size = FIELD_OFFSET(BTR_STACK_PAGE, Buckets[ENTRY_COUNT_PER_STACK_PAGE]);
	Page = (PBTR_STACK_PAGE)VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE);
	if (!Page) {
		return NULL;
	}

	Page->Count = ENTRY_COUNT_PER_STACK_PAGE;
	Page->Next = 0;

	return Page;
}

PBTR_STACK_ENTRY
BtrAllocateStackEntry(
	VOID
	)
{
	PBTR_STACK_PAGE Page;
	PBTR_STACK_ENTRY Entry;

	//
	// If it's out of memory, BtrStackPageFree is NULL, we don't bother
	// to continue
	//

	BtrAcquireLock(&BtrStackPageLock);	

	if (BtrStackPageFree->Next < BtrStackPageFree->Count) {
		Entry = &BtrStackPageFree->Buckets[BtrStackPageFree->Next];
		BtrStackPageFree->Next += 1;

	} else {

		//
		// Current page is full, retire it and allocate new page
		//

		InsertHeadList(&BtrStackPageRetireList, &BtrStackPageFree->ListEntry);

		Page = BtrAllocateStackPage();
		if (Page != NULL) {

			ASSERT(Page->Next == 0);
			Entry = &Page->Buckets[Page->Next];
			Page->Next += 1;
			
			BtrStackPageFree = Page;

		} else {
			Entry = NULL;
		}

	}

	BtrReleaseLock(&BtrStackPageLock);	

	return Entry;
}

ULONG
BtrInsertStackEntry(
	__in PBTR_STACK_RECORD Record
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_STACK_ENTRY Entry;
	PBTR_SPIN_HASH Hash;
	BOOLEAN Found = FALSE;

	Bucket = STACK_ENTRY_BUCKET(Record->Hash);
	Hash = &BtrStackTable.Hash[Bucket];

	BtrAcquireSpinLock(&Hash->SpinLock);
	
	ListHead = &Hash->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, BTR_STACK_ENTRY, ListEntry);
		if (Entry->Hash == Record->Hash && Entry->Depth == Record->Depth &&
			Entry->Frame[0] == Record->Frame[0] && Entry->Frame[1] == Record->Frame[1]) {

			//
			// Increase counters
			//

			Entry->Count += 1;
			Entry->SizeOfAllocs += Record->SizeOfAllocs;
			Found = TRUE;
			break;
		}
		ListEntry = ListEntry->Flink;
	}

	if (Found) {
		BtrReleaseSpinLock(&Hash->SpinLock);
		BtrFreeStackRecord(Record);
		return Entry->StackId;
	}

	Entry = BtrAllocateStackEntry();
	if (!Entry) {
		BtrReleaseSpinLock(&Hash->SpinLock);
		return STACK_INVALID_ID;
	}

	//
	// Insert the new stack entry into bucket
	//

	Entry->Hash = Record->Hash;
	Entry->Depth = Record->Depth;
	Entry->Frame[0] = Record->Frame[0];
	Entry->Frame[1] = Record->Frame[1];
	Entry->Count = 1;
	Entry->SizeOfAllocs = Record->SizeOfAllocs;

	//
	// Generate stack record id
	//

	Entry->StackId = BtrAcquireStackId();
	Record->StackId = Entry->StackId;
	Record->SizeOfAllocs = 0;

	InsertHeadList(ListHead, &Entry->ListEntry);
	BtrReleaseSpinLock(&Hash->SpinLock);

	InterlockedPushEntrySList(&BtrStackRecordQueue, &Record->ListEntry);
	InterlockedIncrement(&BtrStackTable.Count);

	return Entry->StackId;
}

PBTR_STACK_ENTRY 
BtrLookupStackEntry(
	__in PBTR_STACK_ENTRY Lookup 
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_STACK_ENTRY Entry;
	PBTR_SPIN_HASH Hash;
	BOOLEAN Found;

	Bucket = STACK_ENTRY_BUCKET(Lookup->Hash);
	Hash = &BtrStackTable.Hash[Bucket];

	BtrAcquireSpinLock(&Hash->SpinLock);
	
	Found = FALSE;
	ListHead = &Hash->ListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, BTR_STACK_ENTRY, ListEntry);
		if (Entry->Hash == Lookup->Hash && 	Entry->Depth == Lookup->Depth &&
			Entry->Frame[0] == Lookup->Frame[0] && Entry->Frame[1] == Lookup->Frame[1]) {
			Found = TRUE;
			break;
		}
		ListEntry = ListEntry->Flink;
	}
	
	BtrReleaseSpinLock(&Hash->SpinLock);

	if (Found) {
		return Entry;
	}

	return NULL;
}

PBTR_STACK_RECORD
BtrAllocateStackRecord(
	VOID
	)
{
	PBTR_STACK_RECORD Entry;
	PSLIST_ENTRY ListEntry;

	ListEntry = InterlockedPopEntrySList(&BtrStackRecordLookaside);

	if (ListEntry != NULL) {
		Entry = CONTAINING_RECORD(ListEntry, BTR_STACK_RECORD, ListEntry);
	}
	else {
		Entry = (PBTR_STACK_RECORD)BtrAlignedMalloc(sizeof(BTR_STACK_RECORD), 
			                                        MEMORY_ALLOCATION_ALIGNMENT);
	}

	return Entry;
}

VOID
BtrFreeStackRecord(
	__in PBTR_STACK_RECORD Record
	)
{
	USHORT Depth;

	Depth = QueryDepthSList(&BtrStackRecordLookaside);

	if (Depth < BtrStackLookasideThreshold) {
		InterlockedPushEntrySList(&BtrStackRecordLookaside, &Record->ListEntry);
	} 
	else {
		BtrAlignedFree(Record);
	}
}

VOID
BtrFlushStackRecord(
	VOID
	)
{
	PSLIST_ENTRY ListEntry;
	PBTR_STACK_RECORD Record;

	while (ListEntry = InterlockedPopEntrySList(&BtrStackRecordQueue)) {
		Record = CONTAINING_RECORD(ListEntry, BTR_STACK_RECORD, ListEntry);
		Record->ListEntry.Next = NULL;
		BtrWriteStackRecord(Record);
		BtrFreeStackRecord(Record);
	} 
}

ULONG
BtrWriteStackRecord(
	__in PBTR_STACK_RECORD Record
	)
{
	ULONG Status;
	PBTR_MAPPING_OBJECT MappingObject;
	ULONG64 Start;
	ULONG64 End;
	ULONG64 MappedStart;
	ULONG64 MappedEnd;
	PVOID Address;
	ULONG64 NewLength;

	//
	// N.B. Record position is strictly order by its id, this make index
	// unnecessary, since stack record is fixed size, consumer can locate
	// the record by its id directly. the record on disk is valid only if
	// its Committed bit is set.
	//

	Start = Record->StackId * STACK_RECORD_SIZE;

	MappingObject = &BtrStackMapping;
	End = Start + sizeof(BTR_STACK_RECORD); 

	if (STACK_IS_MAPPED_OFFSET(MappingObject, Start, End)) {

		Address = (PUCHAR)MappingObject->MappedVa + (ULONG)(Start - MappingObject->MappedOffset);
		Status = BtrCopyMemory(Address, Record, STACK_RECORD_SIZE);

		if (Status == S_OK) {
			((PBTR_STACK_RECORD)Address)->Committed = 1;
			BtrSharedData->StackValidLength += STACK_RECORD_SIZE;
		}

		return Status;
	}

	if (MappingObject->MappedVa != NULL) {
		FlushViewOfFile(MappingObject->MappedVa, 0);
		UnmapViewOfFile(MappingObject->MappedVa);
		MappingObject->MappedVa = NULL;
		MappingObject->MappedOffset = 0;
		MappingObject->MappedLength = 0;
	}

repeat:

	if (STACK_IS_FILE_OFFSET(MappingObject, End)) {

		//
		// Adjust file mapping view to hold this position,
		// note that we must ensure the mapped end is within
		// current file mapping object's length.
		//

		MappedStart = BtrUlong64RoundDown(Start, BTR_MAP_INCREMENT);
		MappedEnd = BtrUlong64RoundUp(End, BTR_MAP_INCREMENT);
		MappedEnd = min(MappedEnd, MappingObject->FileLength);

		MappingObject->MappedVa = MapViewOfFile(MappingObject->Object, FILE_MAP_READ|FILE_MAP_WRITE, 
			                                    (DWORD)(MappedStart>>32), (DWORD)MappedStart, 
												(SIZE_T)(MappedEnd - MappedStart));
		if (!MappingObject->MappedVa) {
			ASSERT(0);
			return BTR_E_MAPVIEWOFFILE;
		}
		
		MappingObject->MappedOffset = MappedStart;
		MappingObject->MappedLength = (ULONG)(MappedEnd - MappedStart);

		Address = (PUCHAR)MappingObject->MappedVa + (ULONG)(Start - MappedStart);
		Status = BtrCopyMemory(Address, Record, STACK_RECORD_SIZE);
		
		if (Status == S_OK) {
			((PBTR_STACK_RECORD)Address)->Committed = 1;
			BtrSharedData->StackValidLength += STACK_RECORD_SIZE;
		}

		return Status;
	}
	else {
	
		NewLength = BtrSharedData->StackFileLength + STACK_FILE_INCREMENT;
		Status = BtrExtendFileLength(BtrProfileObject->StackFileObject, NewLength);

		if (Status == S_OK) {
			BtrSharedData->StackFileLength = NewLength;
		} else {
			Status = BTR_E_DISK_FULL;
			return Status;
		}
	}

	CloseHandle(MappingObject->Object);
	MappingObject->Object = CreateFileMapping(BtrProfileObject->StackFileObject, NULL, 
		                                      PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->Object) {
		ASSERT(0);
		return BTR_E_CREATEFILEMAPPING;
	}

	MappingObject->FileLength = BtrSharedData->StackFileLength;
	goto repeat;

	return S_FALSE;
}

VOID
BtrLockStackTable(
	VOID
	)
{
	ULONG i;

	//
	// Acquire all spinlocks of stack table to prevent insert new stack entry,
	// this ensure that the stack entry is consistent with stack record file.
	//

	for(i = 0; i < STACK_RECORD_BUCKET; i++) {
		BtrAcquireSpinLock(&BtrStackTable.Hash[i].SpinLock);
	}
}

//
// BtrRemoteWalkStack is primarily used for CPU profiling to capture stack trace
// fast and reliably
//


#if defined(_M_IX86)

//
// N.B. Frame pointer must be validate against stack pointer before walking the stack
// in BtrRemoteWalkStack, some binary spill ebp as temporary register, so it's possible
// that ebp is not a frame pointer when you GetThreadContext(), the following is a
// sample from wmplayer.exe, it crash since ebp is invalid, wmplayer.exe is one of
// the best test application you can easily get on Windows, it's full of quirks,
// this is due to lack of registers for x86, CFloatFft_realCore do complex floating
// point calculation, so ebp is optimized to be used as a temporary register.
//

//
// wmpeffects!FFTWrapper::CFloatFft_realCore:
// 701e3c3f 83ec64          sub     esp,64h
// 701e3c42 53              push    ebx
// 701e3c43 55              push    ebp
// 701e3c44 8b6c2474        mov     ebp,dword ptr [esp+74h]
// 701e3c48 8bc5            mov     eax,ebp
//

//
// N.B. The above is a failure case BtrRemoteWalkStack can not work as expected due to
// invalid ebp register, a solution is to walk the stack in reverse order. 
//

ULONG
BtrRemoteWalkStack(
	__in ULONG ThreadId,
	__in ULONG_PTR StackBase,
	__in ULONG_PTR StackLimit,
	__in PCONTEXT Context,
    __in ULONG Count, 
    __out PVOID *Callers,
	__out PULONG Hash
	)
{
	ULONG_PTR StackHigh;
	ULONG_PTR StackLow;
    ULONG_PTR BackwardFp;
	ULONG_PTR ReturnAddress;
	ULONG_PTR Fp;
	ULONG Number;
	ULONG Delta;

	Delta = 0;
	*Hash = 0;

	if (Context->Eip == PtrToUlong(KiFastSystemCallRet)) {

		//
		// N.B. If the thread is doing system call,
		// we need capture top 3 frames since it's not EBP based.
		//

		Callers[0] = (PVOID)*(PULONG)Context->Esp;
		Callers[1] = (PVOID)*(PULONG)(Context->Esp + 4);
		Callers = (PVOID *)((PULONG)Callers + 2);
		Delta = 2;
	}

	Fp = (ULONG_PTR)Context->Ebp;

	//
	// N.B. Frame pointer must be validated against stack range 
	//

	if (Fp < StackLimit || Fp > StackBase) {
		return 0;
	}

	StackLow = Fp;
	StackHigh = StackBase - sizeof(ULONG_PTR);

	for (Number = 0; Number < Count - Delta; Number += 1) {

		if (Fp < StackLow || Fp > StackHigh) {
			break;
		}

		BackwardFp = *(PULONG_PTR)Fp;
		ReturnAddress = *((PULONG_PTR)(Fp + sizeof(ULONG_PTR)));

		if (Fp >= BackwardFp || BackwardFp > StackHigh) {
			break;
		}

		if (StackLow < ReturnAddress && ReturnAddress < StackHigh) {
			break;
		}

		if (ReturnAddress != 0) {

			Callers[Number] = (PVOID)ReturnAddress;
			*Hash += PtrToUlong(Callers[Number]);
			Fp = BackwardFp;

		} else {

			break;
		}
	}

	return Number + Delta;
}

#elif defined(_M_X64) 

//
// The function has no handler.
//

#define UNW_FLAG_NHANDLER  0x0   

//
// The function has an exception handler that should be called.
//

#define UNW_FLAG_EHANDLER  0x1   

//
// The function has a termination handler that should be called when unwinding an exception.
//

#define UNW_FLAG_UHANDLER  0x2   

//
// The FunctionEntry member is the contents of a previous function table entry.
//

#define UNW_FLAG_CHAININFO 0x4   
 
//
// RtlVirtualUnwind
//

typedef EXCEPTION_DISPOSITION (*PEXCEPTION_ROUTINE) (
    __in struct _EXCEPTION_RECORD *ExceptionRecord,
    __in PVOID EstablisherFrame,
    __in struct _CONTEXT *ContextRecord,
    __in PVOID DispatcherContext
    );

#if _WIN32_WINNT > 0x0600

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    union {
        PM128A FloatingContext[16];
        struct {
            PM128A Xmm0;
            PM128A Xmm1;
            PM128A Xmm2;
            PM128A Xmm3;
            PM128A Xmm4;
            PM128A Xmm5;
            PM128A Xmm6;
            PM128A Xmm7;
            PM128A Xmm8;
            PM128A Xmm9;
            PM128A Xmm10;
            PM128A Xmm11;
            PM128A Xmm12;
            PM128A Xmm13;
            PM128A Xmm14;
            PM128A Xmm15;
        };
    };

    union {
        PULONG64 IntegerContext[16];
        struct {
            PULONG64 Rax;
            PULONG64 Rcx;
            PULONG64 Rdx;
            PULONG64 Rbx;
            PULONG64 Rsp;
            PULONG64 Rbp;
            PULONG64 Rsi;
            PULONG64 Rdi;
            PULONG64 R8;
            PULONG64 R9;
            PULONG64 R10;
            PULONG64 R11;
            PULONG64 R12;
            PULONG64 R13;
            PULONG64 R14;
            PULONG64 R15;
        };
    };
} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

#endif

#if _WIN32_WINNT > 0x0601

PEXCEPTION_ROUTINE NTAPI 
RtlVirtualUnwind (
    __in ULONG HandlerType,
    __in ULONG64 ImageBase,
    __in ULONG64 ControlPc,
    __in PRUNTIME_FUNCTION FunctionEntry,
    __in PCONTEXT ContextRecord,
    __out PVOID *HandlerData,
    __out PULONG64 EstablisherFrame,
    __out PKNONVOLATILE_CONTEXT_POINTERS ContextPointers 
    );

#endif

BOOLEAN
BtrIsFrameInBounds(
    __in ULONG64 LowLimit,
    __in ULONG64 StackFrame,
    __in ULONG64 HighLimit
    )
{
    if ((StackFrame & 0x7) != 0) {
        return FALSE;
    }

    if (StackFrame < LowLimit || StackFrame >= HighLimit) {
        return FALSE;
    } else {
        return TRUE;
    }
}

FORCEINLINE
PRUNTIME_FUNCTION
BtrConvertFunctionEntry (
    __in PRUNTIME_FUNCTION FunctionEntry,
    __in ULONG64 ImageBase
    )
{

    //
    // If the specified function entry is not NULL and specifies indirection,
    // then compute the address of the master function table entry.
    //

    if ((FunctionEntry != NULL) &&
        ((FunctionEntry->UnwindData & RUNTIME_FUNCTION_INDIRECT) != 0)) {

        FunctionEntry = (PRUNTIME_FUNCTION)(FunctionEntry->UnwindData + ImageBase - 1);
    }

    return FunctionEntry;
}

PRUNTIME_FUNCTION
BtrLookupFunctionEntryForStackWalks (
    __in ULONG64 ControlPc,
    __out PULONG64 ImageBase
    )
{
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    LONG High;
    LONG Low;
    LONG Middle;
    ULONG RelativePc;
    ULONG SizeOfTable;

    //
    // Attempt to find an image that contains the specified control PC. If
    // an image is found, then search its function table for a function table
    // entry that contains the specified control PC. 
    // If an image is not found then do not search the dynamic function table.
    // The dynamic function table has the same lock as the loader lock, 
    // and searching that table could deadlock a caller of RtlpWalkFrameChain
    //

    //
    // attempt to find a matching entry in the loaded module list.
    //

    FunctionTable = RtlLookupFunctionTable((PVOID)ControlPc,
                                            (PVOID *)ImageBase,
                                            &SizeOfTable);

    //
    // If a function table is located, then search for a function table
    // entry that contains the specified control PC.
    //

    if (FunctionTable != NULL) {
        Low = 0;
        High = (SizeOfTable / sizeof(RUNTIME_FUNCTION)) - 1;
        RelativePc = (ULONG)(ControlPc - *ImageBase);
        while (High >= Low) {

            //
            // Compute next probe index and test entry. If the specified
            // control PC is greater than of equal to the beginning address
            // and less than the ending address of the function table entry,
            // then return the address of the function table entry. Otherwise,
            // continue the search.
            //

            Middle = (Low + High) >> 1;
            FunctionEntry = &FunctionTable[Middle];

            if (RelativePc < FunctionEntry->BeginAddress) {
                High = Middle - 1;

            } else if (RelativePc >= FunctionEntry->EndAddress) {
                Low = Middle + 1;

            } else {
                break;
            }
        }

        if (High < Low) {
            FunctionEntry = NULL;
        }

    } else {
    
        FunctionEntry = NULL;
        
    }

    return BtrConvertFunctionEntry(FunctionEntry, *ImageBase);
}

ULONG
BtrRemoteWalkStack(
	__in ULONG ThreadId,
	__in ULONG_PTR StackBase,
	__in ULONG_PTR StackLimit,
	__in PCONTEXT Context,
    __in ULONG Count, 
    __out PVOID *Callers[],
	__out PULONG Hash
	)
{
	ULONG64 EstablisherFrame;
	PRUNTIME_FUNCTION FunctionEntry;
	PVOID HandlerData;
	ULONG64 HighLimit;
	ULONG64 ImageBase;
	ULONG Index;
	ULONG64 LowLimit;

	LowLimit = (ULONG64)StackLimit;
	HighLimit = (ULONG64)StackBase;

	Index = 0;

	while ((Index < Count) && (Context->Rip != 0)) {

		//
		// Lookup the function table entry using the point at which control
		// left the function.
		//

		FunctionEntry = BtrLookupFunctionEntryForStackWalks(Context->Rip,
			                                                 &ImageBase);

		//
		// If there is a function table entry for the routine and the stack is
		// within limits, then virtually unwind to the caller of the routine
		// to obtain the return address. Otherwise, discontinue the stack walk.
		//

		if ((FunctionEntry != NULL) &&
			((BtrIsFrameInBounds(LowLimit, Context->Rsp, HighLimit) == TRUE))) {

				RtlVirtualUnwind(UNW_FLAG_NHANDLER,
								 ImageBase,
								 Context->Rip,
								 FunctionEntry,
								 Context,
								 &HandlerData,
								 &EstablisherFrame,
								 NULL);

				Callers[Index] = (PVOID)Context->Rip;
				Index += 1;

		} else {
			break;
		}
	}

	return Index;
}

#endif

//
// Valid pointer must be above PTR_64KB
//

#define PTR_64KB   ((ULONG_PTR)(1204 * 64))

ULONG
BtrValidateStackTrace(
	__in PULONG_PTR Callers,
	__in ULONG Depth
	)
{
	ULONG Number;
	ULONG Valid;
	ULONG_PTR Frame[MAX_STACK_DEPTH];

	Valid = 0;
	for(Number = 0; Number < Depth; Number += 1) {
		if (Callers[Number] > PTR_64KB) {
			Frame[Valid] = Callers[Number];
            ASSERT(Frame[Valid] != 0);
			Valid += 1;
		}
	}

	RtlCopyMemory(Callers, Frame, sizeof(ULONG_PTR) * Valid);
	return Valid;
}