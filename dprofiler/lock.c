#include <windows.h>
#include <intrin.h>
#include "lock.h"

VOID
BtrInitSpinLock(
	IN PBTR_SPINLOCK Lock
	)
{
	Lock->Acquired = 0;
	Lock->OwnerThreadId = 0;
}

VOID
BtrAcquireSpinLock(
	IN PBTR_SPINLOCK Lock
	)
{
	LONG Acquired;
	LONG OwnerId;
	LONG ThreadId;

	ThreadId = GetCurrentThreadId();
	OwnerId = InterlockedCompareExchange((volatile LONG *)&Lock->OwnerThreadId,
										 ThreadId,
										 ThreadId);
	if (OwnerId == ThreadId) {
		return;
	}

	while (TRUE) {
		Acquired = InterlockedBitTestAndSet((volatile LONG *)&Lock->Acquired, 0);
		if (Acquired != TRUE) {
			Lock->OwnerThreadId = ThreadId;
			break;
		}
		while (Lock->Acquired != 0) {
			SwitchToThread();
		}
	}	
}

VOID
BtrReleaseSpinLock(
	IN PBTR_SPINLOCK Lock
	)
{	
	Lock->OwnerThreadId = 0;
	_InterlockedAnd((volatile LONG *)&Lock->Acquired, 0);
}