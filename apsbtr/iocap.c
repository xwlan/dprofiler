//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "callback.h"
#include "heap.h"
#include "lock.h"
#include "thread.h"
#include "hal.h"
#include "stacktrace.h"
#include "util.h"
#include "ioprof.h"

#pragma comment(lib, "ws2_32.lib")

//
// Socket function pointer entries
//

typedef struct _IO_FUNCTION_POINTER_ENTRY {
	PCWSTR Name;
	GUID Guid;
	PVOID Pointer;
} IO_FUNCTION_POINTER_ENTRY, *PIO_FUNCTION_POINTER_ENTRY;

typedef enum _IO_FUNCTION_POINTER_ORDINAL {
	_IoFpAcceptEx,
	_IoFpConnectEx,
	_IoFpDisonnectEx,
	_IoFpGetAcceptExSockAddrs,
	_IoFpTransmitFile,
	_IoFpTransmitPackets,
	_IoFpWSARecvMsg,
	_IoFpWSASendMsg,
} IO_FUNCTION_POINTER_ORDINAL;

IO_FUNCTION_POINTER_ENTRY IoFunctionPointerTable[] = {
	{ L"AcceptedEx", WSAID_ACCEPTEX, NULL },
	{ L"ConnectEx",  WSAID_CONNECTEX, NULL },
	{ L"DisconnectEx",  WSAID_DISCONNECTEX, NULL },
	{ L"GetAcceptExSockAddrs", WSAID_GETACCEPTEXSOCKADDRS, NULL },
	{ L"TransmitFile", WSAID_TRANSMITFILE, NULL },
	{ L"TransmitPackets", WSAID_TRANSMITPACKETS, NULL },
	{ L"WSARecvMsg", WSAID_WSARECVMSG, NULL },
	{ L"WSASendMsg", WSAID_WSASENDMSG, NULL },
};

ULONG IoFunctionPointerCount = ARRAYSIZE(IoFunctionPointerTable);

//
// N.B. Use IoGetAcceptExSockaddrs to query accepted socket addresses
//

GetAcceptExSockaddrsPtr IoGetAcceptExSockaddrs;


BTR_CALLBACK IoCallback[] = {
		
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoExitProcessCallback, "kernel32.dll", "ExitProcess", 0 },

	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoCreateFile, "kernel32.dll", "CreateFileW", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoDuplicateHandle, "kernel32.dll", "DuplicateHandle", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoCloseHandle, "kernel32.dll", "CloseHandle", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoReadFile, "kernel32.dll", "ReadFile", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWriteFile, "kernel32.dll", "WriteFile", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoReadFileEx, "kernel32.dll", "ReadFileEx", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWriteFileEx, "kernel32.dll", "WriteFileEx", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoGetOverlappedResult, "kernel32.dll", "GetOverlappedResult", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoGetQueuedCompletionStatus, "kernel32.dll", "GetQueuedCompletionStatus", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoGetQueuedCompletionStatusEx, "kernel32.dll", "GetQueuedCompletionStatusEx", 0 },  // NT 6+
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoPostQueuedCompletionStatus, "kernel32.dll", "PostQueuedCompletionStatus", 0 },  

	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoSocket, "ws2_32.dll", "socket", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoConnect, "ws2_32.dll", "connect", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoAccept, "ws2_32.dll", "accept", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoCtlSocket, "ws2_32.dll", "ioctlsocket", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoClosesocket, "ws2_32.dll", "closesocket", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSASocket, "ws2_32.dll", "WSASocketW", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSAConnect, "ws2_32.dll", "WSAConnect", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSAAccept, "ws2_32.dll", "WSAAccept", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSAIoctl, "ws2_32.dll", "WSAIoctl", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSAEventSelect, "ws2_32.dll", "WSAEventSelect", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSAAsyncSelect, "ws2_32.dll", "WSAAsyncSelect", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSAGetOverlappedResult, "ws2_32.dll", "WSAGetOverlappedResult", 0 },

	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoRecv, "ws2_32.dll", "recv", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoSend, "ws2_32.dll", "send", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoRecvfrom, "ws2_32.dll", "recvfrom", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoSendto, "ws2_32.dll", "sendto", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSARecv, "ws2_32.dll", "WSARecv", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSASend, "ws2_32.dll", "WSASend", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSARecvFrom, "ws2_32.dll", "WSARecvFrom", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSASendTo, "ws2_32.dll", "WSASendTo", 0 },

	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoCreateThreadpoolIo, "kernel32.dll", "CreateThreadpoolIo", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoCreateIoCompletionPort, "kernel32.dll", "CreateIoCompletionPort", 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoSetFileCompletionNotificationModes, "kernel32.dll", "SetFileCompletionNotificationModes", 0 },
	
	//
	// N.B. The followings are dynamically retrieved function pointers from mswsock.dll, 
	// require speical process in QueryHotPatch callback
	//

	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoAcceptEx, NULL, NULL, 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoConnectEx, NULL, NULL, 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoTransmitFile, NULL, NULL, 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoTransmitPackets, NULL, NULL, 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSARecvMsg, NULL, NULL, 0 },
	{  IoCallbackType, CALLBACK_OFF,0,0,0, IoWSASendMsg, NULL, NULL, 0 },
};

ULONG IoCallbackCount = ARRAYSIZE(IoCallback);

PBTR_CALLBACK FORCEINLINE
IoGetCallback(
	IN ULONG Ordinal
	)
{
	return &IoCallback[Ordinal];
}

//
// TP_IO context list head
//

LIST_ENTRY IoTpContextList;
BTR_SPINLOCK IoTpContextLock;

volatile ULONG IoRequestId = (ULONG)-1;
volatile ULONG IoObjectId = (ULONG)-1;

//
// Mark irp as synchronous
//

#define IoMarkIrpSynchronous(_I) \
	_I->Flags.Synchronous = 1;

//
// Is irp a synchronous I/O
//

#define IoIsIrpSynchronous(_I) \
	(_I->Flags.Synchronous != 0)

//
// _T, Thread object
// _C, Callback
// _I, Irp
//

#define IoCaptureStackTrace(_T, _C, _I)\
{\
	ULONG Depth;\
	ULONG_PTR *Pc;\
	Pc = (ULONG_PTR *)_T->Buffer;\
	Pc[0] = (ULONG_PTR)CALLER;\
	Pc[1] = (ULONG_PTR)_I->RequestBytes;\
	Pc[2] = 0;\
	BtrCaptureStackTraceEx((PVOID *)_T->Buffer,\
							MAX_STACK_DEPTH, BtrGetFramePointer(),\
		                   _C->Address, &_I->StackId, &Depth);\
}

#define IO_GET_SOCKET_OBJECT(_S, _O)								\
{																	\
	*_O = IoLookupObjectByHandleEx(SK_HANDLE(_S), HANDLE_SOCKET);\
	if (!(*_O)) {													\
		*_O = IoAllocateSocketObject(SK_HANDLE(_S));				\
		if (*_O) {													\
			IoReferenceObject(Object);								\
		}															\
	}																\
}

#define IO_GET_FILE_OBJECT(_H, _O)						\
{														\
	*_O = IoLookupObjectByHandleEx(_H, HANDLE_FILE);	\
	if (!(*_O)) {										\
		*_O = IoAllocateSocketObject(_H));			\
		if (*_O)) {										\
			IoReferenceObject(Object);					\
		}												\
	}													\
}


ULONG
IoAcquireObjectId(
	VOID
	)
{
	ULONG Id;
	Id = _InterlockedIncrement(&IoObjectId);
	return Id;
}

ULONG
IoAcquireRequestId(
	VOID
	)
{
	ULONG Id;
	Id = _InterlockedIncrement(&IoRequestId);
	return Id;
}

PBTR_USER_APC
IoDequeueApcByOverlapped(
	__in PLIST_ENTRY ApcList,
	__in LPOVERLAPPED lpOverlapped
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_USER_APC Apc;

	ListEntry = ApcList->Flink;
	while (ListEntry != ApcList) {
		Apc = CONTAINING_RECORD(ListEntry, BTR_USER_APC, ListEntry);
		if ((LPOVERLAPPED)Apc->Context == lpOverlapped) {
			RemoveEntryList(ListEntry);
			return Apc;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

PIO_IRP
IoAllocateIrp(
	_In_ PIO_OBJECT Object	
	)
{
	PIO_IRP Irp;

	Irp = (PIO_IRP)BtrMallocLookaside(LOOKASIDE_IO_IRP);

	ASSERT((ULONG_PTR)&Irp->PendingSListEntry % MEMORY_ALLOCATION_ALIGNMENT == 0);
	ASSERT((ULONG_PTR)&Irp->CompleteSListEntry % MEMORY_ALLOCATION_ALIGNMENT == 0);

	//
	// Sanity check that there's no problem in list manipulation with
	// lookaside and irp flush
	//

	ASSERT(Irp->InCall != 1);

	RtlZeroMemory(Irp, sizeof(IO_IRP));

	Irp->IrpTag = IO_IRP_TAG;
	Irp->RequestId = IoAcquireRequestId();
	Irp->RequestThreadId = GetCurrentThreadId();
	Irp->ObjectId = Object->Id;
	Irp->Object = Object->Object;
	Irp->InCall = 1;

	if (Object->Type == HANDLE_FILE) {
		Irp->Flags.File = 1;
	}
	else if (Object->Type == HANDLE_SOCKET) {
		Irp->Flags.Socket = 1;
	}
	else {
		ASSERT(0);
	}

	//
	// Queue the irp into pending list
	//

	InterlockedPushEntrySList(&IoIrpPendingSListHead, &Irp->PendingSListEntry);
	return Irp;
}

VOID
IoFreeIrp(
	_In_ PIO_IRP Irp
	)
{
	if (Irp->Flags.Socket)
		DebugTrace("FREE IRP: ptr=%p, RID=%d, SOCKET=%p, OP=%d", Irp, Irp->RequestId, Irp->Object, Irp->Operation);
	if (Irp->Flags.File)
		DebugTrace("FREE IRP: ptr=%p, RID=%d, FILE=%p, OP=%d", Irp, Irp->RequestId, Irp->Object, Irp->Operation);

	BtrFreeLookaside(LOOKASIDE_IO_IRP, Irp);
}

PIO_IRP_TRACK
IoAllocateIrpTrack(
	_In_ PIO_IRP Irp
	)
{
	PIO_IRP_TRACK Track;
	Track = (PIO_IRP_TRACK)BtrMallocLookaside(LOOKASIDE_IO_IRP_TRACK);
	Track->Irp = Irp;
	Track->IoStatus = 0;
	return Track;
}

VOID
IoFreeIrpTrack(
	_In_ PIO_IRP_TRACK Track
	)
{
	BtrFreeLookaside(LOOKASIDE_IO_IRP_TRACK, Track);
}

VOID
IoIrpClearInCall(
	_In_ PIO_IRP Irp
	)
{
	InterlockedCompareExchange(&Irp->InCall, 0, 1);
}

BOOLEAN
IoIsIrpInCall(
	_In_ PIO_IRP Irp
	)
{
	return (BOOLEAN)InterlockedCompareExchange(&Irp->InCall, 1, 1);
}

BOOLEAN
IoCopyOverlapped(
	_In_ LPOVERLAPPED From,
	_Inout_ LPOVERLAPPED To
	)
{
	if (!From || !To)
		return FALSE;

	__try {
		RtlCopyMemory(To, From, sizeof(*From));
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return FALSE;
	}

	return TRUE;
}

//
// N.B. This routine is for GetQueuedCompletionStatus/Ex,
// IO completion routines, the system will pass us the lpOverlapped
// pointer we replace when issue IO request.
//

PIO_IRP
IoOverlappedToIrp(
	_In_ LPOVERLAPPED Overlapped
	)
{
	PIO_IRP Irp;

	__try {
		Irp = CONTAINING_RECORD(Overlapped, IO_IRP, Overlapped);	
		if (Irp->IrpTag == IO_IRP_TAG) {
			return Irp;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}
	return NULL;
}

//
// N.B. This routine is only for GetOverlappedResult,
// WSAGetOverlappedResult, for IO completion routines
// we can always get a legal lpOverlapped. To avoid
// access violation overhead, use CAN_BE_WSA_STATUS()
// to filter out those overlapped not hijacked by us,
// because our irp is allocated from private heap, its
// address range must not be in WSABASEERR * 2, it's
// 10000 - 20000, a small number.
//

#define CAN_BE_WSA_STATUS(_I) \
	((ULONG_PTR)_I < (ULONG_PTR)WSABASEERR * 2)

#define CAN_NOT_BE_POINTER(_I) \
	((LONG_PTR)_I < 0)

#define CAN_BE_NTSTATUS(_I)\
	(((ULONG)_I & 0xC0000000) == 0)

PIO_IRP
IoGetIrpFromInternal(
	_In_ LPOVERLAPPED Overlapped
	)
{
	PIO_IRP Irp;

	__try {
		Irp = (PIO_IRP)Overlapped->Internal;
		if (Irp != NULL) {
			if (CAN_BE_WSA_STATUS(Irp) || CAN_NOT_BE_POINTER(Irp) || CAN_BE_NTSTATUS(Irp)) {
				return NULL;
			}
			if (Irp->IrpTag == IO_IRP_TAG) {
				return Irp;
			}
		}
		return NULL;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return NULL;
	}
}

LPOVERLAPPED
IoHijackOverlapped(
	_In_ PIO_IRP Irp,
	_In_ LPOVERLAPPED lpOverlapped
	)
{
	RtlCopyMemory(&Irp->Overlapped, lpOverlapped, sizeof(OVERLAPPED));
	Irp->Original = lpOverlapped;
	lpOverlapped->Internal = (ULONG_PTR)Irp;
	return &Irp->Overlapped;
}

BOOLEAN
IoCheckOverlapped(
	_In_ HANDLE Handle,
	_In_ HANDLE_TYPE Type,
	_In_ LPOVERLAPPED lpOverlapped
	)
{
	OVERLAPPED Copy;

	UNREFERENCED_PARAMETER(Handle);
	UNREFERENCED_PARAMETER(Type);

	//
	// N.B. This indicate a synchronous I/O, overlapped I/O 
	// must provide a valid lpOverlapped, even it use a completion
	// routine.
	//

	if (!lpOverlapped) {
		return FALSE;
	}

	//
	// an overlapped object with readable, writable lpOverlapped
	// can do asynchronous I/O.
	//

	if (!IoCopyOverlapped(lpOverlapped, &Copy)) {
		return FALSE;
	}
	
	if (!IoCopyOverlapped(&Copy, lpOverlapped)) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
IoRefObjectCheckOverlapped(
	_In_ HANDLE Handle,
	_In_ HANDLE_TYPE Type,
	_In_ LPOVERLAPPED lpOverlapped,
	_Out_ PIO_OBJECT *Object,
	_Out_ BOOLEAN *IsOverlapped,
	_Out_ BOOLEAN *SkipOnSuccess
	)
{
	PIO_OBJECT IoObject;
	BOOLEAN Overlapped;
	OVERLAPPED Copy;

	*Object = NULL;
	*IsOverlapped = FALSE;
	*SkipOnSuccess = FALSE;
	Overlapped = FALSE;

	IoObject = IoLookupObjectByHandleEx(Handle, Type);
	if (!IoObject) {
		return FALSE;
	}

	//
	// N.B. This indicate a synchronous I/O, overlapped I/O 
	// must provide a valid lpOverlapped, even it use a completion
	// routine.
	//

	if (!lpOverlapped) {
		*Object = IoObject;
		return TRUE;
	}

	//
	// an overlapped object with readable, writable lpOverlapped
	// can do asynchronous I/O.
	//

	if (IoIsObjectOverlapped(IoObject)) {
		if (lpOverlapped) {
			if (!IoCopyOverlapped(lpOverlapped, &Copy)) {
				Overlapped = FALSE;
			}
			else if (!IoCopyOverlapped(&Copy, lpOverlapped)) {
				Overlapped = FALSE;
			}
			Overlapped = TRUE;
		} else {
			Overlapped = FALSE;
		}
	}

	*Object = IoObject;
	*IsOverlapped = Overlapped;

	if (FlagOn(IoObject->Flags, OF_IOCPASSOCIATE) && FlagOn(IoObject->Flags, OF_SKIPONSUCCESS)){
		*SkipOnSuccess = TRUE;
	}
	return TRUE;
}

VOID
IoQuerySocketAddress(
	_In_ PIO_OBJECT Object,
	_In_ SOCKET s
	)
{
	int Length;

	ASSERT(Object->Type == HANDLE_SOCKET);

	Length = sizeof(SOCKADDR_STORAGE);
	if (!FlagOn(Object->Flags, OF_LOCAL_VALID)) {
		if (!getsockname(s, (SOCKADDR *)&Object->u.Socket.Local, &Length)){
			SetFlag(Object->Flags, OF_LOCAL_VALID);
		}
	}
	if (!FlagOn(Object->Flags, OF_REMOTE_VALID)) {
		if (!getpeername(s, (SOCKADDR *)&Object->u.Socket.Remote, &Length)){
			SetFlag(Object->Flags, OF_REMOTE_VALID);
		}
	}

#ifdef _DEBUG
	IoDebugPrintSkAddress(Object);
#endif
}

VOID CALLBACK 
IoFileCompleteCallback(
	_In_    DWORD        dwErrorCode,
	_In_    DWORD        dwNumberOfBytesTransfered,
	_Inout_ LPOVERLAPPED lpOverlapped
	)
{
	PIO_IRP Irp;
	LPOVERLAPPED Original;
	LPOVERLAPPED_COMPLETION_ROUTINE Callback;

	__try {
		
		Irp = IoOverlappedToIrp(lpOverlapped);
		Irp->IoStatus = dwErrorCode;
		Irp->CompleteBytes = dwNumberOfBytesTransfered;
		Irp->CompleteThreadId = GetCurrentThreadId();
		QueryPerformanceCounter(&Irp->End);

		//
		// Duplicate IO status block to user's lpOverlapped 
		//

		Original = Irp->Original;
		Callback = (LPOVERLAPPED_COMPLETION_ROUTINE)Irp->ApcCallback;
		RtlCopyMemory(&Irp->Overlapped, Original, sizeof(OVERLAPPED));

		//
		// Queue irp to flush list and call user's routine, note that
		// if user provide an invalid callback, it will crash.
		//

		IoQueueFlushList(Irp);
		(*Callback)(dwErrorCode, dwNumberOfBytesTransfered, Original);

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}

VOID CALLBACK 
IoNetCompleteCallback(
	_In_    DWORD        dwErrorCode,
	_In_    DWORD        dwNumberOfBytesTransfered,
	_Inout_ LPOVERLAPPED lpOverlapped,
	_In_ DWORD Flags
	)
{
	PIO_IRP Irp;
	LPOVERLAPPED Original;
	PIO_OBJECT Object;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE Callback;

	__try {
		
		Irp = IoOverlappedToIrp(lpOverlapped);
		Irp->IoStatus = dwErrorCode;
		Irp->CompleteBytes = dwNumberOfBytesTransfered;
		Irp->CompleteThreadId = GetCurrentThreadId();
		QueryPerformanceCounter(&Irp->End);

		//
		// Duplicate IO status block to user's lpOverlapped 
		//

		Original = Irp->Original;
		Callback = (LPWSAOVERLAPPED_COMPLETION_ROUTINE)Irp->ApcCallback;
		RtlCopyMemory(&Irp->Overlapped, Original, sizeof(OVERLAPPED));

		Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_SOCKET);
		if (Object) {
			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
			IoUnreferenceObject(Object);
		}

		if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
			Object = (PIO_OBJECT)Irp->ControlContext;
			if (dwErrorCode == ERROR_SUCCESS) {
				if (!Irp->ControlData) {
					IoClearObjectOverlapped(Object);
				} else {
					IoMarkObjectOverlapped(Object);
				}
			}
			IoUnreferenceObject(Object);
		}

		//
		// Queue irp to flush list and call user's routine
		//

		IoQueueFlushList(Irp);
		(*Callback)(dwErrorCode, dwNumberOfBytesTransfered, Original, Flags);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}

HANDLE WINAPI 
IoCreateFile(
	_In_     LPCWSTR               lpFileName,
	_In_     DWORD                 dwDesiredAccess,
	_In_     DWORD                 dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_     DWORD                 dwCreationDisposition,
	_In_     DWORD                 dwFlagsAndAttributes,
	_In_opt_ HANDLE                hTemplateFile
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoCreateFilePtr CallbackPtr;
	HANDLE File;
	PIO_OBJECT Object;
	ULONG Length;

	Callback = IoGetCallback(_IoCreateFile);
	CallbackPtr = (IoCreateFilePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		InterlockedIncrement(&Callback->References);
		File = (*CallbackPtr)(lpFileName, dwDesiredAccess,
							dwShareMode, lpSecurityAttributes,
							dwCreationDisposition, 
							dwFlagsAndAttributes,
							hTemplateFile
							);
		InterlockedDecrement(&Callback->References);
		return File;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	File = (*CallbackPtr)(lpFileName, dwDesiredAccess,
						dwShareMode, lpSecurityAttributes,
						dwCreationDisposition, 
						dwFlagsAndAttributes,
						hTemplateFile
						);

	if (File == INVALID_HANDLE_VALUE) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return INVALID_HANDLE_VALUE;
	}
	
	Object = IoAllocateObject();
	Object->Id = IoAcquireObjectId();
	Object->Object = File;
	Object->Type = HANDLE_FILE;
	Object->Flags = OF_FILE;

	if (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED) {
		SetFlag(Object->Flags, OF_OVERLAPPED);
	}
	
	GetSystemTimeAsFileTime(&Object->Start);

	//
	// Save file object name
	//

	Length = (ULONG)wcslen(lpFileName) + 1;
	Object->u.File.Name = (PWSTR)BtrMalloc(Length * sizeof(WCHAR));
	ZeroMemory(Object->u.File.Name, Length * sizeof(WCHAR));

	Object->u.File.Length = Length;  // include L'\0' 
	wcscpy_s(Object->u.File.Name, Length, lpFileName);
	IoInsertObject(Object);

	SetLastError(ERROR_SUCCESS);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return File;
}

BOOL WINAPI 
IoDuplicateHandle(
	_In_  HANDLE   hSourceProcessHandle,
	_In_  HANDLE   hSourceHandle,
	_In_  HANDLE   hTargetProcessHandle,
	_Out_ LPHANDLE lpTargetHandle,
	_In_  DWORD    dwDesiredAccess,
	_In_  BOOL     bInheritHandle,
	_In_  DWORD    dwOptions
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoDuplicateHandlePtr CallbackPtr;
	PIO_OBJECT Object;
	PIO_OBJECT Clone;
	ULONG Length;
	BOOL Status;
	DWORD CurrentPid;
	DWORD TargetPid;

	Callback = IoGetCallback(_IoDuplicateHandle);
	CallbackPtr = (IoDuplicateHandlePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hSourceProcessHandle,
					hSourceHandle,
					hTargetProcessHandle,
					lpTargetHandle,
					dwDesiredAccess,
					bInheritHandle,
					dwOptions
					);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hSourceProcessHandle,
		hSourceHandle,
		hTargetProcessHandle,
		lpTargetHandle,
		dwDesiredAccess,
		bInheritHandle,
		dwOptions
		);

	if (!Status) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	//
	// Check whether the target PID is current process, we don't care if
	// user is duplicating a handle into another process's address space
	//

	CurrentPid = GetCurrentProcessId();
	TargetPid = GetProcessId(hTargetProcessHandle);
	if (CurrentPid != TargetPid) {
		SetLastError(ERROR_SUCCESS);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	//
	// N.B. We only care about the handle of file type, socket can not be
	// duplicated by this way
	//

	Object = IoLookupObjectByHandleEx(hSourceHandle, HANDLE_FILE);
	if (!Object) {
		SetLastError(ERROR_SUCCESS);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	ASSERT(FlagOn(Object->Flags, OF_FILE));
	ASSERT(Object->Type == HANDLE_FILE);

	Clone = IoAllocateObject();
	Clone->Object = *lpTargetHandle;
	Clone->Id = IoAcquireObjectId();
	Clone->References = 0;
	Clone->Type = Object->Type;
	Clone->Flags = Object->Flags;

	Length = Object->u.File.Length;
	if (Object->u.File.Name != NULL) {
		Clone->u.File.Name = (PWSTR)BtrMalloc(Length * sizeof(WCHAR));
		Clone->u.File.Length = Length; 
		wcscpy_s(Clone->u.File.Name, Length, Object->u.File.Name);
	}

	GetSystemTimeAsFileTime(&Clone->Start);

	//
	// Unreference source object and insert duplicated object
	//

	IoUnreferenceObject(Object);
	IoInsertObject(Clone);

	SetLastError(ERROR_SUCCESS);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

HANDLE WINAPI 
IoCreateIoCompletionPort(
	_In_     HANDLE    FileHandle,
	_In_opt_ HANDLE    ExistingCompletionPort,
	_In_     ULONG_PTR CompletionKey,
	_In_     DWORD     NumberOfConcurrentThreads
	)
{
	HANDLE Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	CreateIoCompletionPortPtr CallbackPtr;
	PIO_OBJECT Object;

	Callback = IoGetCallback(_IoCreateIoCompletionPort);
	CallbackPtr = (CreateIoCompletionPortPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){ 
Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (FileHandle == INVALID_HANDLE_VALUE) {
		goto Skip;
	}

	//
	// Skip if it's not our tracking object or not for overlapped I/O 
	//

	Object = IoLookupObjectByHandle(FileHandle);
	if (!Object) {
		goto Skip;
	}

	if (!FlagOn(Object->Flags, OF_OVERLAPPED)) {
		IoUnreferenceObject(Object);
		goto Skip;
	}

	Status = (*CallbackPtr)(FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads);
	if (Status) {
		Object->Flags |= OF_IOCPASSOCIATE;
	}

	IoUnreferenceObject(Object);
	return Status;
}

//
// N.B. This routine must be intercepted because if a I/O is completed
// in synchronization fashion, kernel may not queue a notification packet
// to completion port which the I/O target bind to.
//

BOOL WINAPI 
IoSetFileCompletionNotificationModes(
	_In_ HANDLE FileHandle,
	_In_ UCHAR  Flags
	)
{
	ULONG Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	SetFileCompletionNotificationModesPtr CallbackPtr;
	PIO_OBJECT Object;

	Callback = IoGetCallback(_IoSetFileCompletionNotificationModes);
	CallbackPtr = (SetFileCompletionNotificationModesPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(FileHandle, Flags);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	//
	// Skip if it's not our tracking object or not for overlapped I/O 
	//

	Object = IoLookupObjectByHandle(FileHandle);
	if (!Object) {
		goto Skip;
	}

	if (!FlagOn(Object->Flags, OF_OVERLAPPED)) {
		IoUnreferenceObject(Object);
		goto Skip;
	}

	Status = (*CallbackPtr)(FileHandle, Flags);
	if (Status) { 
		if (Flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) {
			Object->Flags |= OF_SKIPONSUCCESS;
		}
		if (Flags & FILE_SKIP_SET_EVENT_ON_HANDLE) {
			Object->Flags |= OF_SKIPSETEVENT;
		}
	}

	IoUnreferenceObject(Object);
	return Status;
}

BOOL WINAPI
IoCloseHandle(
    __in HANDLE Handle
    )
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoCloseHandlePtr CallbackPtr;
	BOOL Status;

	Callback = IoGetCallback(_IoCloseHandle);
	CallbackPtr = (IoCloseHandlePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(Handle);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(Handle);
	if (!Status) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	IoRemoveObjectByHandleEx(Handle, HANDLE_FILE);

	SetLastError(ERROR_SUCCESS);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

//
// N.B. Synchronous IO can use lpOverlapped too.
//

BOOL WINAPI 
IoReadFile(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
  )
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoReadFilePtr CallbackPtr;
	DWORD IoStatus;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoReadFile);
	CallbackPtr = (IoReadFilePtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToRead,
					lpNumberOfBytesRead,
					lpOverlapped
					);
		return Status;
	}
	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_READ;
	Irp->RequestBytes = nNumberOfBytesToRead;
	IoUnreferenceObject(Object);

	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}
	
	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToRead,
					lpNumberOfBytesRead,
					Overlapped
					);
	IoStatus = GetLastError();
	Irp->LastError = IoStatus;

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRead, lpOverlapped);
	} 
	else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRead, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	SetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

BOOL WINAPI 
IoReadFileEx(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToRead,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
  )
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoReadFileExPtr CallbackPtr;
	PIO_IRP Irp;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	ULONG IoStatus;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoReadFileEx);
	CallbackPtr = (IoReadFileExPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread || !lpCompletionRoutine){
Skip:
		Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToRead,
					lpOverlapped,
					lpCompletionRoutine
					);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_READ;
	Irp->RequestBytes = nNumberOfBytesToRead;
	Irp->Flags.UserApc = 1;
	IoUnreferenceObject(Object);
	IoCaptureStackTrace(Thread, Callback, Irp);

	IoHijackOverlapped(Irp, lpOverlapped);
	Irp->ApcCallback = lpCompletionRoutine;
	
	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToRead,
					&Irp->Overlapped,
					IoFileCompleteCallback	
					);

	IoStatus = GetLastError();
	Irp->LastError = IoStatus;
	IoIrpClearInCall(Irp);

	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

BOOL WINAPI 
IoWriteFile(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToWrite,
  _Out_opt_   LPDWORD      lpNumberOfBytesWrite,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
  )
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoWriteFilePtr CallbackPtr;
	BOOLEAN SkipOnSuccess;
	
	DWORD IoStatus;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;

	Callback = IoGetCallback(_IoWriteFile);
	CallbackPtr = (IoWriteFilePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToWrite,
					lpNumberOfBytesWrite,
					lpOverlapped
					);
		return Status;
	}
	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_WRITE;
	Irp->RequestBytes = nNumberOfBytesToWrite;
	IoUnreferenceObject(Object);
	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}
	
	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToWrite,
					lpNumberOfBytesWrite,
					Overlapped
					);

	IoStatus = GetLastError();
	Irp->LastError = IoStatus;

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesWrite, lpOverlapped);
	} else {
		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesWrite, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	SetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

BOOL WINAPI 
IoWriteFileEx(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToWrite,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
  )
{
	BOOL Status;
	PBTR_CALLBACK Callback;
	IoWriteFileExPtr CallbackPtr;
	PBTR_THREAD_OBJECT Thread;
	DWORD IoStatus;
	PIO_IRP Irp;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWriteFileEx);
	CallbackPtr = (IoWriteFileExPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread || !lpOverlapped || !lpCompletionRoutine){
Skip:
		Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToWrite,
					lpOverlapped,
					lpCompletionRoutine
					);
		return Status;
	}
	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_WRITE;
	Irp->RequestBytes = nNumberOfBytesToWrite;
	Irp->Flags.UserApc = 1;

	IoUnreferenceObject(Object);
	IoCaptureStackTrace(Thread, Callback, Irp);

	IoHijackOverlapped(Irp, lpOverlapped);
	Irp->ApcCallback = lpCompletionRoutine;
	
	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(hFile,
					lpBuffer,
					nNumberOfBytesToWrite,
					&Irp->Overlapped,
					IoFileCompleteCallback	
					);

	IoStatus = GetLastError();
	Irp->LastError = IoStatus;
	IoIrpClearInCall(Irp);

	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

//
// N.B. GetOverlappedResult assume caller don't modify the
// lpOverlapped's internal fields, because we embed an irp
// pointer to track this io request, since we pass our irp
// to kernel, if caller modify the lpOverlapped->Internal,
// we have no way to track this irp again, fortunately, most
// callers don't, because after an io request was issued,
// it's illegal to modify the overlapped fields before 
// GetOverlappedResult return a completed (success or failure)
// status.
//

BOOL WINAPI 
IoGetOverlappedResult(
	_In_  HANDLE       hFile,
	_In_  LPOVERLAPPED lpOverlapped,
	_Out_ LPDWORD      lpNumberOfBytesTransferred,
	_In_  BOOL         bWait
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	GetOverlappedResultPtr CallbackPtr;
	ULONG IoStatus;
	PIO_IRP Irp;
	PIO_OBJECT Object;

	Callback = IoGetCallback(_IoGetOverlappedResult);
	CallbackPtr = (GetOverlappedResultPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	//
	// N.B. Try to get an irp pointer if we're lucky 
	//

	Irp = IoGetIrpFromInternal(lpOverlapped);
	if (!Irp) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hFile, &Irp->Overlapped, lpNumberOfBytesTransferred, bWait);
	IoStatus = GetLastError();

	ASSERT(Irp->Original == lpOverlapped);
	if ((ULONG_PTR)Irp->Original->Internal != (ULONG_PTR)Irp) {

		//
		// This indicate that user changed his overlapped's Internal
		// field before issue the call, it's not harmful since we're
		// already get the verified IRP, just print a warning in debug
		// log
		//

		DebugTrace("IO: RID: %d, Irp->Original->Internal != Irp %p", Irp->RequestId, Irp);
	} 
	else {
		DebugTrace("IO: GOR: RID: %d: Irp %p", Irp->RequestId, Irp);
	}

	//
	// If io is still pending, user may retry, just return here,
	// note that we still embed an irp pointer in lpOverlapped->Internal
	//

	if (!Status && IoStatus == ERROR_IO_INCOMPLETE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	//
	// IO is completed, success or failure, duplicate
	// the io status to user's lpOverlapped, if the user
	// retry to use the same lpOverlapped repeatly call
	// this routine, we won't get an irp pointer.
	//

	QueryPerformanceCounter(&Irp->End);
	IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);

	if (Irp->Flags.Socket) {
		Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_SOCKET);
		if (Object) { 
			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
			IoUnreferenceObject(Object);
		}
	}

	if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
		Object = (PIO_OBJECT)Irp->ControlContext;
		if (Status == ERROR_SUCCESS) {
			if (!Irp->ControlData) {
				IoClearObjectOverlapped(Object);
			} else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoUnreferenceObject(Object);
	}

	Irp->IoStatus = IoStatus;
	Irp->CompleteBytes = IO_COMPLETE_SIZE(Irp);
	Irp->CompleteThreadId = GetCurrentThreadId();
	Irp->Flags.Completed = 1;

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);

	SetLastError(IoStatus);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

BOOL WINAPI 
IoGetQueuedCompletionStatus(
	_In_  HANDLE       CompletionPort,
	_Out_ LPDWORD      lpNumberOfBytes,
	_Out_ PULONG_PTR   lpCompletionKey,
	_Out_ LPOVERLAPPED *lpOverlapped,
	_In_  DWORD        dwMilliseconds
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	GetQueuedCompletionStatusPtr CallbackPtr;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;

	Callback = IoGetCallback(_IoGetQueuedCompletionStatus);
	CallbackPtr = (GetQueuedCompletionStatusPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread || !lpCompletionKey || !lpNumberOfBytes || !lpOverlapped || !BtrProbePtrPtr(lpOverlapped)) {
		Status = (*CallbackPtr)(CompletionPort, lpNumberOfBytes, lpCompletionKey, lpOverlapped, dwMilliseconds);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
 
	Status = (*CallbackPtr)(CompletionPort, lpNumberOfBytes, lpCompletionKey, lpOverlapped, dwMilliseconds);
	if (!Status) {
		if (!*lpOverlapped) {

			//
			// No packet dequeued
			//

			ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
			return Status;
		}	

		//
		// a failed IO packet dequeued
		//
	}

	IoStatus = GetLastError();
	Overlapped = *lpOverlapped;

	//
	// N.B. This check can be skipped, we check here for performance,
	// if we don't check, IoOverlappedToIrp() may cause lots of access
	// violation since the posted iocp packet are not irps issued before,
	// they are typically used to post workload to thread pool
	//

	if (IoIsPostedOverlapped(*lpOverlapped)) {

		PIO_IOCP_PACKET Packet;
		Packet = (PIO_IOCP_PACKET)*lpOverlapped;
		*lpOverlapped = Packet->Overlapped;
		IoFreeIocpPacket(Packet);

		SetLastError(IoStatus);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	Irp = IoOverlappedToIrp(*lpOverlapped);
	if (!Irp) {
		SetLastError(IoStatus);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Irp->End);
	if ((ULONG_PTR)Irp->Original->Internal != (ULONG_PTR)Irp) {

		//
		// This indicate that user changed his overlapped's Internal
		// field before issue the call, it's not harmful since we're
		// already get the verified IRP, just print a warning in debug
		// log
		//

		DebugTrace("IO: GQCS: RID: %d, Irp->Original->Internal %p != Irp %p, Status=%d, IoStatus=%d, O.IoStatus=%d, MS=%d", 
			Irp->RequestId, Irp->Original->Internal, Irp, Status, IoStatus, Irp->Overlapped.Internal, dwMilliseconds);
	}
	else {
		DebugTrace("IO: GQCS: RID: %d: Irp %p, Status=%d, IoStatus=%d, O.IoStatus=%d, MS=%d", 
			Irp->RequestId, Irp, Status, IoStatus, Irp->Overlapped.Internal, dwMilliseconds);
	}

	//
	// Copy original user's lpOverlapped and io status
	//

	*lpOverlapped = Irp->Original;
	IoCopyOverlapped(&Irp->Overlapped, Irp->Original);

	Irp->IoStatus = IoStatus;
	Irp->CompleteBytes = IO_COMPLETE_SIZE(Irp);
	Irp->CompleteThreadId = GetCurrentThreadId();
	Irp->Flags.Completed = 1;

	Object = NULL;
	if (Irp->Flags.Socket) {

		//
		// N.B. Object can be destroyed, we must check this,
		// because io request can be aborted and object can 
		// be closed when we still get io notification issued before.
		//

		Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_SOCKET);
		if (Object) { 

			//
			// If this is a IO_OP_ACCEPT from AcceptEx, and successful to established
			// connection, update socket context to retrieve socket pair address
			//

			if (Irp->Operation == IO_OP_ACCEPT && IoStatus == ERROR_SUCCESS) {
				ASSERT(Irp->SkListen != 0);
				setsockopt((SOCKET)Irp->Object, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
							(char *)&Irp->SkListen, sizeof(SOCKET));
			}

			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
			IoUnreferenceObject(Object);
		}
	}

	if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
		ASSERT(Object != NULL);
		if (Object && Status == ERROR_SUCCESS) {
			if (!Irp->ControlData) {
				IoClearObjectOverlapped(Object);
			} else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoUnreferenceObject(Object);
	}

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);

	SetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

//
// N.B. This routine is used to filter out all non-IO completion request,
// many applications may use it as a queue mechanism to dispatch workloads 
//

BOOL WINAPI 
IoPostQueuedCompletionStatus(
	_In_     HANDLE       CompletionPort,
	_In_     DWORD        dwNumberOfBytesTransferred,
	_In_     ULONG_PTR    dwCompletionKey,
	_In_opt_ LPOVERLAPPED lpOverlapped
	)
{
	PBTR_CALLBACK Callback;
	PostQueuedCompletionStatusPtr CallbackPtr;
	PBTR_THREAD_OBJECT Thread;
	BOOL Status;
	PIO_IOCP_PACKET Packet;

	Callback = IoGetCallback(_IoPostQueuedCompletionStatus);
	CallbackPtr = (PostQueuedCompletionStatusPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(CompletionPort, dwNumberOfBytesTransferred, dwCompletionKey, lpOverlapped);
		InterlockedDecrement(&Callback->References);
		return Status;
	}
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Packet = IoAllocateIocpPacket();
	Packet->Overlapped = lpOverlapped;

	//
	// N.B. Replace the original lpOverlapped with our packet, in GetQueuedCompletionStatus,
	// we can determine whether it's a IO completion by scan of our overlapped list, since
	// the kernel don't touch the lpOverlapped parameter, it's safe.
	//

	Status = (*CallbackPtr)(CompletionPort, dwNumberOfBytesTransferred, dwCompletionKey, (LPOVERLAPPED)Packet);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

BOOL WINAPI 
IoGetQueuedCompletionStatusEx(
	_In_  HANDLE             CompletionPort,
	_Out_ LPOVERLAPPED_ENTRY lpCompletionPortEntries,
	_In_  ULONG              ulCount,
	_Out_ PULONG             ulNumEntriesRemoved,
	_In_  DWORD              dwMilliseconds,
	_In_  BOOL               fAlertable
)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	BOOL Status;
	LPOVERLAPPED Overlapped;
	GetQueuedCompletionStatusExPtr CallbackPtr;
	LARGE_INTEGER End;
	PIO_IRP Irp;
	ULONG Number;
	PIO_OBJECT Object;

	Callback = IoGetCallback(_IoGetQueuedCompletionStatusEx);
	CallbackPtr = (GetQueuedCompletionStatusExPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(CompletionPort, lpCompletionPortEntries, ulCount, 
							ulNumEntriesRemoved, dwMilliseconds, fAlertable);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!lpCompletionPortEntries) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(CompletionPort, lpCompletionPortEntries, ulCount, 
							ulNumEntriesRemoved, dwMilliseconds, fAlertable);

	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&End);

	for(Number = 0; Number < *ulNumEntriesRemoved; Number += 1) {

		Overlapped = lpCompletionPortEntries[Number].lpOverlapped;
		if (IoIsPostedOverlapped(Overlapped)) {

			PIO_IOCP_PACKET Packet;
			Packet = (PIO_IOCP_PACKET)Overlapped;
			Overlapped = Packet->Overlapped;
			IoFreeIocpPacket(Packet);

			//
			// fix the overlapped entry with user's original one
			//

			lpCompletionPortEntries[Number].lpOverlapped = Overlapped;
			continue;
		}

		Irp = IoOverlappedToIrp(Overlapped);
		if (!Irp) {
			continue;
		}

		if ((ULONG_PTR)Irp->Original->Internal != (ULONG_PTR)Irp) {

			//
			// This indicate that user changed his overlapped's Internal
			// field before issue the call, it's not harmful since we're
			// already get the verified IRP, just print a warning in debug
			// log
			//

			DebugTrace("IO: RID: %d, Irp->Original->Internal != Irp %p", Irp->RequestId);
		}

		Overlapped = Irp->Original;
		IoCopyOverlapped(&Irp->Overlapped, Overlapped);

		Irp->IoStatus = (ULONG)Irp->Overlapped.Internal;
		Irp->CompleteBytes = IO_COMPLETE_SIZE(Irp);
		Irp->End.QuadPart = End.QuadPart;
		Irp->CompleteThreadId = GetCurrentThreadId();
		Irp->Flags.Completed = 1;

		if (Irp->Flags.Socket) {
			Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_SOCKET);
			if (Object) {
				IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
				IoUnreferenceObject(Object);
			}
		}
		if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
			if (Status == ERROR_SUCCESS) {
				if (!Irp->ControlData) {
					IoClearObjectOverlapped(Object);
				} else {
					IoMarkObjectOverlapped(Object);
				}
			}
			IoUnreferenceObject(Object);
		}
		IoQueueFlushList(Irp);

		//
		// fix the overlapped entry with user's original one
		//

		lpCompletionPortEntries[Number].lpOverlapped = Overlapped;
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

SOCKET WINAPI
IoSocket(
	_In_ int af,
	_In_ int type,
	_In_ int protocal
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	SocketPtr CallbackPtr;
	PIO_OBJECT Object;
	SOCKET s;

	Callback = IoGetCallback(_IoSocket);
	CallbackPtr = (SocketPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		s = (*CallbackPtr)(af, type, protocal);
		InterlockedDecrement(&Callback->References);
		return s;
	}

	if ((af != AF_INET && af != AF_INET6) || (type != SOCK_STREAM && type != SOCK_DGRAM)) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	s = (*CallbackPtr)(af, type, protocal);
	if (s == INVALID_SOCKET) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return INVALID_SOCKET;
	}
	
	Object = IoAllocateObject();
	Object->Id = IoAcquireObjectId();
	Object->Object = SK_HANDLE(s);
	Object->Type = HANDLE_SOCKET;

	if (af == AF_INET) {
		SetFlag(Object->Flags, OF_SKIPV4);
	}
	else { 
		SetFlag(Object->Flags, OF_SKIPV6);
	}
	if (type == SOCK_STREAM) {
		SetFlag(Object->Flags, OF_SKTCP);
	}
	else {
		SetFlag(Object->Flags, OF_SKUDP);
	}
	GetSystemTimeAsFileTime(&Object->Start);

	IoInsertObject(Object);

	WSASetLastError(0);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return s;
}

int WINAPI IoWSARecv(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSARecvPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback; 
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSARecv);
	CallbackPtr = (WSARecvPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		Status = (*CallbackPtr)(s,
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesRecvd,
							lpFlags,
							lpOverlapped,
							lpCompletionRoutine
							);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	Size = 0;
	for (i = 0; i < dwBufferCount; i++) {
		Size += lpBuffers[i].len; 
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->CallType = _IoWSARecv;
	Irp->RequestBytes = Size;
	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}
	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	} else {
		ApcCallback = lpCompletionRoutine;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(s,
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesRecvd,
							lpFlags,
							Overlapped,
							ApcCallback
							);

	DebugTrace("%s: irp=%p, rid=%d, IsOverlapped=%d", __FUNCTION__, Irp, Irp->RequestId, IsOverlapped);

	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}
	
	Irp->LastError = IoStatus;
	if (!IsOverlapped && IoStatus != WSA_IO_PENDING) {

		IoQuerySocketAddress(Object, s);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
		IoUnreferenceObject(Object);

	} else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus == ERROR_SUCCESS) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			DebugTrace("%s: SkipOnSuccess, irp=%p, rid=%d, IoStatus=%d", 
						__FUNCTION__, Irp, Irp->RequestId, IoStatus);
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;;
}

SOCKET WINAPI
IoWSASocket(
	_In_ int af,
	_In_ int type,
	_In_ int protocol,
	_In_ LPWSAPROTOCOL_INFO lpProtocolInfo,
	_In_ GROUP g,
	_In_ DWORD dwFlags
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSASocketPtr CallbackPtr;
	PIO_OBJECT Object;
	SOCKET s;

	Callback = IoGetCallback(_IoWSASocket);
	CallbackPtr = (WSASocketPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		s = (*CallbackPtr)(af, type, protocol, lpProtocolInfo, g, dwFlags);
		InterlockedDecrement(&Callback->References);
		return s;
	}

	if ((af != AF_INET && af != AF_INET6) || (type != SOCK_STREAM && type != SOCK_DGRAM)) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	s = (*CallbackPtr)(af, type, protocol, lpProtocolInfo, g, dwFlags);
	if (s == INVALID_SOCKET) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return INVALID_SOCKET;
	}
	
	Object = IoAllocateObject();
	Object->Id = IoAcquireObjectId();
	Object->Object = SK_HANDLE(s);
	Object->Type = HANDLE_SOCKET;

	if (af == AF_INET) {
		SetFlag(Object->Flags, OF_SKIPV4);
	}
	else { 
		SetFlag(Object->Flags, OF_SKIPV6);
	}
	if (type == SOCK_STREAM) {
		SetFlag(Object->Flags, OF_SKTCP);
	}
	else {
		SetFlag(Object->Flags, OF_SKUDP);
	}
	if (dwFlags & WSA_FLAG_OVERLAPPED) {
		SetFlag(Object->Flags, OF_OVERLAPPED);
	}

	GetSystemTimeAsFileTime(&Object->Start);

	IoInsertObject(Object);

	WSASetLastError(0);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return s;
}

SOCKET WINAPI
IoAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ int* addrlen
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	AcceptPtr CallbackPtr;
	PIO_OBJECT Object;
	PIO_OBJECT Accepted;
	SOCKET Status;
	ULONG IoStatus;
	int Length;
	SOCKADDR_STORAGE Address;

	Callback = IoGetCallback(_IoAccept);
	CallbackPtr = (AcceptPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		Status = (*CallbackPtr)(s, addr, addrlen);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	IO_GET_SOCKET_OBJECT(s, &Object);
	if (!Object){
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		goto Skip;
		return Status;
	}

	Length = sizeof(SOCKADDR_STORAGE);
	Status = (*CallbackPtr)(s, (struct sockaddr *)&Address, &Length);

	IoStatus = WSAGetLastError();
	if (Status == INVALID_SOCKET) {
		IoUnreferenceObject(Object);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	} 

	Accepted = IoAllocateObject();
	Accepted->Id = IoAcquireObjectId();
	Accepted->Object = SK_HANDLE(Status);
	Accepted->Type = HANDLE_SOCKET;

	if (FlagOn(Object->Flags, OF_SKTCP)){
		Accepted->Flags |= OF_SKTCP;
	}
	if (FlagOn(Object->Flags, OF_SKUDP)){
		Accepted->Flags |= OF_SKUDP;
	}
	if (FlagOn(Object->Flags, OF_SKIPV4)) {
		Accepted->Flags |= OF_SKIPV4;
	}
	if (FlagOn(Object->Flags, OF_SKIPV6)) {
		Accepted->Flags |= OF_SKIPV6;
	}

	//
	// fill socket address, copy from object as local address
	//

	if (Length) {
		Accepted->Flags |= OF_REMOTE_VALID;
		RtlCopyMemory(&Accepted->u.Socket.Remote, &Address, Length);
		Accepted->Flags |= OF_LOCAL_VALID;
		RtlCopyMemory(&Accepted->u.Socket.Local, &Object->u.Socket.Local, Length);
	}

	if (addr && addrlen) {
		RtlCopyMemory(addr, &Address, (*addrlen >= Length) ? Length : *addrlen);
	}

	IoUnreferenceObject(Object);
	IoInsertObject(Accepted);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

int WINAPI
IoConnect(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ConnectPtr CallbackPtr;
	PIO_OBJECT Object;
	int Status;
	ULONG IoStatus;

	Callback = IoGetCallback(_IoConnect);
	CallbackPtr = (ConnectPtr)BtrGetCallbackDestine(Callback);
	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		return (*CallbackPtr)(s, name, namelen);
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	Status = (*CallbackPtr)(s, name, namelen);
	if (Status == SOCKET_ERROR){
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}
	IoStatus = WSAGetLastError();

	IO_GET_SOCKET_OBJECT(s, &Object);
	if (!Object) {
		WSASetLastError(IoStatus);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}
	
	ASSERT(FlagOn(Object->Flags, OF_LOCAL_VALID));

	RtlCopyMemory(&Object->u.Socket.Remote, name, namelen);
	SetFlag(Object->Flags, OF_REMOTE_VALID);

	IoUnreferenceObject(Object);
	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return 0;
}

int WINAPI
IoCtlSocket(
	_In_  SOCKET s,
	_In_  long cmd,
	_Out_ u_long* argp
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	IoCtlSocketPtr CallbackPtr;
	PIO_OBJECT Object;
	ULONG IoStatus;

	Callback = IoGetCallback(_IoCtlSocket);
	CallbackPtr = (IoCtlSocketPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread || cmd != FIONBIO){
Skip:
		Status = (*CallbackPtr)(s, cmd, argp);
		return Status;
	}

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	Status = (*CallbackPtr)(s, cmd, argp);
	IoStatus = WSAGetLastError();
	if (Status == ERROR_SUCCESS) {
		if (*argp != 0) {
			IoMarkObjectOverlapped(Object);
		} else {
			IoClearObjectOverlapped(Object);
		}
	}

	IoUnreferenceObject(Object);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

int WINAPI
IoWSAConnect(
	_In_ SOCKET s,
	_In_ const struct sockaddr* name,
	_In_ int namelen,
	_In_ LPWSABUF lpCallerData,
	_Out_ LPWSABUF lpCalleeData,
	_In_  LPQOS lpSQOS,
	_In_  LPQOS lpGQOS
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSAConnectPtr CallbackPtr;
	PIO_OBJECT Object;
	int Status;
	ULONG IoStatus;

	Callback = IoGetCallback(_IoWSAConnect);
	CallbackPtr = (WSAConnectPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object)
		goto Skip;

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
	IoStatus = WSAGetLastError();
	if (Status != SOCKET_ERROR) {
		*(SOCKADDR *)&Object->u.Socket.Remote = *name;
		SetFlag(Object->Flags, OF_REMOTE_VALID);
	} 
	IoUnreferenceObject(Object);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

SOCKET WINAPI
IoWSAAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ LPINT addrlen,
	_In_  LPCONDITIONPROC lpfnCondition,
	_In_  DWORD dwCallbackData
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSAAcceptPtr CallbackPtr;
	PIO_OBJECT Object;
	PIO_OBJECT Accepted;
	SOCKET Status;
	ULONG IoStatus;
	int Length;
	int RealLength;
	SOCKADDR_STORAGE Address;

	Callback = IoGetCallback(_IoWSAAccept);
	CallbackPtr = (WSAAcceptPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, addr, addrlen, lpfnCondition, dwCallbackData);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object)
		goto Skip;

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Length = sizeof(SOCKADDR_STORAGE);
	Status = (*CallbackPtr)(s, (struct sockaddr *)&Address, &Length, lpfnCondition, dwCallbackData);

	IoStatus = WSAGetLastError();
	if (Status == INVALID_SOCKET) {
		IoUnreferenceObject(Object);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	} 

	Accepted = IoAllocateObject();
	Accepted->Id = IoAcquireObjectId();
	Accepted->Object = SK_HANDLE(Status);
	Accepted->Type = HANDLE_SOCKET;

	SetFlag(Accepted->Flags, OF_SKTCP);
	if (FlagOn(Object->Flags, OF_SKIPV4)) {
		SetFlag(Accepted->Flags, OF_SKIPV4);
	}
	else {
		SetFlag(Accepted->Flags, OF_SKIPV6);
	}

	//
	// fill socket address
	//

	RealLength = Length;
	*(SOCKADDR *)&Accepted->u.Socket.Remote = *(SOCKADDR *)&Address;
	SetFlag(Accepted->Flags, OF_REMOTE_VALID);

	Length = sizeof(SOCKADDR);
	if (!getsockname(Status, (struct sockaddr *)&Accepted->u.Socket.Local, &Length)) {
		SetFlag(Object->Flags, OF_LOCAL_VALID);
	}
	if (addr && addrlen) {
		memcpy(addr, &Address, (*addrlen >= RealLength) ? RealLength : *addrlen);
	}

	IoUnreferenceObject(Object);
	IoInsertObject(Accepted);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

int WINAPI 
IoWSAEventSelect(
	_In_ SOCKET s,
	_In_ WSAEVENT hEventObject,	
	_In_ long lNetworkEvents
	)
{
	int Status;
	PVOID Caller;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSAEventSelectPtr CallbackPtr;
	PIO_OBJECT Object;
	ULONG IoStatus;

	Caller = _ReturnAddress();
	Callback = IoGetCallback(_IoWSAEventSelect);
	CallbackPtr = (WSAEventSelectPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		Status = (*CallbackPtr)(s, hEventObject, lNetworkEvents);
		return Status;
	}

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	//
	// N.B. WSAAsyncSelect automatically set socket as
	// nonblocking, we mark it as overlapped since it support
	// overlapped io.
	//

	Status = (*CallbackPtr)(s, hEventObject, lNetworkEvents);
	IoStatus = WSAGetLastError();
	if (Status == ERROR_SUCCESS) {
		IoMarkObjectOverlapped(Object);
	}

	IoUnreferenceObject(Object);
	WSASetLastError(IoStatus);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

int WINAPI
IoWSAAsyncSelect(
	_In_ SOCKET s,
	_In_ HWND hWnd,
	_In_ unsigned int wMsg,
	_In_ long lEvent
	)
{	
	int Status;
	PVOID Caller;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSAAsyncSelectPtr CallbackPtr;
	PIO_OBJECT Object;
	ULONG IoStatus;

	Caller = _ReturnAddress();
	Callback = IoGetCallback(_IoWSAAsyncSelect);
	CallbackPtr = (WSAAsyncSelectPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		Status = (*CallbackPtr)(s, hWnd, wMsg, lEvent);
		return Status;
	}

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		goto Skip;
	}

	//
	// N.B. WSAAsyncSelect automatically set socket as
	// nonblocking, we mark it as overlapped since it support
	// overlapped io.
	//

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(s, hWnd, wMsg, lEvent);
	IoStatus = WSAGetLastError();
	if (Status == ERROR_SUCCESS) {
		IoMarkObjectOverlapped(Object);
	}

	IoUnreferenceObject(Object);
	WSASetLastError(IoStatus);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

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
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSAIoctlPtr CallbackPtr;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	ULONG IoStatus;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSAIoctl);
	CallbackPtr = (WSAIoctlPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread || dwIoControlCode != FIONBIO){
Skip:
		Status = (*CallbackPtr)(s,
								dwIoControlCode,
								lpvInBuffer,
								cbInBuffer,
								lpvOutBuffer,
								cbOutBuffer,
								lpcbBytesReturned,
								lpOverlapped,
								lpCompletionRoutine
								);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object,&IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}
	
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_IOCONTROL;
	Irp->RequestBytes = sizeof(ULONG);

	ApcCallback = lpCompletionRoutine;
	if (IsOverlapped) { 
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
		if (lpCompletionRoutine) {
			ApcCallback = IoNetCompleteCallback;
			Irp->ApcCallback = lpCompletionRoutine;
		}
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}

	//
	// N.B. We attach the object ptr to control context, the completor
	// must set object's state and unreference object.
	//

	Irp->ControlCode = FIONBIO;
	Irp->ControlData = *(PULONG)lpvInBuffer;
	Irp->ControlContext = Object;

	Status = (*CallbackPtr)(s, dwIoControlCode, lpvInBuffer, cbInBuffer, 
								lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
								Overlapped, ApcCallback);
	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;

	if (!IsOverlapped){
		if (Status == ERROR_SUCCESS) {
			if (*(PULONG)lpvInBuffer == 0) {
				IoClearObjectOverlapped(Object);
			} else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpcbBytesReturned, Overlapped);
	}else {
		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpcbBytesReturned, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

int WINAPI 
IoClosesocket(
  _In_ SOCKET s
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG IoStatus;
	ClosesocketPtr CallbackPtr;
	PIO_OBJECT Object;

	Callback = IoGetCallback(_IoClosesocket);
	CallbackPtr = (ClosesocketPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		Status = (*CallbackPtr)(s);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object){
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		goto Skip;
	}

	//
	// Before close handle, query its address
	//

	IoQuerySocketAddress(Object, s);

	Status = (*CallbackPtr)(s);
	IoStatus = WSAGetLastError();
	
	IoRemoveObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);

	WSASetLastError(IoStatus);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

VOID
IoDebugPrintSkAddress(
	_In_ PIO_OBJECT Object
	)
{
#ifdef _DEBUG
	SOCKADDR *Addr;
	struct sockaddr_in *in;

	ASSERT(Object->Type == HANDLE_SOCKET);

	if (FlagOn(Object->Flags, OF_LOCAL_VALID)) {
		Addr = (SOCKADDR *)&Object->u.Socket.Local;
		in = (struct sockaddr_in *)Addr;
		BtrTrace("Local: %s:%u", inet_ntoa(in->sin_addr),  ntohs(in->sin_port));
	}
	else {
		BtrTrace("Local: INVALID");
	}
	if (FlagOn(Object->Flags, OF_REMOTE_VALID)) {
		Addr = (SOCKADDR *)&Object->u.Socket.Remote;
		in = (struct sockaddr_in *)Addr;
		BtrTrace("Remote: %s:%u", inet_ntoa(in->sin_addr), ntohs(in->sin_port));
	}
	else {
		BtrTrace("Remote: INVALID");
	}
#endif
}

VOID
IoDebugPrintSkAddressV6(
	_In_ PIO_OBJECT Object
	)
{
#ifdef _DEBUG
	SOCKADDR *Addr;
	struct sockaddr_in *in;

	if (FlagOn(Object->Flags, OF_LOCAL_VALID)) {
		Addr = (SOCKADDR *)&Object->u.Socket.Local;
		in = (struct sockaddr_in *)Addr;
		BtrTrace("Local: %s:%u", inet_ntoa(in->sin_addr),  ntohs(in->sin_port));
	}
	else {
		BtrTrace("Local: INVALID");
	}
	if (FlagOn(Object->Flags, OF_REMOTE_VALID)) {
		Addr = (SOCKADDR *)&Object->u.Socket.Remote;
		in = (struct sockaddr_in *)Addr;
		BtrTrace("Remote: %s:%u", inet_ntoa(in->sin_addr), ntohs(in->sin_port));
	}
	else {
		BtrTrace("Remote: INVALID");
	}
#endif
}

int WINAPI 
IoRecv(
  _In_  SOCKET s,
  _Out_ char   *buf,
  _In_  int    len,
  _In_  int    flags
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	RecvPtr CallbackPtr;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LARGE_INTEGER Start;
	FILETIME Time;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoRecv);
	CallbackPtr = (RecvPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, buf, len, flags);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, NULL, &Object,&IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = (*CallbackPtr)(s, buf, len, flags);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = 1;
	IoCaptureStackTrace(Thread, Callback, Irp);

	IoQuerySocketAddress(Object, s);
	IoUnreferenceObject(Object);

	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

int WINAPI IoRecvfrom(
  _In_        SOCKET          s,
  _Out_       char            *buf,
  _In_        int             len,
  _In_        int             flags,
  _Out_       struct sockaddr *from,
  _Inout_opt_ int             *fromlen
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	RecvfromPtr CallbackPtr;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LARGE_INTEGER Start;
	FILETIME Time;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoRecvfrom);
	CallbackPtr = (RecvfromPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, buf, len, flags, from, fromlen);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, NULL, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = (*CallbackPtr)(s, buf, len, flags, from, fromlen);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = 1;
	IoCaptureStackTrace(Thread, Callback, Irp);

	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

int WINAPI IoSend(
  _In_       SOCKET s,
  _In_ const char   *buf,
  _In_       int    len,
  _In_       int    flags
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	SendPtr CallbackPtr;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LARGE_INTEGER Start;
	FILETIME Time;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoSend);
	CallbackPtr = (SendPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, buf, len, flags);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, NULL, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = (*CallbackPtr)(s, buf, len, flags);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = 1;
	IoCaptureStackTrace(Thread, Callback, Irp);

	IoQuerySocketAddress(Object, s);
	IoUnreferenceObject(Object);

	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

int WINAPI IoSendto(
  _In_       SOCKET                s,
  _In_ const char                  *buf,
  _In_       int                   len,
  _In_       int                   flags,
  _In_       const struct sockaddr *to,
  _In_       int                   tolen
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	SendtoPtr CallbackPtr;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LARGE_INTEGER Start;
	FILETIME Time;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoSendto);
	CallbackPtr = (SendtoPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, buf, len, flags, to, tolen);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, NULL, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = (*CallbackPtr)(s, buf, len, flags, to, tolen);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = 1;
	IoCaptureStackTrace(Thread, Callback, Irp);

	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

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
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSARecvFromPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback; 
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSARecvFrom);
	CallbackPtr = (WSARecvFromPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s,
								lpBuffers,
								dwBufferCount,
								lpNumberOfBytesRecvd,
								lpFlags,
								lpFrom,
								lpFromlen,
								lpOverlapped,
								lpCompletionRoutine
								);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess))
		goto Skip;

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Size = 0;
	for(i = 0; i < dwBufferCount; i++)
		Size += lpBuffers[i].len;

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->CallType = _IoWSARecvFrom;
	Irp->RequestBytes = Size;

	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	}
	else { 
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}

	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	} else {
		ApcCallback = NULL;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);
	Status = (*CallbackPtr)(s,
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesRecvd, 
							lpFlags,
							lpFrom,
							lpFromlen,
							Overlapped,
							ApcCallback	
							);

	DebugTrace("%s: irp=%p, rid=%d, IsOverlapped=%d", __FUNCTION__, Irp, Irp->RequestId, IsOverlapped);
	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
	}else {
		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {	
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			DebugTrace("%s: SkipOnSuccess, irp=%p, rid=%d, IoStatus=%d", 
						__FUNCTION__, Irp, Irp->RequestId, IoStatus);
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

int WINAPI 
IoWSASend(
  _In_  SOCKET                             s,
  _In_  LPWSABUF                           lpBuffers,
  _In_  DWORD                              dwBufferCount,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  DWORD                              dwFlags,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSASendPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback; 
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSASend);
	CallbackPtr = (WSASendPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s,
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesSent,
							dwFlags,
							lpOverlapped,
							lpCompletionRoutine
							);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	Size = 0;
	for (i = 0; i < dwBufferCount; i++) {
		Size += lpBuffers[i].len; 
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoWSASend;
	Irp->RequestBytes = Size;

	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = NULL;
	}
	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	} else {
		ApcCallback = NULL;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(s,
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesSent,
							dwFlags,
							Overlapped,
							ApcCallback	
							);

	DebugTrace("%s: irp=%p, rid=%d, IsOverlapped=%d", __FUNCTION__, Irp, Irp->RequestId, IsOverlapped);
	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (!IsOverlapped) {
		IoQuerySocketAddress(Object, s);
#ifdef _DEBUG
		IoDebugPrintSkAddress(Object);
#endif
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
	}else {
		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			DebugTrace("%s: SkipOnSuccess, irp=%p, rid=%d, IoStatus=%d", 
						__FUNCTION__, Irp, Irp->RequestId, IoStatus);
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	IoUnreferenceObject(Object);
	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;;
}

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
)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSASendToPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback; 
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSASendTo);
	CallbackPtr = (WSASendToPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s,
								lpBuffers,
								dwBufferCount,
								lpNumberOfBytesSent,
								dwFlags,
								lpTo,
								iToLen,
								lpOverlapped,
								lpCompletionRoutine
								);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess))
		goto Skip;

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Size = 0;
	for(i = 0; i < dwBufferCount; i++)
		Size += lpBuffers[i].len;

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoWSASendTo;
	Irp->RequestBytes = Size;
	
	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	}
	else { 
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}

	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	} else {
		ApcCallback = NULL;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);
	Status = (*CallbackPtr)(s,
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesSent,
							dwFlags,
							lpTo,
							iToLen,
							lpOverlapped,
							lpCompletionRoutine
							);

	DebugTrace("%s: irp=%p, rid=%d, IsOverlapped=%d", __FUNCTION__, Irp, Irp->RequestId, IsOverlapped);
	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
	}else {
		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			DebugTrace("%s: SkipOnSuccess, irp=%p, rid=%d, IoStatus=%d", 
						__FUNCTION__, Irp, Irp->RequestId, IoStatus);
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;
}

//
// N.B. WSAGetOverlappedResult internally call
// GetOverlappedResult, so this routine can be
// removed.
//

BOOL WINAPI 
IoWSAGetOverlappedResult(
  _In_  SOCKET          s,
  _In_  LPWSAOVERLAPPED lpOverlapped,
  _Out_ LPDWORD         lpcbTransfer,
  _In_  BOOL            fWait,
  _Out_ LPDWORD         lpdwFlags
)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSAGetOverlappedResultPtr CallbackPtr;
	ULONG IoStatus;
	BOOL Status;
	PIO_IRP Irp;
	PIO_OBJECT Object;

	Callback = IoGetCallback(_IoWSAGetOverlappedResult);
	CallbackPtr = (WSAGetOverlappedResultPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	//
	// Test whether it's our irp
	//

	Irp = IoGetIrpFromInternal(lpOverlapped);
	if (!Irp) {
		goto Skip;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(s, &Irp->Overlapped, lpcbTransfer, fWait, lpdwFlags);
	IoStatus = WSAGetLastError();

	ASSERT(Irp->Original == lpOverlapped);
	if ((ULONG_PTR)Irp->Original->Internal != (ULONG_PTR)Irp) {

		//
		// This indicate that user changed his overlapped's Internal
		// field before issue the call, it's not harmful since we're
		// already get the verified IRP, just print a warning in debug
		// log
		//

		DebugTrace("IO: WGOR: RID: %d: Irp->Original->Internal != Irp %p", Irp->RequestId, Irp);
	}
	else {
		DebugTrace("IO: WGOR: RID: %d: Irp %p", Irp->RequestId, Irp);
	}

	//
	// If io is still pending, user may retry, just return here,
	// note that we still embed an irp pointer in lpOverlapped->Internal
	//

	if (!Status && IoStatus == WSA_IO_INCOMPLETE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	//
	// IO is completed, success or failure, duplicate
	// the io status to user's lpOverlapped, if the user
	// retry to use the same lpOverlapped repeatly call
	// this routine, we won't get an irp pointer.
	//

	QueryPerformanceCounter(&Irp->End);
	IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);

	if (Irp->Flags.Socket) {
		Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_SOCKET);
		if (Object){
			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
			IoUnreferenceObject(Object);
		}
	}

	if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
		Object = (PIO_OBJECT)Irp->ControlContext;
		if (Status == ERROR_SUCCESS) {
			if (!Irp->ControlData) {
				IoClearObjectOverlapped(Object);
			} else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoUnreferenceObject(Object);
	}

	Irp->IoStatus = IoStatus;
	Irp->CompleteBytes = IO_COMPLETE_SIZE(Irp);
	Irp->CompleteThreadId = GetCurrentThreadId();
	Irp->Flags.Completed = 1;

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);

	WSASetLastError(IoStatus);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

VOID WINAPI 
IoThreadPoolIoCallback(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_opt_ PVOID                 Overlapped,
    _In_        ULONG                 IoResult,
    _In_        ULONG_PTR             NumberOfBytesTransferred,
    _Inout_     PTP_IO                Io
    )
{
	PBTR_THREAD_OBJECT Thread;
	PIO_TPIO_CONTEXT IoContext;
	PIO_IRP Irp;
	LPOVERLAPPED lpOverlapped;
	PIO_OBJECT Object;

	Thread = BtrIsExemptedCall(CALLER);
	ASSERT(Thread != NULL);

	IoContext = (PIO_TPIO_CONTEXT)Context;
	Irp = IoOverlappedToIrp((LPOVERLAPPED)Overlapped);

	if (!Irp) { 
		(*IoContext->pfnio)(Instance, 
							IoContext->pv, // user's context
							Overlapped, 
							IoResult,
							NumberOfBytesTransferred, 
							Io
							);		
		return;
	}

	lpOverlapped = Irp->Original;
	ASSERT(lpOverlapped->Internal == (ULONG_PTR)Irp);

	IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);

	QueryPerformanceCounter(&Irp->End);

	Object = NULL;
	if (Irp->Flags.Socket) {
		Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_SOCKET);
		if (Object) { 
			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
			IoUnreferenceObject(Object);
		}
	}

	if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
		Object = (PIO_OBJECT)Irp->ControlContext;
		if (IoResult == ERROR_SUCCESS) {
			if (!Irp->ControlData) {
				IoClearObjectOverlapped(Object);
			} else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoUnreferenceObject(Object);
	}

	Irp->IoStatus = IoResult;
	Irp->CompleteBytes = (ULONG)NumberOfBytesTransferred;
	Irp->CompleteThreadId = GetCurrentThreadId();
	Irp->Flags.Completed = 1;

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);

	//
	// Call user's callback with user's lpOverlapped
	//
	(*IoContext->pfnio)(Instance, 
						IoContext->pv, // user's context
						lpOverlapped,  // user's lpOverlapped
						IoResult,
						NumberOfBytesTransferred, 
						Io
						);		
}
	
//
// N.B. We're interested in the completion routine,
// which is replaced with our own.
//

PTP_IO WINAPI 
IoCreateThreadpoolIo(
	_In_        HANDLE                fl,
	_In_        PTP_WIN32_IO_CALLBACK pfnio,
	_Inout_opt_ PVOID                 pv,
	_In_opt_    PTP_CALLBACK_ENVIRON  pcbe
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	CreateThreadpoolIoPtr CallbackPtr;
	PTP_IO TpIo;
	PIO_TPIO_CONTEXT Context;
	DWORD Error;

	Callback = IoGetCallback(_IoCreateThreadPoolIo);
	CallbackPtr = (CreateThreadpoolIoPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){
		InterlockedIncrement(&Callback->References);
		TpIo = (*CallbackPtr)(fl, pfnio, pv, pcbe);
		InterlockedDecrement(&Callback->References);
		return TpIo;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Context = (PIO_TPIO_CONTEXT)BtrMalloc(sizeof(IO_TPIO_CONTEXT));
	Context->fl = fl;
	Context->pfnio = pfnio;
	Context->pv = pv;
	Context->pcbe = pcbe;
	Context->Callback = IoThreadPoolIoCallback;

	//
	// Execute CreateThreadpoolIo with our's callback and context
	//

	TpIo = (*CallbackPtr)(fl, IoThreadPoolIoCallback, Context, pcbe);
	Error = GetLastError();
	if (!TpIo) {
		BtrFree(Context);
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	Context->TpIo = TpIo;

	BtrAcquireSpinLock(&IoTpContextLock);
	InsertTailList(&IoTpContextList, &Context->ListEntry);
	BtrReleaseSpinLock(&IoTpContextLock);

	SetLastError(Error);
	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TpIo;
}

ULONG
IoGetFunctionPointers(
	VOID
	)
{
	SOCKET Sk; 
	ULONG Number;
	ULONG Size;
	HMODULE Dll;
	WSADATA wsaData;

	Dll = LoadLibrary("ws2_32.dll");
	if (!Dll) {
		return GetLastError();
	}

	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
		return WSAGetLastError();
	}

	Sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!Sk) {
		return GetLastError();
	}

	for(Number = 0; Number < IoFunctionPointerCount; Number += 1) {
		WSAIoctl(Sk, SIO_GET_EXTENSION_FUNCTION_POINTER, 
					&IoFunctionPointerTable[Number].Guid, sizeof(GUID), 
					&IoFunctionPointerTable[Number].Pointer, sizeof(PVOID), 
					&Size, NULL, NULL);

	}
	
	closesocket(Sk);

	//
	// Fill the callback pointers by ordinal
	//

	IoCallback[_IoAcceptEx].Address = IoFunctionPointerTable[_IoFpAcceptEx].Pointer;
	IoCallback[_IoConnectEx].Address = IoFunctionPointerTable[_IoFpConnectEx].Pointer;
	IoCallback[_IoTransmitFile].Address = IoFunctionPointerTable[_IoFpTransmitFile].Pointer;
	IoCallback[_IoTransmitPackets].Address = IoFunctionPointerTable[_IoFpTransmitPackets].Pointer;
	IoCallback[_IoWSARecvMsg].Address = IoFunctionPointerTable[_IoFpWSARecvMsg].Pointer;
	IoCallback[_IoWSASendMsg].Address = IoFunctionPointerTable[_IoFpWSASendMsg].Pointer;

	return S_OK;
}

BOOL WINAPI IoAcceptEx(
	_In_  SOCKET       sListenSocket,
	_In_  SOCKET       sAcceptSocket,
	_In_  PVOID        lpOutputBuffer,
	_In_  DWORD        dwReceiveDataLength,
	_In_  DWORD        dwLocalAddressLength,
	_In_  DWORD        dwRemoteAddressLength,
	_Out_ LPDWORD      lpdwBytesReceived,
	_In_  LPOVERLAPPED lpOverlapped
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	AcceptExPtr CallbackPtr;
	PIO_OBJECT Object;
	BOOL Status;
	ULONG IoStatus;
	SOCKADDR *Local;
	SOCKADDR *Remote;
	int LocalLength;
	int RemoteLength;
	PIO_IRP Irp;
	BOOLEAN IsOverlapped;
	BOOLEAN SkipOnSuccess;
	LPOVERLAPPED Overlapped;

	Callback = IoGetCallback(_IoAcceptEx);
	CallbackPtr = (AcceptExPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		Status = (*CallbackPtr)(sListenSocket, 
								sAcceptSocket, 
								lpOutputBuffer, 
								dwReceiveDataLength, 
								dwLocalAddressLength, 
								dwRemoteAddressLength,
								lpdwBytesReceived,
								lpOverlapped
								);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(sAcceptSocket), HANDLE_SOCKET, 
									lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)){
		goto Skip;
	}

	//
	// Allocate irp and save listen socket handle
	//

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_ACCEPT;
	Irp->CallType = _IoAcceptEx;
	Irp->SkListen = sListenSocket;
	Irp->RequestBytes = dwReceiveDataLength;
	IoCaptureStackTrace(Thread, Callback, Irp);

	//
	// Hijack with our overlapped, otherwise
	// mark this irp as synchronous, most likely it will fail since
	// AcceptEx enforce to be overlapped IO
	//

	Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	//
	// Call AcceptEx with our hijacked overlapped	
	//

	Status = (*CallbackPtr)(sListenSocket, 
							sAcceptSocket, 
							lpOutputBuffer, 
							dwReceiveDataLength, 
							dwLocalAddressLength, 
							dwRemoteAddressLength,
							lpdwBytesReceived,
							Overlapped
							);

	IoStatus = WSAGetLastError();
	if (!Status && IoStatus != WSA_IO_PENDING) {

		//
		// Because it failed, we complete this irp as synchronous one
		//

		Irp->RequestBytes = 0;
		Irp->CompleteBytes = 0;
		IoCompleteSynchronousIo(Irp, Status, IoStatus, NULL, NULL);

		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	} 

	Irp->LastError = IoStatus;

	//
	// In most cases, ERROR_IO_PENDING is returned
	//

	if (IoStatus == WSA_IO_PENDING) {
		IoIrpClearInCall(Irp);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	ASSERT(Status == TRUE);

	//
	// Retrieve socket pair addresses
	//

	(*IoGetAcceptExSockaddrs)(lpOutputBuffer, dwReceiveDataLength, 
							dwLocalAddressLength, dwRemoteAddressLength, 
							&Local, &LocalLength, &Remote, &RemoteLength);

	if (SkipOnSuccess) {

		RtlCopyMemory(&Object->u.Socket.Local, Local, LocalLength);
		SetFlag(Object->Flags, OF_LOCAL_VALID);
		RtlCopyMemory(&Object->u.Socket.Remote, Remote, RemoteLength);
		SetFlag(Object->Flags, OF_REMOTE_VALID);

		IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpdwBytesReceived, lpOverlapped);
		IoUnreferenceObject(Object);

	} else {
		IoIrpClearInCall(Irp);
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

BOOL WINAPI 
IoTransmitFile(
	SOCKET                  hSocket,
	HANDLE                  hFile,
	DWORD                   nNumberOfBytesToWrite,
	DWORD                   nNumberOfBytesPerSend,
	LPOVERLAPPED            lpOverlapped,
	LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
	DWORD                   dwFlags
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	TransmitFilePtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoTransmitFile);
	CallbackPtr = (TransmitFilePtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hSocket,
								hFile,
								nNumberOfBytesToWrite,
								nNumberOfBytesPerSend,
								lpOverlapped,
								lpTransmitBuffers,
								dwFlags
								);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(hSocket), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoTransmitFile;
	Irp->RequestBytes = nNumberOfBytesToWrite + 
						lpTransmitBuffers->HeadLength + 
						lpTransmitBuffers->TailLength;

	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = NULL;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(hSocket,
							hFile,
							nNumberOfBytesToWrite,
							nNumberOfBytesPerSend,
							Overlapped,
							lpTransmitBuffers,
							dwFlags
							);

	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (!IsOverlapped) {

		IoQuerySocketAddress(Object, hSocket);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, &Irp->RequestBytes, NULL);
		IoUnreferenceObject(Object);

	} else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			//IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, NULL, lpOverlapped);
			IoUnreferenceObject(Object);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;;
}

BOOL WINAPI 
IoTransmitPackets(
	SOCKET                     hSocket,
	LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,
	DWORD                      nElementCount,
	DWORD                      nSendSize,
	LPOVERLAPPED               lpOverlapped,
	DWORD                      dwFlags
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	TransmitPacketsPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoTransmitPackets);
	CallbackPtr = (TransmitPacketsPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hSocket,
								lpPacketArray,
								nElementCount,
								nSendSize,
								lpOverlapped,
								dwFlags
								);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(hSocket), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoTransmitPackets;
	Irp->RequestBytes = lpPacketArray->cLength;

	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = NULL;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(hSocket,
							lpPacketArray,
							nElementCount,
							nSendSize,
							Overlapped,
							dwFlags
							);	

	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (!IsOverlapped) {

		IoQuerySocketAddress(Object, hSocket);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, &Irp->RequestBytes, NULL);
		IoUnreferenceObject(Object);

	} else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, NULL, lpOverlapped);
			//IoUnreferenceObject(Object);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;;
}

BOOL WINAPI 
IoDisconnectEx(
	_In_ SOCKET       hSocket,
	_In_ LPOVERLAPPED lpOverlapped,
	_In_ DWORD        dwFlags,
	_In_ DWORD        reserved
	)
{
	return 0;
}

int WINAPI 
IoWSARecvMsg(
	_In_    SOCKET                             s,
	_Inout_ LPWSAMSG                           lpMsg,
	_Out_   LPDWORD                            lpdwNumberOfBytesRecvd,
	_In_    LPWSAOVERLAPPED                    lpOverlapped,
	_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSARecvMsgPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback; 
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSARecvMsg);
	CallbackPtr = (WSARecvMsgPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s,
							lpMsg,
							lpdwNumberOfBytesRecvd,
							lpOverlapped,
							lpCompletionRoutine
							);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	Size = 0;
	for(i = 0; i < lpMsg->dwBufferCount; i++) {
		Size += lpMsg->lpBuffers[i].len;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->CallType = _IoWSARecvMsg;
	Irp->RequestBytes = Size;
	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}
	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	} else {
		ApcCallback = lpCompletionRoutine;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(s,
							lpMsg,
							lpdwNumberOfBytesRecvd,
							Overlapped,
							ApcCallback
							);

	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}
	
	Irp->LastError = IoStatus;
	if (!IsOverlapped) {

		IoQuerySocketAddress(Object, s);
#ifdef _DEBUG
		IoDebugPrintSkAddress(Object);
#endif
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpdwNumberOfBytesRecvd, lpOverlapped);
		IoUnreferenceObject(Object);

	} else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpdwNumberOfBytesRecvd, lpOverlapped);
			IoUnreferenceObject(Object);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;;
}

int WINAPI 
IoWSASendMsg(
	_In_  SOCKET                             s,
	_In_  LPWSAMSG                           lpMsg,
	_In_  DWORD                              dwFlags,
	_Out_ LPDWORD                            lpNumberOfBytesSent,
	_In_  LPWSAOVERLAPPED                    lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	WSASendMsgPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback; 
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoWSASendMsg);
	CallbackPtr = (WSASendMsgPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s,
							lpMsg,
							dwFlags,
							lpNumberOfBytesSent,
							lpOverlapped,
							lpCompletionRoutine
							);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	Size = 0;
	for(i = 0; i < lpMsg->dwBufferCount; i++) {
		Size += lpMsg->lpBuffers[i].len;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoWSASendMsg;
	Irp->RequestBytes = Size;
	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}
	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	} else {
		ApcCallback = lpCompletionRoutine;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(s,
							lpMsg,
							dwFlags,
							lpNumberOfBytesSent,
							Overlapped,
							ApcCallback
							);

	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}
	
	Irp->LastError = IoStatus;
	if (!IsOverlapped) {

		IoQuerySocketAddress(Object, s);
#ifdef _DEBUG
		IoDebugPrintSkAddress(Object);
#endif
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
		IoUnreferenceObject(Object);

	} else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			//IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;;
}

BOOL WINAPI
IoConnectEx(
	_In_     SOCKET                s,
	_In_     const struct sockaddr *name,
	_In_     int                   namelen,
	_In_opt_ PVOID                 lpSendBuffer,
	_In_     DWORD                 dwSendDataLength,
	_Out_    LPDWORD               lpdwBytesSent,
	_In_     LPOVERLAPPED          lpOverlapped
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ConnectExPtr CallbackPtr;
	PIO_IRP Irp;
	DWORD IoStatus;
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	BOOLEAN SkipOnSuccess;

	Callback = IoGetCallback(_IoConnectEx);
	CallbackPtr = (ConnectExPtr)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread){

Skip:
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(s,
								name,
								namelen,
								lpSendBuffer,
								dwSendDataLength,
								lpdwBytesSent,
								lpOverlapped	
								);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess)) {
		goto Skip;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_CONNECT;
	Irp->CallType = _IoConnectEx;
	Irp->RequestBytes = dwSendDataLength;

	IoCaptureStackTrace(Thread, Callback, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	} else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = NULL;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	Status = (*CallbackPtr)(s,
							name,
							namelen,
							lpSendBuffer,
							dwSendDataLength,
							lpdwBytesSent,
							lpOverlapped	
							);

	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {
		
		IoUnreferenceObject(Object);
		WSASetLastError(IoStatus); 
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (!IsOverlapped) {

		IoQuerySocketAddress(Object, s);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpdwBytesSent, NULL);

	} else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, NULL, lpOverlapped);
			//IoUnreferenceObject(Object);
		} else {
			IoIrpClearInCall(Irp);
		}
	}

	WSASetLastError(IoStatus);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedDecrement(&Callback->References);
	return Status;;
}

PIO_OBJECT
IoAllocateSocketObject(
	_In_ HANDLE Handle
	)
{
	PIO_OBJECT Object;
	int Length;
	int Status;
	int Type = 0;

	//
	// First get socket type SOCK_STREAM OR SOCK_DGRAM
	//

	Length = sizeof(int);
	Status = getsockopt((SOCKET)Handle, SOL_SOCKET, SO_TYPE,(char*)&Type, &Length);
	if (Status == SOCKET_ERROR || Type != SOCK_STREAM || Type != SOCK_DGRAM){
		return NULL;
	}

	//
	// This is a new socket object, we allocate and insert into object table
	//

	Object = IoAllocateObject();
	Length = sizeof(Object->u.Socket.Local);
	Status = getsockname((SOCKET)Handle, (struct sockaddr *)&Object->u.Socket.Local, &Length); 

	//
	// We're only interested in IPV4/IPV6
	//

	if (Status != 0 || Object->u.Socket.Local.ss_family != AF_INET || 
		Object->u.Socket.Local.ss_family != AF_INET6) {
		IoFreeObject(Object);
		return NULL;
	}

	if (Object->u.Socket.Local.ss_family == AF_INET){
		Object->Flags |= OF_SKIPV4;
	}
	if (Object->u.Socket.Local.ss_family == AF_INET6){
		Object->Flags |= OF_SKIPV6;
	}

	if (Type == SOCK_STREAM) {
		Object->Flags |= OF_SKTCP;
	} else {
		Object->Flags |= OF_SKUDP;
	}

	Object->Flags |= OF_LOCAL_VALID;

	//
	// Insert object and increase its reference, we must increase reference since
	// the following region will unreference it.
	//

	IoInsertObject(Object);
	return Object;
}

PIO_OBJECT
IoAllocateFileObject(
	_In_ HANDLE Handle,
	_In_ BOOLEAN Overlapped
	)
{
	PIO_OBJECT Object;
	DWORD Size;
	WCHAR Path[MAX_PATH];

	//
	// Size is number of WCHAR without terminated NULL
	//

	Size = GetFinalPathNameByHandleW(Handle, Path, MAX_PATH, VOLUME_NAME_DOS|FILE_NAME_NORMALIZED);
	if (!Size){
		return NULL;
	}

	Object = IoAllocateObject();
	Object->Id = IoAcquireObjectId();
	Object->Object = Handle;
	Object->Type = HANDLE_FILE;
	Object->Flags = OF_FILE;

	if (Overlapped) {
		SetFlag(Object->Flags, OF_OVERLAPPED);
	}

	Object->u.File.Name = (PWSTR)BtrMalloc(Size + sizeof(WCHAR));
	Object->u.File.Name[Size] = 0;
	Object->u.File.Length = Size + 1;

	if (Size <= MAX_PATH){
		RtlCopyMemory(Object->u.File.Name, Path, Size);
	}
	else {
		Size = GetFinalPathNameByHandleW(Handle, Object->u.File.Name, 
						Size, VOLUME_NAME_DOS|FILE_NAME_NORMALIZED);
		ASSERT(Size == Object->u.File.Length - 1);
	}

	GetSystemTimeAsFileTime(&Object->Start);
	IoInsertObject(Object);
	return Object;
}