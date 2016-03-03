//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _CCR_CONTENTION_H_
#define _CCR_CONTENTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "dialog.h"

HWND
CcrContentionCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	);

LRESULT
CcrContentionOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CcrContentionOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CcrContentionProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CcrContentionOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CcrContentionOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CcrContentionOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CcrContentionOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

VOID
CcrContentionInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

PCCR_LOCK_TABLE
CcrBuildLockTable(
	_In_ PPF_REPORT_HEAD Head
	);

#ifdef __cplusplus
}
#endif
#endif
