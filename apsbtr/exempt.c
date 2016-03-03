//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "hal.h"
#include "exempt.h"
#include "thread.h"
#include "util.h"
#include <mmintrin.h>

DECLSPEC_CACHEALIGN
ULONG BtrStopped = 0;

BOOLEAN BtrIgnoreSystemDll;

//
// KnownDlls and most frequently loaded dlls
//

#define BTR_NAME L"apsbtr.dll"

PWSTR BtrSystemDllName[] = {
	BTR_NAME,
	L"ntdll.dll",
	L"kernel32.dll",
	L"kernelbase.dll",
	L"user32.dll",
	L"gdi32.dll",
	L"advapi32.dll",
	L"rpcrt4.dll",
	L"ole32.dll",
};

ULONG BtrSystemDllCount = ARRAYSIZE(BtrSystemDllName);
ULONG BtrSystemDllFilledCount;

DECLSPEC_CACHEALIGN
BTR_ADDRESS BtrSystemDllAddress[ARRAYSIZE(BtrSystemDllName)];


BOOLEAN FORCEINLINE
BtrIsExemptedAddress(
	IN ULONG_PTR Address 
	)
{
	ULONG i;

	//
	// If it's runtime address, exempt it
	//

	if ((ULONG_PTR)Address >= BtrDllBase && (ULONG_PTR)Address < BtrDllBase + BtrDllSize) {
		return TRUE;
	}
	
	if (BtrIgnoreSystemDll) {
		
#if defined (_M_IX86)
		_mm_prefetch((const char *)BtrSystemDllAddress, _MM_HINT_T0);
#elif defined (_M_X64)
		_mm_prefetch((const char *)BtrSystemDllAddress, _MM_HINT_T0);
		_mm_prefetch((const char *)BtrSystemDllAddress + SYSTEM_CACHE_ALIGNMENT_SIZE, _MM_HINT_T0);
#endif

		for(i = 0; i < BtrSystemDllFilledCount; i++) {
			if (Address >= BtrSystemDllAddress[i].StartVa && Address < BtrSystemDllAddress[i].EndVa) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

PBTR_THREAD_OBJECT
BtrIsExemptedCall(
	IN ULONG_PTR Caller 
	)
{
	PBTR_THREAD_OBJECT Thread;

	if (BtrIsExemptedAddress(Caller)) {
		return NULL;
	}

	if (BtrIsRuntimeThread(BtrCurrentThreadId())) {
		return NULL;
	}

	if (BtrIsExemptionCookieSet(GOLDEN_RATIO)) {
		return NULL;
	}

	if (BtrIsStopped() || !BtrIsAcspValid()) {
		return NULL;
	}

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);

	if (!Thread) {
		__debugbreak();
	}

	if (FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		return NULL;
	}

	return Thread;
}

PBTR_THREAD_OBJECT
BtrIsExemptedCallLite(
	IN ULONG_PTR Caller 
	)
{
	PBTR_THREAD_OBJECT Thread;

	if (BtrIsExemptedAddress(Caller)) {
		return NULL;
	}

	if (BtrIsRuntimeThread(BtrCurrentThreadId())) {
		return NULL;
	}

	if (BtrIsExemptionCookieSet(GOLDEN_RATIO)) {
		return NULL;
	}

	if (BtrIsStopped() || !BtrIsAcspValid()) {
		return NULL;
	}

	Thread = BtrGetCurrentThread();
	return Thread;
}

BOOLEAN 
BtrIsSystemDllAddress(
	IN PVOID Address
	)
{
	ULONG i;
	ULONG_PTR Value;

	Value = (ULONG_PTR)Address;

	for(i = 0; i < BtrSystemDllFilledCount; i++) {
		if (Value >= BtrSystemDllAddress[i].StartVa && Value < BtrSystemDllAddress[i].EndVa) {
			return TRUE;
		}
	}

	return FALSE;
}

VOID
BtrInitSystemDllAddress(
	VOID
	)
{
	ULONG i, j;
	HMODULE DllHandle;
	MODULEINFO Info;
	BOOL Status;

	for(i = 0, j = 0; i < BtrSystemDllCount; i++) {

		DllHandle = GetModuleHandleW(BtrSystemDllName[i]);
		if (DllHandle != NULL) {

			Status = GetModuleInformation(GetCurrentProcess(), DllHandle, &Info, sizeof(Info));
			if (Status) {
				BtrSystemDllAddress[j].StartVa = (ULONG_PTR)Info.lpBaseOfDll;
				BtrSystemDllAddress[j].EndVa = (ULONG_PTR)Info.lpBaseOfDll + (ULONG_PTR)Info.SizeOfImage;
				j += 1;
			}
		}
	}

	BtrSystemDllFilledCount = j;
}

VOID
BtrEnterExemptionRegion(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)){
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}

VOID
BtrLeaveExemptionRegion(
	_In_ PBTR_THREAD_OBJECT Thread
	)
{
	if (FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)){
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}
