//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#ifndef _CPUWIZ_H_
#define _CPUWIZ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

VOID
CpuWizard(
	IN HWND hWndParent,
	IN PWIZARD_CONTEXT Context
	);

//
// CPU Page
//

INT_PTR CALLBACK
CpuProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CpuOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

//
// Attach Page
//

INT_PTR CALLBACK
CpuTaskProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuTaskOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CpuTaskOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuTaskOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuTaskOnRefresh(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
CpuTaskOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

VOID
CpuInsertTask(
	IN HWND hWndParent,
	IN PLIST_ENTRY ListHead
	);

VOID
CpuDeleteTask(
	IN HWND hWnd 
	);

//
// Run Page
//

INT_PTR CALLBACK
CpuRunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuRunOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CpuRunOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CpuRunOnPath(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
CpuRunOnFinish(
	IN HWND hWnd,
	IN PWIZARD_CONTEXT Context
	);

BOOLEAN
CpuRunCheckPath(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif