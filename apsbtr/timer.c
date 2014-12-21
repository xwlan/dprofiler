//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "timer.h"
#include "heap.h"
#include <mmsystem.h>

LIST_ENTRY BtrTimerListHead;

//
// Require 1ms of timer resolution
//

ULONG BtrTimerResolution = 1;
BOOLEAN BtrTimerEndPeriod;

typedef MMRESULT 
(WINAPI *TIME_BEGIN_PERIOD)(
	__in UINT uPeriod
	);

typedef MMRESULT 
(WINAPI *TIME_END_PERIOD)(
	__in UINT uPeriod
	);

HMODULE WinMmHandle;
TIME_BEGIN_PERIOD TimeBeginPeriodPtr;
TIME_END_PERIOD TimeEndPeriodPtr;


VOID
BtrInitializeTimerList(
	VOID
	)
{
	MMRESULT Result;

	InitializeListHead(&BtrTimerListHead);
	WinMmHandle = LoadLibrary(L"winmm.dll");
	if (WinMmHandle) {

		//
		// Get timer resolution routines, which are wrapper of NtSetTimerResolution
		//

		TimeBeginPeriodPtr = (TIME_BEGIN_PERIOD)GetProcAddress(WinMmHandle, "timeBeginPeriod");
		TimeEndPeriodPtr = (TIME_END_PERIOD)GetProcAddress(WinMmHandle, "timeEndPeriod");

		if (TimeBeginPeriodPtr) {

			//
			// Set timer resolution
			//

			Result = (*TimeBeginPeriodPtr)(BtrTimerResolution);
			if (Result == TIMERR_NOERROR) {
				BtrTimerEndPeriod = TRUE; 
			} else {
				BtrTimerEndPeriod = FALSE; 
			}
		}
	}
}

VOID
BtrDestroyTimerList(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_TIMER Timer;

	//
	// Deactivate all timers and free them
	//

	while (!IsListEmpty(&BtrTimerListHead)) {
		ListEntry = RemoveHeadList(&BtrTimerListHead);
		Timer = CONTAINING_RECORD(ListEntry, BTR_TIMER, ListEntry);
		CancelWaitableTimer(Timer->Handle);
		CloseHandle(Timer->Handle);
		BtrFree(Timer);
	}
	
	//
	// Restore timer resolution
	//

	if (WinMmHandle) {
		if (TimeEndPeriodPtr && BtrTimerEndPeriod) {
			(*TimeEndPeriodPtr)(BtrTimerResolution);
		}
		FreeLibrary(WinMmHandle);
	}
}