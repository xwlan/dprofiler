//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_HEAP_H_
#define _MM_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"

HWND
MmHeapCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
MmHeapOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
MmHeapOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmHeapOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmHeapOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
MmHeapProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmHeapOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmHeapOnTreeListSelChanged(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID CALLBACK
MmHeapFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

int CALLBACK
MmHeapCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
MmHeapInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

HTREEITEM
MmHeapInsertBackTrace(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem,
	__in PBTR_STACK_RECORD Record
	);

VOID
MmHeapInsertBackTraceEx(
	__in HWND hWnd,
	__in HWND hWndTree,
	__in HTREEITEM hTreeItem
	);

#ifdef __cplusplus
}
#endif
#endif