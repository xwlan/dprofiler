//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _CPU_PC_H_
#define _CPU_PC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"

typedef enum _CPU_PC_ENUM {
	CpuPcNameColumn,
	CpuPcModuleColumn,
	CpuPcSampleColumn,
	CpuPcPercentColumn,
	CpuPcLineColumn,
} CPU_PC_ENUM;

typedef struct _CPU_PC_CONTEXT {
	int PercentSpace;
	int PercentMinimum;
} CPU_PC_CONTEXT, *PCPU_PC_CONTEXT;

HWND
CpuPcCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT CALLBACK 
CpuPcListProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuPcListOnHdnItemChanging(
    __in PDIALOG_OBJECT Object,
	__in LPNMHEADER lpnmhdr 
	);

LRESULT
CpuPcOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuPcOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuPcProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuPcOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CpuPcOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

int CALLBACK
CpuPcSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
CpuPcInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

LRESULT 
CpuPcOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

int
CpuPcComputePercentSpace(
	__in PCPU_PC_CONTEXT Context,
	__in HDC hdc
	);

int
CpuPcFillPercentRect(
	__in PCPU_PC_CONTEXT Context,
	__in HWND hWnd,
	__in HDC hdc,
	__in int index,
	__in LPRECT lpRect,
	__in BOOLEAN Focus
	);

#ifdef __cplusplus
}
#endif
#endif
