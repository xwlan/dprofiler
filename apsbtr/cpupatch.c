#pragma once
#include "iatpatch.h"
#include "btr.h"
#include "apsbtr.h"
#include "util.h"
#include "cpuprof.h"
#include "thread.h"
#include "exempt.h"

BTR_IAT_PATCH CpuIatPatch[] = {
	{ "kernel32.dll", "ExitProcess", CpuIatExitProcess, NULL, NULL, 0 },
};

int CpuIatPatchCount = ARRAYSIZE(CpuIatPatch);


VOID WINAPI
CpuIatExitProcess(
	IN UINT ExitCode
	)
{
	ULONG Current;
	static ULONG Concurrency = 0;
	PBTR_THREAD_OBJECT Thread;

	Thread = BtrGetCurrentThread();
	BtrEnterExemptionRegion(Thread);
	
	Current = InterlockedCompareExchange(&Concurrency, 1, 0);
	if (Current != 0) {

		//
		// Another thread acquires lock to call ExitProcess
		//

		return;
	}

	__try {

		BtrProfileObject->ExitStatus = BTR_S_EXITPROCESS;
		SignalObjectAndWait(BtrProfileObject->ExitProcessEvent,
			BtrProfileObject->ExitProcessAckEvent,
			INFINITE, FALSE);

		ExitProcess(ExitCode);

	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}
}

VOID
CpuInitializeIatPatch(
	VOID
	)
{
	PBTR_MODULE Module;
	Module = BtrGetModule(L"kernel32.dll");
	CpuIatPatch[0].Address = GetProcAddress(Module->Base, "ExitProcess");
}

VOID
CpuIatApplyPatch(
	VOID
	)
{
	CpuInitializeIatPatch();
	BtrApplyIatPatch(CpuIatPatch, CpuIatPatchCount, TRUE);
}

VOID
CpuDllLoadCallback(
	IN struct _BTR_PROFILE_OBJECT* Object,
	IN struct _BTR_MODULE* Dll,
	IN BOOLEAN Load
	)
{
	if (Load) {
		BtrApplyPatchForDllByAddress(Dll, CpuIatPatch, CpuIatPatchCount, TRUE);
		return;
	} 
}