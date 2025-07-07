//
// Author: xwlan@outlook.com
// Copyright(C) 2009-2025
//

#include "apsbtr.h"
#include "lock.h"
#include "thread.h"
#include "hal.h"
#include "stacktrace.h"
#include "util.h"
#include "iatpatch.h"
#include "ioiat.h"
#include "ioprof.h"
#include "heap.h"
#include <winsock2.h>
#include <ws2tcpip.h>

//
// The following order must align with definitions in enum IO_PATCH
//

BTR_IAT_PATCH IoPatch[] = {
	{ "kernel32.dll", "ExitProcess", IoExitProcessCallback },
	{ "kernel32.dll", "CreateFileW", IatCreateFile, },
	{ "kernel32.dll", "CloseHandle", IatCloseHandle, },
	{ "kernel32.dll", "ReadFile", IatReadFile, },
	{ "kernel32.dll", "WriteFile", IatWriteFile, },
	{ "kernel32.dll", "GetOverlappedResult", IatGetOverlappedResult, },
	{ "kernel32.dll", "GetQueuedCompletionStatus", IatGetQueuedCompletionStatus, },
	{ "kernel32.dll", "GetQueuedCompletionStatusEx", IatGetQueuedCompletionStatusEx, },  // NT 6+
	{ "kernel32.dll", "PostQueuedCompletionStatus", IatPostQueuedCompletionStatus, },  
	{ "ws2_32.dll", "accept", IatAccept },
	{ "ws2_32.dll", "recv", IatRecv },
	{ "ws2_32.dll", "send", IatSend },
	{ "ws2_32.dll", "closesocket", IatCloseSocket },
	{ "ws2_32.dll", "WSAAccept", IatWSAAccept },
	{ "ws2_32.dll", "WSARecv", IatWSARecv },
	{ "ws2_32.dll", "WSASend", IatWSASend },
	{ "ws2_32.dll", "WSAGetOverlappedResult", IatWSAGetOverlappedResult },
	{ "mswsock.dll", "AcceptEx", IatAcceptEx },
	{ "mswsock.dll", "TransmitFile", IatTransmitFile }
};

ULONG IoPatchCount = ARRAYSIZE(IoPatch);

PBTR_IAT_PATCH FORCEINLINE
IoGetPatch(
	IN IO_PATCH Ordinal
	)
{
	return &IoPatch[Ordinal];
}

VOID
IoIatInitialize(
	VOID
	)
{
	ULONG i;
	HMODULE Handle;

	for (i = 0; i < IoPatchCount; i++) {
		Handle = GetModuleHandleA(IoPatch[i].From);
		ASSERT(Handle != NULL);
		IoPatch[i].Address = GetProcAddress(Handle, IoPatch[i].Function);
	}
}

VOID
IoIatApplyPatch(
	VOID
	)
{
	IoIatInitialize();
	BtrApplyIatPatch(IoPatch, IoPatchCount, TRUE);
}

VOID
IoDllLoadCallback(
	IN struct _BTR_PROFILE_OBJECT* Object,
	IN struct _BTR_MODULE* Dll,
	IN BOOLEAN Load
	)
{
	if (Load) {
		BtrApplyPatchForDllByAddress(Dll, IoPatch, IoPatchCount, TRUE);
		return;
	}
}

//
// IAT mode don't use old inline callback, replace with _A
// with address of target function
// _T, Thread object 
// _A, target function
// _I, IRP object
//

#define IoCaptureStackTrace(_T, _A, _I)\
{\
	ULONG Depth;\
	ULONG_PTR *Pc;\
	Pc = (ULONG_PTR *)_T->Buffer;\
	Pc[0] = (ULONG_PTR)CALLER;\
	Pc[1] = (ULONG_PTR)_I->RequestBytes;\
	Pc[2] = 0;\
	BtrCaptureStackTraceEx((PVOID *)_T->Buffer,\
							MAX_STACK_DEPTH, BtrGetFramePointer(),\
		                   _A, &_I->StackId, &Depth);\
}

HANDLE WINAPI
IatCreateFile(
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
	HANDLE File;
	PIO_OBJECT Object;
	ULONG Length;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		File = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
							dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		return File;
	}

	BtrEnterExemptionRegion(Thread);

	File = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
						dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if (File == INVALID_HANDLE_VALUE) {
		BtrLeaveExemptionRegion(Thread);
		return INVALID_HANDLE_VALUE;
	}

	Object = IoAllocateObject();
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
	BtrLeaveExemptionRegion(Thread);
	return File;
}

BOOL WINAPI
IatCloseHandle(
	_In_ HANDLE Handle
	)
{
	PBTR_THREAD_OBJECT Thread;
	BOOL Status;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		Status = CloseHandle(Handle);
		return Status;
	}

	BtrEnterExemptionRegion(Thread);
	Status = CloseHandle(Handle);
	if (!Status) {
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	//
	// Reconsider mark the object as deleted, not
	// removed from object table, it's possible that
	// after the object is closed, there's still pending
	// or failed IO request need process by runtime
	//

	IoRemoveObjectByHandleEx(Handle, HANDLE_FILE);

	SetLastError(ERROR_SUCCESS);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

//
// N.B. Synchronous IO can use lpOverlapped too.
//

BOOL WINAPI
IatReadFile(
	_In_        HANDLE       hFile,
	_Out_       LPVOID       lpBuffer,
	_In_        DWORD        nNumberOfBytesToRead,
	_Out_opt_   LPDWORD      lpNumberOfBytesRead,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_IAT_PATCH Patch;
	DWORD IoStatus;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	BOOLEAN SkipOnSuccess;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
Skip:
		Status = ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
		return Status;
	}
	
	BtrEnterExemptionRegion(Thread);

	//
	// Check overlapped and allocate IO object if required
	//

	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		BtrLeaveExemptionRegion(Thread);
		goto Skip;
	}

	//
	// Allocate and fill IO request
	//

	Irp = IoAllocateIrp(Object);
	if (!Irp) {
		BtrLeaveExemptionRegion(Thread);
		goto Skip;
	}

	Irp->Operation = IO_OP_READ;
	Irp->RequestBytes = nNumberOfBytesToRead;
	IoUnreferenceObject(Object);

	Patch = IoGetPatch(_IatReadFile);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	}
	else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	//
	// Queue IO request to IO object before issue the IO call
	//

	IoQueueIrpToObject(Object, Irp);

	Status = ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, Overlapped);
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
			IoCompleteSkipOnSuccess(Irp, Status, IoStatus, lpNumberOfBytesRead, lpOverlapped);
		}
	}

	SetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}


BOOL WINAPI
IatWriteFile(
	_In_        HANDLE       hFile,
	_Out_       LPVOID       lpBuffer,
	_In_        DWORD        nNumberOfBytesToWrite,
	_Out_opt_   LPDWORD      lpNumberOfBytesWrite,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PBTR_IAT_PATCH Patch;
	DWORD IoStatus;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	BOOLEAN SkipOnSuccess;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
Skip:
		Status = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWrite, lpOverlapped);
		return Status;
	}
	
	BtrEnterExemptionRegion(Thread);
	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		BtrLeaveExemptionRegion(Thread);
		goto Skip;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_WRITE;
	Irp->RequestBytes = nNumberOfBytesToWrite;
	IoUnreferenceObject(Object);

	Patch = IoGetPatch(_IatWriteFile);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	}
	else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = lpOverlapped;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	//
	// Queue IO request to IO object before issue the IO call
	//

	IoQueueIrpToObject(Object, Irp);

	Status = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWrite, Overlapped);
	IoStatus = GetLastError();
	Irp->LastError = IoStatus;

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesWrite, lpOverlapped);
	}
	else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			IoCompleteSkipOnSuccess(Irp, Status, IoStatus, lpNumberOfBytesWrite, lpOverlapped);
		}
	}

	SetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
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
// HasOverlappedIoCompleted() macro compare Overlapped.Internal with 
// STATUS_PENDING, if it's not STATUS_PENDING, it return TRUE, that's
// to say, since the system set STATUS_PENDING to Internal field after
// the overlapped IO is issued, if it's changed then it's completed
// whatever it's successful or failed. 
//

BOOL WINAPI
IatGetOverlappedResult(
	_In_  HANDLE       hFile,
	_In_  LPOVERLAPPED lpOverlapped,
	_Out_ LPDWORD      lpNumberOfBytesTransferred,
	_In_  BOOL         bWait
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	DWORD IoStatus;
	PIO_IRP Irp;

	Irp = IoGetIrpFromInternal(lpOverlapped);
	if (!Irp) {
		Status = GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
		return Status;
	}

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Status = GetOverlappedResult(hFile, &Irp->Overlapped, lpNumberOfBytesTransferred, bWait);
	IoStatus = GetLastError();

	if (IoIsOverlappedChanged(Irp)) {

		//
		// Consider log an entry for debugging
		//
	}

	//
	// Copy our overlapped to user provided to duplicate the current
	// IO status, we need re-attach again our IRP pointer to user
	// provided overlapped if required
	//

	IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);

	//
	// If the IO is pending, user may retry, we need
	// re-attach our IRP pointer to user overlapped,
	// this assume system won't write to InternalHigh
	//

	if (!Status && IoStatus == ERROR_IO_INCOMPLETE) {
		IoAttachIrpToOverlapped(lpOverlapped, Irp);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	IoCompleteOverlappedIo(Irp, Status, IoStatus, lpNumberOfBytesTransferred, lpOverlapped);

	SetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

//
// If a call to GetQueuedCompletionStatus fails because the completion port handle associated with it is closed while the 
// call is outstanding, the function returns FALSE, *lpOverlapped will be NULL, and GetLastError will return ERROR_ABANDONED_WAIT_0.
// 
// Even if you have passed the function a file handle associated with a completion port and a valid OVERLAPPED structure, 
// an application can prevent completion port notification.This is done by specifying a valid event handle for the hEvent 
// member of the OVERLAPPED structure, and setting its low-order bit. A valid event handle whose low - order bit is set 
// prevents the completion of the overlapped I/O from enqueing a completion packet to the completion port.
//

BOOL WINAPI
IatGetQueuedCompletionStatus(
	_In_  HANDLE       CompletionPort,
	_Out_ LPDWORD      lpNumberOfBytes,
	_Out_ PULONG_PTR   lpCompletionKey,
	_Out_ LPOVERLAPPED* lpOverlapped,
	_In_  DWORD        dwMilliseconds
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	DWORD IoStatus;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	PIO_OBJECT Object;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		Status = GetQueuedCompletionStatus(CompletionPort, lpNumberOfBytes, lpCompletionKey, lpOverlapped, dwMilliseconds);
		return Status;
	}

	BtrEnterExemptionRegion(Thread);

	Status = GetQueuedCompletionStatus(CompletionPort, lpNumberOfBytes, lpCompletionKey, lpOverlapped, dwMilliseconds);
	if (!Status) {
		if (!*lpOverlapped) {

			//
			// No packet dequeued
			//

			BtrLeaveExemptionRegion(Thread);
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
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Irp = IoOverlappedToIrp(*lpOverlapped);
	if (!Irp) {
		SetLastError(IoStatus);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	QueryPerformanceCounter(&Irp->End);
	if (IoIsOverlappedChanged(Irp)) {

		//
		// Consider write a log entry for debugging
		//
	}

	//
	// Copy original user's lpOverlapped and io status
	//

	*lpOverlapped = Irp->Original;
	IoCopyOverlapped(&Irp->Overlapped, Irp->Original);

	Irp->IoStatus = IoGetCompletionStatus(Irp);
	Irp->CompleteBytes = IoGetCompletionSize(Irp);
	Irp->CompleteThreadId = GetCurrentThreadId();
	Irp->Flags.Completed = 1;

	Object = Irp->IoObject;
	ASSERT(Object != NULL);

	if (Irp->Flags.Socket) {

		//
		// If it's an overlapped AcceptEx operation, first update
		// the accept socket's context 
		//

		if (Irp->Operation == IO_OP_ACCEPT) {
			IoSocketUpdateAcceptContext(Irp);
		}

		IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
	}

	if (!FlagOn(Object->Flags, OF_IOCPASSOCIATE)) {
		SetFlag(Object->Flags, OF_IOCPASSOCIATE);
	}

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueCompletedIrp(Irp);

	SetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

//
// N.B. This routine is used to filter out all non-IO completion request,
// many applications may use it as a queue mechanism to dispatch workloads 
//

BOOL WINAPI
IatPostQueuedCompletionStatus(
	_In_     HANDLE       CompletionPort,
	_In_     DWORD        dwNumberOfBytesTransferred,
	_In_     ULONG_PTR    dwCompletionKey,
	_In_opt_ LPOVERLAPPED lpOverlapped
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IOCP_PACKET Packet;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		Status = PostQueuedCompletionStatus(CompletionPort, dwNumberOfBytesTransferred, 
											dwCompletionKey, lpOverlapped);
		return Status;
	}

	BtrEnterExemptionRegion(Thread);

	Packet = IoAllocateIocpPacket();
	Packet->Overlapped = lpOverlapped;

	//
	// N.B. Replace the original lpOverlapped with our packet, in GetQueuedCompletionStatus,
	// we can determine whether it's a IO completion by scan of our overlapped list, since
	// the kernel don't touch the lpOverlapped parameter, it's safe.
	//

	Status = PostQueuedCompletionStatus(CompletionPort, dwNumberOfBytesTransferred, 
										dwCompletionKey, (LPOVERLAPPED)Packet);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

//
// 
// This function returns TRUE when at least one pending I/O is completed, but it is possible that one or more I/O 
// operations failed. Note that it is up to the user of this function to check the list of returned entries in the 
// lpCompletionPortEntries parameter to determine which of them correspond to any possible failed I/O operations 
// by looking at the status contained in the lpOverlapped member in each OVERLAPPED_ENTRY.
// 
// 
// This function returns FALSE when no I / O operation was dequeued.This typically means that an error occurred 
// while processing the parameters to this call, or that the CompletionPort handle was closed or is otherwise invalid.
// The GetLastError function provides extended error information.
//
// If a call to GetQueuedCompletionStatusEx fails because the handle associated with it is closed, the function returns
// FALSE and GetLastError will return ERROR_ABANDONED_WAIT_0.
//
//

BOOL WINAPI
IatGetQueuedCompletionStatusEx(
	_In_  HANDLE             CompletionPort,
	_Out_ LPOVERLAPPED_ENTRY lpCompletionPortEntries,
	_In_  ULONG              ulCount,
	_Out_ PULONG             ulNumEntriesRemoved,
	_In_  DWORD              dwMilliseconds,
	_In_  BOOL               fAlertable
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	LPOVERLAPPED Overlapped;
	PIO_OBJECT Object;
	ULONG Number;
	LARGE_INTEGER End;

	Thread = BtrIsExemptedCall(CALLER);
	if (!Thread) {
		Status = GetQueuedCompletionStatusEx(CompletionPort, lpCompletionPortEntries, ulCount, 
											ulNumEntriesRemoved, dwMilliseconds, fAlertable);
		return Status;
	}

	BtrEnterExemptionRegion(Thread);

	Status = GetQueuedCompletionStatusEx(CompletionPort, lpCompletionPortEntries, ulCount, 
										ulNumEntriesRemoved, dwMilliseconds, fAlertable);
	if (!Status) {
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	QueryPerformanceCounter(&End);

	for (Number = 0; Number < *ulNumEntriesRemoved; Number += 1) {

		Overlapped = lpCompletionPortEntries[Number].lpOverlapped;

		//
		// Skip the posted IOCP packet
		//

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

		//
		// Check whether it's our hijacked overlapped
		//

		Irp = IoOverlappedToIrp(Overlapped);
		if (!Irp) {
			continue;
		}

		if (IoIsOverlappedChanged(Irp)) {

			//
			// Consider write a log entry for debugging
			//

		}

		Object = Irp->IoObject;
		ASSERT(Object != NULL);

		Overlapped = Irp->Original;
		IoCopyOverlapped(&Irp->Overlapped, Overlapped);

		Irp->IoStatus = IoGetCompletionStatus(Irp);
		Irp->CompleteBytes = IoGetCompletionSize(Irp);
		Irp->End.QuadPart = End.QuadPart;
		Irp->CompleteThreadId = GetCurrentThreadId();
		Irp->Flags.Completed = 1;

		if (Irp->Flags.Socket) {

			//
			// If it's an overlapped AcceptEx operation, first update
			// the accept socket's context 
			//

			if (Irp->Operation == IO_OP_ACCEPT) {
				IoSocketUpdateAcceptContext(Irp);
			}

			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
		}

		IoUpdateRequestCounters(Irp);
		IoUpdateCompleteCounters(Irp);
		IoQueueCompletedIrp(Irp);

		//
		// fix the overlapped entry with user's original one
		//

		lpCompletionPortEntries[Number].lpOverlapped = Overlapped;
	}

	BtrLeaveExemptionRegion(Thread);
	return Status;
}

//
// If WSAGetOverlappedResult returns FALSE, this means that either the overlapped operation has not completed, 
// the overlapped operation completed but with errors, or the overlapped operation's completion status could 
// not be determined due to errors in one or more parameters to WSAGetOverlappedResult. On failure, the value 
// pointed to by lpcbTransfer will not be updated. Use WSAGetLastError to determine the cause of the failure
//
// WSA_IO_INCOMPLETE, WSAGetOverlappedResult
// WSA_IO_PENDING, WSASend/WSARecv etc
//

BOOL WINAPI
IatWSAGetOverlappedResult(
	_In_  SOCKET          s,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_Out_ LPDWORD         lpcbTransfer,
	_In_  BOOL            fWait,
	_Out_ LPDWORD         lpdwFlags
	)
{
	BOOL Status;
	PBTR_THREAD_OBJECT Thread;
	DWORD IoStatus;
	PIO_IRP Irp;

	Irp = IoGetIrpFromInternal(lpOverlapped);
	if (!Irp) {
		Status = WSAGetOverlappedResult(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
		return Status;
	}

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Status = WSAGetOverlappedResult(s, &Irp->Overlapped, lpcbTransfer, fWait, lpdwFlags);
	IoStatus = GetLastError();

	if (IoIsOverlappedChanged(Irp)) {

		//
		// Consider log an entry for debugging
		//
	}

	//
	// Copy our overlapped to user provided to duplicate the current
	// IO status, we need re-attach again our IRP pointer to user
	// provided overlapped if required
	//

	IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);

	//
	// If the IO is pending, user may retry, we need
	// re-attach our IRP pointer to user overlapped,
	// this assume system won't write to InternalHigh
	//

	if (!Status && IoStatus == WSA_IO_INCOMPLETE) {
		IoAttachIrpToOverlapped(lpOverlapped, Irp);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	IoCompleteOverlappedIo(Irp, Status, IoStatus, lpcbTransfer, lpOverlapped);

	SetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI
IatWSASend(
	_In_  SOCKET s,
	_In_  LPWSABUF lpBuffers,
	_In_  DWORD dwBufferCount,
	_Out_ LPDWORD lpNumberOfBytesSent,
	_In_  DWORD dwFlags,
	_In_  LPWSAOVERLAPPED lpOverlapped,
	_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback;
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		Status = WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	//
	// Accumulate all buffer length
	//

	Size = 0;
	for (i = 0; i < dwBufferCount; i++) {
		Size += lpBuffers[i].len;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoWSASend;
	Irp->RequestBytes = Size;

	Patch = IoGetPatch(_IatWSASend);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	if (IsOverlapped) {
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	}
	else {
		IoMarkIrpSynchronous(Irp);
		Overlapped = NULL;
	}
	if (IsOverlapped && lpCompletionRoutine) {
		ApcCallback = IoNetCompleteCallback;
		Irp->ApcCallback = lpCompletionRoutine;
	}
	else {
		ApcCallback = NULL;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	IoQueueIrpToObject(Object, Irp);

	Status = WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, Overlapped, ApcCallback);
	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;

	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {

		IoDequeueIrpFromObject(Object, Irp);
		IoUpdateFailedCounters(Irp);
		IoFreeIrp(Irp);

		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
	}
	else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != WSA_IO_PENDING) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
		}
	}

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;;
}

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
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		Status = WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iToLen, lpOverlapped, lpCompletionRoutine);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Size = 0;
	for (i = 0; i < dwBufferCount; i++)
		Size += lpBuffers[i].len;

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->CallType = _IoWSASendTo;
	Irp->RequestBytes = Size;

	Patch = IoGetPatch(_IatWSASendTo);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

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
	}
	else {
		ApcCallback = NULL;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	IoQueueIrpToObject(Object, Irp);

	Status = WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, 
						lpTo, iToLen, lpOverlapped, lpCompletionRoutine);

	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;

	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {

		IoDequeueIrpFromObject(Object, Irp);
		IoUpdateFailedCounters(Irp);
		IoFreeIrp(Irp);

		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
	}
	else {

		//
		// If it's IOCP associated and SkipOnSuccess, and I/O is completed (success or failure),
		// complete this I/O as synchronous since we won't get a notification from IOCP.
		//

		if (SkipOnSuccess && IoStatus != ERROR_IO_PENDING) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesSent, lpOverlapped);
		}
	}

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI 
IatWSARecv(
	_In_    SOCKET s,
	_Inout_ LPWSABUF lpBuffers,
	_In_    DWORD dwBufferCount,
	_Out_   LPDWORD lpNumberOfBytesRecvd,
	_Inout_ LPDWORD lpFlags,
	_In_    LPWSAOVERLAPPED lpOverlapped,
	_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback;
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		Status = WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Size = 0;
	for (i = 0; i < dwBufferCount; i++) {
		Size += lpBuffers[i].len;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->CallType = _IoWSARecv;
	Irp->RequestBytes = Size;

	Patch = IoGetPatch(_IatWSARecv);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

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
	}
	else {
		ApcCallback = lpCompletionRoutine;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	IoQueueIrpToObject(Object, Irp);

	Status = WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, Overlapped, ApcCallback);
	IoStatus = WSAGetLastError();

	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {

		IoDequeueIrpFromObject(Object, Irp);
		IoUpdateFailedCounters(Irp);
		IoFreeIrp(Irp);

		WSASetLastError(IoStatus);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Irp->LastError = IoStatus;
	if (!IsOverlapped) {
		IoQuerySocketAddress(Object, s);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
	}
	else {
		if (SkipOnSuccess && IoStatus == ERROR_SUCCESS) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
		}
	}

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;;
}

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
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	DWORD IoStatus;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE ApcCallback;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LPOVERLAPPED Overlapped;
	ULONG i, Size;
	BOOLEAN SkipOnSuccess;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		Status = WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags,
							lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Size = 0;
	for (i = 0; i < dwBufferCount; i++) {
		Size += lpBuffers[i].len;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->CallType = _IoWSARecvFrom;
	Irp->RequestBytes = Size;

	Patch = IoGetPatch(_IatWSARecvFrom);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

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
	}
	else {
		ApcCallback = NULL;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	IoQueueIrpToObject(Object, Irp);

	Status = WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, 
						lpFrom, lpFromlen, Overlapped, ApcCallback);
	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;

	if (Status == SOCKET_ERROR && IoStatus != WSA_IO_PENDING) {

		IoDequeueIrpFromObject(Object, Irp);
		IoUpdateFailedCounters(Irp);
		IoFreeIrp(Irp);

		WSASetLastError(IoStatus);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
	}
	else {

		if (SkipOnSuccess && IoStatus != WSA_IO_PENDING) {
			ASSERT(HalQuerySkipOnSuccess(SK_HANDLE(s)));
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesRecvd, lpOverlapped);
		}
	}

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI
IatCloseSocket(
	_In_ SOCKET s
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	ULONG IoStatus;
	PIO_OBJECT Object;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Object = IoLookupObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		Status = closesocket(s);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	//
	// Before close handle, query its address
	//

	IoQuerySocketAddress(Object, s);

	Status = closesocket(s);
	IoStatus = WSAGetLastError();

	IoRemoveObjectByHandleEx(SK_HANDLE(s), HANDLE_SOCKET);

	BtrLeaveExemptionRegion(Thread);
	WSASetLastError(IoStatus);
	return Status;
}

SOCKET WINAPI
IatWSAAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ LPINT addrlen,
	_In_  LPCONDITIONPROC lpfnCondition,
	_In_  DWORD dwCallbackData
	)
{
	PBTR_THREAD_OBJECT Thread;
	PIO_OBJECT Object;
	PIO_OBJECT Accepted;
	SOCKET Status;
	ULONG IoStatus;
	int Length;
	SOCKADDR_STORAGE Address;
	PIO_IRP Irp;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Object = IoGetObjectByHandle(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		Status = WSAAccept(s, addr, addrlen, lpfnCondition, dwCallbackData);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Length = sizeof(SOCKADDR_STORAGE);
	Status = WSAAccept(s, (struct sockaddr*)&Address, &Length, lpfnCondition, dwCallbackData);

	IoStatus = WSAGetLastError();
	if (Status == INVALID_SOCKET) {
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Accepted = IoAllocateObject();
	Accepted->Object = SK_HANDLE(Status);
	Accepted->Type = HANDLE_SOCKET;

	if (FlagOn(Object->Flags, OF_SKTCP)) {
		Accepted->Flags |= OF_SKTCP;
	}
	if (FlagOn(Object->Flags, OF_SKUDP)) {
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

		ULONG AddressLength = SOCKET_ADDRESS_LIMIT;

		//
		// Fill local and remote address
		//

		Accepted->Flags |= OF_REMOTE_VALID;
		WSAAddressToStringA((LPSOCKADDR)&Address, Length, NULL, &Accepted->u.Socket.Remote[0], &AddressLength);

		Accepted->Flags |= OF_LOCAL_VALID;
		StringCchCopyA(&Accepted->u.Socket.Local[0], SOCKET_ADDRESS_LIMIT, &Object->u.Socket.Local[0]);

		//
		// Fill local and remote port
		//

		Accepted->u.Socket.LocalPort = Object->u.Socket.LocalPort;
		if (FlagOn(Object->Flags, OF_SKIPV4)) {
			Accepted->u.Socket.RemotePort = ntohs(((struct sockaddr_in*)&Address)->sin_port);
		}
		else {
			Accepted->u.Socket.RemotePort = ntohs(((struct sockaddr_in6*)&Address)->sin6_port);
		}
	}

	if (addr && addrlen) {
		RtlCopyMemory(addr, &Address, (*addrlen >= Length) ? Length : *addrlen);
	}

	IoInsertObject(Accepted);

	//
	// Craft an accept IRP and complete it
	//

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_ACCEPT;
	Irp->IoStatus = ERROR_SUCCESS;
	Irp->Flags.Queued = FALSE;

	IoUpdateCompleteCounters(Irp);
	IoQueueCompletedIrp(Irp);

	BtrLeaveExemptionRegion(Thread);
	WSASetLastError(IoStatus);
	return Status;
}

SOCKET WINAPI
IatAccept(
	_In_  SOCKET s,
	_Out_ struct sockaddr* addr,
	_Out_ int* addrlen
	)
{
	PBTR_THREAD_OBJECT Thread;
	PIO_OBJECT Object;
	PIO_OBJECT Accepted;
	SOCKET Status;
	ULONG IoStatus;
	int Length;
	SOCKADDR_STORAGE Address;
	PIO_IRP Irp;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Object = IoGetObjectByHandle(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		Status = accept(s, addr, addrlen);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Length = sizeof(SOCKADDR_STORAGE);
	Status = accept(s, (struct sockaddr*)&Address, &Length);

	IoStatus = WSAGetLastError();
	if (Status == INVALID_SOCKET) {
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Accepted = IoAllocateObject();
	Accepted->Object = SK_HANDLE(Status);
	Accepted->Type = HANDLE_SOCKET;

	if (FlagOn(Object->Flags, OF_SKTCP)) {
		Accepted->Flags |= OF_SKTCP;
	}
	if (FlagOn(Object->Flags, OF_SKUDP)) {
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

		ULONG AddressLength = SOCKET_ADDRESS_LIMIT;

		//
		// Fill local and remote address
		//

		Accepted->Flags |= OF_REMOTE_VALID;
		WSAAddressToStringA((LPSOCKADDR)&Address, Length, NULL, &Accepted->u.Socket.Remote[0], &AddressLength);

		Accepted->Flags |= OF_LOCAL_VALID;
		StringCchCopyA(&Accepted->u.Socket.Local[0], SOCKET_ADDRESS_LIMIT, &Object->u.Socket.Local[0]);

		//
		// Fill local and remote port
		//

		Accepted->u.Socket.LocalPort = Object->u.Socket.LocalPort;
		if (FlagOn(Object->Flags, OF_SKIPV4)) {
			Accepted->u.Socket.RemotePort = ((struct sockaddr_in*)&Address)->sin_port;
		}
		else {
			Accepted->u.Socket.RemotePort = ((struct sockaddr_in6*)&Address)->sin6_port;
		}
	}

	if (addr && addrlen) {
		RtlCopyMemory(addr, &Address, (*addrlen >= Length) ? Length : *addrlen);
	}

	IoInsertObject(Accepted);

	//
	// Craft an accept IRP and complete it
	//

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_ACCEPT;
	Irp->IoStatus = ERROR_SUCCESS;
	Irp->Flags.Queued = FALSE;

	IoUpdateCompleteCounters(Irp);
	IoQueueCompletedIrp(Irp);

	BtrLeaveExemptionRegion(Thread);
	WSASetLastError(IoStatus);
	return Status;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PIO_OBJECT Object;
	BOOL Status;
	ULONG IoStatus;
	PIO_IRP Irp;
	BOOLEAN IsOverlapped;
	BOOLEAN SkipOnSuccess;
	LPOVERLAPPED Overlapped;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(sAcceptSocket), HANDLE_SOCKET,
		lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {

		Status = AcceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength,
							dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);

		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	//
	// Allocate irp and save listen socket handle
	//

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_ACCEPT;
	Irp->CallType = _IoAcceptEx;
	Irp->SkListen = sListenSocket;
	Irp->RequestBytes = dwReceiveDataLength;
	Irp->AcceptSocket = sAcceptSocket;

	Patch = IoGetPatch(_IatAcceptEx);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	//
	// Hijack with our overlapped, otherwise
	// mark this irp as synchronous, most likely it will fail since
	// AcceptEx enforce to be overlapped IO
	//

	Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	IoQueueIrpToObject(Object, Irp);

	//
	// Call AcceptEx with our hijacked overlapped	
	//

	Status = AcceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength,
						dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, 
						Overlapped);

	IoStatus = WSAGetLastError();
	if (!Status && IoStatus != WSA_IO_PENDING) {

		//
		// Because it failed, we complete this irp as synchronous one
		//

		Irp->RequestBytes = 0;
		Irp->CompleteBytes = 0;
		IoCompleteSynchronousIo(Irp, Status, IoStatus, NULL, NULL);

		WSASetLastError(IoStatus);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Irp->LastError = IoStatus;

	//
	// In most cases, WSA_IO_PENDING is returned
	//

	if (IoStatus == WSA_IO_PENDING) {
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	ASSERT(Status == TRUE);

	//
	// Update accept socket's context, note that we don't call GetAcceptExSockaddrs()
	// here, we only need get the address information when user's code call send/recv
	// etc IO method to send/recv data, the accept socket's context is updated so
	// getsockname() and getpeername() can successfully return desired address information.
	//

	if (!IsOverlapped) {
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpdwBytesReceived, lpOverlapped);
		IoSocketUpdateAcceptContext(Irp);
	}

	if (SkipOnSuccess && IsOverlapped) {
		IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, lpdwBytesReceived, lpOverlapped);
		IoSocketUpdateAcceptContext(Irp);
	}

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI
IatRecv(
	_In_ SOCKET s,
	_Out_ char* buf,
	_In_ int len,
	_In_ int flags
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	LARGE_INTEGER Start;
	FILETIME Time;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Object = IoGetObjectByHandle(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		Status = recv(s, buf, len, flags);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = recv(s, buf, len, flags);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		IoUpdateFailedCountersEx(HANDLE_SOCKET, IO_OP_READ);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = TRUE;
	Irp->Flags.Queued = FALSE;

	Patch = IoGetPatch(_IatRecv);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	IoQuerySocketAddress(Object, s);
	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI 
IatRecvFrom(
	_In_  SOCKET s,
	_Out_ char* buf,
	_In_  int len,
	_In_  int flags,
	_Out_ struct sockaddr* from,
	_Inout_opt_ int* fromlen
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	FILETIME Time;
	BOOLEAN SkipOnSuccess;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, NULL, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		Status = recvfrom(s, buf, len, flags, from, fromlen);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = recvfrom(s, buf, len, flags, from, fromlen);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		IoUpdateFailedCountersEx(HANDLE_SOCKET, IO_OP_READ);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	QueryPerformanceCounter(&End);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_RECV;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = TRUE;
	Irp->Flags.Queued = FALSE;
	Irp->Start = Start;
	Irp->End = End;

	Patch = IoGetPatch(_IatRecvFrom);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI 
IatSend(
	_In_ SOCKET s,
	_In_ const char* buf,
	_In_ int len,
	_In_ int flags
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	FILETIME Time;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Object = IoGetObjectByHandle(SK_HANDLE(s), HANDLE_SOCKET);
	if (!Object) {
		Status = send(s, buf, len, flags);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = send(s, buf, len, flags);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		IoUpdateFailedCountersEx(HANDLE_SOCKET, IO_OP_READ);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	QueryPerformanceCounter(&End);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = TRUE;
	Irp->Flags.Queued = FALSE;
	Irp->Start = Start;
	Irp->End = End;

	Patch = IoGetPatch(_IatRecvFrom);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	IoQuerySocketAddress(Object, s);
	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

int WINAPI 
IatSendTo(
	_In_  SOCKET s,
	_In_ const char* buf,
	_In_ int len,
	_In_ int flags,
	_In_ const struct sockaddr* to,
	_In_ int tolen
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	ULONG IoStatus;
	PIO_OBJECT Object;
	BOOLEAN IsOverlapped;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	FILETIME Time;
	BOOLEAN SkipOnSuccess;
	PBTR_IAT_PATCH Patch;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	if (!IoRefObjectCheckOverlapped(SK_HANDLE(s), HANDLE_SOCKET, NULL, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		Status = sendto(s, buf, len, flags, to, tolen);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	GetSystemTimeAsFileTime(&Time);
	QueryPerformanceCounter(&Start);

	Status = sendto(s, buf, len, flags, to, tolen);
	IoStatus = WSAGetLastError();
	if (Status == SOCKET_ERROR) {
		IoUpdateFailedCountersEx(HANDLE_SOCKET, IO_OP_WRITE);
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	QueryPerformanceCounter(&End);

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_SEND;
	Irp->RequestBytes = len;
	Irp->CompleteBytes = Status;
	Irp->CompleteThreadId = Irp->RequestThreadId;
	Irp->Flags.Completed = 1;

	Patch = IoGetPatch(_IatSendTo);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	IoCompleteSynchronousIo(Irp, 0, 0, NULL, NULL);

	WSASetLastError(IoStatus);
	BtrLeaveExemptionRegion(Thread);
	return Status;
}

BOOL WINAPI
IatTransmitFile(
	IN SOCKET hSocket,
	IN HANDLE hFile,
	DWORD nNumberOfBytesToWrite,
	DWORD nNumberOfBytesPerSend,
	LPOVERLAPPED lpOverlapped,
	LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
	DWORD dwFlags
	)
{
	int Status;
	PBTR_THREAD_OBJECT Thread;
	PIO_IRP Irp;
	DWORD IoStatus;
	BOOLEAN IsOverlapped;
	PIO_OBJECT Object;
	LPOVERLAPPED Overlapped;
	BOOLEAN SkipOnSuccess;
	LARGE_INTEGER Size;
	BOOLEAN FileValid;
	PBTR_IAT_PATCH Patch;
	ULONG Complete;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);

	Object = IoGetObjectByHandle(SK_HANDLE(hSocket), HANDLE_SOCKET);
	if (!Object) {
		Status = TransmitFile(hSocket, hFile, nNumberOfBytesToWrite, nNumberOfBytesPerSend,
							lpOverlapped, lpTransmitBuffers, dwFlags);
		return Status;
	}

	Size.QuadPart = 0;
	FileValid = FALSE;

	//
	// Check whether file handle is valid
	//

	if (hFile) {
		if (GetFileSizeEx(hFile, &Size)) {
			FileValid = TRUE;
		}
	}

	Irp = IoAllocateIrp(Object);
	Irp->Operation = IO_OP_TRANSMITFILE;
	Irp->CallType = _IoTransmitFile;
	Irp->RequestBytes = 0;
	Irp->Flags.Queued = FALSE;
	
	//
	// If file handle is valid and nNumberOfBytesToWrite is 0,
	// this call transmit all file data
	//

	//
	// The maximum number of bytes that can be transmitted using a single call to 
	// the TransmitFile function is 2,147,483,646, the maximum value for a 32 bit integer minus 1.
	//

	if (FileValid && nNumberOfBytesToWrite == 0) {
		Irp->RequestBytes = (ULONG64)Size.QuadPart;
	}
	if (FileValid && nNumberOfBytesToWrite != 0) {
		Irp->RequestBytes = nNumberOfBytesToWrite;
	}
	if (lpTransmitBuffers) {
		Irp->RequestBytes += lpTransmitBuffers->HeadLength + lpTransmitBuffers->TailLength;
	}

	Patch = IoGetPatch(_IatTransmitFile);
	IoCaptureStackTrace(Thread, Patch->Address, Irp);

	if (IoProbeOverlapped(lpOverlapped)) {
		IsOverlapped = TRUE;
		Overlapped = IoHijackOverlapped(Irp, lpOverlapped);
	}
	else {
		IsOverlapped = FALSE;
		IoMarkIrpSynchronous(Irp);
		Overlapped = NULL;
	}

	GetSystemTimeAsFileTime(&Irp->Time);
	QueryPerformanceCounter(&Irp->Start);

	if (IsOverlapped) {
		IoQueueIrpToObject(Object, Irp);
		Status = TransmitFile(hSocket, hFile, nNumberOfBytesToWrite, nNumberOfBytesPerSend,
								Overlapped, lpTransmitBuffers, dwFlags);
	}
	else {
		Status = TransmitFile(hSocket, hFile, nNumberOfBytesToWrite, nNumberOfBytesPerSend,
								lpOverlapped, lpTransmitBuffers, dwFlags);
	}

	IoStatus = WSAGetLastError();
	Irp->LastError = IoStatus;
	if (Status == FALSE && IoStatus != WSA_IO_PENDING) {

		IoUpdateFailedCountersEx(HANDLE_SOCKET, IO_OP_TRANSMITFILE);

		IoDequeueIrpFromObject(Object, Irp);
		IoFreeIrp(Irp);

		BtrLeaveExemptionRegion(Thread);
		WSASetLastError(IoStatus);
		return Status;
	}

	if (Status == FALSE) {
		Complete = 0;
	}
	else {
		Complete = (ULONG)Irp->RequestBytes;
	}

	if (!IsOverlapped) {
		IoQuerySocketAddress(Object, hSocket);
		IoCompleteSynchronousIo(Irp, Status, IoStatus, &Complete, NULL);
	}
	else {
		SkipOnSuccess = HalQuerySkipOnSuccess(SK_HANDLE(hSocket));
		if (SkipOnSuccess && IoStatus != WSA_IO_PENDING) {
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			IoCompleteSynchronousIo(Irp, Status, IoStatus, NULL, lpOverlapped);
		}
	}

	BtrLeaveExemptionRegion(Thread);
	WSASetLastError(IoStatus);
	return Status;;
}
