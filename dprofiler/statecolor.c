//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "statecolor.h"

#define SDK_STATECOLOR_CLASS L"SdkStateColor"

//
// CPU Sample State Color 
//

STATECOLOR_ENTRY StateColorTable[] = {
	{ CPU_STATE_RUN, RGB(0x9E, 0xCA, 0x9E), L"Execution" },
	{ CPU_STATE_WAIT, RGB(0xFF, 0x79, 0x71), L"Wait" },
	{ CPU_STATE_IO, RGB(0xCB, 0x98, 0xB6), L"I/O" },
	{ CPU_STATE_SLEEP, RGB(0x89, 0xAB, 0xBD), L"Sleep" },
	{ CPU_STATE_MEMORY, RGB(0xFF, 0x99, 0x00), L"Memory" },
	{ CPU_STATE_PREEMPTED, RGB(0x00, 0x99, 0xFF), L"Preempted" },
	{ CPU_STATE_UI, RGB(0xFD, 0x32, 0xFF), L"UI" }
};

static COLORREF crBorder = RGB(0x3C, 0x94, 0x3C);

BOOLEAN 
StateColorInitialize(
	VOID
	)
{
    WNDCLASSEX wc = {0};

	wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = StateColorProcedure;
    wc.hInstance = SdkInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = SDK_STATECOLOR_CLASS;

	if (!RegisterClassEx(&wc)) {
        return FALSE;
	}

    return TRUE;
}

HWND 
StateColorCreateControl(
    __in HWND hWndParent,
    __in INT_PTR Id
    )
{
	HWND hWnd;

    hWnd = CreateWindow(SDK_STATECOLOR_CLASS, L"",
						WS_CHILD | WS_VISIBLE,
						CW_USEDEFAULT, CW_USEDEFAULT, 
						CW_USEDEFAULT, CW_USEDEFAULT, 
						hWndParent, (HMENU)Id, SdkInstance,NULL);
	return hWnd;
}

BOOLEAN
StateColorActivateTooltip(
	__in PSTATECOLOR_CONTROL Object,
    __in BOOLEAN Activate
	)
{
    TOOLINFO ToolInfo = {0};
	   
    ToolInfo.cbSize = sizeof(TOOLINFO);
    ToolInfo.uFlags = TTF_TRACK|TTF_IDISHWND|TTF_ABSOLUTE;
	ToolInfo.hwnd = Object->hWnd;
	ToolInfo.uId = (UINT_PTR)Object->hWnd;
    ToolInfo.hinst = SdkInstance;
    ToolInfo.lpszText = LPSTR_TEXTCALLBACK;

    SendMessage(Object->hWndTooltip, TTM_TRACKACTIVATE, (WPARAM)Activate, (LPARAM)&ToolInfo);
	Object->IsTooltipActivated = Activate;
    return TRUE;    
}

HWND
StateColorCreateTooltip(
	__in PSTATECOLOR_CONTROL Object
	)
{
    HWND hWnd;
	TOOLINFO ToolInfo = {0};
	   
    hWnd = CreateWindow(TOOLTIPS_CLASS, TEXT(""),
                        WS_POPUP,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, (HMENU)NULL, SdkInstance,
                        NULL);

    ToolInfo.cbSize = sizeof(TOOLINFO);
    ToolInfo.uFlags = TTF_TRACK|TTF_IDISHWND|TTF_ABSOLUTE;
	ToolInfo.hwnd = Object->hWnd;
	ToolInfo.uId = (UINT_PTR)Object->hWnd;
    ToolInfo.hinst = SdkInstance;
    ToolInfo.lpszText = LPSTR_TEXTCALLBACK;

	Object->hWndTooltip = hWnd;
    SendMessage(hWnd, TTM_ADDTOOL, 0, (LPARAM)&ToolInfo);
    return hWnd;    
}

LRESULT
StateColorOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PSTATECOLOR_CONTROL Object;

	Object = (PSTATECOLOR_CONTROL)SdkMalloc(sizeof(STATECOLOR_CONTROL));
	Object->hWnd = hWnd;
	Object->hWndTooltip = NULL;
    memcpy_s(Object->Entry, sizeof(StateColorTable), StateColorTable, sizeof(StateColorTable));

    StateColorCreateTooltip(Object);

	SdkSetObject(hWnd, Object);
	ShowWindow(hWnd, SW_SHOW);
	return 0L;
}

LRESULT
StateColorOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PSTATECOLOR_CONTROL Object;
	PAINTSTRUCT Paint;
	RECT rc;
	HDC hdc;
	ULONG Number;

	Object = (PSTATECOLOR_CONTROL)SdkGetObject(hWnd);
	if (!Object) {
		return 0;
	}

	hdc = BeginPaint(hWnd, &Paint);

    //
    // Draw state
    //

	rc.left = 1;
	rc.top = 1;
	rc.right = 21;
	rc.bottom = 21;

	for(Number = 0; Number < CPU_STATE_COUNT; Number += 1) {

		SetDCBrushColor(hdc, Object->Entry[Number].Color);
		FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        FrameRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

		rc.left = rc.right + 1;
		rc.right += 20;
	}

	EndPaint(hWnd, &Paint);
	return 0;
}

LRESULT
StateColorOnMouseMove(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
    PSTATECOLOR_CONTROL Object = NULL;
	TRACKMOUSEEVENT Event = {0};
    POINT pt;
    int index;

	Object = (PSTATECOLOR_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    StateColorActivateTooltip(Object, TRUE);

    pt.x = GET_X_LPARAM(lp);
    pt.y = GET_Y_LPARAM(lp);

    index = StateColorHitTest(&pt);
    if (index == -1) {
        StateColorActivateTooltip(Object, FALSE);
        return 0;
    }

    ClientToScreen(hWnd, &pt);
    pt.y += 20;
    SendMessage(Object->hWndTooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

	Event.cbSize = sizeof(Event);
	Event.dwFlags = TME_LEAVE;
	Event.hwndTrack = Object->hWnd;
	TrackMouseEvent(&Event);
	return 0;
}

LRESULT
StateColorOnMouseLeave(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
    PSTATECOLOR_CONTROL Object = NULL;

	Object = (PSTATECOLOR_CONTROL)SdkGetObject(hWnd);
	if (!Object) {
		return 0;
	}

    StateColorActivateTooltip(Object, FALSE);
	return 0;
}

LRESULT
StateColorOnLButtonDown(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PSTATECOLOR_CONTROL Object;
    CHOOSECOLOR Choose = { sizeof(Choose) };
    COLORREF Colors[16] = { 0 };

	Object = (PSTATECOLOR_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	return 0L;
}

LRESULT CALLBACK 
StateColorProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PSTATECOLOR_CONTROL Object = NULL;

    switch (uMsg) {

    case WM_CREATE:
		StateColorOnCreate(hWnd, uMsg, wp, lp);
        break;

    case WM_DESTROY:
		Object = (PSTATECOLOR_CONTROL)SdkGetObject(hWnd);
		SdkFree(Object);
        break;

    case WM_PAINT:
		StateColorOnPaint(hWnd, uMsg, wp, lp);
        break;

    case WM_MOUSEMOVE:
		StateColorOnMouseMove(hWnd, uMsg, wp, lp);
        break;

    case WM_MOUSELEAVE:
        StateColorOnMouseLeave(hWnd, uMsg, wp, lp);
        break;

    case WM_LBUTTONDOWN:
        StateColorOnLButtonDown(hWnd, uMsg, wp, lp);
        break;

    case WM_NOTIFY:
        return StateColorOnNotify(hWnd, uMsg, wp, lp);
    }

    return DefWindowProc(hWnd, uMsg, wp, lp);
}

LRESULT
StateColorOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	LRESULT Status;
	LPNMHDR pNmhdr = (LPNMHDR)lp;

	Status = 0;

	switch (pNmhdr->code) {
		case TTN_GETDISPINFO:
			return StateColorOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
StateColorOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PSTATECOLOR_CONTROL Object;
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;
    POINT pt;
    int index;

    Object = (PSTATECOLOR_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);

    index = StateColorHitTest(&pt);
    if (index == -1) {
        lpnmtdi->lpszText = NULL;
        return 0;
    }

	lpnmtdi->lpszText = Object->Entry[index].Description;
    return 0;
}

int
StateColorHitTest(
    __in LPPOINT pt
    )
{ 
    RECT rc;

    rc.left = 1;
	rc.top = 1;
	rc.right = 1 + 21 * CPU_STATE_COUNT;
	rc.bottom = 21;

    if (!PtInRect(&rc, *pt)) {
        return -1;
    }

    return (pt->x - rc.left) / 21;
}