//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#ifndef _HELP_H_
#define _HELP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

INT_PTR
HelpDialog(
	__in HWND hWndParent	
	);

INT_PTR CALLBACK
HelpProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
HelpOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

DWORD CALLBACK
HelpStreamCallback(
	__in DWORD_PTR dwCookie,
    __in LPBYTE pbBuff,
    __in LONG cb,
    __out LONG *pcb
	);

#ifdef __cplusplus
}
#endif

#endif