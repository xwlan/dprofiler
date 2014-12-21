//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _CPU_FUNC_H_
#define _CPU_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"

HWND
CpuFuncCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
CpuFuncOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuFuncHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuFuncOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuFuncOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuFuncProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuFuncOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuFuncOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuFuncOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT 
CpuFuncOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

LRESULT 
CpuFuncOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

int CALLBACK
CpuFuncSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
CpuFuncInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

#ifdef __cplusplus
}
#endif
#endif
