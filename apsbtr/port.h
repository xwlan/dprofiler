//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PORT_H_
#define _PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"

typedef struct _BTR_PORT_OBJECT {
	LIST_ENTRY ListEntry;
	WCHAR Name[64];
	ULONG ProcessId;
	ULONG ThreadId;
	HANDLE Object;
	PVOID Buffer;
	ULONG BufferLength;
	HANDLE CompleteEvent; 
	OVERLAPPED Overlapped;
} BTR_PORT_OBJECT, *PBTR_PORT_OBJECT;

ULONG
BtrCreatePort(
	__in PBTR_PORT_OBJECT Port	
	);

ULONG
BtrAcceptPort(
	__in PBTR_PORT_OBJECT Port	
	);

ULONG
BtrDisconnectPort(
	__in PBTR_PORT_OBJECT Port	
	);

ULONG
BtrClosePort(
	__in PBTR_PORT_OBJECT Port	
	);

ULONG
BtrWaitPortIoComplete(
	__in PBTR_PORT_OBJECT Port,
	__in ULONG Milliseconds,
	__out PULONG CompleteBytes
	);

ULONG
BtrGetMessage(
	__in PBTR_PORT_OBJECT Port,
	__out PBTR_MESSAGE_HEADER Packet,
	__in ULONG BufferLength,
	__out PULONG CompleteBytes
	);

ULONG
BtrSendMessage(
	__in PBTR_PORT_OBJECT Port,
	__in PBTR_MESSAGE_HEADER PacketHeader,
	__out PULONG CompleteBytes 
	);

extern BTR_PORT_OBJECT BtrPortObject;

#ifdef __cplusplus
}
#endif
#endif