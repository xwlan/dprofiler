//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _CPU_MODULE_H_
#define _CPU_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"

HWND
CpuModuleCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
CpuModuleOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuModuleHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuModuleOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuModuleOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuModuleProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuModuleOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuModuleOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CpuModuleOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

LRESULT
CpuModuleOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

VOID
CpuModuleInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

HTREEITEM
CpuModuleInsertPc(
	__in HWND hWndTree,
	__in HTREEITEM hItemDll,
	__in PBTR_DLL_ENTRY DlLEntry,
	__in PBTR_TEXT_TABLE TextTable
	);

VOID CALLBACK
CpuModuleFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

int CALLBACK
CpuModuleCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

LRESULT
CpuModuleOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif
#endif
