//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _ATTACHDLG_H_
#define _ATTACHDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dprofiler.h"

INT_PTR
CpuAttachDialog(
	IN HWND hWndParent,
	OUT PBSP_PROCESS *Process,
	OUT PULONG Period
	);

INT_PTR CALLBACK
CpuAttachProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuAttachOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CpuAttachOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuAttachOnRefresh(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
CpuAttachInsertTask(
	IN PDIALOG_OBJECT Object,
	IN PLIST_ENTRY ListHead
	);

VOID
CpuAttachCleanUp(
	IN PDIALOG_OBJECT Object
	);

#ifdef __cplusplus
}
#endif

#endif