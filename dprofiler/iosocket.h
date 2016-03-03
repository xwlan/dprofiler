//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _IO_SK_H_
#define _IO_SK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsprofile.h"
#include "sdk.h"
#include "dialog.h"

typedef struct _IO_SK_CONTEXT {
	int PercentSpace;
	int PercentMinimum;
} IO_SK_CONTEXT, *PIO_SK_CONTEXT;

HWND
IoSkCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
IoSkOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
IoSkOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
IoSkProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoSkOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoSkOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoSkOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
IoSkOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

int CALLBACK
IoSkSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
IoSkInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

LRESULT 
IoSkOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

LRESULT
IoSkOnStackTrace(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);


#ifdef __cplusplus
}
#endif
#endif
