//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _STATECOLOR_H_
#define _STATECOLOR_H_

#include "sdk.h"
#include "apsprofile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _STATECOLOR_ENTRY {
	BTR_CPU_STATE_TYPE Type;
	COLORREF Color;
    PWSTR Description;
} STATECOLOR_ENTRY, *PSTATECOLOR_ENTRY;

typedef struct _STATECOLOR_CONTROL {

	HWND hWnd;
	STATECOLOR_ENTRY Entry[CPU_STATE_COUNT];

	HWND hWndTooltip;
    BOOLEAN IsTooltipActivated;

} STATECOLOR_CONTROL, *PSTATECOLOR_CONTROL;

BOOLEAN 
StateColorInitialize(
	VOID
	);

HWND 
StateColorCreateControl(
    __in HWND hWndParent,
    __in INT_PTR Id
    );

HWND
StateColorCreateTooltip(
	__in PSTATECOLOR_CONTROL Object
	);

BOOLEAN
StateColorActivateTooltip(
	__in PSTATECOLOR_CONTROL Object,
    __in BOOLEAN Activate
	);

LRESULT
StateColorOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
StateColorOnMouseMove(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
StateColorOnMouseLeave(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
StateColorOnLButtonDown(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
StateColorOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT CALLBACK 
StateColorProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

LRESULT
StateColorOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
StateColorOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

int
StateColorHitTest(
    __in LPPOINT pt
    );

#ifdef __cplusplus
}
#endif
#endif
