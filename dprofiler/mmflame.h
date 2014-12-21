//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _MM_FLAME_H_
#define _MM_FLAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "sdk.h"
#include "treelist.h"
#include "calltree.h"

HWND
MmFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
MmFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
MmFlameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
MmFlameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
MmFlameOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmFlameOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmFlameOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
MmFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
MmFlameSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    );


#ifdef __cplusplus
}
#endif
#endif