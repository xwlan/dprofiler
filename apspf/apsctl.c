//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsctl.h"
#include "apspri.h"
#include "apsbtr.h"
#include "aps.h"
#include "apsfile.h"
#include "aps.h"
#include "ntapi.h"
#include "apspdb.h"
#include "apsrpt.h"
#include "apsqueue.h"
#include "apsport.h"
#include "apslog.h"

WCHAR ApsLogPath[MAX_PATH];
WCHAR ApsLocalSymPath[MAX_PATH];
WCHAR ApsDllPath[MAX_PATH];

PAPS_PROFILE_OBJECT ApsProfileObject;

PAPS_PROFILE_OBJECT
ApsCurrentProfile(
	VOID
	)
{
	return ApsProfileObject;
}

ULONG
ApsInitializeProfile(
	VOID
	)
{
	return APS_STATUS_OK;
}

ULONG
ApsCreateProfile(
	__out PAPS_PROFILE_OBJECT *Profile,
	__in BTR_PROFILE_MODE Mode,
	__in BTR_PROFILE_TYPE Type,
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in PWSTR ReportPath
	)
{
	ULONG Status;
	WCHAR Uuid[64];
	WCHAR StackPath[MAX_PATH];
	WCHAR IndexPath[MAX_PATH];
	WCHAR DataPath[MAX_PATH];
	HANDLE UnloadEvent = NULL;
	HANDLE IndexHandle = NULL;
	HANDLE DataHandle = NULL;
	HANDLE StackHandle = NULL;
	HANDLE ReportHandle = NULL;
	HANDLE ExitProcessEvent = NULL;
	HANDLE ExitProcessAckEvent = NULL;
	PAPS_PROFILE_OBJECT Object;
	HANDLE SharedObject;
	PBTR_SHARED_DATA SharedObjectPtr;
	ULONG Length;

	Object = (PAPS_PROFILE_OBJECT)ApsMalloc(sizeof(APS_PROFILE_OBJECT));
	RtlZeroMemory(Object, sizeof(*Object));

	Length = ApsUlongRoundUp(sizeof(BTR_SHARED_DATA), ApsPageSize);
	SharedObject = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
		                             PAGE_READWRITE, 0, Length, NULL);
	if (!SharedObject) {
		Status = GetLastError();
		ApsFree(Object);
		return Status;
	}

	SharedObjectPtr = (PBTR_SHARED_DATA)MapViewOfFile(SharedObject, FILE_MAP_READ|FILE_MAP_WRITE,
		                                              0, 0, 0);
	if (!SharedObjectPtr) {
		Status = GetLastError();
		CloseHandle(SharedObject);
		ApsFree(Object);
		return Status;
	}

	Object->SharedDataObject = SharedObject;
	Object->SharedData = SharedObjectPtr;

	//
	// Create UUID for profile object
	//

	Status = ApsCreateUuidString(Uuid, 64);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	__try {

		//
		// Create and duplicate unload event
		//

		UnloadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!UnloadEvent) {
			Status = GetLastError();
			__leave;
		}

		//
		// Create and duplicate exit process event
		//

		ExitProcessEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!ExitProcessEvent) {
			Status = GetLastError();
			__leave;
		}

		//
		// Create and duplicate exit process ack event
		//

		ExitProcessAckEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!ExitProcessAckEvent) {
			Status = GetLastError();
			__leave;
		}

		//
		// Create profile report file and duplciate its handle
		//

		ReportHandle = CreateFile(ReportPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
								  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (ReportHandle == INVALID_HANDLE_VALUE) {
			Status = GetLastError();
			__leave;
		}

		//
		// Create stack record file and duplicate its handle
		//

		StringCchPrintf(StackPath, MAX_PATH, L"%s\\{%s}.s", ApsLogPath, Uuid);
		StackHandle = CreateFile(StackPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
								 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (StackHandle == INVALID_HANDLE_VALUE) {
			Status = GetLastError();
			__leave;
		}

		//
		// Create index file and duplicate its handle
		//

		StringCchPrintf(IndexPath, MAX_PATH, L"%s\\{%s}.i", ApsLogPath, Uuid);
		IndexHandle = CreateFile(IndexPath, GENERIC_READ|GENERIC_WRITE, 
								 FILE_SHARE_READ|FILE_SHARE_WRITE,
								 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (IndexHandle == INVALID_HANDLE_VALUE) {
			Status = GetLastError();
			__leave;
		}

		SetFilePointer(IndexHandle, FS_FILE_INCREMENT, NULL, FILE_BEGIN);
		SetEndOfFile(IndexHandle);
		SetFilePointer(IndexHandle, 0, NULL, FILE_BEGIN);

		//
		// Create data file and duplicate its handle
		//

		StringCchPrintf(DataPath, MAX_PATH, L"%s\\{%s}.d", ApsLogPath, Uuid);
		DataHandle = CreateFile(DataPath, GENERIC_READ|GENERIC_WRITE, 
								FILE_SHARE_READ|FILE_SHARE_WRITE,
								NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (DataHandle == INVALID_HANDLE_VALUE) {
			Status = GetLastError();
			__leave;
		}

		SetFilePointer(DataHandle, FS_FILE_INCREMENT, NULL, FILE_BEGIN);
		SetEndOfFile(DataHandle);
		SetFilePointer(DataHandle, 0, NULL, FILE_BEGIN);

		Status = APS_STATUS_OK;

	}
	
	__finally {

		if (Status != APS_STATUS_OK) {

			if (UnloadEvent) {
				CloseHandle(UnloadEvent);
				UnloadEvent = NULL;
			}

			if (ExitProcessEvent) {
				CloseHandle(ExitProcessEvent);
				ExitProcessEvent = NULL;
			}

			if (ExitProcessAckEvent) {
				CloseHandle(ExitProcessAckEvent);
				ExitProcessAckEvent = NULL;
			}

			if (IndexHandle) {
				CloseHandle(IndexHandle);
				IndexHandle = NULL;
			}

			if (DataHandle) {
				CloseHandle(DataHandle);
				DataHandle = NULL;
			}

			if (StackHandle) {
				CloseHandle(StackHandle);
				StackHandle = NULL;
			}

			if (ReportHandle) {
				CloseHandle(ReportHandle);
				ReportHandle = NULL;
			}

			DeleteFile(IndexPath);
			DeleteFile(DataPath);
			DeleteFile(StackPath);
			DeleteFile(ReportPath);
		}
	}

	if (Status != APS_STATUS_OK) {
		*Profile = NULL;
		return Status;
	}

	Object->Mode = Mode;
	Object->Type = Type;

	Object->ProcessId = ProcessId;
	Object->ProcessHandle = ProcessHandle;

	StringCchCopy(Object->Uuid, 64, Uuid);
	StringCchCopy(Object->IndexPath, MAX_PATH, IndexPath);
	StringCchCopy(Object->DataPath, MAX_PATH, DataPath);
	StringCchCopy(Object->StackPath, MAX_PATH, StackPath);
	StringCchCopy(Object->ReportPath, MAX_PATH, ReportPath);

	//
	// Create internal control event
	//

	Object->CommandEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Object->StartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Object->UnloadEvent = UnloadEvent;
	Object->ExitProcessEvent = ExitProcessEvent;
	Object->ExitProcessAckEvent = ExitProcessAckEvent;

	Object->IndexFileHandle = IndexHandle;
	Object->DataFileHandle = DataHandle;
	Object->StackFileHandle = StackHandle;
	Object->ReportFileHandle = ReportHandle;

	ApsInitCriticalSection(&Object->Lock);

	*Profile = Object;

	//
	// Set current profile object
	//

	ApsProfileObject = Object;
	return APS_STATUS_OK;
}

ULONG
ApsStartProfile(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	ULONG Status;
	ULONG ProcessId;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	HANDLE ControlEnd;
	PAPS_QUEUE QueueObject;
	PAPS_PORT PortObject;
	PAPS_QUEUE_PACKET Packet;

	ProcessId = Profile->ProcessId;
	ProcessHandle = Profile->ProcessHandle;

	ApsEnterCriticalSection(&Profile->Lock);

	//
	// Load runtime dll into target address space
	//

	Status = ApsLoadLibrary(ProcessId, ProcessHandle, ApsDllPath);
	if (Status != APS_STATUS_OK) {

		//
		// Add diagnoistic information to log
		//

		if (!ApsIsProcessLive(ProcessHandle)) {
			ApsWriteLogEntry(ProcessId, LogLevelFailure, "pid=%u is terminated, err=%u", 
							 ProcessId, Status);
		}
		else {
			ApsWriteLogEntry(ProcessId, LogLevelFailure, "pid=%u is live, err=%u", 
							 ProcessId, Status);
		}

		ApsLeaveCriticalSection(&Profile->Lock);
		return Status;
	}
	
	//
	// Create queue object
	//

	Status = ApsCreateQueue(&QueueObject);
	if (Status != APS_STATUS_OK) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return Status;
	}

	//
	// Create port object
	//

	PortObject = ApsCreatePort(ProcessId, BTR_BUFFER_LENGTH);
	if (!PortObject) {
		ApsCloseQueue(QueueObject);
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_ERROR;
	}

	Profile->QueueObject = QueueObject;
	Profile->PortObject = PortObject;

	//
	// Create profile worker thread
	//

	ThreadHandle = CreateThread(NULL, 0, ApsProfileProcedure, 
			                    Profile, 0, NULL);
	Profile->ProfileThreadHandle = ThreadHandle;

	//
	// Wait for profile thread to finish initialization
	//

	WaitForSingleObject(Profile->StartEvent, INFINITE);

	//
	// Allocate and initialize start packet
	//

	Packet = (PAPS_QUEUE_PACKET)ApsMalloc(sizeof(APS_QUEUE_PACKET));
	RtlZeroMemory(Packet, sizeof(*Packet));

	Packet->Type = APS_CTL_START;
	Packet->CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	Packet->Start.SharedData = ApsDuplicateHandle(ProcessHandle, Profile->SharedDataObject);
	if (!Packet->Start.SharedData) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	Packet->Start.IndexFile = ApsDuplicateHandle(ProcessHandle, Profile->IndexFileHandle);
	if (!Packet->Start.IndexFile) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	CloseHandle(Profile->IndexFileHandle);
	Profile->IndexFileHandle = NULL;

	Packet->Start.DataFile = ApsDuplicateHandle(ProcessHandle, Profile->DataFileHandle);
	if (!Packet->Start.DataFile) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	CloseHandle(Profile->DataFileHandle);
	Profile->DataFileHandle = NULL;

	Packet->Start.StackFile = ApsDuplicateHandle(ProcessHandle, Profile->StackFileHandle);
	if (!Packet->Start.StackFile) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	CloseHandle(Profile->StackFileHandle);
	Profile->StackFileHandle = NULL;

	Packet->Start.ReportFile = ApsDuplicateHandle(ProcessHandle, Profile->ReportFileHandle);
	if (!Packet->Start.ReportFile) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	CloseHandle(Profile->ReportFileHandle);
	Profile->ReportFileHandle = NULL;

	Packet->Start.UnloadEvent = ApsDuplicateHandle(ProcessHandle, Profile->UnloadEvent);
	if (!Packet->Start.UnloadEvent) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	Packet->Start.ExitProcessEvent = ApsDuplicateHandle(ProcessHandle, Profile->ExitProcessEvent);
	if (!Packet->Start.ExitProcessEvent) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	Packet->Start.ExitProcessAckEvent = ApsDuplicateHandle(ProcessHandle, Profile->ExitProcessAckEvent);
	if (!Packet->Start.ExitProcessAckEvent) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}

	//
	// Duplicate current process's handle to target process
	//

	ControlEnd = OpenProcess(SYNCHRONIZE, FALSE, GetCurrentProcessId()); 
	Packet->Start.ControlEnd = ApsDuplicateHandle(ProcessHandle, ControlEnd);
	if (!Packet->Start.ControlEnd) {
		ApsLeaveCriticalSection(&Profile->Lock);
		return APS_STATUS_DUPLICATEHANDLE;
	}
	CloseHandle(ControlEnd);

	RtlCopyMemory(&Packet->Start.Attribute, &Profile->Attribute, 
		          sizeof(BTR_PROFILE_ATTRIBUTE));

	//
	// Queue start packet to profile procedure and wait for its completion
	//
	
	ApsQueuePacket(QueueObject, Packet);
	SetEvent(Profile->CommandEvent);

	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	CloseHandle(Packet->CompleteEvent);

	ApsLeaveCriticalSection(&Profile->Lock);

	Status = Packet->Status;
	ApsFree(Packet);
	return Status;
}

ULONG
ApsPauseProfile(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	ULONG Status;
	PAPS_QUEUE QueueObject;
	PAPS_QUEUE_PACKET Packet;

	ApsEnterCriticalSection(&Profile->Lock);

	QueueObject = Profile->QueueObject;
	ASSERT(QueueObject != NULL);

	//
	// Allocate and initialize pause packet
	//

	Packet = (PAPS_QUEUE_PACKET)ApsMalloc(sizeof(APS_QUEUE_PACKET));
	RtlZeroMemory(Packet, sizeof(*Packet));

	Packet->Type = APS_CTL_PAUSE;
	Packet->Pause.Duration = -1;
	Packet->CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//
	// Queue pause packet
	//
	
	ApsQueuePacket(QueueObject, Packet);
	SetEvent(Profile->CommandEvent);

	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	CloseHandle(Packet->CompleteEvent);

	ApsLeaveCriticalSection(&Profile->Lock);

	Status = Packet->Status;
	ApsFree(Packet);
	
	return Status;
}

ULONG
ApsResumeProfile(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	ULONG Status;
	PAPS_QUEUE QueueObject;
	PAPS_QUEUE_PACKET Packet;

	ApsEnterCriticalSection(&Profile->Lock);

	QueueObject = Profile->QueueObject;
	ASSERT(QueueObject != NULL);

	//
	// Allocate and initialize resume packet
	//

	Packet = (PAPS_QUEUE_PACKET)ApsMalloc(sizeof(APS_QUEUE_PACKET));
	RtlZeroMemory(Packet, sizeof(*Packet));

	Packet->Type = APS_CTL_RESUME;
	Packet->Resume.Spare = 0;
	Packet->CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//
	// Queue resume packet
	//
	
	ApsQueuePacket(QueueObject, Packet);
	SetEvent(Profile->CommandEvent);

	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	CloseHandle(Packet->CompleteEvent);

	ApsLeaveCriticalSection(&Profile->Lock);

	Status = Packet->Status;
	ApsFree(Packet);
	
	return Status;
}

ULONG
ApsMarkProfile(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	ULONG Status;
	PAPS_QUEUE QueueObject;
	PAPS_QUEUE_PACKET Packet;

	ApsEnterCriticalSection(&Profile->Lock);

	QueueObject = Profile->QueueObject;
	ASSERT(QueueObject != NULL);

	//
	// Allocate and initialize mark packet
	//

	Packet = (PAPS_QUEUE_PACKET)ApsMalloc(sizeof(APS_QUEUE_PACKET));
	RtlZeroMemory(Packet, sizeof(*Packet));

	Packet->Type = APS_CTL_MARK;
	Packet->Mark.Spare = 0;
	Packet->CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//
	// Queue mark packet
	//
	
	ApsQueuePacket(QueueObject, Packet);
	SetEvent(Profile->CommandEvent);

	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	CloseHandle(Packet->CompleteEvent);

	ApsLeaveCriticalSection(&Profile->Lock);

	Status = Packet->Status;
	ApsFree(Packet);
	
	return Status;
}

ULONG
ApsStopProfile(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	ULONG Status;
	PAPS_QUEUE_PACKET Packet;
	PAPS_QUEUE QueueObject;

	ApsEnterCriticalSection(&Profile->Lock);

	Packet = (PAPS_QUEUE_PACKET)ApsMalloc(sizeof(APS_QUEUE_PACKET));
	RtlZeroMemory(Packet, sizeof(*Packet));

	Packet->Type = APS_CTL_STOP;
	Packet->CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	QueueObject = Profile->QueueObject;
	ASSERT(QueueObject != NULL);

	//
	// Queue stop packet to profile work thread
	//

	ApsQueuePacket(QueueObject, Packet);
	SetEvent(Profile->CommandEvent);

	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	CloseHandle(Packet->CompleteEvent);

	Status = Packet->Status;
	ApsFree(Packet);

	ApsLeaveCriticalSection(&Profile->Lock);

	WaitForSingleObject(Profile->ProfileThreadHandle, INFINITE);
	ApsCloseProfile(Profile);
	return Status;
}

VOID
ApsCloseProfile(
	__in PAPS_PROFILE_OBJECT Profile
	)
{
	ApsDeleteCriticalSection(&Profile->Lock);

	if (Profile->ProcessHandle) {
		CloseHandle(Profile->ProcessHandle);
	}

	if (Profile->ProfileThreadHandle) {
		CloseHandle(Profile->ProfileThreadHandle);
	}

	if (Profile->SharedData) {
		UnmapViewOfFile(Profile->SharedData);
	}

	if (Profile->SharedDataObject) {
		CloseHandle(Profile->SharedDataObject);
	}

	if (Profile->IndexFileHandle) {
		CloseHandle(Profile->IndexFileHandle);
	}

	if (Profile->DataFileHandle) {
		CloseHandle(Profile->DataFileHandle);
	}

	if (Profile->StackFileHandle) {
		CloseHandle(Profile->StackFileHandle);
	}

	if (Profile->ReportFileHandle) {
		CloseHandle(Profile->ReportFileHandle);
	}

	if (Profile->CommandEvent) {
		CloseHandle(Profile->CommandEvent);
	}

	if (Profile->StartEvent) {
		CloseHandle(Profile->StartEvent);
	}

	if (Profile->StopEvent) {
		CloseHandle(Profile->StopEvent);
	}

	if (Profile->UnloadEvent) {
		CloseHandle(Profile->UnloadEvent);
	}

	if (Profile->ExitProcessEvent) {
		CloseHandle(Profile->ExitProcessEvent);
	}

	if (Profile->ExitProcessAckEvent) {
		CloseHandle(Profile->ExitProcessAckEvent);
	}

	if (Profile->QueueObject) {
		ApsCloseQueue(Profile->QueueObject);
		ApsFree(Profile->QueueObject);
	}

	if (Profile->PortObject) {
		ApsClosePort(Profile->PortObject);
		ApsFree(Profile->PortObject);
	}

    ASSERT(Profile = ApsProfileObject);

    ApsFree(Profile);
	ApsProfileObject = NULL;
}

VOID
ApsRegisterCallback(
	__in PAPS_PROFILE_OBJECT Object,
	__in APS_PROFILE_CALLBACK Callback,
	__in PVOID Context
	)
{
	Object->CallbackRoutine = Callback;
	Object->CallbackContext = Context;
}

BOOLEAN
ApsIsExecutingPc(
	__in PULONG_PTR Pc,
	__in ULONG PcCount,
	__in PBTR_ADDRESS_RANGE Range,
	__in ULONG RangeCount
	)
{
	ULONG i, j;

	for(i = 0; i < PcCount; i++) {
		for(j = 0; j < RangeCount; j++) {
			if ((Pc[i] >= Range[j].Address) && (Pc[i] < Range[j].Address + Range[j].Size)) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

ULONG
ApsLoadLibrary(
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in PWSTR DllPath
	)
{
	ULONG Status;
	HANDLE ThreadHandle;
	PVOID Buffer;
	SIZE_T Length;
	PVOID ThreadRoutine;

	Buffer = VirtualAllocEx(ProcessHandle, NULL, ApsPageSize, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) { 
		Status = GetLastError();
		ApsWriteLogEntry(ProcessId, LogLevelFailure, 
			             "VirtualAllocEx() failed, gle=%u", Status);
		return Status;
	}

	Length = (wcslen(DllPath) + 1) * sizeof(WCHAR);
	Status = WriteProcessMemory(ProcessHandle, Buffer, DllPath, Length, NULL); 
	if (!Status) {
		Status = GetLastError();
		ApsWriteLogEntry(ProcessId, LogLevelFailure, 
			             "WriteProcessMemory() failed, gle=%u", Status);
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;
	}	
	
	ThreadRoutine = ApsGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
	if (!ThreadRoutine) {
		ApsWriteLogEntry(ProcessId, LogLevelFailure, 
			             "ApsGetRemoteApiAddress() failed, err=%u", ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	//
	// Execute LoadLibraryW in remote address space as thread routine
	//

	ThreadHandle = 0;

	Status = ApsCreateRemoteThread(ProcessHandle, ThreadRoutine,
		                           Buffer, &ThreadHandle);
	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		ApsWriteLogEntry(ProcessId, LogLevelFailure, 
			             "ApsCreateRemoteThread() failed, err=%u", Status);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);
	CloseHandle(ThreadHandle);

	//
	// Check whether it's really loaded into target address space
	//

	Status = ApsGetModuleInformation(ProcessId, DllPath, TRUE, 
		                             NULL, NULL, NULL);

	if (!Status) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		ApsWriteLogEntry(ProcessId, LogLevelFailure, 
			             "ApsGetModuleInformation() failed, err=%u", Status);
		return ERROR_BAD_ARGUMENTS;
	}

	return APS_STATUS_OK;
}

ULONG CALLBACK
ApsProfileProcedure(
	__in PVOID Context
	)
{
	ULONG Status;
	HANDLE Handles[3];
	ULONG Signal;
	PAPS_PROFILE_OBJECT Object;

	Object = (PAPS_PROFILE_OBJECT)Context;

	ASSERT(Object->StartEvent != NULL);
	ASSERT(Object->QueueObject != NULL);
	ASSERT(Object->PortObject != NULL);

	Handles[0] = Object->CommandEvent;
	Handles[1] = Object->ExitProcessEvent;
	Handles[2] = Object->ProcessHandle;

	Status = APS_STATUS_OK;
	Object->ExitStatus = 0;

	SetEvent(Object->StartEvent);

	while (TRUE) {
	
		Signal = WaitForMultipleObjects(3, Handles, FALSE, INFINITE);

		switch (Signal) {

			case WAIT_OBJECT_0:

				Status = ApsOnCommand(Object);
				ResetEvent(Handles[0]);
				break;

			case WAIT_OBJECT_0 + 1:

				//
				// Stop profiling and signal ExitProcessAckEvent to release
				// ExitProcessEnter to execute host code
				//

				Status = ApsOnExitProcess(Object);
				SignalObjectAndWait(Object->ExitProcessAckEvent,
									Object->ProcessHandle, 
									INFINITE, FALSE);
				break;

			case WAIT_OBJECT_0 + 2:

				//
				// Target process was terminated unexpectedly
				//

				Status = ApsOnTerminated(Object);
				break;

			default:
				break;
		} 

		if (Status != APS_STATUS_OK) {
			Object->ExitStatus = Status;
			break;
		}

		if (Object->ExitStatus != 0) {
			break;
		}
	}

	return Object->ExitStatus;
}

ULONG
ApsOnCommand(
	__in PAPS_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	PAPS_QUEUE QueueObject;
	PAPS_QUEUE_PACKET Packet;
	
	Status = APS_STATUS_OK;

	QueueObject = Object->QueueObject;
	ASSERT(QueueObject != NULL);

	//
	// Dequeue command packet from queue object
	//

	Status = ApsDeQueuePacket(QueueObject, &Packet);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	switch (Packet->Type) {

		case APS_CTL_START:
			Status = ApsOnStart(Object, Packet);
			break;

		case APS_CTL_STOP:
			Status = ApsOnStop(Object, Packet);
			break;

		case APS_CTL_PAUSE:
			Status = ApsOnPause(Object, Packet);
			break;

		case APS_CTL_RESUME:
			Status = ApsOnResume(Object, Packet);
			break;
		
		case APS_CTL_MARK:
			Status = ApsOnMark(Object, Packet);
			break;

		default:
			ASSERT(0);
			break;
	}

	Packet->Status = Status;

	//
	// N.B. CompleteEvent is required for any packet
	//

	ASSERT(Packet->CompleteEvent != NULL);
	SetEvent(Packet->CompleteEvent);

	return Status;
}

ULONG
ApsOnStart(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PAPS_PORT Port;
	PBTR_MESSAGE_START Request;
	PBTR_MESSAGE_START_ACK Ack;
	PBTR_MESSAGE_START_STATUS HotpatchAck;

	ASSERT(Packet->Type == APS_CTL_START);

	//
	// Connect profiler port
	//

	Port = Object->PortObject;
	Status = ApsConnectPort(Port);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Send start message to target process
	//

	Request = (PBTR_MESSAGE_START)Port->Buffer;
	Request->Header.Type = MESSAGE_START;
	Request->Header.Length = sizeof(BTR_MESSAGE_START);

	Request->SharedData = Packet->Start.SharedData;
	Request->IndexFile = Packet->Start.IndexFile;
	Request->DataFile = Packet->Start.DataFile;
	Request->StackFile = Packet->Start.StackFile;
	Request->ReportFile= Packet->Start.ReportFile;
	Request->UnloadEvent = Packet->Start.UnloadEvent;
	Request->ExitEvent = Packet->Start.ExitProcessEvent;
	Request->ExitAckEvent = Packet->Start.ExitProcessAckEvent;
	Request->ControlEnd = Packet->Start.ControlEnd;

	RtlCopyMemory(&Request->Attribute, &Packet->Start.Attribute, 
		          sizeof(BTR_PROFILE_ATTRIBUTE));

	Status = ApsSendMessage(Port, &Request->Header, &Complete);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Wait for start ack packet
	//

	Ack = (PBTR_MESSAGE_START_ACK)Port->Buffer;
	Status = ApsGetMessage(Port, &Ack->Header, Port->BufferLength, &Complete);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Commit hotpatch to target process based on received hotpatch entries
	//

	Status = ApsCommitHotpatch(&Ack->Hotpatch[0], Ack->Count);
	
	//
	// Send hotpatch status to target process
	//

	HotpatchAck = (PBTR_MESSAGE_START_STATUS)Port->Buffer;
	HotpatchAck->Header.Type = MESSAGE_START_STATUS;
	HotpatchAck->Header.Length = sizeof(BTR_MESSAGE_START_STATUS);
	HotpatchAck->Status = Status;

	ApsSendMessage(Port, &HotpatchAck->Header, &Complete);
	return Status;
}

ULONG
ApsOnStop(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PAPS_PORT Port;
	PBTR_MESSAGE_STOP Request;
	PBTR_MESSAGE_STOP_ACK Ack;
	PBTR_MESSAGE_STOP_STATUS HotpatchAck;

	ASSERT(Packet->Type == APS_CTL_STOP);

	Port = Object->PortObject;

	//
	// Send stop message to target process
	//

	Request = (PBTR_MESSAGE_STOP)Port->Buffer;
	Request->Header.Type = MESSAGE_STOP;
	Request->Header.Length = sizeof(BTR_MESSAGE_STOP);

	Status = ApsSendMessage(Port, &Request->Header, &Complete);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Wait for stop ack packet
	//

	Ack = (PBTR_MESSAGE_STOP_ACK)Port->Buffer;
	Status = ApsGetMessage(Port, &Ack->Header, Port->BufferLength, &Complete);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Commit hotpatch to target process based on received hotpatch entries
	//

	Status = ApsCommitHotpatch(&Ack->Hotpatch[0], Ack->Count);
	
	//
	// Send hotpatch status to target process
	//

	HotpatchAck = (PBTR_MESSAGE_STOP_STATUS)Port->Buffer;
	HotpatchAck->Header.Type = MESSAGE_STOP_STATUS;
	HotpatchAck->Header.Length = sizeof(BTR_MESSAGE_STOP_STATUS);
	HotpatchAck->Status = Status;

	ApsSendMessage(Port, &HotpatchAck->Header, &Complete);

	//
	// Wait for unload event
	//

	WaitForSingleObject(Object->UnloadEvent, INFINITE);

	//
	// Write profile report
	//

	ApsWriteReport(Object);

	//
	// N.B. Return APS_STATUS_UNLOAD to force exit
	// the profile procedure
	//

	return APS_STATUS_UNLOAD;
}

ULONG
ApsOnPause(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PAPS_PORT Port;
	PBTR_MESSAGE_PAUSE Request;
	PBTR_MESSAGE_PAUSE_ACK Ack;

	ASSERT(Packet->Type == APS_CTL_PAUSE);

	Port = Object->PortObject;

	//
	// Send pause message to target process
	//

	Request = (PBTR_MESSAGE_PAUSE)Port->Buffer;
	Request->Header.Type = MESSAGE_PAUSE;
	Request->Header.Length = sizeof(BTR_MESSAGE_PAUSE);
	Request->Duration = Packet->Pause.Duration;

	Status = ApsSendMessage(Port, &Request->Header, &Complete);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Wait for pause ack packet
	//

	Ack = (PBTR_MESSAGE_PAUSE_ACK)Port->Buffer;
	Status = ApsGetMessage(Port, &Ack->Header, Port->BufferLength, &Complete);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	return Status;
}


ULONG
ApsOnResume(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PAPS_PORT Port;
	PBTR_MESSAGE_RESUME Request;
	PBTR_MESSAGE_RESUME_ACK Ack;

	ASSERT(Packet->Type == APS_CTL_RESUME);

	Port = Object->PortObject;

	//
	// Send resume message to target process
	//

	Request = (PBTR_MESSAGE_RESUME)Port->Buffer;
	Request->Header.Type = MESSAGE_RESUME;
	Request->Header.Length = sizeof(BTR_MESSAGE_RESUME);

	Status = ApsSendMessage(Port, &Request->Header, &Complete);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Wait for resume ack packet
	//

	Ack = (PBTR_MESSAGE_RESUME_ACK)Port->Buffer;
	Status = ApsGetMessage(Port, &Ack->Header, Port->BufferLength, &Complete);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	return Status;
}

ULONG
ApsOnMark(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	ULONG Complete;
	PAPS_PORT Port;
	PBTR_MESSAGE_MARK Request;
	PBTR_MESSAGE_MARK_ACK Ack;

	ASSERT(Packet->Type == APS_CTL_MARK);

	Port = Object->PortObject;

	//
	// Send mark message to target process
	//

	Request = (PBTR_MESSAGE_MARK)Port->Buffer;
	Request->Header.Type = MESSAGE_MARK;
	Request->Header.Length = sizeof(BTR_MESSAGE_MARK);

	Status = ApsSendMessage(Port, &Request->Header, &Complete);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Wait for mark ack packet
	//

	Ack = (PBTR_MESSAGE_MARK_ACK)Port->Buffer;
	Status = ApsGetMessage(Port, &Ack->Header, Port->BufferLength, &Complete);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	return Status;
}

ULONG
ApsOnExitProcess(
	__in PAPS_PROFILE_OBJECT Object
	)
{
	ULONG Status;
	ULONG Complete;
	PAPS_PORT Port;
	PBTR_MESSAGE_STOP Request;
	PBTR_MESSAGE_STOP_ACK Ack;
	PBTR_MESSAGE_STOP_STATUS HotpatchAck;

	Port = Object->PortObject;

	//
	// Send stop message to target process
	//

	Request = (PBTR_MESSAGE_STOP)Port->Buffer;
	Request->Header.Type = MESSAGE_STOP;
	Request->Header.Length = sizeof(BTR_MESSAGE_STOP);

	Status = ApsSendMessage(Port, &Request->Header, &Complete);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Wait for stop ack packet
	//

	Ack = (PBTR_MESSAGE_STOP_ACK)Port->Buffer;
	Status = ApsGetMessage(Port, &Ack->Header, Port->BufferLength, &Complete);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Commit hotpatch to target process based on received hotpatch entries
	//

	Status = ApsCommitHotpatch(&Ack->Hotpatch[0], Ack->Count);
	
	//
	// Send hotpatch status to target process
	//

	HotpatchAck = (PBTR_MESSAGE_STOP_STATUS)Port->Buffer;
	HotpatchAck->Header.Type = MESSAGE_STOP_STATUS;
	HotpatchAck->Header.Length = sizeof(BTR_MESSAGE_STOP_STATUS);
	HotpatchAck->Status = Status;

	ApsSendMessage(Port, &HotpatchAck->Header, &Complete);

	//
	// Wait for unload event
	//

	WaitForSingleObject(Object->UnloadEvent, INFINITE);

	//
	// Write profile report
	//

	ApsWriteReport(Object);

	if (Object->CallbackRoutine != NULL) {
		(*Object->CallbackRoutine)(Object, Object->CallbackContext, 
			                       APS_STATUS_EXITPROCESS);
	}

	return APS_STATUS_EXITPROCESS;
}

ULONG
ApsOnTerminated(
	__in PAPS_PROFILE_OBJECT Object
	)
{
	if (Object->CallbackRoutine != NULL) {
		(*Object->CallbackRoutine)(Object, Object->CallbackContext, 
			                       APS_STATUS_TERMINATED);
	}

	return APS_STATUS_TERMINATED;
}

VOID
ApsBuildAddressRangeByHotpatch(
	__in PBTR_HOTPATCH_ENTRY Hotpatch,
	__in ULONG Count,
	__out PBTR_ADDRESS_RANGE *Range 
	)
{
	ULONG Number;
	PBTR_ADDRESS_RANGE Address;

	Address = (PBTR_ADDRESS_RANGE)ApsMalloc(sizeof(BTR_ADDRESS_RANGE) * Count);

	for(Number = 0; Number < Count; Number += 1) {
		Address[Number].Address = (ULONG_PTR)Hotpatch[Number].Address;
		Address[Number].Size = Hotpatch[Number].Length;
	}

	*Range = Address;
}

ULONG
ApsWriteHotpatch(
	__in HANDLE ProcessHandle,
	__in PBTR_HOTPATCH_ENTRY Hotpatch,
	__in ULONG Count
	)
{
	ULONG Status;
	ULONG Result;
	ULONG Number;
	ULONG Old;
	ULONG Committed;
	SIZE_T Complete;
	PBTR_HOTPATCH_ENTRY Copy;

	Copy = (PBTR_HOTPATCH_ENTRY)ApsMalloc(sizeof(BTR_HOTPATCH_ENTRY) * Count);
	RtlZeroMemory(Copy, sizeof(BTR_HOTPATCH_ENTRY) * Count);

	Committed = 0;
	Status = TRUE;

	for(Number = 0; Number < Count; Number += 1) {

		//
		// Make the target page writable
		//

		Status = VirtualProtectEx(ProcessHandle, Hotpatch[Number].Address, 
			                      (SIZE_T)Hotpatch[Number].Length, 
			                      PAGE_EXECUTE_READWRITE, &Old);
		if (Status != TRUE) {
			break;
		}

		//
		// Copy old code bytes for recovery
		//

		Copy[Number].Address = Hotpatch[Number].Address;
		Copy[Number].Length = Hotpatch[Number].Length;
		
		Status = ReadProcessMemory(ProcessHandle, Copy[Number].Address,
						           &Copy[Number].Code[0], Copy[Number].Length, &Complete);
		if (Status != TRUE) {
			break;
		}

		Status = WriteProcessMemory(ProcessHandle, Hotpatch[Number].Address, 
			                        Hotpatch[Number].Code, Hotpatch[Number].Length, 
									&Complete);
		if (Status != TRUE) {
			break;
		}

		//
		// Recover target page protection
		//

		VirtualProtectEx(ProcessHandle, Hotpatch[Number].Address, 
			             (SIZE_T)Hotpatch[Number].Length, Old, NULL);

		Committed += 1;
	}


	if (Status != TRUE) {

		Result = GetLastError();

		//
		// Try best effort to recover old code bytes
		//

		for(Number = 0; Number < Committed; Number += 1) {

			Status = VirtualProtectEx(ProcessHandle, Copy[Number].Address, 
									  (SIZE_T)Copy[Number].Length, 
								      PAGE_EXECUTE_READWRITE, &Old);
			if (Status != TRUE) {
				continue;
			}

			WriteProcessMemory(ProcessHandle, Copy[Number].Address, 
			                   Copy[Number].Code, Copy[Number].Length, 
							   &Complete);

			VirtualProtectEx(ProcessHandle, Copy[Number].Address, 
							 (SIZE_T)Copy[Number].Length, 
						     Old, NULL);
		}

	} else {

		Result = APS_STATUS_OK;
	}

	ApsFree(Copy);
	return Result;
}

ULONG
ApsCommitHotpatch(
	__in PBTR_HOTPATCH_ENTRY Hotpatch,
	__in ULONG Count
	)
{
	ULONG Status;
	ULONG ProcessId;
	HANDLE ProcessHandle;
	HANDLE Handle;
	PAPS_PROCESS Process;
	PBTR_ADDRESS_RANGE Range;
	ULONG i, j;
	PULONG_PTR ThreadPc;
	PSYSTEM_THREAD_INFORMATION Thread;
	CONTEXT Context;
	BOOLEAN Complete;
	PAPS_PROFILE_OBJECT Object;

	Object = ApsCurrentProfile();

	ProcessId = Object->ProcessId;
	ProcessHandle = Object->ProcessHandle;
	Context.ContextFlags = CONTEXT_CONTROL;

	//
	// Build address range to be patched 
	//

	Complete = FALSE;
	ApsBuildAddressRangeByHotpatch(Hotpatch, Count, &Range);

	while (TRUE) {

		//
		// Transactional suspend target process
		//

		ApsSuspendProcess(ProcessHandle);

		//
		// Build active thread list and query its thread context
		//

		ApsQueryThreadList(ProcessId, &Process);
		ThreadPc = (PULONG_PTR)ApsMalloc(sizeof(ULONG_PTR) * Process->ThreadCount);

		for(i = 0, j = 0; i < Process->ThreadCount; i++) {

			Thread = &Process->Thread[i];
			Handle = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, 
				                FALSE, HandleToUlong(Thread->ClientId.UniqueThread));
			if (!Handle) {
				continue;
			}

			if (GetThreadContext(Handle, &Context)) {

#if defined (_M_IX86)
				ThreadPc[j] = Context.Eip;
#elif defined (_M_X64)
				ThreadPc[j] = Context.Rip;
#endif
				j += 1;
			}

			CloseHandle(Handle);

		}

		//
		// Check whether any thread's pc is executing the area to be patched
		//

		if (!ApsIsExecutingPc(ThreadPc, j, Range, Count)) {

			//
			// It's safe to write patch 
			//

			Status = ApsWriteHotpatch(ProcessHandle, Hotpatch, Count);
			Complete = TRUE;
		}

		ApsResumeProcess(ProcessHandle);

		ApsFreeProcess(Process);
		ApsFree(ThreadPc);

		if (Complete) {
			break;

		} else {

			//
			// Rlease target threads to try again on next schedule cycle
			//

			SwitchToThread();
		}
	}

	if (Range != NULL) {
		ApsFree(Range);
	}

	return Status;
}

BOOLEAN
ApsIsProfileTerminated(
	__in PAPS_PROFILE_OBJECT Object
	)
{
	if (Object->ExitStatus == APS_STATUS_TERMINATED ||
		Object->ExitStatus == APS_STATUS_EXITPROCESS ) {
		return TRUE;
	}

	return FALSE;
}