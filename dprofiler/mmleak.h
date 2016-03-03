//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _MM_LEAK_H_
#define _MM_LEAK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "dialog.h"

HWND
MmLeakCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
MmLeakOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
MmLeakOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmLeakOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmLeakOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmLeakOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
MmLeakProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmLeakOnTreeListSelChanged(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmLeakOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
MmLeakSortHeapEntry(
    __in PTREELIST_OBJECT TreeList,
    __in HTREEITEM Parent,
    __in ULONG Column
    );

VOID CALLBACK
MmLeakFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

int CALLBACK
MmLeakCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
MmLeakInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
MmLeakInsertHeapData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	);

HTREEITEM
MmLeakInsertPerHeapData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem,
	__in PBTR_LEAK_FILE File
	);

VOID
MmLeakInsertPageData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	);

VOID
MmLeakInsertHandleData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	);

VOID
MmLeakInsertGdiData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	);

VOID
MmLeakInsertBackTrace(
	__in HWND hWnd,
	__in PBTR_LEAK_CONTEXT Context
	);

LRESULT 
MmLeakOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

#ifdef __cplusplus
}
#endif
#endif