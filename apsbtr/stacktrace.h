//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"

ULONG
BtrInitializeStack(
	__in PBTR_PROFILE_OBJECT Object
	);

VOID
BtrUninitializeStack(
	VOID
	);

VOID
BtrCloseStackFile(
	VOID
	);

PBTR_STACK_PAGE
BtrAllocateStackPage(
	VOID
	);

PBTR_STACK_ENTRY
BtrAllocateStackEntry(
	VOID
	);

ULONG
BtrInsertStackEntry(
	__in PBTR_STACK_RECORD Record
	);

PBTR_STACK_ENTRY 
BtrLookupStackEntry(
	__in PBTR_STACK_ENTRY Lookup 
	);

PBTR_STACK_RECORD
BtrAllocateStackRecord(
	VOID
	);

VOID
BtrFreeStackRecord(
	__in PBTR_STACK_RECORD Record
	);

VOID
BtrFlushStackRecord(
	VOID
	);

ULONG
BtrWriteStackRecord(
	__in PBTR_STACK_RECORD Record
	);

VOID
BtrLockStackTable(
	VOID
	);

VOID
BtrCaptureStackTrace(
	__in PVOID Frame, 
	__in PVOID Address,
	__out PULONG Hash,
	__out PULONG Depth
	);

ULONG
BtrCaptureStackTraceEx(
	__in struct _BTR_THREAD_OBJECT *Thread,
	__in PVOID Callers[],
	__in ULONG MaxDepth,
	__in PVOID Frame, 
	__in PVOID Address,
	__out PULONG Hash,
	__out PULONG Depth
	);

BOOLEAN
BtrWalkCallStack(
	__in PVOID Fp,
	__in PVOID Address,
	__in ULONG Count,
	__out PULONG CapturedCount,
	__out PVOID  *Frame,
	__out PULONG Hash
	);

ULONG
BtrWalkFrameChain(
	__in ULONG_PTR Fp,
    __in ULONG Count, 
    __out PVOID *Callers,
	__out PULONG Hash
    );


ULONG
BtrRemoteWalkStack(
	__in ULONG ThreadId,
	__in ULONG_PTR StackBase,
	__in ULONG_PTR StackLimit,
	__in PCONTEXT Context,
    __in ULONG Count, 
    __out PVOID *Callers,
	__out PULONG Hash
	);

ULONG
BtrValidateStackTrace(
	__in PULONG_PTR Callers,
	__in ULONG Depth
	);

extern BTR_STACK_TABLE BtrStackTable;

#ifdef __cplusplus
}
#endif
#endif