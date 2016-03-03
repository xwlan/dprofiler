//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "port.h"


ULONG CALLBACK
BtrMessageProcedure(
	__in PVOID Context
	);

ULONG
BtrOnMessage(
	__in PBTR_MESSAGE_HEADER Packet
	);

ULONG
BtrOnMessageStart(
	__in PBTR_MESSAGE_START Packet
	);

ULONG
BtrOnMessageStop(
	__in PBTR_MESSAGE_STOP Packet
	);

ULONG
BtrOnMessagePause(
	__in PBTR_MESSAGE_PAUSE Packet
	);

ULONG
BtrOnMessageResume(
	__in PBTR_MESSAGE_RESUME Packet
	);

ULONG
BtrOnMessageMark(
	__in PBTR_MESSAGE_MARK Packet
	);

ULONG
BtrOnMessageQuery(
	__in PBTR_MESSAGE_QUERY Packet
	);


extern ULONG BtrMessageThreadId;
extern HANDLE BtrMessageThreadHandle;


#ifdef __cplusplus
}
#endif
#endif 

