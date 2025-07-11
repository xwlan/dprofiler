//
// xwlan@outlook.com
// Copyright(C) 2025
//

#ifndef _CPU_ON_FLAME_H_
#define _CPU_ON_FLAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "sdk.h"
#include "dialog.h"
#include "treelist.h"
#include "calltree.h"

HWND
CpuOnFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR CtrlId
	);

LRESULT
CpuOnFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuOnFlameOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuOnFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuOnFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuOnFlameProcedure(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuOnFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object,
	__in LPNMHDR lpnmhdr
	);

LRESULT
CpuOnFlameOnDrawItem(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuOnFlameOnQueryNode(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuOnFlameOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
CpuOnFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
CpuOnFlameSetGraph(
	__in PDIALOG_OBJECT Object,
	__in PCALL_GRAPH Graph
	);


#ifdef __cplusplus
}
#endif
#endif
