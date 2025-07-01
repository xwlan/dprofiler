#ifndef _CCR_PATCH_H_
#define _CCR_PATCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"

//
// Routines for CCR profile work under IAT mode
//

VOID
CcrIatInitialize(
	VOID
	);

VOID
CcrIatApplyPatch(
	VOID
	);

VOID
WINAPI
CcrIatEnterCriticalSection(
	_Inout_ LPCRITICAL_SECTION lpCriticalSection
	);

VOID
WINAPI
CcrIatLeaveCriticalSection(
	_Inout_ LPCRITICAL_SECTION lpCriticalSection
	);

BOOL
WINAPI
CcrIatTryEnterCriticalSection(
	_Inout_ LPCRITICAL_SECTION lpCriticalSection
	);

VOID
WINAPI
CcrIatReleaseSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
	);

VOID
WINAPI
CcrIatReleaseSRWLockShared(
	_Inout_ PSRWLOCK SRWLock
	);

VOID
WINAPI
CcrIatAcquireSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
	);

VOID
WINAPI
CcrIatAcquireSRWLockShared(
	_Inout_ PSRWLOCK SRWLock
	);

BOOLEAN
WINAPI
CcrIatTryAcquireSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
	);

BOOLEAN
WINAPI
CcrIatTryAcquireSRWLockShared(
	_Inout_ PSRWLOCK SRWLock
	);

BOOL WINAPI
CcrIatSleepConditionVariableSRW(
	_Inout_ PCONDITION_VARIABLE ConditionVariable,
	_Inout_ PSRWLOCK            SRWLock,
	_In_    DWORD               dwMilliseconds,
	_In_    ULONG               Flags
	);

#ifdef __cplusplus
}
#endif
#endif