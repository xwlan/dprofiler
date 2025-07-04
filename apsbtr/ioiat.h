#ifndef _IO_IAT_H_
#define _IO_IAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"

//
// Routines for IO profile work under IAT mode
//

typedef enum _IO_PATCH {
	_IatExitProcess,
	_IatCreateFile,
	_IatCloseHandle,
	_IatReadFile,
	_IatWriteFile,
	_IatGetOverlappedResult,
	_IatGetQueuedCompletionStatus,
	_IatGetQueuedCompletionStatusEx,  // NT 6+
	_IatPostQueuedCompletionStatus,
} IO_PATCH;

VOID
IoIatInitialize(
	VOID
	);

VOID
IoIatApplyPatch(
	VOID
	);

VOID
IoDllLoadCallback(
	IN struct _BTR_PROFILE_OBJECT* Object,
	IN struct _BTR_MODULE* Dll,
	IN BOOLEAN Load
	);


//
// IAT mode routines
//

HANDLE WINAPI
IatCreateFile(
	_In_     LPCWSTR               lpFileName,
	_In_     DWORD                 dwDesiredAccess,
	_In_     DWORD                 dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_     DWORD                 dwCreationDisposition,
	_In_     DWORD                 dwFlagsAndAttributes,
	_In_opt_ HANDLE                hTemplateFile
	);

BOOL WINAPI
IatCloseHandle(
	_In_ HANDLE Handle
	);

BOOL WINAPI
IatReadFile(
	_In_        HANDLE       hFile,
	_Out_       LPVOID       lpBuffer,
	_In_        DWORD        nNumberOfBytesToRead,
	_Out_opt_   LPDWORD      lpNumberOfBytesRead,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	);

BOOL WINAPI
IatWriteFile(
	_In_        HANDLE       hFile,
	_Out_       LPVOID       lpBuffer,
	_In_        DWORD        nNumberOfBytesToWrite,
	_Out_opt_   LPDWORD      lpNumberOfBytesWrite,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	);

BOOL WINAPI
IatGetOverlappedResult(
	_In_  HANDLE       hFile,
	_In_  LPOVERLAPPED lpOverlapped,
	_Out_ LPDWORD      lpNumberOfBytesTransferred,
	_In_  BOOL         bWait
	);

BOOL WINAPI
IatGetQueuedCompletionStatus(
	_In_  HANDLE       CompletionPort,
	_Out_ LPDWORD      lpNumberOfBytes,
	_Out_ PULONG_PTR   lpCompletionKey,
	_Out_ LPOVERLAPPED* lpOverlapped,
	_In_  DWORD        dwMilliseconds
	);

BOOL WINAPI
IatGetQueuedCompletionStatusEx(
	_In_  HANDLE             CompletionPort,
	_Out_ LPOVERLAPPED_ENTRY lpCompletionPortEntries,
	_In_  ULONG              ulCount,
	_Out_ PULONG             ulNumEntriesRemoved,
	_In_  DWORD              dwMilliseconds,
	_In_  BOOL               fAlertable
	);

BOOL WINAPI
IatPostQueuedCompletionStatus(
	_In_     HANDLE       CompletionPort,
	_In_     DWORD        dwNumberOfBytesTransferred,
	_In_     ULONG_PTR    dwCompletionKey,
	_In_opt_ LPOVERLAPPED lpOverlapped
	);

#ifdef __cplusplus
}
#endif
#endif