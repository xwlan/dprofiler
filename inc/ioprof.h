// 
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _IO_PROF_H_
#define _IO_PROF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "callback.h"
#include "iodefs.h"

#define MAX_IO_BUCKET  4093


//
// IO_IRP, IO_IRP_TRACK must be MEMORY_ALLOCATION_ALIGNMENT bytes aligned 
// required by SLIST_ENTRY, allocated by BtrMallocAligned, free by BtrFreeAligned.
//

#pragma pack(push, MEMORY_ALLOCATION_ALIGNMENT)

typedef struct DECLSPEC_ALIGN(16) _IO_IRP {

	//
	// N.B. We need manual padding on x86, otherwise CompleteSListEntry is not 8 bytes aligned.
	//

	SLIST_ENTRY PendingSListEntry;     // for IoIrpPendingSListHead;
#if defined(_M_IX86)
	PVOID Padding;
#endif

	SLIST_ENTRY CompleteSListEntry;    // for IoIrpCompleteSListHead;
	LIST_ENTRY TrackListEntry;         // for IoIrpTrackListHead
	LIST_ENTRY InCallListEntry;        // for IoIrpInCallListHead

	volatile LONG InCall;
	volatile ULONG LastError;

	ULONG IrpTag;
	ULONG RequestId;
	ULONG ObjectId;
	ULONG StackId;
	ULONG IoStatus;
	ULONG RequestBytes;
	ULONG CompleteBytes;
	ULONG RequestThreadId;
	ULONG CompleteThreadId;

	IO_OPERATION Operation;
	IO_CALLBACK_TYPE CallType;
	IO_FLAGS Flags;

	ULONG ControlCode;
	ULONG_PTR ControlData;
	union {
		PVOID ControlContext;
		SOCKET SkListen;
	};

	HANDLE Object;
	FILETIME Time;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;

	OVERLAPPED Overlapped;
	LPOVERLAPPED Original;
	PVOID ApcCallback;

} IO_IRP, *PIO_IRP;

typedef struct _IO_IRP_TRACK {
	SLIST_ENTRY SListEntry;
	PIO_IRP Irp;
	ULONG IoStatus;
} IO_IRP_TRACK, *PIO_IRP_TRACK;

#pragma pack(pop)

//
// IO_IRP_TABLE track all allocated irp, it can only be
// accessed from flush thread, no lock protection.
//

#define IO_IRP_BUCKET_COUNT 1024

//
// Irp bucket hash is modular of its request id by 1024
//

#define IO_IRP_BUCKET(_I) \
	((IO_IRP_BUCKET_COUNT) % _I->RequestId)

typedef struct _IO_IRP_TABLE {
	ULONG QueueCount;
	ULONG PendingCount;
	LIST_ENTRY ListHead[IO_IRP_BUCKET_COUNT];
	LIST_ENTRY InCallListHead; 
} IO_IRP_TABLE, *PIO_IRP_TABLE;

//
// N.B. IO_IRP_ON_DISK is the on disk equalent of 
// IO_IRP structure
//

#pragma pack(push, 1)
#pragma pack(pop)

//
// N.B. When GetOverlappedResult, or GetQueueCompleteStatus/Ex returns,
// an IO_COMPLETION_PACKET is assembled, and queue to BtrIoCompletionList
// of current CPU, Overlapped is the only key to distinguish which request
// map to which completion packet, this is only for asynchronous IO.
//

typedef struct _IO_COMPLETION_PACKET {
	union {
		LIST_ENTRY ListEntry;
		SLIST_ENTRY SListEntry;
	};
	OVERLAPPED *Overlapped; 
	ULONG IoStatus;
	ULONG CompleteBytes;
	ULONG CompleteThreadId;
	LARGE_INTEGER End;
} IO_COMPLETION_PACKET, *PIO_COMPLETION_PACKET;

//
// N.B. IO_IOCP_PACKET define an overlapped
// wrapper to help interceptor determine whether
// it's an packet posted to IOCP by PostQueuedCompletionStatus,
// interceptor will ignore this kind of calls, only
// record IO completion packet. note that this structure
// MUST be allocated from IO_OVERLAPPED, since we depend
// on address range to convert a recived lpOverlapped
// structure.
//

//
// Each IO_OVERLAPPED hold PACKET_SLOT_COUNT
// entries, scan the BitMap to find the empty
// slot, if the BitMap is full, allocate a new
// IO_OVERLAPPED, and chain into IoOverlappedList,
// this list is protected by a lock.
//

#define PACKET_SLOT_COUNT 512
C_ASSERT(PACKET_SLOT_COUNT % 32 == 0);

#pragma pack(push, 1)
typedef struct _IO_IOCP_PACKET {
	LPOVERLAPPED Overlapped;
} IO_IOCP_PACKET, *PIO_IOCP_PACKET;

typedef struct _IO_OVERLAPPED {
	LIST_ENTRY ListEntry;
	BTR_BITMAP BitMap;
	IO_IOCP_PACKET Packet[PACKET_SLOT_COUNT];
} IO_OVERLAPPED, *PIO_OVERLAPPED;
#pragma pack(pop)

//
// Maximum request count supported, when the threshod arrive,
// automatically stop profiling, default it's 1 million, 
// however, it's configurable via BtrSetConfiguration.
//

extern ULONG BtrIoMaximumRequestCount;


ULONG
IoInitialize(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	);

ULONG
IoStartProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoStopProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoUnload(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
IoThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	);

BOOLEAN
IoIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	);

ULONG
IoQueryHotpatch(
	__in PBTR_PROFILE_OBJECT Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	);

//
// IO profie internal routines
//

BOOLEAN
IoCloseFiles(
	VOID
	);

ULONG CALLBACK
IoProfileProcedure(
	__in PVOID Context
	);

ULONG
IoFlushObject(
	VOID
	);

ULONG
IoFlushIrp(
	VOID
	);

ULONG
IoFlushRecord(
	VOID
	);

ULONG
IoFlushIrpOnStop(
	VOID
	);

ULONG
IoFlushObjectOnStop(
	VOID
	);

ULONG
IoFlushRecordOnStop(
	VOID
	);

BOOLEAN
IoIsOrphanedIrp(
	_In_ PIO_IRP Irp,
	_In_ PLARGE_INTEGER Current
	);

BOOLEAN
IoDllCanUnload(
	VOID
	);

VOID
IoInitIrpTable(
	VOID
	);

VOID
BtrIoQueueCompletion(
	__in LPOVERLAPPED Overlapped,
	__in PVOID Key,
	__in ULONG CompleteBytes,
	__in ULONG IoStatus
	);

PIO_IRP
IoInsertCompletion(
	_In_ IO_COMPLETION_MODE Mode,
	_In_ PIO_IRP Irp,
	_In_ LPOVERLAPPED Overlapped,
	_In_ ULONG CompleteBytes,
	_In_ ULONG IoStatus,
	_In_ ULONG ThreadId,
	_In_ LARGE_INTEGER *End
	);

VOID
IoUpdateCounters(
	_In_ PIO_IRP Irp
	);

VOID
IoInsertRequest(
	__in PIO_IRP Packet
	);

BOOLEAN
BtrIoMatchCompletion(
	__in PIO_COMPLETION_PACKET Completion,
	__in ULONG Bucket
	);

VOID 
IoQueueFlushList(
	_In_ PIO_IRP Irp
	);

VOID
IoUpdateRequestCounters(
	_In_ PIO_IRP Irp
	);

VOID
IoUpdateCompleteCounters(
	_In_ PIO_IRP Irp
	);

PIO_IRP
IoAllocateIrp(
	_In_ PIO_OBJECT Object	
	);

VOID
IoFreeIrp(
	_In_ PIO_IRP Irp
	);

PIO_IRP_TRACK
IoAllocateIrpTrack(
	_In_ PIO_IRP Irp
	);

VOID
IoFreeIrpTrack(
	_In_ PIO_IRP_TRACK Track
	);

VOID
IoIrpClearInCall(
	_In_ PIO_IRP Irp
	);

BOOLEAN
IoIsIrpInCall(
	_In_ PIO_IRP Irp
	);

BOOLEAN
IoCopyOverlapped(
	_In_ LPOVERLAPPED From,
	_Inout_ LPOVERLAPPED To
	);

PIO_IRP
IoOverlappedToIrp(
	_In_ LPOVERLAPPED Overlapped
	);

PIO_IRP
IoGetIrpFromInternal(
	_In_ LPOVERLAPPED Overlapped
	);

BOOLEAN
IoRefObjectCheckOverlapped(
	_In_ HANDLE Handle,
	_In_ HANDLE_TYPE Type,
	_In_ LPOVERLAPPED lpOverlapped,
	_Out_ PIO_OBJECT *Object,
	_Out_ BOOLEAN *IsOverlapped,
	_Out_ BOOLEAN *SkipOnSuccess
	);

BOOLEAN
IoIsObjectSkipOnSuccess(
	_In_ PIO_OBJECT Object
	);
LPOVERLAPPED
IoHijackOverlapped(
	_In_ PIO_IRP Irp,
	_In_ LPOVERLAPPED lpOverlapped
	);

BOOLEAN
IoCheckOverlapped(
	_In_ HANDLE Handle,
	_In_ HANDLE_TYPE Type,
	_In_ LPOVERLAPPED lpOverlapped
	);

VOID
IoQuerySocketAddress(
	_In_ PIO_OBJECT Object,
	_In_ SOCKET s
	);

VOID
IoCompleteSynchronousIo(
	_In_ PIO_IRP Irp,
	_In_ ULONG Status,
	_In_ ULONG IoStatus,
	_Inout_ PULONG lpCompleteBytes,
	_Inout_ LPOVERLAPPED lpOverlapped
	);

VOID
IoCompleteAcceptIo(
	_In_ PIO_IRP Irp,
	_In_ ULONG Status,
	_In_ ULONG IoStatus,
	_In_ PULONG lpCompleteBytes,
	_In_ LPOVERLAPPED lpOverlapped
	);

VOID CALLBACK 
IoFileCompleteCallback(
	_In_    DWORD        dwErrorCode,
	_In_    DWORD        dwNumberOfBytesTransfered,
	_Inout_ LPOVERLAPPED lpOverlapped
	);

ULONG
IoAcquireObjectId(
	VOID
	);

ULONG
IoAcquireRequestId(
	VOID
	);

PBTR_USER_APC
IoDequeueApcByOverlapped(
	__in PLIST_ENTRY ApcList,
	__in LPOVERLAPPED lpOverlapped
	);

BOOLEAN
IoIsPostedOverlapped(
	__in LPOVERLAPPED Overlapped
	);

PIO_IOCP_PACKET
IoAllocateIocpPacket(
	VOID
	);

VOID
IoFreeIocpPacket(
	__in PIO_IOCP_PACKET Packet
	);

extern BTR_CALLBACK IoCallback[];
extern ULONG IoCallbackCount;

extern IO_IRP_TABLE IoIrpTable;
extern SLIST_HEADER IoIrpPendingSListHead;
extern SLIST_HEADER IoIrpCompleteSListHead;
extern BTR_SPINLOCK IoPerformanceLock;

extern volatile ULONG IoRequestId;
extern volatile ULONG IoObjectId;

#ifdef __cplusplus
}
#endif
#endif