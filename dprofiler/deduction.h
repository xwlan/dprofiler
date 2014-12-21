//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2014
// 

#ifndef _DEDUCTION_H_
#define _DEDUCTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

VOID
DeductionDialog(
	__in HWND hWndParent	
	);

INT_PTR CALLBACK
DeductionProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
DeductionOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
DeductionOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
DeductionOnTrimming(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif