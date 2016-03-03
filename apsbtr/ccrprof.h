// 
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _CCR_PROF_H_
#define _CCR_PROF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "callback.h"

ULONG
CcrInitialize(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrValidateAttribute(
	__in PBTR_PROFILE_ATTRIBUTE Attr
	);

ULONG
CcrStartProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrStopProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrPauseProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrResumeProfile(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrUnload(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrThreadAttach(
	__in PBTR_PROFILE_OBJECT Object
	);

ULONG
CcrThreadDetach(
	__in PBTR_PROFILE_OBJECT Object
	);

BOOLEAN
CcrIsRuntimeThread(
	__in PBTR_PROFILE_OBJECT Object,
	__in ULONG ThreadId
	);

ULONG
CcrQueryHotpatch(
	__in PBTR_PROFILE_OBJECT Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	);

//
// CCR profie internal routines
//

BOOLEAN
CcrCloseFiles(
	VOID
	);

ULONG CALLBACK
CcrProfileProcedure(
	__in PVOID Context
	);

BOOLEAN
CcrDllCanUnload(
	VOID
	);

VOID WINAPI 
CcrExitProcessCallback(
	__in UINT ExitCode
	);

//
// Runtime unloading routines
//

ULONG
CcrPrepareProfileStream(
	VOID
	);

ULONG
CcrRetireAllThreads(
	VOID
	);

ULONG
CcrInsertStackRecord(
	_In_ PBTR_STACK_RECORD Record
	);

VOID
CcrFreeStackPagePerThread(
	_In_ PBTR_THREAD_OBJECT Thread
	);

VOID
CcrFreePerThreadResource(
	VOID
	);

//
// N.B. This routine must be called with caller's thread
// has exemption flag set. otherwise it may trap into
// recursive calls.
//

PCCR_LOCK_CACHE
CcrGetLockCache(
	_In_ PBTR_THREAD_OBJECT Thread	
	);

PCCR_LOCK_TRACK
CcrLookupLockTrack(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PVOID LockPtr,
	_In_ BOOLEAN Allocate
	);

VOID
CcrTrackLockAcquire(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PCCR_LOCK_TRACK Track,
	_In_ CCR_PROBE_TYPE Type,
	_In_ PLARGE_INTEGER Start,
	_In_ PLARGE_INTEGER End,
	_In_ BOOLEAN Owner
	);

VOID
CcrTrackLockRelease(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PCCR_LOCK_TRACK Track,
	_In_ CCR_PROBE_TYPE Type,
	_In_ PLARGE_INTEGER End
	);

PCCR_LOCK_TRACK
CcrAllocateLockTrack(
	VOID
	);

VOID
CcrFreeLockTrack(
	_In_ PCCR_LOCK_TRACK Track 
	);

PCCR_STACKTRACE
CcrAllocateStackTrace(
	_In_ PBTR_THREAD_OBJECT Thread
	);

PCCR_STACKTRACE_PAGE
CcrAllocateStackTracePage(
	VOID
	);

VOID
CcrInsertStackTrace(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PCCR_LOCK_TRACK Track,
	_In_ PBTR_STACK_RECORD Record
	);

VOID
CcrUpdateStackTraceId(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PBTR_STACK_RECORD Record,
	_In_ ULONG StackId
	);

VOID
CcrUpdateLockTrackByStackId(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PBTR_STACK_RECORD Record,
	_In_ ULONG StackId
	);

//
// Intercal callbacks
//

PVOID NTAPI
CcrRtlAllocateHeap( 
	_In_ PVOID  HeapHandle,
    _In_ ULONG  Flags,
    _In_ SIZE_T  Size
	); 

typedef PVOID 
(NTAPI *RtlAllocateHeapPtr)( 
	_In_ PVOID  HeapHandle,
    _In_ ULONG  Flags,
    _In_ SIZE_T  Size
	); 

BOOLEAN NTAPI
CcrRtlFreeHeap( 
    _In_ PVOID  HeapHandle,
    _In_ ULONG  Flags,
    _In_ PVOID  HeapBase
    ); 

typedef BOOLEAN
(NTAPI *RtlFreeHeapPtr)(
    _In_ PVOID  HeapHandle,
    _In_ ULONG  Flags,
    _In_ PVOID  HeapBase
    ); 

VOID
WINAPI
CcrEnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

typedef VOID
(WINAPI *EnterCriticalSectionPtr)(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

//
// N.B. RtlLeaveCriticalSection has return value
//

ULONG
WINAPI
CcrLeaveCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

typedef ULONG 
(WINAPI *LeaveCriticalSectionPtr)(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

BOOL
WINAPI
CcrTryEnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

typedef BOOL
(WINAPI *TryEnterCriticalSectionPtr)(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

VOID
WINAPI
CcrReleaseSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

VOID
WINAPI
CcrReleaseSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

VOID
WINAPI
CcrAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

VOID
WINAPI
CcrAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

BOOLEAN
WINAPI
CcrTryAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

BOOLEAN
WINAPI
CcrTryAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

BOOL WINAPI 
CcrSleepConditionVariableSRW(
  _Inout_ PCONDITION_VARIABLE ConditionVariable,
  _Inout_ PSRWLOCK            SRWLock,
  _In_    DWORD               dwMilliseconds,
  _In_    ULONG               Flags
);

typedef VOID
(WINAPI *ReleaseSRWLockExclusivePtr)(
    _Inout_ PSRWLOCK SRWLock
    );

typedef VOID
(WINAPI *ReleaseSRWLockSharedPtr)(
    _Inout_ PSRWLOCK SRWLock
    );

typedef VOID
(WINAPI *AcquireSRWLockExclusivePtr)(
    _Inout_ PSRWLOCK SRWLock
    );

typedef VOID
(WINAPI *AcquireSRWLockSharedPtr)(
    _Inout_ PSRWLOCK SRWLock
    );

typedef BOOLEAN
(WINAPI *TryAcquireSRWLockExclusivePtr)(
    _Inout_ PSRWLOCK SRWLock
    );

typedef BOOLEAN
(WINAPI *TryAcquireSRWLockSharedPtr)(
    _Inout_ PSRWLOCK SRWLock
    );

typedef BOOL 
(WINAPI *SleepConditionVariableSRWPtr)(
	_Inout_ PCONDITION_VARIABLE ConditionVariable,
	_Inout_ PSRWLOCK            SRWLock,
	_In_    DWORD               dwMilliseconds,
	_In_    ULONG               Flags
	);


NTSTATUS
NTAPI
CcrNtWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

typedef NTSTATUS
(NTAPI *NtWaitForSingleObjectPtr)(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

NTSTATUS
NTAPI
CcrNtWaitForKeyedEvent (
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID KeyValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

typedef NTSTATUS
(NTAPI *NtWaitForKeyedEventPtr)(
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID KeyValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

NTSTATUS
NTAPI
CcrNtTerminateThread(
    _In_opt_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    );

typedef NTSTATUS
(NTAPI *NtTerminateThreadPtr)(
    _In_opt_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    );


extern BTR_CALLBACK CcrCallback[];
extern ULONG CcrCallbackCount;
extern BOOLEAN CcrTrackSystemLock;

#ifdef __cplusplus
}
#endif
#endif