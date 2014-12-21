//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _CPU_THREAD_H_
#define _CPU_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "apscpu.h"


HWND
CpuThreadCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
CpuThreadOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuThreadHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuThreadOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuThreadOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuThreadProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuThreadOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuThreadOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CpuThreadOnColumnClick(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

LRESULT 
CpuThreadOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

int CALLBACK
CpuThreadSortPcCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

int CALLBACK
CpuThreadSortThreadCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
CpuThreadInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

VOID
CpuTranslateThreadPriority(
    __out PWCHAR Buffer,
    __in SIZE_T Length,
    __in ULONG Priority
    );

LRESULT 
CpuThreadOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

PCPU_THREAD_TABLE
CpuThreadGetTable(
    __in HWND hWnd
    );

PCPU_THREAD
CpuThreadGetMostBusyThread(
    __in HWND hWnd
    );

#ifdef __cplusplus
}
#endif
#endif
