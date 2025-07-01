#ifndef _MM_HEAP_IAT_H_
#define _MM_HEAP_IAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"

//
// Routines for MM Heap profile work under IAT mode
//

typedef enum _MM_HEAP_PATCH {
	_HeapAlloc,
	_HeapReAlloc,
	_HeapFree,
} MM_HEAP_PATCH;

VOID
MmHeapIatInitialize(
	VOID
	);

VOID
MmHeapIatApplyPatch(
	VOID
	);

VOID
MmHeapDllLoadCallback(
	IN struct _BTR_PROFILE_OBJECT* Object,
	IN struct _BTR_MODULE* Dll,
	IN BOOLEAN Load
	);

PVOID WINAPI
MmHeapAlloc(
	IN PVOID  HeapHandle,
	IN ULONG  Flags,
	IN SIZE_T Size
	);

PVOID WINAPI
MmHeapReAlloc(
	HANDLE Heap,
	ULONG Flags,
	PVOID Ptr,
	SIZE_T Size
	);

BOOLEAN WINAPI
MmHeapFree(
	IN PVOID  HeapHandle,
	IN ULONG  Flags,
	IN PVOID  HeapBase
	);

#ifdef __cplusplus
}
#endif
#endif