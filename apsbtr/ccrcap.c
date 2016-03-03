//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
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

BTR_CALLBACK CcrCallback[] = {
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrExitProcessCallback, "kernel32.dll", "ExitProcess", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrEnterCriticalSection, "ntdll.dll", "RtlEnterCriticalSection", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrTryEnterCriticalSection, "ntdll.dll", "RtlTryEnterCriticalSection", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrLeaveCriticalSection, "ntdll.dll", "RtlLeaveCriticalSection", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrAcquireSRWLockExclusive, "ntdll.dll", "RtlAcquireSRWLockExclusive", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrAcquireSRWLockShared, "ntdll.dll", "RtlAcquireSRWLockShared", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrTryAcquireSRWLockExclusive, "ntdll.dll", "RtlTryAcquireSRWLockExclusive", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrTryAcquireSRWLockShared, "ntdll.dll", "RtlTryAcquireSRWLockShared", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrReleaseSRWLockExclusive, "ntdll.dll", "RtlReleaseSRWLockExclusive", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrReleaseSRWLockShared, "ntdll.dll", "RtlReleaseSRWLockShared", 0 },
	{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrSleepConditionVariableSRW, "ntdll.dll", "SleepConditionVariableSRW", 0 },
	//{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrNtWaitForSingleObject, "ntdll.dll", "NtWaitForSingleObject", 0 },
	//{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrNtWaitForKeyedEvent, "ntdll.dll", "NtWaitForKeyedEvent", 0 },
	//{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrRtlAllocateHeap, "ntdll.dll", "RtlAllocateHeap", 0 },
	//{  CcrCallbackType, CALLBACK_OFF,0,0,0, CcrRtlFreeHeap, "ntdll.dll", "RtlFreeHeap", 0 },
};

ULONG CcrCallbackCount = ARRAYSIZE(CcrCallback);

PBTR_CALLBACK FORCEINLINE
CcrGetCallback(
	IN ULONG Ordinal
	)
{
	return &CcrCallback[Ordinal];
}

volatile ULONG CcrRequestId = (ULONG)-1;
volatile ULONG CcrObjectId = (ULONG)-1;

BOOLEAN CcrTrackSystemLock = TRUE;

LIST_ENTRY CcrCallerRangeList;

//
// _T, Thread object
// _C, Callback
// _R, PBTR_STACK_RECORD *
//

#define CcrCaptureStackTrace(_T, _C, _R)\
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
		                   _C->Address, &StackId, &Depth);\
	BtrLeaveExemptionRegion(_T);\
}

FORCEINLINE
PCCR_LOCK_TRACK
CcrMarkThreadInAcquire(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ PVOID LockPtr,
	_In_ CCR_LOCK_TYPE Type
	)
{
	PCCR_LOCK_TRACK Track;
	PCCR_LOCK_CACHE Cache;

	Track = CcrLookupLockTrack(Thread, LockPtr, TRUE); 
	if (!Track->LockPtr) { 

		//
		// First time mark acquisition, fill its pointer and type
		//

		Track->LockPtr = LockPtr;
		Track->Type = Type;

		if (Type == CCR_LOCK_CS)
			DebugTrace2("TID:%d FIRST-TIME ACQUIRE CS %p, RecursionCount=%d", Thread->ThreadId, 
						LockPtr, ((PRTL_CRITICAL_SECTION)LockPtr)->RecursionCount);

	}

	Cache = Thread->CcrLockCache;
	Cache->StackInAcquire = TRUE;
	return Track;
}

FORCEINLINE
VOID
CcrMarkThreadInHeapAlloc(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	PCCR_LOCK_CACHE Cache;

	Cache = CcrGetLockCache(Thread);
	ASSERT(Cache != NULL);
	Cache->StackInHeapAlloc = TRUE;
}


FORCEINLINE
VOID
CcrMarkThreadInHeapFree(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	PCCR_LOCK_CACHE Cache;

	Cache = CcrGetLockCache(Thread);
	ASSERT(Cache != NULL);
	Cache->StackInHeapFree = TRUE;
}

FORCEINLINE
VOID
CcrMarkThreadInKernelWait(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	PCCR_LOCK_CACHE Cache;

	Cache = CcrGetLockCache(Thread);
	ASSERT(Cache != NULL);
	Cache->StackInKernelWait = TRUE;
}
 
BOOLEAN FORCEINLINE
CcrIsSystemDllAddress(
	IN ULONG_PTR Address 
	)
{
	ULONG i;

	//
	// If it's runtime address, exempt it
	//

	if ((ULONG_PTR)Address >= BtrDllBase && (ULONG_PTR)Address < BtrDllBase + BtrDllSize) {
		return TRUE;
	}

#if defined (_M_IX86)
	_mm_prefetch((const char *)BtrSystemDllAddress, _MM_HINT_T0);
#elif defined (_M_X64)
	_mm_prefetch((const char *)BtrSystemDllAddress, _MM_HINT_T0);
	_mm_prefetch((const char *)BtrSystemDllAddress + SYSTEM_CACHE_ALIGNMENT_SIZE, _MM_HINT_T0);
#endif

	for(i = 0; i < BtrSystemDllFilledCount; i++) {
		if (Address >= BtrSystemDllAddress[i].StartVa && Address < BtrSystemDllAddress[i].EndVa) {
			return TRUE;
		}
	}

	return FALSE;
}


FORCEINLINE
BOOLEAN
CcrShouldTrackLock(
	_In_ ULONG_PTR Caller,
	_In_ PVOID LockPtr
	)
{
	PLIST_ENTRY ListEntry;
	PCCR_CALLER_RANGE Range;

	UNREFERENCED_PARAMETER(LockPtr);

	if (CcrTrackSystemLock) {
		return TRUE;
	}

	//
	// If it's system dll address, skip it
	//

	if (CcrIsSystemDllAddress((ULONG_PTR)Caller)){
		return FALSE;
	}

	return TRUE;

	//
	// N.B. Reserve the following code region if we want to support
	// user specified lock profiling, user can specify a group of
	// dll to be inspected, not support now.
	//

	ListEntry = CcrCallerRangeList.Flink;
	while (ListEntry != &CcrCallerRangeList) {
		Range = CONTAINING_RECORD(ListEntry, CCR_CALLER_RANGE, ListEntry);	
		if (Caller >= Range->Base && Caller < Range->Base + Range->Size) {
			return TRUE;
		}
	}

	return FALSE;
}

VOID
WINAPI
CcrEnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	EnterCriticalSectionPtr CallbackPtr;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record = NULL;
	
	Callback = CcrGetCallback(_CcrEnterCriticalSection);
	CallbackPtr = (EnterCriticalSectionPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		(*CallbackPtr)(lpCriticalSection);
		return;
	}

	if (!CcrShouldTrackLock(CALLER, lpCriticalSection)) {
		(*CallbackPtr)(lpCriticalSection);
		return;
	}

	CcrCaptureStackTrace(Thread, Callback, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//
	 
	Track = CcrMarkThreadInAcquire(Thread, lpCriticalSection, CCR_LOCK_CS);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	(*CallbackPtr)(lpCriticalSection);
	QueryPerformanceCounter(&End);
	
	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_ENTER_CS, &Start, &End, TRUE);

	DebugTrace2("TID:%d ACQUIRE CS %p", Thread->ThreadId, lpCriticalSection);
}

BOOL
WINAPI
CcrTryEnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	TryEnterCriticalSectionPtr CallbackPtr;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	BOOL Acquired;
	PBTR_STACK_RECORD Record = NULL;
	
	Callback = CcrGetCallback(_CcrTryEnterCriticalSection);
	CallbackPtr = (TryEnterCriticalSectionPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(lpCriticalSection);
	}

	if (!CcrShouldTrackLock(CALLER, lpCriticalSection)) {
		return (*CallbackPtr)(lpCriticalSection);
	}

	CcrCaptureStackTrace(Thread, Callback, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//
	 
	Track = CcrMarkThreadInAcquire(Thread, lpCriticalSection, CCR_LOCK_CS);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	Acquired = (*CallbackPtr)(lpCriticalSection);
	QueryPerformanceCounter(&End);
	
	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_TRY_ENTER_CS, &Start, &End, Acquired);
	DebugTrace2("TID:%d TRY ACQUIRE CS %p, RESULT=%d", Thread->ThreadId, lpCriticalSection, Acquired);
	return Acquired;
}

ULONG
WINAPI
CcrLeaveCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	LeaveCriticalSectionPtr CallbackPtr;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;
	ULONG Status;
	
	Callback = CcrGetCallback(_CcrLeaveCriticalSection);
	CallbackPtr = (LeaveCriticalSectionPtr)BtrGetCallbackDestine(Callback);

	Status = (*CallbackPtr)(lpCriticalSection);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return Status;
	}

	//
	// If we never track the lock acquisition, skip it
	//

	Track = CcrLookupLockTrack(Thread, lpCriticalSection, FALSE);
	if (!Track) {
		return Status;
	}

	QueryPerformanceCounter(&End);

	if (!Track->LockOwner) { 
		DebugTrace2("TID:%d WARNING => LEAVE CS %p NOT LOCK OWNER", Thread->ThreadId, lpCriticalSection);
	}

	CcrTrackLockRelease(Thread, Track, CCR_PROBE_LEAVE_CS, &End);
	DebugTrace2("TID:%d LEAVE CS %p", Thread->ThreadId, lpCriticalSection);
	return Status;
}

PVOID NTAPI
CcrRtlAllocateHeap( 
	_In_ PVOID  HeapHandle,
	_In_ ULONG  Flags,
	_In_ SIZE_T  Size
	) 
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	RtlAllocateHeapPtr CallbackPtr;
	
	Callback = CcrGetCallback(_CcrRtlAllocateHeap);
	CallbackPtr = (RtlAllocateHeapPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(HeapHandle, Flags, Size);
	}

	CcrMarkThreadInHeapAlloc(Thread);
	return (*CallbackPtr)(HeapHandle, Flags, Size);
}

BOOLEAN NTAPI
CcrRtlFreeHeap( 
    _In_ PVOID  HeapHandle,
    _In_ ULONG  Flags,
    _In_ PVOID  HeapBase
    ) 
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	RtlFreeHeapPtr CallbackPtr;
	
	Callback = CcrGetCallback(_CcrRtlFreeHeap);
	CallbackPtr = (RtlFreeHeapPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(HeapHandle, Flags, HeapBase);
	}

	CcrMarkThreadInHeapFree(Thread);
	return (*CallbackPtr)(HeapHandle, Flags, HeapBase);
}

NTSTATUS
NTAPI
CcrNtWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	NtWaitForSingleObjectPtr CallbackPtr;
	PCCR_LOCK_CACHE Cache;
	
	Callback = CcrGetCallback(_CcrNtWaitForSingleObject);
	CallbackPtr = (NtWaitForSingleObjectPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(Handle, Alertable, Timeout);
	}

	//
	// If the kernel wait is not inside a lock acquisition, skip it
	//

	Cache = Thread->CcrLockCache;
	if (!Cache->StackInAcquire) {
		return (*CallbackPtr)(Handle, Alertable, Timeout);
	}

	CcrMarkThreadInKernelWait(Thread);
	return (*CallbackPtr)(Handle, Alertable, Timeout);
}

NTSTATUS
NTAPI
CcrNtWaitForKeyedEvent (
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID KeyValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	NtWaitForKeyedEventPtr CallbackPtr;
	PCCR_LOCK_CACHE Cache;
	
	Callback = CcrGetCallback(_CcrNtWaitForKeyedEvent);
	CallbackPtr = (NtWaitForKeyedEventPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(KeyedEventHandle, KeyValue, Alertable, Timeout);
	}

	//
	// If the kernel wait is not inside a lock acquisition, skip it
	//

	Cache = Thread->CcrLockCache;
	if (!Cache->StackInAcquire) {
		return (*CallbackPtr)(KeyedEventHandle, KeyValue, Alertable, Timeout);
	}

	CcrMarkThreadInKernelWait(Thread);
	return (*CallbackPtr)(KeyedEventHandle, KeyValue, Alertable, Timeout);
}

VOID
WINAPI
CcrReleaseSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ReleaseSRWLockExclusivePtr CallbackPtr;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;
	
	Callback = CcrGetCallback(_CcrReleaseSRWLockExclusive);
	CallbackPtr = (ReleaseSRWLockExclusivePtr)BtrGetCallbackDestine(Callback);
	(*CallbackPtr)(SRWLock);

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
CcrReleaseSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ReleaseSRWLockSharedPtr CallbackPtr;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;
	
	Callback = CcrGetCallback(_CcrReleaseSRWLockShared);
	CallbackPtr = (ReleaseSRWLockSharedPtr)BtrGetCallbackDestine(Callback);
	(*CallbackPtr)(SRWLock);

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
CcrAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	AcquireSRWLockExclusivePtr CallbackPtr;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record = NULL;
	
	Callback = CcrGetCallback(_CcrAcquireSRWLockExclusive);
	CallbackPtr = (AcquireSRWLockExclusivePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		(*CallbackPtr)(SRWLock);
		return;
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		(*CallbackPtr)(SRWLock);
		return;
	}

	CcrCaptureStackTrace(Thread, Callback, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//
	 
	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	(*CallbackPtr)(SRWLock);
	QueryPerformanceCounter(&End);
	
	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_ACQUIRE_SRW_EXCLUSIVE, &Start, &End, TRUE);
}

VOID
WINAPI
CcrAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	AcquireSRWLockSharedPtr CallbackPtr;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	PBTR_STACK_RECORD Record = NULL;
	
	Callback = CcrGetCallback(_CcrAcquireSRWLockShared);
	CallbackPtr = (AcquireSRWLockSharedPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		(*CallbackPtr)(SRWLock);
		return;
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		(*CallbackPtr)(SRWLock);
		return;
	}

	CcrCaptureStackTrace(Thread, Callback, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//
	 
	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	(*CallbackPtr)(SRWLock);
	QueryPerformanceCounter(&End);
	
	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_ACQUIRE_SRW_SHARED, &Start, &End, TRUE);
}

BOOLEAN
WINAPI
CcrTryAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{	
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	TryAcquireSRWLockExclusivePtr CallbackPtr;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	BOOL Acquired;
	PBTR_STACK_RECORD Record = NULL;
	
	Callback = CcrGetCallback(_CcrTryAcquireSRWLockExclusive);
	CallbackPtr = (TryAcquireSRWLockExclusivePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(SRWLock);
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		return (*CallbackPtr)(SRWLock);
	}

	CcrCaptureStackTrace(Thread, Callback, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//
	 
	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	Acquired = (*CallbackPtr)(SRWLock);
	QueryPerformanceCounter(&End);
	
	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_TRY_ACQUIRE_SRW_EXCLUSIVE, &Start, &End, Acquired);
	return Acquired;
}

BOOLEAN
WINAPI
CcrTryAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	TryAcquireSRWLockSharedPtr CallbackPtr;
	LARGE_INTEGER Start, End;
	PCCR_LOCK_TRACK Track;
	BOOL Acquired;
	PBTR_STACK_RECORD Record = NULL;
	
	Callback = CcrGetCallback(_CcrTryAcquireSRWLockShared);
	CallbackPtr = (TryAcquireSRWLockSharedPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(SRWLock);
	}

	if (!CcrShouldTrackLock(CALLER, SRWLock)) {
		return (*CallbackPtr)(SRWLock);
	}

	CcrCaptureStackTrace(Thread, Callback, &Record);

	//
	// N.B. If we don't mark lock in acquire state, the underlying
	// probe of NtWaitForSingleObject, NtWaitForKeyedEvent will ignore
	// and skip the call, they're are only interested in wait inside
	// a lock like CS/SRW.
	//
	 
	Track = CcrMarkThreadInAcquire(Thread, SRWLock, CCR_LOCK_SRW);
	CcrInsertStackTrace(Thread, Track, Record);

	QueryPerformanceCounter(&Start);
	Acquired = (*CallbackPtr)(SRWLock);
	QueryPerformanceCounter(&End);
	
	End.QuadPart = End.QuadPart - Start.QuadPart;
	CcrTrackLockAcquire(Thread, Track, CCR_PROBE_TRY_ACQUIRE_SRW_SHARED, &Start, &End, Acquired);
	return Acquired;
}

BOOL WINAPI 
CcrSleepConditionVariableSRW(
	_Inout_ PCONDITION_VARIABLE ConditionVariable,
	_Inout_ PSRWLOCK            SRWLock,
	_In_    DWORD               dwMilliseconds,
	_In_    ULONG               Flags
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	SleepConditionVariableSRWPtr CallbackPtr;
	PCCR_LOCK_TRACK Track;
	LARGE_INTEGER End;
	BOOL Status;
	ULONG LastError;
	
	Callback = CcrGetCallback(_CcrSleepConditionVariableSRW);
	CallbackPtr = (SleepConditionVariableSRWPtr)BtrGetCallbackDestine(Callback);
	Status = (*CallbackPtr)(ConditionVariable, SRWLock, dwMilliseconds, Flags);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		return Status;
	}

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