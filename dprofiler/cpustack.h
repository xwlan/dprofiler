//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _CPU_STACK_H_
#define _CPU_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"

HWND
CpuStackCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
CpuStackOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuStackHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuStackOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuStackOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuStackProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuStackOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuStackOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CpuStackOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

LRESULT
CpuStackOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

VOID
CpuStackInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

HTREEITEM
CpuStackInsertPc(
	__in HWND hWndTree,
	__in HTREEITEM hItemDll,
	__in PBTR_DLL_ENTRY DlLEntry,
	__in PBTR_TEXT_TABLE TextTable
	);

VOID CALLBACK
CpuStackFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

int CALLBACK
CpuStackCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

LRESULT
CpuStackOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif
#endif
