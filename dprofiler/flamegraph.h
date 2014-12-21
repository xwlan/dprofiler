//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#ifndef _FLAME_GRAPH_H_
#define _FLAME_GRAPH_H_

#include "sdk.h"
#include "calltree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FLAME_RECT {
    RECT Rect;
    COLORREF Color;
} FLAME_RECT, *PFLAME_RECT;

typedef struct _FLAME_NODE {
	LIST_ENTRY ListEntry;
	PVOID Node;
    RECT Rect;
    COLORREF Color;
	ULONG Level;
} FLAME_NODE, *PFLAME_NODE;

typedef struct _FLAME_LEVEL {
	LIST_ENTRY ListHead;
	ULONG Count;
} FLAME_LEVEL, *PFLAME_LEVEL;

typedef struct _FLAME_STACK {
	ULONG Depth; 
	FLAME_LEVEL Level[MAX_STACK_DEPTH];
} FLAME_STACK, *PFLAME_STACK;

//
// Parent window of flamegraph must be prepared to 
// handle WM_FLAME_QUERYNODE notification, the lp
// is a pointer to NM_FLAME_QUERYNODE structure,
// parent should fill the Text field if there's
// any display information to show in flamegraph
//

#define FLAME_QUERY_SYMBOL  0x1
#define FLAME_QUERY_PERCENT 0x2
#define FLAME_QUERY_ALL     (FLAME_QUERY_SYMBOL|FLAME_QUERY_PERCENT)

typedef struct _NM_FLAME_QUERYNODE {
    ULONG Flags;
    PCALL_NODE Node;
    WCHAR Text[MAX_PATH];
} NM_FLAME_QUERYNODE, *PNM_FLAME_QUERYNODE; 

typedef struct _FLAME_CONTROL {

	HWND hWnd;
	INT_PTR CtrlId;

	int TitleHeight;
	int ViewHoriWidth;  // in pixel
	int ViewHoriEdge;   // in pixel
	int ViewVertEdge;   // in row
	int NodeHoriEdge;   // in pixel
	int MaximumDepth;   // in row
	int MinimumWidth;
	SIZE EmptyLimit;    // width of 'M'
	RECT ValidRect;

	PFLAME_STACK Flame; 
    PCALL_GRAPH Graph;

    int Width;
    int Height;
    int Linedx;
    int Linedy;

    COLORREF FirstColor;
    COLORREF LastColor;
    COLORREF BackColor;
    COLORREF EdgeColor;
    LONG HoriMargin;
    LONG VertMargin;

	HDC hdcFlame;
    HBITMAP hbmpFlame;
    HBITMAP hbmpFlameOld;
    RECT bmpRect;
	LONG FrameHeight;
    HFONT hNormalFont;
    HFONT hBoldFont;
    HFONT hOldFont;

	HWND hWndTooltip;
	BOOLEAN IsTooltipActivated;
	PFLAME_NODE MouseNode;

} FLAME_CONTROL, *PFLAME_CONTROL;

BOOLEAN
FlameInitialize(
	VOID
	);

PFLAME_CONTROL
FlameCreateControl(
    __in HWND hWndParent,
    __in INT_PTR CtrlId,
	__in LPRECT Rect,
	__in COLORREF crFirst,
	__in COLORREF crLast,
	__in COLORREF crBack,
	__in COLORREF crEdge
    );

PFLAME_CONTROL
FlameInitializeControl(
    __in HWND hWnd,
    __in INT_PTR CtrlId,
    __in COLORREF crFirst,
	__in COLORREF crLast,
	__in COLORREF crBack,
	__in COLORREF crEdge
    );

LRESULT CALLBACK 
FlameProcedure(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

LRESULT
FlameOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
FlameOnDestroy(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
FlameOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
FlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
FlameOnMouseMove(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
FlameOnScroll(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
FlameOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
FlameOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

VOID
FlameBuildGraph(
	__in PFLAME_CONTROL Object
	);

VOID
FlameInitializeStack(
	__in PFLAME_CONTROL Object
	);

VOID
FlameFreeStack(
	__in PFLAME_CONTROL Object
	);

HWND
FlameCreateTooltip(
	__in PFLAME_CONTROL Object
	);

BOOLEAN
FlameActivateTooltip(
	__in PFLAME_CONTROL Object
	);
BOOLEAN
FlameDeactivateTooltip(
	__in PFLAME_CONTROL Object
	);

VOID
FlameDrawFocusRect(
	__in PFLAME_CONTROL Object,
	__in PFLAME_NODE FlameNode
    );
VOID
FlameInvalidateRect(
	__in PFLAME_CONTROL Object,
	__in PFLAME_NODE FlameNode
    );

VOID
FlameFillSolidRect(
    __in HWND hWnd,
    __in HDC hdc,
	__in COLORREF rgb,
	__in LPRECT rc 
    );

BOOLEAN
FlameQueryNode(
    __in PFLAME_CONTROL Object,
    __in PFLAME_NODE Node,
    __in ULONG Flags,
    __out PWCHAR Buffer,
    __in SIZE_T Length
    );

PFLAME_NODE
FlameHitTest(
	__in PFLAME_CONTROL Object,
	__in LPPOINT Point
	);

LONG
FlameComputeLevel(
	__in PFLAME_CONTROL Object,
	__in LONG y
	);

PFLAME_NODE
FlameScanNode(
	__in PFLAME_LEVEL Level,
	__in LONG x 
	);

VOID
FlameCreateBitmap(
	__in PFLAME_CONTROL Object
	);

VOID
FlameDrawNodes(
	__in PFLAME_CONTROL Object
	);

BOOLEAN
FlameSetFonts(
    __in PFLAME_CONTROL Object
    );

BOOLEAN
FlameInvalidate(
	__in HWND hWnd
	);

VOID
FlameGenerateFillColor(
    __in COLORREF *Color
    );

VOID 
FlameSetSize(
    __in HWND hWnd,
    __in int Width, 
    __in int Height, 
    __in int Linedx, 
    __in int Linedy, 
    __in BOOLEAN Resize 
    );

VOID 
FlameSetScrollBar(
    __in HWND hWnd,
    __in int Side, 
    __in int MaximumSize, 
    __in int PageSize 
    );

VOID
FlameDebugPrintNode(
	__in PFLAME_CONTROL Object,
	__in PFLAME_NODE FlameNode
	);

#ifdef __cplusplus
}
#endif

#endif