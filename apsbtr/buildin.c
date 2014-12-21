//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "buildin.h"
#include "apsprofile.h"
#include "util.h"

BTR_CALLBACK BuildinCallback[BUILDIN_CALLBACK_COUNT] = {
	{ BuildinCallbackType, 0,0,0,0, ExitProcessEnter, "kernel32.dll", "ExitProcess", 0 },
};

ULONG BuildinCallbackCount = ARRAYSIZE(BuildinCallback);

PBTR_CALLBACK FORCEINLINE 
BtrGetBuildinCallback(
	IN ULONG Ordinal
	)
{
	return &BuildinCallback[Ordinal];
}

ULONG
BtrRegisterBuildinCallbacks(
	VOID
	)
{
	ULONG Number;
	PBTR_CALLBACK Callback;
	HMODULE DllHandle;
	ULONG Count;
	ULONG Status;

	Count = 0;

	for(Number = 0; Number < BUILDIN_CALLBACK_COUNT; Number += 1) {

		Callback = &BuildinCallback[Number];
		DllHandle = GetModuleHandleA(Callback->DllName);

		if (!DllHandle) {
			return BTR_E_LOADLIBRARY; 
		}

		Callback->Address = GetProcAddress(DllHandle, Callback->ApiName);

		if (Callback->Address) {

			Status = BtrRegisterCallbackEx(Callback);
			if (Status == S_OK) {
				Count += 1;
			}
		}
	}

	return Count;
}

ULONG
BtrUnregisterBuildinCallbacks(
	VOID
	)
{
	ULONG Number;
	PBTR_CALLBACK Callback;

	for(Number = 0; Number < BUILDIN_CALLBACK_COUNT; Number += 1) {
		Callback = &BuildinCallback[Number];
		BtrUnregisterCallbackEx(Callback);
	}

	return S_OK;
}

VOID WINAPI 
ExitProcessEnter(
	__in UINT ExitCode
	)
{
	ULONG_PTR Value;
	ULONG Current;
	static ULONG Concurrency = 0;

	BtrSetExemptionCookie(GOLDEN_RATIO, &Value);

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
		//
		// N.B. The following code won't enter runtime anymore since all
		// callbacks are unregistered by control end.
		//

		ExitProcess(ExitCode);

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
}