//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
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
NTQUERYINFORMATIONFILE    NtQueryInformationFile;

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

double BtrNanosecondPerHardwareTick;
LARGE_INTEGER BtrHardwareFrequency;


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

	QueryPerformanceFrequency(&BtrHardwareFrequency);
	BtrNanosecondPerHardwareTick = 1.0f / BtrHardwareFrequency.QuadPart;

	//
	// Get system routine address
	//

	DllHandle = GetModuleHandleW(L"ntdll.dll");
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

	Address = GetProcAddress(DllHandle, "NtQueryInformationFile");
	if (!Address) {
		return BTR_E_GETPROCADDRESS;
	}
	
	NtQueryInformationFile = (NTQUERYINFORMATIONFILE)Address;

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

//
// N.B. This routine just works as expected
//

BOOLEAN
HalQuerySkipOnSuccess(
	_In_ HANDLE Handle
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};
	ULONG Information;

	Status = (*NtQueryInformationFile)(Handle, &IoStatus, &Information, sizeof(ULONG), 
										FileIoCompletionNotificationInformation);
	if (NT_SUCCESS(Status)){
		return Information ? TRUE : FALSE;
	}
	return FALSE;
}

typedef struct _FILE_MODE_INFORMATION {
  ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION {
  ULONG ReadMode;
  ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

#define FILE_PIPE_QUEUE_OPERATION (0x00000000)    // Blocking mode 
#define FILE_PIPE_COMPLETE_OPERATION (0x00000001) // Non-blocking mode 

// 
// The FILE_COMPLETION_INFORMATION structure is used to replace the completion information 
// for a port handle set in Port. Completion information is replaced with the ZwSetInformationFile 
// routine with the FileInformationClass parameter set to FileReplaceCompletionInformation. 
// The Port and Key members of FILE_COMPLETION_INFORMATION are set to their new values. 
// To remove an existing completion port for a file handle, Port is set to NULL.
// (Require Windows 8.1+), this can be used to support live socket migration between a cluster of IOCP.
//

typedef struct _FILE_COMPLETION_INFORMATION {
  HANDLE Port;
  PVOID  Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

BOOLEAN
HalQueryOverlapped(
	_In_ HANDLE Handle
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};
	FILE_MODE_INFORMATION Information = {0};
	FILE_PIPE_INFORMATION Information2 = {0};

	//
	// From WDK
	//

	#define FILE_SYNCHRONOUS_IO_ALERT			0x00000010
	#define FILE_SYNCHRONOUS_IO_NONALERT		0x00000020

	Status = (*NtQueryInformationFile)(Handle, &IoStatus, &Information, sizeof(Information), 
										FileModeInformation);
	if (NT_SUCCESS(Status)){
		return !FlagOn(Information.Mode, FILE_SYNCHRONOUS_IO_NONALERT | FILE_SYNCHRONOUS_IO_ALERT);
	}
	return FALSE;
};


/*
int
SockNtStatusToSocketError (
    IN NTSTATUS Status
    )
{

    switch ( Status ) {

    case STATUS_PENDING:
        return ERROR_IO_PENDING;

    case STATUS_INVALID_HANDLE:
    case STATUS_OBJECT_TYPE_MISMATCH:
        return WSAENOTSOCK;

    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_PAGEFILE_QUOTA:
    case STATUS_COMMITMENT_LIMIT:
    case STATUS_WORKING_SET_QUOTA:
    case STATUS_NO_MEMORY:
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_QUOTA_EXCEEDED:
    case STATUS_TOO_MANY_PAGING_FILES:
    case STATUS_REMOTE_RESOURCES:
    case STATUS_TOO_MANY_ADDRESSES:
        return WSAENOBUFS;

    case STATUS_SHARING_VIOLATION:
    case STATUS_ADDRESS_ALREADY_EXISTS:
        return WSAEADDRINUSE;

    case STATUS_LINK_TIMEOUT:
    case STATUS_IO_TIMEOUT:
    case STATUS_TIMEOUT:
        return WSAETIMEDOUT;

    case STATUS_GRACEFUL_DISCONNECT:
        return WSAEDISCON;

    case STATUS_REMOTE_DISCONNECT:
    case STATUS_CONNECTION_RESET:
    case STATUS_LINK_FAILED:
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_PORT_UNREACHABLE:
        return WSAECONNRESET;

    case STATUS_LOCAL_DISCONNECT:
    case STATUS_TRANSACTION_ABORTED:
    case STATUS_CONNECTION_ABORTED:
        return WSAECONNABORTED;

    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_PROTOCOL_UNREACHABLE:
        return WSAENETUNREACH;

    case STATUS_HOST_UNREACHABLE:
        return WSAEHOSTUNREACH;

    case STATUS_CANCELLED:
    case STATUS_REQUEST_ABORTED:
        return WSAEINTR;

    case STATUS_BUFFER_OVERFLOW:
    case STATUS_INVALID_BUFFER_SIZE:
        return WSAEMSGSIZE;

    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_ACCESS_VIOLATION:
        return WSAEFAULT;

    case STATUS_DEVICE_NOT_READY:
    case STATUS_REQUEST_NOT_ACCEPTED:
        return WSAEWOULDBLOCK;

    case STATUS_INVALID_NETWORK_RESPONSE:
    case STATUS_NETWORK_BUSY:
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        return WSAENETDOWN;

    case STATUS_INVALID_CONNECTION:
        return WSAENOTCONN;

    case STATUS_REMOTE_NOT_LISTENING:
    case STATUS_CONNECTION_REFUSED:
        return WSAECONNREFUSED;

    case STATUS_PIPE_DISCONNECTED:
        return WSAESHUTDOWN;

    case STATUS_INVALID_ADDRESS:
    case STATUS_INVALID_ADDRESS_COMPONENT:
        return WSAEADDRNOTAVAIL;

    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_IMPLEMENTED:
        return WSAEOPNOTSUPP;

    case STATUS_ACCESS_DENIED:
        return WSAEACCES;

    default:

        if ( NT_SUCCESS(Status) ) {
            DebugTrace("SockNtStatusToSocketError: success status %lx not mapped\n", Status);
            return NO_ERROR;
        }
        DebugTrace("SockNtStatusToSocketError: unable to map 0x%lX, returning\n", Status);
        //
        // Fall over to default error code
        //

    case STATUS_UNSUCCESSFUL:
    case STATUS_INVALID_PARAMETER:
    case STATUS_ADDRESS_CLOSED:
    case STATUS_CONNECTION_INVALID:
    case STATUS_ADDRESS_ALREADY_ASSOCIATED:
    case STATUS_ADDRESS_NOT_ASSOCIATED:
    case STATUS_CONNECTION_ACTIVE:
    case STATUS_INVALID_DEVICE_STATE:
    case STATUS_INVALID_DEVICE_REQUEST:
        return WSAEINVAL;

    }

} // SockNtStatusToSocketError

*/