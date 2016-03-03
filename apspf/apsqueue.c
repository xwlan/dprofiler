//
// lan.john@gmail.com 
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "aps.h"
#include "apsdefs.h"
#include "apsqueue.h"

ULONG
ApsCreateQueue(
	__out PAPS_QUEUE *Object
	)
{
	HANDLE QueueHandle;
	PAPS_QUEUE Queue;

	QueueHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!QueueHandle) {
		return GetLastError();
	}

	Queue = (PAPS_QUEUE)ApsMalloc(sizeof(APS_QUEUE));
	Queue->ObjectHandle = QueueHandle;
	InitializeListHead(&Queue->ListEntry);

	*Object = Queue;
	return APS_STATUS_OK;
}

ULONG
ApsCloseQueue(
	__in PAPS_QUEUE Object
	)
{
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PAPS_QUEUE_PACKET Packet;

	InitializeListHead(&ListHead);
	ApsDeQueuePacketList(Object, &ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Packet = CONTAINING_RECORD(ListEntry, APS_QUEUE_PACKET, ListEntry);

		if (FlagOn(Packet->Flag, PACKET_FLAG_FREE)) {
			ApsFree(Packet);
		}
	}

	CloseHandle(Object->ObjectHandle);
	Object->ObjectHandle = NULL;
	return APS_STATUS_OK;
}

ULONG
ApsQueuePacket(
	__in PAPS_QUEUE Object,
	__in PAPS_QUEUE_PACKET Packet
	)
{
	ULONG Status;

	Status = PostQueuedCompletionStatus(Object->ObjectHandle, 
		                                0, 
										(ULONG_PTR)Packet, 
										NULL);
	
	if (Status) {
		return APS_STATUS_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
ApsDeQueuePacket(
	__in PAPS_QUEUE Object,
	__out PAPS_QUEUE_PACKET *Packet
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;

	*Packet = NULL;
	Status = GetQueuedCompletionStatus(Object->ObjectHandle, 
				                       &TransferedLength, 
									   &CompletionKey, 
									   &Overlapped, INFINITE);
	
	if (Status) {
		*Packet = (PAPS_QUEUE_PACKET)CompletionKey;
		return APS_STATUS_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
ApsDeQueuePacketEx(
	__in PAPS_QUEUE Object,
	__in ULONG Milliseconds,
	__out PAPS_QUEUE_PACKET *Packet
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;

	*Packet = NULL;
	Status = GetQueuedCompletionStatus(Object->ObjectHandle, 
				                       &TransferedLength, 
									   &CompletionKey, 
									   &Overlapped, Milliseconds);
	
	if (Status) {
		*Packet = (PAPS_QUEUE_PACKET)CompletionKey;
		return APS_STATUS_OK;
	} 
	
	else if (Overlapped == NULL) {
	
		//
		// N.B. If it's timeout, NT null overlapped structure and return FALSE
		//

		return APS_STATUS_TIMEOUT;

	}

	Status = GetLastError();
	return Status;
}

ULONG
ApsDeQueuePacketList(
	__in PAPS_QUEUE Object,
	__out PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;
	PAPS_QUEUE_PACKET Packet;

	Status = TRUE;
	InitializeListHead(ListHead);

	while (Status) {
		Status = GetQueuedCompletionStatus(Object->ObjectHandle, 
					                       &TransferedLength, 
										   &CompletionKey, 
										   &Overlapped, 0);
		
		if (Status) {
			Packet = (PAPS_QUEUE_PACKET)CompletionKey;
			InsertTailList(ListHead, &Packet->ListEntry);
		}
	}

	Status = GetLastError();
	return Status;
}
