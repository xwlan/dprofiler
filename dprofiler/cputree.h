//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2016
//

#ifndef _CPU_TREE_H_
#define _CPU_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "calltree.h"

typedef struct _CPU_TREE_CONTEXT {
    PPF_REPORT_HEAD Head;
    PCALL_GRAPH Graph;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_LINE_ENTRY LineEntry;
    PBTR_TEXT_TABLE TextTable;
} CPU_TREE_CONTEXT, *PCPU_TREE_CONTEXT;

HWND
CpuTreeCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
CpuTreeOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuTreeListProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT 
CpuTreeOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuTreeOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuTreeOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuTreeProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuTreeOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
CpuTreeOnDblclk(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuTreeOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuTreeOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuTreeOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID CALLBACK
CpuTreeFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

VOID
CpuTreeInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
CpuTreeInsertTree(
    __in HWND hWndTree,
    __in PPF_REPORT_HEAD Head,
    __in PCALL_GRAPH Graph,
	__in PTREELIST_OBJECT TreeList
    );

HTREEITEM
CpuTreeInsertNode(
    __in HWND hWndTree,
    __in HTREEITEM ParentItem,
    __in PCALL_NODE Node,
	__in PCPU_TREE_CONTEXT Context
    );

VOID
CpuTreeOnDeduction(
	__in HWND hWnd 
	);

PCPU_THREAD_TABLE
CpuTreeCreateThreadedGraph(
    VOID
    );

#ifdef __cplusplus
}
#endif
#endif
