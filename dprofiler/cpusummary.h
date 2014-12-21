//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _CPU_SUMMARY_H_
#define _CPU_SUMMARY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"


HWND
CpuSummaryCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	);

LRESULT
CpuSummaryOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuSummaryOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuSummaryProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuSummaryOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuSummaryOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
CpuSummaryInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

#ifdef __cplusplus
}
#endif
#endif
