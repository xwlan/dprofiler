//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "stacktrace.h"
#include "mmprof.h"
#include "mmpage.h"
#include "thread.h"

BTR_CALLBACK MmPageCallback[] = {
	{ PageCallbackType, 0,0,0,0, VirtualAllocEnter,   "kernel32.dll",  "VirtualAlloc",   0 },
	{ PageCallbackType, 0,0,0,0, VirtualFreeEnter,    "kernel32.dll",  "VirtualFree",    0 },
	{ PageCallbackType, 0,0,0,0, VirtualAllocExEnter, "kernel32.dll",  "VirtualAllocEx", 0 },
	{ PageCallbackType, 0,0,0,0, VirtualFreeExEnter,  "kernel32.dll",  "VirtualFreeEx",  0 },
};

ULONG MmPageCallbackCount = ARRAYSIZE(MmPageCallback);

PBTR_CALLBACK FORCEINLINE
MmGetPageCallback(
	IN ULONG Ordinal
	)
{
	return &MmPageCallback[Ordinal];
}

LPVOID WINAPI 
VirtualAllocEnter(
	__in  LPVOID lpAddress,
	__in  SIZE_T dwSize,
	__in  DWORD flAllocationType,
	__in  DWORD flProtect
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	VIRTUALALLOC CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetPageCallback(_VirtualAlloc);
	CallbackPtr = (VIRTUALALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(lpAddress, dwSize, flAllocationType, flProtect);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(lpAddress, dwSize, flAllocationType, flProtect);
	if (!Address) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)dwSize;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (FlagOn(flAllocationType, MEM_COMMIT)) {
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertPageRecord(Address, (ULONG)dwSize, Duration, Hash, (USHORT)Depth, _VirtualAlloc);
	
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

BOOL WINAPI 
VirtualFreeEnter(
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD dwFreeType
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	VIRTUALFREE CallbackPtr;
	PVOID Caller;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetPageCallback(_VirtualFree);
	CallbackPtr = (VIRTUALFREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(lpAddress, dwSize, dwFreeType);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(lpAddress, dwSize, dwFreeType);
	if (!Status) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (FlagOn(dwFreeType, MEM_RELEASE)) {
	}

	if (lpAddress != NULL) {
		MmRemovePageRecord(lpAddress);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

LPVOID WINAPI 
VirtualAllocExEnter(
	__in HANDLE hProcess,
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD flAllocationType,
	__in DWORD flProtect
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	VIRTUALALLOCEX CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG ProcessId;
	BOOLEAN Current;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetPageCallback(_VirtualAllocEx);
	CallbackPtr = (VIRTUALALLOCEX)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	ProcessId = GetProcessId(hProcess);
	if (ProcessId == GetCurrentProcessId()) {
		Current = TRUE;
	} else {
		Current = FALSE;
	}

	Address = (*CallbackPtr)(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
	if (!Address || !Current) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)dwSize;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (FlagOn(flAllocationType, MEM_COMMIT)) {
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertPageRecord(Address, (ULONG)dwSize, Duration, Hash, (USHORT)Depth, _VirtualAlloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

BOOL WINAPI 
VirtualFreeExEnter(
	__in HANDLE hProcess,
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD dwFreeType
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	VIRTUALFREEEX CallbackPtr;
	PVOID Caller;
	ULONG ProcessId;
	BOOLEAN Current;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetPageCallback(_VirtualFreeEx);
	CallbackPtr = (VIRTUALFREEEX)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hProcess, lpAddress, dwSize, dwFreeType);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	ProcessId = GetProcessId(hProcess);
	if (ProcessId == GetCurrentProcessId()) {
		Current = TRUE;
	} else {
		Current = FALSE;
	}

	Status = (*CallbackPtr)(hProcess, lpAddress, dwSize, dwFreeType);
	if (!Status || !Current) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (FlagOn(dwFreeType, MEM_RELEASE)) {
	}

	if (lpAddress != NULL) {
		MmRemovePageRecord(lpAddress);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}