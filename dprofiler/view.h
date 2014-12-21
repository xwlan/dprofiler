//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _VIEW_H_
#define _VIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

typedef struct _VIEW_CHILD {
	LIST_ENTRY ListEntry;
	HWND hWnd;
	UINT_PTR Id;
	PVOID Context;
} VIEW_CHILD, *PVIEW_CHILD;

typedef struct _VIEW_OBJECT {
	HWND hWnd;
	LIST_ENTRY ChildListHead;
} VIEW_OBJECT, *PVIEW_OBJECT;

BOOLEAN
ViewInitialize(
	VOID
	);

LRESULT
ViewOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT 
ViewOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
ViewOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
ViewOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
ViewOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
ViewOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK
ViewProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif
#endif