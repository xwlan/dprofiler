//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _CPU_PC_STACK_H_
#define _CPU_PC_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"
#include "apsbtr.h"
#include "apsprofile.h"


typedef struct _CPU_PCSTACK_CONTEXT {
	PCPU_PC_ENTRY Pc;
	ULONG ThreadId;
	PCPU_PC_STACKTRACE Stack;
} CPU_PCSTACK_CONTEXT, *PCPU_PCSTACK_CONTEXT;

HWND
CpuPcStackCreate(
	IN HWND hWndParent,
	IN ULONG CtrlId,
	IN PPF_REPORT_HEAD Head,
	IN PCPU_PC_ENTRY Pc,
	IN ULONG ThreadId
	);

LRESULT
CpuPcStackOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuPcStackHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuPcStackOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuPcStackOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuPcStackProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuPcStackOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuPcStackOnExport(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuPcStackOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuPcStackOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CpuPcStackOnColumnClick(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

LRESULT 
CpuPcStackOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

int CALLBACK
CpuPcStackSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
CpuPcStackInsertPcCount(
	IN HWND hWnd,
	IN PPF_REPORT_HEAD Head
	);

VOID
CpuPcStackInsertBackTrace(
	__in HWND hWnd,
    __in int Index 
	);

#ifdef __cplusplus
}
#endif
#endif
