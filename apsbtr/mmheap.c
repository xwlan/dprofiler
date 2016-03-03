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

BTR_CALLBACK MmHeapCallback[] = {

	{ HeapCallbackType, 0,0,0,0, RtlAllocateHeapEnter, "ntdll.dll", "RtlAllocateHeap" ,0 },
	{ HeapCallbackType, 0,0,0,0, RtlReAllocateHeapEnter , "ntdll.dll", "RtlReAllocateHeap" ,0},
	{ HeapCallbackType, 0,0,0,0, RtlFreeHeapEnter , "ntdll.dll", "RtlFreeHeap", 0, },

	{ HeapCallbackType, 0,0,0,0, MallocEnter,  "msvcrt.dll", "malloc" ,0},
	{ HeapCallbackType, 0,0,0,0, ReallocEnter, "msvcrt.dll", "realloc" ,0},
	{ HeapCallbackType, 0,0,0,0, CallocEnter,  "msvcrt.dll", "calloc" ,0},
	{ HeapCallbackType, 0,0,0,0, FreeEnter,    "msvcrt.dll", "free",0},

	{ HeapCallbackType, 0,0,0,0, GlobalAllocEnter , "kernel32.dll", "GlobalAlloc" ,0},
	{ HeapCallbackType, 0,0,0,0, GlobalReAllocEnter, "kernel32.dll", "GlobalReAlloc" ,0},
	{ HeapCallbackType, 0,0,0,0, GlobalFreeEnter , "kernel32.dll", "GlobalFree" ,0},

	{ HeapCallbackType, 0,0,0,0, LocalAllocEnter , "kernel32.dll", "LocalAlloc" ,0},
	{ HeapCallbackType, 0,0,0,0, LocalReAllocEnter, "kernel32.dll", "LocalReAlloc" ,0},
	{ HeapCallbackType, 0,0,0,0, LocalFreeEnter , "kernel32.dll", "LocalFree" ,0},

	{ HeapCallbackType, 0,0,0,0, SysAllocStringEnter , "oleaut32.dll", "SysAllocString" ,0},
	{ HeapCallbackType, 0,0,0,0, SysAllocStringLenEnter , "oleaut32.dll", "SysAllocStringLen" ,0},
	{ HeapCallbackType, 0,0,0,0, SysAllocStringByteLenEnter , "oleaut32.dll", "SysAllocStringByteLen" ,0},
	{ HeapCallbackType, 0,0,0,0, SysReAllocStringEnter , "oleaut32.dll", "SysReAllocString" ,0},
	{ HeapCallbackType, 0,0,0,0, SysReAllocStringLenEnter , "oleaut32.dll", "SysReAllocStringLen" ,0},
	{ HeapCallbackType, 0,0,0,0, SysFreeStringEnter , "oleaut32.dll", "SysFreeString" ,0},
	
	{ HeapCallbackType, 0,0,0,0, HeapCreateEnter , "kernel32.dll", "HeapCreate" ,0},
	{ HeapCallbackType, 0,0,0,0, HeapDestroyEnter , "kernel32.dll", "HeapDestroy" ,0},

	{ HeapCallbackType, 0,0,0,0, RtlCreateHeapEnter , "ntdll.dll", "RtlCreateHeap" ,0},
	{ HeapCallbackType, 0,0,0,0, RtlDestroyHeapEnter , "ntdll.dll", "RtlDestroyHeap" ,0},
};

ULONG MmHeapCallbackCount = ARRAYSIZE(MmHeapCallback);

PBTR_CALLBACK FORCEINLINE
MmGetHeapCallback(
	IN ULONG Ordinal
	)
{
	return &MmHeapCallback[Ordinal];
}

//
// RtlAllocateHeap
//

PVOID NTAPI
RtlAllocateHeapEnter( 
	IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T  Size
	)
{
	PBTR_THREAD_OBJECT Thread;
	BOOLEAN Exempted = FALSE;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	RTLALLOCATEHEAP CallbackPtr;
	PVOID Address;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_RtlAllocateHeap);
	CallbackPtr = (RTLALLOCATEHEAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(HeapHandle, Flags, Size);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(HeapHandle, Flags, Size);
	if (!Address || Size == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)(Size);
	Callers[2] = (PVOID)1;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(HeapHandle, Address, (ULONG)Size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_RtlAllocateHeap);


	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// RtlReAllocateHeap
//

PVOID NTAPI
RtlReAllocateHeapEnter(
    HANDLE Heap,
    ULONG Flags,
    PVOID Ptr,
    SIZE_T Size
	)
{
	PBTR_THREAD_OBJECT Thread;
	BOOLEAN Exempted = FALSE;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	RTLREALLOCATEHEAP CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_RtlReAllocateHeap);
	CallbackPtr = (RTLREALLOCATEHEAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(Heap, Flags, Ptr, Size);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(Heap, Flags, Ptr, Size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)Size;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (Ptr != NULL) {
		MmRemoveHeapRecord(Heap, Ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(Heap, Address, (ULONG)Size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_RtlReAllocateHeap);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// RtlFreeHeap
//

BOOLEAN NTAPI
RtlFreeHeapEnter( 
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID  HeapBase
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	RTLFREEHEAP CallbackPtr;
	PVOID Caller;
	BOOLEAN Status;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_RtlFreeHeap);
	CallbackPtr = (RTLFREEHEAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(HeapHandle, Flags, HeapBase);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(HeapHandle, Flags, HeapBase);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	if (HeapBase != NULL) {
		MmRemoveHeapRecord(HeapHandle, HeapBase);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}

//
// RtlCreateHeap
//

PVOID NTAPI
RtlCreateHeapEnter( 
    IN ULONG  Flags,
    IN PVOID  HeapBase OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize OPTIONAL,
    IN PVOID  Lock OPTIONAL,
    IN PVOID Parameters OPTIONAL
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	RTLCREATEHEAP CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	PVOID HeapHandle;
	PMM_HEAP_TABLE Table;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_RtlCreateHeap);
	CallbackPtr = (RTLCREATEHEAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		HeapHandle = (*CallbackPtr)(Flags, HeapBase, ReserveSize,
			                  CommitSize, Lock, Parameters);
		InterlockedDecrement(&Callback->References);
		return HeapHandle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	HeapHandle = (*CallbackPtr)(Flags, HeapBase, ReserveSize,
			                    CommitSize, Lock, Parameters);
	if (!HeapHandle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return HeapHandle; 
	}

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	//
	// Track heap creator's stack trace
	//

	Table = MmInsertHeapTable(HeapHandle);
	Table->CreateStackHash = Hash;
	Table->CreateStackDepth = Depth;
	Table->CreatePc = Caller;

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return HeapHandle;
}

//
// RtlDestroyHeap
//

PVOID NTAPI
RtlDestroyHeapEnter( 
    IN PVOID  HeapHandle
    ) 
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	RTLDESTROYHEAP CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	PVOID Return;
	PMM_HEAP_TABLE Table;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_RtlDestroyHeap);
	CallbackPtr = (RTLDESTROYHEAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Return = (*CallbackPtr)(HeapHandle);
		InterlockedDecrement(&Callback->References);
		return Return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Table = MmLookupHeapTable(HeapHandle);
	Return = (*CallbackPtr)(HeapHandle);

	if (Return != NULL || !Table) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Return;
	}

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 


	//
	// Track heap destroyer's stack trace and retire the heap table
	//

	Table->DestroyStackHash = Hash;
	Table->DestroyStackDepth = Depth;
	Table->DestroyPc = Caller;
	MmRetireHeapTable(HeapHandle);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Return;
}


//
// HeapCreate
//

PVOID WINAPI
HeapCreateEnter( 
	__in DWORD flOptions,
	__in SIZE_T dwInitialSize,
	__in SIZE_T dwMaximumSize
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	HEAPCREATE CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	PVOID HeapHandle;
	PMM_HEAP_TABLE Table;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_HeapCreate);
	CallbackPtr = (HEAPCREATE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		HeapHandle = (*CallbackPtr)(flOptions, dwInitialSize, dwMaximumSize);
		InterlockedDecrement(&Callback->References);
		return HeapHandle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	HeapHandle = (*CallbackPtr)(flOptions, dwInitialSize, dwMaximumSize);
	if (!HeapHandle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return HeapHandle;
	}

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	//
	// Track heap creator's stack trace
	//

	Table = MmInsertHeapTable(HeapHandle);
	Table->CreateStackHash = Hash;
	Table->CreateStackDepth = Depth;
	Table->CreatePc = Caller;

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return HeapHandle;
}

//
// HeapDestroy
//

BOOL WINAPI
HeapDestroyEnter( 
    IN HANDLE HeapHandle
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	HEAPDESTROY CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	PMM_HEAP_TABLE Table;
	BOOL Return;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_HeapDestroy);
	CallbackPtr = (HEAPDESTROY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Return = (*CallbackPtr)(HeapHandle);
		InterlockedDecrement(&Callback->References);
		return Return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Table = MmLookupHeapTable(HeapHandle);
	Return = (*CallbackPtr)(HeapHandle);

	if (!Return || !Table) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Return;
	}

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	//
	// Track heap destroyer's stack trace and retire the heap table
	//

	Table->DestroyStackHash = Hash;
	Table->DestroyStackDepth = Depth;
	Table->DestroyPc = Caller;
	MmRetireHeapTable(HeapHandle);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Return;
}

//
// GlobalAlloc
//

HGLOBAL WINAPI 
GlobalAllocEnter(
	__in UINT uFlags,
	__in SIZE_T dwBytes
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GLOBALALLOC CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_GlobalAlloc);
	CallbackPtr = (GLOBALALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(uFlags, dwBytes);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(uFlags, dwBytes);
	if (!Address || dwBytes == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)dwBytes;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 


	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmProcessHeap, Address, (ULONG)dwBytes, Duration, 
		                Hash, (USHORT)Depth, _GlobalAlloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// GlobalFree
//

HGLOBAL WINAPI 
GlobalFreeEnter(
	__in HGLOBAL hMem
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	GLOBALFREE CallbackPtr;
	PVOID Address;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_GlobalFree);
	CallbackPtr = (GLOBALFREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hMem);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Address = (*CallbackPtr)(hMem);
	if (Address != NULL || !hMem) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;

	}

	MmRemoveHeapRecord(MmProcessHeap, hMem);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// GlobalReAlloc
//

HGLOBAL WINAPI 
GlobalReAllocEnter(
	__in HGLOBAL hMem,
	__in SIZE_T dwBytes,
	__in UINT uFlags
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GLOBALREALLOC CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_GlobalReAlloc);
	CallbackPtr = (GLOBALREALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hMem, dwBytes, uFlags);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(hMem, dwBytes, uFlags);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)dwBytes;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (hMem != NULL) {
		MmRemoveHeapRecord(MmProcessHeap, hMem);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmProcessHeap, Address, (ULONG)dwBytes, Duration, 
		                Hash, (USHORT)Depth, _GlobalReAlloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// LocalAlloc
//

HLOCAL WINAPI 
LocalAllocEnter(
	__in UINT uFlags,
	__in SIZE_T dwBytes
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	LOCALALLOC CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_LocalAlloc);
	CallbackPtr = (LOCALALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(uFlags, dwBytes);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(uFlags, dwBytes);
	if (!Address || dwBytes == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)(dwBytes);
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmProcessHeap, Address, (ULONG)dwBytes, Duration, 
		                Hash, (USHORT)Depth, _LocalAlloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// LocalFree
//

HLOCAL WINAPI 
LocalFreeEnter(
	__in HLOCAL hMem
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	LOCALFREE CallbackPtr;
	PVOID Address;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_LocalFree);
	CallbackPtr = (LOCALFREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hMem);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Address = (*CallbackPtr)(hMem);
	if (Address != NULL || !hMem) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;

	}

	MmRemoveHeapRecord(MmProcessHeap, hMem);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// LocalReAlloc
//

HLOCAL WINAPI 
LocalReAllocEnter(
	__in HLOCAL hMem,
	__in SIZE_T dwBytes,
	__in UINT uFlags
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	LOCALREALLOC CallbackPtr;
	PVOID Frame;
	PVOID Address;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_LocalReAlloc);
	CallbackPtr = (LOCALREALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hMem, dwBytes, uFlags);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(hMem, dwBytes, uFlags);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)(ULONG_PTR)(dwBytes);
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (hMem != NULL) {
		MmRemoveHeapRecord(MmProcessHeap, hMem);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmProcessHeap, Address, (ULONG)dwBytes, Duration, 
		                Hash, (USHORT)Depth, _LocalReAlloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

BSTR WINAPI
SysAllocStringEnter(
	__in const OLECHAR *sz
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	SYSALLOCSTRING SysAllocStringPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BSTR Address;
	ULONG Size;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_SysAllocString);
	SysAllocStringPtr = (SYSALLOCSTRING)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*SysAllocStringPtr)(sz);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*SysAllocStringPtr)(sz);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Size = SysStringByteLen(Address);
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);

	Callers[0] = (PVOID)Caller;
	Callers[1] = UlongToPtr(Size);
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	MmInsertHeapRecord(MmBstrHeap, Address, Size, Duration, Hash, (USHORT)Depth,
		                _SysAllocString);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}
 
//
// SysAllocStringLen
//

BSTR WINAPI
SysAllocStringLenEnter(
	__in const OLECHAR *str, 
	__in UINT cch 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	SYSALLOCSTRINGLEN SysAllocStringLenPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BSTR Address;
	ULONG Size;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_SysAllocStringLen);
	SysAllocStringLenPtr = (SYSALLOCSTRINGLEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*SysAllocStringLenPtr)(str, cch);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*SysAllocStringLenPtr)(str, cch);
	if (!Address || cch == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Size = SysStringByteLen(Address);
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);

	Callers[0] = (PVOID)Caller;
	Callers[1] = UlongToPtr(Size);
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	MmInsertHeapRecord(MmBstrHeap, Address, Size, Duration, Hash, (USHORT)Depth,
		                _SysAllocStringLen);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// SysAllocStringByteLen
//

BSTR WINAPI 
SysAllocStringByteLenEnter(
	__in LPCSTR psz, 
	__in UINT len
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	SYSALLOCSTRINGBYTELEN SysAllocStringByteLenPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BSTR Address;
	ULONG Size;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_SysAllocStringByteLen);
	SysAllocStringByteLenPtr = (SYSALLOCSTRINGBYTELEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*SysAllocStringByteLenPtr)(psz, len);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*SysAllocStringByteLenPtr)(psz, len);
	if (!Address || len == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = UintToPtr(len);
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Size = SysStringByteLen(Address);
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmBstrHeap, Address, Size, Duration, Hash, (USHORT)Depth,
		                _SysAllocStringByteLen);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

INT WINAPI
SysReAllocStringEnter(
	__in BSTR *pbstr,
	__in const OLECHAR *psz
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	SYSREALLOCSTRING SysReAllocStringPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BSTR New;
	BSTR Old;
	ULONG Size;
	ULONG Duration;
	INT Return;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_SysReAllocString);
	SysReAllocStringPtr = (SYSREALLOCSTRING)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Return = (*SysReAllocStringPtr)(pbstr, psz);
		InterlockedDecrement(&Callback->References);
		return Return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	__try {
		Old = *pbstr;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		Old = NULL;
	}

	QueryPerformanceCounter(&Enter);

	Return = (*SysReAllocStringPtr)(pbstr, psz);
	if (!Return) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Return;
	}

	QueryPerformanceCounter(&Exit);

	New = *pbstr;
	Size = SysStringLen(New);
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);

	Callers[0] = (PVOID)Caller;
	Callers[1] = UlongToPtr(Size);
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (Old != NULL) {
		MmRemoveHeapRecord(MmBstrHeap, Old);
	}
	
	MmInsertHeapRecord(MmBstrHeap, New, Size, Duration, Hash, (USHORT)Depth,
		                _SysReAllocString);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Return;
}

//
// SysReAllocStringLen
//

INT WINAPI
SysReAllocStringLenEnter(
	__in BSTR* pbstr, 
	__in const OLECHAR* psz, 
	__in unsigned int len
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	SYSREALLOCSTRINGLEN SysReAllocStringLenPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BSTR New;
	BSTR Old;
	ULONG Size;
	ULONG Duration;
	INT Return;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_SysReAllocStringLen);
	SysReAllocStringLenPtr = (SYSREALLOCSTRINGLEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Return = (*SysReAllocStringLenPtr)(pbstr, psz, len);
		InterlockedDecrement(&Callback->References);
		return Return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	__try {
		Old = *pbstr;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		Old = NULL;
	}

	QueryPerformanceCounter(&Enter);

	Return = (*SysReAllocStringLenPtr)(pbstr, psz, len);
	if (!Return) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Return;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = UintToPtr(len);
	Callers[2] = (PVOID)1;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (Old != NULL) {
		MmRemoveHeapRecord(MmBstrHeap, Old);
	}
	
	New = *pbstr;
	Size = SysStringLen(New);

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmBstrHeap, New, Size, Duration, Hash, (USHORT)Depth,
		                _SysReAllocStringLen);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Return;
}

//
// SysFreeString
//

VOID WINAPI
SysFreeStringEnter(
	__in BSTR bstr
	)
{ 
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	SYSFREESTRING SysFreeStringPtr;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_SysFreeString);
	SysFreeStringPtr = (SYSFREESTRING)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		(*SysFreeStringPtr)(bstr);
		InterlockedDecrement(&Callback->References);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*SysFreeStringPtr)(bstr);
	MmRemoveHeapRecord(MmBstrHeap, bstr);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

//
// malloc
//

void* __cdecl
MallocEnter(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	MALLOC Malloc;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_Malloc);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*Malloc)(size);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*Malloc)(size);
	if (!Address || size == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)size;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmMsvcrtHeap, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}


// 
// calloc
//

void* __cdecl
CallocEnter( 
	size_t num,
	size_t size 
	)
{ 
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CALLOC Calloc;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_Calloc);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*Calloc)(num, size);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*Calloc)(num, size);
	if (!Address || (size * num) == 0) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)size;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmMsvcrtHeap, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// realloc
//

void* __cdecl
ReallocEnter(
	void *ptr,
	size_t size 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REALLOC Realloc;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_Realloc);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread || MmIsSkipAllocators()) {
		InterlockedIncrement(&Callback->References);
		Address = (*Realloc)(ptr, size);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*Realloc)(ptr, size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = (PVOID)size;
	Callers[2] = (PVOID)1UL;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		MmRemoveHeapRecord(MmMsvcrtHeap, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHeapRecord(MmMsvcrtHeap, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// free
//

void __cdecl
FreeEnter(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = MmGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		(*Free)(ptr);
		InterlockedDecrement(&Callback->References);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		MmRemoveHeapRecord(MmMsvcrtHeap, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}