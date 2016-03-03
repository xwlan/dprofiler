//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "queue.h"
#include "list.h"
#include "thread.h"
#include "util.h"
#include "heap.h"
#include "status.h"

ULONG
BtrCreateQueue(
	__in PBTR_QUEUE_OBJECT Object
	)
{
	HANDLE QueueHandle;

	QueueHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!QueueHandle) {
		return GetLastError();
	}

	Object->Object = QueueHandle;
	InitializeListHead(&Object->ListEntry);

	return S_OK;
}

ULONG
BtrDestroyQueue(
	__in PBTR_QUEUE_OBJECT Object
	)
{
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PBTR_QUEUE_PACKET Packet;

	InitializeListHead(&ListHead);
	BtrDeQueuePacketList(Object, &ListHead);
	CloseHandle(Object->Object);

	while (IsListEmpty(&ListHead) != TRUE) {
		ListEntry = RemoveHeadList(&ListHead);
		Packet = CONTAINING_RECORD(ListEntry, BTR_QUEUE_PACKET, ListEntry);
		BtrFree(Packet);
	}

	return S_OK;
}

ULONG
BtrQueuePacket(
	__in PBTR_QUEUE_OBJECT Object,
	__in PBTR_QUEUE_PACKET Notification
	)
{
	ULONG Status;

	Status = PostQueuedCompletionStatus(Object->Object, 
		                                0, 
										(ULONG_PTR)Notification, 
										NULL);
	
	if (Status) {
		return S_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
BtrDeQueuePacket(
	__in PBTR_QUEUE_OBJECT Object,
	__in ULONG Milliseconds,
	__out PBTR_QUEUE_PACKET *Notification
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped = UlongToPtr(1);

	*Notification = NULL;
	Status = GetQueuedCompletionStatus(Object->Object, 
				                       &TransferedLength, 
									   &CompletionKey, 
									   &Overlapped, Milliseconds);
	
	if (Status) {
		*Notification = (PBTR_QUEUE_PACKET)CompletionKey;
		return S_OK;
	}

	//
	// This is timeout case
	//

	if (!Status && Overlapped == NULL) {
		return S_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
BtrDeQueuePacketList(
	__in PBTR_QUEUE_OBJECT Object,
	__out PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;
	PBTR_QUEUE_PACKET Notification;

	Status = TRUE;
	InitializeListHead(ListHead);

	while (Status) {
		Status = GetQueuedCompletionStatus(Object->Object, 
					                       &TransferedLength, 
										   &CompletionKey, 
										   &Overlapped, 0);
		
		if (Status) {
			Notification = (PBTR_QUEUE_PACKET)CompletionKey;
			InsertTailList(ListHead, &Notification->ListEntry);
		}
	}

	Status = GetLastError();
	return Status;
}