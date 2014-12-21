//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _MM_SUMMARY_H_
#define _MM_SUMMARY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"


HWND
MmSummaryCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	);

LRESULT
MmSummaryOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
MmSummaryOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
MmSummaryProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmSummaryOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
MmSummaryOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
MmSummaryInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

#ifdef __cplusplus
}
#endif
#endif
