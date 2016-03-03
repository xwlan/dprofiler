//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _CCR_SUMMARY_H_
#define _CCR_SUMMARY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"


HWND
CcrSummaryCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	);

LRESULT
CcrSummaryOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CcrSummaryOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CcrSummaryProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CcrSummaryOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CcrSummaryOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
CcrSummaryInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

#ifdef __cplusplus
}
#endif
#endif
