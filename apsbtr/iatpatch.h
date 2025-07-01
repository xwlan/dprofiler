#ifndef _IAT_PATCH_H_
#define _IAT_PATCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "ntapi.h"

#define MAX_IAT_PATCH 32

typedef struct _BTR_IAT_PATCH {
	PCSTR From;
	union {
		PCSTR Function;
		USHORT Ordinal;
	};
	PVOID Callback;
	PVOID Address;
	PVOID PatchAddress;
	ULONG_PTR Copy;
} BTR_IAT_PATCH, *PBTR_IAT_PATCH;

typedef struct _BTR_MODULE {
	PVOID Base;
	SIZE_T Size;
	ULONG PatchCount;
	WCHAR BaseName[64];
	WCHAR Path[MAX_PATH];
	BTR_IAT_PATCH Patch[MAX_IAT_PATCH];
} BTR_MODULE, *PBTR_MODULE;


VOID
BtrInitializeIatMode(
	VOID
	);

ULONG
BtrBuildModuleList(
	VOID
	);

PBTR_MODULE
BtrGetModule(
	IN PCWSTR BaseName
	);

ULONG
BtrRegisterDllLoadCallback(
	IN BOOLEAN Register	
	);

VOID CALLBACK
BtrDllNotification(
	IN ULONG Reason,
	IN PLDR_DLL_NOTIFICATION_DATA Data,
	IN PVOID Context
	);

BOOLEAN
BtrApplyPatchForDll(
	IN PBTR_MODULE Dll,
	IN PBTR_IAT_PATCH Patch,
	IN ULONG Count,
	IN BOOLEAN Apply
	);

BOOLEAN
BtrApplyPatchForDllByAddress(
	IN PBTR_MODULE Dll,
	IN PBTR_IAT_PATCH Patch,
	IN ULONG Count,
	IN BOOLEAN Apply
	);

BOOLEAN
BtrApplyPatchSingle(
	IN ULONG_PTR Base,
	IN PBTR_IAT_PATCH Patch,
	IN PIMAGE_IMPORT_DESCRIPTOR Entry,
	IN BOOLEAN Apply
	);

BOOLEAN
BtrApplyPatchSingleByAddress(
	IN ULONG_PTR Base,
	IN PBTR_IAT_PATCH Patch,
	IN PIMAGE_IMPORT_DESCRIPTOR Entry,
	IN BOOLEAN Apply
	);

BOOLEAN
BtrWriteProtectedPointer(
	IN PVOID Address,
	IN PVOID Value
	);

BOOLEAN
BtrRollbackPatchForDll(
	IN PBTR_MODULE Dll
	);

BOOLEAN
BtrApplyIatPatch(
	IN PBTR_IAT_PATCH Patch,
	IN ULONG Count,
	IN BOOLEAN Apply
	);

VOID
BtrRollbackAndCleanup(
	VOID
	);

#ifdef __cplusplus
}
#endif
#endif