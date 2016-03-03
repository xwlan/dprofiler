//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _IO_DEF_H_
#define _IO_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "mmhandle.h"
#include "ioprof.h"

typedef struct _IO_OBJECT {

	union {
		LIST_ENTRY ListEntry;
		SLIST_ENTRY SListEntry;
	};

	LIST_ENTRY DuplicatedList;

	HANDLE Object;
	HANDLE_TYPE Type;
	ULONG Id;
	ULONG StackId;
	LONG References;
	BTR_SPINLOCK Lock;

	ULONG Flags;
	ULONG Hash;

	union {
		struct {
			ULONG Length;
			PWSTR Name;
		} File;

		struct {
			SOCKADDR_STORAGE Local;  // for both IPV4/IPV6
			SOCKADDR_STORAGE Remote; // for both IPV4/IPV6
		} Socket;
	} u;

	ULONG ReadCount;
	ULONG WriteCount;
	ULONG AbortCount;

	ULONG64 ReadBytes;
	ULONG64 WriteBytes;
	ULONG64 AbortBytes;

	//
	// Object live time
	//

	FILETIME Start;
	FILETIME End;

} IO_OBJECT, *PIO_OBJECT;

#define MAX_OBJECT_BUCKET 4093

typedef struct _IO_OBJECT_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Entry[MAX_OBJECT_BUCKET];
} IO_OBJECT_TABLE, *PIO_OBJECT_TABLE;

//
// Thread pool IO callback to hijack the user's callback
//

typedef struct _IO_TPIO_CONTEXT {

	LIST_ENTRY ListEntry;

	//
	// 4 parameters when threadpool io is created
	//

	HANDLE                fl;
	PTP_WIN32_IO_CALLBACK pfnio;
	PVOID                 pv;
	PTP_CALLBACK_ENVIRON  pcbe;

	//
	// The returned _TP_IO ptr 
	//

	PTP_IO TpIo;

	//
	// Our's context information
	//

	PTP_WIN32_IO_CALLBACK Callback;

} IO_TPIO_CONTEXT, *PIO_TPIO_CONTEXT;	 	

//
// return address of intercepted routine
//

#define IO_COMPLETE_SIZE(_I) \
	((ULONG)_I->Overlapped.InternalHigh)

#define SK_HANDLE(_S) \
	((HANDLE)(ULONG_PTR)_S)

ULONG
IoInitObjectTable(
	VOID
	);

PIO_OBJECT
IoAllocateObject(
	VOID
	);

PIO_OBJECT
IoAllocateFileObject(
	_In_ HANDLE Handle,
	_In_ BOOLEAN Overlapped
	);

PIO_OBJECT
IoAllocateSocketObject(
	_In_ HANDLE Handle
	);

VOID
IoFreeObject(
	_In_ PIO_OBJECT Object 
	);

PIO_OBJECT
IoInsertObject(
	__in PIO_OBJECT Object
	);

VOID
IoMarkObjectOverlapped(
	_In_ PIO_OBJECT Object
	);

VOID
IoClearObjectOverlapped(
	_In_ PIO_OBJECT Object
	);
	
BOOLEAN
IoIsObjectOverlapped(
	_In_ PIO_OBJECT Object 
	);

BOOLEAN
IoIsObjectSkipOnSuccess(
	_In_ PIO_OBJECT Object 
	);

BOOLEAN
IoRemoveObjectByHandle(
	__in HANDLE Handle 
	);

BOOLEAN
IoRemoveObjectByHandleEx(
	__in HANDLE Handle,
	__in HANDLE_TYPE Type
	);

PIO_OBJECT
IoLookupObjectByHandle(
	__in HANDLE Handle
	);

PIO_OBJECT
IoLookupObjectByHandleEx(
	__in HANDLE Handle,
	__in HANDLE_TYPE Type
	);

VOID
IoLockObjectBucket(
	__in PIO_OBJECT Object
	);

VOID
IoUnlockObjectBucket(
	__in PIO_OBJECT Object
	);

VOID
IoReferenceObject(
	__in PIO_OBJECT Object
	);

VOID
IoUnreferenceObject(
	__in PIO_OBJECT Object
	);

struct _IO_IRP;

VOID
IoQueueThreadIrp(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ struct _IO_IRP *Irp
	);

VOID
IoDequeueThreadIrp(
	_In_ PBTR_THREAD_OBJECT Thread,
	_In_ struct _IO_IRP *Irp
	);

//
// File and General Device IO
//

typedef HANDLE 
(WINAPI *IoCreateFilePtr)(
  _In_     LPCWSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile
);

HANDLE WINAPI 
IoCreateFile(
	_In_     LPCWSTR               lpFileName,
	_In_     DWORD                 dwDesiredAccess,
	_In_     DWORD                 dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_     DWORD                 dwCreationDisposition,
	_In_     DWORD                 dwFlagsAndAttributes,
	_In_opt_ HANDLE                hTemplateFile
	);

typedef BOOL 
(WINAPI *IoDuplicateHandlePtr)(
	_In_  HANDLE   hSourceProcessHandle,
	_In_  HANDLE   hSourceHandle,
	_In_  HANDLE   hTargetProcessHandle,
	_Out_ LPHANDLE lpTargetHandle,
	_In_  DWORD    dwDesiredAccess,
	_In_  BOOL     bInheritHandle,
	_In_  DWORD    dwOptions
	);

BOOL WINAPI 
IoDuplicateHandle(
	_In_  HANDLE   hSourceProcessHandle,
	_In_  HANDLE   hSourceHandle,
	_In_  HANDLE   hTargetProcessHandle,
	_Out_ LPHANDLE lpTargetHandle,
	_In_  DWORD    dwDesiredAccess,
	_In_  BOOL     bInheritHandle,
	_In_  DWORD    dwOptions
	);

typedef BOOL 
(WINAPI *IoCloseHandlePtr)(
    __in HANDLE Handle
    );

BOOL WINAPI
IoCloseHandle(
    __in HANDLE Handle
    );

typedef BOOL 
(WINAPI *SetFileCompletionNotificationModesPtr)(
	_In_ HANDLE FileHandle,
	_In_ UCHAR  Flags
	);

BOOL WINAPI 
IoSetFileCompletionNotificationModes(
	_In_ HANDLE FileHandle,
	_In_ UCHAR  Flags
	);

typedef BOOL 
(WINAPI *IoReadFileExPtr)(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToRead,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

BOOL WINAPI IoReadFileEx(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToRead,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef BOOL
(WINAPI *IoReadFilePtr)(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
);

BOOL WINAPI IoReadFile(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
);

typedef BOOL 
(WINAPI *IoWriteFileExPtr)(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToWrite,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

BOOL WINAPI IoWriteFileEx(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToWrite,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef BOOL
(WINAPI *IoWriteFilePtr)(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToWrite,
  _Out_opt_   LPDWORD      lpNumberOfBytesWrite,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
);

BOOL WINAPI IoWriteFile(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToWrite,
  _Out_opt_   LPDWORD      lpNumberOfBytesWrite,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
);

BOOL WINAPI 
IoGetOverlappedResult(
  _In_  HANDLE       hFile,
  _In_  LPOVERLAPPED lpOverlapped,
  _Out_ LPDWORD      lpNumberOfBytesTransferred,
  _In_  BOOL         bWait
);

typedef BOOL
(WINAPI *GetOverlappedResultPtr)(
  _In_  HANDLE       hFile,
  _In_  LPOVERLAPPED lpOverlapped,
  _Out_ LPDWORD      lpNumberOfBytesTransferred,
  _In_  BOOL         bWait
);

typedef HANDLE 
(WINAPI *CreateIoCompletionPortPtr)(
  _In_     HANDLE    FileHandle,
  _In_opt_ HANDLE    ExistingCompletionPort,
  _In_     ULONG_PTR CompletionKey,
  _In_     DWORD     NumberOfConcurrentThreads
);

HANDLE WINAPI IoCreateIoCompletionPort(
  _In_     HANDLE    FileHandle,
  _In_opt_ HANDLE    ExistingCompletionPort,
  _In_     ULONG_PTR CompletionKey,
  _In_     DWORD     NumberOfConcurrentThreads
);

BOOL WINAPI 
IoGetQueuedCompletionStatus(
  _In_  HANDLE       CompletionPort,
  _Out_ LPDWORD      lpNumberOfBytes,
  _Out_ PULONG_PTR   lpCompletionKey,
  _Out_ LPOVERLAPPED *lpOverlapped,
  _In_  DWORD        dwMilliseconds
);

typedef BOOL 
(WINAPI *GetQueuedCompletionStatusPtr)(
  _In_  HANDLE       CompletionPort,
  _Out_ LPDWORD      lpNumberOfBytes,
  _Out_ PULONG_PTR   lpCompletionKey,
  _Out_ LPOVERLAPPED *lpOverlapped,
  _In_  DWORD        dwMilliseconds
);

typedef BOOL 
(WINAPI *PostQueuedCompletionStatusPtr)(
	_In_     HANDLE       CompletionPort,
	_In_     DWORD        dwNumberOfBytesTransferred,
	_In_     ULONG_PTR    dwCompletionKey,
	_In_opt_ LPOVERLAPPED lpOverlapped
	);

BOOL WINAPI 
IoPostQueuedCompletionStatus(
	_In_     HANDLE       CompletionPort,
	_In_     DWORD        dwNumberOfBytesTransferred,
	_In_     ULONG_PTR    dwCompletionKey,
	_In_opt_ LPOVERLAPPED lpOverlapped
	);

BOOL WINAPI 
IoGetQueuedCompletionStatusEx(
  _In_  HANDLE             CompletionPort,
  _Out_ LPOVERLAPPED_ENTRY lpCompletionPortEntries,
  _In_  ULONG              ulCount,
  _Out_ PULONG             ulNumEntriesRemoved,
  _In_  DWORD              dwMilliseconds,
  _In_  BOOL               fAlertable
);

typedef BOOL
(WINAPI *GetQueuedCompletionStatusExPtr)(
  _In_  HANDLE             CompletionPort,
  _Out_ LPOVERLAPPED_ENTRY lpCompletionPortEntries,
  _In_  ULONG              ulCount,
  _Out_ PULONG             ulNumEntriesRemoved,
  _In_  DWORD              dwMilliseconds,
  _In_  BOOL               fAlertable
);

//
// Socket IO
//

int WINAPI 
IoClosesocket(
  _In_ SOCKET s
);

typedef int 
(WINAPI *ClosesocketPtr)(
  _In_ SOCKET s
);

typedef int 
(WINAPI *RecvPtr)(
  _In_  SOCKET s,
  _Out_ char   *buf,
  _In_  int    len,
  _In_  int    flags
);

int WINAPI 
IoRecv(
  _In_  SOCKET s,
  _Out_ char   *buf,
  _In_  int    len,
  _In_  int    flags
);

typedef int 
(WINAPI *RecvfromPtr)(
  _In_        SOCKET          s,
  _Out_       char            *buf,
  _In_        int             len,
  _In_        int             flags,
  _Out_       struct sockaddr *from,
  _Inout_opt_ int             *fromlen
);

int WINAPI IoRecvfrom(
  _In_        SOCKET          s,
  _Out_       char            *buf,
  _In_        int             len,
  _In_        int             flags,
  _Out_       struct sockaddr *from,
  _Inout_opt_ int             *fromlen
);

typedef int 
(WINAPI *SendPtr)(
  _In_       SOCKET s,
  _In_ const char   *buf,
  _In_       int    len,
  _In_       int    flags
);

int WINAPI IoSend(
  _In_       SOCKET s,
  _In_ const char   *buf,
  _In_       int    len,
  _In_       int    flags
);

typedef int 
(WINAPI *SendtoPtr)(
  _In_       SOCKET                s,
  _In_ const char                  *buf,
  _In_       int                   len,
  _In_       int                   flags,
  _In_       const struct sockaddr *to,
  _In_       int                   tolen
);

int WINAPI IoSendto(
  _In_       SOCKET                s,
  _In_ const char                  *buf,
  _In_       int                   len,
  _In_       int                   flags,
  _In_       const struct sockaddr *to,
  _In_       int                   tolen
);

typedef int 
(WINAPI *WSARecvPtr)(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI IoWSARecv(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef int 
(WINAPI *WSARecvFromPtr)(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _Out_   struct sockaddr                    *lpFrom,
  _Inout_ LPINT                              lpFromlen,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI IoWSARecvFrom(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _Out_   struct sockaddr                    *lpFrom,
  _Inout_ LPINT                              lpFromlen,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef int
(WINAPI *WSASendPtr)(
  _In_  SOCKET                             s,
  _In_  LPWSABUF                           lpBuffers,
  _In_  DWORD                              dwBufferCount,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  DWORD                              dwFlags,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI IoWSASend(
  _In_  SOCKET                             s,
  _In_  LPWSABUF                           lpBuffers,
  _In_  DWORD                              dwBufferCount,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  DWORD                              dwFlags,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef int 
(WINAPI *WSASendToPtr)(
  _In_  SOCKET                             s,
  _In_  LPWSABUF                           lpBuffers,
  _In_  DWORD                              dwBufferCount,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  DWORD                              dwFlags,
  _In_  const struct sockaddr              *lpTo,
  _In_  int                                iToLen,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI IoWSASendTo(
  _In_  SOCKET                             s,
  _In_  LPWSABUF                           lpBuffers,
  _In_  DWORD                              dwBufferCount,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  DWORD                              dwFlags,
  _In_  const struct sockaddr              *lpTo,
  _In_  int                                iToLen,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);


SOCKET WINAPI
IoSocket(
	_In_ int af,
	_In_ int type,
	_In_ int protocal
	);

typedef SOCKET 
(WINAPI *SocketPtr)(
	_In_ int af,
	_In_ int type,
	_In_ int protocal
	);

SOCKET WINAPI
IoWSASocket(
	_In_ int af,
	_In_ int type,
	_In_ int protocol,
	_In_ LPWSAPROTOCOL_INFO lpProtocolInfo,
	_In_ GROUP g,
	_In_ DWORD dwFlags
	);

typedef SOCKET 
(WINAPI *WSASocketPtr)(
	_In_ int af,
	_In_ int type,
	_In_ int protocol,
	_In_ LPWSAPROTOCOL_INFO lpProtocolInfo,
	_In_ GROUP g,
	_In_ DWORD dwFlags
	);

int WINAPI
IoBind(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen
	);

typedef int 
(WINAPI *BindPtr)(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen
	);

SOCKET WINAPI
IoAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ int* addrlen
	);

typedef SOCKET 
(WINAPI *AcceptPtr)(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ int* addrlen
	);

int WINAPI
IoConnect(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen
	);

typedef int 
(WINAPI *ConnectPtr)(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen
	);

int WINAPI
IoCtlSocket(
	_In_  SOCKET s,
	_In_  long cmd,
	_Out_ u_long* argp
	);

typedef int 
(WINAPI *IoCtlSocketPtr)(
	_In_  SOCKET s,
	_In_  long cmd,
	_Out_ u_long* argp
	);

int WINAPI
IoWSAConnect(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen,
	_In_ LPWSABUF lpCallerData,
	_Out_ LPWSABUF lpCalleeData,
	_In_  LPQOS lpSQOS,
	_In_  LPQOS lpGQOS
	);

typedef int 
(WINAPI *WSAConnectPtr)(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen,
	_In_ LPWSABUF lpCallerData,
	_Out_ LPWSABUF lpCalleeData,
	_In_  LPQOS lpSQOS,
	_In_  LPQOS lpGQOS
	);


SOCKET WINAPI
IoWSAAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ LPINT addrlen,
	_In_  LPCONDITIONPROC lpfnCondition,
	_In_  DWORD dwCallbackData
	);

typedef SOCKET 
(WINAPI *WSAAcceptPtr)(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ LPINT addrlen,
	_In_  LPCONDITIONPROC lpfnCondition,
	_In_  DWORD dwCallbackData
	);

int WINAPI 
IoWSAEventSelect(
	_In_ SOCKET s,
	_In_ WSAEVENT hEventObject,	
	_In_ long lNetworkEvents
	);

typedef int 
(WINAPI *WSAEventSelectPtr)(
	_In_ SOCKET s,
	_In_ WSAEVENT hEventObject,	
	_In_ long lNetworkEvents
	);

int WINAPI
IoWSAAsyncSelect(
	_In_ SOCKET s,
	_In_ HWND hWnd,
	_In_ unsigned int wMsg,
	_In_ long lEvent
	);

typedef int 
(WINAPI *WSAAsyncSelectPtr)(
	_In_ SOCKET s,
	_In_ HWND hWnd,
	_In_ unsigned int wMsg,
	_In_ long lEvent
	);

int WINAPI
IoWSAIoctl(
	_In_  SOCKET s,
	_In_  DWORD dwIoControlCode,
	_In_  LPVOID lpvInBuffer,
	_In_  DWORD cbInBuffer,
	_Out_ LPVOID lpvOutBuffer,
	_In_  DWORD cbOutBuffer,
	_Out_ LPDWORD lpcbBytesReturned,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

typedef int 
(WINAPI *WSAIoctlPtr)(
	_In_  SOCKET s,
	_In_  DWORD dwIoControlCode,
	_In_  LPVOID lpvInBuffer,
	_In_  DWORD cbInBuffer,
	_Out_ LPVOID lpvOutBuffer,
	_In_  DWORD cbOutBuffer,
	_Out_ LPDWORD lpcbBytesReturned,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

typedef BOOL 
(WINAPI *WSAGetOverlappedResultPtr)(
  _In_  SOCKET          s,
  _In_  LPWSAOVERLAPPED lpOverlapped,
  _Out_ LPDWORD         lpcbTransfer,
  _In_  BOOL            fWait,
  _Out_ LPDWORD         lpdwFlags
);

BOOL WINAPI 
IoWSAGetOverlappedResult(
  _In_  SOCKET          s,
  _In_  LPWSAOVERLAPPED lpOverlapped,
  _Out_ LPDWORD         lpcbTransfer,
  _In_  BOOL            fWait,
  _Out_ LPDWORD         lpdwFlags
);

//
// N.B. Windows thread pool can bind to IO completion
// port, we need handle this case, e.g. IE11 use this
// technique.
//

typedef PTP_IO
(WINAPI *CreateThreadpoolIoPtr)(
  _In_        HANDLE                fl,
  _In_        PTP_WIN32_IO_CALLBACK pfnio,
  _Inout_opt_ PVOID                 pv,
  _In_opt_    PTP_CALLBACK_ENVIRON  pcbe
);

PTP_IO WINAPI IoCreateThreadpoolIo(
  _In_        HANDLE                fl,
  _In_        PTP_WIN32_IO_CALLBACK pfnio,
  _Inout_opt_ PVOID                 pv,
  _In_opt_    PTP_CALLBACK_ENVIRON  pcbe
);

typedef VOID 
(WINAPI *StartThreadpoolIoPtr)(
  _Inout_ PTP_IO pio
);

VOID WINAPI IoStartThreadpoolIo(
  _Inout_ PTP_IO pio
);

typedef VOID 
(WINAPI *CancelThreadpoolIoPtr)(
  _Inout_ PTP_IO pio
);

VOID WINAPI IoCancelThreadpoolIo(
  _Inout_ PTP_IO pio
);

typedef VOID
(WINAPI *CloseThreadpoolIoPtr)(
  _Inout_ PTP_IO pio
);

VOID WINAPI IoCloseThreadpoolIo(
  _Inout_ PTP_IO pio
);

//
// N.B. WSAIoctl to get the pointers to AcceptEx,
// TransmitFile, TransmitPackets.
//

BOOL WINAPI IoAcceptEx(
  _In_  SOCKET       sListenSocket,
  _In_  SOCKET       sAcceptSocket,
  _In_  PVOID        lpOutputBuffer,
  _In_  DWORD        dwReceiveDataLength,
  _In_  DWORD        dwLocalAddressLength,
  _In_  DWORD        dwRemoteAddressLength,
  _Out_ LPDWORD      lpdwBytesReceived,
  _In_  LPOVERLAPPED lpOverlapped
);

typedef BOOL 
(WINAPI *AcceptExPtr)(
  _In_  SOCKET       sListenSocket,
  _In_  SOCKET       sAcceptSocket,
  _In_  PVOID        lpOutputBuffer,
  _In_  DWORD        dwReceiveDataLength,
  _In_  DWORD        dwLocalAddressLength,
  _In_  DWORD        dwRemoteAddressLength,
  _Out_ LPDWORD      lpdwBytesReceived,
  _In_  LPOVERLAPPED lpOverlapped
);

typedef BOOL 
(WINAPI *TransmitFilePtr)(
   SOCKET                  hSocket,
   HANDLE                  hFile,
   DWORD                   nNumberOfBytesToWrite,
   DWORD                   nNumberOfBytesPerSend,
   LPOVERLAPPED            lpOverlapped,
   LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
   DWORD                   dwFlags
);

BOOL WINAPI 
IoTransmitFile(
   SOCKET                  hSocket,
   HANDLE                  hFile,
   DWORD                   nNumberOfBytesToWrite,
   DWORD                   nNumberOfBytesPerSend,
   LPOVERLAPPED            lpOverlapped,
   LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
   DWORD                   dwFlags
);

BOOL WINAPI 
IoTransmitPackets(
   SOCKET                     hSocket,
   LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,
   DWORD                      nElementCount,
   DWORD                      nSendSize,
   LPOVERLAPPED               lpOverlapped,
   DWORD                      dwFlags
);

typedef BOOL 
(WINAPI *TransmitPacketsPtr)(
   SOCKET                     hSocket,
   LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,
   DWORD                      nElementCount,
   DWORD                      nSendSize,
   LPOVERLAPPED               lpOverlapped,
   DWORD                      dwFlags
);

BOOL WINAPI
IoConnectEx(
  _In_     SOCKET                s,
  _In_     const struct sockaddr *name,
  _In_     int                   namelen,
  _In_opt_ PVOID                 lpSendBuffer,
  _In_     DWORD                 dwSendDataLength,
  _Out_    LPDWORD               lpdwBytesSent,
  _In_     LPOVERLAPPED          lpOverlapped
);

typedef BOOL 
(WINAPI *ConnectExPtr)(
  _In_     SOCKET                s,
  _In_     const struct sockaddr *name,
  _In_     int                   namelen,
  _In_opt_ PVOID                 lpSendBuffer,
  _In_     DWORD                 dwSendDataLength,
  _Out_    LPDWORD               lpdwBytesSent,
  _In_     LPOVERLAPPED          lpOverlapped
);

BOOL WINAPI 
IoDisconnectEx(
  _In_ SOCKET       hSocket,
  _In_ LPOVERLAPPED lpOverlapped,
  _In_ DWORD        dwFlags,
  _In_ DWORD        reserved
);

typedef BOOL 
(WINAPI *DisconnectExPtr)(
  _In_ SOCKET       hSocket,
  _In_ LPOVERLAPPED lpOverlapped,
  _In_ DWORD        dwFlags,
  _In_ DWORD        reserved
);

typedef void 
(WINAPI *GetAcceptExSockaddrsPtr)(
  _In_  PVOID      lpOutputBuffer,
  _In_  DWORD      dwReceiveDataLength,
  _In_  DWORD      dwLocalAddressLength,
  _In_  DWORD      dwRemoteAddressLength,
  _Out_ LPSOCKADDR *LocalSockaddr,
  _Out_ LPINT      LocalSockaddrLength,
  _Out_ LPSOCKADDR *RemoteSockaddr,
  _Out_ LPINT      RemoteSockaddrLength
);

int WINAPI 
IoWSARecvMsg(
  _In_    SOCKET                             s,
  _Inout_ LPWSAMSG                           lpMsg,
  _Out_   LPDWORD                            lpdwNumberOfBytesRecvd,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef int 
(WINAPI *WSARecvMsgPtr)(
  _In_    SOCKET                             s,
  _Inout_ LPWSAMSG                           lpMsg,
  _Out_   LPDWORD                            lpdwNumberOfBytesRecvd,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

int WINAPI 
IoWSASendMsg(
  _In_  SOCKET                             s,
  _In_  LPWSAMSG                           lpMsg,
  _In_  DWORD                              dwFlags,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef int 
(WINAPI *WSASendMsgPtr)(
  _In_  SOCKET                             s,
  _In_  LPWSAMSG                           lpMsg,
  _In_  DWORD                              dwFlags,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

ULONG
IoGetFunctionPointers(
	VOID
	);

//
// ExitProcess guard 
//

VOID WINAPI 
IoExitProcessCallback(
	__in UINT ExitCode
	);

extern LIST_ENTRY IoTpContextList;
extern BTR_SPINLOCK IoTpContextLock;

//
// Internal debug routines
//

VOID
IoDebugPrintSkAddress(
	_In_ PIO_OBJECT Object
	);

#ifdef __cplusplus
}
#endif
#endif