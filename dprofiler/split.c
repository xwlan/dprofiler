//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#pragma warning(disable: 4100 4201)

#include "sdk.h"
#include "split.h"

#define PROP_SPLIT_OBJECT  L"SplitObject"
#define PROP_SPLIT_START   L"SplitStart"


BOOLEAN 
SplitInitialize(
	__in HINSTANCE Instance
	) 
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = SplitProcedure;
    wc.hInstance = Instance;
	wc.lpszClassName = SdkSplitClass;
	return RegisterClass(&wc) ? TRUE : FALSE;
}

VOID 
SplitCreate(
    __in HWND hWnd, 
	__in int id, 
	__in PSPLIT_OBJECT Object
	)
{
    HWND hWndCtrl = GetDlgItem(hWnd, id);
    SplitSetObject(hWndCtrl, Object);
}

PSPLIT_OBJECT 
SplitGetObject(
	__in HWND hWnd
	) 
{
    return (SPLIT_OBJECT *)GetProp(hWnd, PROP_SPLIT_OBJECT);
}

VOID
SplitSetObject(
	__in HWND hWnd, 
	__in PSPLIT_OBJECT Object
	) 
{
    SetProp(hWnd, PROP_SPLIT_OBJECT, (HANDLE)Object);
}

LONG 
SplitGetStart(
	__in HWND hWnd
	) 
{
    return HandleToLong(GetProp(hWnd, PROP_SPLIT_START));
}

VOID 
SplitSetStart(
	__in HWND hWnd, 
	__in LONG Start
	) 
{
    SetProp(hWnd, PROP_SPLIT_START, LongToHandle(Start));
}

BOOLEAN 
SplitIsVertical(
	__in HWND hWnd
	) 
{
	PSPLIT_OBJECT Object;
	
	Object = SplitGetObject(hWnd);
	return Object->Vertical;
}

VOID
SplitOnLButtonDown(
    __in HWND hWnd, 
	__in BOOL fDoubleClick, 
	__in int x, 
	__in int y, 
	__in UINT keyFlags
	)
{
    SetCapture(hWnd);
    SplitSetStart(hWnd, SplitIsVertical(hWnd) ? x : y);
}

HDWP 
SplitMoveDlgItem(
    __in HDWP hdwp, 
	__in HWND hWnd, 
	__in int id, 
	__in RECT *prc
	)
{
    return DeferWindowPos(hdwp, GetDlgItem(hWnd, id),
                          0, prc->left, prc->top,
						  prc->right  - prc->left,
						  prc->bottom - prc->top, SWP_NOZORDER);
}

RECT 
SplitGetDlgItemRect(
	__in HWND hWnd, 
	__in int id
	) 
{
    RECT rc = {0};
    HWND hWndChild;
	
	hWndChild = GetDlgItem(hWnd, id);
    GetWindowRect(hWndChild, &rc);
    MapWindowRect(HWND_DESKTOP, hWnd, &rc);
    return rc;
}

RECT 
SplitGetOffsetCtl(
    __in HWND hWndParent, 
	__in int which, 
	__in int id,
    __in LONG xDelta, 
	__in LONG yDelta
	)
{
    RECT rc;
	
	rc = SplitGetDlgItemRect(hWndParent, id);

    if (which & SPLIT_TOP) { 
		rc.top    += yDelta; 
	}

    if (which & SPLIT_LEFT) { 
		rc.left   += xDelta; 
	}

    if (which & SPLIT_RIGHT) { 
		rc.right  += xDelta; 
	}

    if (which & SPLIT_BOTTOM) { 
		rc.bottom += yDelta; 
	}

    return rc;
}

VOID 
SplitMoveControls(
    __in HWND hWnd, 
	__in LONG xDelta, 
	__in LONG yDelta
	)
{
	HDWP hdwp;
    PSPLIT_OBJECT Object;
	PSPLIT_INFO Info;
	HWND hWndParent;
	RECT rcItem;
	int id;
    int i = 0;
	
	Object = SplitGetObject(hWnd);

	hdwp = BeginDeferWindowPos(Object->NumberOfCtrls + 1);
    id = GetDlgCtrlID(hWnd);
    hWndParent = GetParent(hWnd);
    rcItem = SplitGetDlgItemRect(hWndParent, id);

    OffsetRect(&rcItem, xDelta, yDelta);
    hdwp = SplitMoveDlgItem(hdwp, hWndParent, id, &rcItem);

	for (i = 0; i < Object->NumberOfCtrls; ++i) {
		Info = Object->Info + i;
        rcItem = SplitGetOffsetCtl(hWndParent, Info->Flag, 
			                       Info->CtrlId, xDelta, yDelta);
		hdwp = SplitMoveDlgItem(hdwp, hWndParent, Info->CtrlId, &rcItem);
    }

    EndDeferWindowPos(hdwp);
}

SIZE 
SplitGetSmallest(
	__in HWND hWndParent,
    __in PSPLIT_OBJECT Object, 
	__in SPLIT_FLAG Flag 
	)
{
    SIZE size = { LONG_MAX, LONG_MAX };
	PSPLIT_INFO Info;
	RECT rc;
    int i = 0;

    for (i = 0; i < Object->NumberOfCtrls; ++i ) {
		Info = Object->Info + i;
        if (Info->Flag == Flag) {
			rc = SplitGetDlgItemRect(hWndParent, Info->CtrlId);
            size.cx = min(size.cx, rc.right - rc.left);
            size.cy = min(size.cy, rc.bottom - rc.top);
        }
    }

    return size;
}

LONG 
SplitAdjustDelta(
	__in HWND hWnd, 
	__in int Delta 
	) 
{
	SIZE Left;
	SIZE Right;
	LONG MinLeft;
	LONG MinRight;
    LONG AdjustedDelta = Delta;

    SPLIT_OBJECT *Object = SplitGetObject(hWnd);

    Left = SplitGetSmallest(GetParent(hWnd), Object, SPLIT_RIGHT);
    Right = SplitGetSmallest(GetParent(hWnd), Object, SPLIT_LEFT);
    MinLeft = SplitIsVertical(hWnd) ? Left.cx : Left.cy;
    MinRight = SplitIsVertical(hWnd) ? Right.cx : Right.cy;

    if (Delta < 0 && MinLeft + Delta < Object->Min) {
        AdjustedDelta = Object->Min - MinLeft;
    }

    if (0 < Delta && MinRight - Delta < Object->Min) {
        AdjustedDelta = MinRight - Object->Min;
    }

    return AdjustedDelta;
}

VOID 
SplitOnMouseMove(
	__in HWND hWnd, 
	__in int x, 
	__in int y, 
	__in UINT flags
	) 
{
	LONG xDelta;
	LONG yDelta;
	LONG Delta;
	LONG NewPos;

    if (hWnd == GetCapture()) {

        NewPos = SplitIsVertical(hWnd) ? x : y;
        Delta = SplitAdjustDelta(hWnd, NewPos - SplitGetStart(hWnd));

        if (Delta != 0) {
            xDelta = SplitIsVertical(hWnd) ? Delta : 0;
            yDelta = SplitIsVertical(hWnd) ? 0 : Delta;
            SplitMoveControls(hWnd, xDelta, yDelta);
        }
    }
}

VOID
SplitOnLButtonUp(
	__in HWND hWnd, 
	__in int x, 
	__in int y, 
	__in UINT flags
	) 
{
    ReleaseCapture();
}

BOOL 
SplitOnSetCursor(
	__in HWND hWnd, 
	__in HWND hWndCursor, 
	__in UINT codeHitTest, 
	__in UINT uMsg
	)
{
    PWSTR Cursor;
	
	Cursor = SplitIsVertical(hWnd) ? IDC_SIZEWE : IDC_SIZENS;
    SetCursor(LoadCursor(0, Cursor));
    return TRUE;
}

LRESULT CALLBACK 
SplitProcedure(
    __in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wParam, 
	__in LPARAM lParam 
	)
{
    switch (uMsg) {
		HANDLE_MSG(hWnd, WM_SETCURSOR,   SplitOnSetCursor);
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN, SplitOnLButtonDown);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE,   SplitOnMouseMove);
		HANDLE_MSG(hWnd, WM_LBUTTONUP,   SplitOnLButtonUp);
		HANDLE_MSG(hWnd, WM_PAINT,       SplitOnPaint);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID
SplitOnPaint(
	__in HWND hWnd 
	)
{
	RECT Rect;
	PAINTSTRUCT ps;

	if (!GetUpdateRect(hWnd, &Rect, FALSE)) {
		return;
	}

	BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);
	return;
}