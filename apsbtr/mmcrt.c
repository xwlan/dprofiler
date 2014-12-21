//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "btr.h"
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
#include "ipc.h"
#include "cache.h"
#include "backtrace.h"
#include "util.h"
#include "queue.h"
#include "btrdpf.h"
#include "mmcrt.h"

typedef intptr_t 
(__cdecl *CRT_GETHEAPHANDLE)(void);
 
//
// get_heap_handle() ptr, heap handle for all possible
// msvcrt versions, include its debug version, only apply
// to crt's dll version, not for static linked crt.
//

CRT_GETHEAPHANDLE CrtGetHeapHandle;
CRT_GETHEAPHANDLE CrtGetHeapHandleD;
PVOID CrtHeapHandle;
PVOID CrtHeapHandleD;

CRT_GETHEAPHANDLE CrtGetHeapHandle71;
CRT_GETHEAPHANDLE CrtGetHeapHandle71D;
PVOID CrtHeapHandle71;
PVOID CrtHeapHandle71D;

CRT_GETHEAPHANDLE CrtGetHeapHandle80;
CRT_GETHEAPHANDLE CrtGetHeapHandle80D;
PVOID CrtHeapHandle80;
PVOID CrtHeapHandle80D;

CRT_GETHEAPHANDLE CrtGetHeapHandle90;
CRT_GETHEAPHANDLE CrtGetHeapHandle90D;
PVOID CrtHeapHandle90;
PVOID CrtHeapHandle90D;

CRT_GETHEAPHANDLE CrtGetHeapHandle100;
CRT_GETHEAPHANDLE CrtGetHeapHandle100D;
PVOID CrtHeapHandle100;
PVOID CrtHeapHandle100D;

//
// malloc
//

void* __cdecl
CrtMalloc(
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
	Callback = BtrGetHeapCallback(_Malloc);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMallocD(
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
	Callback = BtrGetHeapCallback(_MallocD);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandleD, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_MallocD);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc71(
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
	Callback = BtrGetHeapCallback(_Malloc71);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc71);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc71D(
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
	Callback = BtrGetHeapCallback(_Malloc71D);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc71D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc80(
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
	Callback = BtrGetHeapCallback(_Malloc80);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc80);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc80D(
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
	Callback = BtrGetHeapCallback(_Malloc80D);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc80D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc90(
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
	Callback = BtrGetHeapCallback(_Malloc90);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc90);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc90D(
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
	Callback = BtrGetHeapCallback(_Malloc90D);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc90D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc100(
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
	Callback = BtrGetHeapCallback(_Malloc100);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc100);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtMalloc100D(
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
	Callback = BtrGetHeapCallback(_Malloc100D);
	Malloc = (MALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Malloc)(size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Malloc100D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

// 
// calloc
//

void* __cdecl
CrtCalloc( 
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
	Callback = BtrGetHeapCallback(_Calloc);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}
 
void* __cdecl
CrtCallocD( 
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
	Callback = BtrGetHeapCallback(_CallocD);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandleD, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_CallocD);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc71( 
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
	Callback = BtrGetHeapCallback(_Calloc71);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc71);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc71D( 
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
	Callback = BtrGetHeapCallback(_Calloc71D);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71D, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc71D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc80( 
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
	Callback = BtrGetHeapCallback(_Calloc80);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc80);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc80D( 
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
	Callback = BtrGetHeapCallback(_Calloc80D);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80D, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc80D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc90( 
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
	Callback = BtrGetHeapCallback(_Calloc90);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc90);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc90D( 
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
	Callback = BtrGetHeapCallback(_Calloc90D);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90D, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc90D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc100( 
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
	Callback = BtrGetHeapCallback(_Calloc100);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc100);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtCalloc100D( 
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
	Callback = BtrGetHeapCallback(_Calloc100D);
	Calloc = (CALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Calloc)(num, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100D, Address, (ULONG)(num * size), Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Calloc100D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// realloc
//

void* __cdecl
CrtRealloc(
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
	Callback = BtrGetHeapCallback(_Realloc);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}
 
void* __cdecl
CrtReallocD(
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
	Callback = BtrGetHeapCallback(_ReallocD);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandleD, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandleD, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_ReallocD);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc71(
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
	Callback = BtrGetHeapCallback(_Realloc71);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc71);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc71D(
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
	Callback = BtrGetHeapCallback(_Realloc71D);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71D, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc71D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc80(
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
	Callback = BtrGetHeapCallback(_Realloc80);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc80);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc80D(
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
	Callback = BtrGetHeapCallback(_Realloc80D);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80D, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc80D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc90(
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
	Callback = BtrGetHeapCallback(_Realloc90);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc90);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc90D(
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
	Callback = BtrGetHeapCallback(_Realloc90D);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90D, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc90D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc100(
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
	Callback = BtrGetHeapCallback(_Realloc100);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc100);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtRealloc100D(
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
	Callback = BtrGetHeapCallback(_Realloc100D);
	Realloc = (REALLOC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*Realloc)(ptr, size);
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

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100D, ptr);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_Realloc100D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// free
//

void __cdecl
CrtFree(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFreeD(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandleD, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree71(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree71D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71D, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree80(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree80D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80D, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree90(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree90D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90D, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree100(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtFree100D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	FREE Free;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Free);
	Free = (FREE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Free)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Free)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100D, ptr);	
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}


void* __cdecl
CrtNew(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewD(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewD);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandleD, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewD);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNew71(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New71);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New71);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNew71D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New71D);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New71D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNew80(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New80);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New80);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNew80D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New80D);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New80D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNew90(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New90);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New90);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNew90D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New90);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New90D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void * __cdecl 
CrtNew100(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New100);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New100);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void * __cdecl 
CrtNew100D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEW New;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_New100D);
	New = (OP_NEW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*New)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*New)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_New100D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArrayD(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArrayD);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandleD, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArrayD);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray71(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray71);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray71);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray71D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray71D);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle71D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray71D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray80(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray80);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray80);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray80D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray80D);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle80D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray80D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray90(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray90);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray90);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void* __cdecl
CrtNewArray90D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray90D);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle90D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray90D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void * __cdecl 
CrtNewArray100(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray100);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray100);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

void * __cdecl 
CrtNewArray100D(
	size_t size
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OP_NEWARRAY NewArray;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_NewArray100D);
	NewArray = (OP_NEWARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		return (*NewArray)(size);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*NewArray)(size);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;	
	}

	QueryPerformanceCounter(&Exit);

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	BtrInsertHeapRecord(CrtHeapHandle100D, Address, (ULONG)size, Duration, 
		                Hash, (USHORT)Depth, (USHORT)_NewArray100D);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

//
// operator delete
//

void __cdecl
CrtDelete(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteD(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteD);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandleD, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete71(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete71);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete71D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete71D);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete80(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete80);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete80D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete80D);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete90(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete90);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete90D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete90D);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete100(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete100);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDelete100D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETE Delete;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_Delete100D);
	Delete = (OP_DELETE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*Delete)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*Delete)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArrayD(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArrayD);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandleD, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray71(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray71);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray71D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray71D);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle71D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray80(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray80);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray80D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray80D);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle80D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray90(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray90);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray90D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray90D);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle90D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray100(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray100);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

void __cdecl
CrtDeleteArray100D(
	void *ptr
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	OP_DELETEARRAY DeleteArray;
	PVOID Caller;

	Caller = _ReturnAddress();
	Callback = BtrGetHeapCallback(_DeleteArray100D);
	DeleteArray = (OP_DELETEARRAY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		(*DeleteArray)(ptr);
		return;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	(*DeleteArray)(ptr);

	if (ptr) {
		BtrRemoveHeapRecord(CrtHeapHandle100D, ptr);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}