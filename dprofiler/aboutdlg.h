//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#ifndef _ABOUTDLG_H_
#define _ABOUTDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

VOID
AboutDialog(
	__in HWND hWndParent	
	);

INT_PTR CALLBACK
AboutProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
AboutOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
AboutOnCtlColorStatic(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
AboutOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif