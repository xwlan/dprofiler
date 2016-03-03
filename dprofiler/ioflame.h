//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _IO_FLAME_H_
#define _IO_FLAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "sdk.h"
#include "treelist.h"
#include "calltree.h"
#include "dialog.h"

HWND
IoFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
IoFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
IoFlameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
IoFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
IoFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
IoFlameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
IoFlameOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoFlameOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoFlameOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
IoFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
IoFlameSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    );


#ifdef __cplusplus
}
#endif
#endif