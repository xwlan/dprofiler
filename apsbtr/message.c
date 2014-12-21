//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "message.h"
#include "thread.h"
#include "heap.h"
#include "queue.h"
#include "cache.h"
#include "util.h"
#include "marker.h"

ULONG BtrMessageThreadId;
HANDLE BtrMessageThreadHandle;


ULONG CALLBACK 
BtrMessageProcedure(
	__in PVOID Context
	)
{
	ULONG Status;
	ULONG Complete;
	ULONG_PTR Value;
	PBTR_MESSAGE_HEADER Packet;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value);

	RtlZeroMemory(&BtrPortObject, sizeof(BTR_PORT_OBJECT));
	BtrPortObject.BufferLength = BTR_BUFFER_LENGTH;

	Status = BtrCreatePort(&BtrPortObject);
	if (Status != S_OK) {
		
		//
		// If initially failed to accept connection from profiler,
		// just exit and unload runtime library
		//

		FreeLibraryAndExitThread((HMODULE)BtrDllBase, Status);
		return Status;
	}

	Status = BtrAcceptPort(&BtrPortObject);
	if (Status != S_OK) {

		//
		// If initially failed to accept connection from profiler,
		// just exit and unload runtime library
		//

		FreeLibraryAndExitThread((HMODULE)BtrDllBase, Status);
		return Status;
	}
	
	Packet = (PBTR_MESSAGE_HEADER)BtrPortObject.Buffer;

	while (TRUE) {

		Status = BtrGetMessage(&BtrPortObject, Packet, 
			                   BtrPortObject.BufferLength, 
							   &Complete);

		if (Status != S_OK) {
			CancelIo(BtrPortObject.Object);
			break;
		}

		//
		// If I/O failed, break out here
		//

		Status = BtrOnMessage(Packet);
		if (Status != S_OK) {
			break;
		}
	}

	//
	// Close port object
	//

	BtrClosePort(&BtrPortObject);

	if (Status == BTR_S_UNLOAD) {
		(*BtrProfileObject->Unload)(BtrProfileObject);
	}

	return Status;
}

ULONG
BtrOnMessage(
	__in PBTR_MESSAGE_HEADER Packet
	)
{
	ULONG Status;	

	switch (Packet->Type) {

		case MESSAGE_START:
			Status = BtrOnMessageStart((PBTR_MESSAGE_START)Packet);
			break;

		case MESSAGE_STOP:
			Status = BtrOnMessageStop((PBTR_MESSAGE_STOP)Packet);
			break;

		case MESSAGE_PAUSE:
			Status = BtrOnMessagePause((PBTR_MESSAGE_PAUSE)Packet);
			break;

		case MESSAGE_RESUME:
			Status = BtrOnMessageResume((PBTR_MESSAGE_RESUME)Packet);
			break;

		case MESSAGE_MARK:
			Status = BtrOnMessageMark((PBTR_MESSAGE_MARK)Packet);
			break;

		case MESSAGE_QUERY:
			Status = BtrOnMessageQuery((PBTR_MESSAGE_QUERY)Packet);
			break;

		default:

			ASSERT(0);

			//
			// Ignore all unknown packets
			//

			Status = S_OK;
	}

	return Status;	
}

ULONG
BtrOnMessageStart(
	__in PBTR_MESSAGE_START Packet
	)
{
	PBTR_MESSAGE_START_ACK Ack;
	ULONG Complete;
	ULONG Status;
	ULONG Length;
	ULONG Count;
	PBTR_HOTPATCH_ENTRY Hotpatch;
	PBTR_MESSAGE_START_STATUS Commit;

	Ack = (PBTR_MESSAGE_START_ACK)BtrPortObject.Buffer;
	
	//
	// Initialize runtime and create profile object
	//

	Status = BtrOnStart(Packet, &BtrProfileObject);
	if (Status != S_OK) {
		
		Ack->Header.Type = MESSAGE_START_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_START_ACK);
		Ack->Status = Status;
		Ack->Count = 0;

		BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
		return Status;

	}

	ASSERT(BtrProfileObject != NULL);
	ASSERT(BtrProfileObject->QueryHotpatch != NULL);

	//
	// Query hotpatch information
	//

	Status = (*BtrProfileObject->QueryHotpatch)(BtrProfileObject, 
		                                        HOTPATCH_COMMIT,
									            &Hotpatch,
									            &Count);
	if (Status != S_OK) {
		
		Ack->Header.Type = MESSAGE_START_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_START_ACK);
		Ack->Status = Status;
		Ack->Count = 0;

		BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
		return Status;
	}

	//
	// Ensure buffer length is in bound
	//

	Length = FIELD_OFFSET(BTR_MESSAGE_START_ACK, Hotpatch[Count]);
	if (Length > BtrPortObject.BufferLength) {
		
		Ack->Header.Type = MESSAGE_START_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_START_ACK);
		Ack->Status = BTR_E_BUFFER_LIMITED;
		Ack->Count = 0;

		BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
		return Status;
	}

	//
	// Send start ack to control end
	//

	Ack->Header.Type = MESSAGE_START_ACK;
	Ack->Header.Length = Length;
	Ack->Status = S_OK;
	Ack->Count = Count;

	RtlCopyMemory(&Ack->Hotpatch[0], Hotpatch, sizeof(BTR_HOTPATCH_ENTRY) * Count);
	BtrFree(Hotpatch);

	Status = BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
	if (Status != S_OK) {
		return Status;
	}

	//
	// Wait control end commit hotpatch and return status to us
	//

	Status = BtrGetMessage(&BtrPortObject, (PBTR_MESSAGE_HEADER)BtrPortObject.Buffer, 
		                   BtrPortObject.BufferLength, &Complete);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Check whether hotpatch is committed successfully
	//

	Commit = (PBTR_MESSAGE_START_STATUS)BtrPortObject.Buffer;
	if (Commit->Status != S_OK) {
		return Commit->Status;
	}

	//
	// It's done, start profiling
	//

	Status = (*BtrProfileObject->Start)(BtrProfileObject);

	if (Status == S_OK) {
		BtrIsStarted = TRUE;
	}

	return Status;
}

ULONG
BtrOnMessageStop(
	__in PBTR_MESSAGE_STOP Packet
	)
{
	ULONG Status;
	ULONG Complete;
	ULONG Length;
	ULONG Count;
	PBTR_HOTPATCH_ENTRY Hotpatch;
	PBTR_MESSAGE_STOP_ACK Ack;
	PBTR_MESSAGE_STOP_STATUS Commit;

	//
	// Execute stop profiling
	//

	Ack = (PBTR_MESSAGE_STOP_ACK)BtrPortObject.Buffer;
	Ack->Header.Type = MESSAGE_STOP_ACK;

	Status = BtrOnStop(Packet, BtrProfileObject);

	if (Status != S_OK) {

		Ack->Header.Length = sizeof(BTR_MESSAGE_STOP_ACK);
		Ack->Status = Status;
		Ack->Count = 0;

		BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
		return Status;
	}

	//
	// Query hotpatch information
	//

	Status = (*BtrProfileObject->QueryHotpatch)(BtrProfileObject, 
		                                        HOTPATCH_DECOMMIT,
									            &Hotpatch,
									            &Count);
	if (Status != S_OK) {
		
		Ack->Header.Length = sizeof(BTR_MESSAGE_STOP_ACK);
		Ack->Status = Status;
		Ack->Count = 0;

		BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
		return Status;
	}

	//
	// Ensure buffer length is in bound
	//

	Length = FIELD_OFFSET(BTR_MESSAGE_STOP_ACK, Hotpatch[Count]);
	if (Length > BtrPortObject.BufferLength) {
		
		Ack->Header.Type = MESSAGE_STOP_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_STOP_ACK);
		Ack->Status = BTR_E_BUFFER_LIMITED;
		Ack->Count = 0;

		BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
		return Status;
	}

	//
	// Send stop ack to control end
	//

	Ack->Header.Type = MESSAGE_STOP_ACK;
	Ack->Header.Length = Length;
	Ack->Status = S_OK;
	Ack->Count = Count;
	RtlCopyMemory(&Ack->Hotpatch[0], Hotpatch, sizeof(BTR_HOTPATCH_ENTRY) * Count);

	BtrFree(Hotpatch);
	Status = BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	

	if (Status != S_OK) {
		return Status;
	}

	//
	// Wait control end commit hotpatch and return status to us
	//

	Status = BtrGetMessage(&BtrPortObject, (PBTR_MESSAGE_HEADER)BtrPortObject.Buffer, 
		                   BtrPortObject.BufferLength, &Complete);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Check whether hotpatch is committed successfully
	//

	Commit = (PBTR_MESSAGE_STOP_STATUS)BtrPortObject.Buffer;
	if (Commit->Status != S_OK) {
		return Commit->Status;
	}

	//
	// N.B. If application call ExitProcess, the we still receive
	// the stop request, but the ExitStatus field is filled with
	// BTR_S_EXITPROCESS, for this case, we don't unload runtime.
	//

	if (BtrProfileObject->ExitStatus != BTR_S_EXITPROCESS) {
		BtrProfileObject->ExitStatus = BTR_S_USERSTOP;
	}

	return BTR_S_UNLOAD;
}

ULONG
BtrOnMessagePause(
	__in PBTR_MESSAGE_PAUSE Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PBTR_MESSAGE_PAUSE_ACK Ack;

	if (BtrProfileObject->Pause != NULL) {
		Status = (*BtrProfileObject->Pause)(BtrProfileObject);

	} else {

		//
		// If profile object does not support pause, just return success
		//

		Status = S_OK;
	}
	
	Ack = (PBTR_MESSAGE_PAUSE_ACK)BtrPortObject.Buffer;
	Ack->Header.Type = MESSAGE_PAUSE_ACK;
	Ack->Header.Length = sizeof(BTR_MESSAGE_PAUSE_ACK);
	Ack->Status = Status;

	Status = BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
	return Status;
}

ULONG
BtrOnMessageResume(
	__in PBTR_MESSAGE_RESUME Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PBTR_MESSAGE_RESUME_ACK Ack;

	if (BtrProfileObject->Resume != NULL) {
		Status = (*BtrProfileObject->Resume)(BtrProfileObject);

	} else {

		//
		// If profile object does not support pause, just return success
		//

		Status = S_OK;
	}
	
	Ack = (PBTR_MESSAGE_RESUME_ACK)BtrPortObject.Buffer;
	Ack->Header.Type = MESSAGE_RESUME_ACK;
	Ack->Header.Length = sizeof(BTR_MESSAGE_RESUME_ACK);
	Ack->Status = Status;

	Status = BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
	return Status;
}

ULONG
BtrOnMessageMark(
	__in PBTR_MESSAGE_MARK Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PBTR_MESSAGE_MARK_ACK Ack;

	//
	// Insert user mark
	//

	BtrInsertMark(BtrCurrentSequence(), MARK_USER);

	Ack = (PBTR_MESSAGE_MARK_ACK)BtrPortObject.Buffer;
	Ack->Header.Type = MESSAGE_MARK_ACK;
	Ack->Header.Length = sizeof(BTR_MESSAGE_MARK_ACK);
	Ack->Status = S_OK;

	Status = BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
	return Status;
}

ULONG
BtrOnMessageQuery(
	__in PBTR_MESSAGE_QUERY Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PBTR_MESSAGE_QUERY_ACK Ack;

	//
	// N.B. Not implemented yet
	//

	Ack = (PBTR_MESSAGE_QUERY_ACK)BtrPortObject.Buffer;
	Ack->Header.Type = MESSAGE_QUERY_ACK;
	Ack->Header.Length = sizeof(BTR_MESSAGE_QUERY_ACK);
	Ack->Status = S_OK;

	Status = BtrSendMessage(&BtrPortObject, &Ack->Header, &Complete);	
	return Status;
}
