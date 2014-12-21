//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "btr.h"
#include "hal.h"
#include "util.h"
#include "apsprofile.h"

ULONG BtrNtMajorVersion;
ULONG BtrNtMinorVersion;
ULONG BtrNtServicePack;

ULONG_PTR BtrMaximumUserAddress;
ULONG_PTR BtrMinimumUserAddress;
ULONG_PTR BtrAllocationGranularity;
ULONG_PTR BtrPageSize;
ULONG BtrNumberOfProcessors;

RTLCREATEUSERTHREAD RtlCreateUserThread;
RTLEXITUSERTHREAD RtlExitUserThread;

RTLINITANSISTRING RtlInitAnsiString;
RTLFREEANSISTRING RtlFreeAnsiString;
RTLINITUNICODESTRING RtlInitUnicodeString;
RTLFREEUNICODESTRING RtlFreeUnicodeString;
RTLUNICODESTRINGTOANSISTRING RtlUnicodeStringToAnsiString;
RTLANSISTRINGTOUNICODESTRING RtlAnsiStringToUnicodeString;

NTQUERYSYSTEMINFORMATION  NtQuerySystemInformation;
NTQUERYINFORMATIONPROCESS NtQueryInformationProcess;
NTQUERYINFORMATIONTHREAD  NtQueryInformationThread;

LDRGETPROCEDUREADDRESS LdrGetProcedureAddress;
LDRLOCKLOADERLOCK LdrLockLoaderLock;
LDRUNLOCKLOADERLOCK LdrUnlockLoaderLock;

RTLGETCURRENTPROCESSORNUMBER RtlGetCurrentProcessorNumber;

//
// IsWow64Process()
//

typedef BOOL (WINAPI *ISWOW64PROCESS)(HANDLE, PBOOL);
ISWOW64PROCESS HalIsWow64ProcessPtr;

ULONG_PTR BtrDllBase;
ULONG_PTR BtrDllSize;

double BtrMillisecondPerHardwareTick;

#if defined(_M_X64)
RTLLOOKUPFUNCTIONTABLE RtlLookupFunctionTable;
#endif

#if defined(_M_X64)

BTR_CPU_TYPE BtrCpuType = CPU_X86_64;
BTR_TARGET_TYPE BtrTargetType = TARGET_WIN64;

#elif defined(_M_IX86)

BTR_CPU_TYPE BtrCpuType = CPU_X86_32;
BTR_TARGET_TYPE BtrTargetType = TARGET_WIN32;

#endif


ULONG
BtrInitializeHal(
	VOID
	)
{
	HMODULE DllHandle;
	OSVERSIONINFOEX Information;
	SYSTEM_INFO SystemInformation = {0};
	MODULEINFO Module = {0};
	PVOID Address;
	LARGE_INTEGER Frequency;

	Information.dwOSVersionInfoSize = sizeof(Information);
	GetVersionEx((LPOSVERSIONINFO)&Information);

	BtrNtMajorVersion = Information.dwMajorVersion;
	BtrNtMinorVersion = Information.dwMinorVersion;
	BtrNtServicePack = Information.wServicePackMajor;

	GetSystemInfo(&SystemInformation);
	BtrMinimumUserAddress = (ULONG_PTR)SystemInformation.lpMinimumApplicationAddress;
	BtrMaximumUserAddress = (ULONG_PTR)SystemInformation.lpMaximumApplicationAddress;
	BtrAllocationGranularity = (ULONG_PTR)SystemInformation.dwAllocationGranularity;
	BtrPageSize = (ULONG_PTR)SystemInformation.dwPageSize;
	BtrNumberOfProcessors = SystemInformation.dwNumberOfProcessors;

	//
	// Compute milliseconds per hardware tick
	//

	QueryPerformanceFrequency(&Frequency);
	BtrMillisecondPerHardwareTick = (1.0 * 1000) / Frequency.QuadPart;

	//
	// Get system routine address
	//

	DllHandle = GetModuleHandle(L"ntdll.dll");
	if (!DllHandle) {
		return BTR_E_GETMODULEHANDLE;
	}

	Address = GetProcAddress(DllHandle, "RtlCreateUserThread");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlCreateUserThread = (RTLCREATEUSERTHREAD)Address;

	Address = GetProcAddress(DllHandle, "RtlExitUserThread");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlExitUserThread = (RTLEXITUSERTHREAD)Address;

	Address = GetProcAddress(DllHandle, "RtlInitUnicodeString");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlInitUnicodeString = (RTLINITUNICODESTRING)Address;

	Address = GetProcAddress(DllHandle, "RtlFreeUnicodeString");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlFreeUnicodeString = (RTLFREEUNICODESTRING)Address;

	Address = GetProcAddress(DllHandle, "RtlInitAnsiString");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlInitAnsiString = (RTLINITANSISTRING)Address;

	Address = GetProcAddress(DllHandle, "RtlFreeAnsiString");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlFreeAnsiString = (RTLFREEANSISTRING)Address;

	Address = GetProcAddress(DllHandle, "RtlUnicodeStringToAnsiString");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlUnicodeStringToAnsiString = (RTLUNICODESTRINGTOANSISTRING)Address;

	Address = GetProcAddress(DllHandle, "RtlAnsiStringToUnicodeString");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	RtlAnsiStringToUnicodeString = (RTLANSISTRINGTOUNICODESTRING)Address;

	Address = GetProcAddress(DllHandle, "NtQuerySystemInformation");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}

	NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)Address;

	Address = GetProcAddress(DllHandle, "NtQueryInformationProcess");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	NtQueryInformationProcess = (NTQUERYINFORMATIONPROCESS)Address;

	Address = GetProcAddress(DllHandle, "NtQueryInformationThread");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	NtQueryInformationThread = (NTQUERYINFORMATIONTHREAD)Address;

	Address = GetProcAddress(DllHandle, "LdrGetProcedureAddress");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	LdrGetProcedureAddress = (LDRGETPROCEDUREADDRESS)Address;

	Address = GetProcAddress(DllHandle, "LdrLockLoaderLock");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	LdrLockLoaderLock = (LDRLOCKLOADERLOCK)Address;

	Address = GetProcAddress(DllHandle, "LdrUnlockLoaderLock");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	LdrUnlockLoaderLock = (LDRUNLOCKLOADERLOCK)Address;

#if defined (_M_X64)

	Address = GetProcAddress(DllHandle, "RtlLookupFunctionTable");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	RtlLookupFunctionTable = (RTLLOOKUPFUNCTIONTABLE)Address;

#endif

#if defined(_M_IX86)
	
	KiFastSystemCallRet = GetProcAddress(DllHandle, "KiFastSystemCallRet");

#endif

	//
	// N.B. RtlGetCurrentProcessorNumber is available for NT 5.2 above,
	// Windows XP is not supported.
	//

	Address = GetProcAddress(DllHandle, "RtlGetCurrentProcessorNumber");
	RtlGetCurrentProcessorNumber = (RTLGETCURRENTPROCESSORNUMBER)Address;

	//
	// Get runtime dll base address and its size 
	//

	GetModuleInformation(GetCurrentProcess(), (HMODULE)BtrDllBase, &Module, sizeof(Module));
	BtrDllSize = Module.SizeOfImage;

	//
	// If it's WOW64, change its target type
	//

	if (HalIsWow64Process()) {
		BtrTargetType = TARGET_WOW64;
	}

	return S_OK;
}

ULONG 
BtrCurrentProcessor(
	VOID
	)
{
	if (RtlGetCurrentProcessorNumber != NULL) {
		return (*RtlGetCurrentProcessorNumber)();
	}

	//
	// N.B. It's only for Windows XP
	//

	return 0;
}

VOID
BtrUninitializeHal(
	VOID
	)
{
	return;
}

VOID
BtrPrepareUnloadDll(
	IN PVOID Address 
	)
{
	PPEB Peb;
	PTEB Teb;
	PPEB_LDR_DATA Ldr; 
	PLDR_DATA_TABLE_ENTRY Module; 
	ULONG Cookie = 0;
	BOOLEAN Status;

	Teb = NtCurrentTeb();
	Peb = (PPEB)Teb->ProcessEnvironmentBlock;
	Ldr = Peb->Ldr;

	Status = BtrLockLoaderLock(&Cookie);

	__try {

		Module = (LDR_DATA_TABLE_ENTRY *)Ldr->InLoadOrderModuleList.Flink;

		while (Module->DllBase != 0) { 

			if ((ULONG_PTR)Module->DllBase <= (ULONG_PTR)Address && 
				(ULONG_PTR)Address < ((ULONG_PTR)Module->DllBase + Module->SizeOfImage)) {

					Module->LoadCount = 1;
					break;
			}

			Module = (LDR_DATA_TABLE_ENTRY *)Module->InLoadOrderLinks.Flink; 
		} 

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		
		//
		// Nothing can be done here
		//
	}
	
	if (Status) {

		//
		// Only unload if loader lock was locked
		//

		BtrUnlockLoaderLock(Cookie);
	}
}

ULONG
BtrGetProcedureAddress(
	IN PVOID BaseAddress,
	IN PSTR Name,
	IN ULONG Ordinal,
	OUT PVOID *ProcedureAddress
	)
{
	NTSTATUS Status;
	ANSI_STRING Ansi;

	__try {
		(*RtlInitAnsiString)(&Ansi, Name);
		Status = (*LdrGetProcedureAddress)(BaseAddress, &Ansi, Ordinal, ProcedureAddress);
		(*RtlFreeAnsiString)(&Ansi);
	}__except(EXCEPTION_EXECUTE_HANDLER) {
		return S_FALSE;
	}

	if (NT_SUCCESS(Status)) {
		return S_OK;
	}

	return S_FALSE;
}

BOOLEAN
BtrLockLoaderLock(
	OUT PULONG Cookie	
	)
{
	NTSTATUS Status;

	__try {
		if (LdrLockLoaderLock) {
			Status = (*LdrLockLoaderLock)(1, NULL, Cookie);
			return (Status == STATUS_SUCCESS) ? TRUE : FALSE;
		}
	}__except(EXCEPTION_EXECUTE_HANDLER) {
		return FALSE;
	}

	return FALSE;
}

VOID
BtrUnlockLoaderLock(
	IN ULONG Cookie	
	)
{
	__try {
		if (LdrUnlockLoaderLock) {
			(*LdrUnlockLoaderLock)(1, Cookie);
		}
	}__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}

ULONG
BtrQueryProcessInformation(
	IN ULONG ProcessId,
	OUT PSYSTEM_PROCESS_INFORMATION *Information
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

	Status = BtrAllocateQueryBuffer(SystemProcessInformation, 
		                            &StartVa,
								    &Length,
									&ReturnedLength);

	if (Status != ERROR_SUCCESS) {
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
		*Information  = (PSYSTEM_PROCESS_INFORMATION)VirtualAlloc(NULL, Length, MEM_COMMIT, PAGE_READWRITE);
		RtlCopyMemory(*Information, Entry, Length);
		break;
	}
	
	BtrReleaseQueryBuffer(StartVa);
	return Status;
}

ULONG
BtrAllocateQueryBuffer(
	IN SYSTEM_INFORMATION_CLASS InformationClass,
	OUT PVOID *Buffer,
	OUT PULONG BufferLength,
	OUT PULONG ActualLength
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
		// Incremental unit is 64K this is aligned with Mm subsystem
		//

		Length += 0x10000;

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

	return ERROR_SUCCESS;
}

VOID
BtrReleaseQueryBuffer(
	IN PVOID StartVa
	)
{
	VirtualFree(StartVa, 0, MEM_RELEASE);
}

PVOID
BtrQueryTebAddress(
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
HalGetProcedureAddress(
	__in PVOID ModulePtr,
	__in PCSTR Name
	)
{
	PVOID Address;

	Address = (PVOID)GetProcAddress((HMODULE)ModulePtr, Name);
	return Address;
}

PVOID
HalGetModulePtr(
	__in PCSTR Name
	)
{
	PVOID Ptr;

	Ptr = (PVOID)GetModuleHandleA(Name);
	return Ptr;
}

BOOLEAN 
HalIsWow64Process(
	VOID
	)
{
    BOOL IsWow64 = FALSE;
   
    if (!HalIsWow64ProcessPtr) {
		HalIsWow64ProcessPtr = (ISWOW64PROCESS)GetProcAddress(
								GetModuleHandleA("kernel32.dll"), "IsWow64Process"); 
		if (!HalIsWow64ProcessPtr) {
			return FALSE;
		}
    }
    
	HalIsWow64ProcessPtr(GetCurrentProcess(), &IsWow64);
    return (BOOLEAN)IsWow64;
}

BTR_CPU_TYPE
HalGetCpuType(
	VOID
	)
{
	return BtrCpuType;
}

BTR_TARGET_TYPE
HalGetTargetType(
	VOID
	)
{
	return BtrTargetType;
}