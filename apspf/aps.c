//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "aps.h"
#include "apsdefs.h"
#include <stdio.h>
#include <psapi.h>
#include <strsafe.h>
#include <time.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <rpc.h>
#include "apspri.h"
#include "apsctl.h"
#include "apslog.h"

#pragma comment(lib, "rpcrt4.lib")

BOOLEAN ApsIsInitialized = FALSE;
WCHAR ApsCurrentPath[MAX_PATH];
CHAR ApsCurrentPathA[MAX_PATH];

//
// System Information
//

ULONG ApsMajorVersion;
ULONG ApsMinorVersion;
ULONG ApsPageSize;
ULONG ApsProcessorNumber;

//
// For 3GB option of 32 bits system
//

PVOID ApsMaximumUserAddress;

//
// Milliseconds per hardware tick
//

double ApsSecondPerHardwareTick;

//
// NT API
//

NTQUERYSYSTEMINFORMATION  NtQuerySystemInformation;
NTQUERYINFORMATIONPROCESS NtQueryInformationProcess;
NTQUERYINFORMATIONTHREAD  NtQueryInformationThread;

NTSUSPENDPROCESS NtSuspendProcess;
NTRESUMEPROCESS  NtResumeProcess;

RTLCREATEUSERTHREAD RtlCreateUserThread;

//
// Compile time process type flag
//

#if defined (_M_IX86)
BOOLEAN ApsIs64Bits = FALSE;

#elif defined (_M_X64)
BOOLEAN ApsIs64Bits = TRUE;

#endif

//
// Runtime process type flag
//

BOOLEAN ApsIsWow64;

typedef BOOL 
(WINAPI *ISWOW64PROCESS)(
	__in HANDLE, 
	__out PBOOL
	);

ISWOW64PROCESS ApsIsWow64ProcessPtr;


#define APS_OPEN_PROCESS_FLAG (PROCESS_QUERY_INFORMATION |  \
                               PROCESS_CREATE_THREAD     |  \
							   PROCESS_VM_OPERATION      |  \
							   PROCESS_VM_WRITE)
		
//
// DllMain
//

BOOL WINAPI 
DllMain(
	IN HMODULE DllHandle,
    IN ULONG Reason,
	IN PVOID Reserved
	)
{
	BOOL Status = TRUE;

	switch (Reason) {

	case DLL_PROCESS_ATTACH:
		if (ApsInitialize(0) != APS_STATUS_OK) {
			Status = FALSE;
		}
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;

	}

	return Status;
}

//
// N.B. ApsInitialize must be called before call any Aps routine
//

ULONG
ApsInitialize(
	__in ULONG Flag	
	)
{
	ULONG Status;
	SYSTEM_INFO Information;
	OSVERSIONINFO Version = {0};
	LARGE_INTEGER HardwareFrequency = {0};
	WCHAR Path[MAX_PATH];
	USHORT Length;
    HANDLE DllHandle;

	if (ApsIsInitialized) {
		return APS_STATUS_OK;
	}

	//
	// Get current platform information
	//

	Version.dwOSVersionInfoSize = sizeof(Version);
	GetVersionEx(&Version);

	ApsMajorVersion = Version.dwMajorVersion;
	ApsMinorVersion = Version.dwMinorVersion;

	GetSystemInfo(&Information);
	ApsPageSize = Information.dwPageSize;
	ApsProcessorNumber = Information.dwNumberOfProcessors;
	ApsMaximumUserAddress = Information.lpMaximumApplicationAddress;

	//
	// Check whether it's wow64 process if it's 64 bits system 
	//

	ApsIsWow64ProcessPtr = (ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), 
		                                                  "IsWow64Process");
	if (!ApsIs64Bits) {
		ApsIsWow64 = ApsIsWow64Process(GetCurrentProcess());
	}

	//
	// Compute hardware clock accuracy
	//

	QueryPerformanceFrequency(&HardwareFrequency);
	ApsSecondPerHardwareTick = 1.0 / HardwareFrequency.QuadPart;

	//
	// Enable debug privilege
	//

	Status = ApsAdjustPrivilege(SE_DEBUG_NAME, TRUE);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Get system routine address
	//

	Status = ApsGetSystemRoutine();
	if (!Status) {
		return APS_STATUS_ERROR;
	}

	//
	// Get runtime dll path
	//

	ApsGetProcessPath(ApsCurrentPath, MAX_PATH, &Length);
    StringCchCopy(Path, MAX_PATH, ApsCurrentPath);
	StringCchPrintf(ApsDllPath, MAX_PATH, L"%s\\apsbtr.dll", Path);

	//
	// Create log path
	//

	StringCchPrintf(ApsLogPath, MAX_PATH, L"%s\\log", Path);
	Status = CreateDirectory(ApsLogPath, NULL);
	if (!Status) { 
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			return APS_STATUS_ERROR;
		}
	}

	//
	// Create local sym path
	//

	StringCchPrintf(ApsLocalSymPath, MAX_PATH, L"%s\\sym", Path);
	Status = CreateDirectory(ApsLocalSymPath, NULL);
	if (!Status) { 
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			return APS_STATUS_ERROR;
		}
	}

	Status = ApsInitializeLog(APS_FLAG_LOGGING);
	ApsIsInitialized = TRUE;
	return APS_STATUS_OK;
}

ULONG
ApsUninitialize(
	VOID
	)
{
	ApsUninitializeLog();
	return APS_STATUS_OK;
}

BOOLEAN
ApsGetSystemRoutine(
	VOID
	)
{
	HMODULE DllHandle;

	//
	// Get system routine address
	//

	DllHandle = LoadLibrary(L"ntdll.dll");
	if (!DllHandle) {
		return FALSE;
	}

	NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(DllHandle, "NtQuerySystemInformation");
	if (!NtQuerySystemInformation) {
		return FALSE;
	}

	NtQueryInformationProcess = (NTQUERYINFORMATIONPROCESS)GetProcAddress(DllHandle, "NtQueryInformationProcess");
	if (!NtQueryInformationProcess) {
		return FALSE;
	}

	NtQueryInformationThread = (NTQUERYINFORMATIONTHREAD)GetProcAddress(DllHandle, "NtQueryInformationThread");
	if (!NtQueryInformationThread) {
		return FALSE;
	}

	NtSuspendProcess = (NTSUSPENDPROCESS)GetProcAddress(DllHandle, "NtSuspendProcess");
	if (!NtSuspendProcess) {
		return FALSE;
	}

	NtResumeProcess = (NTRESUMEPROCESS)GetProcAddress(DllHandle, "NtResumeProcess");
	if (!NtResumeProcess) {
		return FALSE;
	}

	if (ApsIsWindowsXPAbove()) {
		RtlCreateUserThread = (RTLCREATEUSERTHREAD)GetProcAddress(DllHandle, "RtlCreateUserThread");
		if (!RtlCreateUserThread) {
			return FALSE;
		}
	}
	
	return TRUE;
}

BOOLEAN 
ApsIsWow64Process(
	__in HANDLE ProcessHandle	
	)
{
    BOOL IsWow64 = FALSE;
   
    if (ApsIsWow64ProcessPtr) {
        ApsIsWow64ProcessPtr(ProcessHandle, &IsWow64);
    }
    return (BOOLEAN)IsWow64;
}

BOOLEAN
ApsIs64BitsProcess(
	VOID
	)
{
	return ApsIs64Bits;
}

BOOLEAN
ApsIsLegitimateProcess(
	__in ULONG ProcessId
	)
{
	BOOLEAN Status;
	HANDLE ProcessHandle;

	//
	// If can't open process, we can do nothing.
	//

	ProcessHandle = OpenProcess(APS_OPEN_PROCESS_FLAG, FALSE, ProcessId);
	if (!ProcessHandle) {
		return FALSE;
	}

	if (ApsIs64Bits) {

		//
		// Native 64bits APS only handle 64 bits process.
		//

		Status = ApsIsWow64Process(ProcessHandle);
		CloseHandle(ProcessHandle);
		return !Status;
	}
	else {
		
		//
		// APS runs on WOW64
		//

		if (ApsIsWow64) {
			Status = ApsIsWow64Process(ProcessHandle);
			return Status;
		}

		//
		// APS runs on native 32 bits system
		//

		return TRUE;
	}
}

BOOLEAN
ApsIsVistaAbove(
	VOID
	)
{
	if (ApsMajorVersion >= 6) {
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
ApsIsWindowsXPAbove(
	VOID
	)
{
	if (ApsMajorVersion > 5) {
		return TRUE;
	}

	if (ApsMajorVersion == 5 &&ApsMinorVersion >= 1) {
		return TRUE;
	}

	return FALSE;
}

PAPS_PROCESS
ApsQueryProcessById(
	__in PLIST_ENTRY ListHead,
	__in ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PAPS_PROCESS Process;
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Process = CONTAINING_RECORD(ListEntry, APS_PROCESS, ListEntry);
		if (Process->ProcessId == ProcessId) {	
			return Process;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

ULONG
ApsQueryProcessByName(
	__in PWSTR Name,
	__out PLIST_ENTRY ListHead
	)
{
	LIST_ENTRY ListHead2;
	PLIST_ENTRY ListEntry;
	PAPS_PROCESS Process;
	ULONG Number;

	Number = 0;
	InitializeListHead(ListHead);
	InitializeListHead(&ListHead2);

	ApsQueryProcessList(&ListHead2);

	while (IsListEmpty(&ListHead2) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead2);
		Process = CONTAINING_RECORD(ListEntry, APS_PROCESS, ListEntry);

		if (!_wcsicmp(Name, Process->Name)) {	
			InsertTailList(ListHead, ListEntry);
			Number += 1;
		} else {
			ApsFree(Process);
		}
	}

	return Number;
}

VOID
ApsFreeProcess(
	__in PAPS_PROCESS Process
	)
{
	if (Process->Name) {
		ApsFree(Process->Name);
	}

	if (Process->FullPath) {
		ApsFree(Process->FullPath);
	}

	if (Process->CommandLine) {
		ApsFree(Process->CommandLine);
	}

	if (Process->Thread) {
		ApsFree(Process->Thread);
	}

	ApsFree(Process);
}

VOID
ApsFreeProcessList(
	__in PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY ListEntry;
	PAPS_PROCESS Process;

	while (IsListEmpty(ListHead) != TRUE) {
		ListEntry = RemoveHeadList(ListHead);
		Process = CONTAINING_RECORD(ListEntry, APS_PROCESS, ListEntry);
		ApsFreeProcess(Process);
	}
}

ULONG
ApsAllocateQueryBuffer(
	__in SYSTEM_INFORMATION_CLASS InformationClass,
	__out PVOID *Buffer,
	__out PULONG BufferLength,
	__out PULONG ActualLength
	)
{
	PVOID BaseVa;
	ULONG Length;
	ULONG ReturnedLength;
	NTSTATUS Status;

	BaseVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	do {
		if (BaseVa != NULL) {
			VirtualFree(BaseVa, 0, MEM_RELEASE);	
		}

		//
		// Incremental unit is 256KB 
		//

		Length += 1024 * 256;

		BaseVa = VirtualAlloc(NULL, Length, MEM_COMMIT, PAGE_READWRITE);
		if (BaseVa == NULL) {
			Status = GetLastError();
			break;
		}

		Status = (*NtQuerySystemInformation)(InformationClass, 
			                                 BaseVa, 
										     Length, 
										     &ReturnedLength);
		
	} while (Status == STATUS_INFO_LENGTH_MISMATCH);

	if (Status != STATUS_SUCCESS) {
		return (ULONG)Status;
	} 
	
	*Buffer = BaseVa;
	*BufferLength = Length;
	*ActualLength = ReturnedLength;

	return APS_STATUS_OK;
}

VOID
ApsReleaseQueryBuffer(
	__in PVOID StartVa
	)
{
	VirtualFree(StartVa, 0, MEM_RELEASE);
}

ULONG
ApsAdjustPrivilege(
    __in PWSTR PrivilegeName, 
    __in BOOLEAN Enable
    ) 
{
    HANDLE Handle;
    TOKEN_PRIVILEGES Privileges;
    BOOLEAN Status;
	
	Status = OpenProcessToken(GetCurrentProcess(),  
		                      TOKEN_ADJUST_PRIVILEGES, 
							  &Handle);
    if (Status != TRUE){
        return GetLastError();
    }
    
    Privileges.PrivilegeCount = 1;

    Status = LookupPrivilegeValue(NULL, PrivilegeName, &Privileges.Privileges[0].Luid);

	if (Status != TRUE) {
		CloseHandle(Handle);
		return GetLastError();
	}

	//
	// N.B. SE_PRIVILEGE_REMOVED must be avoided since this will remove privilege from
	// primary token and never get recovered again for current process. For disable case
	// we should use 0.
	//

    Privileges.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);
    Status = AdjustTokenPrivileges(Handle, 
		                           FALSE, 
								   &Privileges, 
								   sizeof(Privileges), 
								   NULL, 
								   NULL);
	if (Status != TRUE) {
		CloseHandle(Handle);
		return GetLastError();
	}

    CloseHandle(Handle);

    return APS_STATUS_OK;
}

ULONG
ApsQueryProcessInformation(
	__in ULONG ProcessId,
	__out PSYSTEM_PROCESS_INFORMATION *Information
	)
{
	ULONG Status;
	PVOID StartVa;
	ULONG Length;
	ULONG ReturnedLength;
	ULONG Offset;

	HANDLE ProcessHandle;
	SYSTEM_PROCESS_INFORMATION *Entry;

	ASSERT(ProcessId != 0 && ProcessId != 4);

	//
	// Ensure it's querying system process
	//

	if (ProcessId == 0 || ProcessId == 4) {
		return ERROR_INVALID_PARAMETER;
	}

	StartVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	Status = ApsAllocateQueryBuffer(SystemProcessInformation, 
		                            &StartVa,
								    &Length,
									&ReturnedLength);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	Offset = 0;
	Status = 0;

	while (Offset < ReturnedLength) {
		
		Entry = (SYSTEM_PROCESS_INFORMATION *)((PCHAR)StartVa + Offset);
		Offset += Entry->NextEntryOffset;

		//
		// Skip all non queried process id
		//

		if (HandleToUlong(Entry->UniqueProcessId) != ProcessId) {
			continue;
		}

		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, 
			                        HandleToUlong(Entry->UniqueProcessId));
		if (!ProcessHandle) {
			Status = GetLastError();
			break;
		}

		CloseHandle(ProcessHandle);

		//
		// Allocate and copy SYSTEM_PROCESS_INFORMATION block
		//

		Length = FIELD_OFFSET(SYSTEM_PROCESS_INFORMATION, Threads[Entry->NumberOfThreads]);
		*Information  = (PSYSTEM_PROCESS_INFORMATION)ApsMalloc(Length);
		RtlCopyMemory(*Information, Entry, Length);
		break;
	}
	
	ApsReleaseQueryBuffer(StartVa);
	return Status;
}

ULONG
ApsQueryProcessList(
	__out PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	NTSTATUS NtStatus;
	PVOID StartVa;
	ULONG Length;
	ULONG ReturnedLength;
	ULONG Offset;

	PROCESS_BASIC_INFORMATION  ProcessInformation;
	HANDLE ProcessHandle;
	PAPS_PROCESS Process;
	SYSTEM_PROCESS_INFORMATION *Entry;
	ULONG_PTR Address;
	WCHAR Buffer[1024];
	SIZE_T Complete;

	ASSERT(ListHead != NULL);

	InitializeListHead(ListHead);

	StartVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	Status = ApsAllocateQueryBuffer(SystemProcessInformation, 
		                            &StartVa,
								    &Length,
									&ReturnedLength);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	Offset = 0;

	while (Offset < ReturnedLength) {
		
		Entry = (SYSTEM_PROCESS_INFORMATION *)((PCHAR)StartVa + Offset);
		Offset += Entry->NextEntryOffset;

		if (Entry->UniqueProcessId == (HANDLE)0 || Entry->UniqueProcessId == (HANDLE)4) {
			continue;
		}

		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, HandleToUlong(Entry->UniqueProcessId));
		if (ProcessHandle == NULL) {

			//
			// Some process can not be debugged, e.g. audiodiag.exe on Vista/2008
			//

			if (Entry->NextEntryOffset == 0) {
				Status = APS_STATUS_OK;
				break;
			} else {
				continue;
			}			
		}
	
		Process = (PAPS_PROCESS)ApsMalloc(sizeof(APS_PROCESS));
		if (Process == NULL) {
			Status = GetLastError();
			break;
		}

		ZeroMemory(Process, sizeof(APS_PROCESS));
		Process->Name = ApsMalloc(Entry->ImageName.Length + sizeof(WCHAR));
		wcscpy(Process->Name, Entry->ImageName.Buffer);

		Process->ProcessId = HandleToUlong(Entry->UniqueProcessId);
		Process->SessionId = Entry->SessionId;
		Process->ParentId  = HandleToUlong(Entry->InheritedFromUniqueProcessId);
		Process->KernelTime = Entry->KernelTime;
		Process->UserTime = Entry->UserTime;
		Process->VirtualBytes = Entry->VirtualSize;
		Process->PrivateBytes = Entry->PrivatePageCount;
		Process->WorkingSetBytes = Entry->WorkingSetSize;
		Process->ThreadCount = Entry->NumberOfThreads;
		Process->KernelHandleCount = Entry->HandleCount;
		Process->ReadTransferCount = Entry->ReadTransferCount;
		Process->WriteTransferCount = Entry->WriteTransferCount;
		Process->OtherTransferCount = Entry->OtherTransferCount;

		Process->GdiHandleCount = GetGuiResources(ProcessHandle, GR_GDIOBJECTS);
		Process->UserHandleCount = GetGuiResources(ProcessHandle, GR_USEROBJECTS);

		Process->FullPath = ApsMalloc(MAX_PATH * 2 + sizeof(WCHAR));
		GetModuleFileNameEx(ProcessHandle, NULL, Process->FullPath, MAX_PATH * 2 - 1);

		//
		// Special case for smss.exe
		//

		/*if (_wcsicmp(L"smss.exe", Process->Name) == 0) {
			GetSystemDirectory(&SystemRoot[0], MAX_PATH - 1);
			wcsncat(SystemRoot, L"\\", MAX_PATH - 1);
			wcsncat(SystemRoot, Process->Name, MAX_PATH - 1);
			wcsncpy(Process->FullPath, SystemRoot, MAX_PATH - 1);
		}*/

		Length = 0;
		NtStatus = (*NtQueryInformationProcess)(ProcessHandle,
											    ProcessBasicInformation,
											    &ProcessInformation,
											    sizeof(PROCESS_BASIC_INFORMATION),
											    &Length);		
		//
		// N.B. PEB is the primary key data structure we're interested.
		// We need command line since many processes can only be distingushed
		// from command line parameter, e.g. svchost, dllhost etc.
		//

		if (NtStatus == STATUS_SUCCESS) {

			RtlZeroMemory(Buffer, sizeof(Buffer));

			Process->PebAddress = ProcessInformation.PebBaseAddress;
			Address = (ULONG_PTR)Process->PebAddress + FIELD_OFFSET(PEB, ProcessParameters);
			ReadProcessMemory(ProcessHandle, (PVOID)Address, Buffer, sizeof(PVOID), &Complete); 

			//
			// Get RTL_USER_PROCESS_PARAMETER address
			//

			Address = *(PULONG_PTR)Buffer;
			Address = Address + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine);
			ReadProcessMemory(ProcessHandle, (PVOID)Address, Buffer, sizeof(UNICODE_STRING), &Complete);

			//
			// Get CommandLine UNICODE_STRING data
			//

			Address = (ULONG_PTR)((PUNICODE_STRING)Buffer)->Buffer;
			Length = ((PUNICODE_STRING)Buffer)->Length;
			
			Buffer[1023] = 0;
			Length = min(1023 * sizeof(WCHAR), Length);

			ReadProcessMemory(ProcessHandle, (PVOID)Address, Buffer, Length, &Complete);

			if (Length > 0 && Complete > 0) {

				Process->CommandLine = ApsMalloc(Length + sizeof(WCHAR));
				wcscpy(Process->CommandLine, Buffer);

			} else {
				Process->CommandLine = NULL;
			}
		}
		
		CloseHandle(ProcessHandle);
		ProcessHandle = NULL;

		InsertTailList(ListHead, &Process->ListEntry);

		if (Entry->NextEntryOffset == 0) {
			Status = APS_STATUS_OK;
			break;
		}
	}
	
	ApsReleaseQueryBuffer(StartVa);
	return Status;
}

ULONG
ApsQueryThreadList(
	__in ULONG ProcessId,
	__out PAPS_PROCESS *Object
	)
{
	ULONG Status;
	PVOID StartVa;
	ULONG Length;
	ULONG ReturnedLength;
	ULONG Offset;
	HANDLE ProcessHandle;
	PAPS_PROCESS Process;
	SYSTEM_PROCESS_INFORMATION *Entry;

	StartVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	Status = ApsAllocateQueryBuffer(SystemProcessInformation, 
		                            &StartVa,
								    &Length,
									&ReturnedLength);

	if (Status != APS_STATUS_OK) {
		return Status;
	}

	Offset = 0;

	while (Offset < ReturnedLength) {
		
		Entry = (SYSTEM_PROCESS_INFORMATION *)((PCHAR)StartVa + Offset);
		Offset += Entry->NextEntryOffset;

		if (HandleToUlong(Entry->UniqueProcessId) != ProcessId) {
			continue;
		}

		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
		if (!ProcessHandle) {
			return GetLastError();
		}
	
		Process = (PAPS_PROCESS)ApsMalloc(sizeof(APS_PROCESS));
		ZeroMemory(Process, sizeof(APS_PROCESS));

		Process->Name = ApsMalloc(Entry->ImageName.Length + sizeof(WCHAR));
		wcscpy(Process->Name, Entry->ImageName.Buffer);

		Process->ProcessId = ProcessId;
		Process->SessionId = Entry->SessionId;
		Process->ParentId  = HandleToUlong(Entry->InheritedFromUniqueProcessId);
		Process->KernelTime = Entry->KernelTime;
		Process->UserTime = Entry->UserTime;
		Process->VirtualBytes = Entry->VirtualSize;
		Process->PrivateBytes = Entry->PrivatePageCount;
		Process->WorkingSetBytes = Entry->WorkingSetSize;
		Process->ThreadCount = Entry->NumberOfThreads;
		Process->KernelHandleCount = Entry->HandleCount;
		Process->ReadTransferCount = Entry->ReadTransferCount;
		Process->WriteTransferCount = Entry->WriteTransferCount;
		Process->OtherTransferCount = Entry->OtherTransferCount;

		Process->GdiHandleCount = GetGuiResources(ProcessHandle, GR_GDIOBJECTS);
		Process->UserHandleCount = GetGuiResources(ProcessHandle, GR_USEROBJECTS);

		Process->FullPath = ApsMalloc(MAX_PATH + sizeof(WCHAR));
		GetModuleFileNameEx(ProcessHandle, NULL, Process->FullPath, MAX_PATH - 1);

		//
		// Allocate and copy thread list
		//

		Length = sizeof(SYSTEM_THREAD_INFORMATION) * Process->ThreadCount;
		Process->Thread = (PSYSTEM_THREAD_INFORMATION)ApsMalloc(Length);
		RtlCopyMemory(Process->Thread, Entry->Threads, Length);

		CloseHandle(ProcessHandle);
		break;
	}
	
	ApsReleaseQueryBuffer(StartVa);
	*Object = Process;
	return Status;
}

ULONG
ApsSuspendProcess(
	__in HANDLE ProcessHandle
	)
{
	ULONG Status;

	Status = NtSuspendProcess(ProcessHandle);
	if (NT_SUCCESS(Status)) {
		return APS_STATUS_OK;
	}

	//
	// N.B. This compensation call require further
	// investigation.
	//

	NtResumeProcess(ProcessHandle);
	return APS_STATUS_ERROR;
}

ULONG
ApsResumeProcess(
	__in HANDLE ProcessHandle
	)
{
	//
	// Don't care its return status, always return
	// APS_STATUS_OK
	//

	NtResumeProcess(ProcessHandle);
	return APS_STATUS_OK;
}

//
// N.B. ApsCreateRemoteThread create remote thread initially suspended
//

ULONG
ApsCreateRemoteThread(
	__in HANDLE ProcessHandle,
	__in PVOID StartAddress,
	__in PVOID Context,
	__out PHANDLE ThreadHandle
	)
{
	ULONG Status;
	
	//
	// N.B. For NT 6+, RtlCreateUserThread is used to make it cross session,
	// CreateRemoteThread works for NT 5.1, 5.2 only.
	//

	if (ApsMajorVersion >= 6) {
		Status = (ULONG)(*RtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
									           (LPTHREAD_START_ROUTINE)StartAddress, 
											    Context, ThreadHandle, NULL);
	} else {
		*ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)StartAddress, 
										   Context, CREATE_SUSPENDED, NULL);
		if (*ThreadHandle != NULL) {
			Status = APS_STATUS_OK;
		} else {
			Status = GetLastError();
		}
	}

	return Status;
}

ULONG
ApsQueryModule(
	__in ULONG ProcessId,
	__in BOOLEAN IncludeVersion, 
	__out PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	HANDLE Handle;
	MODULEENTRY32 Entry;
    PAPS_MODULE Module;

	InitializeListHead(ListHead);

	if (ProcessId == 0 || ProcessId == 4) {
		return APS_STATUS_OK;
	}

    Handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);
    if (Handle == INVALID_HANDLE_VALUE){
        Status = GetLastError();
		return Status;
    }	
	 
	Entry.dwSize = sizeof(Entry);

	if (!Module32First(Handle, &Entry)){
        CloseHandle(Handle);
        Status = GetLastError();
		return Status;
    }
    
	do {
		
		Module = (PAPS_MODULE)ApsMalloc(sizeof(APS_MODULE));

		Module->BaseVa = Entry.modBaseAddr;
		Module->Size = Entry.modBaseSize;

		Module->Name = ApsMalloc((ULONG)(wcslen(Entry.szModule) + 1) * sizeof(WCHAR));
		wcscpy(Module->Name, Entry.szModule);

		Module->FullPath = ApsMalloc((ULONG)(wcslen(Entry.szExePath) + 1) * sizeof(WCHAR));
		wcscpy(Module->FullPath, Entry.szExePath);
		
		if (IncludeVersion == TRUE) {
			ApsGetModuleVersion(Module);
		}

		InsertTailList(ListHead, &Module->ListEntry);

    } while (Module32Next(Handle, &Entry));

	CloseHandle(Handle);
	return APS_STATUS_OK;
}

ULONG
ApsGetModuleVersion(
	__in PAPS_MODULE Module
	)
{
	BOOLEAN Status;
	ULONG Length;
	ULONG Handle;
	PVOID FileVersion;
	PVOID Value;
	ULONG64 TimeStamp;
	WCHAR SubBlock[64];

	struct {
		USHORT Language;
		USHORT CodePage;
	} *Translate;	

	Length = GetFileVersionInfoSize(Module->FullPath, &Handle);
	if (Length == 0) {
		return GetLastError();
	}

	FileVersion = ApsMalloc(Length);

	GetFileVersionInfo(Module->FullPath, Handle, Length, FileVersion); 
	Status = VerQueryValue(FileVersion, L"\\VarFileInfo\\Translation", &Translate, &Length);
	if (Status != TRUE) {
		return GetLastError();
	}

	wsprintf(SubBlock,
             L"\\StringFileInfo\\%04x%04x\\ProductVersion",
             Translate[0].Language, 
			 Translate[0].CodePage);

	Value = NULL;
	Status = VerQueryValue(FileVersion, SubBlock, &Value, &Length);						
	if (Status != TRUE) {
		return GetLastError();
	}
	if (Value != NULL) {
		Module->Version = ApsMalloc((Length + 1) *sizeof(WCHAR));
		wcscpy(Module->Version, Value);
	}

	wsprintf(SubBlock,
		     L"\\StringFileInfo\\%04x%04x\\CompanyName",
			 Translate[0].Language, 
			 Translate[0].CodePage);
	
	Value = NULL;
	Status = VerQueryValue(FileVersion, SubBlock, &Value, &Length);						
	if (Status != TRUE) {
		return GetLastError();
	}
	if (Value != NULL) {
		Module->Company = ApsMalloc((Length + 1) * sizeof(WCHAR));
		wcscpy(Module->Company, Value);
	}

	wsprintf(SubBlock,
		     L"\\StringFileInfo\\%04x%04x\\FileDescription",
			 Translate[0].Language, 
			 Translate[0].CodePage);
	
	Value = NULL;
	Status = VerQueryValue(FileVersion, SubBlock, &Value, &Length);						
	if (Status != TRUE) {
		return GetLastError();
	}
	if (Value != NULL) {
		Module->Description = ApsMalloc((Length + 1) * sizeof(WCHAR));
		wcscpy(Module->Description, Value);
	}
	
	ApsGetModuleTimeStamp(Module, &TimeStamp);	
	ApsFree(FileVersion);

	return APS_STATUS_OK;
}

ULONG
ApsGetModuleTimeStamp(
	__in PAPS_MODULE Module,
	__out PULONG64 TimeStamp
	)
{
    HANDLE FileHandle;
	HANDLE FileMappingHandle;
	PVOID MappedVa;
	ULONG64 LinkerTime;
	WCHAR Value[MAX_PATH];
	ULONG Length;
	struct tm *time;

	ASSERT(Module != NULL);

	FileHandle = CreateFile(Module->FullPath,
						    GENERIC_READ,
							FILE_SHARE_READ,
						    NULL, 
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	
	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

    FileMappingHandle = CreateFileMapping(FileHandle, 
		                                  NULL,
										  PAGE_READONLY,
										  0, 
										  0, 
										  NULL);
    
	if (!FileMappingHandle) {
        CloseHandle(FileHandle);
		return GetLastError();
	}
	
	MappedVa = MapViewOfFile(FileMappingHandle, FILE_MAP_READ, 0, 0, 0);
	if (!MappedVa){
        CloseHandle(FileMappingHandle);
        CloseHandle(FileHandle);
		return GetLastError();
	}

	//
	// N.B. Linker time is required, which is real time when the file was generated.
	//

	LinkerTime = GetTimestampForLoadedLibrary(MappedVa);
	*TimeStamp = LinkerTime;
	
	time = localtime(&LinkerTime);
	time->tm_year += 1900;
	time->tm_mon += 1;

	Length = wsprintf(Value, L"%04d/%02d/%02d %02d:%02d", 
					  time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour, time->tm_min);

	Module->TimeStamp = ApsMalloc((Length + 1) * sizeof(WCHAR));
	wcscpy(Module->TimeStamp, Value);

	UnmapViewOfFile(MappedVa);
	CloseHandle(FileHandle);
	CloseHandle(FileMappingHandle);
	
	return APS_STATUS_OK;
}

VOID
ApsFreeModuleList(
	__in PLIST_ENTRY ListHead
	)
{
    PLIST_ENTRY ListEntry;
	PAPS_MODULE Module;
	
	__try {
		while (IsListEmpty(ListHead) != TRUE) {
			ListEntry = RemoveHeadList(ListHead);
			Module = CONTAINING_RECORD(ListEntry, APS_MODULE, ListEntry);
			ApsFree(Module);
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		
	}
}

VOID
ApsFreeModule(
	__in PAPS_MODULE Module 
	)
{
	__try {
		if (Module->Name)
			ApsFree(Module->Name);

		if (Module->FullPath)
			ApsFree(Module->FullPath);

		if (Module->Version) 
			ApsFree(Module->Version);

		if (Module->TimeStamp)
			ApsFree(Module->TimeStamp);

		if (Module->Company)
			ApsFree(Module->Company);

		if (Module->Description)
			ApsFree(Module->Description);

		ApsFree(Module);

	} __except (EXCEPTION_EXECUTE_HANDLER) {
		
	}
}

ULONG
ApsEnumerateDllExport(
	__in PWSTR ModuleName,
	__in PAPS_MODULE Module, 
	__out PULONG Count,
	__out PAPS_EXPORT_DESCRIPTOR *Descriptor
	)
{
	PULONG Names;
	PUCHAR Base; 
	HANDLE FileHandle;
	HANDLE ViewHandle;
	ULONG Length, i;
	PULONG Routines;
	PUSHORT Ordinal;
	PIMAGE_DATA_DIRECTORY Directory;
	PIMAGE_SECTION_HEADER SectionHeader;
	PIMAGE_EXPORT_DIRECTORY Export;
	PIMAGE_NT_HEADERS NtHeader;
	PAPS_EXPORT_DESCRIPTOR Buffer;
	ULONG NumberOfSections;
	ULONG NamesOffset, FunctionsOffset, OrdinalsOffset;
	PCHAR Pointer;

	*Count = 0;
	*Descriptor = NULL;

	Base = ApsMapImageFile(ModuleName, &FileHandle, &ViewHandle);
	if (!Base){
		return ERROR_INVALID_HANDLE;
	}

	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);

#if defined (_M_IX86)
	
	if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
		ApsUnmapImageFile(Base, FileHandle, ViewHandle);
		return ERROR_INVALID_HANDLE;
	}

#elif defined (_M_X64)
	
	if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
		ApsUnmapImageFile(Base, FileHandle, ViewHandle);
		return ERROR_INVALID_HANDLE;
	}

#endif

	Directory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!Directory->VirtualAddress) {

		//
		// There's no export information
		//

		ApsUnmapImageFile(Base, FileHandle, ViewHandle);
		return ERROR_NOT_FOUND;
	}

	ApsDebugTrace("Walking export of %S ...", ModuleName);

	SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
	NumberOfSections = NtHeader->FileHeader.NumberOfSections;

	Export = (PIMAGE_EXPORT_DIRECTORY)(Base + ApsRvaToOffset(Directory->VirtualAddress, SectionHeader, NumberOfSections));
	NamesOffset = ApsRvaToOffset(Export->AddressOfNames, SectionHeader, NumberOfSections);
	FunctionsOffset = ApsRvaToOffset(Export->AddressOfFunctions, SectionHeader, NumberOfSections);
	OrdinalsOffset = ApsRvaToOffset(Export->AddressOfNameOrdinals, SectionHeader, NumberOfSections);

	//
	// Compute virtual address of name array
	//

	Names    = (PULONG)((ULONG_PTR)Base + (ULONG_PTR)NamesOffset); 
	Routines = (PULONG)((ULONG_PTR)Base + (ULONG_PTR)FunctionsOffset);
	Ordinal  = (PUSHORT)((ULONG_PTR)Base + (ULONG_PTR)OrdinalsOffset);

	//
	// Allocate export descriptor buffer and copy export names to
	// buffer.
	//

	Buffer = (APS_EXPORT_DESCRIPTOR *)ApsMalloc(Export->NumberOfNames * sizeof(APS_EXPORT_DESCRIPTOR));
	
	for(i = 0; i < Export->NumberOfNames; i++) {

		//
		// N.B. PE specification has a bug that claim Ordinal should subtract
		// OrdinalBase to get real index into function address table, experiment
		// indicates the Ordinal[i] is real index, don't subtract OrdinalBase.
		//

		Buffer[i].Rva = Routines[Ordinal[i]];
		if (Buffer[i].Rva >= Directory->VirtualAddress && 
			Buffer[i].Rva < Directory->VirtualAddress + Directory->Size) {
				
				//
				// This is a forwarder, Rva points to a Ascii string of DLL forwarder
				//

				Pointer = (PUCHAR)((ULONG_PTR)Base + ApsRvaToOffset(Names[i], SectionHeader, NumberOfSections));
				Length = (ULONG)strlen(Pointer);
				Buffer[i].Name = (char *)ApsMalloc(Length + 1);
				strcpy(Buffer[i].Name, Pointer);

				Pointer = (PUCHAR)(ULONG_PTR)Base + ApsRvaToOffset(Buffer[i].Rva, SectionHeader, NumberOfSections);
				Length = (ULONG)strlen(Pointer);
				Buffer[i].Forward = (char *)ApsMalloc(Length + 1);
				strcpy(Buffer[i].Forward, Pointer);

				Buffer[i].Ordinal = Ordinal[i];
				Buffer[i].Rva = 0;
				Buffer[i].Va = 0;
				continue;
		}

		Buffer[i].Ordinal = Ordinal[i];

		if (Module != NULL) {
			Buffer[i].Va = (ULONG_PTR)((ULONG_PTR)Module->BaseVa + (ULONG)Buffer[i].Rva);
		} else {
			Buffer[i].Va = (ULONG_PTR)((ULONG_PTR)NtHeader->OptionalHeader.ImageBase + (ULONG)Buffer[i].Rva);
		}

		Pointer = (PUCHAR)((ULONG_PTR)Base + ApsRvaToOffset(Names[i], SectionHeader, NumberOfSections));
		Length = (ULONG)strlen(Pointer);
		Buffer[i].Name = (char *)ApsMalloc(Length + 1);
		strcpy(Buffer[i].Name, Pointer);

		Buffer[i].Forward = NULL;
	}

	*Count = Export->NumberOfNames;
	*Descriptor = Buffer;

	ApsUnmapImageFile(Base, FileHandle, ViewHandle);
	return APS_STATUS_OK;
}

PVOID
ApsGetDllForwardAddress(
	__in ULONG ProcessId,
	__in PSTR DllForward
	)
{
	HANDLE Handle;
	PVOID Address;
	CHAR DllName[MAX_PATH];
	CHAR ApiName[MAX_PATH];
	PWCHAR Unicode;

	ApsGetForwardDllName(DllForward, DllName, MAX_PATH);
	ApsGetForwardApiName(DllForward, ApiName, MAX_PATH);

	if (ProcessId == GetCurrentProcessId()) {

		Handle = GetModuleHandleA(DllName);
		if (!Handle) {
			return NULL;
		}

		Address = GetProcAddress(Handle, ApiName);
		return Address;
	}

	ApsConvertAnsiToUnicode(DllName, &Unicode);
	Address = ApsGetRemoteApiAddress(ProcessId, Unicode, ApiName);

	ApsFree(Unicode);
	return Address;
}

VOID
ApsGetForwardDllName(
	__in PSTR Forward,
	__out PCHAR DllName,
	__in ULONG Length
	)
{
	PCHAR Dot;

	Dot = strchr(Forward, '.');
	if (!Dot) {
		DllName[0] = '\0';
		return;
	}

	*Dot = '\0';
	StringCchCopyA(DllName, Length, Forward);
	*Dot = '.';
}

VOID
ApsGetForwardApiName(
	__in PSTR Forward,
	__out PCHAR ApiName,
	__in ULONG Length
	)
{
	PCHAR Dot;

	Dot = strchr(Forward, '.');
	if (!Dot) {
		ApiName[0] = '\0';
		return;
	}

	StringCchCopyA(ApiName, Length, Dot + 1);
}

PAPS_MODULE
ApsGetModuleByName(
	__in PLIST_ENTRY ListHead,
	__in PWSTR ModuleName
	)
{
	PLIST_ENTRY ListEntry;
	PAPS_MODULE Module;
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Module = CONTAINING_RECORD(ListEntry, APS_MODULE, ListEntry);
		if (!wcsicmp(Module->Name, ModuleName)){
			return Module;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PVOID
ApsMapImageFile(
	IN PWSTR ModuleName,
	OUT PHANDLE FileHandle,
	OUT PHANDLE ViewHandle
	)
{
	PIMAGE_DOS_HEADER MappedBase;
	PIMAGE_NT_HEADERS NtHeadersVa;

	if (ModuleName == NULL || FileHandle == NULL || ViewHandle == NULL) {
		return NULL;
	}

	*FileHandle = CreateFile(ModuleName, GENERIC_READ, 
							 FILE_SHARE_READ, NULL, 
							 OPEN_EXISTING, 0, 0);

	if (*FileHandle == INVALID_HANDLE_VALUE) {
		*ViewHandle = NULL;
		return NULL;
	}

	*ViewHandle = CreateFileMapping(*FileHandle, NULL, PAGE_READONLY,
									0, 0, NULL);

	if (*ViewHandle == NULL) {
		CloseHandle(*FileHandle);
		*FileHandle = INVALID_HANDLE_VALUE;
		return NULL;
	}

	MappedBase = (PIMAGE_DOS_HEADER) MapViewOfFile(*ViewHandle, FILE_MAP_READ, 0, 0, 0);
	if (MappedBase == NULL) {
		CloseHandle(*ViewHandle);
		CloseHandle(*FileHandle);
		return NULL;
	}

	//
	// Simple validation of PE signature
	//

	if (MappedBase->e_magic != IMAGE_DOS_SIGNATURE) {
		UnmapViewOfFile(MappedBase);
		CloseHandle(*ViewHandle);
		CloseHandle(*FileHandle);
		return NULL;
	}

	NtHeadersVa = (PIMAGE_NT_HEADERS)((ULONG_PTR)MappedBase + (ULONG_PTR)MappedBase->e_lfanew);
	if (NtHeadersVa->Signature != IMAGE_NT_SIGNATURE) {
		UnmapViewOfFile(MappedBase);
		CloseHandle(*ViewHandle);
		CloseHandle(*FileHandle);
		return NULL;
	}

	return MappedBase;
}

VOID
ApsUnmapImageFile(
	IN PVOID  MappedBase,
	IN HANDLE FileHandle,
	IN HANDLE ViewHandle
	)
{
	UnmapViewOfFile(MappedBase);
	CloseHandle(ViewHandle);
	CloseHandle(FileHandle);
}

ULONG
ApsRvaToOffset(
	IN ULONG Rva,
	IN PIMAGE_SECTION_HEADER Headers,
	IN ULONG NumberOfSections
	)
{
	ULONG i;

	for(i = 0; i < NumberOfSections; i++) {
		if(Rva >= Headers->VirtualAddress) {
			if(Rva < Headers->VirtualAddress + Headers->Misc.VirtualSize) {
				return (ULONG)(Rva - Headers->VirtualAddress + Headers->PointerToRawData);
			}
		}
		Headers += 1;
	}

	return (ULONG)-1;
}

ULONG_PTR
ApsGetModuleBase(
	__in PWSTR ModuleName
	)
{
	return 0;
}

ULONG_PTR
ApsGetRoutineAddress(
	__in PWSTR ModuleName,
	__in PWSTR RoutineName
	)
{
	return 0;
}

PAPS_MODULE
ApsGetModuleByAddress(
	__in PLIST_ENTRY ListHead,
	__in PVOID StartVa
	)
{
	PLIST_ENTRY ListEntry;
	PAPS_MODULE Module;
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Module = CONTAINING_RECORD(ListEntry, APS_MODULE, ListEntry);
		if ((ULONG_PTR)Module->BaseVa <= (ULONG_PTR)StartVa && 
			(ULONG_PTR)Module->BaseVa + (ULONG_PTR)Module->Size > (ULONG_PTR)StartVa) {	
			return Module;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

ULONG
ApsEnumerateDllExportByName(
	__in ULONG ProcessId,
	__in PWSTR ModuleName,
	__in PWSTR ApiName,
	__out PULONG Count,
	__out PLIST_ENTRY ApiList
	)
{
	ULONG Status;
	PAPS_MODULE Module;
	ULONG ApiCount = 0;
	PAPS_EXPORT_DESCRIPTOR Api;
	PAPS_EXPORT_DESCRIPTOR Descriptor;
	ULONG i;
	ULONG Number = 0;
	LIST_ENTRY ModuleListHead;
	CHAR AnsiPattern[MAX_PATH];
	CHAR AnsiApiName[MAX_PATH];
	PCHAR Position;

	Status = ApsQueryModule(ProcessId, FALSE, &ModuleListHead);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	Module = ApsGetModuleByName(&ModuleListHead, ModuleName);
	if (Module == NULL) {
		return ERROR_NOT_FOUND;
	}

	ApsEnumerateDllExport(Module->FullPath,
	                Module,
					&ApiCount,
					&Api);

	WideCharToMultiByte(CP_ACP, 0, ApiName, -1, AnsiPattern, 256, NULL, NULL);
	strlwr(AnsiPattern);

	for(i = 0; i < ApiCount; i++) {

		strncpy(AnsiApiName, Api[i].Name, MAX_PATH - 1);
		strlwr(AnsiApiName);
		
		Position = strstr(AnsiApiName, AnsiPattern);
		
		if (Position != NULL) {

			Descriptor = (APS_EXPORT_DESCRIPTOR *)ApsMalloc(sizeof(APS_EXPORT_DESCRIPTOR));
			
			Descriptor->Rva = Api[i].Rva;
			Descriptor->Va = Api[i].Va;
			Descriptor->Ordinal = Api[i].Ordinal;
			Descriptor->Name = (PCHAR)ApsMalloc((ULONG)strlen(Api[i].Name) + 1);
			strcpy(Descriptor->Name, Api[i].Name);

			if (Api[i].Forward != NULL){
				Descriptor->Forward = (PCHAR)ApsMalloc((ULONG)strlen(Api[i].Forward) + 1);
				strcpy(Descriptor->Forward, Api[i].Forward);
			}

			Number += 1;
			InsertTailList(ApiList, &Descriptor->Entry);
		}
	}
	
	*Count = Number;

	if (ApiCount > 0) {
		ApsFreeExportDescriptor(Api, ApiCount);	
	}

	if (IsListEmpty(ApiList)) {
		return ERROR_NOT_FOUND;
	}

	return APS_STATUS_OK;
}

VOID
ApsFreeExportDescriptor(
	__in PAPS_EXPORT_DESCRIPTOR Descriptor,
	__in ULONG Count
	)
{
	ULONG i;
	
	ASSERT(Descriptor != NULL);

	for(i = 0; i < Count; i++) {
		ApsFree(Descriptor[i].Name);
		if(Descriptor[i].Forward != NULL) {
			ApsFree(Descriptor[i].Forward);
		}
	}

	ApsFree(Descriptor);
}

ULONG
ApsCreateProcess(
	__in PWSTR ImagePath,
	__in PWSTR Parameter,
	__in PWSTR WorkPath,
	__in BOOLEAN Suspend,
	__out PHANDLE ProcessHandle,
	__out PHANDLE ThreadHandle
	)
{
	ASSERT(0);
	return APS_STATUS_OK;
}

ULONG
ApsAttachProcess(
	__in ULONG ProcessId
	)
{
	ULONG ErrorCode;
	BOOLEAN Status;

	Status = DebugActiveProcess(ProcessId);
	if (Status != TRUE) {
		ErrorCode = GetLastError();
		return ErrorCode;
	}

	return APS_STATUS_OK;
}

PVOID
ApsQueryTebAddress(
	__in HANDLE ThreadHandle
	)
{
	NTSTATUS Status;	
	THREAD_BASIC_INFORMATION BasicInformation = {0};

	Status = (*NtQueryInformationThread)(ThreadHandle,
										 ThreadBasicInformation,
									     &BasicInformation,
									     sizeof(THREAD_BASIC_INFORMATION),
									     NULL);
	if (NT_SUCCESS(Status)) {
		return BasicInformation.TebBaseAddress;	
	}

	return NULL;
}

PVOID
ApsQueryPebAddress(
	__in HANDLE ProcessHandle 
	)
{
	ULONG Length = 0;
	NTSTATUS NtStatus;
	PROCESS_BASIC_INFORMATION Information = {0};

	NtStatus = (*NtQueryInformationProcess)(ProcessHandle,
										    ProcessBasicInformation,
										    &Information,
										    sizeof(PROCESS_BASIC_INFORMATION),
										    &Length);		
	if (NtStatus == STATUS_SUCCESS) {
		return Information.PebBaseAddress;
	}

	return NULL;
}

VOID
ApsGetWorkDirectory(
	__in PWSTR ImagePath,
	__out PWCHAR Buffer
	)
{
	_wsplitpath(ImagePath, NULL, Buffer, NULL, NULL);
}

ULONG
ApsCaptureMemoryDump(
	__in ULONG ProcessId,
	__in BOOLEAN Full,
	__in PWSTR Path
	)
{
	#define BSP_MINIDUMP_FULL  MiniDumpWithFullMemory        | \
	                           MiniDumpWithFullMemoryInfo    | \
							   MiniDumpWithHandleData        | \
							   MiniDumpWithThreadInfo        | \
							   MiniDumpWithUnloadedModules   |\
							   MiniDumpWithFullAuxiliaryState

	HANDLE ProcessHandle;
	HANDLE FileHandle;
	MINIDUMP_TYPE Type;
	ULONG Status;

	ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (!ProcessHandle) {
		Status = GetLastError();
		return Status;
	}

	FileHandle = CreateFile(Path,
	                        GENERIC_READ | GENERIC_WRITE,
	                        FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL,
	                        CREATE_ALWAYS,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		CloseHandle(ProcessHandle);
		return Status;
	}

	Type = Full ? (BSP_MINIDUMP_FULL) : MiniDumpNormal;
	Status = MiniDumpWriteDump(ProcessHandle, ProcessId, FileHandle, Type, NULL, NULL, NULL);

	if (!Status) {
		Status = GetLastError();
	} else {
		Status = APS_STATUS_OK;
	}

	CloseHandle(FileHandle);
	return Status;
}

ULONG
ApsGetFullPathName(
	__in PWSTR BaseName,
	__in PWCHAR Buffer,
	__in USHORT Length,
	__out PUSHORT ActualLength
	)
{
	HMODULE ModuleHandle;
	PWCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Buffer, Length);
	Slash = wcsrchr(Buffer, L'\\');
	wcscpy(Slash + 1, BaseName);
	*ActualLength = (USHORT)wcslen(Buffer);

	return APS_STATUS_OK;
}

ULONG
ApsGetProcessPath(
	__in PWCHAR Buffer,
	__in USHORT Length,
	__out PUSHORT ActualLength
	)
{
	HMODULE ModuleHandle;
	PWCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Buffer, Length);

	Slash = wcsrchr(Buffer, L'\\');
	Slash[0] = 0;

	*ActualLength = (USHORT)(((ULONG_PTR)Slash - (ULONG_PTR)Buffer) / sizeof(WCHAR));
	return APS_STATUS_OK;
}

ULONG
ApsGetProcessPathA(
	__in PCHAR Buffer,
	__in USHORT Length,
	__out PUSHORT ActualLength
)
{
	HMODULE ModuleHandle;
	PCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileNameA(ModuleHandle, Buffer, Length);

	Slash = strrchr(Buffer, '\\');
	Slash[0] = 0;

	*ActualLength = (USHORT)(((ULONG_PTR)Slash - (ULONG_PTR)Buffer) / sizeof(CHAR));
	return APS_STATUS_OK;
}

BOOLEAN
ApsGetModuleInformation(
	__in ULONG ProcessId,
	__in PWSTR DllName,
	__in BOOLEAN FullPath,
	__out HMODULE *DllHandle,
	__out PULONG_PTR Address,
	__out SIZE_T *Size
	)
{ 
	BOOLEAN Found = FALSE;
	MODULEENTRY32 Module; 
	HANDLE Handle = INVALID_HANDLE_VALUE; 

	Handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId); 
	if (Handle == INVALID_HANDLE_VALUE) { 
		return FALSE;
	} 

	Module.dwSize = sizeof(MODULEENTRY32); 

	if (!Module32First(Handle, &Module)) { 
		CloseHandle(Handle);
		return FALSE; 
	} 

	do { 

		if (FullPath) {
			Found = !_wcsicmp(DllName, Module.szExePath);
		} else {
			Found = !_wcsicmp(DllName, Module.szModule);
		}

		if (Found) {

			if (DllHandle) {
				*DllHandle = Module.hModule;
			}

			if (Address) {
				*Address = (ULONG_PTR)Module.modBaseAddr;	
			}

			if (Size) {
				*Size = Module.modBaseSize;
			}

			CloseHandle(Handle);
			return TRUE;
		}

	} while (Module32Next(Handle, &Module));

	CloseHandle(Handle); 
	return FALSE; 
} 

//
//PVOID
//ApsGetRemoteApiAddress(
//	__in ULONG ProcessId,
//	__in PWSTR DllName,
//	__in PSTR ApiName
//	)
//{ 
//	HMODULE DllHandle;
//	ULONG_PTR Address;
//	HMODULE LocalDllHandle;
//	ULONG_PTR LocalAddress;
//	SIZE_T Size;
//	ULONG Status;
//	WCHAR Buffer[MAX_PATH];
//
//	ASSERT(GetCurrentProcessId() != ProcessId);
//	
//	LocalDllHandle = LoadLibrary(DllName);
//	if (!LocalDllHandle) {
//		return NULL;
//	}
//	
//	LocalAddress = (ULONG_PTR)GetProcAddress(LocalDllHandle, ApiName);
//	if (!LocalAddress) {
//		FreeLibrary(LocalDllHandle);
//		return NULL;
//	}
//
//	StringCchCopy(Buffer, MAX_PATH, DllName);
//	wcslwr(Buffer);
//
//	if (!wcsstr(Buffer, L".dll")) {
//		StringCchPrintf(Buffer, MAX_PATH, L"%s.dll", DllName);
//	}
//
//	Status = ApsGetModuleInformation(ProcessId, Buffer, FALSE, &DllHandle, &Address, &Size);
//	if (!Status) {
//		FreeLibrary(LocalDllHandle);
//		return NULL;
//	}
//
//	FreeLibrary(LocalDllHandle);
//	return (PVOID)((ULONG_PTR)DllHandle + (LocalAddress - (ULONG_PTR)LocalDllHandle));	
//}

ULONG
ApsQuerySystemVersion(
	__in PWCHAR Buffer,
	__in ULONG Length
	)
{
	ULONG Status;
	DWORD Size;
	DWORD Type;
    HKEY Key;
	WCHAR Product[MAX_PATH];
	PWSTR Bits;
	OSVERSIONINFO Info;

	Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 
		                  0, KEY_READ, &Key);
	
	if (Status != APS_STATUS_OK) {
		Status = GetLastError();
		return Status;
	}

	Size = 512;
	Status = RegQueryValueEx(Key, L"ProductName", 0, &Type, (LPBYTE)Product, &Size);
	if (Status != APS_STATUS_OK) {
		RegCloseKey(Key);
		return Status;
	}

	RegCloseKey(Key);

	ZeroMemory(&Info, sizeof(Info));
	Info.dwOSVersionInfoSize = sizeof(Info);
	GetVersionEx(&Info);

	if (ApsIs64Bits || ApsIsWow64) {
		Bits = L"x64";
	} else {
		Bits = L"x86";
	}

	StringCchPrintf(Buffer, Length, L"%s %s %s", Product, Info.szCSDVersion, Bits);

	return Status;
}

ULONG
ApsQueryCpuModel(
	__in PWCHAR Buffer,
	__in ULONG Length
	)
{
	ULONG Status;
	DWORD Size;
	DWORD Type;
    HKEY Key;
	WCHAR Cpu[MAX_PATH];

	Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		                  L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
		                  0, KEY_READ, &Key);
	
	if (Status != APS_STATUS_OK) {
		Status = GetLastError();
		return Status;
	}

	Size = MAX_PATH;
	Status = RegQueryValueEx(Key, L"ProcessorNameString", 0, &Type, (LPBYTE)Cpu, &Size);
	if (Status != APS_STATUS_OK) {
		RegCloseKey(Key);
		return Status;
	}

	RegCloseKey(Key);

	StringCchCopy(Buffer, Length, Cpu);
	return Status;
}

ULONG
ApsGetComputerName(
	__in PWCHAR Buffer,
	__in ULONG Length
	)
{
	return GetComputerName(Buffer, &Length) ? APS_STATUS_OK : GetLastError();
}

ULONG64
ApsGetPhysicalMemorySize(
	VOID
	)
{
	MEMORYSTATUSEX State;

	State.dwLength = sizeof(State);
	GlobalMemoryStatusEx(&State);

	return (ULONG64)State.ullTotalPhys;
}

ULONG
ApsLoadRuntime(
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in PWSTR DllFullPathName,
	__in PSTR StartApiName,
	__in PVOID Argument
	)
{
	ULONG Status;
	HANDLE ThreadHandle;
	PVOID Buffer;
	SIZE_T Length;
	PVOID ThreadRoutine;

	Buffer = (PWCHAR)VirtualAllocEx(ProcessHandle, NULL, ApsPageSize, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) { 
		Status = GetLastError();
		return Status;
	}

	Length = (wcslen(DllFullPathName) + 1) * sizeof(WCHAR);
	Status = WriteProcessMemory(ProcessHandle, Buffer, DllFullPathName, Length, NULL); 
	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;
	}	
	
	ThreadRoutine = ApsGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
	if (!ThreadRoutine) {
		return APS_STATUS_INVALID_PARAMETER;
	}

	//
	// Runtime initialization stage 0 to do fundamental initialization
	//

	ThreadHandle = 0;
	if (ApsMajorVersion >= 6) {
		Status = (ULONG)(*RtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								               (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										       Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);
	CloseHandle(ThreadHandle);

	//
	// Check whether it's really loaded into target address space
	//

	Status = ApsGetModuleInformation(ProcessId, DllFullPathName, TRUE, 
		                             NULL, NULL, NULL);

	if (!Status) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return APS_STATUS_ERROR;
	}

	//
	// Runtime initialization stage 1 to start runtime threads
	//

	ThreadRoutine = ApsGetRemoteApiAddress(ProcessId, DllFullPathName, StartApiName);
	if (!ThreadRoutine) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return APS_STATUS_ERROR;
	}

	Status = WriteProcessMemory(ProcessHandle, Buffer, &Argument, sizeof(Argument), NULL); 
	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;
	}	

	ThreadHandle = 0;
	if (ApsMajorVersion >= 6) {
		Status = (ULONG)(*RtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								               (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										       Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);

	VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
	GetExitCodeThread(ThreadHandle, &Status);
	CloseHandle(ThreadHandle);
	return Status;
}

ULONG
ApsUnloadRuntime(
	__in ULONG ProcessId
	)
{
	return APS_STATUS_OK;
}

PVOID
ApsMalloc(
	__in SIZE_T Length
	)
{
	PVOID Ptr;

	Ptr = malloc(Length);
	if (Ptr != NULL) {
		RtlZeroMemory(Ptr, Length);
	}

	return Ptr;
}

VOID
ApsFree(
	__in PVOID Address
	)
{
	free(Address);
}

PVOID
ApsAlignedMalloc(
	__in ULONG ByteCount,
	__in ULONG Alignment
	)
{
	return _aligned_malloc(ByteCount, Alignment);
}

VOID
ApsAlignedFree(
	__in PVOID Address
	)
{
	_aligned_free(Address);
}

ULONG
ApsQueryProcessFullPath(
	__in ULONG ProcessId,
	__in PWSTR Buffer,
	__in ULONG Length,
	__out PULONG Result
	)
{
	HANDLE Handle;

	Handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (!Handle) {
		return APS_STATUS_ERROR;
	}
	
	*Result = GetModuleFileNameEx(Handle, NULL, Buffer, Length);
	CloseHandle(Handle);

	if (*Result != 0) {
		return APS_STATUS_OK;
	}

	return APS_STATUS_ERROR;
}

ULONG_PTR
ApsUlongPtrRoundDown(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	)
{
	return Value & ~(Align - 1);
}

ULONG
ApsUlongRoundDown(
	__in ULONG Value,
	__in ULONG Align
	)
{
	return Value & ~(Align - 1);
}

ULONG64
ApsUlong64RoundDown(
	__in ULONG64 Value,
	__in ULONG64 Align
	)
{
	return Value & ~(Align - 1);
}

ULONG_PTR
ApsUlongPtrRoundUp(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG
ApsUlongRoundUp(
	__in ULONG Value,
	__in ULONG Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG64
ApsUlong64RoundUp(
	__in ULONG64 Value,
	__in ULONG64 Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

BOOLEAN
ApsIsUlongAligned(
	__in ULONG Value,
	__in ULONG Unit
	)
{
	return ((Value & (Unit - 1)) == 0) ? TRUE : FALSE;
}

BOOLEAN
ApsIsUlong64Aligned(
	__in ULONG64 Value,
	__in ULONG64 Unit
	)
{
	return ((Value & (Unit - 1)) == 0) ? TRUE : FALSE;
}

double
ApsComputeMilliseconds(
	__in ULONG Duration
	)
{
	return (Duration * ApsSecondPerHardwareTick) * 1000;
}

VOID
ApsComputeHardwareFrequency(
	VOID
	)
{
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	ApsSecondPerHardwareTick = 1.0 / Frequency.QuadPart;
}

double
ApsNanoUnitToMilliseconds(
    __in ULONG NanoUnit
    )
{
    double Milliseconds;

    //
    // N.B. NanoUnit is 100 nanoseconds, this is provided by kernel,
    // e.g. GetThreadTimes(), 1 (ms) equal 1000 (us) * 1000 (ns)
    //

    Milliseconds = (NanoUnit * 100.0) / (1000.0 * 1000.0);
    return Milliseconds;
}

ULONG
ApsCreateWorldSecurityAttr(
	__out PAPS_SECURITY_ATTRIBUTE Attr
	)
{
    PACL Acl;
    PSID EveryoneSid;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // Initialize the security descriptor that allow Everyone group to access 
    //

	Acl = (PACL)ApsMalloc(1024);
    InitializeAcl(Acl, 1024, ACL_REVISION);

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)ApsMalloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    
	AllocateAndInitializeSid(&WorldAuthority, 1, SECURITY_WORLD_RID,
                             0, 0, 0, 0, 0, 0, 0, &EveryoneSid);

    AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, EveryoneSid);
    SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);

    Attr->Attribute.nLength = sizeof(SECURITY_ATTRIBUTES);
    Attr->Attribute.lpSecurityDescriptor = SecurityDescriptor;
    Attr->Attribute.bInheritHandle = FALSE;

	Attr->Acl = Acl;
	Attr->Sid = EveryoneSid;
	Attr->Descriptor = SecurityDescriptor;

	return APS_STATUS_OK;
}

HANDLE
ApsDuplicateHandle(
	__in HANDLE DestineProcess,
	__in HANDLE SourceHandle
	)
{
	ULONG Status;
	HANDLE DestineHandle;

	Status = DuplicateHandle(GetCurrentProcess(), 
				             SourceHandle,
							 DestineProcess,
							 &DestineHandle,
							 DUPLICATE_SAME_ACCESS,
							 FALSE,
							 DUPLICATE_SAME_ACCESS);
	if (Status) {
		return DestineHandle;
	}

	return NULL;
}

VOID
ApsInitCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	)
{
	ASSERT(Lock != NULL);
	InitializeCriticalSection(Lock);
}

VOID
ApsInitCriticalSectionEx(
	__in PAPS_CRITICAL_SECTION Lock,
	__in ULONG SpinCount
	)
{
	ASSERT(Lock != NULL);
	InitializeCriticalSectionAndSpinCount(Lock, SpinCount);
}

VOID
ApsEnterCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	)
{
	ASSERT(Lock != NULL);
	EnterCriticalSection(Lock);
}

BOOLEAN
ApsTryEnterCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	)
{
	ASSERT(Lock != NULL);
	return (BOOLEAN)TryEnterCriticalSection(Lock);
}

VOID
ApsLeaveCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	)
{
	ASSERT(Lock != NULL);
	LeaveCriticalSection(Lock);
}

VOID
ApsDeleteCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	)
{
	ASSERT(Lock != NULL);
	DeleteCriticalSection(Lock);
}

VOID
ApsInitSpinLock(
	__in PBTR_SPINLOCK Lock,
	__in ULONG SpinCount
	)
{
	Lock->Acquired = 0;
	Lock->ThreadId = 0;
	Lock->SpinCount = SpinCount;
}

VOID
ApsAcquireSpinLock(
	__in PBTR_SPINLOCK Lock
	)
{
	ULONG Acquired;
	ULONG ThreadId;
	ULONG OwnerId;
	ULONG Count;
	ULONG SpinCount;

	ThreadId = GetCurrentThreadId();
	OwnerId = ReadForWriteAccess(&Lock->ThreadId);
	if (OwnerId == ThreadId) {
	    return;
	}

	SpinCount = Lock->SpinCount;

	while (TRUE) {
		
		Acquired = InterlockedBitTestAndSet((volatile LONG *)&Lock->Acquired, 0);
		if (Acquired != 1) {
			Lock->ThreadId = ThreadId;
			break;
		}

		Count = 0;

		do {

			YieldProcessor();
			Acquired = ReadForWriteAccess(&Lock->Acquired);

			Count += 1;
			if (Count >= SpinCount) { 
				SwitchToThread();
				break;
			}
		} while (Acquired != 0);
	}	
}

VOID
ApsReleaseSpinLock(
	__in PBTR_SPINLOCK Lock
	)
{	
	Lock->ThreadId = 0;
	_InterlockedAnd((volatile LONG *)&Lock->Acquired, 0);
}

ULONG
ApsCreateUuidString(
	__in PWCHAR Buffer,
	__in ULONG Length
	)
{
	RPC_STATUS Status;
	UUID Uuid;
	PWSTR Ptr;

	Status = UuidCreate(&Uuid);
	if (Status != RPC_S_OK && Status != RPC_S_UUID_LOCAL_ONLY) {
		return APS_STATUS_NO_UUID;
	}

	UuidToString(&Uuid, &Ptr);	
	StringCchCopy(Buffer, Length, Ptr);
	RpcStringFree(&Ptr);

	return APS_STATUS_OK;
}

VOID
ApsConvertGuidToString(
	__in GUID *Guid,
	__out PCHAR Buffer,
	__in ULONG Length
	) 
{
	StringCchPrintfA(Buffer, Length, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
		             Guid->Data1, Guid->Data2, Guid->Data3, 
					 Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], 
					 Guid->Data4[3], Guid->Data4[4], Guid->Data4[5], 
					 Guid->Data4[6], Guid->Data4[7]); 
}

SIZE_T 
ApsConvertUnicodeToAnsi(
	__in PWSTR Source, 
	__out PCHAR *Destine
	)
{
	int Length;
	int RequiredLength;
	PCHAR Buffer;

	if (!Source) {
		*Destine = NULL;
		return 0;
	}

	Length = (int)wcslen(Source) + 1;
	RequiredLength = WideCharToMultiByte(CP_UTF8, 0, Source, Length, 0, 0, 0, 0);

	Buffer = (PCHAR)ApsMalloc(RequiredLength);
	Buffer[0] = 0;

	WideCharToMultiByte(CP_UTF8, 0, Source, Length, Buffer, RequiredLength, 0, 0);
	*Destine = Buffer;

	return Length;
}

VOID 
ApsConvertAnsiToUnicode(
	__in PCHAR Source,
	__out PWCHAR *Destine
	)
{
	ULONG Length;
	PWCHAR Buffer;

	if (!Source) {
		*Destine = NULL;
		return;
	}

	Length = (ULONG)strlen(Source) + 1;
	Buffer = (PWCHAR)ApsMalloc(Length * sizeof(WCHAR));
	Buffer[0] = L'0';

	MultiByteToWideChar(CP_UTF8, 0, Source, Length, Buffer, Length);
	*Destine = Buffer;
}

ULONG
ApsFormatAddress(
    __in PVOID Buffer,
    __in ULONG Length,
    __in PVOID Address,
    __in BOOLEAN Unicode
    )
{
   ULONG Cch;

   if (Unicode) {

#if defined(_M_IX86)
       Cch = swprintf_s((wchar_t *)Buffer, Length, L"0x%08x", Address);
#elif defined(_M_X64)
       Cch = swprintf_s((wchar_t *)Buffer, Length, L"0x%08x'%08x", 
                         (ULONG)((ULONG64)Address >> 32), (ULONG)Address);
#endif

   } else {

#if defined(_M_IX86)
       Cch = sprintf_s((char *)Buffer, Length, "0x%08x", Address);
#elif defined(_M_X64)
       Cch = sprintf_s((char *)Buffer, Length, "0x%08x'%08x", 
                       (ULONG)((ULONG64)Address >> 32), (ULONG)Address);
#endif
   }

   return Cch;
}

BOOLEAN
ApsIsRuntimeLoaded(
	__in ULONG ProcessId,
	__in PWSTR DllFullPathName
	)
{
	BOOLEAN Status;

	Status = ApsGetModuleInformation(ProcessId, DllFullPathName, TRUE, 
		                             NULL, NULL, NULL);
	return Status;
}

ULONG
ApsGetRuntimeStatus(
	__in PWSTR Path,
	__out PULONG Status
	)
{
	return APS_STATUS_NOT_IMPLEMENTED;
}

PVOID
ApsGetRemoteApiAddress(
	__in ULONG ProcessId,
	__in PWSTR DllName,
	__in PSTR ApiName
	)
{ 
	HMODULE DllHandle;
	ULONG_PTR Address;
	HMODULE LocalDllHandle;
	ULONG_PTR LocalAddress;
	SIZE_T Size;
	ULONG Status;
	BOOLEAN Free = FALSE;

	ASSERT(GetCurrentProcessId() != ProcessId);
	
	LocalDllHandle = GetModuleHandle(DllName);
	if (!LocalDllHandle) {

		//
		// Dll not loaded yet, load it, free it after usage
		//

		LocalDllHandle = LoadLibrary(DllName);
		if (!LocalDllHandle) {
			return NULL;
		}

		Free = TRUE;
	}
	
	LocalAddress = (ULONG_PTR)GetProcAddress(LocalDllHandle, ApiName);
	if (!LocalAddress) {

		if (Free) {
			FreeLibrary(LocalDllHandle);
		}

		return NULL;
	}

	//
	// N.B. It's only valid to a few system dll (non side by side), we call this
	// routine primarily to handle ASLR case.
	//

	Status = ApsGetModuleInformation(ProcessId, DllName, FALSE, &DllHandle, &Address, &Size);
	if (!Status) {
		
		if (Free) {
			FreeLibrary(LocalDllHandle);
		}

		return NULL;
	}

	if (Free) {
		FreeLibrary(LocalDllHandle);
	}

	return (PVOID)((ULONG_PTR)DllHandle + (LocalAddress - (ULONG_PTR)LocalDllHandle));	
}

VOID __cdecl
ApsDebugTrace(
	__in PSTR Format,
	__in ...
	)
{
#ifdef _DEBUG

	va_list arg;
	char format[512];
	char buffer[512];
	
	va_start(arg, Format);

	__try {
		StringCchVPrintfA(format, 512, Format, arg);
		StringCchPrintfA(buffer, 512, "[apspf]: %s\n", format);
		OutputDebugStringA(buffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
	
	va_end(arg);

#endif
}

VOID __cdecl
ApsTrace(
	__in PSTR Format,
	__in ...
	)
{
	va_list arg;
	char format[512];
	char buffer[512];
	
	va_start(arg, Format);

	__try {
		StringCchVPrintfA(format, 512, Format, arg);
		StringCchPrintfA(buffer, 512, "[apspf]: %s\n", format);
		OutputDebugStringA(buffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
	
	va_end(arg);
}

BOOLEAN
ApsIsValidPath(
	__in PWSTR Path
	)
{
	WCHAR Drive[6];
	WCHAR Dir[MAX_PATH];
	WCHAR Filename[MAX_PATH];

	_wsplitpath(Path, Drive, Dir, Filename, NULL);

	if (wcslen(Drive) != 0 && wcslen(Dir) != 0) {
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
ApsIsValidFolder(
	__in PWSTR Path
	)
{
	BOOL Status;

	Status = CreateDirectory(Path, NULL);
	return Status;
}

VOID
ApsFailFast(
	VOID
	)
{
	__debugbreak();
}