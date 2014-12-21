//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "port.h"
#include "util.h"

BTR_PORT_OBJECT BtrPortObject;

VOID 
BtrInitOverlapped(
	VOID
	)
{
	ASSERT(BtrPortObject.CompleteEvent != NULL);
	RtlZeroMemory(&BtrPortObject.Overlapped, sizeof(OVERLAPPED));
	BtrPortObject.Overlapped.hEvent = BtrPortObject.CompleteEvent;
}

ULONG
BtrCreatePort(
	__in PBTR_PORT_OBJECT Port
	)
{
	PVOID Buffer;
	HANDLE PortObject;
	HANDLE CompleteEventObject;
	WCHAR NameBuffer[MAX_PATH];

	StringCchPrintf(NameBuffer, MAX_PATH, L"\\\\.\\pipe\\dpf\\%u", GetCurrentProcessId());
	StringCchCopy(Port->Name, 64, NameBuffer);

	PortObject = CreateNamedPipe(NameBuffer,
							     PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 
				                 PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE | PIPE_WAIT,
							     PIPE_UNLIMITED_INSTANCES,
				                 Port->BufferLength,
				                 Port->BufferLength,
				                 NMPWAIT_USE_DEFAULT_WAIT,
							     NULL);

	if (PortObject == NULL) {
		return S_FALSE;
	}
    
	CompleteEventObject = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (CompleteEventObject == NULL) {
		CloseHandle(PortObject);
		return S_FALSE;
	}

	Buffer = VirtualAlloc(NULL, Port->BufferLength, MEM_COMMIT, PAGE_READWRITE);
	if (Buffer == NULL) {
		CloseHandle(PortObject);
		CloseHandle(CompleteEventObject);
		return S_FALSE;
	}
	
	BtrPortObject.Object = PortObject;
	BtrPortObject.Buffer = Buffer;
	BtrPortObject.BufferLength = Port->BufferLength;
	BtrPortObject.CompleteEvent = CompleteEventObject;

	BtrInitOverlapped();
	return S_OK;
}

ULONG
BtrWaitPortIoComplete(
	__in PBTR_PORT_OBJECT Port,
	__in ULONG Milliseconds,
	__out PULONG CompleteBytes
	)
{
	ULONG Status;

	Status = WaitForSingleObject(BtrPortObject.Overlapped.hEvent, Milliseconds);

	if (Status != WAIT_OBJECT_0) {
		DebugTrace("BtrWaitForPortIoComplete status=0x%08x", Status);
		CancelIo(BtrPortObject.Overlapped.hEvent);
		return Status;
	}

	Status = GetOverlappedResult(BtrPortObject.Object, 
								 &BtrPortObject.Overlapped,
                                 CompleteBytes,
						         FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugTrace("BtrWaitForPortIoComplete GetOverlappedResult status=0x%08x", Status);
		return Status;
	}
	
	return S_OK;
}

ULONG
BtrAcceptPort(
	__in PBTR_PORT_OBJECT Port
	)
{
	BOOLEAN Status;
	ULONG Result;
	ULONG CompleteBytes;

	BtrInitOverlapped();
	Status = ConnectNamedPipe(BtrPortObject.Object, &BtrPortObject.Overlapped);

	if (Status != TRUE) {

		switch (GetLastError()) {
			case ERROR_IO_PENDING:
				Result = BtrWaitPortIoComplete(&BtrPortObject, INFINITE, &CompleteBytes);
				break;

			case ERROR_PIPE_CONNECTED:
				Result = S_OK;
				break;

			default:
				Result = S_FALSE;
		}
		return Result;
	} 
	
	return S_OK;
}

ULONG
BtrClosePort(
	__in PBTR_PORT_OBJECT Port
	)
{
	FlushFileBuffers(BtrPortObject.Object);
	DisconnectNamedPipe(BtrPortObject.Object);
	CloseHandle(BtrPortObject.Object);
	CloseHandle(BtrPortObject.CompleteEvent);
	VirtualFree(BtrPortObject.Buffer, 0, MEM_RELEASE);
	return S_OK;
}

ULONG
BtrDisconnectPort(
	__in PBTR_PORT_OBJECT Port
	)
{
	DisconnectNamedPipe(BtrPortObject.Object);
	return S_OK;
}

ULONG
BtrGetMessage(
	__in PBTR_PORT_OBJECT Port,
	__out PBTR_MESSAGE_HEADER Packet,
	__in ULONG BufferLength,
	__out PULONG CompleteBytes
	)
{
	BOOLEAN Status;
	ULONG ErrorCode;
	ULONG Result;

	if (BufferLength > BtrPortObject.BufferLength) {
		return S_FALSE;
	}

	BtrInitOverlapped();

	Status = ReadFile(BtrPortObject.Object, Packet, BufferLength, CompleteBytes, &BtrPortObject.Overlapped);
	if (Status != TRUE) {

		ErrorCode = GetLastError();
		if (ErrorCode != ERROR_IO_PENDING) {
			DebugTrace("BtrGetMessage status=0x%08x", ErrorCode);
			return S_FALSE;
		}

		Result = BtrWaitPortIoComplete(&BtrPortObject, INFINITE, CompleteBytes);
		return Result;
	} 
	
	return S_OK;
}

ULONG
BtrSendMessage(
	__in PBTR_PORT_OBJECT Port,
	__in PBTR_MESSAGE_HEADER Packet,
	__out PULONG CompleteBytes 
	)
{
	BOOLEAN Status;
	ULONG Result;
	ULONG ErrorCode;

	if (Packet->Length > BtrPortObject.BufferLength) {
		return S_FALSE;
	}

	BtrInitOverlapped();

	Status = WriteFile(BtrPortObject.Object, Packet, Packet->Length, CompleteBytes, &BtrPortObject.Overlapped);
	if (Status != TRUE) {
		
		ErrorCode = GetLastError();
		if (ErrorCode != ERROR_IO_PENDING) {
			DebugTrace("BtrSendMessage status=0x%08x", ErrorCode);
			return ErrorCode;
		}

	    Result = BtrWaitPortIoComplete(&BtrPortObject, INFINITE, CompleteBytes);
		return Result;
	} 
	
	return S_OK;
}