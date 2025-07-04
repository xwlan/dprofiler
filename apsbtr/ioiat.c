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

	Handle = GetModuleHandleA("kernel32.dll");
	for (i = 0; i < IoPatchCount; i++) {
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
	if (!IoRefObjectCheckOverlapped(hFile, HANDLE_FILE, lpOverlapped, &Object, &IsOverlapped, &SkipOnSuccess, TRUE)) {
		goto Skip;
	}

	BtrEnterExemptionRegion(Thread);

	Irp = IoAllocateIrp(Object);
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
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
		}
		else {
			IoIrpClearInCall(Irp);
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
			IoCopyOverlapped(&Irp->Overlapped, lpOverlapped);
			//IoCompleteSynchronousIo(Irp, Status, IoStatus, lpNumberOfBytesWrite, lpOverlapped);
		}
		else {
			IoIrpClearInCall(Irp);
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
	PIO_OBJECT Object;

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
	IoAttachIrpToOverlapped(lpOverlapped, Irp);

	//
	// If io is still pending, user may retry, just return here,
	// note that we still embed an irp pointer in lpOverlapped->Internal
	//

	if (!Status && IoStatus == ERROR_IO_INCOMPLETE) {
		BtrLeaveExemptionRegion(Thread);
		return Status;
	}

	//
	// IO is completed, success or failure, duplicate
	// the io status to user's lpOverlapped, if the user
	// retry to use the same lpOverlapped repeatly call
	// this routine, we won't get an irp pointer.
	//

	QueryPerformanceCounter(&Irp->End);

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
			}
			else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoUnreferenceObject(Object);
	}

	Irp->IoStatus = IoGetCompletionStatus(Irp);
	Irp->CompleteBytes = IoGetCompletionSize(Irp);
	Irp->CompleteThreadId = GetCurrentThreadId();
	Irp->Flags.Completed = 1;

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);

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

			if (Irp->Operation == IO_OP_ACCEPT && Irp->IoStatus == ERROR_SUCCESS) {
				ASSERT(Irp->SkListen != 0);
				setsockopt((SOCKET)Irp->Object, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
							(char*)&Irp->SkListen, sizeof(SOCKET));
			}

			IoQuerySocketAddress(Object, (SOCKET)Irp->Object);
			IoUnreferenceObject(Object);

			if (!FlagOn(Object->Flags, OF_IOCPASSOCIATE)) {
				SetFlag(Object->Flags, OF_IOCPASSOCIATE);
			}
		}
	}

	if (Irp->Flags.File) {
		Object = IoLookupObjectByHandleEx(Irp->Object, HANDLE_FILE);
		if (Object) {
			if (!FlagOn(Object->Flags, OF_IOCPASSOCIATE)) {
				SetFlag(Object->Flags, OF_IOCPASSOCIATE);
			}
		}
	}

	if (Irp->Operation == IO_OP_IOCONTROL && Irp->ControlCode == FIONBIO) {
		ASSERT(Object != NULL);
		if (Object && Irp->IoStatus == ERROR_SUCCESS) {
			if (!Irp->ControlData) {
				IoClearObjectOverlapped(Object);
			}
			else {
				IoMarkObjectOverlapped(Object);
			}
		}
		IoUnreferenceObject(Object);
	}

	IoUpdateRequestCounters(Irp);
	IoUpdateCompleteCounters(Irp);
	IoQueueFlushList(Irp);

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

		Overlapped = Irp->Original;
		IoCopyOverlapped(&Irp->Overlapped, Overlapped);

		Irp->IoStatus = IoGetCompletionStatus(Irp);
		Irp->CompleteBytes = IoGetCompletionSize(Irp);
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
				}
				else {
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

	BtrLeaveExemptionRegion(Thread);
	return Status;
}
