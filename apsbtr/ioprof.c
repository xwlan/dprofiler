//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include <psapi.h>
#include "ioprof.h"
#include "buildin.h"
#include "lock.h"
#include "heap.h"
#include "btr.h"
#include "util.h"
#include "cache.h"
#include "stream.h"
#include "marker.h"
#include "iodefs.h"
#include "thread.h"

//
// IO worker thread
//

HANDLE IoThreadHandle;
ULONG IoThreadId;

//
// N.B. BtrIoListWalkDuration is the interval that IO worker thread
// wake up to scan all the BtrIoRequestList array to process the queued
// IO request records, this is for busy work load, we don't wake up 
// worker thread in every interception, since wake up action will incur
// a system call into kernel, this will impact the acurracy of IO counter.
// default is 20 ms, near a thread scheduling quantum of Windows.
//

ULONG BtrIoListWalkDuration = 20;

#define IO_FLUSH_CACHE_SIZE (1024 * 64)
#define IO_FLUSH_IRP_LIMIT ((LONG)(IO_FLUSH_CACHE_SIZE / sizeof(IO_IRP_ON_DISK)))
#define IO_FLUSH_OBJ_LIMIT ((LONG)(IO_FLUSH_CACHE_SIZE / sizeof(IO_OBJECT_ON_DISK)))

#define IO_OVERLAPPED_BUCKET(_O) \
		(_O % MAX_IO_BUCKET)

DECLSPEC_CACHEALIGN
SLIST_HEADER IoIrpPendingSListHead;

DECLSPEC_CACHEALIGN
SLIST_HEADER IoIrpCompleteSListHead;

DECLSPEC_CACHEALIGN
SLIST_HEADER IoIrpAcceptSListHead;

LIST_ENTRY IoIrpTrackListHead;
LIST_ENTRY IoIrpInCallListHead;

ULONG IoFlushIrpCount;
ULONG IoOrphanedIrpCount;

IO_IRP_TABLE IoIrpTable;

PVOID IoFlushCache;
PVOID IoFlushCache2;

ULONG IoObjectNameOffset;
ULONG IoRetiredObjectCount;

//
// For PostQueuedCompletionStatus(), the packets
// are dropped since they do not track I/O.
//

ULONG IoPostQueueCount;
ULONG IoPostDequeueCount;
LIST_ENTRY IoOverlappedList;
BTR_LOCK IoOverlappedLock;

#define IO_OBJECT_BUCKET(_H) \
	((ULONG_PTR)_H % MAX_OBJECT_BUCKET)


IO_OBJECT_TABLE IoObjectTable;

DECLSPEC_CACHEALIGN
SLIST_HEADER IoRetiredObjectList;

typedef enum _IO_COUNTER_TYPE {
	IO_COUNTER_SUM,
	IO_COUNTER_FILE,
	IO_COUNTER_NET,
	IO_COUNTER_NUM,
} IO_COUNTER_TYPE;

typedef struct _IO_PERFORMANCE {
	ULONG ReadCount;
	ULONG ReadCompleteCount;
	ULONG WriteCount;
	ULONG WriteCompleteCount;
	ULONG ReadFailedCount;
	ULONG WriteFailedCount;
	ULONG64 ReadBytes;
	ULONG64 ReadCompleteBytes;
	ULONG64 WriteBytes;
	ULONG64 WriteCompleteBytes;
	ULONG64 ReadFailedBytes;
	ULONG64 WriteFailedBytes;
} IO_PERFORMANCE, *PIO_PERFORMANCE;

DECLSPEC_CACHEALIGN
BTR_SPINLOCK IoPerformanceLock;

DECLSPEC_CACHEALIGN
IO_PERFORMANCE IoPerformance[IO_COUNTER_NUM];


BOOLEAN
IoCloseFiles(
	VOID
	)
{
	CloseHandle(BtrProfileObject->IoObjectFile);
	BtrProfileObject->IoObjectFile = NULL;
	CloseHandle(BtrProfileObject->IoNameFile);
	BtrProfileObject->IoNameFile = NULL;
	CloseHandle(BtrProfileObject->IoIrpFile);
	BtrProfileObject->IoIrpFile = NULL;
	return TRUE;
}

ULONG CALLBACK
IoProfileProcedure(
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
				IoFlushRecord();
				BtrFlushStackRecord();
				Status = S_OK;
			}

			//
			// Control end issue a stop request
			//

			else if (Status == WAIT_OBJECT_0 + 1) {
				IoFlushRecord();
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
				IoFlushRecord();
				BtrFlushStackRecord();
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
IoIsPostedOverlapped(
	__in LPOVERLAPPED Overlapped
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PIO_OVERLAPPED Entry;
	ULONG_PTR Packet;
	ULONG_PTR StartVa;
	ULONG_PTR EndVa;
	ULONG_PTR Distance;
	ULONG Slot;
	BOOLEAN Status;

	Status = FALSE;
	Packet = (ULONG_PTR)Overlapped;
	ListHead = &IoOverlappedList;
	ListEntry = ListHead->Flink;

	BtrAcquireLock(&IoOverlappedLock);

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, IO_OVERLAPPED, ListEntry);
		StartVa = (ULONG_PTR)&Entry->Packet[0];
		EndVa = (ULONG_PTR)&Entry->Packet[PACKET_SLOT_COUNT - 1];

		if (Packet >= StartVa && Packet <= EndVa) {
			Distance = Packet - StartVa;
			if (Distance % sizeof(IO_IOCP_PACKET) == 0) {
				Slot = (ULONG)(Distance / sizeof(IO_IOCP_PACKET));
				Status = TRUE; 
				break;
			}
		}
		ListEntry = ListEntry->Flink;
	}

	BtrReleaseLock(&IoOverlappedLock);
	return Status;
}

PIO_IOCP_PACKET
IoAllocateIocpPacket(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PIO_OVERLAPPED Entry;
	PIO_IOCP_PACKET Packet;
	ULONG Slot;

	ListHead = &IoOverlappedList;
	ListEntry = ListHead->Flink;
	Packet = NULL;

	BtrAcquireLock(&IoOverlappedLock);

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, IO_OVERLAPPED, ListEntry);
		Slot = BtrFindFirstClearBit(&Entry->BitMap, 0);
		if (Slot != (ULONG)-1) {
			Packet = &Entry->Packet[Slot];
			BtrSetBit(&Entry->BitMap, Slot);
			break;
		}
		ListEntry = ListEntry->Flink;
	}

	if (!Packet) {

		PVOID Buffer;
		Entry = (PIO_OVERLAPPED)BtrMalloc(sizeof(IO_OVERLAPPED));

		//
		// BitMap buffer should be 32 bits ULONG aligned
		//

		Buffer = BtrMalloc(PACKET_SLOT_COUNT / 8);
		RtlZeroMemory(Buffer, PACKET_SLOT_COUNT / 8);
		BtrInitializeBitMap(&Entry->BitMap, (PULONG)Buffer, PACKET_SLOT_COUNT);

		BtrSetBit(&Entry->BitMap, 0);
		Packet =  &Entry->Packet[0];
		InsertTailList(&IoOverlappedList, &Entry->ListEntry);
	}

	BtrReleaseLock(&IoOverlappedLock);
	InterlockedIncrement(&IoPostQueueCount);

	RtlZeroMemory(Packet, sizeof(IO_IOCP_PACKET));
	return Packet;
}

VOID
IoFreeIocpPacket(
	__in PIO_IOCP_PACKET Packet
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PIO_OVERLAPPED Entry;
	ULONG_PTR Address;
	ULONG_PTR StartVa;
	ULONG_PTR EndVa;
	ULONG_PTR Distance;
	ULONG Slot;
	BOOLEAN Found = FALSE;

	ListHead = &IoOverlappedList;
	ListEntry = ListHead->Flink;

	BtrAcquireLock(&IoOverlappedLock);

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, IO_OVERLAPPED, ListEntry);
		Address = (ULONG_PTR)Packet;
		StartVa = (ULONG_PTR)&Entry->Packet[0];
		EndVa = (ULONG_PTR)&Entry->Packet[PACKET_SLOT_COUNT - 1];

		if (Address >= StartVa && Address <= EndVa) {
			Distance = Address - StartVa;
			if (Distance % sizeof(IO_IOCP_PACKET) == 0) {
				Slot = (ULONG)(Distance / sizeof(IO_IOCP_PACKET));
				if (BtrTestBit(&Entry->BitMap, Slot)) {
					BtrClearBit(&Entry->BitMap, Slot);
					Found = TRUE;
					break;
				}
			}
		}
		ListEntry = ListEntry->Flink;
	}

	BtrReleaseLock(&IoOverlappedLock);

	if (Found)
		InterlockedIncrement(&IoPostDequeueCount);
}

ULONG
IoFlushObject(
	VOID
	)
{
	PSLIST_ENTRY SListEntry;
	PIO_OBJECT Object;
	PIO_OBJECT_ON_DISK Entry;
	ULONG NameLengthInBytes;
	PUCHAR NamePtr;
	ULONG Size;
	ULONG Size2;
	ULONG Complete;
	ULONG Status;
	BOOLEAN Full;

	SListEntry = InterlockedFlushSList(&IoRetiredObjectList); 

repeat:

	Entry = (PIO_OBJECT_ON_DISK)IoFlushCache;
	NamePtr = (PUCHAR)IoFlushCache2;
	Size = 0;
	Size2 = 0;
	Full = FALSE;

	while (SListEntry != NULL) {

		ZeroMemory(Entry, sizeof(IO_OBJECT_ON_DISK));
		Object = CONTAINING_RECORD(SListEntry, IO_OBJECT, SListEntry);
		Entry->ObjectId = Object->Id;
		Entry->StackId = Object->StackId;
		Entry->Flags = Object->Flags;
		Entry->FirstIrp = -1;
		Entry->CurrentIrp = -1;

		//
		// Mark current object's name offset in name file 
		//

		Entry->NameOffset = IoObjectNameOffset;

		//
		// Increase size to be written
		//

		Size += sizeof(IO_OBJECT_ON_DISK);

		NameLengthInBytes = 0;

		if (Object->Flags & OF_FILE) {
			NameLengthInBytes = Object->u.File.Length * sizeof(WCHAR);
			RtlCopyMemory(NamePtr, Object->u.File.Name, NameLengthInBytes);
			NamePtr += NameLengthInBytes;
			BtrTrace("IO: file object retired %S", Object->u.File.Name);
		}
		else if (Object->Flags & (OF_SKIPV4|OF_SKIPV6)) {

			//
			// N.B. We need carefully copy SOCKADDR_STORAGE, it's a structure inside a structure,
			// we can not copy the whole structure, its size may not be same as 2 SOCKADDR_STORAGE.
			//

			RtlCopyMemory(NamePtr, (PVOID)&Object->u.Socket.Local, sizeof(SOCKADDR_STORAGE));
			NamePtr += sizeof(SOCKADDR_STORAGE);
			RtlCopyMemory(NamePtr, (PVOID)&Object->u.Socket.Remote, sizeof(SOCKADDR_STORAGE));
			NamePtr += sizeof(SOCKADDR_STORAGE);
			NameLengthInBytes = sizeof(SOCKADDR_STORAGE) * 2;
			BtrTrace("IO: socket object retired, %d", Object->Object);
		}
		else {
			DebugBreak();
			ASSERT(0);
		}

		Size2 += NameLengthInBytes;
		IoObjectNameOffset += NameLengthInBytes;

		//
		// Must iterate to next SLIST_ENTRY before free
		// the object otherwise we will read bad memory!
		//

		SListEntry = SListEntry->Next;
		Entry += 1;

		//
		// Free object
		//

		if ((Object->Flags & OF_FILE) && Object->u.File.Name) {
			BtrFree(Object->u.File.Name);
		}
		BtrFree(Object);

		//
		// Check whether the buffers are full
		//

		if ((Size2 + (MAX_PATH + 1) * sizeof(WCHAR)) > IO_FLUSH_CACHE_SIZE ||
			(Size + sizeof(IO_OBJECT_ON_DISK)) > IO_FLUSH_CACHE_SIZE) {
			Full = TRUE;
			break;
		}
	} 

	if (Size > 0) {
		Status = WriteFile(BtrProfileObject->IoObjectFile, IoFlushCache, Size, &Complete, NULL); 
		if (!Status)  {
			return GetLastError();
		}

		IoRetiredObjectCount += Size / sizeof(IO_OBJECT_ON_DISK);
		DebugTrace("IO_RETIRE_OBJECT_COUNT: %d", IoRetiredObjectCount);
	}
	if (Size2 > 0) {
		Status = WriteFile(BtrProfileObject->IoNameFile, IoFlushCache2, Size2, &Complete, NULL); 
		if (!Status) {
			return GetLastError();
		}
	}

	if (Full) {
		goto repeat;
	}

	return S_OK;
}

ULONG
IoFlushObjectOnStop(
	VOID
	)
{
	ULONG Number; 
	PLIST_ENTRY ListEntry;
	PIO_OBJECT Clone;
	PIO_OBJECT Object;


	//
	// N.B. It's possible even after stop profiling, APC callback,
	// or other asynchronous I/O notification can access object(remove),
	// so we must be caucious to acqure the lock before walk the bucket.
	// to avoid duplicate object, e.g. host process may remove object
	// after we clone it, and this object will be inserted into retired
	// object list, we must ensure each object we access will not be 
	// removed by host process, so we add a big reference to pin it in the table.
	//

	for(Number = 0; Number < MAX_OBJECT_BUCKET; Number += 1) { 

		BtrAcquireSpinLock(&IoObjectTable.Entry[Number].SpinLock);

		ListEntry = IoObjectTable.Entry[Number].ListHead.Flink;
		while (ListEntry != &IoObjectTable.Entry[Number].ListHead) {

			Object = CONTAINING_RECORD(ListEntry, IO_OBJECT, ListEntry);

			//
			// Increase object reference to -1 to pin it
			//

			Object->References = -1;
			
			//
			// Clone object and insert into retired object list
			//

			Clone = (PIO_OBJECT)BtrMalloc(sizeof(IO_OBJECT));				
			RtlCopyMemory(Clone, Object, sizeof(IO_OBJECT));

			if (Clone->Type == HANDLE_SOCKET){
				IoQuerySocketAddress(Clone, (SOCKET)Clone->Object);
			}

			InterlockedPushEntrySList(&IoRetiredObjectList, &Clone->SListEntry);

			ListEntry = ListEntry->Flink;
		}

		BtrReleaseSpinLock(&IoObjectTable.Entry[Number].SpinLock);
	}

	//
	// Finally, leverage IoFlushObject() to flush all objects
	//

	IoFlushObject();

	DebugTrace("Total %d IO object allocated", IoObjectId);
	return S_OK;
}

ULONG
IoFlushIrpOnStop(
	VOID
	)
{
	ULONG Status;
	PSLIST_ENTRY SListEntry;
	PIO_IRP Irp;
	PIO_IRP_ON_DISK Entry;
	LONG Number;
	ULONG Complete;
	PLIST_ENTRY ListEntry;
	LARGE_INTEGER Current;

	//
	// When this routine is called, no new irp generated, all irps are
	// chained into IoIrpTrackListHead.
	//

	//
	// N.B. Even if we decommit the hotpatch to stop intercepting new irp,
	// and call references are 0, we still can not guarantee that the allocated
	// irps are not accessed by host process. because asynchronous I/O. we simply
	// don't deallocate these resources, and terminate the host process.
	//

	QueryPerformanceCounter(&Current);

	//
	// Chain all pending irp to track list
	//

	SListEntry = InterlockedFlushSList(&IoIrpPendingSListHead);
	while (SListEntry != NULL) {
		Irp = CONTAINING_RECORD(SListEntry, IO_IRP, PendingSListEntry);
		InsertTailList(&IoIrpTrackListHead, &Irp->TrackListEntry);
		SListEntry = SListEntry->Next;
	}

	Status = S_OK;

repeat:
	Number = 0;
	Entry = (PIO_IRP_ON_DISK)IoFlushCache;

	while (!IsListEmpty(&IoIrpTrackListHead)) {

		ListEntry = RemoveHeadList(&IoIrpTrackListHead); 
		Irp = CONTAINING_RECORD(ListEntry, IO_IRP, TrackListEntry);

		Entry[Number].RequestId = Irp->RequestId;
		Entry[Number].ObjectId = Irp->ObjectId;
		Entry[Number].StackId = Irp->StackId;
		Entry[Number].Operation = Irp->Operation;
		Entry[Number].RequestBytes = Irp->RequestBytes;
		Entry[Number].CompletionBytes = Irp->RequestBytes;
		Entry[Number].IoStatus = Irp->IoStatus;
		Entry[Number].RequestThreadId = Irp->RequestThreadId;
		Entry[Number].CompleteThreadId = Irp->CompleteThreadId;
		Entry[Number].Time = Irp->Time;
		Entry[Number].Duration.QuadPart = Irp->End.QuadPart - Irp->Start.QuadPart;
		Entry[Number].Asynchronous = !Irp->Flags.Synchronous;
		Entry[Number].UserApc = Irp->Flags.UserApc;
		Entry[Number].Aborted = !Irp->IoStatus;
		Entry[Number].Complete = Irp->Flags.Completed;

		if (IoIsOrphanedIrp(Irp, &Current)) {
			Entry[Number].Orphaned = 1;
		}

		Number += 1;
		if (Number == IO_FLUSH_IRP_LIMIT) {
			break;
		}
	} 

	if (Number > 0) {
		Status = WriteFile(BtrProfileObject->IoIrpFile, IoFlushCache, 
							Number * sizeof(IO_IRP_ON_DISK), &Complete, NULL); 
		if (!Status) {
			return GetLastError();
		}

		//
		// Update the flushed irp count
		//

		IoFlushIrpCount += Number;
		DebugTrace("IO_RETIRE_IRP_COUNT: %d", IoFlushIrpCount);
		
		if (Number == IO_FLUSH_IRP_LIMIT) {
			goto repeat;
		}
	}

	DebugTrace("Total %d IO irp allocated", IoRequestId);
	return S_OK;
}

ULONG
IoFlushIrp(
	VOID
	)
{
	ULONG Status;
	PSLIST_ENTRY SListEntry;
	PIO_IRP Irp;
	PIO_IRP_ON_DISK Entry;
	LONG Number;
	ULONG Complete;
	PLIST_ENTRY ListEntry;
	ULONG RepeatCount = 0;

	//
	// Dequeue pending irp list, and queue to track list, this list is
	// used to avoid spinlock acquisition when insert a pending irp.
	//

	SListEntry = InterlockedFlushSList(&IoIrpPendingSListHead);
	while (SListEntry != NULL) {
		Irp = CONTAINING_RECORD(SListEntry, IO_IRP, PendingSListEntry);
		InsertTailList(&IoIrpTrackListHead, &Irp->TrackListEntry);
		SListEntry = SListEntry->Next;
	}

	//
	// Walk in call list to re-insert it into completion list
	//

	ListEntry = IoIrpInCallListHead.Flink;
	while (ListEntry != &IoIrpInCallListHead) {
		Irp = CONTAINING_RECORD(ListEntry, IO_IRP, InCallListEntry);
		if (IoIsIrpInCall(Irp)) {
			ListEntry = ListEntry->Flink;
			continue;
		}
		RemoveEntryList(ListEntry);
		InterlockedPushEntrySList(&IoIrpCompleteSListHead, &Irp->CompleteSListEntry);
		ListEntry = ListEntry->Flink;
	}

	//
	// Dequeue completed irp list
	//

	Status = S_OK;
	SListEntry = InterlockedFlushSList(&IoIrpCompleteSListHead);

repeat:
	RepeatCount += 1;
	Number = 0;
	Entry = (PIO_IRP_ON_DISK)IoFlushCache;

	while (SListEntry != NULL) {

		Irp = CONTAINING_RECORD(SListEntry, IO_IRP, CompleteSListEntry);
		DebugTrace("IO: FlushIrp=%p", Irp);

		//
		// If I/O request thread is still using irp, chain this irp into
		// IoIrpInCallListHead
		//

		__try {
		if (IoIsIrpInCall(Irp)) {
			InsertTailList(&IoIrpInCallListHead, &Irp->InCallListEntry);
			SListEntry = SListEntry->Next;
			continue;
		}
		}__except(1){
			DebugTrace("!!!!!!!CRASH: Irp=%p", Irp);
			SuspendThread(GetCurrentThread());
		}

		Entry[Number].RequestId = Irp->RequestId;
		Entry[Number].ObjectId = Irp->ObjectId;
		Entry[Number].StackId = Irp->StackId;
		Entry[Number].Operation = Irp->Operation;
		Entry[Number].RequestBytes = Irp->RequestBytes;
		Entry[Number].CompletionBytes = Irp->RequestBytes;
		Entry[Number].IoStatus = Irp->IoStatus;
		Entry[Number].RequestThreadId = Irp->RequestThreadId;
		Entry[Number].CompleteThreadId = Irp->CompleteThreadId;
		Entry[Number].Time = Irp->Time;
		Entry[Number].Duration.QuadPart = Irp->End.QuadPart - Irp->Start.QuadPart;
		Entry[Number].Asynchronous = !Irp->Flags.Synchronous;
		Entry[Number].UserApc = Irp->Flags.UserApc;
		Entry[Number].Aborted = !!Irp->IoStatus;
		Entry[Number].Complete = Irp->Flags.Completed;
		Entry[Number].Complete = !Irp->IoStatus;
		Entry[Number].Orphaned = 0;
		Entry[Number].NextIrp = -1;
		Entry[Number].ThreadedNextIrp = -1;

		//
		// Skip to next entry, remove the irp from track list and free to lookaside
		//
		
		SListEntry = SListEntry->Next;
		RemoveEntryList(&Irp->TrackListEntry);
		IoFreeIrp(Irp);

		Number += 1;
		if (Number == IO_FLUSH_IRP_LIMIT) {
			break;
		}
	} 

	if (Number > 0) {
		Status = WriteFile(BtrProfileObject->IoIrpFile, IoFlushCache, 
							Number * sizeof(IO_IRP_ON_DISK), &Complete, NULL); 
		if (!Status) {
			return GetLastError();
		}

		//
		// Update the flushed irp count
		//

		IoFlushIrpCount += Number;
		DebugTrace("IO_RETIRE_IRP_COUNT: %d", IoFlushIrpCount);
		
		if (Number == IO_FLUSH_IRP_LIMIT) {
			goto repeat;
		}
	}

	return S_OK;
}

ULONG
IoFlushIrp2(
	VOID
	)
{
	ULONG Status;
	PSLIST_ENTRY SListEntry;
	PIO_IRP Irp;
	PIO_IRP_ON_DISK Entry;
	LONG Number;
	ULONG Complete;
	PLIST_ENTRY ListEntry;
	ULONG RepeatCount = 0;

	//
	// Dequeue pending irp list, and queue to track list, this list is
	// used to avoid spinlock acquisition when insert a pending irp.
	//

	SListEntry = InterlockedFlushSList(&IoIrpPendingSListHead);
	while (SListEntry != NULL) {
		Irp = CONTAINING_RECORD(SListEntry, IO_IRP, PendingSListEntry);
		InsertTailList(&IoIrpTrackListHead, &Irp->TrackListEntry);
		SListEntry = SListEntry->Next;
	}

	//
	// Walk in call list to re-insert it into completion list
	//

	ListEntry = IoIrpInCallListHead.Flink;
	while (ListEntry != &IoIrpInCallListHead) {
		Irp = CONTAINING_RECORD(ListEntry, IO_IRP, InCallListEntry);
		if (IoIsIrpInCall(Irp)) {
			ListEntry = ListEntry->Flink;
			continue;
		}
		RemoveEntryList(ListEntry);
		InterlockedPushEntrySList(&IoIrpCompleteSListHead, &Irp->CompleteSListEntry);
		ListEntry = ListEntry->Flink;
	}

	//
	// Dequeue completed irp list
	//

	Status = S_OK;
	SListEntry = InterlockedFlushSList(&IoIrpCompleteSListHead);

repeat:
	RepeatCount += 1;
	Number = 0;
	Entry = (PIO_IRP_ON_DISK)IoFlushCache;

	while (SListEntry != NULL) {

		Irp = CONTAINING_RECORD(SListEntry, IO_IRP, CompleteSListEntry);
		DebugTrace("IO: FlushIrp=%p", Irp);

		//
		// If I/O request thread is still using irp, chain this irp into
		// IoIrpInCallListHead
		//

		__try {
		if (IoIsIrpInCall(Irp)) {
			InsertTailList(&IoIrpInCallListHead, &Irp->InCallListEntry);
			SListEntry = SListEntry->Next;
			continue;
		}
		}__except(1){
			DebugTrace("!!!!!!!CRASH: Irp=%p", Irp);
			SuspendThread(GetCurrentThread());
		}

		Entry[Number].RequestId = Irp->RequestId;
		Entry[Number].ObjectId = Irp->ObjectId;
		Entry[Number].StackId = Irp->StackId;
		Entry[Number].Operation = Irp->Operation;
		Entry[Number].RequestBytes = Irp->RequestBytes;
		Entry[Number].CompletionBytes = Irp->RequestBytes;
		Entry[Number].IoStatus = Irp->IoStatus;
		Entry[Number].RequestThreadId = Irp->RequestThreadId;
		Entry[Number].CompleteThreadId = Irp->CompleteThreadId;
		Entry[Number].Time = Irp->Time;
		Entry[Number].Duration.QuadPart = Irp->End.QuadPart - Irp->Start.QuadPart;
		Entry[Number].Asynchronous = !Irp->Flags.Synchronous;
		Entry[Number].UserApc = Irp->Flags.UserApc;
		Entry[Number].Aborted = !!Irp->IoStatus;
		Entry[Number].Complete = Irp->Flags.Completed;
		Entry[Number].Complete = !Irp->IoStatus;
		Entry[Number].Orphaned = 0;
		Entry[Number].NextIrp = -1;
		Entry[Number].ThreadedNextIrp = -1;

		//
		// Skip to next entry, remove the irp from track list and free to lookaside
		//
		
		SListEntry = SListEntry->Next;
		RemoveEntryList(&Irp->TrackListEntry);
		IoFreeIrp(Irp);

		Number += 1;
		if (Number == IO_FLUSH_IRP_LIMIT) {
			break;
		}
	} 

	if (Number > 0) {
		Status = WriteFile(BtrProfileObject->IoIrpFile, IoFlushCache, 
							Number * sizeof(IO_IRP_ON_DISK), &Complete, NULL); 
		if (!Status) {
			return GetLastError();
		}

		//
		// Update the flushed irp count
		//

		IoFlushIrpCount += Number;
		DebugTrace("IO_RETIRE_IRP_COUNT: %d", IoFlushIrpCount);
		
		if (Number == IO_FLUSH_IRP_LIMIT) {
			goto repeat;
		}
	}

	return S_OK;
}

ULONG
IoFlushRecord(
	VOID
	)
{
	ULONG Status;

	Status = IoFlushIrp();
	if (Status != S_OK) {
		return Status;
	}

	Status = IoFlushObject();
	return Status;
}

//
// N.B. IoFlushRecordOnStop is the last step
// to flush records when user stop profiling or
// process is terminating.
//

//
// Timeout threshold is 10 seconds, after stopping
// profiling, any irp whose duration is beyond 10s
// is considered problematic, should be tracked into
// orphaned ones. time unit is microseconds (us)
//

#define IO_IRP_TIMEOUT_THRESHOLD  10 * 1000 * 1000

BOOLEAN
IoIsOrphanedIrp(
	_In_ PIO_IRP Irp,
	_In_ PLARGE_INTEGER Current
	)
{
	ULONG Microseconds;

	//
	// If duration is longer than IO_IRP_TIMEOUT_THRESHOLD
	//

	Microseconds = (ULONG)((Current->QuadPart - Irp->Start.QuadPart) * 1000000 / BtrHardwareFrequency.QuadPart);
	if (Microseconds > IO_IRP_TIMEOUT_THRESHOLD) {
		return TRUE;
	}
	return FALSE;
}

//
// N.B. This routine can only be called in unloading stage,
// after all hotpatches are recovered and all callback references 
// are cleared, it's safe to flush all allocated objects and irps.
//

ULONG
IoFlushRecordOnStop(
	VOID
	)
{
	ULONG Status;

	Status = IoFlushRecord();
	if (Status != S_OK) {
		return Status;
	}

	Status = IoFlushIrpOnStop();
	if (Status != S_OK) {
		return Status;
	}
	
	IoFlushObjectOnStop();
	if (Status != S_OK) {
		return Status;
	}

	return S_OK;
}

BOOLEAN
IoDllCanUnload(
	VOID
	)
{
	ULONG i;
	PBTR_CALLBACK Callback;

	//
	// This routine is called with all other threads suspended
	//

	for(i = 0; i < IoCallbackCount; i++) {
		Callback = &IoCallback[i];
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
IoValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	ASSERT(Attr->Type == PROFILE_IO_TYPE);

	if (!Attr->EnableFile && !Attr->EnableNet && !Attr->EnableDevice){
		return BTR_E_INVALID_PARAMETER;
	}

	return S_OK;
}

ULONG
IoInitialize(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	//
	// Fill profile object callbacks
	//

	Object->Start = IoStartProfile;
	Object->Stop = IoStopProfile;
	Object->Pause = IoPauseProfile;
	Object->Resume = IoResumeProfile;
	Object->Unload = IoUnload;
	Object->QueryHotpatch = IoQueryHotpatch;
	Object->ThreadAttach = IoThreadAttach;
	Object->ThreadDetach = IoThreadDetach;
	Object->IsRuntimeThread = IoIsRuntimeThread;

	//
	// Initialize mark list
	//

	BtrInitializeMarkList();

	//
	// Initialize IO object and request table
	//

	IoInitObjectTable();
	IoInitIrpTable();

	IoFlushCache = VirtualAlloc(NULL, IO_FLUSH_CACHE_SIZE,
									MEM_COMMIT, PAGE_READWRITE);
	IoFlushCache2 = VirtualAlloc(NULL, IO_FLUSH_CACHE_SIZE,
									MEM_COMMIT, PAGE_READWRITE);
	IoObjectNameOffset = 0;
	IoRetiredObjectCount = 0;

	BtrInitSpinLock(&IoPerformanceLock,100);

	//
	// Thread pool 
	//

	InitializeListHead(&IoTpContextList);
	BtrInitSpinLock(&IoTpContextLock,100);
	return S_OK;
}

ULONG
IoStartProfile(
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

	IoThreadHandle = CreateThread(NULL, 0, IoProfileProcedure, Object, 0, &IoThreadId);
	if (!IoThreadHandle) {
		return GetLastError();
	}

	return S_OK;
}

ULONG
IoStopProfile(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	InterlockedExchange(&BtrStopped, (LONG)TRUE);

	//
	// Signal stop event and wait for profile thread terminated
	//

	SetEvent(Object->StopEvent);
	WaitForSingleObject(IoThreadHandle, INFINITE);
	CloseHandle(IoThreadHandle);
	IoThreadHandle = NULL;

	return S_OK;
}

ULONG
IoPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return S_OK;
}

ULONG
IoResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return S_OK;
}

ULONG
IoThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	return TRUE;
}

ULONG
IoThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	PBTR_THREAD_OBJECT Thread;
	ULONG_PTR Value;
	ULONG ThreadId;

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
IoUnload(
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	BOOLEAN Unload;

	// Flush all allocated objects and irps.
	//

	IoFlushRecordOnStop();
	VirtualFree(IoFlushCache, 0, MEM_RELEASE);
	VirtualFree(IoFlushCache2, 0, MEM_RELEASE);

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

	IoCloseFiles();

	CloseHandle(BtrProfileObject->ReportFileObject);
	BtrProfileObject->ReportFileObject = NULL;

	CloseHandle(BtrProfileObject->StopEvent);
	BtrProfileObject->StopEvent = NULL;

	CloseHandle(BtrProfileObject->ControlEnd);
	BtrProfileObject->ControlEnd = NULL;

	//
	// Wait up to 6 second to check dll unload
	//

	if (!IoDllCanUnload()) {
		Sleep(6000);
	}

	if (!IoDllCanUnload()) {

		//
		// If we can not unload, just terminate host process
		//

		SetEvent(BtrProfileObject->UnloadEvent);
		CloseHandle(BtrProfileObject->UnloadEvent);
		ExitProcess(0);

		//
		// never arrive here
		//
	}

	//
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
IoIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	)
{
	if (ThreadId == IoThreadId) {
		return TRUE;
	}

	return FALSE;
}

ULONG
IoQueryHotpatch(
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

	if (Action == HOTPATCH_COMMIT && Object->Attribute.EnableNet) {

		//
		// Retrieve dynamic function pointers from mswsock.dll
		//

		IoGetFunctionPointers();
	}

	CallbackCount = IoCallbackCount;
	Hotpatch = (PBTR_HOTPATCH_ENTRY)BtrMalloc(sizeof(BTR_HOTPATCH_ENTRY) * CallbackCount);

	if (Object->Attribute.EnableFile) {
	}

	for(Number = 0; Number < IoCallbackCount; Number += 1) {

		Callback = &IoCallback[Number];

		if (!Callback->Address) {
			DllHandle = GetModuleHandleA(Callback->DllName);
			Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);
		}

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
IoExitProcessCallback(
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

VOID
IoInitIrpTable(
	VOID
	)
{
	int i;

	RtlZeroMemory(&IoIrpTable, sizeof(IoIrpTable));
	
	for(i = 0; i < IO_IRP_BUCKET_COUNT; i++) {
		InitializeListHead(&IoIrpTable.ListHead[i]);
	}
	InitializeListHead(&IoIrpTable.InCallListHead);

	InitializeListHead(&IoIrpTrackListHead);
	InitializeListHead(&IoIrpInCallListHead);
	InitializeSListHead(&IoIrpPendingSListHead);
	InitializeSListHead(&IoIrpCompleteSListHead);
	InitializeSListHead(&IoIrpAcceptSListHead);

	IoPostQueueCount = 0;
	IoPostDequeueCount = 0;

	BtrInitLock(&IoOverlappedLock);
	InitializeListHead(&IoOverlappedList);
}


VOID 
IoQueueFlushList(
	_In_ PIO_IRP Irp
	)
{
	ASSERT(Irp != NULL);
	InterlockedPushEntrySList(&IoIrpCompleteSListHead, &Irp->CompleteSListEntry);	
}

VOID
IoUpdateRequestCounters(
	_In_ PIO_IRP Irp
	)
{
	ASSERT(Irp != NULL);

	switch (Irp->Operation) {
		
	case IO_OP_READ:

		InterlockedIncrement(&IoPerformance[IO_COUNTER_SUM].ReadCount);
		BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_SUM].ReadBytes, Irp->RequestBytes, &IoPerformanceLock);

		if (Irp->Flags.File) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_FILE].ReadCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_FILE].ReadBytes, Irp->RequestBytes, &IoPerformanceLock);
		}
		else if (Irp->Flags.Socket) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_NET].ReadCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_NET].ReadBytes, Irp->RequestBytes, &IoPerformanceLock);
		}
		else {
		}
		break;

	case IO_OP_WRITE:

		InterlockedIncrement(&IoPerformance[IO_COUNTER_SUM].WriteCount);
		BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_SUM].WriteBytes, Irp->RequestBytes, &IoPerformanceLock);

		if (Irp->Flags.File) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_FILE].WriteCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_FILE].WriteBytes, Irp->RequestBytes, &IoPerformanceLock);
		}
		else if (Irp->Flags.Socket) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_NET].WriteCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_NET].WriteBytes, Irp->RequestBytes, &IoPerformanceLock);
		} 
		else {
		}
		break;

	case IO_OP_ACCEPT:
	case IO_OP_CONNECT:
		break;

	default:
		break;
	}
}

VOID
IoUpdateCompleteCounters(
	_In_ PIO_IRP Irp
	)
{
	ASSERT(Irp != NULL);
	ASSERT(Irp->Flags.Completed);

	switch (Irp->Operation) { 

	case IO_OP_READ:
		InterlockedIncrement(&IoPerformance[IO_COUNTER_SUM].ReadCompleteCount);
		BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_SUM].ReadCompleteBytes, Irp->CompleteBytes, &IoPerformanceLock);
		if (Irp->Flags.File) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_FILE].ReadCompleteCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_FILE].ReadCompleteBytes, Irp->CompleteBytes, &IoPerformanceLock);
		}
		else if (Irp->Flags.Socket) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_NET].ReadCompleteCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_NET].ReadCompleteBytes, Irp->CompleteBytes, &IoPerformanceLock);
		}
		else {
		}

	case IO_OP_WRITE:
		InterlockedIncrement(&IoPerformance[IO_COUNTER_SUM].WriteCompleteCount);
		BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_SUM].WriteCompleteBytes, Irp->CompleteBytes,&IoPerformanceLock);
		if (Irp->Flags.File) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_FILE].WriteCompleteCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_FILE].WriteCompleteBytes, Irp->CompleteBytes, &IoPerformanceLock);
		}
		else if (Irp->Flags.Socket) {
			InterlockedIncrement(&IoPerformance[IO_COUNTER_NET].WriteCompleteCount);
			BtrInterlockedAdd64(&IoPerformance[IO_COUNTER_NET].WriteCompleteBytes, Irp->CompleteBytes,&IoPerformanceLock);
		} 
		else {
		}
		break;

	case IO_OP_ACCEPT:
	case IO_OP_CONNECT:
		break;

	default:
		break;
	}
}

//
// N.B. don't use the following currently
//

#define IO_IRP_TYPE 1
#define IO_COMPLETION_TYPE 2
#define IO_STOP_TYPE 3 

HANDLE BtrIoCompletionPort;
HANDLE BtrIoCompletionThread;
SLIST_HEADER BtrIoCompletionList;

ULONG CALLBACK
BtrIoCompletionProcedure(
	__in PVOID Context
	)
{
	LPOVERLAPPED_ENTRY Entry;
	ULONG i;
	ULONG Count = 1;
	BOOL Status = FALSE;

	if(BtrNtMajorVersion >= 6) {
		Entry = (LPOVERLAPPED_ENTRY)BtrMalloc(sizeof(OVERLAPPED_ENTRY) * 4096);
	}
	else {
		Entry = (LPOVERLAPPED_ENTRY)BtrMalloc(sizeof(OVERLAPPED_ENTRY));
	}

	while (1) {

		if(BtrNtMajorVersion >= 6) {
			Status = GetQueuedCompletionStatusEx(BtrIoCompletionPort, Entry, 
				4096, &Count, INFINITE, FALSE);
		}
		else {
			Status = GetQueuedCompletionStatus(BtrIoCompletionPort, 
				&Entry->dwNumberOfBytesTransferred,
				&Entry->lpCompletionKey, &Entry->lpOverlapped, 
				INFINITE);
		}

		if (!Status) {
			//
			// Stop profiling
			//
			Status = GetLastError();
			break;
		}

		for(i = 0; i < Count; i++) {

			PIO_COMPLETION_PACKET Packet;
			PIO_IRP Request;

			if (Entry[i].lpCompletionKey == IO_COMPLETION_TYPE) {
				Packet = (PIO_COMPLETION_PACKET)Entry->lpOverlapped;
			}
			else if (Entry[i].lpCompletionKey == IO_IRP_TYPE) { 
				Request = (PIO_IRP)Entry->lpOverlapped;
			}
			else if (Entry[i].lpCompletionKey == IO_STOP_TYPE) {

				//
				// Stop profiling
				//

				return S_OK;
			}
			else {
				//
				// Ill packet received, ignore
				//
			}
		}
		
	}

	return S_OK;
}

ULONG
BtrIoInitWorkQueue(
	VOID
	)
{
	BtrIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!BtrIoCompletionPort)
		return S_FALSE;

	BtrIoCompletionThread = CreateThread(NULL, 0, BtrIoCompletionProcedure, NULL, CREATE_SUSPENDED, NULL);
	if (!BtrIoCompletionThread){
		CloseHandle(BtrIoCompletionPort);
		return S_FALSE;
	}

	ResumeThread(BtrIoCompletionThread);
	return S_OK;
}

ULONG
IoInitObjectTable(
	VOID
	)
{
	ULONG Number;

	IoObjectTable.Count = 0;
	for(Number = 0; Number < MAX_OBJECT_BUCKET; Number += 1) {
		BtrInitSpinLock(&IoObjectTable.Entry[Number].SpinLock, 100);
		InitializeListHead(&IoObjectTable.Entry[Number].ListHead);
	}

	InitializeSListHead(&IoRetiredObjectList);
	return S_OK;
}

VOID
IoMarkObjectOverlapped(
	_In_ PIO_OBJECT Object
	)
{
	ASSERT(Object != NULL);
	IoLockObjectBucket(Object);
	SetFlag(Object->Flags, OF_OVERLAPPED);
	IoUnlockObjectBucket(Object);
}

VOID
IoClearObjectOverlapped(
	_In_ PIO_OBJECT Object
	)
{
	ASSERT(Object != NULL);
	IoLockObjectBucket(Object);
	ClearFlag(Object->Flags, OF_OVERLAPPED);
	IoUnlockObjectBucket(Object);
}

BOOLEAN
IoIsObjectOverlapped(
	_In_ PIO_OBJECT Object 
	)
{
	return FlagOn(Object->Flags, OF_OVERLAPPED) ? TRUE : FALSE;
}

BOOLEAN
IoIsObjectSkipOnSuccess(
	_In_ PIO_OBJECT Object 
	)
{
	return FlagOn(Object->Flags, OF_SKIPONSUCCESS) ? TRUE : FALSE;
}

PIO_OBJECT
IoInsertObject(
	__in PIO_OBJECT Object
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;
	PLIST_ENTRY ListHead;

#ifdef _DEBUG
	if (FlagOn(Object->Flags, OF_FILE)) {
		DebugTrace("IoInsertObject: H=%p, N=%S", Object->Object, Object->u.File.Name);
	} 
	else if (FlagOn(Object->Flags, OF_SKIPV4)) {
		IoDebugPrintSkAddress(Object);
	} 
	else if (FlagOn(Object->Flags, OF_SKIPV6)) {
		DebugTrace("IPV6 ADDRESS");
	}
	else {
	}
#endif

	Bucket = IO_OBJECT_BUCKET(Object->Object);
	HashEntry = &IoObjectTable.Entry[Bucket];
	ListHead = &HashEntry->ListHead;

	BtrAcquireSpinLock(&HashEntry->SpinLock);
	InsertTailList(ListHead, &Object->ListEntry);
	Object->References = 1;
	BtrReleaseSpinLock(&HashEntry->SpinLock);

	InterlockedIncrement(&IoObjectTable.Count);
	return Object;
}

VOID
IoReferenceObject(
	__in PIO_OBJECT Object
	)
{
	IoLockObjectBucket(Object);
	Object->References += 1;
	IoUnlockObjectBucket(Object);
}

VOID
IoLockObjectBucket(
	__in PIO_OBJECT Object
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;

	ASSERT(Object != NULL);
	ASSERT(Object->Object != NULL);

	Bucket = IO_OBJECT_BUCKET(Object->Object);
	HashEntry = &IoObjectTable.Entry[Bucket];
	BtrAcquireSpinLock(&HashEntry->SpinLock);
}

VOID
IoUnlockObjectBucket(
	__in PIO_OBJECT Object
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;

	Bucket = IO_OBJECT_BUCKET(Object->Object);
	HashEntry = &IoObjectTable.Entry[Bucket];
	BtrReleaseSpinLock(&HashEntry->SpinLock);
}

VOID
IoUnreferenceObject(
	__in PIO_OBJECT Object
	)
{
	IoLockObjectBucket(Object);
	Object->References -= 1;
	if (!Object->References) {
		RemoveEntryList(&Object->ListEntry);

		//
		// N.B. InterlockedPushEntrySList() must synchronize with
		// IoFlushObjectOnStop(), otherwise we can miss the object
		// in final report. object's remove must with the spinlock hold.
		//

		GetSystemTimeAsFileTime(&Object->End);
		InterlockedDecrement(&IoObjectTable.Count);
		InterlockedPushEntrySList(&IoRetiredObjectList, &Object->SListEntry);

	}
	IoUnlockObjectBucket(Object);
}

//
// N.B. After the lookup and usage of object,
// the caller should unreference the object,
// otherwise the object can always reside in the table
// since it can not be zero
//

PIO_OBJECT
IoLookupObjectByHandle(
	__in HANDLE Handle
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PIO_OBJECT Object;
	BOOLEAN Found = FALSE;

	Bucket = IO_OBJECT_BUCKET(Handle);
	HashEntry = &IoObjectTable.Entry[Bucket];
	ListHead = &HashEntry->ListHead;

	BtrAcquireSpinLock(&HashEntry->SpinLock);
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Object = CONTAINING_RECORD(ListEntry, IO_OBJECT, ListEntry);
		if (Object->Object == Handle) {
			Object->References += 1;  
			Found = TRUE;
			break;
		}
		ListEntry = ListEntry->Flink;
	}
	
	BtrReleaseSpinLock(&HashEntry->SpinLock);

	if (Found) {
		return Object;
	}
	
	return NULL;
}

PIO_OBJECT
IoLookupObjectByHandleEx(
	__in HANDLE Handle,
	__in HANDLE_TYPE Type
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PIO_OBJECT Object;
	BOOLEAN Found = FALSE;

	Bucket = IO_OBJECT_BUCKET(Handle);
	HashEntry = &IoObjectTable.Entry[Bucket];
	ListHead = &HashEntry->ListHead;

	BtrAcquireSpinLock(&HashEntry->SpinLock);
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Object = CONTAINING_RECORD(ListEntry, IO_OBJECT, ListEntry);
		if (Object->Object == Handle && Object->Type == Type) {
			Object->References += 1;  
			Found = TRUE;
			break;
		}
		ListEntry = ListEntry->Flink;
	}
	
	BtrReleaseSpinLock(&HashEntry->SpinLock);

	if (Found) {
		return Object;
	}
	
	return NULL;
}

BOOLEAN
IoRemoveObjectByHandleEx(
	__in HANDLE Handle,
	__in HANDLE_TYPE Type
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PIO_OBJECT Object;
	BOOLEAN Free = FALSE;


	Bucket = IO_OBJECT_BUCKET(Handle);
	HashEntry = &IoObjectTable.Entry[Bucket];
	ListHead = &HashEntry->ListHead;

	BtrAcquireSpinLock(&HashEntry->SpinLock);
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Object = CONTAINING_RECORD(ListEntry, IO_OBJECT, ListEntry);
		if (Object->Object == Handle) {
			Object->References -= 1;
			if (!Object->References) {

				RemoveEntryList(ListEntry);

				//
				// N.B. We move the code here to avoid race condition
				// with IoFlushObjectOnStop(), the remove should hold
				// spinlock to synchronize with it.
				//

				GetSystemTimeAsFileTime(&Object->End);
				InterlockedDecrement(&IoObjectTable.Count);
				InterlockedPushEntrySList(&IoRetiredObjectList, &Object->SListEntry);
				Free = TRUE;
			}
			break;
		}
		ListEntry = ListEntry->Flink;
	}
	
	BtrReleaseSpinLock(&HashEntry->SpinLock);
	return Free;
}

BOOLEAN
IoRemoveObjectByHandle(
	__in HANDLE Handle
	)
{
	ULONG Bucket;
	PBTR_SPIN_HASH HashEntry;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PIO_OBJECT Object;
	BOOLEAN Free = FALSE;


	Bucket = IO_OBJECT_BUCKET(Handle);
	HashEntry = &IoObjectTable.Entry[Bucket];
	ListHead = &HashEntry->ListHead;

	BtrAcquireSpinLock(&HashEntry->SpinLock);
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Object = CONTAINING_RECORD(ListEntry, IO_OBJECT, ListEntry);
		if (Object->Object == Handle) {
			Object->References -= 1;
			if (!Object->References) {

				RemoveEntryList(ListEntry);

				//
				// N.B. We move the code here to avoid race condition
				// with IoFlushObjectOnStop(), the remove should hold
				// spinlock to synchronize with it.
				//

				GetSystemTimeAsFileTime(&Object->End);
				InterlockedDecrement(&IoObjectTable.Count);
				InterlockedPushEntrySList(&IoRetiredObjectList, &Object->SListEntry);
				Free = TRUE;
			}
			break;
		}
		ListEntry = ListEntry->Flink;
	}
	
	BtrReleaseSpinLock(&HashEntry->SpinLock);
	return Free;
}

//
// N.B. Caller must ensure Source is valid
//

PIO_OBJECT
IoAllocateObject(
	VOID
	)
{
	PIO_OBJECT Object;
	Object = (PIO_OBJECT)BtrMalloc(sizeof(IO_OBJECT));
	RtlZeroMemory(Object,sizeof(IO_OBJECT));
	BtrInitSpinLock(&Object->Lock, 100);
	return Object;
}

VOID
IoFreeObject(
	_In_ PIO_OBJECT Object 
	)
{
	ASSERT(Object != NULL);
	if (Object->u.File.Name) {
		BtrFree(Object->u.File.Name);
	}
	BtrFree(Object);
}

VOID
IoQueueThreadIrp(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PIO_IRP Irp
	)
{
	ASSERT(Thread != NULL);
	ASSERT(Irp != NULL);

	BtrAcquireSpinLock(&Thread->IrpLock); 
	InsertHeadList(&Thread->IrpList, &Irp->TrackListEntry);
	BtrReleaseSpinLock(&Thread->IrpLock);
}

VOID
IoDequeueThreadIrp(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PIO_IRP Irp
	)
{
	ASSERT(Thread != NULL);
	ASSERT(Irp != NULL);

	BtrAcquireSpinLock(&Thread->IrpLock); 
	RemoveEntryList(&Irp->TrackListEntry);
	BtrReleaseSpinLock(&Thread->IrpLock);
}

VOID
IoCompleteSynchronousIo(
	_In_ PIO_IRP Irp,
	_In_ ULONG Status,
	_In_ ULONG IoStatus,
	_In_ PULONG lpCompleteBytes,
	_In_ LPOVERLAPPED lpOverlapped
	)
{
	ASSERT(Irp != NULL);

	QueryPerformanceCounter(&Irp->End);

	Irp->Flags.Synchronous = 1;
	Irp->Flags.Completed = 1;
	Irp->IoStatus = IoStatus;

	if (lpCompleteBytes) {
		Irp->CompleteBytes = *lpCompleteBytes;
	}
	else if (lpOverlapped) {
		Irp->CompleteBytes = (ULONG)lpOverlapped->InternalHigh;
	}

	DebugTrace("%s, RID=%d, IRP=%p", __FUNCTION__, Irp->RequestId, Irp);

	IoIrpClearInCall(Irp);
	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);
}

VOID
IoCompleteAcceptIo(
	_In_ PIO_IRP Irp,
	_In_ ULONG Status,
	_In_ ULONG IoStatus,
	_In_ PULONG lpCompleteBytes,
	_In_ LPOVERLAPPED lpOverlapped
	)
{
	ASSERT(Irp != NULL);

	QueryPerformanceCounter(&Irp->End);

	Irp->Flags.Synchronous = 1;
	Irp->Flags.Completed = 1;
	Irp->IoStatus = IoStatus;

	if (lpCompleteBytes) {
		Irp->CompleteBytes = *lpCompleteBytes;
	}
	else if (lpOverlapped) {
		Irp->CompleteBytes = (ULONG)lpOverlapped->InternalHigh;
	}

	DebugTrace("%s, AcceptEx: RID=%d, IRP=%p", __FUNCTION__, Irp->RequestId, Irp);
	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);

	//InterlockedPushEntrySList(&IoIrpAcceptSListHead, &Irp->CompleteSListEntry);

	//
	// N.B. The last step is to clear irp in call state, because we have no idea
	// how AcceptEx is used, another thread may already queue the irp into flush
	// list. after this function, the irp can not be touched anymore!
	//

	IoIrpClearInCall(Irp);
}