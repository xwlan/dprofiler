//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _INJECTLDR_H_
#define _INJECTLDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

//
// BSP_LDR_DATA define injection data requires to 
// load runtime into target process, note that the
// following structure must be strictly aligned 16
// bytes, same with ldr32.asm or ldr64.asm
//

#pragma pack(push, 16)

typedef struct _BSP_LDR_DATA {

	PVOID Address; 
	ULONG Executed;
	ULONG LastErrorValue; 
	PVOID GetLastErrorPtr;
	PVOID SetLastErrorPtr;
	PVOID CreateThreadPtr;
	PVOID LoadLibraryPtr;
	PVOID CloseHandlePtr;
	PVOID WaitForSingleObjectPtr;

#if defined (_M_IX86)
	ULONG FpuStateRequired;
	ULONG64 FpuState;
#endif

	WCHAR DllFullPath[512];

} BSP_LDR_DATA, *PBSP_LDR_DATA;

#pragma pack(pop)

C_ASSERT((sizeof(BSP_LDR_DATA) % 16 == 0));

//
// Hijackable thread (thread' next pc is ret)
//

typedef struct _BSP_LDR_THREAD {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Context;
	PVOID AdjustedEip;
	PVOID AdjustedEsp;
} BSP_LDR_THREAD, *PBSP_LDR_THREAD;


//
// N.B. This structure should match ldr32.asm, ldr64.asm declaration
//

#pragma pack(push, 1)

typedef struct BSP_LDR_DBG	{
	ULONG_PTR DllFullPath;       
	ULONG_PTR LoadLibraryPtr;    
	ULONG_PTR RtlCreateUserThreadPtr;    
	ULONG_PTR RtlExitUserThreadPtr;
	//ULONG_PTR LdrLoadDllPtr;
	ULONG ThreadId;
	LONG NtStatus;
	WCHAR Path[MAX_PATH];
	CHAR Code[200];
} BSP_LDR_DBG, *PBSP_LDR_DBG;

#pragma pack(pop)

typedef struct _BSP_LDR_DLL {
	UNICODE_STRING FileName;
} BSP_LDR_DLL, *PBS_LDR_DLL;

//
// BSP_PREEXECUTE_CONTEXT is used to inject runtime
// before the process's entry point is executed.
//

#pragma pack(push, 1)

#if defined (_M_IX86) 

typedef struct _BSP_PREEXECUTE_CONTEXT {
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
} BSP_PREEXECUTE_CONTEXT, *PBSP_PREEXECUTE_CONTEXT;

#elif defined (_M_X64)

typedef struct _BSP_PREEXECUTE_CONTEXT {
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
} BSP_PREEXECUTE_CONTEXT, *PBSP_PREEXECUTE_CONTEXT;

#endif

#pragma pack(pop)

//
// Scan hijackable threads and return its handle,
// with debug privilege enabled, all threads suspended
// on success, if error, threads are resumed, debug
// privilege disabled, Handle is only valid on success.
//

ULONG
BspScanHijackableThread(
	IN ULONG ProcessId,
	OUT PHANDLE Handle,
	OUT PLIST_ENTRY ThreadList
	);

//
// Inject runtime dll into address space of target process
//

ULONG
BspExecuteLoadRoutine(
	IN HANDLE ProcessHandle,
	IN PWSTR DllFullPath,
	IN PLIST_ENTRY ThreadList
	);

ULONG
BspLoadLibraryUnsafe(
	IN ULONG ProcessId,
	PWSTR DllFullPath
	);

ULONG 
BspQueueUserApc(
	IN HANDLE ProcessHandle,
	IN PWSTR DllFullPath,
	IN PLIST_ENTRY ThreadList
	);

ULONG
BspLoadLibraryEx(
	IN ULONG ProcessId,
	IN PWSTR DllFullPath
	);

ULONG
BspHijackRemoteBreakIn(
	IN DEBUG_EVENT *DebugEvent,
	IN PWSTR DllFullPath,
	OUT PBOOLEAN Hijacked
	);

ULONG
BspScanOwnedLocks(
	IN HANDLE ProcessHandle
	);

ULONG
BspExecuteRemoteCall(
	IN HANDLE ProcessHandle,
	IN PVOID CallSite,
	IN ULONG ArgumentCount,
	IN PULONG_PTR Argument,
	IN PBSP_LDR_THREAD Thread,
	IN PVOID SuspendThreadPtr,
	IN HANDLE CompleteEvent,
	IN PVOID SetEventPtr
	);

ULONG
BspCleanRemoteCall(
	IN HANDLE ProcessHandle,
	IN PVOID CallSite,
	IN ULONG ArgumentCount,
	IN PBSP_LDR_THREAD Thread 
	);

//
// Declare assembler global label in ldr32.asm or
// ldr64.asm as C function prototypes to make linker
// find them.
//

VOID
BspLoadRoutineCode(
	VOID
	);

VOID
BspLoadRoutineEnd(
	VOID
	);

VOID
BspDebuggerThread(
	VOID
	);

VOID
BspDebuggerThreadEnd(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif