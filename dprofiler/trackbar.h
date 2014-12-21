//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#ifndef _TRACKBAR_H_
#define _TRACKBAR_H_

#include "sdk.h"
#include "apscpu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TRACK_CONTROL {

	HWND hWnd;
	INT_PTR CtrlId;

	PPF_REPORT_HEAD Report;
    PCPU_HISTORY History;

	int Height;
	int Width;
	int VertEdge;

    COLORREF BackColor;
	COLORREF GridColor;
	COLORREF FillColor;
	COLORREF EdgeColor;

	HDC hdcTrack;
    HBITMAP hbmpTrack;
    HBITMAP hbmpTrackOld;
	RECT bmpRect;

    int HoriStep;
    int VertStep;

	//
	// Cursor offset to slider
	//

	int SliderCursorEdge;
	int SliderVertEdge;
	int SliderWidth;
	int SliderHeight;

	HPEN hSliderPen;
	COLORREF SliderColor;

	RECT rcSlider;
	RECT rcOldSlider;
	HBRUSH hBrushSlider;
    BOOLEAN MouseCaptured;

    ULONG FirstSample;
    ULONG LastSample;
	double CpuMin;
	double CpuMax;
	double CpuAverage;

	LONG ValueWidth;
	LONG MergeSteps;
	double Scale;

	HWND hWndTooltip;
    BOOLEAN IsTooltipActivated;
	BOOLEAN IsTracking;

    int ScrollDelta;

} TRACK_CONTROL, *PTRACK_CONTROL;

BOOLEAN
TrackInitialize(
	VOID
	);

PTRACK_CONTROL
TrackCreateControl(
    __in HWND hWndParent,
    __in INT_PTR CtrlId,
	__in LPRECT Rect,
	__in COLORREF crBack,
	__in COLORREF crGrid,
	__in COLORREF crFill,
	__in COLORREF crEdge,
	__in COLORREF crSlider
    );

VOID
TrackInitializeControl(
    __in HWND hWnd,
    __in INT_PTR CtrlId,
    __in COLORREF crBack,
    __in COLORREF crGrid,
    __in COLORREF crFill,
    __in COLORREF crEdge,
	__in COLORREF crSlider
    );

VOID
TrackSetHistoryData(
    __in PTRACK_CONTROL Control,
    __in PCPU_HISTORY History,
	__in LONG MergeSteps
    );

LRESULT CALLBACK 
TrackProcedure(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

LRESULT
TrackOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
TrackOnDestroy(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TrackOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TrackOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TrackOnLButtonDown(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
TrackOnMouseMove(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
TrackOnMouseLeave(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

VOID
TrackSetScrollBar(
    __in HWND hWnd,
    __in int MaximumSize, 
    __in int PageSize 
    );

LRESULT
TrackOnScroll(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
TrackOnLButtonUp(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
TrackOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
TrackOnTtnGetDispInfo(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

HWND
TrackCreateTooltip(
	__in PTRACK_CONTROL Object
	);

BOOLEAN
TrackActivateTooltip(
	__in PTRACK_CONTROL Object,
    __in BOOLEAN Activate
	);

VOID
TrackSetCpuCounters(
	__in PTRACK_CONTROL Object,
	__in double CpuMin,
	__in double CpuMax,
	__in double CpuAverage
	);

//
// TrackBar Routines
//

BOOLEAN
TrackCreateBitmap(
	__in PTRACK_CONTROL Object,
	__in int Height,
	__in int VertEdge
	);

VOID
TrackDrawBitmap(
    __in PTRACK_CONTROL Object
    );

VOID
TrackDrawHistory(
    __in PTRACK_CONTROL Object,
	__in LPRECT Rect,
	__in int VertEdge
	);

BOOLEAN
TrackDrawSlider(
    __in PTRACK_CONTROL Object,
	__in LPPOINT pt
    );

VOID
TrackSetSliderSize(
    __in PTRACK_CONTROL Object,
    __in int VertEdge,
    __in int CursorEdge,
    __in int Width
    );

VOID
TrackSetSliderPosition(
	__in PTRACK_CONTROL Object,
	__in int x
	);

VOID
TrackInvalidateSliderRect(
    __in PTRACK_CONTROL Object,
	__in LPRECT Rect
    );

#ifdef __cplusplus
}
#endif

#endif