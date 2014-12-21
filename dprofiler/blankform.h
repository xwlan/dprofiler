//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BLANK_FORM_H_
#define _BLANK_FORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"

HWND
BlankCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
BlankOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
BlankProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
BlankOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
BlankOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif
#endif