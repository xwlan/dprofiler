//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#ifndef _GRAPHCTRL_H_
#define _GRAPHCTRL_H_

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GRAPH_HISTORY{
	FLOAT *Value;
	FLOAT MinimumValue;
	FLOAT MaximumValue;
	FLOAT Limits; 
    ULONG Count;
    COLORREF Color; 
} GRAPH_HISTORY, *PGRAPH_HISTORY;

typedef struct _GRAPH_CONTROL {

	HWND hWnd;
	INT_PTR CtrlId;

    COLORREF BackColor;
	COLORREF GridColor;
	COLORREF FillColor;
	COLORREF EdgeColor;

	HDC hdcGraph;
    HBITMAP hbmpGraph;
    HBITMAP hbmpGraphOld;
	
    LONG FirstPos;
    PGRAPH_HISTORY History;

    LONG Previous;
    LONG Current;
    LONG HoriStep;
    LONG VertStep;

    HPEN hMarkerPen;
    COLORREF MarkerColor;

} GRAPH_CONTROL, *PGRAPH_CONTROL;

BOOLEAN
GraphInitialize(
	VOID
	);

PGRAPH_CONTROL
GraphCreateControl(
    __in HWND hWndParent,
    __in INT_PTR CtrlId,
	__in LPRECT Rect,
	__in COLORREF crBack,
	__in COLORREF crGrid,
	__in COLORREF crFill,
	__in COLORREF crEdge
    );

VOID
GraphInitializeControl(
    __in HWND hWnd,
    __in INT_PTR CtrlId,
    __in COLORREF crBack,
    __in COLORREF crGrid,
    __in COLORREF crFill,
    __in COLORREF crEdge
    );

VOID
GraphSetHistoryData(
    __in PGRAPH_CONTROL Control,
    __in PGRAPH_HISTORY History
    );

LRESULT CALLBACK 
GraphProcedure(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

LRESULT
GraphOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
GraphOnDestroy(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
GraphOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
GraphOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

BOOLEAN
GraphInvalidate(
	__in HWND hWnd
	);

VOID
GraphDraw(
	__in PGRAPH_CONTROL Object
	);

LRESULT
GraphOnLButtonDown(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);


#ifdef __cplusplus
}
#endif

#endif