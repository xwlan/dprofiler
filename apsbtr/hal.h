//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _HAL_H_
#define _HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ntapi.h"
#include "apsprofile.h"

//
// HAL variables
//

extern ULONG BtrNtMajorVersion;
extern ULONG BtrNtMinorVersion;
extern ULONG_PTR BtrMaximumUserAddress;
extern ULONG_PTR BtrMinimumUserAddress;
extern ULONG_PTR BtrAllocationGranularity;
extern ULONG_PTR BtrPageSize;
extern ULONG BtrNumberOfProcessors;

extern ULONG_PTR BtrDllBase;
extern ULONG_PTR BtrDllSize;

extern RTLINITANSISTRING RtlInitAnsiString;
extern RTLFREEANSISTRING RtlFreeAnsiString;
extern RTLINITUNICODESTRING RtlInitUnicodeString;
extern RTLFREEUNICODESTRING RtlFreeUnicodeString;
extern RTLUNICODESTRINGTOANSISTRING RtlUnicodeStringToAnsiString;
extern RTLANSISTRINGTOUNICODESTRING RtlAnsiStringToUnicodeString;

extern RTLCREATEUSERTHREAD RtlCreateUserThread;
extern RTLEXITUSERTHREAD RtlExitUserThread;

extern NTQUERYSYSTEMINFORMATION  NtQuerySystemInformation;
extern NTQUERYINFORMATIONPROCESS NtQueryInformationProcess;
extern NTQUERYINFORMATIONTHREAD  NtQueryInformationThread;

extern double BtrMillisecondPerHardwareTick;

ULONG
BtrInitializeHal(
	VOID
	);

VOID
BtrUninitializeHal(
	VOID
	);

BOOLEAN
BtrLockLoaderLock(
	OUT PULONG Cookie	
	);

VOID
BtrUnlockLoaderLock(
	IN ULONG Cookie	
	);

VOID
BtrPrepareUnloadDll(
	IN PVOID Address 
	);

ULONG
BtrQueryProcessInformation(
	IN ULONG ProcessId,
	OUT PSYSTEM_PROCESS_INFORMATION *Information
	);

ULONG
BtrAllocateQueryBuffer(
	IN SYSTEM_INFORMATION_CLASS InformationClass,
	OUT PVOID *Buffer,
	OUT PULONG BufferLength,
	OUT PULONG ActualLength
	);

VOID
BtrReleaseQueryBuffer(
	IN PVOID StartVa
	);
	
PVOID
BtrQueryTebAddress(
	__in HANDLE ThreadHandle
	);

ULONG 
BtrCurrentProcessor(
	VOID
	);

PVOID
HalGetModulePtr(
	__in PCSTR Name
	);

BOOLEAN 
HalIsWow64Process(
	VOID
	);

PVOID
HalGetProcedureAddress(
	__in PVOID Module,
	__in PCSTR Name
	);

BTR_CPU_TYPE
HalGetCpuType(
	VOID
	);

BTR_TARGET_TYPE
HalGetTargetType(
	VOID
	);

//
// inlined short routines
//

FORCEINLINE
ULONG
BtrCurrentThreadId(
	VOID
	)
{
	PTEB Teb = NtCurrentTeb();
	return PtrToUlong(Teb->Cid.UniqueThread);
}

FORCEINLINE
ULONG
BtrCurrentProcessId(
	VOID
	)
{
	PTEB Teb = NtCurrentTeb();
	return PtrToUlong(Teb->Cid.UniqueProcess);
}

FORCEINLINE
PULONG_PTR
BtrGetCurrentStackBase(
	VOID
	)
{
	return ((PULONG_PTR)((NT_TIB *)NtCurrentTeb())->StackBase) - 1;
}

FORCEINLINE
PULONG_PTR
BtrGetCurrentStackLimit(
	VOID
	)
{
	return ((PULONG_PTR)((NT_TIB *)NtCurrentTeb())->StackLimit) - 1;
}

FORCEINLINE
BOOLEAN 
BtrIsAcspValid(
	VOID
	)
{
	//
	// N.B. Windows XP embed ACSP in TEB, just return TRUE
	//

	if (BtrNtMajorVersion == 5 && BtrNtMinorVersion == 1) {
		return TRUE;
	}

	return NtCurrentTeb()->ActivationContextStackPointer ? TRUE : FALSE;
}


#if defined(_M_X64)

//
// RtlLookupFunctionTable
//

typedef PRUNTIME_FUNCTION
(NTAPI *RTLLOOKUPFUNCTIONTABLE)(
    IN PVOID ControlPc,
    OUT PVOID *ImageBase,
    OUT PULONG SizeOfTable
    );

extern RTLLOOKUPFUNCTIONTABLE RtlLookupFunctionTable;

#endif

#if defined(_M_IX86)

PVOID KiFastSystemCallRet;

#endif

#ifdef __cplusplus
}
#endif
#endif