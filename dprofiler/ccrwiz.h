//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2015
// 

#ifndef _CCRWIZ_H_
#define _CCRWIZ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "wizard.h"

VOID
CcrWizard(
	IN HWND hWndParent,
	IN PWIZARD_CONTEXT Context
	);

INT_PTR CALLBACK
CcrProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CcrOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CcrOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

//
// Attach Page
//

INT_PTR CALLBACK
CcrTaskProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CcrTaskOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CcrTaskOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CcrTaskOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CcrTaskOnRefresh(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
CcrTaskOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

VOID
CcrInsertTask(
	IN HWND hWndParent,
	IN PLIST_ENTRY ListHead
	);

VOID
CcrDeleteTask(
	IN HWND hWnd 
	);

//
// Run Page
//

INT_PTR CALLBACK
CcrRunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CcrRunOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CcrRunOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CcrRunOnPath(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
CcrRunOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

BOOLEAN
CcrRunCheckPath(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif