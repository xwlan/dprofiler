//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _IOWIZ_H_
#define _IOWIZ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

VOID
IoWizard(
	IN HWND hWndParent,
	IN PWIZARD_CONTEXT Context
	);

//
// CPU Page
//

INT_PTR CALLBACK
IoProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
IoOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
IoOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

//
// Attach Page
//

INT_PTR CALLBACK
IoTaskProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
IoTaskOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
IoTaskOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
IoTaskOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
IoTaskOnRefresh(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
IoTaskOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

VOID
IoInsertTask(
	IN HWND hWndParent,
	IN PLIST_ENTRY ListHead
	);

VOID
IoDeleteTask(
	IN HWND hWnd 
	);

//
// Run Page
//

INT_PTR CALLBACK
IoRunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
IoRunOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
IoRunOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
IoRunOnPath(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
IoRunOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

BOOLEAN
IoRunCheckPath(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif