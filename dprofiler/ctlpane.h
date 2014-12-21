//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _CTL_PANE_H_
#define _CTL_PANE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprofiler.h"
#include "apsctl.h"
#include "frame.h"

//
// N.B. If any error occurs, the pane is switched backed
// to bootstrap state automatically after prompting error
// to user
//

typedef enum _CTL_PANE_STATE {
	CTL_PANE_BOOTSTRAP,
	CTL_PANE_INIT_PAUSED,
	CTL_PANE_PROFILING,
	CTL_PANE_PAUSED,
    CTL_PANE_COMPLETED, 
	CTL_PANE_ANALYZING,
    CTL_PANE_TERMINATED,
    CTL_PANE_RESUME_FAILED,
	CTL_PANE_FAILED,
} CTL_PANE_STATE;


HWND
CtlPaneCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
CtlPaneOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CtlPaneOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CtlPaneOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CtlPaneProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnCtlColorStatic(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
CtlPaneOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

ULONG
CtlPaneProfileCallback(
	__in PAPS_PROFILE_OBJECT Object,
	__in PVOID Context,
	__in ULONG Status
	);

ULONG
CtlPaneAnalysisCallback(
    __in PVOID Argument
    );

ULONG
CtlPaneOpenCallback(
    __in PVOID Argument
    );

LRESULT CALLBACK 
CtlPaneStaticProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT 
CtlPaneStaticOnPaint(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CtlPaneStaticOnSize(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

CTL_PANE_STATE
CtlPaneGetCurrentState(
	__in HWND hWnd
	);

VOID
CtlPaneDrawBootstrap(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawProfiling(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawPaused(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawCompleted(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawAnalyzing(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawTerminated(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawResumeFailed(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneDrawFailed(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneShowAnimation(
	__in PDIALOG_OBJECT Object,
	__in BOOLEAN Show
	);

VOID
CtlPaneStartAnimation(
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneStopAnimation(
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneRedraw(
	__in PDIALOG_OBJECT Object
	);

VOID
CtlPaneStartProfile(
	__in HWND hWndCtlPane
	);

VOID
CtlPaneStartProfilePaused(
	__in HWND hWndCtlPane
	);

LRESULT
CtlPaneOnProfileStart(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnProfileTerminate(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnProfileAnalyzed(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnProfileOpened(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

ULONG
CtlPaneEnterAnalyzing(
    __in PFRAME_OBJECT Object
    );

ULONG
CtlPaneEnterReporting(
    __in PFRAME_OBJECT Object
    );

LRESULT
CtlPaneOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);


LRESULT
CtlPaneOnPauseResume(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnStop(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CtlPaneOnMark(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
CtlPaneSetButtonState(
	__in PDIALOG_OBJECT Object
	);

ULONG
CtlPaneCreateProfileByAttach(
	__in HWND hWndCtlPane,
	__in PWIZARD_CONTEXT Wizard,
    __out PWSTR ReportPath
	);

ULONG
CtlPaneCreateProfileByLaunch(
	__in HWND hWndCtlPane,
	__in PWIZARD_CONTEXT Wizard,
    __out PWSTR ReportPath
	);

VOID
CtlPaneGetProfileAttributes(
    __in HWND hWnd,
    __out BTR_PROFILE_TYPE *Type,
    __out PWCHAR ReportPath,
    __in SIZE_T Length
    );

BOOLEAN
CtlPaneIsAutoAnalyze(
	__in PDIALOG_OBJECT Object
	);

ULONG
CtlPaneRunApplication(
	__in PWSTR ImagePath,
	__in PWSTR Argument,
	__in PWSTR WorkPath,
	__in BOOLEAN Suspend,
	__out PHANDLE ProcessHandle,
	__out PHANDLE ThreadHandle,
	__out PULONG ProcessId,
	__out PULONG ThreadId
	);

ULONG
CtlPaneOpenReport(
	__in HWND hWnd,
	__in PWSTR ReportPath
	);

ULONG
CtlPaneOnAnalyze(
	__in HWND hWnd
	);


#ifdef __cplusplus
}
#endif
#endif
