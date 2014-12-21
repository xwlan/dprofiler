//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "hal.h"
#include "mmprof.h"
#include "callback.h"
#include "heap.h"
#include "lock.h"
#include "trap.h"
#include "thread.h"
#include "cache.h"
#include "stacktrace.h"
#include "util.h"
#include "mmhandle.h"
#include <winsock2.h>

BTR_CALLBACK MmHandleCallback[] = {

	//
	// DuplicateHandle/CloseHandle
	//

	{ HandleCallbackType, 0,0,0,0, DuplicateHandleEnter, "kernel32.dll", "DuplicateHandle" ,0 },
	{ HandleCallbackType, 0,0,0,0, CloseHandleEnter, "kernel32.dll", "CloseHandle" ,0 },

	//
	// process
	//

	{ HandleCallbackType, 0,0,0,0, CreateProcessEnterA, "kernel32.dll", "CreateProcessA" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateProcessEnterW, "kernel32.dll", "CreateProcessW" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateProcessAsUserEnterA, "advapi32.dll", "CreateProcessAsUserA" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateProcessAsUserEnterW, "advapi32.dll", "CreateProcessAsUserW" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateProcessWithLogonWEnter, "advapi32.dll", "CreateProcessWithLogonW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenProcessEnter, "kernel32.dll", "OpenProcess" ,0 },

	//
	// thread
	//

	{ HandleCallbackType, 0,0,0,0, CreateThreadEnter, "kernel32.dll", "CreateThread" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateRemoteThreadEnter, "kernel32.dll", "CreateRemoteThread" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenThreadEnter, "kernel32.dll", "OpenThread" ,0 },

	//
	// job
	//

	{ HandleCallbackType, 0,0,0,0, CreateJobObjectEnter, "kernel32.dll", "CreateJobObjectW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenJobObjectEnter, "kernel32.dll", "OpenJobObjectW" ,0 },
	
	//
	// file
	//

	{ HandleCallbackType, 0,0,0,0, CreateFileWEnter, "kernel32.dll", "CreateFileW" ,0 },

	//
	// file mapping
	//

	{ HandleCallbackType, 0,0,0,0, CreateFileMappingWEnter, "kernel32.dll", "CreateFileMappingW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenFileMappingWEnter, "kernel32.dll", "OpenFileMappingW" ,0 },
	{ HandleCallbackType, 0,0,0,0, MapViewOfFileEnter, "kernel32.dll", "MapViewOfFile" ,0 },
	{ HandleCallbackType, 0,0,0,0, MapViewOfFileExEnter, "kernel32.dll", "MapViewOfFileEx" ,0 },
	{ HandleCallbackType, 0,0,0,0, UnmapViewOfFileEnter, "kernel32.dll", "UnmapViewOfFile" ,0 },

	//
	// registry
	//

	{ HandleCallbackType, 0,0,0,0, RegOpenCurrentUserEnter, "advapi32.dll", "RegOpenCurrentUser" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegConnectRegistryEnter, "advapi32.dll", "RegConnectRegistry" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegOpenUserClassesRootEnter, "advapi32.dll", "RegOpenUserClassesRoot" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegCreateKeyAEnter, "advapi32.dll", "RegCreateKeyA" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegCreateKeyWEnter, "advapi32.dll", "RegCreateKeyW" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegCreateKeyExAEnter, "advapi32.dll", "RegCreateKeyExA" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegCreateKeyExWEnter, "advapi32.dll", "RegCreateKeyExW" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegOpenKeyExAEnter, "advapi32.dll", "RegOpenKeyExA" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegOpenKeyExWEnter, "advapi32.dll", "RegOpenKeyExW" ,0 },
	{ HandleCallbackType, 0,0,0,0, RegCloseKeyEnter, "advapi32.dll", "RegCloseKey" ,0 },

	// 
	// socket
	//
	
	{ HandleCallbackType, 0,0,0,0, WSASocketEnter, "ws2_32.dll", "WSASocketW" ,0 },
	{ HandleCallbackType, 0,0,0,0, WSAAcceptEnter, "ws2_32.dll", "WSAAccept" ,0 },
	{ HandleCallbackType, 0,0,0,0, CloseSocketEnter, "ws2_32.dll", "closesocket" ,0 },

	//
	// Event
	//

	{ HandleCallbackType, 0,0,0,0, CreateEventWEnter, "kernel32.dll", "CreateEventW" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateEventExWEnter, "kernel32.dll", "CreateEventExW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenEventEnter, "kernel32.dll", "OpenEventW" ,0 },

	// 
	// Mutex
	//

	{ HandleCallbackType, 0,0,0,0, CreateMutexWEnter, "kernel32.dll", "CreateMutexW" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateMutexExWEnter, "kernel32.dll", "CreateMutexExW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenMutexEnter, "kernel32.dll", "OpenMutexW" ,0 },
	
	// 
	// Semaphore
	//

	{ HandleCallbackType, 0,0,0,0, CreateSemaphoreWEnter, "kernel32.dll", "CreateSemaphoreW" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateSemaphoreExWEnter, "kernel32.dll", "CreateSemaphoreExW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenSemaphoreEnter, "kernel32.dll", "OpenSemaphoreW" ,0 },

	// 
	// WaitableTimer
	//

	{ HandleCallbackType, 0,0,0,0, CreateWaitableTimerExEnter, "kernel32.dll", "CreateWaitableTimerExW" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenWaitableTimerEnter, "kernel32.dll", "OpenWaitableTimerW" ,0 },

	//
	// I/O Completion Port
	//

	{ HandleCallbackType, 0,0,0,0, CreateIoCompletionPortEnter, "kernel32.dll", "CreateIoCompletionPort" ,0 },

	//
	// Pipe
	//

	{ HandleCallbackType, 0,0,0,0, CreatePipeEnter, "kernel32.dll", "CreatePipe" ,0 },
	{ HandleCallbackType, 0,0,0,0, CreateNamedPipeEnter, "kernel32.dll", "CreateNamedPipeW" ,0 },

	//
	// Mailslot
	//
	
	{ HandleCallbackType, 0,0,0,0, CreateNamedPipeEnter, "kernel32.dll", "CreateMailslotW" ,0 },

	//
	// Token
	//

	{ HandleCallbackType, 0,0,0,0, CreateRestrictedTokenEnter, "advapi32.dll", "CreateRestrictedToken" ,0 },
	{ HandleCallbackType, 0,0,0,0, DuplicateTokenEnter, "advapi32.dll", "DuplicateToken" ,0 },
	{ HandleCallbackType, 0,0,0,0, DuplicateTokenExEnter, "advapi32.dll", "DuplicateTokenEx" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenProcessTokenEnter, "advapi32.dll", "OpenProcessToken" ,0 },
	{ HandleCallbackType, 0,0,0,0, OpenThreadTokenEnter, "advapi32.dll", "OpenThreadToken" ,0 },

};
	
ULONG MmHandleCallbackCount = ARRAYSIZE(MmHandleCallback);

PBTR_CALLBACK FORCEINLINE
MmGetHandleCallback(
	IN ULONG Ordinal
	)
{
	return &MmHandleCallback[Ordinal];
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPROCESS_A CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BOOL Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateProcessA);
	CallbackPtr = (CREATEPROCESS_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
			                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
							  lpStartupInfo, lpProcessInformation); 
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
		                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
						  lpStartupInfo, lpProcessInformation); 

	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(lpProcessInformation->hProcess, HANDLE_PROCESS, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessA);

	MmInsertHandleRecord(lpProcessInformation->hThread, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPROCESS_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateProcessW);
	CallbackPtr = (CREATEPROCESS_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
			                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
							  lpStartupInfo, lpProcessInformation); 
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
		                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
						  lpStartupInfo, lpProcessInformation); 

	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(lpProcessInformation->hProcess, HANDLE_PROCESS, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessW);

	MmInsertHandleRecord(lpProcessInformation->hThread, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPROCESSASUSER_A CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BOOL Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateProcessAsUserA);
	CallbackPtr = (CREATEPROCESSASUSER_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
			                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
							  lpStartupInfo, lpProcessInformation); 
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
		                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
						  lpStartupInfo, lpProcessInformation); 

	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(lpProcessInformation->hProcess, HANDLE_PROCESS, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessAsUserA);

	MmInsertHandleRecord(lpProcessInformation->hThread, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessAsUserA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPROCESSASUSER_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BOOL Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateProcessAsUserW);
	CallbackPtr = (CREATEPROCESSASUSER_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
			                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
							  lpStartupInfo, lpProcessInformation); 
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
		                  bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
						  lpStartupInfo, lpProcessInformation); 

	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(lpProcessInformation->hProcess, HANDLE_PROCESS, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessAsUserW);

	MmInsertHandleRecord(lpProcessInformation->hThread, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessAsUserW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPROCESSWITHLOGONW CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BOOL Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateProcessWithLogonW);
	CallbackPtr = (CREATEPROCESSWITHLOGONW)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(lpUsername, lpDomain, lpPassword, dwLogonFlags, 
			                  lpApplicationName, lpCommandLine, dwCreationFlags, 
			                  lpEnvironment, lpCurrentDirectory,
							  lpStartupInfo, lpProcessInfo); 
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(lpUsername, lpDomain, lpPassword, dwLogonFlags, 
		                  lpApplicationName, lpCommandLine, dwCreationFlags, 
		                  lpEnvironment, lpCurrentDirectory,
						  lpStartupInfo, lpProcessInfo); 

	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(lpProcessInfo->hProcess, HANDLE_PROCESS, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessWithLogonW);

	MmInsertHandleRecord(lpProcessInfo->hThread, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateProcessWithLogonW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

HANDLE WINAPI 
OpenProcessEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENPROCESS CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenProcess);
	CallbackPtr = (OPENPROCESS)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, dwProcessId);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, dwProcessId);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_PROCESS, Duration, Hash, 
		                  (USHORT)Depth, _OpenProcess);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateThreadEnter(
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATETHREAD CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateThread);
	CallbackPtr = (CREATETHREAD)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpThreadAttributes, dwStackSize, lpStartAddress,
			                  lpParameter, dwCreationFlags, lpThreadId);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpThreadAttributes, dwStackSize, lpStartAddress,
	  	                    lpParameter, dwCreationFlags, lpThreadId);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateThread);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateRemoteThreadEnter(
	__in  HANDLE hProcess,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEREMOTETHREAD CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateRemoteThread);
	CallbackPtr = (CREATEREMOTETHREAD)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hProcess, lpThreadAttributes, dwStackSize, lpStartAddress,
			                  lpParameter, dwCreationFlags, lpThreadId);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(hProcess, lpThreadAttributes, dwStackSize, lpStartAddress,
	  	                    lpParameter, dwCreationFlags, lpThreadId);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _CreateRemoteThread);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
OpenThreadEnter(
	__in DWORD dwDesiredAccess,
	__in BOOL bInheritHandle,
	__in DWORD dwThreadId
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENTHREAD CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenThread);
	CallbackPtr = (OPENTHREAD)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, dwThreadId);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, dwThreadId);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_THREAD, Duration, Hash, 
		                  (USHORT)Depth, _OpenThread);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateJobObjectEnter(
	__in  LPSECURITY_ATTRIBUTES lpJobAttributes,
	__in  PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEJOBOBJECT_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateJobObjectW);
	CallbackPtr = (CREATEJOBOBJECT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpJobAttributes, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpJobAttributes, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_JOB, Duration, Hash, 
		                  (USHORT)Depth, _CreateJobObjectW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
OpenJobObjectEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENJOBOBJECT_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenJobObjectW);
	CallbackPtr = (OPENJOBOBJECT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_JOB, Duration, Hash, 
		                  (USHORT)Depth, _OpenJobObjectW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI
CreateFileWEnter(
	IN PWSTR lpFileName,
	IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFILE_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateFileW);
	CallbackPtr = (CREATEFILE_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, 
			                  dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile); 
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, 
		                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile); 
	if (Handle == INVALID_HANDLE_VALUE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_FILE, Duration, Hash, 
		                  (USHORT)Depth, _CreateFileW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}


HANDLE WINAPI 
FindFirstVolumeEnter(
	OUT PWSTR lpszVolumeName,
	IN  DWORD cchBufferLength
	)
{
	return 0;
}

BOOL WINAPI 
FindVolumeCloseEnter(
	IN HANDLE hFindVolume
	)
{
	return 0;
}

HANDLE WINAPI 
FindFirstVolumeMountPointEnter(
	__in  PWSTR lpszRootPathName,
	__out PWSTR lpszVolumeMountPoint,
	__in  DWORD cchBufferLength
	)
{
	return 0;
}

BOOL WINAPI 
FindVolumeMountPointCloseEnter(
	__in  HANDLE hFindVolumeMountPoint
	)
{
	return 0;
}

HANDLE WINAPI 
FindFirstChangeNotificationEnter(
	__in  PWSTR lpPathName,
	__in  BOOL bWatchSubtree,
	__in  DWORD dwNotifyFilter
	)
{
	return 0;
}

BOOL WINAPI 
FindCloseChangeNotificationEnter(
	__in  HANDLE hChangeHandle
	)
{
	return 0;
}

HANDLE WINAPI 
CreateFileMappingWEnter(
	IN HANDLE hFile,
	IN LPSECURITY_ATTRIBUTES lpAttributes,
	IN DWORD flProtect,
	IN DWORD dwMaximumSizeHigh,
	IN DWORD dwMaximumSizeLow,
	IN PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFILEMAPPING_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateFileMappingW);
	CallbackPtr = (CREATEFILEMAPPING_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hFile, lpAttributes, flProtect, dwMaximumSizeHigh, 
			                  dwMaximumSizeLow, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(hFile, lpAttributes, flProtect, dwMaximumSizeHigh, 
		                    dwMaximumSizeLow, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_FILEMAPPING, Duration, Hash, 
		                  (USHORT)Depth, _CreateFileMappingW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

LPVOID WINAPI 
MapViewOfFileEnter(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	MAPVIEWOFFILE CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_MapViewOfFile);
	CallbackPtr = (MAPVIEWOFFILE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, 
			                  dwNumberOfBytesToMap);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, 
		                     dwNumberOfBytesToMap);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	//
	// N.B. For file mapping object, a clean close must close all views,
	// we treat mapped address as handle, UnmapViewOfFile as CloseHandle
	//

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Address, HANDLE_FILEMAPPING, Duration, Hash, 
		                  (USHORT)Depth, _MapViewOfFile);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

LPVOID WINAPI 
MapViewOfFileExEnter(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap,
	IN LPVOID lpBaseAddress
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	MAPVIEWOFFILEEX CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	PVOID Address;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_MapViewOfFileEx);
	CallbackPtr = (MAPVIEWOFFILEEX)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Address = (*CallbackPtr)(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, 
			                  dwNumberOfBytesToMap, lpBaseAddress);
		InterlockedDecrement(&Callback->References);
		return Address;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Address = (*CallbackPtr)(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, 
		                     dwNumberOfBytesToMap, lpBaseAddress);
	if (!Address) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Address;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	//
	// N.B. For file mapping object, a clean close must close all views,
	// we treat mapped address as handle, UnmapViewOfFile as CloseHandle
	//

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Address, HANDLE_FILEMAPPING, Duration, Hash, 
		                  (USHORT)Depth, _MapViewOfFileEx);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Address;
}

BOOL WINAPI 
UnmapViewOfFileEnter(
	IN LPCVOID lpBaseAddress
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	UNMAPVIEWOFFILE CallbackPtr;
	PVOID Caller;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_UnmapViewOfFile);
	CallbackPtr = (UNMAPVIEWOFFILE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(lpBaseAddress);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(lpBaseAddress);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (lpBaseAddress) {
		MmRemoveHandleRecord((HANDLE)lpBaseAddress);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

HANDLE WINAPI 
OpenFileMappingWEnter(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENFILEMAPPING_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenFileMappingW);
	CallbackPtr = (OPENFILEMAPPING_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName); 
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName); 
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_FILEMAPPING, Duration, Hash, 
		                  (USHORT)Depth, _OpenFileMappingW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

SOCKET WINAPI
WSASocketEnter(
	IN int af,
	IN int type,
	IN int protocol,
	IN LPWSAPROTOCOL_INFO lpProtocolInfo,
	IN GROUP g,
	IN DWORD dwFlags
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	WSASOCKET CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	SOCKET sk;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_WSASocketW);
	CallbackPtr = (WSASOCKET)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		sk = (*CallbackPtr)(af, type, protocol, lpProtocolInfo, g, dwFlags);
		InterlockedDecrement(&Callback->References);
		return sk;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	sk = (*CallbackPtr)(af, type, protocol, lpProtocolInfo, g, dwFlags);
	if (sk == INVALID_SOCKET) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return sk;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord((HANDLE)sk, HANDLE_SOCKET, Duration, Hash, 
		                  (USHORT)Depth, _WSASocketW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return sk;
}

SOCKET WINAPI
WSAAcceptEnter(
	IN SOCKET s,
	OUT struct sockaddr* addr,
	OUT int* addrlen,
	IN LPCONDITIONPROC lpfnCondition,
	IN DWORD dwCallbackData
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	WSAACCEPT CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	SOCKET sk;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_WSAAccept);
	CallbackPtr = (WSAACCEPT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		sk = (*CallbackPtr)(s, addr, addrlen, lpfnCondition, dwCallbackData);
		InterlockedDecrement(&Callback->References);
		return sk;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	sk = (*CallbackPtr)(s, addr, addrlen, lpfnCondition, dwCallbackData);
	if (sk == INVALID_SOCKET) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return sk;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord((HANDLE)sk, HANDLE_SOCKET, Duration, Hash, 
		                  (USHORT)Depth, _WSAAccept);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return sk;
}

int WINAPI
CloseSocketEnter(
	IN SOCKET sk
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	CLOSESOCKET CallbackPtr;
	PVOID Caller;
	int Status;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_closesocket);
	CallbackPtr = (CLOSESOCKET)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(sk);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(sk);
	if (Status == SOCKET_ERROR) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (sk) {
		MmRemoveHandleRecord((HANDLE)sk);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}
 
HANDLE WINAPI 
CreateEventWEnter(
	__in LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in BOOL bManualReset,
	__in BOOL bInitialState,
	__in PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEEVENT_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateEventW);
	CallbackPtr = (CREATEEVENT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpEventAttributes, bManualReset, 
			                  bInitialState, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpEventAttributes, bManualReset, 
		                    bInitialState, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_EVENT, Duration, Hash, 
		                  (USHORT)Depth, _CreateEventW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
	
}

HANDLE WINAPI 
CreateEventExWEnter(
	__in  LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEEVENTEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateEventExW);
	CallbackPtr = (CREATEEVENTEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpEventAttributes, lpName, 
			                  dwFlags, dwDesiredAccess);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpEventAttributes, lpName, 
		                    dwFlags, dwDesiredAccess);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_EVENT, Duration, Hash, 
		                  (USHORT)Depth, _CreateEventExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}


HANDLE WINAPI
OpenEventEnter( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	) 
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENEVENT_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenEventW);
	CallbackPtr = (OPENEVENT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_EVENT, Duration, Hash, 
		                  (USHORT)Depth, _OpenEventW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateMutexWEnter(
	__in  LPSECURITY_ATTRIBUTES lpMutexAttributes,
	__in  BOOL bInitialOwner,
	__in  PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEMUTEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateMutexW);
	CallbackPtr = (CREATEMUTEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpMutexAttributes, bInitialOwner, lpName); 
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpMutexAttributes, bInitialOwner, lpName); 
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_MUTEX, Duration, Hash, 
		                  (USHORT)Depth, _CreateMutexW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateMutexExWEnter(
	__in  LPSECURITY_ATTRIBUTES lpMutexAttributes,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEMUTEXEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateMutexExW);
	CallbackPtr = (CREATEMUTEXEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_MUTEX, Duration, Hash, 
		                  (USHORT)Depth, _CreateMutexExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI
OpenMutexEnter( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	) 
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENMUTEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenMutexW);
	CallbackPtr = (OPENMUTEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_MUTEX, Duration, Hash, 
		                  (USHORT)Depth, _OpenMutexW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateSemaphoreWEnter(
	__in LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in LONG lInitialCount,
	__in LONG lMaximumCount,
	__in PWSTR lpName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATESEMAPHORE_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateSemaphoreW);
	CallbackPtr = (CREATESEMAPHORE_W )BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName); 
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName); 
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_SEMAPHORE, Duration, Hash, 
		                  (USHORT)Depth, _CreateSemaphoreW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateSemaphoreExWEnter(
	__in  LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in  LONG lInitialCount,
	__in  LONG lMaximumCount,
	__in  PWSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATESEMAPHOREEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateSemaphoreExW);
	CallbackPtr = (CREATESEMAPHOREEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpSemaphoreAttributes, lInitialCount, lMaximumCount, 
			                  lpName, dwFlags, dwDesiredAccess); 
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpSemaphoreAttributes, lInitialCount, lMaximumCount, 
		                    lpName, dwFlags, dwDesiredAccess); 
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_SEMAPHORE, Duration, Hash, 
		                  (USHORT)Depth, _CreateSemaphoreExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI
OpenSemaphoreEnter( 
	__in DWORD dwDesiredAccess, 
	__in BOOL bInheritHandle, 
	__in PWSTR lpName 
	) 
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENSEMAPHORE_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenSemaphoreW);
	CallbackPtr = (OPENSEMAPHORE_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_SEMAPHORE, Duration, Hash, 
		                  (USHORT)Depth, _OpenSemaphoreW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateWaitableTimerExEnter(
	__in  LPSECURITY_ATTRIBUTES lpTimerAttributes,
	__in  PWSTR lpTimerName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEWAITABLETIMEREX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateWaitableTimerExW);
	CallbackPtr = (CREATEWAITABLETIMEREX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpTimerAttributes, lpTimerName, dwFlags, dwDesiredAccess);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpTimerAttributes, lpTimerName, dwFlags, dwDesiredAccess);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_TIMER, Duration, Hash, 
		                  (USHORT)Depth, _CreateWaitableTimerExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
OpenWaitableTimerEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  PWSTR lpTimerName
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENWAITABLETIMER_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenWaitableTimerW);
	CallbackPtr = (OPENWAITABLETIMER_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpTimerName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(dwDesiredAccess, bInheritHandle, lpTimerName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_TIMER, Duration, Hash, 
		                  (USHORT)Depth, _OpenWaitableTimerW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateIoCompletionPortEnter(
	__in HANDLE FileHandle,
	__in HANDLE ExistingCompletionPort,
	__in ULONG_PTR CompletionKey,
	__in DWORD NumberOfConcurrentThreads
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEIOCOMPLETIONPORT CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateIoCompletionPort);
	CallbackPtr = (CREATEIOCOMPLETIONPORT )BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(FileHandle, ExistingCompletionPort,
							  CompletionKey, NumberOfConcurrentThreads);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(FileHandle, ExistingCompletionPort,
						    CompletionKey, NumberOfConcurrentThreads);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_IOCP, Duration, Hash, 
		                  (USHORT)Depth, _CreateIoCompletionPort);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

BOOL WINAPI 
CreatePipeEnter(
	__out PHANDLE hReadPipe,
	__out PHANDLE hWritePipe,
	__in  LPSECURITY_ATTRIBUTES lpPipeAttributes,
	__in  DWORD nSize
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPIPE CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BOOL Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreatePipe);
	CallbackPtr = (CREATEPIPE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hReadPipe, hWritePipe, lpPipeAttributes, nSize);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hReadPipe, hWritePipe, lpPipeAttributes, nSize);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*hReadPipe, HANDLE_PIPE, Duration, Hash, 
		                  (USHORT)Depth, _CreatePipe);

	MmInsertHandleRecord(*hWritePipe, HANDLE_PIPE, Duration, Hash, 
		                  (USHORT)Depth, _CreatePipe);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATENAMEDPIPE_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateNamedPipeW);
	CallbackPtr = (CREATENAMEDPIPE_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpName, dwOpenMode, dwPipeMode, nMaxInstances,
			                  nOutBufferSize, nInBufferSize, nDefaultTimeOut,
							  lpSecurityAttributes);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpName, dwOpenMode, dwPipeMode, nMaxInstances,
		                    nOutBufferSize, nInBufferSize, nDefaultTimeOut,
						    lpSecurityAttributes);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_PIPE, Duration, Hash, 
		                  (USHORT)Depth, _CreateNamedPipeW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
CreateMailslotEnter(
	__in PWSTR lpName,
	__in DWORD nMaxMessageSize,
	__in DWORD lReadTimeout,
	__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEMAILSLOT_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateMailslotW);
	CallbackPtr = (CREATEMAILSLOT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpName, nMaxMessageSize, lReadTimeout,
							  lpSecurityAttributes);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(lpName, nMaxMessageSize, lReadTimeout,
							lpSecurityAttributes);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(Handle, HANDLE_MAILSLOT, Duration, Hash, 
		                  (USHORT)Depth, _CreateMailslotW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATERESTRICTEDTOKEN CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CreateRestrictedToken);
	CallbackPtr = (CREATERESTRICTEDTOKEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(ExistingTokenHandle, Flags, DisableSidCount,
			                  SidsToDisable, DeletePrivilegeCount, PrivilegesToDelete,
							  RestrictedSidCount, SidsToRestrict, NewTokenHandle);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(ExistingTokenHandle, Flags, DisableSidCount,
		                    SidsToDisable, DeletePrivilegeCount, PrivilegesToDelete,
						    RestrictedSidCount, SidsToRestrict, NewTokenHandle);
	if (Status != TRUE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*NewTokenHandle, HANDLE_TOKEN, Duration, Hash, 
		                  (USHORT)Depth, _CreateRestrictedToken);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}

BOOL WINAPI 
DuplicateTokenEnter(
	__in  HANDLE ExistingTokenHandle,
	__in  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	__out PHANDLE DuplicateTokenHandle
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	DUPLICATETOKEN CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_DuplicateToken);
	CallbackPtr = (DUPLICATETOKEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(ExistingTokenHandle, ImpersonationLevel, DuplicateTokenHandle);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(ExistingTokenHandle, ImpersonationLevel, DuplicateTokenHandle);
	if (Status != TRUE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*DuplicateTokenHandle, HANDLE_TOKEN, Duration, Hash, 
		                  (USHORT)Depth, _DuplicateToken);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

BOOL WINAPI 
DuplicateTokenExEnter(
	__in  HANDLE hExistingToken,
	__in  DWORD dwDesiredAccess,
	__in_opt  LPSECURITY_ATTRIBUTES lpTokenAttributes,
	__in  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	__in  TOKEN_TYPE TokenType,
	__out PHANDLE phNewToken
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	DUPLICATETOKENEX CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_DuplicateTokenEx);
	CallbackPtr = (DUPLICATETOKENEX)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hExistingToken, dwDesiredAccess, lpTokenAttributes,
			                  ImpersonationLevel, TokenType, phNewToken);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hExistingToken, dwDesiredAccess, lpTokenAttributes,
		                    ImpersonationLevel, TokenType, phNewToken);
	if (Status != TRUE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phNewToken, HANDLE_TOKEN, Duration, Hash, 
		                  (USHORT)Depth, _DuplicateTokenEx);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}

BOOL WINAPI 
OpenProcessTokenEnter(
	__in  HANDLE ProcessHandle,
	__in  DWORD DesiredAccess,
	__out PHANDLE TokenHandle
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENPROCESSTOKEN CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenProcessToken);
	CallbackPtr = (OPENPROCESSTOKEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(ProcessHandle, DesiredAccess, TokenHandle);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(ProcessHandle, DesiredAccess, TokenHandle);
	if (Status != TRUE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*TokenHandle, HANDLE_TOKEN, Duration, Hash, 
		                  (USHORT)Depth, _OpenProcessToken);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}

BOOL WINAPI 
OpenThreadTokenEnter(
	__in  HANDLE ThreadHandle,
	__in  DWORD DesiredAccess,
	__in  BOOL OpenAsSelf,
	__out PHANDLE TokenHandle
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	OPENTHREADTOKEN CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_OpenThreadToken);
	CallbackPtr = (OPENTHREADTOKEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
	if (Status != TRUE) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*TokenHandle, HANDLE_TOKEN, Duration, Hash, 
		                  (USHORT)Depth, _OpenThreadToken);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}

LONG WINAPI 
RegOpenCurrentUserEnter(
	__in  REGSAM samDesired,
	__out PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGOPENCURRENTUSER CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegOpenCurrentUser);
	CallbackPtr = (REGOPENCURRENTUSER)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(samDesired, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(samDesired, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegOpenCurrentUser);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegConnectRegistryEnter(
	__in  LPCTSTR lpMachineName,
	__in  HKEY hKey,
	__out PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGCONNECTREGISTRY CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegConnectRegistry);
	CallbackPtr = (REGCONNECTREGISTRY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(lpMachineName, hKey, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(lpMachineName, hKey, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegConnectRegistry);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegOpenUserClassesRootEnter(
	__in HANDLE hToken,
	__in DWORD dwOptions,
	__in  REGSAM samDesired,
	__out PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGOPENUSERCLASSESROOT CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegOpenUserClassesRoot);
	CallbackPtr = (REGOPENUSERCLASSESROOT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hToken, dwOptions, samDesired, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hToken, dwOptions, samDesired, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegOpenUserClassesRoot);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegCreateKeyAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	OUT PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGCREATEKEY_A CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegCreateKeyA);
	CallbackPtr = (REGCREATEKEY_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hKey, lpSubKey, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hKey, lpSubKey, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegCreateKeyA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegCreateKeyWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	OUT PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGCREATEKEY_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegCreateKeyW);
	CallbackPtr = (REGCREATEKEY_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hKey, lpSubKey, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hKey, lpSubKey, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegCreateKeyW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGCREATEKEYEX_A CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegCreateKeyExA);
	CallbackPtr = (REGCREATEKEYEX_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hKey, lpSubKey, Reserved, lpClass, dwOptions,
			                  samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hKey, lpSubKey, Reserved, lpClass, dwOptions,
			                samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegCreateKeyExA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGCREATEKEYEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegCreateKeyExW);
	CallbackPtr = (REGCREATEKEYEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hKey, lpSubKey, Reserved, lpClass, dwOptions,
			                  samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hKey, lpSubKey, Reserved, lpClass, dwOptions,
			                samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegCreateKeyExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegOpenKeyExAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGOPENKEYEX_A CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegOpenKeyExA);
	CallbackPtr = (REGOPENKEYEX_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegOpenKeyExA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegOpenKeyExWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	REGOPENKEYEX_W CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	LONG Status;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegOpenKeyExW);
	CallbackPtr = (REGOPENKEYEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertHandleRecord(*phkResult, HANDLE_KEY, Duration, Hash, 
		                  (USHORT)Depth, _RegOpenKeyExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

LONG WINAPI 
RegCloseKeyEnter(
	__in HKEY hObject
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	REGCLOSEKEY CallbackPtr;
	PVOID Caller;
	LONG Status;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_RegCloseKey);
	CallbackPtr = (REGCLOSEKEY)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hObject);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hObject);
	if (Status != ERROR_SUCCESS) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	if (hObject) {
		MmRemoveHandleRecord(hObject);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return ERROR_SUCCESS;
}

BOOL WINAPI
CloseHandleEnter(
	__in HANDLE hObject
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	CLOSEHANDLE CallbackPtr;
	PVOID Caller;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_CloseHandle);
	CallbackPtr = (CLOSEHANDLE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hObject);
		InterlockedDecrement(&Callback->References);
		return Status;
	}
	
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hObject);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return FALSE;
	}

	if (hObject) {
		MmRemoveHandleRecord(hObject);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}
	
BOOL WINAPI 
DuplicateHandleEnter(
	__in  HANDLE hSourceProcessHandle,
	__in  HANDLE hSourceHandle,
	__in  HANDLE hTargetProcessHandle,
	__out LPHANDLE lpTargetHandle,
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwOptions
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	DUPLICATEHANDLE CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	PVOID Callers[MAX_STACK_DEPTH];
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	BOOL Status;
	BOOL FromCurrent;
	BOOL ToCurrent;
	BOOL Close;
	ULONG CurrentId;
	ULONG FromId;
	ULONG ToId;
	ULONG Duration;
	PPF_HANDLE_RECORD Record;
	HANDLE_TYPE Type;

	Caller = _ReturnAddress();
	Callback = MmGetHandleCallback(_DuplicateHandle);
	CallbackPtr = (DUPLICATEHANDLE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle,
							  lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions);
		InterlockedDecrement(&Callback->References);
		return Status;
	}
	
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	ToId = GetProcessId(hTargetProcessHandle);
	FromId = GetProcessId(hSourceProcessHandle);
	CurrentId = BtrCurrentProcessId();

	if (ToId != CurrentId) {
		ToCurrent = FALSE;
	} else {
		ToCurrent = TRUE;
	}

	if (FromId != CurrentId) {
		FromCurrent = FALSE;
	} else {
		FromCurrent = TRUE;
	}
	
	if (FlagOn(dwOptions, DUPLICATE_CLOSE_SOURCE)) {
		Close = TRUE;
	} else {
		Close = FALSE;
	}

	QueryPerformanceCounter(&Enter);

	Status = (*CallbackPtr)(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle,
						    lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Thread, Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);

	//
	// N.B. Principle is to track what we can track in current process,
	// if target is another process, or source is another process when 
	// DUPLICATE_CLOSE_SOURCE is on, we do nothing here.
	//

	if (ToCurrent) {

		//
		// Duplicate from current process, let's check whether source handle's
		// type is known
		//

		if (FromCurrent) {

			Record = MmLookupHandleRecord(hSourceHandle);
			if (Record != NULL) {
				Type = Record->Type;
			} 
			else {
				Type = HANDLE_GENERIC;
			}
		} 
		
		//
		// From another process, we don't know its type
		//

		else {
			Type = HANDLE_GENERIC;
		}

		MmInsertHandleRecord(*lpTargetHandle, Type, Duration, Hash,
			                  (USHORT)Depth, _DuplicateHandle);
	}

	//
	// If DUPLICATE_CLOSE_SOURCE is on and source handle is 
	// from current process, close it if it's available
	//

	if (Close && FromCurrent) {
		MmRemoveHandleRecord(hSourceHandle);
	}

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return TRUE;
}