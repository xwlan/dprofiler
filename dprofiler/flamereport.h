//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _FLAMEREPORT_H_
#define _FLAMEREPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "calltree.h"

VOID
FlameReportCreate(
	__in HWND hWndParent,
    __in PPF_REPORT_HEAD Head,
    __in PBTR_STACK_RECORD Record,
    __in ULONG First,
    __in ULONG Last
	);

LRESULT
FlameReportOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
FlameReportOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FlameReportOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FlameReportOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
FlameReportProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FlameReportOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
FlameReportOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FlameReportOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
FlameReportOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
FlameReportInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
FlameReportSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    );


#ifdef __cplusplus
}
#endif
#endif