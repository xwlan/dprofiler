//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_HANDLE_H_
#define _MM_HANDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mmprof.h"
#include <winsock2.h>


//
// Process
//

BOOL WINAPI 
CreateProcessEnterA(
	__in  PSTR lpApplicationName,
	__inout PSTR lpCommandLine,
	__in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  BOOL bInheritHandles,
	__in  DWORD dwCreationFlags,
	__in  LPVOID lpEnvironment,
	__in  PSTR lpCurrentDirectory,
	__in  LPSTARTUPINFO lpStartupInfo,
	__out LPPROCESS_INFORMATION lpProcessInformation
	);

typedef BOOL 
(WINAPI *CREATEPROCESS_A)(
	__in  PSTR lpApplicationName,
	__inout PSTR lpCommandLine,
	__in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  BOOL bInheritHandles,
	__in  DWORD dwCreationFlags,
	__in  LPVOID lpEnvironment,
	__in  PSTR lpCurrentDirectory,
	__in  LPSTARTUPINFO lpStartupInfo,
	__out LPPROCESS_INFORMATION lpProcessInformation
	);

BOOL WINAPI 
CreateProcessEnterW(
	__in  PWSTR lpApplicationName,
	__inout PWSTR lpCommandLine,
	__in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  BOOL bInheritHandles,
	__in  DWORD dwCreationFlags,
	__in  LPVOID lpEnvironment,
	__in  PWSTR lpCurrentDirectory,
	__in  LPSTARTUPINFO lpStartupInfo,
	__out LPPROCESS_INFORMATION lpProcessInformation
	);

typedef BOOL 
(WINAPI *CREATEPROCESS_W)(
	__in  PWSTR lpApplicationName,
	__inout PWSTR lpCommandLine,
	__in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  BOOL bInheritHandles,
	__in  DWORD dwCreationFlags,
	__in  LPVOID lpEnvironment,
	__in  PWSTR lpCurrentDirectory,
	__in  LPSTARTUPINFO lpStartupInfo,
	__out LPPROCESS_INFORMATION lpProcessInformation
	);

BOOL WINAPI 
CreateProcessAsUserEnterA(
  __in  HANDLE hToken,
  __in  PSTR lpApplicationName,
  __in  PSTR lpCommandLine,
  __in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  __in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  __in  BOOL bInheritHandles,
  __in  DWORD dwCreationFlags,
  __in  LPVOID lpEnvironment,
  __in  PSTR lpCurrentDirectory,
  __in  LPSTARTUPINFO lpStartupInfo,
  __out LPPROCESS_INFORMATION lpProcessInformation
);

typedef BOOL 
(WINAPI *CREATEPROCESSASUSER_A)(
  __in  HANDLE hToken,
  __in  PSTR lpApplicationName,
  __in  PSTR lpCommandLine,
  __in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  __in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  __in  BOOL bInheritHandles,
  __in  DWORD dwCreationFlags,
  __in  LPVOID lpEnvironment,
  __in  PSTR lpCurrentDirectory,
  __in  LPSTARTUPINFO lpStartupInfo,
  __out LPPROCESS_INFORMATION lpProcessInformation
  );

BOOL WINAPI 
CreateProcessAsUserEnterW(
  __in  HANDLE hToken,
  __in  PWSTR lpApplicationName,
  __in  PWSTR lpCommandLine,
  __in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  __in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  __in  BOOL bInheritHandles,
  __in  DWORD dwCreationFlags,
  __in  LPVOID lpEnvironment,
  __in  PWSTR lpCurrentDirectory,
  __in  LPSTARTUPINFO lpStartupInfo,
  __out LPPROCESS_INFORMATION lpProcessInformation
  );

typedef BOOL 
(WINAPI *CREATEPROCESSASUSER_W)(
  __in  HANDLE hToken,
  __in  PWSTR lpApplicationName,
  __in  PWSTR lpCommandLine,
  __in  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  __in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  __in  BOOL bInheritHandles,
  __in  DWORD dwCreationFlags,
  __in  LPVOID lpEnvironment,
  __in  PWSTR lpCurrentDirectory,
  __in  LPSTARTUPINFO lpStartupInfo,
  __out LPPROCESS_INFORMATION lpProcessInformation
  );

BOOL WINAPI 
CreateProcessWithLogonWEnter(
	__in  LPCWSTR lpUsername,
	__in  LPCWSTR lpDomain,
	__in  LPCWSTR lpPassword,
	__in  DWORD dwLogonFlags,
	__in  LPCWSTR lpApplicationName,
	__in  LPWSTR lpCommandLine,
	__in  DWORD dwCreationFlags,
	__in  LPVOID lpEnvironment,
	__in  LPCWSTR lpCurrentDirectory,
	__in  LPSTARTUPINFOW lpStartupInfo,
	__out LPPROCESS_INFORMATION lpProcessInfo
	);

typedef BOOL 
(WINAPI *CREATEPROCESSWITHLOGONW)(
	__in  LPCWSTR lpUsername,
	__in  LPCWSTR lpDomain,
	__in  LPCWSTR lpPassword,
	__in  DWORD dwLogonFlags,
	__in  LPCWSTR lpApplicationName,
	__in  LPWSTR lpCommandLine,
	__in  DWORD dwCreationFlags,
	__in  LPVOID lpEnvironment,
	__in  LPCWSTR lpCurrentDirectory,
	__in  LPSTARTUPINFOW lpStartupInfo,
	__out LPPROCESS_INFORMATION lpProcessInfo
	);

HANDLE WINAPI 
OpenProcessEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	);

typedef HANDLE 
(WINAPI *OPENPROCESS)(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	);

//
// Thread
//

HANDLE WINAPI 
CreateThreadEnter(
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);


typedef HANDLE 
(WINAPI *CREATETHREAD)(
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

HANDLE WINAPI 
CreateRemoteThreadEnter(
	__in  HANDLE hProcess,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

typedef HANDLE 
(WINAPI *CREATEREMOTETHREAD)(
	__in  HANDLE hProcess,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

HANDLE WINAPI 
OpenThreadEnter(
	__in DWORD dwDesiredAccess,
	__in BOOL bInheritHandle,
	__in DWORD dwThreadId
	);

typedef HANDLE
(WINAPI *OPENTHREAD)(
	__in DWORD dwDesiredAccess,
	__in BOOL bInheritHandle,
	__in DWORD dwThreadId
	);


//
// Job
//

HANDLE WINAPI 
CreateJobObjectEnter(
	__in  LPSECURITY_ATTRIBUTES lpJobAttributes,
	__in  PWSTR lpName
	);

typedef HANDLE 
(WINAPI *CREATEJOBOBJECT_W)(
	__in  LPSECURITY_ATTRIBUTES lpJobAttributes,
	__in  PWSTR lpName
	);

HANDLE WINAPI 
OpenJobObjectEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  PWSTR lpName
	);

typedef HANDLE 
(WINAPI *OPENJOBOBJECT_W)(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandles,
	__in  PWSTR lpName
	);

//
// File System (file, directory, volume etc)
//

HANDLE WINAPI
CreateFileWEnter(
	IN PWSTR lpFileName,
	IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
	);

typedef HANDLE 
(WINAPI *CREATEFILE_W)(
	IN PWSTR lpFileName,
	IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
	);

HANDLE WINAPI 
FindFirstFileExWEnter(
	__in  PWSTR lpFileName,
	__in  FINDEX_INFO_LEVELS fInfoLevelId,
	__out LPVOID lpFindFileData,
	__in  FINDEX_SEARCH_OPS fSearchOp,
	__in  LPVOID lpSearchFilter,
	__in DWORD dwAdditionalFlags
	);

typedef HANDLE 
(WINAPI *FINDFIRSTFILEEX_W)(
	__in  PWSTR lpFileName,
	__in  FINDEX_INFO_LEVELS fInfoLevelId,
	__out LPVOID lpFindFileData,
	__in  FINDEX_SEARCH_OPS fSearchOp,
	__in  LPVOID lpSearchFilter,
	__in DWORD dwAdditionalFlags
	);

BOOL WINAPI
FindCloseEnter(
	IN HANDLE hFile
	);

typedef BOOL 
(WINAPI *FINDCLOSE)(
	IN HANDLE hFile
	);

HANDLE WINAPI 
FindFirstVolumeEnter(
	OUT PWSTR lpszVolumeName,
	IN  DWORD cchBufferLength
	);

typedef HANDLE 
(WINAPI *FINDFIRSTVOLUME_W)(
	OUT PWSTR lpszVolumeName,
	IN  DWORD cchBufferLength
	);

BOOL WINAPI 
FindVolumeCloseEnter(
	IN HANDLE hFindVolume
	);

typedef BOOL
(WINAPI *FINDVOLUMECLOSE)(
	IN HANDLE hFindVolume
	);

HANDLE WINAPI 
FindFirstVolumeMountPointEnter(
	__in  PWSTR lpszRootPathName,
	__out PWSTR lpszVolumeMountPoint,
	__in  DWORD cchBufferLength
	);

typedef HANDLE 
(WINAPI *FINDFIRSTVOLUMEMOUNTPOINT_W)(
	__in  PWSTR lpszRootPathName,
	__out PWSTR lpszVolumeMountPoint,
	__in  DWORD cchBufferLength
	);

BOOL WINAPI 
FindVolumeMountPointCloseEnter(
	__in  HANDLE hFindVolumeMountPoint
	);

typedef BOOL 
(WINAPI *FINDVOLUMEMOUNTPOINTCLOSE)(
	__in  HANDLE hFindVolumeMountPoint
	);

HANDLE WINAPI 
FindFirstChangeNotificationEnter(
	__in  PWSTR lpPathName,
	__in  BOOL bWatchSubtree,
	__in  DWORD dwNotifyFilter
	);

typedef HANDLE 
(WINAPI *FINDFIRSTCHANGENOTIFICATION_W)(
	__in  PWSTR lpPathName,
	__in  BOOL bWatchSubtree,
	__in  DWORD dwNotifyFilter
	);

BOOL WINAPI 
FindCloseChangeNotificationEnter(
	__in  HANDLE hChangeHandle
	);

typedef BOOL
(WINAPI *FINDCLOSECHANGENOTIFICATION)(
	__in  HANDLE hChangeHandle
	);

//
// File Mapping
//

HANDLE WINAPI 
CreateFileMappingWEnter(
	IN HANDLE hFile,
	IN LPSECURITY_ATTRIBUTES lpAttributes,
	IN DWORD flProtect,
	IN DWORD dwMaximumSizeHigh,
	IN DWORD dwMaximumSizeLow,
	IN PWSTR lpName
	);

typedef HANDLE 
(WINAPI *CREATEFILEMAPPING_W)(
	IN HANDLE hFile,
	IN LPSECURITY_ATTRIBUTES lpAttributes,
	IN DWORD flProtect,
	IN DWORD dwMaximumSizeHigh,
	IN DWORD dwMaximumSizeLow,
	IN PWSTR lpName
	);

HANDLE WINAPI 
OpenFileMappingWEnter(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN PWSTR lpName
	);

typedef HANDLE 
(WINAPI *OPENFILEMAPPING_W)(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN PWSTR lpName
	);

LPVOID WINAPI 
MapViewOfFileEnter(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap
	);

typedef LPVOID 
(WINAPI *MAPVIEWOFFILE)(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap
	);

LPVOID WINAPI 
MapViewOfFileExEnter(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap,
	IN LPVOID lpBaseAddress
	);

typedef LPVOID 
(WINAPI *MAPVIEWOFFILEEX)(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap,
	IN LPVOID lpBaseAddress
	);

BOOL WINAPI 
UnmapViewOfFileEnter(
	IN LPCVOID lpBaseAddress
	);

typedef BOOL 
(WINAPI *UNMAPVIEWOFFILE)(
	IN LPCVOID lpBaseAddress
	);

HANDLE WINAPI 
OpenFileMappingWEnter(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN PWSTR lpName
	);

typedef HANDLE 
(WINAPI *OPENFILEMAPPING_W)(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN PWSTR lpName
	);

//
// Socket
//

SOCKET WINAPI
WSASocketEnter(
	IN int af,
	IN int type,
	IN int protocol,
	IN LPWSAPROTOCOL_INFO lpProtocolInfo,
	IN GROUP g,
	IN DWORD dwFlags
	);

typedef SOCKET 
(WINAPI *WSASOCKET)(
	IN int af,
	IN int type,
	IN int protocol,
	IN LPWSAPROTOCOL_INFO lpProtocolInfo,
	IN GROUP g,
	IN DWORD dwFlags
	);

SOCKET WINAPI
WSAAcceptEnter(
	IN SOCKET s,
	OUT struct sockaddr* addr,
	OUT int* addrlen,
	IN LPCONDITIONPROC lpfnCondition,
	IN DWORD dwCallbackData
	);

typedef SOCKET 
(WINAPI *WSAACCEPT)(
	IN SOCKET s,
	OUT struct sockaddr* addr,
	OUT int* addrlen,
	IN LPCONDITIONPROC lpfnCondition,
	IN DWORD dwCallbackData
	);

int WINAPI
CloseSocketEnter(
	IN SOCKET s
	);

typedef int 
(WINAPI *CLOSESOCKET)(
	IN SOCKET s
	);

//
// Event 
//

HANDLE WINAPI 
CreateEventWEnter(
	__in LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in BOOL bManualReset,
	__in BOOL bInitialState,
	__in PWSTR lpName
	);

typedef HANDLE 
(WINAPI *CREATEEVENT_W)(
	__in LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in BOOL bManualReset,
	__in BOOL bInitialState,
	__in PWSTR lpName
	);

HANDLE WINAPI 
CreateEventExWEnter(
	__in  LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

typedef HANDLE 
(WINAPI *CREATEEVENTEX_W)(
	__in  LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI
OpenEventEnter( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	); 

typedef HANDLE 
(WINAPI *OPENEVENT_W)( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	); 


//
// Mutex
//

HANDLE WINAPI 
CreateMutexWEnter(
	__in  LPSECURITY_ATTRIBUTES lpMutexAttributes,
	__in  BOOL bInitialOwner,
	__in  PWSTR lpName
	);

typedef HANDLE 
(WINAPI *CREATEMUTEX_W)(
	__in  LPSECURITY_ATTRIBUTES lpMutexAttributes,
	__in  BOOL bInitialOwner,
	__in  PWSTR lpName
	);

HANDLE WINAPI 
CreateMutexExWEnter(
	__in  LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

typedef HANDLE 
(WINAPI *CREATEMUTEXEX_W)(
	__in  LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI
OpenMutexEnter( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	); 

typedef HANDLE 
(WINAPI *OPENMUTEX_W)( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	); 

//
// Semaphore
//

HANDLE WINAPI 
CreateSemaphoreWEnter(
	__in LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in LONG lInitialCount,
	__in LONG lMaximumCount,
	__in PWSTR lpName
	);

typedef HANDLE 
(WINAPI *CREATESEMAPHORE_W)(
	__in LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in LONG lInitialCount,
	__in LONG lMaximumCount,
	__in PWSTR lpName
	);

HANDLE WINAPI 
CreateSemaphoreExWEnter(
	__in  LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in  LONG lInitialCount,
	__in  LONG lMaximumCount,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

typedef HANDLE 
(WINAPI *CREATESEMAPHOREEX_W)(
	__in  LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in  LONG lInitialCount,
	__in  LONG lMaximumCount,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI
OpenSemaphoreEnter( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	); 

typedef HANDLE 
(WINAPI *OPENSEMAPHORE_W)( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	); 

//
// Timer
//

HANDLE WINAPI 
CreateWaitableTimerExEnter(
	__in  LPSECURITY_ATTRIBUTES lpTimerAttributes,
	__in  PWSTR lpTimerName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

typedef HANDLE 
(WINAPI *CREATEWAITABLETIMEREX_W)(
	__in  LPSECURITY_ATTRIBUTES lpTimerAttributes,
	__in  PWSTR lpTimerName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI 
OpenWaitableTimerEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  PWSTR lpTimerName
	);

typedef HANDLE 
(WINAPI *OPENWAITABLETIMER_W)(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  PWSTR lpTimerName
	);

//
// I/O Completion Port
//

HANDLE WINAPI 
CreateIoCompletionPortEnter(
	__in HANDLE FileHandle,
	__in HANDLE ExistingCompletionPort,
	__in ULONG_PTR CompletionKey,
	__in DWORD NumberOfConcurrentThreads
	);

typedef HANDLE 
(WINAPI *CREATEIOCOMPLETIONPORT)(
	__in HANDLE FileHandle,
	__in HANDLE ExistingCompletionPort,
	__in ULONG_PTR CompletionKey,
	__in DWORD NumberOfConcurrentThreads
	);

//
// Pipe
//

BOOL WINAPI 
CreatePipeEnter(
	__out PHANDLE hReadPipe,
	__out PHANDLE hWritePipe,
	__in  LPSECURITY_ATTRIBUTES lpPipeAttributes,
	__in  DWORD nSize
	);

typedef BOOL 
(WINAPI *CREATEPIPE)(
	__out PHANDLE hReadPipe,
	__out PHANDLE hWritePipe,
	__in  LPSECURITY_ATTRIBUTES lpPipeAttributes,
	__in  DWORD nSize
	);


HANDLE WINAPI 
CreateNamedPipeEnter(
	__in  PWSTR lpName,
	__in  DWORD dwOpenMode,
	__in  DWORD dwPipeMode,
	__in  DWORD nMaxInstances,
	__in  DWORD nOutBufferSize,
	__in  DWORD nInBufferSize,
	__in  DWORD nDefaultTimeOut,
	__in  LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

typedef HANDLE 
(WINAPI *CREATENAMEDPIPE_W)(
	__in  PWSTR lpName,
	__in  DWORD dwOpenMode,
	__in  DWORD dwPipeMode,
	__in  DWORD nMaxInstances,
	__in  DWORD nOutBufferSize,
	__in  DWORD nInBufferSize,
	__in  DWORD nDefaultTimeOut,
	__in  LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

//
// Mailslot
//

HANDLE WINAPI 
CreateMailslotEnter(
	__in PWSTR lpName,
	__in DWORD nMaxMessageSize,
	__in DWORD lReadTimeout,
	__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

typedef HANDLE 
(WINAPI *CREATEMAILSLOT_W)(
	__in PWSTR lpName,
	__in DWORD nMaxMessageSize,
	__in DWORD lReadTimeout,
	__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

//
// Token
//

BOOL WINAPI 
CreateRestrictedTokenEnter(
	__in     HANDLE ExistingTokenHandle,
	__in     DWORD Flags,
	__in     DWORD DisableSidCount,
	__in_opt PSID_AND_ATTRIBUTES SidsToDisable,
	__in     DWORD DeletePrivilegeCount,
	__in_opt PLUID_AND_ATTRIBUTES PrivilegesToDelete,
	__in     DWORD RestrictedSidCount,
	__in_opt PSID_AND_ATTRIBUTES SidsToRestrict,
	__out    PHANDLE NewTokenHandle
	);

typedef BOOL 
(WINAPI *CREATERESTRICTEDTOKEN)(
	__in     HANDLE ExistingTokenHandle,
	__in     DWORD Flags,
	__in     DWORD DisableSidCount,
	__in_opt PSID_AND_ATTRIBUTES SidsToDisable,
	__in     DWORD DeletePrivilegeCount,
	__in_opt PLUID_AND_ATTRIBUTES PrivilegesToDelete,
	__in     DWORD RestrictedSidCount,
	__in_opt PSID_AND_ATTRIBUTES SidsToRestrict,
	__out    PHANDLE NewTokenHandle
	);

BOOL WINAPI 
DuplicateTokenEnter(
	__in  HANDLE ExistingTokenHandle,
	__in  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	__out PHANDLE DuplicateTokenHandle
	);

typedef BOOL 
(WINAPI *DUPLICATETOKEN)(
	__in  HANDLE ExistingTokenHandle,
	__in  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	__out PHANDLE DuplicateTokenHandle
	);

BOOL WINAPI DuplicateTokenExEnter(
	__in  HANDLE hExistingToken,
	__in  DWORD dwDesiredAccess,
	__in_opt  LPSECURITY_ATTRIBUTES lpTokenAttributes,
	__in  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	__in  TOKEN_TYPE TokenType,
	__out PHANDLE phNewToken
	);

typedef BOOL 
(WINAPI *DUPLICATETOKENEX)(
	__in  HANDLE hExistingToken,
	__in  DWORD dwDesiredAccess,
	__in_opt  LPSECURITY_ATTRIBUTES lpTokenAttributes,
	__in  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	__in  TOKEN_TYPE TokenType,
	__out PHANDLE phNewToken
	);

BOOL WINAPI 
OpenProcessTokenEnter(
	__in  HANDLE ProcessHandle,
	__in  DWORD DesiredAccess,
	__out PHANDLE TokenHandle
	);

typedef BOOL 
(WINAPI *OPENPROCESSTOKEN)(
	__in  HANDLE ProcessHandle,
	__in  DWORD DesiredAccess,
	__out PHANDLE TokenHandle
	);

BOOL WINAPI 
OpenThreadTokenEnter(
	__in  HANDLE ThreadHandle,
	__in  DWORD DesiredAccess,
	__in  BOOL OpenAsSelf,
	__out PHANDLE TokenHandle
	);

typedef BOOL 
(WINAPI *OPENTHREADTOKEN)(
	__in  HANDLE ThreadHandle,
	__in  DWORD DesiredAccess,
	__in  BOOL OpenAsSelf,
	__out PHANDLE TokenHandle
	);

//
// Registry
//

LONG WINAPI 
RegOpenCurrentUserEnter(
	__in  REGSAM samDesired,
	__out PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGOPENCURRENTUSER)(
	__in  REGSAM samDesired,
	__out PHKEY phkResult
	);

LONG WINAPI 
RegConnectRegistryEnter(
	__in  LPCTSTR lpMachineName,
	__in  HKEY hKey,
	__out PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGCONNECTREGISTRY)(
	__in  LPCTSTR lpMachineName,
	__in  HKEY hKey,
	__out PHKEY phkResult
	);

LONG WINAPI 
RegOpenUserClassesRootEnter(
	__in HANDLE hToken,
	__in DWORD dwOptions,
	__in  REGSAM samDesired,
	__out PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGOPENUSERCLASSESROOT)(
	__in HANDLE hToken,
	__in DWORD dwOptions,
	__in  REGSAM samDesired,
	__out PHKEY phkResult
	);

LONG WINAPI 
RegCreateKeyAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	OUT PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGCREATEKEY_A)(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	OUT PHKEY phkResult
	);

LONG WINAPI 
RegCreateKeyWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	OUT PHKEY phkResult
	);

typedef LONG
(WINAPI *REGCREATEKEY_W)(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	OUT PHKEY phkResult
	);

LONG WINAPI 
RegCreateKeyExAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN DWORD Reserved,
	IN PSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	);

typedef LONG 
(WINAPI *REGCREATEKEYEX_A)(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN DWORD Reserved,
	IN PSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	);

LONG WINAPI 
RegCreateKeyExWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN DWORD Reserved,
	IN PWSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	);

typedef LONG 
(WINAPI *REGCREATEKEYEX_W)(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN DWORD Reserved,
	IN PWSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	);

LONG WINAPI 
RegOpenKeyExAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGOPENKEYEX_A)(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

LONG WINAPI 
RegOpenKeyExWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGOPENKEYEX_W)(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

LONG WINAPI 
RegCloseKeyEnter(
	__in HKEY hKey
	);

typedef LONG 
(WINAPI *REGCLOSEKEY)(
	__in HKEY hKey
	);

//
// CloseHandle
//

BOOL WINAPI
CloseHandleEnter(
	__in HANDLE hObject
	);

typedef BOOL 
(WINAPI *CLOSEHANDLE)(
	__in HANDLE hObject
	);
	
//
// DuplicateHandle
//

BOOL WINAPI 
DuplicateHandleEnter(
	__in  HANDLE hSourceProcessHandle,
	__in  HANDLE hSourceHandle,
	__in  HANDLE hTargetProcessHandle,
	__out LPHANDLE lpTargetHandle,
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwOptions
	);

typedef BOOL 
(WINAPI *DUPLICATEHANDLE)(
	__in  HANDLE hSourceProcessHandle,
	__in  HANDLE hSourceHandle,
	__in  HANDLE hTargetProcessHandle,
	__out LPHANDLE lpTargetHandle,
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwOptions
	);

//
// External Declaration
//

extern BTR_CALLBACK MmHandleCallback[];
extern ULONG MmHandleCallbackCount;

#ifdef __cplusplus
}
#endif
#endif