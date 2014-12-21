//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "aps.h"
#include "apsdefs.h"
#include "apsldr.h"

#define DBG_COMPLETE 1

#if defined (_M_IX86)

CHAR ApsPreExecuteCode[] = {
					
	0x8d, 0x4b, 0x44,			// lea  ecx, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Path] 
	0x51,						// push ecx
	0xff, 0x53, 0x34,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.LoadLibraryAddr]
					
	0x83, 0xf8, 0x00,           // cmp eax, 0
	0x74, 0x40,                 // jz Failed

	0xff, 0x73, 0x2c,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.SuccessEvent]
	0xff, 0x53, 0x38,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.SetEventAddr]
					
	0x6a, 0xff,					// push -1
	0xff, 0x73, 0x28,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.CompleteEvent]
	0xff, 0x53, 0x3c,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.WaitForSingleObjectAddr]
					
	0xff, 0x73, 0x2c,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.SuccessEvent]
	0xff, 0x53, 0x40,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.CloseHandleAddr]

	0xff, 0x73, 0x30,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.ErrorEvent]
	0xff, 0x53, 0x40,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.CloseHandleAddr]

	0xff, 0x73, 0x28,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.CompleteEvent]
	0xff, 0x53, 0x40,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.CloseHandleAddr]

	0x8b, 0x43, 0x20,			// mov eax, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xax]
	0x8b, 0x4b, 0x1c,			// mov ecx, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xcx]
	0x8b, 0x53, 0x18,			// mov edx, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xdx]
	0x8b, 0x73, 0x08,			// mov esi, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xsi]
	0x8b, 0x7b, 0x04,			// mov edi, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xdi]
	0x8b, 0x6b, 0x0c,			// mov ebp, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xbp]
	0x8b, 0x63, 0x10,			// mov esp, dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xsp]
					
	0xff, 0x33,					// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.XFlags]
	0x9d,						// popfd
					
	0xff, 0x73, 0x24,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xip]
	0xff, 0x73, 0x14,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.Xbx]
	0x5b,						// pop  ebx
	0xc3,						// ret

	0xff, 0x73, 0x30,			// push dword ptr[ebx + APS_PREEXECUTE_CONTEXT.ErrorEvent]
	0xff, 0x53, 0x38,			// call dword ptr[ebx + APS_PREEXECUTE_CONTEXT.SetEventAddr]	
	0xeb, 0xbe					// jmp  Complete
};

#elif defined(_M_X64)

CHAR ApsPreExecuteCode[] = {

	0x48, 0x83, 0xec, 0x28,			// sub rsp, 28h
	0x48, 0x8d, 0x4b, 0x68,			// lea rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.Path] 
	0xff, 0x53, 0x48,				// call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.LoadLibraryAddr]
					
	0x48, 0x83, 0xf8, 0x00,			// cmp rax, 0
	0x74, 0x43,						// jz  Failed
					
	0x48, 0x8b, 0x4b, 0x38,			// mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.SuccessEvent]
	0xff, 0x53, 0x50,				// call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.SetEventAddr]	
					
	0xba, 0xff, 0xff, 0xff, 0xff,	// mov edx, -1 ; INFINITE (dword)
	0x48, 0x8b, 0x4b, 0x30,			// mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.CompleteEvent]
	0xff, 0x53, 0x58,				// call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.WaitForSingleObjectAddr]
					
	0x48, 0x8b, 0x4b, 0x38,			// mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.SuccessEvent]
	0xff, 0x53, 0x60,			    // call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.CloseHandleAddr]
					
	0x48, 0x8b, 0x4b, 0x40,		    // mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.ErrorEvent]
	0xff, 0x53, 0x60,			    // call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.CloseHandleAddr]
					
	0x48, 0x8b, 0x4b, 0x30,			// mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.CompleteEvent]
	0xff, 0x53, 0x60,		        // call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.CloseHandleAddr]
					
	0x48, 0x8b, 0x4b, 0x10,			// mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT._Rcx]
	0x48, 0x8b, 0x53, 0x18,		    // mov rdx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT._Rdx]
	0x4c, 0x8b, 0x43, 0x20,			// mov r8,  qword ptr[rbx + APS_PREEXECUTE_CONTEXT._R8]
	0x4c, 0x8b, 0x4b, 0x28,			// mov r9,  qword ptr[rbx + APS_PREEXECUTE_CONTEXT._R9]
					
	0x48, 0x83, 0xc4, 0x28,			// add rsp, 28h
					
	0xff, 0x33,						// push qword ptr[rbx + APS_PREEXECUTE_CONTEXT._Rip]
	0xff, 0x73, 0x08,				// push qword ptr[rbx + APS_PREEXECUTE_CONTEXT._Rbx]
	0x5b,							// pop  rbx
					
	0xc3,							// ret
					
	0x48, 0x8b, 0x4b, 0x40,			// mov rcx, qword ptr[rbx + APS_PREEXECUTE_CONTEXT.ErrorEvent]
	0xff, 0x53, 0x50,				// call qword ptr[rbx + APS_PREEXECUTE_CONTEXT.SetEventAddr]	
	0xeb, 0xbb						// jmp  Complete
};

#endif

ULONG
ApsInjectPreExecute(
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in HANDLE ThreadHandle,
	__in PWSTR RuntimePath,
	__in HANDLE CompleteEvent,
	__in HANDLE SuccessEvent, 
	__in HANDLE ErrorEvent,
	__out PVOID *CodePtr
	)
{
	ULONG Status;
	DEBUG_EVENT DebugEvent;
	PVOID EntryPoint;
	BOOLEAN FirstBreak = FALSE;
	CHAR BreakPoint = 0xcc;
	CHAR Opcode;
	ULONG Protect;
	PCHAR Code;
	CONTEXT Context;
	APS_PREEXECUTE_CONTEXT PreExecuteContext = {0};

	Status = DebugSetProcessKillOnExit(FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugActiveProcessStop(ProcessId);	
		return Status;
	}

	//
	// N.B. This assume caller use CreateProcess to suspend
	// target already.
	//

	ResumeThread(ThreadHandle);

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
					// and redirect thread's pc to APS_PREEXECUTE_CODE stub
					//
					
					if (DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress == EntryPoint) {

						Context.ContextFlags = CONTEXT_FULL;
						WriteProcessMemory(ProcessHandle, EntryPoint, &Opcode, sizeof(Opcode), NULL);
						VirtualProtectEx(ProcessHandle, EntryPoint, 4096, Protect, &Protect);
						GetThreadContext(ThreadHandle, &Context);

						//
						// Allocate stub memory and copy ApsPreExecuteCode template to target address space
						//

#if defined(_M_IX86)
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
#elif defined (_M_X64)
						PreExecuteContext.Rip = Context.Rip- 1;
						PreExecuteContext.Rbx = Context.Rbx;
						PreExecuteContext.Rcx = Context.Rcx;
						PreExecuteContext.Rdx = Context.Rdx;
						PreExecuteContext.R8 = Context.R8;
						PreExecuteContext.R9 = Context.R9;
#endif

						DuplicateHandle(GetCurrentProcess(), CompleteEvent, ProcessHandle, 
							            &PreExecuteContext.CompleteEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
							            
						DuplicateHandle(GetCurrentProcess(), SuccessEvent, ProcessHandle, 
							            &PreExecuteContext.SuccessEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
							            
						DuplicateHandle(GetCurrentProcess(), ErrorEvent, ProcessHandle, 
							            &PreExecuteContext.ErrorEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
							            
						PreExecuteContext.LoadLibraryAddr = ApsGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
						PreExecuteContext.SetEventAddr = ApsGetRemoteApiAddress(ProcessId, L"kernel32.dll", "SetEvent");
						PreExecuteContext.WaitForSingleObjectAddr = ApsGetRemoteApiAddress(ProcessId, L"kernel32.dll", "WaitForSingleObject");
						PreExecuteContext.CloseHandleAddr = ApsGetRemoteApiAddress(ProcessId, L"kernel32.dll", "CloseHandle");

						//
						// N.B. RuntimePath is full path of the DLL to be injected
						//

						StringCchCopy(PreExecuteContext.Path, MAX_PATH, RuntimePath);

						Code = VirtualAllocEx(ProcessHandle, NULL, ApsPageSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
						WriteProcessMemory(ProcessHandle, Code, &PreExecuteContext, sizeof(PreExecuteContext), NULL);
						WriteProcessMemory(ProcessHandle, Code + sizeof(APS_PREEXECUTE_CONTEXT), 
							               ApsPreExecuteCode, sizeof(ApsPreExecuteCode), NULL);

						//
						// Load the pre-execute context address into ebx register and redirect
						// thread's pc to execute the stub code
						//

#if defined (_M_IX86)
						Context.Ebx = PtrToUlong(Code);
						Context.Eip = PtrToUlong(Code + sizeof(APS_PREEXECUTE_CONTEXT));
#elif defined(_M_X64)
						Context.Rbx = PtrToUlong(Code);
						Context.Rip = PtrToUlong(Code + sizeof(APS_PREEXECUTE_CONTEXT));
#endif

						SetThreadContext(ThreadHandle, &Context);

						//
						// allocated memory should be freed after execution
						//

						*CodePtr = (PVOID)Code;
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
	return APS_STATUS_OK;
}