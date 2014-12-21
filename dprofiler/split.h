//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#ifndef _SPLIT_H_
#define _SPLIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SdkSplitClass    L"SdkSplit"

//
// SPLIT_FLAG is splitter's position relative to
// adjacent controls
//

typedef enum _SPLIT_FLAG {
	SPLIT_TOP = 1,
	SPLIT_BOTTOM,
	SPLIT_LEFT = SPLIT_TOP,
	SPLIT_RIGHT = SPLIT_BOTTOM,
	SPLIT_BOTH = SPLIT_TOP | SPLIT_BOTTOM,
} SPLIT_FLAG, *PSLIT_FLAG;

typedef struct _SPLIT_INFO {
	int CtrlId;
	SPLIT_FLAG Flag;
} SPLIT_INFO, *PSPLIT_INFO;

typedef struct _SPLIT_OBJECT {
	BOOLEAN Vertical;
	PSPLIT_INFO Info;
	int NumberOfCtrls;
	int Min;
} SPLIT_OBJECT, *PSPLIT_OBJECT;

//
// Public
//

BOOLEAN 
SplitInitialize(
	__in HINSTANCE Instance
	);

VOID
SplitCreate(
	__in HWND hWndParent,
	__in int id,
	__in PSPLIT_OBJECT Object
	);

VOID
SplitSetObject(
	__in HWND hWnd, 
	__in PSPLIT_OBJECT Object
	);

PSPLIT_OBJECT 
SplitGetObject(
	__in HWND hWnd
	); 

//
// Private
//

VOID 
SplitSetStart(
	__in HWND hWnd, 
	__in LONG Start
	); 

BOOLEAN 
SplitIsVertical(
	__in HWND hWnd
	); 

VOID
SplitOnLButtonDown(
    __in HWND hWnd, 
	__in BOOL fDoubleClick, 
	__in int x, 
	__in int y, 
	__in UINT keyFlags
	);

HDWP 
SplitMoveDlgItem(
    __in HDWP hdwp, 
	__in HWND hWnd, 
	__in int id, 
	__in RECT *prc
	);

RECT 
SplitGetDlgItemRect(
	__in HWND hWnd, 
	__in int id
	);

RECT 
SplitGetOffsetCtl(
    __in HWND hWndParent, 
	__in int which, 
	__in int id,
    __in LONG xDelta, 
	__in LONG yDelta
	);

VOID 
SplitMoveControls(
    __in HWND hWnd, 
	__in LONG xDelta, 
	__in LONG yDelta
	);

SIZE 
SplitGetSmallest(
	__in HWND hWndParent,
    __in PSPLIT_OBJECT Object, 
	__in SPLIT_FLAG Flag 
	);

LONG 
SplitAdjustDelta(
	__in HWND hWnd, 
	__in int Delta 
	);

VOID 
SplitOnMouseMove(
	__in HWND hWnd, 
	__in int x, 
	__in int y, 
	__in UINT flags
	);

VOID
SplitOnLButtonUp(
	__in HWND hWnd, 
	__in int x, 
	__in int y, 
	__in UINT flags
	);

BOOL 
SplitOnSetCursor(
	__in HWND hWnd, 
	__in HWND hWndCursor, 
	__in UINT codeHitTest, 
	__in UINT uMsg
	);

VOID
SplitOnPaint(
	__in HWND hWnd 
	);

LRESULT CALLBACK 
SplitProcedure(
    __in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wParam, 
	__in LPARAM lParam 
	);

#ifdef __cplusplus
}
#endif

#endif