//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#ifndef _IOVIEW_H_
#define _IOVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"

HWND
IoFormCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	);

LRESULT
IoViewOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
IoInsertData(
	__in HWND hWnd
	);

LRESULT 
IoOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
IoOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
IoOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
IoViewProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID CALLBACK
IoFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	);

int CALLBACK
IoCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

LRESULT
IoOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif
#endif
