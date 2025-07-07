#include "btr.h"
#include "apsbtr.h"
#include "hal.h"
#include "util.h"
#include <tlhelp32.h>
#include "ntapi.h"
#include "iatpatch.h"
#include "heap.h"

#define MAX_DLL_COUNT 512 

ULONG BtrDllCount;

BTR_MODULE BtrDllTable[MAX_DLL_COUNT];

PVOID BtrLdrCookie;

PCWSTR BtrIgnoreDll[] = {
	L"ntdll.dll",
	L"kernel32.dll",
	L"gdi32.dll",
	L"user32.dll",
	L"kernelbase.dll",
	L"advapi32.dll"
	L"rpcrt4.dll",
	L"ole32.dll", 
	L"psapi.dll",
	L"ws2_32.dll",
	L"mswsock.dll"
};

int BtrIgnoreDllCount = ARRAYSIZE(BtrIgnoreDll);

BOOLEAN
BtrIsIgnoreDll(
	IN PCWSTR Name
	)
{
	int i;

	for (i = 0; i < BtrIgnoreDllCount; i++) {
		if (!_wcsicmp(Name, BtrIgnoreDll[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

VOID
BtrInitializeIatMode(
	VOID
	)
{
	if (!BtrDllCount) {
		BtrBuildModuleList();
	}
}

ULONG
BtrBuildModuleList(
	VOID
	)
{
	ULONG Required;
	ULONG Status;
	ULONG Count;
	ULONG i, j;
	HMODULE* Modules;
	PBTR_MODULE Dll;
	HANDLE ProcessHandle;
	MODULEINFO Info;
	ULONG ProcessId;
	WCHAR Path[MAX_PATH];
	WCHAR BaseName[MAX_PATH];

	Modules = BtrMalloc(sizeof(HMODULE) * MAX_DLL_COUNT);
	if (!Modules) {
		return BTR_E_OUTOFMEMORY;
	}

	ProcessId = GetCurrentProcessId();
	ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}

	Status = EnumProcessModules(ProcessHandle, Modules,
								sizeof(HMODULE) * MAX_DLL_COUNT, &Required);
	if (!Status) {
		Status = GetLastError();
		BtrFree(Modules);
		CloseHandle(ProcessHandle);
		return Status;
	}

	Count = Required / sizeof(HMODULE);

	for (i = 0, j = 0; i < Count; i++) {

		Dll = &BtrDllTable[j];

		GetModuleInformation(ProcessHandle, Modules[i], &Info, sizeof(Info));
		if ((ULONG_PTR)Info.lpBaseOfDll == BtrDllBase) {
			continue;
		}

		GetModuleFileNameW(Modules[i], Path, MAX_PATH);
		BtrGetBaseNameW(Path, BaseName, 64);
		if (BtrIsIgnoreDll(BaseName)) {
			continue;
		}

		Dll->Base = Info.lpBaseOfDll;
		Dll->Size = Info.SizeOfImage;
		wcscpy_s(Dll->Path, MAX_PATH, Path);
		wcscpy_s(Dll->BaseName, 64, BaseName);

		j += 1;
	}

	BtrDllCount = j;

	BtrFree(Modules);
	CloseHandle(ProcessHandle);
	return S_OK;
}

PBTR_MODULE
BtrGetModule(
	IN PCWSTR BaseName
	)
{
	for (ULONG i = 0; i < BtrDllCount; i++) {
		if (!_wcsicmp(BtrDllTable[i].BaseName, BaseName)) {
			return &BtrDllTable[i];
		}
	}

	return NULL;
}

ULONG
BtrRegisterDllLoadCallback(
	IN BOOLEAN Register	
	)
{
	ULONG Status;
	if (Register) {
		Status = LdrRegisterDllNotification(0, BtrDllNotification, NULL, &BtrLdrCookie);
	}
	else {
		ASSERT(BtrLdrCookie != NULL);
		Status = LdrUnregisterDllNotification(BtrLdrCookie);
	}
	return Status;
}

VOID CALLBACK
BtrDllNotification(
	IN ULONG Reason,
	IN PLDR_DLL_NOTIFICATION_DATA Data,
	IN PVOID Context
	)
{
	PBTR_MODULE Entry;
	BOOLEAN Same;
	BOOLEAN ExecuteCallback = FALSE;
	WCHAR BaseName[64] = { 0 };

	if (Reason == LDR_DLL_NOTIFICATION_REASON_UNLOADED) {
		for (ULONG i = 0; i < BtrDllCount; i++) {
			Entry = &BtrDllTable[i];
			Same = !_wcsicmp(Entry->Path, Data->Loaded.FullDllName->Buffer);
			if (Same) {

				//
				// Execute dll unload callback to cleanup IAT patch information
				//

				(*BtrProfileObject->DllLoadCallback)(BtrProfileObject, Entry, FALSE);

				//
				// Move the last entry if any to to fill the hole
				//

				if (i != BtrDllCount - 1) {
					*Entry = BtrDllTable[BtrDllCount - 1];
				}
				BtrDllCount -= 1;
				return;
			}
		}
		return;
	}

	if (Reason == LDR_DLL_NOTIFICATION_REASON_LOADED) {

		//
		// If it's ignored dll, nothing to do
		//

		RtlCopyMemory(BaseName, Data->Loaded.BaseDllName->Buffer, 
						Data->Loaded.BaseDllName->Length * sizeof(WCHAR));

		if (BtrIsIgnoreDll(BaseName)) {
			return;
		}

		for (int i = 0; i < MAX_DLL_COUNT; i++) {
			Entry = &BtrDllTable[i];
			if (!Entry->Base) {
				Entry->Base = Data->Loaded.DllBase;
				Entry->Size = Data->Loaded.SizeOfImage;
				wcscpy_s(Entry->Path, MAX_PATH, Data->Loaded.FullDllName->Buffer);

				//
				// Execute dll load callback to apply IAT patch
				//

				(*BtrProfileObject->DllLoadCallback)(BtrProfileObject, Entry, TRUE);
				BtrDllCount += 1;
				break;
			}
		}
	}
}

BOOLEAN
BtrApplyIatPatch(
	IN PBTR_IAT_PATCH Patch,
	IN ULONG Count,
	IN BOOLEAN Apply
	)
{
	PBTR_MODULE Module;
	ULONG i;

	for (i = 0; i < BtrDllCount; i++) {
		Module = &BtrDllTable[i];
		BtrApplyPatchForDllByAddress(Module, Patch, Count, TRUE);
	}

	return TRUE;
}

VOID
BtrCopyPatchToDll(
	IN PBTR_MODULE Module,
	IN PBTR_IAT_PATCH Patch
	)
{
	if (Module->PatchCount < MAX_IAT_PATCH - 1) {
		Module->Patch[Module->PatchCount] = *Patch;
		Module->PatchCount += 1;
		return;
	}

	ASSERT(0);
}

BOOLEAN
BtrApplyPatchForDll(
	IN PBTR_MODULE Dll,
	IN PBTR_IAT_PATCH Patch,
	IN ULONG Count,
	IN BOOLEAN Apply
	)
{
	PIMAGE_NT_HEADERS Head;
	ULONG_PTR Base;
	PCHAR DllName;
	BOOLEAN Status;

	PIMAGE_IMPORT_DESCRIPTOR Entry;
	ULONG Number;


	Head = ImageNtHeader(Dll->Base);
	if (!Head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ||
		!Head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
		return FALSE;
	}

	Base = (ULONG_PTR)Dll->Base;

	for (Number = 0; Number < Count; Number += 1) {

		Entry = (PIMAGE_IMPORT_DESCRIPTOR)(Base + Head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		for (; (Entry->Name != 0); Entry += 1) {

			DllName = (PCHAR)(Base + Entry->Name);

			if (_stricmp(DllName, Patch[Number].From) != 0) {
				continue;
			}

			// 
			// Apply the patch
			//
			
			Status = BtrApplyPatchSingleByAddress(Base, &Patch[Number], Entry, Apply);
			if (Status) {
				BtrCopyPatchToDll(Dll, &Patch[Number]);
			}
		}
	}

	return TRUE;
}

BOOLEAN
BtrApplyPatchForDllByAddress(
	IN PBTR_MODULE Dll,
	IN PBTR_IAT_PATCH Patch,
	IN ULONG Count,
	IN BOOLEAN Apply
	)
{
	PIMAGE_NT_HEADERS Head;
	ULONG_PTR Base;
	BOOLEAN Status;

	PIMAGE_IMPORT_DESCRIPTOR Entry;
	ULONG Number;


	Head = ImageNtHeader(Dll->Base);
	if (!Head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ||
		!Head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
		return FALSE;
	}

	Base = (ULONG_PTR)Dll->Base;

	for (Number = 0; Number < Count; Number += 1) {

		ASSERT(Patch[Number].Address != NULL);

		Entry = (PIMAGE_IMPORT_DESCRIPTOR)(Base + Head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		for (; (Entry->Name != 0); Entry += 1) {

			Status = BtrApplyPatchSingleByAddress(Base, &Patch[Number], Entry, Apply);
			if (Status) {
				BtrCopyPatchToDll(Dll, &Patch[Number]);
			}
		}
	}

	return TRUE;
}

BOOLEAN
BtrApplyPatchSingleByAddress(
	IN ULONG_PTR Base,
	IN PBTR_IAT_PATCH Patch,
	IN PIMAGE_IMPORT_DESCRIPTOR Entry,
	IN BOOLEAN Apply
	)
{
	PIMAGE_THUNK_DATA Bound;
	BOOLEAN Status = FALSE;

	ASSERT(Patch->Address != NULL);

	Bound = (PIMAGE_THUNK_DATA)(Base + Entry->FirstThunk);
	while (Bound->u1.Function) {
		if ((ULONG_PTR)Bound->u1.Function == (ULONG_PTR)Patch->Address) {
			Status = TRUE;
			if (Apply) {
				Patch->Copy = Bound->u1.Function;
				Patch->PatchAddress = &Bound->u1.Function;
				BtrWriteProtectedPointer(&Bound->u1.Function, Patch->Callback);
			}
			else {
				BtrWriteProtectedPointer(&Bound->u1.Function, (PVOID)Patch->Copy);
			}
		}
		Bound += 1;
	}

	return Status;
}

BOOLEAN
BtrApplyPatchSingle(
	IN ULONG_PTR Base,
	IN PBTR_IAT_PATCH Patch, 
	IN PIMAGE_IMPORT_DESCRIPTOR Entry,
	IN BOOLEAN Apply 
	)
{
	BOOLEAN Status = FALSE;
	PIMAGE_THUNK_DATA Unbound;
	PIMAGE_THUNK_DATA Bound;
	PIMAGE_IMPORT_BY_NAME Name;

	Unbound = (PIMAGE_THUNK_DATA)(Base + Entry->OriginalFirstThunk);
	Bound = (PIMAGE_THUNK_DATA)(Base + Entry->FirstThunk);

	for ( ; Unbound->u1.Function != 0; Unbound += 1, Bound += 1) {

		if (IMAGE_ORDINAL_FLAG == (Unbound->u1.Ordinal & IMAGE_ORDINAL_FLAG)){

			if ((USHORT)Patch->Function == LOWORD(Unbound->u1.Ordinal)) {
				if (Apply) {
					Patch->Copy = Bound->u1.Function;
					Patch->PatchAddress = &Bound->u1.Function;
					BtrWriteProtectedPointer(&Bound->u1.Function, Patch->Callback);
				} else {
					BtrWriteProtectedPointer(&Bound->u1.Function, (PVOID)Patch->Copy);
				}
				Status = TRUE;
				break;
			}
		}
		else {
			Name = (PIMAGE_IMPORT_BY_NAME)(Base + Unbound->u1.AddressOfData);
			if (!_stricmp(Name->Name, Patch->Function)) {
				if (Apply) {
					Patch->Copy = Bound->u1.Function;
					Patch->PatchAddress = &Bound->u1.Function;
					BtrWriteProtectedPointer(&Bound->u1.Function, Patch->Callback);
				}
				else {
					BtrWriteProtectedPointer(&Bound->u1.Function, (PVOID)Patch->Copy);
				}
				Status = TRUE;
				break;
			}
		}
	}

	if (Status) {
		
	}
	return Status;
}

BOOLEAN
BtrWriteProtectedPointer(
	IN PVOID Address,
	IN PVOID Value
	)
{
	BOOL Status;
	ULONG Protect;

	Status = VirtualProtect(Address, sizeof(PVOID), PAGE_EXECUTE_READWRITE, &Protect);
	if (!Status) {
		return FALSE;
	}

	__try {
		*(PULONG_PTR)Address = (ULONG_PTR)Value;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

		//
		// This error should be written to runtime log file
		//

		return FALSE;
	}

	VirtualProtect(Address, sizeof(PVOID), Protect, &Protect);
	return TRUE;
}

BOOLEAN
BtrRollbackPatchForDll(
	IN PBTR_MODULE Dll
	)
{
	ULONG i;
	PBTR_IAT_PATCH Patch;

	for (i = 0; i < Dll->PatchCount; i += 1) {
		Patch = &Dll->Patch[i];
		if (*(PULONG_PTR)Patch->PatchAddress == (ULONG_PTR)Patch->Callback) {
			BtrWriteProtectedPointer(Patch->PatchAddress, (PVOID)Patch->Copy);
		}
	}

	return TRUE;
}

VOID
BtrRollbackAndCleanup(
	VOID
	)
{
	ULONG i;

	for (i = 0; i < BtrDllCount; i++) {
		BtrRollbackPatchForDll(&BtrDllTable[i]);
	}
}

