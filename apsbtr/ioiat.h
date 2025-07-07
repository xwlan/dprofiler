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
	_IatAccept,
	_IatRecv,
	_IatSend,
	_IatCloseSocket,
	_IatWSAAccept,
	_IatWSARecv,
	_IatWSASend,
	_IatWSAGetOverlappedResult,
	_IatAcceptEx,
	_IatTransmitFile,

	//
	// UDP is default disabled
	//

	_IatRecvFrom,
	_IatSendTo,
	_IatWSARecvFrom,
	_IatWSASendTo,
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

SOCKET WINAPI
IatWSAAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ LPINT addrlen,
	_In_  LPCONDITIONPROC lpfnCondition,
	_In_  DWORD dwCallbackData
	);

int WINAPI
IatWSARecv(
	_In_    SOCKET s,
	_Inout_ LPWSABUF lpBuffers,
	_In_    DWORD dwBufferCount,
	_Out_   LPDWORD lpNumberOfBytesRecvd,
	_Inout_ LPDWORD lpFlags,
	_In_    LPWSAOVERLAPPED lpOverlapped,
	_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

int WINAPI
IatWSASend(
	_In_  SOCKET s,
	_In_  LPWSABUF lpBuffers,
	_In_  DWORD dwBufferCount,
	_Out_ LPDWORD lpNumberOfBytesSent,
	_In_  DWORD dwFlags,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

BOOL WINAPI
IatWSAGetOverlappedResult(
	_In_  SOCKET          s,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_Out_ LPDWORD         lpcbTransfer,
	_In_  BOOL            fWait,
	_Out_ LPDWORD         lpdwFlags
	);

BOOL WINAPI
IatAcceptEx(
	_In_  SOCKET sListenSocket,
	_In_  SOCKET sAcceptSocket,
	_In_  PVOID lpOutputBuffer,
	_In_  DWORD dwReceiveDataLength,
	_In_  DWORD dwLocalAddressLength,
	_In_  DWORD dwRemoteAddressLength,
	_Out_ LPDWORD lpdwBytesReceived,
	_In_  LPOVERLAPPED lpOverlapped
	);

BOOL WINAPI
IatTransmitFile(
	IN SOCKET hSocket,
	IN HANDLE hFile,
	DWORD nNumberOfBytesToWrite,
	DWORD nNumberOfBytesPerSend,
	LPOVERLAPPED lpOverlapped,
	LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
	DWORD dwFlags
	);

SOCKET WINAPI
IatAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ int* addrlen
	);

int WINAPI
IatRecv(
	_In_  SOCKET s,
	_Out_ char* buf,
	_In_  int    len,
	_In_  int    flags
	);

int WINAPI
IatSend(
	_In_ SOCKET s,
	_In_ const char* buf,
	_In_ int len,
	_In_ int flags
	);

int WINAPI
IatCloseSocket(
	_In_ SOCKET s
	);

int WINAPI
IatRecvFrom(
	_In_  SOCKET s,
	_Out_ char* buf,
	_In_  int len,
	_In_  int flags,
	_Out_ struct sockaddr* from,
	_Inout_opt_ int* fromlen
	);

int WINAPI
IatSendTo(
	_In_  SOCKET s,
	_In_ const char* buf,
	_In_ int len,
	_In_ int flags,
	_In_ const struct sockaddr* to,
	_In_ int tolen
	);

int WINAPI
IatWSARecvFrom(
	_In_    SOCKET s,
	_Inout_ LPWSABUF lpBuffers,
	_In_    DWORD dwBufferCount,
	_Out_   LPDWORD lpNumberOfBytesRecvd,
	_Inout_ LPDWORD lpFlags,
	_Out_   struct sockaddr* lpFrom,
	_Inout_ LPINT lpFromlen,
	_In_    LPWSAOVERLAPPED lpOverlapped,
	_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

int WINAPI
IatWSASendTo(
	_In_  SOCKET s,
	_In_  LPWSABUF lpBuffers,
	_In_  DWORD dwBufferCount,
	_Out_ LPDWORD lpNumberOfBytesSent,
	_In_  DWORD dwFlags,
	_In_  const struct sockaddr* lpTo,
	_In_  int iToLen,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

#ifdef __cplusplus
}
#endif
#endif