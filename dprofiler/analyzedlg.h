//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _ANALYZE_DLG_H_
#define _ANALYZE_DLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "apsbtr.h"
#include "apsprofile.h"

VOID
AnalyzeDialog(
	IN HWND hWndParent,
	IN BTR_PROFILE_TYPE Type,
	IN PVOID Object
	);

INT_PTR CALLBACK
AnalyzeProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
AnalyzeOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
AnalyzeOnTimer(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

//
// Analyze percent handler
//

#define WM_USER_ANALYZE (WM_USER + 12)

VOID CALLBACK
AnalyzeCallback(
	IN PVOID Context,
	IN ULONG Percent
	);

#ifdef __cplusplus
}
#endif

#endif