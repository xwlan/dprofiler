//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _WIZARD_H_
#define _WIZARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "aps.h"
#include "apsprofile.h"

typedef struct _WIZARD_CONTEXT {

	BTR_PROFILE_TYPE Type;

	union {

		struct {
			ULONG SamplingPeriod;
			ULONG StackDepth;
			BOOLEAN IncludeWait;
			BOOLEAN RecordMode;
		} Cpu;

		struct {
			ULONG EnableHeap   : 1;
			ULONG EnablePage   : 1;
			ULONG EnableHandle : 1;
			ULONG EnableGdi    : 1;
			ULONG Measure      : 1;
			ULONG Distribution : 1;
		} Mm;

		struct {
			ULONG EnableNet    : 1;
			ULONG EnableDisk   : 1;
			ULONG EnablePipe   : 1;
			ULONG EnableDevice : 1;
		} Io;
	};

	BOOLEAN InitialPause;
	BOOLEAN Attach;
	PAPS_PROCESS Process;

	WCHAR ImagePath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];

	//
	// UI State
	//

	BOOLEAN Cancel;
    HFONT hTitleFont; 

} WIZARD_CONTEXT, *PWIZARD_CONTEXT;


VOID
WizardDialog(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
WizardProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK
WizardTypeProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK
WizardCpuProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK
WizardMmProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK
WizardIoProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK
WizardAttachProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK
WizardRunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
WizardOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
WizardOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
WizardOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

#if defined (_M_IX86)

#ifndef GWL_USERDATA
#define GWL_USERDATA GWL_USERDATA
#define DWL_MSGRESULT DWL_MSGRESULT
#endif

#elif defined (_M_X64)

#ifndef GWL_USERDATA
#define GWL_USERDATA GWLP_USERDATA
#define DWL_MSGRESULT DWLP_MSGRESULT
#endif

#endif


#ifdef __cplusplus
}
#endif

#endif