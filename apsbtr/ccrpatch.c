//
// Author: xwlan@outlook.com
// Copyright(C) 2009-2025
//

#include "apsbtr.h"
#include "callback.h"
#include "heap.h"
#include "lock.h"
#include "thread.h"
#include "hal.h"
#include "stacktrace.h"
#include "util.h"
#include "ccrprof.h"
#include "iatpatch.h"
#include "ccrpatch.h"

BTR_IAT_PATCH CcrIatPatch[] = {
	{ "kernel32.dll", "ExitProcess", CcrExitProcessCallback },
	{ "kernel32.dll", "EnterCriticalSection", CcrIatEnterCriticalSection },
	{ "kernel32.dll", "TryEnterCriticalSection", CcrIatTryEnterCriticalSection },
	{ "kernel32.dll", "LeaveCriticalSection", CcrIatLeaveCriticalSection },
	{ "kernel32.dll", "AcquireSRWLockExclusive", CcrIatAcquireSRWLockExclusive },
	{ "kernel32.dll", "AcquireSRWLockShared", CcrIatAcquireSRWLockShared },
	{ "kernel32.dll", "TryAcquireSRWLockExclusive", CcrIatTryAcquireSRWLockExclusive },
	{ "kernel32.dll", "TryAcquireSRWLockShared", CcrIatTryAcquireSRWLockShared },
	{ "kernel32.dll", "ReleaseSRWLockExclusive", CcrIatReleaseSRWLockExclusive },
	{ "kernel32.dll", "ReleaseSRWLockShared", CcrIatReleaseSRWLockShared },
	{ "kernel32.dll", "SleepConditionVariableSRW", CcrIatSleepConditionVariableSRW },
};

ULONG CcrIatPatchCount = ARRAYSIZE(CcrIatPatch);

FORCEINLINE
PBTR_IAT_PATCH
CcrIatCurrentPatch(
	IN CCR_CALLBACK_TYPE Type
	)
{
	return &CcrIatPatch[Type];
}

VOID
CcrIatApplyPatch(
	VOID
	)
{	
	CcrIatInitialize();
	BtrApplyIatPatch(CcrIatPatch, CcrIatPatchCount, TRUE);
}

VOID
CcrIatInitialize(
	VOID
	)
{
	ULONG i;
	PBTR_IAT_PATCH Patch;
	HMODULE Handle;

	Handle = GetModuleHandleA("kernel32.dll");
	CcrIatPatch[0].Address = GetProcAddress(Handle, "ExitProcess");

	for (i = 1; i < CcrIatPatchCount; i++) {
		Patch = &CcrIatPatch[i];
		Patch->Address = GetProcAddress(Handle, Patch->Function);
		ASSERT(Patch->Address != NULL);
	}
}

//
// _T, Thread object
// _A, Target address
// _R, PBTR_STACK_RECORD *
//

#define CcrIatCaptureStackTrace(_T, _A, _R)\
{\
	ULONG_PTR *Pc;\
	ULONG Depth;\
	ULONG StackId;\
	Pc = (ULONG_PTR *)_T->Buffer;\
	Pc[0] = (ULONG_PTR)CALLER;\
	Pc[1] = 0;\
	Pc[2] = 0;\
	BtrEnterExemptionRegion(_T);\
	*(_R) = BtrCaptureStackTracePerThread(_T, (PVOID *)_T->Buffer,\
							MAX_STACK_DEPTH, BtrGetFramePointer(),\
							_A, &StackId, &Depth);\
	BtrLeaveExemptionRegion(_T);\
}

VOID
WINAPI
CcrIatEnterCriticalSection(
	_Inout_ LPCRITICAL_SECTION lpCriticalSection
	)
{
	PBTR_THREAD_OBJECT Thread;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record;
	LARGE_INTEGER Start, End;
	PBTR_IAT_PATCH Patch;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		EnterCriticalSection(lpCriticalSection);
		return;
	}

	if (!CcrShouldTrackLock(CALLER, lpCriticalSection)) {
		EnterCriticalSection(lpCriticalSection);
		return;
	}

	Record = NULL;
	Patch = CcrIatCurrentPatch(_CcrEnterCriticalSection);
	CcrIatCaptureStackTrace(Thread, Patch->Address, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//

	Track = CcrMarkThreadInAcquire(Thread, lpCriticalSection, CCR_LOCK_CS);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	EnterCriticalSection(lpCriticalSection);
	QueryPerformanceCounter(&End);

	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_ENTER_CS, &Start, &End, TRUE);

	DebugTrace2("TID:%d ACQUIRE CS %p", Thread->ThreadId, lpCriticalSection);
}

BOOL
WINAPI
CcrIatTryEnterCriticalSection(
	_Inout_ LPCRITICAL_SECTION lpCriticalSection
	)
{
	PBTR_THREAD_OBJECT Thread;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	BOOL Acquired;
	PBTR_STACK_RECORD Record;
	PBTR_IAT_PATCH Patch;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return TryEnterCriticalSection(lpCriticalSection);
	}

	if (!CcrShouldTrackLock(CALLER, lpCriticalSection)) {
		return TryEnterCriticalSection(lpCriticalSection);
	}

	Record = NULL;
	Patch = CcrIatCurrentPatch(_CcrTryEnterCriticalSection);
	CcrIatCaptureStackTrace(Thread, Patch->Address, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//

	Track = CcrMarkThreadInAcquire(Thread, lpCriticalSection, CCR_LOCK_CS);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	Acquired = TryEnterCriticalSection(lpCriticalSection);
	QueryPerformanceCounter(&End);

	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_TRY_ENTER_CS, &Start, &End, Acquired);
	DebugTrace2("TID:%d TRY ACQUIRE CS %p, RESULT=%d", Thread->ThreadId, lpCriticalSection, Acquired);
	return Acquired;
}

VOID
WINAPI
CcrIatLeaveCriticalSection(
	_Inout_ LPCRITICAL_SECTION lpCriticalSection
	)
{
	PBTR_THREAD_OBJECT Thread;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;

	LeaveCriticalSection(lpCriticalSection);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return;
	}

	//
	// If we never track the lock acquisition, skip it
	//

	Track = CcrLookupLockTrack(Thread, lpCriticalSection, FALSE);
	if (!Track) {
		return;
	}

	QueryPerformanceCounter(&End);

	if (!Track->LockOwner) {
		DebugTrace2("TID:%d WARNING => LEAVE CS %p NOT LOCK OWNER", Thread->ThreadId, lpCriticalSection);
		return;
	}

	CcrTrackLockRelease(Thread, Track, CCR_PROBE_LEAVE_CS, &End);
	DebugTrace2("TID:%d LEAVE CS %p", Thread->ThreadId, lpCriticalSection);
}

VOID
WINAPI
CcrIatReleaseSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
	)
{
	PBTR_THREAD_OBJECT Thread;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;

	ReleaseSRWLockExclusive(SRWLock);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return;
	}

	//
	// If we never track the lock acquisition, skip it
	//

	Track = CcrLookupLockTrack(Thread, SRWLock, FALSE);
	if (!Track) {
		return;
	}

	QueryPerformanceCounter(&End);
	CcrTrackLockRelease(Thread, Track, CCR_PROBE_RELEASE_SRW_EXCLUSIVE, &End);
}

VOID
WINAPI
CcrIatReleaseSRWLockShared(
	_Inout_ PSRWLOCK SRWLock
	)
{
	PBTR_THREAD_OBJECT Thread;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;

	ReleaseSRWLockShared(SRWLock);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return;
	}

	//
	// If we never track the lock acquisition, skip it
	//

	Track = CcrLookupLockTrack(Thread, SRWLock, FALSE);
	if (!Track) {
		return;
	}

	QueryPerformanceCounter(&End);
	CcrTrackLockRelease(Thread, Track, CCR_PROBE_RELEASE_SRW_SHARED, &End);
}

VOID
WINAPI
CcrIatAcquireSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
	)
{
	PBTR_THREAD_OBJECT Thread;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record;
	PBTR_IAT_PATCH Patch;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		AcquireSRWLockExclusive(SRWLock);
		return;
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		AcquireSRWLockExclusive(SRWLock);
		return;
	}

	Record = NULL;
	Patch = CcrIatCurrentPatch(_CcrAcquireSRWLockExclusive);
	CcrIatCaptureStackTrace(Thread, Patch->Address, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//

	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	AcquireSRWLockExclusive(SRWLock);
	QueryPerformanceCounter(&End);

	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_ACQUIRE_SRW_EXCLUSIVE, &Start, &End, TRUE);
}

VOID
WINAPI
CcrIatAcquireSRWLockShared(
	_Inout_ PSRWLOCK SRWLock
	)
{
	PBTR_THREAD_OBJECT Thread;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record;
	PBTR_IAT_PATCH Patch;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		AcquireSRWLockShared(SRWLock);
		return;
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		AcquireSRWLockShared(SRWLock);
		return;
	}

	Record = NULL;
	Patch = CcrIatCurrentPatch(_CcrAcquireSRWLockShared);
	CcrIatCaptureStackTrace(Thread, Patch->Address, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//

	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	AcquireSRWLockShared(SRWLock);
	QueryPerformanceCounter(&End);

	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_ACQUIRE_SRW_SHARED, &Start, &End, TRUE);
}

BOOLEAN
WINAPI
CcrIatTryAcquireSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
	)
{
	PBTR_THREAD_OBJECT Thread;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record;
	PBTR_IAT_PATCH Patch;
	BOOLEAN Acquired;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return TryAcquireSRWLockExclusive(SRWLock);
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		return TryAcquireSRWLockExclusive(SRWLock);
	}

	Record = NULL;
	Patch = CcrIatCurrentPatch(_CcrTryAcquireSRWLockExclusive);
	CcrIatCaptureStackTrace(Thread, Patch->Address, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//

	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	Acquired = TryAcquireSRWLockExclusive(SRWLock);
	QueryPerformanceCounter(&End);

	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_TRY_ACQUIRE_SRW_EXCLUSIVE, &Start, &End, Acquired);
	return Acquired;
}

BOOLEAN
WINAPI
CcrIatTryAcquireSRWLockShared(
	_Inout_ PSRWLOCK SRWLock
	)
{
	PBTR_THREAD_OBJECT Thread;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record;
	PBTR_IAT_PATCH Patch;
	BOOLEAN Acquired;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return TryAcquireSRWLockShared(SRWLock);
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		return TryAcquireSRWLockShared(SRWLock);
	}

	Record = NULL;
	Patch = CcrIatCurrentPatch(_CcrTryAcquireSRWLockShared);
	CcrIatCaptureStackTrace(Thread, Patch->Address, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//

	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	Acquired = TryAcquireSRWLockShared(SRWLock);
	QueryPerformanceCounter(&End);

	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_TRY_ACQUIRE_SRW_SHARED, &Start, &End, Acquired);
	return Acquired;
}

//
// SleepConditionVariableSRW can hang and stop unload of runtime
//

BOOL WINAPI
CcrIatSleepConditionVariableSRW(
	_Inout_ PCONDITION_VARIABLE ConditionVariable,
	_Inout_ PSRWLOCK            SRWLock,
	_In_    DWORD               dwMilliseconds,
	_In_    ULONG               Flags
	)
{
	PBTR_THREAD_OBJECT Thread;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;
	BOOL Status;
	ULONG LastError;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) { 
		return SleepConditionVariableSRW(ConditionVariable, SRWLock, dwMilliseconds, Flags);
	}
	
	Status = SleepConditionVariableSRW(ConditionVariable, SRWLock, dwMilliseconds, Flags);
	LastError = GetLastError();

	//
	// If we never track the lock acquisition, skip it
	//

	Track = CcrLookupLockTrack(Thread, SRWLock, FALSE);
	if (!Track) {
		SetLastError(LastError);
		return Status;
	}

	QueryPerformanceCounter(&End);
	CcrTrackLockRelease(Thread, Track, CCR_PROBE_SLEEP_ON_CONDITION_SRW, &End);
	SetLastError(LastError);
	return Status;
}

VOID
CcrDllLoadCallback(
	IN struct _BTR_PROFILE_OBJECT* Object,
	IN struct _BTR_MODULE* Dll,
	IN BOOLEAN Load
	)
{
	if (Load) {
		BtrApplyPatchForDllByAddress(Dll, CcrIatPatch, CcrIatPatchCount, TRUE);
		return;
	}
}