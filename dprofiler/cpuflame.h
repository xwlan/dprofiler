//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _CPU_FLAME_H_
#define _CPU_FLAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "sdk.h"
#include "treelist.h"
#include "calltree.h"

HWND
CpuFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
CpuFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
CpuFlameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuFlameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
CpuFlameOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuFlameOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuFlameOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
CpuFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	);

VOID
CpuFlameSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    );


#ifdef __cplusplus
}
#endif
#endif