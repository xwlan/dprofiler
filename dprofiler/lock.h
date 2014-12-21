//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _LOCK_H_
#define _LOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"

typedef enum _BTR_SPINLOCK_FLAG {
	ListEntryInvalid = 0x00000001,  
} BTR_SPINLOCK_FLAG, *PBTR_SPINLOCK_FLAG;

VOID
BtrInitLock(
	IN PBTR_LOCK Lock
	);

VOID
BtrInitLockEx(
	IN PBTR_LOCK Lock,
	IN ULONG SpinCount
	);

VOID
BtrAcquireLock(
	IN PBTR_LOCK Lock
	);

VOID
BtrReleaseLock(
	IN PBTR_LOCK Lock
	);

VOID
BtrDeleteLock(
	IN PBTR_LOCK Lock
	);

VOID
BtrInitSpinLock(
	IN PBTR_SPINLOCK Lock,
	IN ULONG SpinCount
	);

VOID
BtrAcquireSpinLock(
	IN PBTR_SPINLOCK Lock
	);

VOID
BtrReleaseSpinLock(
	IN PBTR_SPINLOCK Lock
	);

#ifdef __cplusplus
}
#endif

#endif