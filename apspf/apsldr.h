//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _APS_LDR_H_
#define _APS_LDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable : 4996)

#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <assert.h>
#include "list.h"

//
// APS_PREEXECUTE_CONTEXT is used to inject runtime
// before the process's entry point is executed.
//

#pragma pack(push, 1)

#if defined (_M_IX86) 

typedef struct _APS_PREEXECUTE_CONTEXT {
	ULONG EFlag;
	ULONG Edi;
	ULONG Esi;
	ULONG Ebp;
	ULONG Esp;
	ULONG Ebx;
	ULONG Edx;
	ULONG Ecx;
	ULONG Eax;
	ULONG Eip;
	HANDLE CompleteEvent;
	HANDLE SuccessEvent;
	HANDLE ErrorEvent;
	PVOID LoadLibraryAddr;
	PVOID SetEventAddr;
	PVOID WaitForSingleObjectAddr;
	PVOID CloseHandleAddr;
	WCHAR Path[MAX_PATH];
} APS_PREEXECUTE_CONTEXT, *PAPS_PREEXECUTE_CONTEXT;

#elif defined (_M_X64)

typedef struct _APS_PREEXECUTE_CONTEXT {
	ULONG64 Rip;
	ULONG64 Rbx;
	ULONG64 Rcx;
	ULONG64 Rdx;
	ULONG64 R8;
	ULONG64 R9;
	HANDLE CompleteEvent;
	HANDLE SuccessEvent;
	HANDLE ErrorEvent;
	PVOID LoadLibraryAddr;
	PVOID SetEventAddr;
	PVOID WaitForSingleObjectAddr;
	PVOID CloseHandleAddr;
	WCHAR Path[MAX_PATH];
} APS_PREEXECUTE_CONTEXT, *PAPS_PREEXECUTE_CONTEXT;

#endif

#pragma pack(pop)


ULONG
ApsInjectPreExecute(
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in HANDLE ThreadHandle,
	__in PWSTR RuntimePath,
	__in HANDLE CompleteEvent,
	__in HANDLE SuccessEvent, 
	__in HANDLE ErrorEvent,
	__out PVOID *CodePtr
	);

#ifdef __cplusplus
}
#endif

#endif // _APS_LDR_H_