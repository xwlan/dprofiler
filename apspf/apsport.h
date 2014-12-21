//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _APSPORT_H_
#define _APSPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef struct _APS_PORT {
	LIST_ENTRY ListEntry;
	WCHAR Name[64];
	ULONG ProcessId;
	ULONG ThreadId;
	HANDLE Object;
	PVOID Buffer;
	ULONG BufferLength;
	HANDLE CompleteEvent; 
	OVERLAPPED Overlapped;
} APS_PORT, *PAPS_PORT;

PAPS_PORT
ApsCreatePort(
	__in ULONG ProcessId,
	__in ULONG BufferLength
	);

ULONG
ApsConnectPort(
	__in PAPS_PORT Port
	);

ULONG
ApsClosePort(
	__in PAPS_PORT Port	
	);

ULONG
ApsGetMessage(
	__in PAPS_PORT Port,
	__out PBTR_MESSAGE_HEADER Packet,
	__in ULONG BufferLength,
	__out PULONG CompleteBytes
	);

ULONG
ApsSendMessage(
	__in PAPS_PORT Port,
	__in PBTR_MESSAGE_HEADER Packet,
	__out PULONG CompleteBytes 
	);

ULONG
ApsWaitPortIoComplete(
	__in PAPS_PORT Port,
	__in ULONG Milliseconds,
	__out PULONG CompleteBytes
	);

#ifdef __cplusplus
}
#endif
#endif