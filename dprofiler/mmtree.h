//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _MM_TREE_H_
#define _MM_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "sdk.h"
#include "treelist.h"
#include "calltree.h"

typedef struct _MM_TREE_CONTEXT {
    PPF_REPORT_HEAD Head;
    PCALL_GRAPH Graph;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_LINE_ENTRY LineEntry;
    PBTR_TEXT_TABLE TextTable;
} MM_TREE_CONTEXT, *PMM_TREE_CONTEXT;


HWND
MmTreeCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
MmTreeOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
MmTreeOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmTreeOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmTreeOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
MmTreeProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmTreeOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
MmTreeOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmTreeOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmTreeOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID CALLBACK
MmTreeFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

VOID
MmTreeInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
MmTreeInsertTree(
    __in HWND hWndTree,
    __in PPF_REPORT_HEAD Head,
    __in PCALL_GRAPH Graph,
	__in PTREELIST_OBJECT TreeList
    );

HTREEITEM
MmTreeInsertNode(
    __in HWND hWndTree,
    __in HTREEITEM ParentItem,
    __in PCALL_NODE Node,
	__in PMM_TREE_CONTEXT Context
    );

VOID
MmTreeOnDeduction(
	__in HWND hWnd 
	);

#ifdef __cplusplus
}
#endif
#endif