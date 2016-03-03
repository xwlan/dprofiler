//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _IO_FS_H_
#define _IO_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsprofile.h"
#include "sdk.h"
#include "dialog.h"

typedef struct _IO_FS_CONTEXT {
	int PercentSpace;
	int PercentMinimum;
} IO_FS_CONTEXT, *PIO_FS_CONTEXT;

HWND
IoFsCreate(
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
IoFsOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
IoFsProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoFsOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoFsOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
IoFsOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
IoFsOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

int CALLBACK
IoFsSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
IoFsInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

LRESULT 
IoFsOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

LRESULT
IoFsOnStackTrace(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);


#ifdef __cplusplus
}
#endif
#endif
