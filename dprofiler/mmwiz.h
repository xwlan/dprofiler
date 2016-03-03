//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#ifndef _MMWIZ_H_
#define _MMWIZ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

VOID
MmWizard(
	IN HWND hWndParent,
	IN PWIZARD_CONTEXT Context
	);

//
// CPU Page
//

INT_PTR CALLBACK
MmProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
MmOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
MmOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

//
// Attach Page
//

INT_PTR CALLBACK
MmTaskProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
MmTaskOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
MmTaskOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
MmTaskOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
MmTaskOnRefresh(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
MmTaskOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

VOID
MmInsertTask(
	IN HWND hWndParent,
	IN PLIST_ENTRY ListHead
	);

VOID
MmDeleteTask(
	IN HWND hWnd 
	);

//
// Run Page
//

INT_PTR CALLBACK
MmRunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
MmRunOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
MmRunOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
MmRunOnPath(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
MmRunOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

BOOLEAN
MmRunCheckPath(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif