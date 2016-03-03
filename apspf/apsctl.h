//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2016
//

#ifndef _APS_CTL_H_
#define _APS_CTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "apsport.h"
#include "apsqueue.h"
#include "aps.h"

typedef struct _APS_ADDRESS {
	LIST_ENTRY ListEntry;
	ULONG_PTR StartVa;
	ULONG_PTR EndVa;
} APS_ADDRESS, *PAPS_ADDRESS;

typedef ULONG
(*APS_PROFILE_CALLBACK)(
	__in struct _APS_PROFILE_OBJECT *Object,
	__in PVOID Context,
	__in ULONG Status
	);

typedef ULONG
(*APS_CREATECOUNTER_ROUTINE)(
	__in struct _APS_PROFILE_OBJECT *Object,
	__in PCWSTR SymbolPath,
	__in HANDLE SymbolHandle
	);

typedef enum _APS_PROFILE_STATE {
	PROFILE_STATE_BOOTSTRAP,
	PROFILE_STATE_INIT_PAUSED,
	PROFILE_STATE_PROFILING,
	PROFILE_STATE_PAUSED,
	PROFILE_STATE_COMPLETED,
	PROFILE_STATE_ANALYZING,
} APS_PROFILE_STATE, *PAPS_PROFILE_STATE;

typedef struct _APS_PROFILE_OBJECT {

	APS_CRITICAL_SECTION Lock;
	APS_PROFILE_STATE State;

	BTR_PROFILE_TYPE Type;
	BTR_PROFILE_MODE Mode;
	BTR_PROFILE_ATTRIBUTE Attribute;

	FILETIME EnterTime;
	FILETIME ExitTime;
	
	ULONG SessionId;
	ULONG ProcessId;
	HANDLE ProcessHandle;
	HANDLE ProfileThreadHandle;

	//
	// Profile control information 
	//
	
	HANDLE SharedDataObject;
	PBTR_SHARED_DATA SharedData;

	//
	// File handles
	//

	HANDLE IndexFileHandle;
	HANDLE DataFileHandle;
	HANDLE StackFileHandle;
	HANDLE ReportFileHandle;

	HANDLE IoObjectFile;
	HANDLE IoIrpFile;
	HANDLE IoNameFile;

	//
	// Control event object
	//

	HANDLE CommandEvent;
	HANDLE StartEvent;
	HANDLE StopEvent;
	HANDLE UnloadEvent;
	HANDLE ExitProcessEvent;
	HANDLE ExitProcessAckEvent;

	WCHAR Uuid[64];
	WCHAR IndexPath[MAX_PATH];
	WCHAR DataPath[MAX_PATH];
	WCHAR StackPath[MAX_PATH];
	WCHAR ReportPath[MAX_PATH];

	WCHAR ImagePath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];
	WCHAR UserName[MAX_PATH];

	PAPS_QUEUE QueueObject;
	PAPS_PORT PortObject;

	ULONG ExitStatus;

	APS_PROFILE_CALLBACK CallbackRoutine;
	PVOID CallbackContext;

	APS_CREATECOUNTER_ROUTINE CreateCounter;

} APS_PROFILE_OBJECT, *PAPS_PROFILE_OBJECT;

ULONG
ApsInitializeProfile(
	VOID
	);

ULONG
ApsCreateProfile(
	__out PAPS_PROFILE_OBJECT *Profile,
	__in BTR_PROFILE_MODE Mode,
	__in BTR_PROFILE_TYPE Type,
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in PWSTR ReportName
	);

ULONG
ApsStartProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

ULONG
ApsPauseProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

ULONG
ApsResumeProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

ULONG
ApsMarkProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

ULONG
ApsStopProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

VOID
ApsCloseProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

VOID
ApsRegisterCallback(
	__in PAPS_PROFILE_OBJECT Object,
	__in APS_PROFILE_CALLBACK Callback,
	__in PVOID Context
	);

ULONG CALLBACK
ApsProfileProcedure(
	__in PVOID Context
	);

ULONG
ApsLoadLibrary(
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in PWSTR DllPath
	);

BOOLEAN
ApsIsExecutingPc(
	__in PULONG_PTR Pc,
	__in ULONG PcCount,
	__in PBTR_ADDRESS_RANGE Range,
	__in ULONG RangeCount
	);

ULONG
ApsUnregisterProfile(
	__in PAPS_PROFILE_OBJECT Profile
	);

//
// MM Profiling 
//

ULONG
ApsCreateMmProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	);

ULONG
ApsCreateIoProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	);

ULONG
ApsCreateCcrProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	);

ULONG
ApsOnCommand(
	__in PAPS_PROFILE_OBJECT Object
	);

ULONG
ApsOnStart(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	);

ULONG
ApsOnStop(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	);

ULONG
ApsOnPause(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	);

ULONG
ApsOnResume(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	);

ULONG
ApsOnMark(
	__in PAPS_PROFILE_OBJECT Object,
	__in PAPS_QUEUE_PACKET Packet
	);

ULONG
ApsOnExitProcess(
	__in PAPS_PROFILE_OBJECT Object
	);

ULONG
ApsOnTerminated(
	__in PAPS_PROFILE_OBJECT Object
	);

VOID
ApsBuildAddressRangeByHotpatch(
	__in PBTR_HOTPATCH_ENTRY Hotpatch,
	__in ULONG Count,
	__out PBTR_ADDRESS_RANGE *Range 
	);

ULONG
ApsCommitHotpatch(
	__in PBTR_HOTPATCH_ENTRY Hotpatch,
	__in ULONG Count
	);

ULONG
ApsWriteHotpatch(
	__in HANDLE ProcessHandle,
	__in PBTR_HOTPATCH_ENTRY Hotpatch,
	__in ULONG Count
	);

BOOLEAN
ApsIsProfileTerminated(
	__in PAPS_PROFILE_OBJECT Object
	);

extern WCHAR ApsLogPath[MAX_PATH];
extern WCHAR ApsLocalSymPath[MAX_PATH];
extern WCHAR ApsDllPath[MAX_PATH];

#ifdef __cplusplus
}
#endif
#endif
