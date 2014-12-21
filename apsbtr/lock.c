//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "lock.h"
#include "list.h"
#include "hal.h"
#include "util.h"

VOID
BtrInitLock(
	IN PBTR_LOCK Lock
	)
{
	InitializeCriticalSection(Lock);
}

VOID
BtrInitLockEx(
	IN PBTR_LOCK Lock,
	IN ULONG SpinCount
	)
{
	InitializeCriticalSectionAndSpinCount(Lock, SpinCount);
}

VOID
BtrAcquireLock(
	IN PBTR_LOCK Lock
	)
{
	EnterCriticalSection(Lock);
}

VOID
BtrReleaseLock(
	IN PBTR_LOCK Lock
	)
{	
	LeaveCriticalSection(Lock);
}

VOID
BtrDeleteLock(
	IN PBTR_LOCK Lock
	)
{
	DeleteCriticalSection(Lock);
}
	
VOID
BtrInitSpinLock(
	IN PBTR_SPINLOCK Lock,
	IN ULONG SpinCount
	)
{
	Lock->Acquired = 0;
	Lock->ThreadId = 0;
	Lock->SpinCount = SpinCount;
}

VOID
BtrAcquireSpinLock(
	IN PBTR_SPINLOCK Lock
	)
{
	ULONG Acquired;
	ULONG ThreadId;
	ULONG OwnerId;
	ULONG Count;
	ULONG SpinCount;

	ThreadId = GetCurrentThreadId();
	OwnerId = ReadForWriteAccess(&Lock->ThreadId);
	if (OwnerId == ThreadId) {
	    return;
	}

	SpinCount = Lock->SpinCount;

	while (TRUE) {
		
		Acquired = InterlockedBitTestAndSet((volatile LONG *)&Lock->Acquired, 0);
		if (Acquired != 1) {
			Lock->ThreadId = ThreadId;
			break;
		}

		Count = 0;

		do {

			YieldProcessor();
			Acquired = ReadForWriteAccess(&Lock->Acquired);

			Count += 1;
			if (Count >= SpinCount) { 
				SwitchToThread();
				break;
			}
		} while (Acquired != 0);
	}	
}

VOID
BtrReleaseSpinLock(
	IN PBTR_SPINLOCK Lock
	)
{	
	Lock->ThreadId = 0;
	_InterlockedAnd((volatile LONG *)&Lock->Acquired, 0);
}