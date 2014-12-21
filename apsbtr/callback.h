//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "hal.h"
#include "trap.h"
#include "thread.h"

ULONG
BtrRegisterCallback(
	IN PBTR_CALLBACK Callback
	);

ULONG
BtrUnregisterCallback(
	IN PBTR_CALLBACK Callback
	);

ULONG
BtrApplyCallback(
	IN PBTR_CALLBACK Callback 
	);

PVOID 
BtrGetCallbackDestine(
	IN PBTR_CALLBACK Callback
	);

BOOLEAN
BtrIsCallbackComplete(
	IN PBTR_CALLBACK Callback,
	IN ULONG Count
	);

ULONG
BtrCreateHotpatch(
	__in PBTR_CALLBACK Callback,
	__in BTR_HOTPATCH_ACTION Action
	);

ULONG
BtrRegisterCallbackEx(
	__in PBTR_CALLBACK Callback
	);

ULONG
BtrUnregisterCallbackEx(
	__in PBTR_CALLBACK Callback
	);

#ifdef __cplusplus
}
#endif
#endif