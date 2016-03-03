//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "apsprofile.h"
#include "btr.h"
#include "buildin.h"
#include "cache.h"
#include "heap.h"
#include "mmprof.h"
#include "mmheap.h"
#include "mmpage.h"
#include "mmhandle.h"
#include "mmgdi.h"
#include "ioprof.h"
#include "cpuprof.h"
#include "ccrprof.h"
#include "util.h"
#include "stream.h"
#include "message.h"
#include "ccrprof.h"

#define SECURITY_WIN32
#include <security.h>

//
// Global shared data between profiler and target process
//

PBTR_SHARED_DATA BtrSharedData;

//
// Profile object of target process
//

PBTR_PROFILE_OBJECT BtrProfileObject;


BOOLEAN BtrIsStarted;

BOOL WINAPI 
DllMain(
	__in HMODULE DllHandle,
    __in ULONG Reason,
	__in PVOID Reserved
	)
{
	BOOL Status = TRUE;

	switch (Reason) {

	case DLL_PROCESS_ATTACH:
		Status = BtrOnProcessAttach(DllHandle);
		break;

	case DLL_THREAD_ATTACH:
		Status = BtrOnThreadAttach();
		break;

	case DLL_THREAD_DETACH:
		Status = BtrOnThreadDetach();
		break;

	case DLL_PROCESS_DETACH:
		Status = BtrOnProcessDetach();
		break;

	}

	return Status;
}

BOOL
BtrOnThreadAttach(
	VOID
	)
{
	if (!BtrIsStarted || !BtrProfileObject) {
		return TRUE;
	}

	//
	// If we're in process of stopping profile, skip the
	// thread attach procedure, to avoid thread enter runtime
	// capture routines, we set current thread's exemption cookie,
	// no thread object created for current thread.
	//

	if (BtrIsStopped()) {
		ULONG_PTR Value;
		BtrSetExemptionCookie(GOLDEN_RATIO, &Value);
		return TRUE;
	}
	
	if (BtrProfileObject->ThreadAttach != NULL) {
		(*BtrProfileObject->ThreadAttach)(BtrProfileObject);
	}

	return TRUE;
}

BOOL
BtrOnThreadDetach(
	VOID
	)
{
	if (!BtrIsStarted || !BtrProfileObject) {
		return TRUE;
	}

	//
	// If we're in process of stopping profile, skip the
	// thread detach procedure
	//

	if (BtrIsStopped()) {
		return TRUE;
	}

	if (BtrProfileObject->ThreadDetach != NULL) {
		(*BtrProfileObject->ThreadDetach)(BtrProfileObject);
	}

	return TRUE;
}

BOOL
BtrOnProcessAttach(
	__in HMODULE DllHandle	
	)
{
	ULONG Status;

	//
	// Execute minimal initialization require to support message procedure
	//

	BtrDllBase = (ULONG_PTR)DllHandle;

	Status = BtrInitializeHal();
	if (Status != S_OK) {
		return FALSE;
	}

	Status = BtrInitializeHeap();
	if (Status != S_OK) {
		return FALSE;
	}

	//
	// Create message thread to wait for profiler connect to us
	//

	BtrMessageThreadHandle = CreateThread(NULL, 0, BtrMessageProcedure, 
		                                  NULL, 0, &BtrMessageThreadId);
	if (!BtrMessageThreadHandle) {
		return FALSE;
	}

	return TRUE;
}

BOOL
BtrOnProcessDetach(
	VOID
	)
{
	return TRUE;
}

ULONG
BtrInitialize(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	ULONG Status;

	__try {

		Status = BtrInitializeCache(Object);
		if (Status != S_OK) {
			__leave;
		}

		Status = BtrInitializeTrap(Object);
		if (Status != S_OK) {
			__leave;
		}

		Status = BtrInitializeStack(Object);
		if (Status != S_OK) {
			__leave;
		}

		Status = BtrInitializeThread(Object);
		if (Status != S_OK) {
			__leave;
		}

		BtrInitializeLookaside();

	}
	__finally {

		if (Status != S_OK) {
		
			BtrUninitializeCache();	
			BtrUninitializeTrap();
			BtrUninitializeStack();
			BtrUninitializeThread();
		
		}

	}

	return Status;
}

VOID
BtrUninitialize(
	VOID
	)
{
	BtrUninitializeCache();	
	BtrUninitializeTrap();
	BtrUninitializeStack();
	BtrUninitializeThread();
}

ULONG
BtrInitializeSharedData(
	__in HANDLE SharedDataObject	
	)
{
	BtrSharedData = (PBTR_SHARED_DATA)MapViewOfFile(SharedDataObject,
		                                            FILE_MAP_READ|FILE_MAP_WRITE,
													0, 0, 0);
	if (!BtrSharedData) {
		return GetLastError();
	}

	BtrInitSpinLock(&BtrSharedData->SpinLock, 100);

	BtrSharedData->Sequence = -1;
	BtrSharedData->StackId = -1;
	BtrSharedData->ObjectId = -1;

	return S_OK;
}

ULONG
BtrOnStart(
	__in PBTR_MESSAGE_START Packet,
	__out PBTR_PROFILE_OBJECT *Object
	)
{
	ULONG Status;
	PBTR_PROFILE_ATTRIBUTE Attr;

	//
	// Initialize shared data
	//

	*Object = NULL;
	Status = BtrInitializeSharedData(Packet->SharedData);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Validate profile object attributes
	//

	Attr = &Packet->Attribute;
	switch (Attr->Type) {

		case PROFILE_CPU_TYPE:
			Status = CpuValidateAttribute(Attr);
			break;

		case PROFILE_MM_TYPE:
			Status = MmValidateAttribute(Attr);
			break;

		case PROFILE_IO_TYPE:
			Status = IoValidateAttribute(Attr);
			break;

		case PROFILE_CCR_TYPE:
			Status = CcrValidateAttribute(Attr);
			break;
	}

	if (Status != S_OK) {
		return Status;
	}

	//
	// Validate kernel object handles
	//

	if (!BtrValidateHandle(Packet->SharedData)) {
		return BTR_E_INVALID_PARAMETER;
	}

	if (!BtrValidateHandle(Packet->StackFile)) {
		return BTR_E_INVALID_PARAMETER;
	}

	if (!BtrValidateHandle(Packet->ReportFile)) {
		return BTR_E_INVALID_PARAMETER;
	}
	
	if (!BtrValidateHandle(Packet->UnloadEvent)) {
		return BTR_E_INVALID_PARAMETER;
	}

	if (!BtrValidateHandle(Packet->ExitEvent)) {
		return BTR_E_INVALID_PARAMETER;
	}

	if (!BtrValidateHandle(Packet->ExitAckEvent)) {
		return BTR_E_INVALID_PARAMETER;
	}

	if (!BtrValidateHandle(Packet->ControlEnd)) {
		return BTR_E_INVALID_PARAMETER;
	}

	//
	// Validate IO handles
	//

	if (Attr->Type == PROFILE_IO_TYPE) {
		if (!BtrValidateHandle(Packet->IoObjectFile)) {
			return BTR_E_INVALID_PARAMETER;
		}
		if (!BtrValidateHandle(Packet->IoIrpFile)) {
			return BTR_E_INVALID_PARAMETER;
		}
		if (!BtrValidateHandle(Packet->IoNameFile)) {
			return BTR_E_INVALID_PARAMETER;
		}
	}

	if (Attr->Mode == RECORD_MODE) {

		if (!BtrValidateHandle(Packet->IndexFile)) {
			return BTR_E_INVALID_PARAMETER;
		}

		if (!BtrValidateHandle(Packet->DataFile)) {
			return BTR_E_INVALID_PARAMETER;
		}

	}
	
	BtrProfileObject = (PBTR_PROFILE_OBJECT)BtrMalloc(sizeof(BTR_PROFILE_OBJECT));
	RtlZeroMemory(BtrProfileObject, sizeof(BTR_PROFILE_OBJECT));
	RtlCopyMemory(&BtrProfileObject->Attribute, Attr, sizeof(*Attr));

	//
	// Fill the duplicated kernel objects to profile object
	//

	BtrProfileObject->SharedDataObject = Packet->SharedData;
	BtrProfileObject->IndexFileObject = Packet->IndexFile;
	BtrProfileObject->DataFileObject = Packet->DataFile;
	BtrProfileObject->StackFileObject = Packet->StackFile;
	BtrProfileObject->ReportFileObject = Packet->ReportFile;
	BtrProfileObject->UnloadEvent = Packet->UnloadEvent;
	BtrProfileObject->ExitProcessEvent = Packet->ExitEvent;
	BtrProfileObject->ExitProcessAckEvent = Packet->ExitAckEvent;
	BtrProfileObject->ControlEnd = Packet->ControlEnd;
	BtrProfileObject->IoObjectFile = Packet->IoObjectFile;
	BtrProfileObject->IoIrpFile = Packet->IoIrpFile;
	BtrProfileObject->IoNameFile = Packet->IoNameFile;

	//
	// N.B. Other subsystems are initialized here, because some part depends
	// on state of profile object, we have to fill valid data to profile object
	// before initialize other parts.
	//

	Status = BtrInitialize(BtrProfileObject);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Further initialization based on profile type
	//

	switch (Attr->Type) {
		
		case PROFILE_CPU_TYPE:
			Status = CpuInitialize(BtrProfileObject);
			break;

		case PROFILE_MM_TYPE:
			Status = MmInitialize(BtrProfileObject);
			break;

		case PROFILE_IO_TYPE:
			Status = IoInitialize(BtrProfileObject);
			break;
		
		case PROFILE_CCR_TYPE:
			Status = CcrInitialize(BtrProfileObject);
			break;
		default:
			ASSERT(0);
	}

	if (Status != S_OK) {
		BtrUninitialize();
		return Status;
	}

	*Object = BtrProfileObject;
	return S_OK;
}

ULONG 
BtrOnStop(
	__in PBTR_MESSAGE_STOP Packet,
	__in PBTR_PROFILE_OBJECT Object
	)
{
	ULONG Status;

	UNREFERENCED_PARAMETER(Packet);

	Status = (*BtrProfileObject->Stop)(BtrProfileObject);
	return Status;
}