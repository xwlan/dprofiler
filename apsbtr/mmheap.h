//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_HEAP_H_
#define _MM_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mmprof.h"

ULONG
MmGetHeapHandles(
	VOID
	);

//
// RtlAllocateHeap
//

PVOID NTAPI
RtlAllocateHeapEnter( 
	IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T  Size
	); 

typedef PVOID 
(NTAPI *RTLALLOCATEHEAP)( 
	IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T  Size
	); 

//
// RtlReAllocateHeap
//

PVOID NTAPI
RtlReAllocateHeapEnter(
    HANDLE Heap,
    ULONG Flags,
    PVOID Ptr,
    SIZE_T Size
	);

typedef PVOID 
(NTAPI *RTLREALLOCATEHEAP)(
    HANDLE Heap,
    ULONG Flags,
    PVOID Ptr,
    SIZE_T Size
	);

//
// RtlFreeHeap
//

BOOLEAN NTAPI
RtlFreeHeapEnter( 
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID  HeapBase
    ); 

typedef BOOLEAN
(NTAPI *RTLFREEHEAP)(
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID  HeapBase
    ); 

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
    ); 

typedef PVOID
(NTAPI *RTLCREATEHEAP)( 
    IN ULONG  Flags,
    IN PVOID  HeapBase  OPTIONAL,
    IN SIZE_T ReserveSize  OPTIONAL,
    IN SIZE_T CommitSize  OPTIONAL,
    IN PVOID  Lock  OPTIONAL,
    IN PVOID Parameters  OPTIONAL
    ); 

//
// RtlDestroyHeap
//

PVOID NTAPI
RtlDestroyHeapEnter( 
    IN PVOID  HeapHandle
    ); 

typedef PVOID 
(NTAPI *RTLDESTROYHEAP)( 
    IN PVOID  HeapHandle
    ); 

//
// HeapAlloc
//

PVOID WINAPI
HeapAllocEnter(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN SIZE_T dwBytes 
	);

typedef PVOID 
(WINAPI *HEAPALLOC)(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN SIZE_T dwBytes 
	);

//
// HeapFree
//

BOOL WINAPI
HeapFreeEnter(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN PVOID lpMem 
	);

typedef BOOL
(WINAPI *HEAPFREE)(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN PVOID lpMem 
	);

//
// HeapReAlloc
//

LPVOID WINAPI 
HeapReAllocEnter(
	__in HANDLE hHeap,
	__in DWORD dwFlags,
	__in LPVOID lpMem,
	__in SIZE_T dwBytes
	);

typedef LPVOID 
(WINAPI *HEAPREALLOC)(
	__in HANDLE hHeap,
	__in DWORD dwFlags,
	__in LPVOID lpMem,
	__in SIZE_T dwBytes
	);

//
// HeapCreate
//

PVOID WINAPI
HeapCreateEnter( 
	__in DWORD flOptions,
	__in SIZE_T dwInitialSize,
	__in SIZE_T dwMaximumSize
	);

typedef PVOID 
(WINAPI *HEAPCREATE)( 
	__in DWORD flOptions,
	__in SIZE_T dwInitialSize,
	__in SIZE_T dwMaximumSize
    ); 

//
// HeapDestroy
//

BOOL WINAPI
HeapDestroyEnter( 
    IN HANDLE hHeap 
    ); 

typedef BOOL 
(WINAPI *HEAPDESTROY)( 
    IN HANDLE hHeap 
    ); 

//
// GlobalAlloc
//

HGLOBAL WINAPI 
GlobalAllocEnter(
	__in UINT uFlags,
	__in SIZE_T dwBytes
	);

typedef HGLOBAL 
(WINAPI *GLOBALALLOC)(
	__in UINT uFlags,
	__in SIZE_T dwBytes
	);

//
// GlobalFree
//

HGLOBAL WINAPI 
GlobalFreeEnter(
	__in HGLOBAL hMem
	);

typedef HGLOBAL 
(WINAPI *GLOBALFREE)(
	__in HGLOBAL hMem
	);

//
// GlobalReAlloc
//

HGLOBAL WINAPI 
GlobalReAllocEnter(
	__in HGLOBAL hMem,
	__in SIZE_T dwBytes,
	__in UINT uFlags
	);

typedef HGLOBAL 
(WINAPI *GLOBALREALLOC)(
	__in HGLOBAL hMem,
	__in SIZE_T dwBytes,
	__in UINT uFlags
	);

//
// LocalAlloc
//

HLOCAL WINAPI 
LocalAllocEnter(
	__in UINT uFlags,
	__in SIZE_T dwBytes
	);

typedef HLOCAL 
(WINAPI *LOCALALLOC)(
	__in UINT uFlags,
	__in SIZE_T dwBytes
	);

//
// LocalFree
//

HLOCAL WINAPI 
LocalFreeEnter(
	__in HLOCAL hMem
	);

typedef HLOCAL 
(WINAPI *LOCALFREE)(
	__in HLOCAL hMem
	);

//
// LocalReAlloc
//

HLOCAL WINAPI 
LocalReAllocEnter(
	__in HLOCAL hMem,
	__in SIZE_T dwBytes,
	__in UINT uFlags
	);

typedef HLOCAL 
(WINAPI *LOCALREALLOC)(
	__in HLOCAL hMem,
	__in SIZE_T dwBytes,
	__in UINT uFlags
	);

//
// SysAllocString
//

BSTR WINAPI
SysAllocStringEnter(
	__in const OLECHAR  *sz
	);

typedef BSTR 
(WINAPI *SYSALLOCSTRING)(
	__in const OLECHAR  *sz
	);

//
// SysAllocStringLen
//

BSTR WINAPI
SysAllocStringLenEnter(
	__in const OLECHAR * str, 
	__in UINT cch
	);

typedef BSTR 
(WINAPI *SYSALLOCSTRINGLEN)(
	__in const OLECHAR * str, 
	__in UINT cch
	);

//
// SysAllocStringByteLen
//

BSTR WINAPI 
SysAllocStringByteLenEnter(
	__in LPCSTR psz, 
	__in UINT len
	);

typedef BSTR 
(WINAPI *SYSALLOCSTRINGBYTELEN)(
	__in LPCSTR psz, 
	__in UINT len
	);

//
// SysReAllocString
//

INT WINAPI
SysReAllocStringEnter(
	__in BSTR  *pbstr,
	__in const OLECHAR *psz
	);

typedef INT 
(WINAPI *SYSREALLOCSTRING)(
	__in BSTR  *pbstr,
	__in const OLECHAR *psz
	);

//
// SysReAllocStringLen
//

INT WINAPI
SysReAllocStringLenEnter(
	__in BSTR* pbstr, 
	__in const OLECHAR* psz, 
	__in unsigned int len
	);
 
typedef INT 
(WINAPI *SYSREALLOCSTRINGLEN)(
	__in BSTR* pbstr, 
	__in const OLECHAR* psz, 
	__in unsigned int len
	);

//
// SysFreeString
//

VOID WINAPI
SysFreeStringEnter(
	__in BSTR bstr
	);

typedef VOID 
(WINAPI *SYSFREESTRING)(
	__in BSTR bstr
	);

//
// malloc
//

void* __cdecl
MallocEnter(
	size_t size
	);

typedef void* 
(__cdecl *MALLOC)(
	size_t size
	);

//
// calloc
//

void* __cdecl
CallocEnter(
	size_t num, 
	size_t size
	);

typedef void* 
(__cdecl *CALLOC)(
	size_t num, 
	size_t size
	);

//
// realloc
//

void* __cdecl 
ReallocEnter(
	void *ptr, 
	size_t size
	);

typedef void* 
(__cdecl *REALLOC)(
	void *ptr, 
	size_t size
	);

//
// free
//

void __cdecl
FreeEnter(
	void *ptr
	);

typedef void 
(__cdecl *FREE)(
	void *ptr
	);

//
// _get_heaphandle() of msvcrt.dll
//

typedef intptr_t 
(__cdecl *GETHEAPHANDLE)(
	void
	);

//
// External Declaration
//

extern BTR_CALLBACK MmHeapCallback[];
extern ULONG MmHeapCallbackCount;

#ifdef __cplusplus
}
#endif
#endif