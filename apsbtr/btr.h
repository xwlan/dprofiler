//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BTR_H_
#define _BTR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"

ULONG
BtrInitialize(
	__in PBTR_PROFILE_OBJECT Object	
	);

VOID
BtrUninitialize(
	VOID
	);

ULONG
BtrInitializeSharedData(
	__in HANDLE SharedDataObject	
	);

ULONG
BtrOnStart(
	__in PBTR_MESSAGE_START Packet,
	__out PBTR_PROFILE_OBJECT *Object 
	);

ULONG
BtrOnStop(
	__in PBTR_MESSAGE_STOP Packet,
	__in PBTR_PROFILE_OBJECT Object
	);

extern PBTR_SHARED_DATA BtrSharedData;
extern PBTR_PROFILE_OBJECT BtrProfileObject;
extern BOOLEAN BtrIsStarted;

#ifdef __cplusplus
}
#endif
#endif