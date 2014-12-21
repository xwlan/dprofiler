//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"

typedef struct _BTR_TIMER {
	LIST_ENTRY ListEntry;
	HANDLE Handle;
	PTIMERAPCROUTINE Callback;
	PVOID Context;
	ULONG TimerId;
	ULONG Period;
} BTR_TIMER, *PBTR_TIMER;

PBTR_TIMER
BtrCreateTimer(
	__in ULONG Expire,
	__in ULONG Period,
	__in PTIMERAPCROUTINE Callback,
	__in PVOID Context
	);

VOID
BtrDeleteTimer(
	__in PBTR_TIMER Timer
	);

BOOLEAN
BtrActiveTimer(
	__in PBTR_TIMER Timer,
	__in BOOLEAN Activate
	);

BOOLEAN
BtrAdjustTimerPeriod(
	__in PBTR_TIMER Timer,
	__in ULONG Period
	);

VOID
BtrInitializeTimerList(
	VOID
	);

VOID
BtrDestroyTimerList(
	VOID
	);

//
// Runtime timer object list
//

extern LIST_ENTRY BtrTimerListHead;

#ifdef __cplusplus
}
#endif

#endif