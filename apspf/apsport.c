//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "aps.h"
#include "apspri.h"
#include "apsdefs.h"
#include "apsport.h"

PAPS_PORT
ApsCreatePort(
	__in ULONG ProcessId,
	__in ULONG BufferLength
	)
{
	PAPS_PORT Port;
	HANDLE Handle;
	PVOID Buffer;

	Handle = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!Handle) {
		return NULL;
	}

	Buffer = VirtualAlloc(NULL, BufferLength, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) {
		CloseHandle(Handle);
		return NULL;
	}

	Port = (PAPS_PORT)ApsMalloc(sizeof(APS_PORT));
	RtlZeroMemory(Port, sizeof(APS_PORT));

	Port->ProcessId = ProcessId;
	StringCchPrintf(&Port->Name[0], 64, L"\\\\.\\pipe\\dpf\\%u", ProcessId);

	Port->Buffer = Buffer;
	Port->BufferLength = BufferLength;
	Port->CompleteEvent = Handle;
	Port->Overlapped.hEvent = Port->CompleteEvent;

	return Port;
}

ULONG
ApsConnectPort(
	__in PAPS_PORT Port
	)
{
	HANDLE FileHandle;
	ULONG Status;

	while (TRUE) {

		FileHandle = CreateFile(Port->Name,
								GENERIC_READ|GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_OVERLAPPED,
								NULL);

		if (FileHandle != INVALID_HANDLE_VALUE){
			ApsDebugTrace("ApsConnectPort(pid=%u)", Port->ProcessId);
			break;
		}

		Status = GetLastError();
		if (Status != ERROR_PIPE_BUSY) {
			ApsDebugTrace("ApsConnectPort(), Error=%u", Status);
			return Status;
		}

		//
		// Timeout set as 10 seconds
		//

		if (!WaitNamedPipe(Port->Name, 1000 * 10)) { 
			Status = GetLastError();
			ApsDebugTrace("ApsConnectPort(), Error=%u", Status);
			return Status;
		} 
	}

	Port->Object = FileHandle;
	return APS_STATUS_OK;
}

ULONG
ApsWaitPortIoComplete(
	__in PAPS_PORT Port,
	__in ULONG Milliseconds,
	__out PULONG CompleteBytes
	)
{
	ULONG Status;

	Status = WaitForSingleObject(Port->Overlapped.hEvent, Milliseconds);

	if (Status != WAIT_OBJECT_0) {
		Status = GetLastError();
		ApsDebugTrace("WaitForSingleObject() failed, status=%u", Status);
		Status = APS_STATUS_ERROR;
		return Status;
	}

	Status = GetOverlappedResult(Port->Object, 
								 &Port->Overlapped,
                                 CompleteBytes,
						         FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		ApsDebugTrace("GetOverlappedResult() failed, status=%u", Status);
		Status = APS_STATUS_ERROR;
		return Status;
	}
	
	return APS_STATUS_OK;
}

ULONG
ApsClosePort(
	__in PAPS_PORT Port	
	)
{
	CloseHandle(Port->Object);
	CloseHandle(Port->CompleteEvent);
	VirtualFree(Port->Buffer, 0, MEM_RELEASE);
	return APS_STATUS_OK;
}

ULONG
ApsGetMessage(
	__in PAPS_PORT Port,
	__out PBTR_MESSAGE_HEADER Packet,
	__in ULONG BufferLength,
	__out PULONG CompleteBytes
	)
{
	ULONG Status;

	if (BufferLength > Port->BufferLength) {
		return APS_STATUS_ERROR;
	}

	Status = (ULONG)ReadFile(Port->Object, Packet, BufferLength, CompleteBytes, &Port->Overlapped);
	if (Status != TRUE) {

		Status = GetLastError();
		if (Status != ERROR_IO_PENDING) {

			if (Status == ERROR_NO_DATA || Status == ERROR_PIPE_NOT_CONNECTED) {
				Status = APS_STATUS_IPC_BROKEN;
			}

			ApsDebugTrace("ReadFile() failed, status=%u", GetLastError());
			return Status;
		}

		Status = ApsWaitPortIoComplete(Port, INFINITE, CompleteBytes);
		return Status;
	} 
	
	return APS_STATUS_OK;
}

ULONG
ApsSendMessage(
	__in PAPS_PORT Port,
	__in PBTR_MESSAGE_HEADER Packet,
	__out PULONG CompleteBytes 
	)
{
	ULONG Status;

	if (Packet->Length > Port->BufferLength) {
		return APS_STATUS_ERROR;
	}

	Status = (ULONG)WriteFile(Port->Object, Packet, Packet->Length, CompleteBytes, &Port->Overlapped);
	if (Status != TRUE) {
		
		Status = GetLastError();
		if (Status != ERROR_IO_PENDING) {

			if (Status == ERROR_NO_DATA || Status == ERROR_PIPE_NOT_CONNECTED) {
				Status = APS_STATUS_IPC_BROKEN;
			}

			ApsDebugTrace("WriteFile() failed, status=%u", GetLastError());
			return Status;
		}

	    Status = ApsWaitPortIoComplete(Port, INFINITE, CompleteBytes);
		return Status;
	} 
	
	return APS_STATUS_OK;
}