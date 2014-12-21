//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "bsp.h"
#include "injectldr.h" 
#include <strsafe.h>
#include <tlhelp32.h>
#include "debugger.h"

CHAR BspDebuggerCode[] = {
	0x55,					//	push ebp 
	0x8b, 0xec,				//	mov  ebp, esp
	0x8b, 0xd8,             //  mov  ebx, eax
	
	0x8d, 0x43, 0x10,		//	lea  eax, dword ptr[ebx + BSP_LDR_DBG.ThreadId]
	0x50,					//	push eax
	0x6a, 0x00,				//	push 0
	0xff, 0x33,				//	push dword ptr[ebx + BSP_LDR_DBG.DllFullPath] 
	0xff, 0x73, 0x04,		//	push dword ptr[ebx + BSP_LDR_DBG.LoadLibraryW]
	0x6a, 0x00,				//	push 0
	0x6a, 0x00, 			//	push 0
	0x6a, 0x00,				//	push 0
	0x6a, 0x00,				//	push 0
	0x6a, 0x00,				//	push 0
	0x6a, 0xff, 			//	push -1
	0xff, 0x53, 0x08,		//	call dword ptr[RtlCreateUserThread]
	0x89, 0x43, 0x14,       //  mov dword ptr[NtStatus], eax
	0x6a, 0x00, 			//	push 0
	0xff, 0x53, 0x0c,       //  call dword ptr[RtlExitUserThread]
	0xcc					//	int 3

};

CHAR BspPreExecuteCode[] = {
					
	0x8d, 0x4b, 0x44,			// lea  ecx, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Path] 
	0x51,						// push ecx
	0xff, 0x53, 0x34,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.LoadLibraryAddr]
					
	0x83, 0xf8, 0x00,           // cmp eax, 0
	0x74, 0x40,                 // jz Failed

	0xff, 0x73, 0x2c,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SuccessEvent]
	0xff, 0x53, 0x38,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SetEventAddr]
					
	0x6a, 0xff,					// push -1
	0xff, 0x73, 0x28,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CompleteEvent]
	0xff, 0x53, 0x3c,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.WaitForSingleObjectAddr]
					
	0xff, 0x73, 0x2c,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SuccessEvent]
	0xff, 0x53, 0x40,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]

	0xff, 0x73, 0x30,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.ErrorEvent]
	0xff, 0x53, 0x40,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]

	0xff, 0x73, 0x28,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CompleteEvent]
	0xff, 0x53, 0x40,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]

	0x8b, 0x43, 0x20,			// mov eax, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xax]
	0x8b, 0x4b, 0x1c,			// mov ecx, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xcx]
	0x8b, 0x53, 0x18,			// mov edx, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xdx]
	0x8b, 0x73, 0x08,			// mov esi, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xsi]
	0x8b, 0x7b, 0x04,			// mov edi, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xdi]
	0x8b, 0x6b, 0x0c,			// mov ebp, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xbp]
	0x8b, 0x63, 0x10,			// mov esp, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xsp]
					
	0xff, 0x33,					// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.XFlags]
	0x9d,						// popfd
					
	0xff, 0x73, 0x24,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xip]
	0xff, 0x73, 0x14,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xbx]
	0x5b,						// pop  ebx
	0xc3,						// ret

	0xff, 0x73, 0x30,			// push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.ErrorEvent]
	0xff, 0x53, 0x38,			// call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SetEventAddr]	
	0xeb, 0xbe					// jmp  Complete
};

typedef NTSTATUS 
(NTAPI *NT_DELAY_EXECUTION)(
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER DelayInterval
	);

typedef struct _BSP_APC_DATA {
	LARGE_INTEGER DelayInterval;
	PVOID NtDelayExecution;
	WCHAR Path[512];
	CHAR  Code[512];
} BSP_APC_DATA, *PBSP_APC_DATA;

CHAR BspApcCode[] = {
	0x50,					//	push eax
	0x52,					//	push edx
	0x8b, 0x54, 0x24, 0x08, //	mov  edx, dword ptr[esp + 8]	
	0x52,					//	push edx
	0x6a, 0x01,				//	push 1
	0xff, 0x52, 0x08,		//	call dword ptr[edx + 8]
	0x5a,					//	pop  edx
	0x58,					//	pop  eax
	0x83, 0xc4, 0x04,		//	add  esp, 4
	0xc3					//	ret
};

ULONG
BspLoadLibraryUnsafe(
	IN ULONG ProcessId,
	PWSTR DllFullPath
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	LIST_ENTRY ThreadList;
	LIST_ENTRY CallList;
	PVOID LoadLibraryPtr;
	PVOID SuspendThreadPtr;
	PBSP_LDR_THREAD Thread;
	PLIST_ENTRY ListEntry;
	PVOID RemotePath;
	PVOID SetEventPtr;
	HANDLE CompleteEvent;
	BOOLEAN ErrorOccured = FALSE;
	BOOLEAN Executed = FALSE;

	InitializeListHead(&ThreadList);
	InitializeListHead(&CallList);

	//
	// Scan hijackable thread and get target process handle
	//

	Status = BspScanHijackableThread(ProcessId, &ProcessHandle, &ThreadList);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	//BspQueueUserApc(ProcessHandle, DllFullPath, &ThreadList);
	//Status = BspExecuteLoadRoutine(ProcessHandle, DllFullPath, &ThreadList);

	LoadLibraryPtr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
	SuspendThreadPtr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "SuspendThread");
	SetEventPtr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "SetEvent");

	if (!LoadLibraryPtr || !SuspendThreadPtr || !SetEventPtr) {
		ErrorOccured = TRUE;
	}

	if (ErrorOccured != TRUE) {
		
		//
		// N.B. This is dangerous, we're going to write to target PEB
		//

		RemotePath = VirtualAllocEx(ProcessHandle, NULL, BspPageSize, MEM_COMMIT, PAGE_READWRITE);
		if (!RemotePath) {
			ErrorOccured = TRUE;

		} else {
		
			Status = WriteProcessMemory(ProcessHandle, RemotePath, DllFullPath, 
				                        (wcslen(DllFullPath) + 1) * 2, NULL);
			if (!Status) {
				ErrorOccured = TRUE;
			}
		}

		if (!ErrorOccured) {

			//
			// Write remote path into target process PEB's (PAGESIZE - sizeof(ULONG_PTR) * 2)
			// why write to this position ? ;)
			//

			ULONG_PTR Peb;
            Peb = (ULONG_PTR)BspQueryPebAddress(ProcessHandle);
			if (Peb != 0) {
				Peb = Peb + BspPageSize - sizeof(ULONG_PTR) * 2;
				WriteProcessMemory(ProcessHandle, Peb, &RemotePath, sizeof(ULONG_PTR), NULL);
			}
		}
	}

	//
	// Create complete notification event
	//

	if (ErrorOccured != TRUE) {
		CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!CompleteEvent) { 
			ErrorOccured = TRUE;
		}
	}

	while (IsListEmpty(&ThreadList) != TRUE) {
	
		ListEntry = RemoveHeadList(&ThreadList);
		Thread = CONTAINING_RECORD(ListEntry, BSP_LDR_THREAD, ListEntry);

		if (ErrorOccured) {
			CloseHandle(Thread->ThreadHandle);
			BspFree(Thread);
			continue;
		}

		Status = BspExecuteRemoteCall(ProcessHandle, LoadLibraryPtr, 1, 
							          &RemotePath, Thread, SuspendThreadPtr,
							          CompleteEvent, SetEventPtr );

		if (Status != S_OK) {
			CloseHandle(Thread->ThreadHandle);
			BspFree(Thread);
			continue;
		}

		ResumeThread(Thread->ThreadHandle);

		Status = WaitForSingleObject(CompleteEvent, INFINITE);
		if (Status != WAIT_OBJECT_0) {
			Status = ERROR_TIMEOUT;
		} else {
			Status = S_OK;
		}	
		
		if (Status != ERROR_TIMEOUT) {

			ULONG Count;
			Count = SuspendThread(Thread->ThreadHandle);

			SetThreadContext(Thread->ThreadHandle, &Thread->Context); 
			ResumeThread(Thread->ThreadHandle);

			if (Count > 1) {
				ResumeThread(Thread->ThreadHandle);
			}

			CloseHandle(Thread->ThreadHandle);
			BspFree(Thread);

			Executed = TRUE;
			break;

		} else {
			

		}

		//
		// Insert tail of thread list, we can rety again
		//

		InsertTailList(&ThreadList, &Thread->ListEntry);

	}

	if (IsListEmpty(&CallList)) {
		(*NtResumeProcess)(ProcessHandle);
		CloseHandle(ProcessHandle);
		CloseHandle(CompleteEvent);
		return S_FALSE;
	}

	//
	// Resume process execution and wait remote call completed
	//

	(*NtResumeProcess)(ProcessHandle);

	Status = WaitForSingleObject(CompleteEvent, INFINITE);
	if (Status != WAIT_OBJECT_0) {
		Status = GetLastError();
	} else {
		Status = S_OK;
	}	

	//
	// Clean up pending remote calls
	//

	while (IsListEmpty(&CallList) != TRUE) {
		
		CONTEXT Context;
		ULONG SuspendCount;

		Context.ContextFlags = CONTEXT_ALL;
		ListEntry = RemoveHeadList(&CallList);
		Thread = CONTAINING_RECORD(ListEntry, BSP_LDR_THREAD, ListEntry);

		SuspendCount = SuspendThread(Thread->ThreadHandle);
		GetThreadContext(Thread->ThreadHandle, &Context);
		
#if defined (_M_IX86)

		if (Context.Eip == (ULONG_PTR)Thread->AdjustedEip && 
			Context.Esp == (ULONG_PTR)Context.Esp) {

#elif defined (_M_X64)
		if (Context.Rip == (ULONG_PTR)Thread->AdjustedEip && 
			Context.Rsp == (ULONG_PTR)Context.Rsp) {
#endif

			//
			// It's not yet get a chance to execute, recover to its old context
			//

			SetThreadContext(Thread->ThreadHandle, &Thread->Context);
			ResumeThread(Thread->ThreadHandle);

		} else {
		
			if (SuspendCount == 2) {
				SetThreadContext(Thread->ThreadHandle, &Thread->Context);
				ResumeThread(Thread->ThreadHandle);
				ResumeThread(Thread->ThreadHandle);
				InsertTailList(&CallList, &Thread->ListEntry);
			}
			else if (SuspendCount == 1) {
				ResumeThread(Thread->ThreadHandle);
				InsertTailList(&CallList, &Thread->ListEntry);
			}
		}
	}

	//
	// N.B. We don't close the complete event handle, yes, leak it.
	//

	CloseHandle(ProcessHandle);
	return Status;
}
	
ULONG 
BspQueueUserApc(
	IN HANDLE ProcessHandle,
	IN PWSTR DllFullPath,
	IN PLIST_ENTRY ThreadList
	)
{
	ULONG Status;
	HMODULE DllHandle;
	PVOID NtDelayExecution;
	PVOID LoadLibraryPtr;
	PBSP_APC_DATA Source;
	PBSP_APC_DATA Destine;
	PLIST_ENTRY ListEntry;
	PBSP_LDR_THREAD Thread;
	SIZE_T Length;
	PCHAR Peb;

	//
	// Build local copy of BSP_APC_DATA
	//

	DllHandle = GetModuleHandle(L"ntdll.dll");
	NtDelayExecution = (PVOID)GetProcAddress(DllHandle, "NtDelayExecution");

	DllHandle = GetModuleHandle(L"kernel32.dll");
	LoadLibraryPtr = (PVOID)GetProcAddress(DllHandle, "LoadLibraryW");

	if (!NtDelayExecution || !LoadLibraryPtr) {
		return S_FALSE;
	}

	Source = (PBSP_APC_DATA)BspMalloc(4096);
	Source->DelayInterval.QuadPart = 0;
	Source->NtDelayExecution = NtDelayExecution;

	StringCchCopy(Source->Path, MAX_PATH, DllFullPath);
	RtlCopyMemory(Source->Code, BspApcCode, sizeof(BspApcCode));

	//
	// Write BSP_APC_DATA to destine process address space
	//

	Destine = (PBSP_APC_DATA)VirtualAllocEx(ProcessHandle, NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!Destine) {
		Status = GetLastError();
		return Status;
	}

	Status = WriteProcessMemory(ProcessHandle, Destine, Source, sizeof(BSP_APC_DATA), NULL);
	BspFree(Source);

	if (!Status) {
		Status = GetLastError();
		VirtualFree(Destine, 0, MEM_RELEASE);
		return Status;
	}
	
	//
	// Modify hijackable thread's pc point to NtDelayExecution to make thread enter
	// an alertable wait once they get CPU slice to execute
	//

	while (IsListEmpty(ThreadList) != TRUE) {
		
		ULONG_PTR OldThreadPc;
		ULONG_PTR AdjustedSp;

		ListEntry = RemoveHeadList(ThreadList);
		Thread = CONTAINING_RECORD(ListEntry, BSP_LDR_THREAD, ListEntry);
	
#if defined (_M_IX86)

		OldThreadPc = Thread->Context.Eip;
		Thread->Context.Eip = &Destine->Code;

		Thread->Context.Esp -= sizeof(ULONG_PTR) * 2;
		AdjustedSp = Thread->Context.Esp;

#elif defined (_M_X64)
		
		OldThreadPc = Thread->Context.Rip;
		Thread->Context.Rip = &Destine->Code; 

		Thread->Context.Rsp -= sizeof(ULONG_PTR) * 2;
		AdjustedSp = Thread->Context.Rsp;
#endif

		//
		// Push BSP_APC_DATA pointer on stack
		//

		Status = WriteProcessMemory(ProcessHandle, (PVOID)AdjustedSp, 
									&Destine, sizeof(ULONG_PTR), NULL);

		//
		// Push OldThreadPc on stack
		//

		Status = WriteProcessMemory(ProcessHandle, (PVOID)(AdjustedSp + sizeof(ULONG_PTR)), 
									&OldThreadPc, sizeof(ULONG_PTR), NULL);


		Status = SetThreadContext(Thread->ThreadHandle, &Thread->Context);
		if (Status != TRUE) {
		}

		//
		// Queue Apc routine as LoadLibraryW
		//

		QueueUserAPC((PAPCFUNC)LoadLibraryPtr, Thread->ThreadHandle, &Destine->Path);
		CloseHandle(Thread->ThreadHandle);
		BspFree(Thread);
	}

	//
	// Get target process's PEB base address and save destine BSP_APC_DATA pointer to
	// PEB unused area, the loaded dll must explicitly free it.
	//

	Peb = (ULONG_PTR)BspQueryPebAddress(ProcessHandle);

	//
	// N.B. PEB allocation is 4K on both x86 and x64 architectures, currently,
	// it may be changed in later system.
	//

#if defined (_M_IX86)
	Peb = Peb + 4096 - sizeof(ULONG_PTR);
#elif defined (_M_X64)
	Peb = Peb + 4096 - sizeof(ULONG_PTR);
#endif

	WriteProcessMemory(ProcessHandle, (PVOID)Peb, &Destine, sizeof(ULONG_PTR), NULL);
	return S_OK;
}
	
ULONG
BspScanHijackableThread(
	IN ULONG ProcessId,
	OUT PHANDLE Handle,
	OUT PLIST_ENTRY ThreadList
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	NTSTATUS NtStatus;
	PSYSTEM_PROCESS_INFORMATION Information;
	ULONG Number;
	ULONG RetryCount;
	SIZE_T CompleteBytes;
	ULONG AccessMask;

	//
	// Open process handle with all access 
	//

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!ProcessHandle) {
		Status = GetLastError();
		return Status;
	}

	RetryCount = 0;
	AccessMask = THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION;
	AccessMask = THREAD_ALL_ACCESS;

	InitializeListHead(ThreadList);

Scan:

	//
	// Suspend target process via NtSuspendProcess
	//

	NtStatus = (*NtSuspendProcess)(ProcessHandle);
	if (NtStatus != STATUS_SUCCESS) {
		CloseHandle(ProcessHandle);
		return (ULONG)NtStatus;
	}

	//
	// N.B. Lock hierarchy must be scanned to ensure safe hijackness
	//
	// BspMajorVersion < 6 is used to debug on NT 6+, we don't actually
	// use this routine on NT 6+, it's only for XP, 2003
	//

	if (BspMajorVersion < 6) {
	
		Status = BspScanOwnedLocks(ProcessHandle);
		if (Status != S_OK) {

			(*NtResumeProcess)(ProcessHandle);
			Sleep(2000);
			
			RetryCount += 1;
			if (RetryCount < 3) {
				goto Scan;
			}

			CloseHandle(ProcessHandle);
			return ERROR_NOT_READY;	
		}

		//
		// Reset RetryCount
		//

		RetryCount = 0;

	}

	//
	// Query all actively running threads
	//

	Information = NULL;
	Status = BspQueryProcessInformation(ProcessId, &Information);
	if (Status != ERROR_SUCCESS) {

		if (Information != NULL) {
			BspFree(Information);
		}

		(*NtResumeProcess)(ProcessHandle);
		CloseHandle(ProcessHandle);
		return Status;
	}

	for(Number = 0; Number < Information->NumberOfThreads; Number += 1) {

		PSYSTEM_THREAD_INFORMATION SystemThread;
		ULONG ThreadId;
		HANDLE ThreadHandle;
		CONTEXT Context;
		ULONG_PTR Executing;
		ULONG_PTR ThreadPc;
		
		SystemThread = &Information->Threads[Number];

		//
		// Make sure thread is accssible
		//

		ThreadId = HandleToUlong(SystemThread->ClientId.UniqueThread);
		ThreadHandle = OpenThread(AccessMask, FALSE, ThreadId); 
		if (!ThreadHandle) {
			continue;
		}

		//
		// Get thread context
		//

		Context.ContextFlags = CONTEXT_FULL;
		Status = GetThreadContext(ThreadHandle, &Context);
		if (Status != TRUE) {
			continue;
		}

#if defined (_M_IX86)
		ThreadPc = Context.Eip;
#elif defined (_M_X64)
		ThreadPc = Context.Rip;
#endif
		Status = ReadProcessMemory(ProcessHandle, (PVOID)ThreadPc, 
								   &Executing, sizeof(ULONG_PTR), &CompleteBytes);

		if (Status != TRUE) {
			CloseHandle(ThreadHandle);
			continue;
		}

		//
		// Thread's next pc is ret, or ret imm16 code, it's hijackable, insert into
		// hijackable thread list
		//

		if ((UCHAR)Executing == (UCHAR)0xc3 || (UCHAR)Executing == (UCHAR)0xc2) {

			PBSP_LDR_THREAD LdrThread;

			LdrThread = (PBSP_LDR_THREAD)BspMalloc(sizeof(BSP_LDR_THREAD));
			LdrThread->ThreadId = ThreadId;
			LdrThread->ThreadHandle = ThreadHandle;
			RtlCopyMemory(&LdrThread->Context, &Context, sizeof(Context));

			InsertTailList(ThreadList, &LdrThread->ListEntry);	

			//
			// DEBUG: Only 1 thread is allowed
			//

			break;

		} else {

			//
			// Non-hijackable, close its handle
			//

			CloseHandle(ThreadHandle);
		}
	}

	BspFree(Information);

	if (IsListEmpty(ThreadList) != TRUE) {
		*Handle = ProcessHandle;
		return ERROR_SUCCESS;
	}

	if (IsListEmpty(ThreadList) && RetryCount < 3) {

		//
		// No hijackable thread available, sleep 2 seconds, and retry again
		//

		(*NtResumeProcess)(ProcessHandle);
		Sleep(2000);
		RetryCount += 1;
		DebugTrace("no hijackable thread, retry %d", RetryCount);
		goto Scan;

	} 

	(*NtResumeProcess)(ProcessHandle);
	CloseHandle(ProcessHandle);
	return ERROR_NOT_READY;
}

ULONG
BspExecuteLoadRoutine(
	IN HANDLE ProcessHandle,
	IN PWSTR DllFullPath,
	IN PLIST_ENTRY ThreadList
	)
{
	ULONG Status;
	PVOID Address;
	PBSP_LDR_DATA LdrData;
	HMODULE DllHandle;
	ULONG_PTR CopyLength;
	ULONG_PTR WrittenBytes;
	PLIST_ENTRY ListEntry;
	PBSP_LDR_THREAD Thread;
	ULONG_PTR Peb;
	ULONG_PTR LdrCookie;

	ASSERT(ProcessHandle != NULL);
	ASSERT(IsListEmpty(ThreadList) != TRUE);

	//
	// Allocate target memory block to execute ldr routine
	//

	Address = VirtualAllocEx(ProcessHandle, NULL, 4096, 
		                     MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (!Address) {
		Status = GetLastError();
		return Status;
	}

	//
	// Allocate local copy of ldr data and fill required data, note that
	// since the block is allocated via VirtualAlloc, it's guaranteed that
	// initial are all zero
	//

	LdrData = (PBSP_LDR_DATA)VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);

	LdrData->Address = Address;
	StringCchCopy(LdrData->DllFullPath, MAX_PATH, DllFullPath);

	//
	// Acquire system routine addresses
	//

	DllHandle = GetModuleHandle(L"kernel32.dll");	
	LdrData->GetLastErrorPtr = GetProcAddress(DllHandle, "GetLastError");
	LdrData->SetLastErrorPtr = GetProcAddress(DllHandle, "SetLastError");
	LdrData->CreateThreadPtr = GetProcAddress(DllHandle, "CreateThread");
	LdrData->LoadLibraryPtr = GetProcAddress(DllHandle, "LoadLibraryW");
	LdrData->CloseHandlePtr = GetProcAddress(DllHandle, "CloseHandle");
	LdrData->WaitForSingleObjectPtr = GetProcAddress(DllHandle, "WaitForSingleObject");

	//
	// Copy routine (code only) the copy is from 'push eax' to
	// BspLoadRoutineEnd, refer to ldr32.asm or ldr64.asm
	//

	CopyLength = (ULONG_PTR)BspLoadRoutineEnd - (ULONG_PTR)BspLoadRoutineCode - sizeof(BSP_LDR_DATA);
	RtlCopyMemory((PVOID)(LdrData + 1), 
		          (PVOID)((ULONG_PTR)BspLoadRoutineCode + sizeof(BSP_LDR_DATA)), 
				  CopyLength); 

#if defined (_DEBUG)
	
	//
	// Ensure that the code is copied correctly
	//

	{
		PCHAR Buffer;

		Buffer = (PCHAR)(LdrData + 1);
		ASSERT(Buffer[0] == (CHAR)0x50);
		ASSERT(Buffer[1] == (CHAR)0x53);
	}

#endif

	//
	// Copy ldrdata into target address space include data and code
	//

	CopyLength = (ULONG_PTR)BspLoadRoutineEnd - (ULONG_PTR)BspLoadRoutineCode;
	Status = WriteProcessMemory(ProcessHandle, Address, LdrData, CopyLength, &WrittenBytes);
	
	//
	// Free local allocated ldr data 
	//

	VirtualFree(LdrData, 0, MEM_RELEASE);

	if (Status != TRUE) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Address, 0, MEM_RELEASE);
		return Status;
	}
	
	//
	// Modify hijackable thread's pc point to ldr's code area 
	//

	while (IsListEmpty(ThreadList) != TRUE) {
		
		ULONG_PTR OldThreadPc;
		ULONG_PTR AdjustedSp;

		ListEntry = RemoveHeadList(ThreadList);
		Thread = CONTAINING_RECORD(ListEntry, BSP_LDR_THREAD, ListEntry);
	
#if defined (_M_IX86)

		OldThreadPc = Thread->Context.Eip;
		Thread->Context.Eip = PtrToUlong(Address) + sizeof(BSP_LDR_DATA);

		Thread->Context.Esp -= sizeof(ULONG_PTR);
		AdjustedSp = Thread->Context.Esp;

#elif defined (_M_X64)
		
		OldThreadPc = Thread->Context.Rip;
		Thread->Context.Rip = (ULONG_PTR)Address + sizeof(BSP_LDR_DATA);

		Thread->Context.Rsp -= sizeof(ULONG_PTR);
		AdjustedSp = Thread->Context.Rsp;
#endif

		//
		// Push OldThreadPc on stack
		//

		Status = WriteProcessMemory(ProcessHandle, (PVOID)AdjustedSp, 
									&OldThreadPc, sizeof(ULONG_PTR), &WrittenBytes);

		Status = SetThreadContext(Thread->ThreadHandle, &Thread->Context);
		if (Status != TRUE) {
		}

		BspFree(Thread);
	}

	//
	// Get target process's PEB base address
	//

	Peb = (ULONG_PTR)BspQueryPebAddress(ProcessHandle);

	//
	// We will write ldr data address to last ULONG_PTR of PEB
	// injected dll must read this cookie to locate the allocated buffer
	// to appropriately free this allocation when stop from target
	//

	//
	// N.B. PEB allocation is 4K on both x86 and x64 architectures, currently
	//

#if defined (_M_IX86)
	Peb = Peb + 4096 - sizeof(ULONG_PTR);
#elif defined (_M_X64)
	Peb = Peb + 4096 - sizeof(ULONG_PTR);
#endif

	LdrCookie = (ULONG_PTR)Address;
	WriteProcessMemory(ProcessHandle, (PVOID)Peb, &LdrCookie, sizeof(ULONG_PTR), &WrittenBytes);

	return S_OK;
}

ULONG
BspLoadLibraryEx(
	IN ULONG ProcessId,
	IN PWSTR DllFullPath
	)
{
	ULONG Status;
	BOOLEAN Complete;
	DEBUG_EVENT DebugEvent;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	PVOID MemoryBlock;
	ULONG LastThreadId;
	LIST_ENTRY ThreadList;
	PBSP_LDR_THREAD ThreadObject;
	PVOID DbgUiRemoteBreakin;
	PVOID RtlUserThreadStart;
	PVOID LoadLibraryPtr;
	PVOID RtlCreateUserThreadPtr;
	PVOID RtlExitUserThreadPtr;
	PVOID GetCurrentProcessPtr;
	PBSP_LDR_DBG LdrDbg;
	HMODULE DllHandle;
	SIZE_T Size;
	PVOID RemotePath;
	SIZE_T CompletedLength;
	PBSP_LDR_DBG LdrCopy;
	LONG NtStatus;

	Complete = FALSE;

	DllHandle = GetModuleHandle(L"ntdll.dll");
	RtlUserThreadStart = GetProcAddress(DllHandle, "RtlUserThreadStart");
	DbgUiRemoteBreakin = GetProcAddress(DllHandle, "DbgUiRemoteBreakin");
	RtlCreateUserThreadPtr = GetProcAddress(DllHandle, "RtlCreateUserThread");
	RtlExitUserThreadPtr = GetProcAddress(DllHandle, "RtlExitUserThread");

	DllHandle = GetModuleHandle(L"kernel32.dll");
	LoadLibraryPtr = GetProcAddress(DllHandle, "LoadLibraryW");
	GetCurrentProcessPtr = GetProcAddress(DllHandle, "GetCurrentProcess");

	Status = DebugActiveProcess(ProcessId);
	if (Status != TRUE) {
		return GetLastError();
	}

	Status = DebugSetProcessKillOnExit(FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugActiveProcessStop(ProcessId);	
		return Status;
	}

	//
	// Print ntdll!DbgBreakPoint address
	//

	LdrDbg = NULL;
	ProcessHandle = NULL;

	while (Complete != TRUE) {

		CONTEXT Context;

		Status = WaitForDebugEvent(&DebugEvent, INFINITE);
		if (Status != TRUE) {
			break;
		}

		switch (DebugEvent.dwDebugEventCode) {

			case CREATE_THREAD_DEBUG_EVENT:

				// (vista, 2k8, win7) if ((ULONG_PTR)RtlUserThreadStart == Context.Eip) {

				// (xp, 2k3)
				//if ((ULONG_PTR)DbgUiRemoteBreakin == Context.Eip) {
					
					//
					// N.B. This is the debugger thread to be ran, we replace it with
					// our loader thread
					//
				{
					BOOLEAN Hijacked = FALSE;	
					//BspHijackRemoteBreakIn(&DebugEvent, DllFullPath, &Hijacked);
					BspHijackDebugger(&DebugEvent, DllFullPath, &Hijacked);
					if (Hijacked) {
						Complete = TRUE;
					}
					break;

				}
				//}

				//
				// Record last notified thread id
				//

				LastThreadId = DebugEvent.dwThreadId;
				break;

			case EXCEPTION_DEBUG_EVENT: 

				{
				Context.ContextFlags = CONTEXT_ALL;			
				ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, DebugEvent.dwThreadId);
				Status = GetThreadContext(ThreadHandle, &Context);
				Complete = TRUE;
				}
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				{
					WCHAR Buffer[MAX_PATH];
					HANDLE Handle;

					RtlZeroMemory(Buffer, sizeof(Buffer));
					Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DebugEvent.dwProcessId);
					ReadProcessMemory(Handle, DebugEvent.u.DebugString.lpDebugStringData, Buffer,
									  DebugEvent.u.DebugString.nDebugStringLength * 2, &Size);
					break;
				}

		}

		ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
	}

	DebugActiveProcessStop(ProcessId);

	{
		WCHAR Buffer[MAX_PATH];
		BSP_LDR_DBG Result;
		ReadProcessMemory(ProcessHandle, (PCHAR)LdrDbg,
			              &Result, sizeof(BSP_LDR_DBG), &CompletedLength);

		StringCchPrintf(Buffer, MAX_PATH, L"NTSTATUS=0x%08x, TID=0x%x", 
						(ULONG)Result.NtStatus, Result.ThreadId, Result);

		MessageBox(NULL, Buffer, "NTSTATUS", MB_OK);
	}

	if (Status != ERROR_SUCCESS) {
		//WaitForSingleObject(ThreadHandle, INFINITE);
		CloseHandle(ProcessHandle);
		//CloseHandle(ThreadHandle);
	}

	return Status;
}
		 
typedef HMODULE 
(WINAPI *LOAD_LIBRARY)(
	IN PWSTR Path
	);

typedef NTSTATUS
(NTAPI *RTL_EXIT_USER_THREAD)(
	IN ULONG Status
	);

//#if defined (_M_IX86)

#pragma pack(push, 1)

typedef struct _BSP_LOADER {
	LOAD_LIBRARY LoadLibraryPtr;
	RTL_EXIT_USER_THREAD RtlExitUserThreadPtr;
	WCHAR Path[MAX_PATH];
	CHAR LoaderThread[100];
} BSP_LOADER, *PBSP_LOADER;

#pragma pack(pop)

CHAR BspLoaderTemplate[] = {
	
	0x64, 0xa1, 0x18, 0x00, 0x00, 0x00, //  mov  eax,dword ptr fs:[00000018h]
	0x8b, 0x40, 0x30,                   //  mov  eax,dword ptr [eax+30h]
	0xc6, 0x40, 0x02, 0x00,				//  mov  byte ptr[eax+2], 0
	0x55,								//  push ebp
	0x8b, 0xec,							//	mov  ebp, esp
	0x8b, 0x5d, 0x08,					//	mov  ebx, dword ptr[ebp+8] 
	0x8d, 0x4b, 0x08,					//	lea  ecx, dword ptr[Path]
	0x51, 								//	push ecx	
	0xff, 0x13,							//	call dword ptr[LoadLibraryW]
	0x6a, 0x00,							//	push 0
	0xff, 0x53, 0x04,					//	call dword ptr[RtlExitUserThread]
	0xcc,								//	int 3
	0xc2, 0x04, 0x00					//	ret 4
};

CHAR BspLoaderTemplate2[] = {
	
	0x64, 0xa1, 0x18, 0x00, 0x00, 0x00, //  mov  eax,dword ptr fs:[00000018h]
	0x8b, 0x40, 0x30,                   //  mov  eax,dword ptr [eax+30h]
	0xc6, 0x40, 0x02, 0x00,				//  mov  byte ptr[eax+2], 0
	0x55,								//  push ebp
	0x8b, 0xec,							//	mov  ebp, esp
	0x8b, 0x5d, 0x08,					//	mov  ebx, dword ptr[ebp+8] 
	0x8d, 0x4b, 0x08,					//	lea  ecx, dword ptr[Path]
	0x51, 								//	push ecx	
	0xff, 0x13,							//	call dword ptr[LoadLibraryW]
	0x6a, 0x00,							//	push 0
	0xff, 0x53, 0x04,					//	call dword ptr[RtlExitUserThread]
	0xcc,								//	int 3
	0xc2, 0x04, 0x00					//	ret 4
};

//#endif


ULONG
BspHijackDebugger(
	IN DEBUG_EVENT *DebugEvent,
	IN PWSTR DllFullPath,
	OUT PBOOLEAN Hijacked
	)
{
	HANDLE ThreadHandle;
	HANDLE ProcessHandle;
	PBSP_LOADER TargetData;
	PBSP_LOADER LocalData;
	HMODULE DllHandle;
	PVOID DbgUiRemoteBreakIn;
	PVOID LoadLibraryPtr;
	PVOID RtlExitUserThreadPtr;
	CONTEXT Context;
	ULONG Status;
	SIZE_T Size;

	//
	// Complete dll injection, jump out of while
	//

	Context.ContextFlags = CONTEXT_ALL;			
	ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, DebugEvent->dwThreadId);
	Status = GetThreadContext(ThreadHandle, &Context);

	//
	// Only hijack the debugger break thread
	//

	DllHandle = GetModuleHandle(L"ntdll.dll");
	DbgUiRemoteBreakIn = GetProcAddress(DllHandle, "DbgUiRemoteBreakin");

#if defined (_M_IX86)
	if ((ULONG_PTR)DbgUiRemoteBreakIn != Context.Eip) {
#elif defined (_M_X64)
	if ((ULONG_PTR)DbgUiRemoteBreakIn != Context.Rip) {
#endif
		*Hijacked = FALSE;
		return S_OK;
	}

	LoadLibraryPtr = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	RtlExitUserThreadPtr = GetProcAddress(DllHandle, "RtlExitUserThread");

	LocalData = (PBSP_LOADER)BspMalloc(4096);
	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DebugEvent->dwProcessId);
	TargetData = (PBSP_LDR_DBG)VirtualAllocEx(ProcessHandle, NULL, 
										      4096, MEM_COMMIT, 
										      PAGE_EXECUTE_READWRITE);

	//
	// Refer to ntdll!RtlUserThreadStart in NT 6+
	//

	LocalData->LoadLibraryPtr = LoadLibraryPtr;
	LocalData->RtlExitUserThreadPtr = RtlExitUserThreadPtr;

	RtlCopyMemory(LocalData->Path, DllFullPath, (wcslen(DllFullPath) + 1) * 2);
	RtlCopyMemory(LocalData->LoaderThread, BspLoaderTemplate, sizeof(BspLoaderTemplate));

	//
	// Adjust ESP/RSP register, ensure it's safe to write data on stack
	//

#if defined (_M_IX86)
	Context.Esp = Context.Esp - 16;
#elif defined (_M_X64)
	Context.Rsp = Context.Rsp - 16 * 2;
#endif

	Status = WriteProcessMemory(ProcessHandle, TargetData, LocalData, sizeof(BSP_LOADER), &Size);

#if defined (_M_IX86)

	Status = WriteProcessMemory(ProcessHandle, Context.Esp + sizeof(ULONG_PTR), 
		                        &TargetData, sizeof(ULONG_PTR), &Size);
	Context.Eip = (ULONG_PTR)&TargetData->LoaderThread;

#elif defined (_M_X64)
	Status = WriteProcessMemory(ProcessHandle, Context.Rsp + sizeof(ULONG_PTR), 
		                        &TargetData, sizeof(ULONG_PTR), &Size);
	Context.Rip = (ULONG_PTR)&TargetData->LoaderThread;
#endif


	Status = SetThreadContext(ThreadHandle, &Context);
	*Hijacked = TRUE;
	return Status;
}

ULONG
BspHijackRemoteBreakIn(
	IN DEBUG_EVENT *DebugEvent,
	IN PWSTR DllFullPath,
	OUT PBOOLEAN Hijacked
	)
{
	HANDLE ThreadHandle;
	HANDLE ProcessHandle;
	PBSP_LDR_DBG TargetData;
	PBSP_LDR_DBG LocalData;
	HMODULE DllHandle;
	PVOID DbgUiRemoteBreakIn;
	PVOID NtClose;
	PVOID LdrLoadDllPtr;
	PVOID LoadLibraryPtr;
	PVOID RtlCreateUserThreadPtr;
	PVOID NtWaitForSingleObjectPtr;
	PVOID RtlExitUserThreadPtr;
	CONTEXT Context;
	ULONG Status;
	SIZE_T Size;

	//
	// Complete dll injection, jump out of while
	//

	Context.ContextFlags = CONTEXT_ALL;			
	ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, DebugEvent->dwThreadId);
	Status = GetThreadContext(ThreadHandle, &Context);

	//
	// Only hijack the debugger break thread
	//

	DllHandle = GetModuleHandle(L"ntdll.dll");
	DbgUiRemoteBreakIn = GetProcAddress(DllHandle, "DbgUiRemoteBreakin");

#if defined (_M_IX86)
	if ((ULONG_PTR)DbgUiRemoteBreakIn != Context.Eip) {
#elif defined (_M_X64)
	if ((ULONG_PTR)DbgUiRemoteBreakIn != Context.Rip) {
#endif
		*Hijacked = FALSE;
		return S_OK;
	}

	LdrLoadDllPtr = GetProcAddress(DllHandle, "LdrLoadDll");
	LoadLibraryPtr = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	NtWaitForSingleObjectPtr = GetProcAddress(DllHandle, "NtWaitForSingleObject");
	RtlCreateUserThreadPtr = GetProcAddress(DllHandle, "RtlCreateUserThread");
	RtlExitUserThreadPtr = GetProcAddress(DllHandle, "RtlExitUserThread");


	LocalData = (PBSP_LDR_DBG)BspMalloc(4096);
	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DebugEvent->dwProcessId);
	TargetData = (PBSP_LDR_DBG)VirtualAllocEx(ProcessHandle, NULL, 
										      4096, MEM_COMMIT, 
										      PAGE_EXECUTE_READWRITE);

	//
	// Refer to ntdll!RtlUserThreadStart in NT 6+
	//

	LocalData->RtlCreateUserThreadPtr = RtlCreateUserThreadPtr;
	//LocalData->LdrLoadDllPtr = LdrLoadDllPtr;
	LocalData->LoadLibraryPtr = LoadLibraryPtr;
	LocalData->RtlExitUserThreadPtr = RtlExitUserThreadPtr;
	LocalData->DllFullPath = (ULONG_PTR)TargetData + FIELD_OFFSET(BSP_LDR_DBG, Path);

	RtlCopyMemory(LocalData->Path, DllFullPath, (wcslen(DllFullPath) + 1) * 2);
	RtlCopyMemory(LocalData->Code, BspDebuggerCode, sizeof(BspDebuggerCode));

	WriteProcessMemory(ProcessHandle,
					   TargetData, LocalData,
					   sizeof(BSP_LDR_DBG),
					   &Size);

#if defined (_M_IX86)
	Context.Eax = (ULONG_PTR)TargetData;
	Context.Eip = (ULONG_PTR)&TargetData->Code;

#elif defined (_M_X64)
	Context.Rax = (ULONG_PTR)TargetData;
	Context.Rip = (ULONG_PTR)&TargetData->Code;

#endif

	Status = SetThreadContext(ThreadHandle, &Context);
	return Status;
}

ULONG CALLBACK
BspLoaderThread(
	IN PVOID Context
	)
{
	PBSP_LOADER Loader;
		
	Loader = (PBSP_LOADER)Context;

	(*Loader->LoadLibraryPtr)(Loader->Path);
	(*Loader->RtlExitUserThreadPtr)(0);
	return 0;
}

//
// N.B. These offset are only valid for Windows XP, Windows Server 2003
//

#define LOADER_LOCK_OFFSET         0x000000A0 
#define FASTPEB_LOCK_OFFSET        0x0000001C
#define PROCESS_HEAP_OFFSET        0x00000018

//
// If FrontEndHeapType is 2, it's LFH, FrontEndHeap is a pointer
// to RTL_CRITICAL_SECTION, we need check this, we don't check 
// other type since they does not incur further locking hierarchy.
//

#define HEAP_LOCK_OFFSET           0x00000578
#define FRONT_END_HEAP_POINTER     0x00000580
#define FRONT_END_HEAP_TYPE_OFFSET 0x00000586 

//
// When target application enable avrf and pageheap, we can not safely
// inject runtime into it, avrf essentially hook functions as us, pageheap
// can very easily cause memory allocation failure and crash the application.
//

// 
// The following macros are registry value representing gflags we're concern about, e.g.
//
// HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\wmplayer.exe
// it's GlobalFlag DWORD value is 0x02000100 if avrf is enabled, this also effectively enable
// pageheap, however, pageheap can be solely enabled by a value 0x02000000, we need check both.
//

#define GFLAG_APPLICATION_VERIFIER 0x00000100
#define GFLAG_HEAP_PAGE_ALLOCS     0x02000000
#define GFLAG_PROBE_EXEMPTION  (GFLAG_APPLICATION_VERIFIER | GFLAG_HEAP_PAGE_ALLOCS)

//
// This routine assume all threads are suspended, this routine can only be called
// to support remote injection of runtime for Windows XP, Windows 2003 (x86 and x64),
// for Vista, Windows Server 2008, Windows 7, Windows Server 2008 R2, RtlCreateUserThread
// is used.
//

ULONG
BspScanOwnedLocks(
	IN HANDLE ProcessHandle
	)
{
	ULONG Status;
	PVOID PebAddress;
	ULONG_PTR ObjectAddress;
	ULONG_PTR UlongPtrValue;
	RTL_CRITICAL_SECTION LockObject;
	CHAR Buffer[4096];

	//
	// This routine is only applied to Windows XP and Windows Server 2003
	//

	PebAddress = BspQueryPebAddress(ProcessHandle);
	if (!PebAddress) {
		return S_FALSE;
	}

	//
	// Read the whole PEB structure
	//

	Status = ReadProcessMemory(ProcessHandle, (PVOID)PebAddress, Buffer, 4096, NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	//
	// Compute loader lock address
	//

	ObjectAddress = *(PULONG_PTR)&Buffer[LOADER_LOCK_OFFSET];
	Status = ReadProcessMemory(ProcessHandle, (PVOID)ObjectAddress, &LockObject, sizeof(LockObject), NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	if (LockObject.OwningThread) {
		return S_FALSE;
	}

	//
	// Compute PEB lock address
	//

	ObjectAddress = *(PULONG_PTR)&Buffer[FASTPEB_LOCK_OFFSET];
	Status = ReadProcessMemory(ProcessHandle, (PVOID)ObjectAddress, &LockObject, sizeof(LockObject), NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	if (LockObject.OwningThread) {
		return S_FALSE;
	}

	//
	// Compute process heap address
	//

	ObjectAddress = *(PULONG_PTR)&Buffer[PROCESS_HEAP_OFFSET];
	Status = ReadProcessMemory(ProcessHandle, (PVOID)ObjectAddress, &UlongPtrValue, sizeof(ULONG_PTR), NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}
	
	//
	// Read process heap structure
	//

	ObjectAddress = UlongPtrValue;
	Status = ReadProcessMemory(ProcessHandle, (PVOID)ObjectAddress, Buffer, 4096, NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}
	
	ObjectAddress = &Buffer[HEAP_LOCK_OFFSET];
	Status = ReadProcessMemory(ProcessHandle, (PVOID)ObjectAddress, &LockObject, sizeof(LockObject), NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	if (LockObject.OwningThread) {
		return S_FALSE;
	}
	
	UlongPtrValue = *(PULONG_PTR)&Buffer[FRONT_END_HEAP_TYPE_OFFSET];
	if (UlongPtrValue != 2) {

		//
		// LFH is not enabled, we can safely execute remote call
		//

		return S_OK;
	}
	
	//
	// For LFH, the FrontEndHeap is a pointer to RTL_CRITICAL_SECTION
	//

	ObjectAddress = *(PULONG_PTR)&Buffer[FRONT_END_HEAP_POINTER];
	Status = ReadProcessMemory(ProcessHandle, (PVOID)ObjectAddress, &LockObject, sizeof(LockObject), NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	if (LockObject.OwningThread) {
		return S_FALSE;
	}
	
	//
	// The whole lock hierarchy is free, it's safe to execute remote call
	//

	return S_OK;
}

#if defined (_M_IX86)

ULONG
BspExecuteRemoteCall(
	IN HANDLE ProcessHandle,
	IN PVOID CallSite,
	IN ULONG ArgumentCount,
	IN PULONG_PTR Argument,
	IN PBSP_LDR_THREAD Thread,
	IN PVOID SuspendThreadPtr,
	IN HANDLE CompleteEvent,
	IN PVOID SetEventPtr
	)
{
	ULONG Status;
    CONTEXT Context;
	ULONG_PTR NewSp;
    ULONG_PTR ArgumentsCopy[16];
	HANDLE DuplicatedThreadHandle;
	HANDLE DuplicatedEventHandle;

	if (ArgumentCount > 12) {
		return ERROR_INVALID_PARAMETER;
	}
	
	//
	// Sanitize thread object's data
	//

	Thread->AdjustedEip = NULL;
	Thread->AdjustedEsp = NULL;

	//
	// Duplicate opened thread handle into target address space, because we need
	// call SetThreadContext(DuplicatedThreadHandle, OldContext) to recover the
	// hijacked thread to its old state
	//

	Status = DuplicateHandle(GetCurrentProcess(), Thread->ThreadHandle, ProcessHandle, 
							 &DuplicatedThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

	Status = DuplicateHandle(GetCurrentProcess(), CompleteEvent, ProcessHandle, 
							 &DuplicatedEventHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	RtlCopyMemory(&Context, &Thread->Context, sizeof(CONTEXT));

	//
	//	Put Context Record on stack first, so it is above other args.
	//

	NewSp = Context.Esp;
	NewSp -= sizeof(CONTEXT);

	Status = WriteProcessMemory(ProcessHandle, (PVOID)NewSp, &Context, sizeof(CONTEXT), NULL);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	//
	// Assign return address as SetEvent then SuspendThread 
	//

	ArgumentsCopy[0] = SetEventPtr;   

	if (ArgumentCount) {
		RtlCopyMemory(&ArgumentsCopy[1], Argument, ArgumentCount * sizeof(ULONG_PTR));
	}

	ArgumentCount++;

	//
	// After execute SetEvent, the thread must call SuspendThread to suspend itself
	//

	ArgumentsCopy[ArgumentCount] = SuspendThreadPtr;
	ArgumentsCopy[ArgumentCount + 1] = DuplicatedEventHandle;
	ArgumentsCopy[ArgumentCount + 2] = 0xbedbabeb;
	ArgumentsCopy[ArgumentCount + 3] = DuplicatedThreadHandle;

	ArgumentCount += 4;

	//
	//	Copy the arguments onto the target stack
	//

	NewSp -= ArgumentCount * sizeof(ULONG_PTR);
	Status = WriteProcessMemory(ProcessHandle, (PVOID)NewSp, ArgumentsCopy, 
			                    ArgumentCount * sizeof(ULONG_PTR), NULL );
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	//
	// Set thread context to execute CallSite, when it return, it will execute
	// a SetThreadContext to recover to its original thread context
	//

	Context.Esp = NewSp;
	Context.Eip = (ULONG_PTR)CallSite;

	//
	// Record adjusted Eip and Esp for recovery those pending threads
	//

	Thread->AdjustedEsp = Context.Esp;
	Thread->AdjustedEip = Context.Eip;

	Status = SetThreadContext(Thread->ThreadHandle, &Context);

	if (!Status) {
		Status = GetLastError();
		return Status;
	}

    return S_OK;
}

#endif


#if defined (_M_X64)

ULONG
BspExecuteRemoteCall(
	IN HANDLE ProcessHandle,
	IN PVOID CallSite,
	IN ULONG ArgumentCount,
	IN PULONG_PTR Argument,
	IN PBSP_LDR_THREAD Thread,
	IN PVOID SuspendThreadPtr,
	IN HANDLE CompleteEvent,
	IN PVOID SetEventPtr
	)
{
	return 0;
}

#endif

PVOID
BspGetRemoteApiAddress(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN PSTR ApiName
	)
{ 
	HMODULE DllHandle;
	ULONG_PTR Address;
	HMODULE LocalDllHandle;
	ULONG_PTR LocalAddress;
	SIZE_T Size;
	ULONG Status;

	ASSERT(GetCurrentProcessId() != ProcessId);
	
	LocalDllHandle = GetModuleHandle(DllName);
	if (!LocalDllHandle) {
		return NULL;
	}
	
	LocalAddress = GetProcAddress(LocalDllHandle, ApiName);
	if (!LocalAddress) {
		return NULL;
	}

	//
	// N.B. It's only valid to a few system dll (non side by side), we call this
	// routine primarily to handle ASLR case.
	//

	Status = BspGetModuleInformation(ProcessId, DllName, FALSE, &DllHandle, &Address, &Size);
	if (!Status) {
		return NULL;
	}

	return (ULONG_PTR)DllHandle + (LocalAddress - (ULONG_PTR)LocalDllHandle);	
}

BOOLEAN
BspGetModuleInformation(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN BOOLEAN FullPath,
	OUT HMODULE *DllHandle,
	OUT PULONG_PTR Address,
	OUT SIZE_T *Size
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
				*Address = Module.modBaseAddr;	
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

ULONG
BspInjectPreExecute(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle,
	IN PWSTR ImagePath,
	IN HANDLE CompleteEvent,
	IN HANDLE SuccessEvent, 
	IN HANDLE ErrorEvent 
	)
{
	ULONG Status;
	DEBUG_EVENT DebugEvent;
	PVOID EntryPoint;
	BOOLEAN FirstBreak = FALSE;
	CHAR BreakPoint = 0xCC;
	CHAR Opcode;
	ULONG Protect;
	WCHAR ModuleName[MAX_PATH];

	Status = DebugSetProcessKillOnExit(FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugActiveProcessStop(ProcessId);	
		return Status;
	}

	while (TRUE) {

		Status = WaitForDebugEvent(&DebugEvent, INFINITE);
		if (Status != TRUE) {
			break;
		}

		switch (DebugEvent.dwDebugEventCode) {

			case CREATE_PROCESS_DEBUG_EVENT:
				EntryPoint = DebugEvent.u.CreateProcessInfo.lpStartAddress;
				Status = DBG_CONTINUE;
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				Status = DBG_COMPLETE;
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case EXIT_THREAD_DEBUG_EVENT: 
				Status = DBG_CONTINUE;
				break;

			case LOAD_DLL_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				Status = DBG_CONTINUE;
				break;
			
			case EXCEPTION_DEBUG_EVENT: 
				
				if (!FirstBreak) {
				
					FirstBreak = TRUE;
					
					//
					// Write a break point at the entry point of the process
					//

					ReadProcessMemory(ProcessHandle, EntryPoint, &Opcode, sizeof(Opcode), NULL);
					VirtualProtectEx(ProcessHandle, EntryPoint, 4096, PAGE_EXECUTE_READWRITE, &Protect);
					WriteProcessMemory(ProcessHandle, EntryPoint, &BreakPoint, sizeof(BreakPoint), NULL);
					Status = DBG_CONTINUE;

				} else {
					
					//
					// Check whether it's our break point, if yes, recover its original opcode
					// and redirect thread's pc to BSP_PREEXECUTE_CODE stub
					//
					
					PCHAR Code;

					if (DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress == EntryPoint) {

						BSP_PREEXECUTE_CONTEXT PreExecuteContext = {0};
						CONTEXT Context;
						USHORT ActualLength;

						Context.ContextFlags = CONTEXT_FULL;
						WriteProcessMemory(ProcessHandle, EntryPoint, &Opcode, sizeof(Opcode), NULL);
						VirtualProtectEx(ProcessHandle, EntryPoint, 4096, Protect, &Protect);
						GetThreadContext(ThreadHandle, &Context);

						//
						// Allocate stub memory and copy BspPreExecuteCode template to target address space
						//

#ifdef _M_IX86
						PreExecuteContext.Eax = Context.Eax;
						PreExecuteContext.Ecx = Context.Ecx;
						PreExecuteContext.Edx = Context.Edx;
						PreExecuteContext.Ebx = Context.Ebx;
						PreExecuteContext.Esi = Context.Esi;
						PreExecuteContext.Edi = Context.Edi;
						PreExecuteContext.Ebp = Context.Ebp;
						PreExecuteContext.Esp = Context.Esp;
						PreExecuteContext.EFlag = Context.EFlags;
						PreExecuteContext.Eip = Context.Eip - 1;

						DuplicateHandle(GetCurrentProcess(), CompleteEvent, ProcessHandle, 
							            &PreExecuteContext.CompleteEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
							            
						DuplicateHandle(GetCurrentProcess(), SuccessEvent, ProcessHandle, 
							            &PreExecuteContext.SuccessEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
							            
						DuplicateHandle(GetCurrentProcess(), ErrorEvent, ProcessHandle, 
							            &PreExecuteContext.ErrorEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
							            
						PreExecuteContext.LoadLibraryAddr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
						PreExecuteContext.SetEventAddr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "SetEvent");
						PreExecuteContext.WaitForSingleObjectAddr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "WaitForSingleObject");
						PreExecuteContext.CloseHandleAddr = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "CloseHandle");
						PspGetRuntimeFullPathName(PreExecuteContext.Path, MAX_PATH, &ActualLength);

						Code = VirtualAllocEx(ProcessHandle, NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
						WriteProcessMemory(ProcessHandle, Code, &PreExecuteContext, sizeof(PreExecuteContext), NULL);
						WriteProcessMemory(ProcessHandle, Code + sizeof(BSP_PREEXECUTE_CONTEXT), 
							               BspPreExecuteCode, sizeof(BspPreExecuteCode), NULL);

						//
						// Load the pre-execute context address into ebx register and redirect
						// thread's pc to execute the stub code
						//

						Context.Ebx = Code;
						Context.Eip = (ULONG)(Code + sizeof(BSP_PREEXECUTE_CONTEXT));

						SetThreadContext(ThreadHandle, &Context);
#endif
						Status = DBG_COMPLETE;

					} else {

						//
						// Don't care about other exceptions
						//

						Status = DBG_CONTINUE;
					}
				}

				break;
		}

		if (Status == DBG_COMPLETE) {
			ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
			break;
		} else {
			ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, Status);
		}
	}

	DebugActiveProcessStop(ProcessId);
	return S_OK;
}

ULONG
BspWaitDllLoaded(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle
	)
{
	ULONG Status;
	DEBUG_EVENT DebugEvent;
	PVOID EntryPoint;
	BOOLEAN FirstBreak = FALSE;
	CHAR BreakPoint = 0xCC;
	CHAR Opcode;
	ULONG Protect;

	/*Status = DebugActiveProcess(ProcessId);
	if (Status != TRUE) {
		return GetLastError();
	}*/

	Status = DebugSetProcessKillOnExit(FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugActiveProcessStop(ProcessId);	
		return Status;
	}

	while (TRUE) {

		Status = WaitForDebugEvent(&DebugEvent, INFINITE);
		if (Status != TRUE) {
			break;
		}

		switch (DebugEvent.dwDebugEventCode) {

			case CREATE_PROCESS_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				Status = DBG_COMPLETE;
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case EXIT_THREAD_DEBUG_EVENT: 
				Status = DBG_CONTINUE;
				break;

			case LOAD_DLL_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
				Status = DBG_CONTINUE;
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				Status = DBG_CONTINUE;
				break;
			
			case EXCEPTION_DEBUG_EVENT: 
				SuspendThread(ThreadHandle);
				Status = DBG_COMPLETE;
				break;
				
		}

		if (Status == DBG_COMPLETE) {
			ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
			break;
		} else {
			ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, Status);
		}
	}

	DebugActiveProcessStop(ProcessId);
	return S_OK;
}