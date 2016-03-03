//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#ifndef _CTRLDLG_H_
#define _CTRLDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dprofiler.h"
#include "apsprofile.h"
#include "apsctl.h"
	
INT_PTR
CtrlDialog(
	IN HWND hWndParent,
	IN PAPS_PROFILE_OBJECT Profile,
	IN BOOLEAN Pause
	);

INT_PTR CALLBACK
CtrlProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CtrlOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CtrlOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CtrlOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CtrlOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CtrlOnStart(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CtrlOnTimer(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
CtrlCleanUp(
	IN PDIALOG_OBJECT Object
	);

LRESULT
CtrlOnTerminate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CtrlOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
CtrlInsertCpuCounters(
	IN HWND hWndCtrl
	);

VOID
CtrlUpdateCpuCounters(
	IN HWND hWndCtrl,
	IN PAPS_PROFILE_OBJECT Object
	);

VOID
CtrlInsertMmCounters(
	IN HWND hWndCtrl
	);

VOID
CtrlUpdateMmCounters(
	IN HWND hWndCtrl,
	IN PAPS_PROFILE_OBJECT Object
	);

VOID
CtrlInsertIoCounters(
	IN HWND hWndCtrl
	);

VOID
CtrlUpdateIoCounters(
	IN HWND hWndCtrl,
	IN PAPS_PROFILE_OBJECT Object
	);

ULONG
CtrlProfileCallback(
	__in PAPS_PROFILE_OBJECT Object,
	__in PVOID Context,
	__in ULONG Status
	);

#ifdef __cplusplus
}
#endif

#endif