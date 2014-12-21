//
// Apsara Labs
// lan.john@gmail.com
// Copyrigth(C) 2009-2013
//

#ifndef _FRAME_H_
#define _FRAME_H_

#include "sdk.h"
#include "rebar.h"
#include "statusbar.h"
#include "splitbar.h"
#include "apsprofile.h"
#include "profileform.h"
#include "wizard.h"
#include "apsctl.h"
#include "find.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// State to facilitate form management
//

typedef enum _FRAME_STATE {
    FRAME_BOOTSTRAP,
    FRAME_PROFILING,
    FRAME_ANALYZING,
    FRAME_REPORTING,
    FRAME_TERMINATED,
} FRAME_STATE;

typedef struct _FRAME_OBJECT {

	HWND hWnd;
	REBAR_OBJECT *Rebar;
	HWND hWndStatusBar;

    FRAME_STATE State;

    HWND hWndCtlPane;
    BOOLEAN ShowCtlPane;
    HWND hWndStartButton;
    HWND hWndStopButton;
    HWND hWndMarkButton;

	HWND hWndCurrent;
	CPU_FORM CpuForm;
	MM_FORM MmForm;

	BTR_PROFILE_TYPE Type;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	WCHAR FilePath[MAX_PATH];

	PPF_REPORT_HEAD Head;
	FIND_CONTEXT FindContext;

	PDIALOG_OBJECT SourceForm;

} FRAME_OBJECT, *PFRAME_OBJECT;

typedef enum _FRAME_CHILD_ID {
	FrameRebarId     = 10,
	FrameStatusBarId = 20,
	FrameChildViewId = 30,
	FrameChildLeakId = 31,
	FrameTreeViewId  = 40,
    FrameCtlPaneId = 60,
} FRAME_CHILD_ID;

BOOLEAN
FrameRegisterClass(
	VOID
	);

HWND 
FrameCreate(
	__in PFRAME_OBJECT Object 
	);

BOOLEAN
FrameLoadBars(
	__in PFRAME_OBJECT Object
	);

VOID
FrameGetViewRect(
	__in PFRAME_OBJECT Object,
	__out RECT *rcView
	);

LRESULT
FrameOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnSwitchView(
	__in HWND hWnd,
	__in HWND hWndCombo
	);

LRESULT 
FrameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnWmUserStatus(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK
FrameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FrameOnCpu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnUserProfileStarted(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnUserProfileAnalyzed(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnUserProfileOpened(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnUserProfileCompleted(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnUserProfileTerminated(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnMm(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnIo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnOpen(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnSave(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnOption(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnAnalyze(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

BOOLEAN
FrameOpenReport(
	__in PFRAME_OBJECT Object,
	__in PWSTR FilePath
	);

VOID
FrameCloseReport(
	__in PFRAME_OBJECT Object
	);

VOID
FrameCleanForm(
	__in PFRAME_OBJECT Object
	);

VOID
FrameGetPaneRect(
	__in PFRAME_OBJECT Object,
	__out RECT *rcView
	);

VOID
FrameShowCtlPane(
	__in PFRAME_OBJECT Object,
	__in BOOLEAN Show
	);

VOID
FrameResizeCtlPane(
	__in PFRAME_OBJECT Object
	);

VOID
FrameSetMenuState(
    __in PFRAME_OBJECT Object
    );

LRESULT
FrameOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FrameOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnFindForward(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnFindBackward(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnDeduction(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
FrameRetrieveFindString(
	__in PFRAME_OBJECT Object,
	__out PWCHAR Buffer,
	__in ULONG Length
	);

LRESULT
FrameOnDeductionUpdate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
FrameShowSource(
	__in HWND hWndFrom,
	__in PWSTR FullPath,
	__in ULONG Line
	);

LRESULT
FrameOnSourceClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FrameOnHelp(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif