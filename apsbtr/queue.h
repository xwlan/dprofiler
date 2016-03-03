//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "thread.h"

typedef struct _BTR_QUEUE_PACKET {
	LIST_ENTRY ListEntry;
	ULONG Type;
	ULONG ThreadId;
	PVOID Packet;
	HANDLE CompleteEvent;
} BTR_QUEUE_PACKET, *PBTR_QUEUE_PACKET;

typedef struct _BTR_QUEUE_OBJECT {
	LIST_ENTRY ListEntry;
	HANDLE Object;
	PVOID Context;
} BTR_QUEUE_OBJECT, *PBTR_QUEUE_OBJECT;

ULONG
BtrCreateQueue(
	__in PBTR_QUEUE_OBJECT Object
	);

ULONG
BtrDestroyQueue(
	__in PBTR_QUEUE_OBJECT Object
	);

ULONG
BtrQueuePacket(
	__in PBTR_QUEUE_OBJECT Object,
	__in PBTR_QUEUE_PACKET Notification
	);

ULONG
BtrDeQueuePacket(
	__in PBTR_QUEUE_OBJECT Object,
	__in ULONG Milliseconds,
	__out PBTR_QUEUE_PACKET *Notification
	);

ULONG
BtrDeQueuePacketList(
	__in PBTR_QUEUE_OBJECT Object,
	__out PLIST_ENTRY ListHead
	);

#ifdef __cplusplus
}
#endif

#endif  // _QUEUE_H_