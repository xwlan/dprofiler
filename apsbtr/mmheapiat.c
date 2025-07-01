//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "mmprof.h"
#include "mmheap.h"
#include "btrsdk.h"
#include "callback.h"
#include "heap.h"
#include "lock.h"
#include "callback.h"
#include "trap.h"
#include "heap.h"
#include "thread.h"
#include "hal.h"
#include "cache.h"
#include "stacktrace.h"
#include "util.h"
#include "iatpatch.h"
#include "mmheapiat.h"

//
// ucrtbase{d} must be dynamically linked to runtime dll if heap 
// handle is used to track heap allocation and free, _get_heap_handle()
// return heap handle shared by all dll loaded into current process 
// address space.
//
// if ucrtbase{d} is statically linked to runtime dll, a psuedo handle
// used to track heap allocation and free, this can work because all
// allocation from ucrtbase{d} is shared by all dlls, the heap handle
// value is not important. Or we can dynamically load ucrtbase and call
// its export get_heap_handle(), then free ucrtbase.dll.
//

BTR_IAT_PATCH MmHeapPatch[] = {
	{ "kernel32.dll", "HeapAlloc" , MmHeapAlloc },
	{ "kernel32.dll", "HeapReAlloc", MmHeapReAlloc },
	{ "kernel32.dll", "HeapFree", MmHeapFree },
	{ "kernel32.dll", "ExitProcess", MmExitProcessCallback },
};

ULONG MmHeapPatchCount = ARRAYSIZE(MmHeapPatch);

PBTR_IAT_PATCH FORCEINLINE
MmGetHeapPatch(
	IN MM_HEAP_PATCH Ordinal
	)
{
	return &MmHeapPatch[Ordinal];
}

VOID
MmHeapIatInitialize(
	VOID
	)
{
	ULONG i;
	HMODULE Handle;

	Handle = GetModuleHandleA("kernel32.dll");
	for (i = 0; i < MmHeapPatchCount; i++) {
		MmHeapPatch[i].Address = GetProcAddress(Handle, MmHeapPatch[i].Function);
	}
}

VOID
MmHeapIatApplyPatch(
	VOID
	)
{
	MmHeapIatInitialize();
	BtrApplyIatPatch(MmHeapPatch, MmHeapPatchCount, TRUE);
}

PVOID WINAPI 
MmHeapAlloc(
	IN PVOID  HeapHandle,
	IN ULONG  Flags,
	IN SIZE_T Size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_IAT_PATCH Patch;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	PVOID Address;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		Address = HeapAlloc(HeapHandle, Flags, Size);
		return Address;
	}

	BtrEnterExemptionRegion(Thread);
	QueryPerformanceCounter(&Enter);

	Address = HeapAlloc(HeapHandle, Flags, Size);
	if (!Address || Size == 0) {
		BtrLeaveExemptionRegion(Thread);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)(Size);
	Callers[2] = (PVOID)1;

	Frame = BtrGetFramePointer();
	Patch = MmGetHeapPatch(_HeapAlloc);
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
						Patch->Address, &Hash, &Depth);

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(HeapHandle, Address, (ULONG)Size, Duration,
					Hash, (USHORT)Depth, (USHORT)_HeapAlloc);

	BtrLeaveExemptionRegion(Thread);
	return Address;
}

PVOID WINAPI 
MmHeapReAlloc(
	HANDLE Heap,
	ULONG Flags,
	PVOID Ptr,
	SIZE_T Size
	)
{
	PBTR_THREAD_OBJECT Thread;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;
	PBTR_IAT_PATCH Patch;

	Caller = _ReturnAddress();

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		Address = HeapReAlloc(Heap, Flags, Ptr, Size);
		return Address;
	}

	BtrEnterExemptionRegion(Thread);
	QueryPerformanceCounter(&Enter);

	Address = HeapReAlloc(Heap, Flags, Ptr, Size);
	if (!Address) {
		BtrLeaveExemptionRegion(Thread);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)Size;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	Patch = MmGetHeapPatch(_HeapReAlloc);
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame,
						Patch->Address, &Hash, &Depth);

	if (Ptr != NULL) {
		MmRemoveHeapRecord(Heap, Ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(Heap, Address, (ULONG)Size, Duration,
					Hash, (USHORT)Depth, (USHORT)_HeapReAlloc);

	BtrLeaveExemptionRegion(Thread);
	return Address;
}

BOOLEAN WINAPI 
MmHeapFree(
	IN PVOID  HeapHandle,
	IN ULONG  Flags,
	IN PVOID  HeapBase
	)
{
	PBTR_THREAD_OBJECT Thread;
	PVOID Caller;
	BOOLEAN Status;

	Caller = _ReturnAddress();

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		Status = HeapFree(HeapHandle, Flags, HeapBase);
		return Status;
	}

	BtrEnterExemptionRegion(Thread);
	Status = HeapFree(HeapHandle, Flags, HeapBase);
	if (!Status) {
		BtrLeaveExemptionRegion(Thread);
		return FALSE;
	}

	if (HeapBase != NULL) {
		MmRemoveHeapRecord(HeapHandle, HeapBase);
	}

	BtrLeaveExemptionRegion(Thread);
	return TRUE;
}

VOID
MmHeapDllLoadCallback(
	IN struct _BTR_PROFILE_OBJECT* Object,
	IN struct _BTR_MODULE* Dll,
	IN BOOLEAN Load
	)
{
	if (Load) {
		BtrApplyPatchForDllByAddress(Dll, MmHeapPatch, MmHeapPatchCount, TRUE);
		return;
	}
}
