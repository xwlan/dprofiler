//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//
//

#ifndef _IO_PROF_H_
#define _IO_PROF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "callback.h"

extern BTR_CALLBACK IoCallback[];
extern ULONG IoCallbackCount;

PBTR_CALLBACK FORCEINLINE
BtrGetIoCallback(
	IN ULONG Ordinal
	)
{
	return &IoCallback[Ordinal];
}

ULONG
BtrRegisterIoCallbacks(
	VOID
	);

ULONG
BtrUnregisterIoCallbacks(
	VOID
	);

#ifdef __cplusplus
}
#endif
#endif