//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#ifndef _FULLSTACK_H_
#define _FULLSTACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
    
typedef struct _FULLSTACK_CONTEXT {
	CPU_FORM_CONTEXT Base;
    PBTR_CPU_RECORD Record;
} FULLSTACK_CONTEXT, *PFULLSTACK_CONTEXT;


VOID
FullStackDialog(
	__in HWND hWndParent,
	__in PPF_REPORT_HEAD Report,
    __in PBTR_CPU_RECORD Record 
	);

INT_PTR CALLBACK
FullStackProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FullStackOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FullStackOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FullStackOnCtlColorStatic(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FullStackOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FullStackOnExpand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FullStackOnCollapse(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FullStackOnExport(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID CALLBACK
FullStackFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

VOID
FullStackInsertData(
    __in PDIALOG_OBJECT Object,
    __in PFULLSTACK_CONTEXT Context,
    __in PTREELIST_OBJECT TreeList
    );

HTREEITEM
FullStackInsertBackTrace(
    __in PFULLSTACK_CONTEXT Context,
    __in HWND hWndTree,
    __in HTREEITEM hItemParent,
    __in PBTR_CPU_SAMPLE Sample 
    );

LRESULT
FullStackOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
FullStackCleanUp(
    __in PDIALOG_OBJECT Object
    );

#ifdef __cplusplus
}
#endif

#endif