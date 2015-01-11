//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#include "trackbar.h"
#include "aps.h"

#define SDK_TRACK_CLASS   L"SdkTrackBar"

BOOLEAN 
TrackInitialize(
    VOID
    )
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = TrackProcedure;
    wc.hInstance = SdkInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = SDK_TRACK_CLASS;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

    if (!RegisterClassEx(&wc)) {
        return FALSE;
    }

    return TRUE;
}

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
    )
{
    HWND hWnd;
    PTRACK_CONTROL Object;

    Object = (PTRACK_CONTROL)SdkMalloc(sizeof(TRACK_CONTROL));
    ZeroMemory(Object, sizeof(TRACK_CONTROL));

    Object->CtrlId = CtrlId;
    Object->BackColor = crBack;
    Object->GridColor = crGrid;
    Object->FillColor = crFill;
    Object->EdgeColor = crEdge;
	Object->SliderColor = crSlider;

	Object->hSliderPen = CreatePen(PS_SOLID, 1, Object->SliderColor);
    hWnd = CreateWindowEx(0, SDK_TRACK_CLASS, L"", 
                          WS_VISIBLE | WS_CHILD, 
                          Rect->left, Rect->top, 
                          Rect->right - Rect->left, 
                          Rect->bottom - Rect->top, 
                          hWndParent, (HMENU)CtrlId, 
                          SdkInstance, Object);
	//
	// Create tooltip
	//

	TrackCreateTooltip(Object);
    return Object;
}

VOID
TrackInitializeControl(
    __in HWND hWnd,
    __in INT_PTR CtrlId,
    __in COLORREF crBack,
    __in COLORREF crGrid,
    __in COLORREF crFill,
    __in COLORREF crEdge,
	__in COLORREF crSlider
    )
{
    PTRACK_CONTROL Object;

    Object = (PTRACK_CONTROL)SdkMalloc(sizeof(TRACK_CONTROL));
    ZeroMemory(Object, sizeof(TRACK_CONTROL));

    Object->CtrlId = CtrlId;
    Object->BackColor = crBack;
    Object->GridColor = crGrid;
    Object->FillColor = crFill;
    Object->EdgeColor = crEdge;
	Object->SliderColor = crSlider;
	
	Object->hSliderPen = CreatePen(PS_SOLID, 1, Object->SliderColor);
    Object->hWnd = hWnd;
    SdkSetObject(hWnd, Object);

	SdkModifyStyle(hWnd, 0, WS_CLIPSIBLINGS|WS_HSCROLL|WS_BORDER, FALSE);

	//
	// Create tooltip
	//

	TrackCreateTooltip(Object);
}

VOID
TrackSetHistoryData(
    __in PTRACK_CONTROL Control,
    __in PCPU_HISTORY History,
	__in LONG MergeSteps
    )
{
    Control->History = History; 
	Control->MergeSteps = MergeSteps;
}


LRESULT
TrackOnCreate(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PTRACK_CONTROL Object;
    LPCREATESTRUCT lpCreate;

    lpCreate = (LPCREATESTRUCT)lp;
    Object = (PTRACK_CONTROL)lpCreate->lpCreateParams;

    if (Object != NULL) {
        Object->hWnd = hWnd;
        SdkSetObject(hWnd, Object);
    }

    return 0;
}

LRESULT
TrackOnPaint(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PAINTSTRUCT Paint;
    PTRACK_CONTROL Object;
    RECT rc;
    HDC hdc;
	int x;
    COLORREF crFill;
    
	Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

	if (!GetUpdateRect(hWnd, &rc, FALSE)) {
		return 0;
    }

    hdc = BeginPaint(hWnd, &Paint);
    
    //
    // Erase background
    //

    GetClientRect(hWnd, &rc);
    crFill = GetSysColor(COLOR_MENU);
    SdkFillSolidRect(hdc, crFill, &rc);

	x = GetScrollPos(hWnd, SB_HORZ);
	BitBlt(hdc, Object->bmpRect.left, Object->bmpRect.top + Object->VertEdge,
		   rc.right - rc.left,  Object->Height, 
		   Object->hdcTrack, x, 0, SRCCOPY);

	//
	// Draw current slider
	//

    rc = Object->rcSlider;
    rc.left -= x;
    rc.right -= x;
	FrameRect(hdc, &rc, Object->hBrushSlider);

    EndPaint(hWnd, &Paint);
    return 0;
}

LRESULT
TrackOnSize(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PTRACK_CONTROL Object;
	RECT rc;
	
    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

	if (!Object->hbmpTrack) {

		//
		// Not yet finish initialization
		//

		return 0;
	}

	GetClientRect(hWnd, &rc);
	TrackSetScrollBar(hWnd, Object->Width,  LOWORD(lp));
   
	InvalidateRect(hWnd, &Object->rcOldSlider, TRUE);
    UpdateWindow(hWnd);
    return 0;
}

LRESULT
TrackOnDestroy(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PTRACK_CONTROL Object;

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
    ASSERT(Object != NULL);

    if (Object->hbmpTrack) {
        DeleteObject(Object->hbmpTrack);
    }

    if (Object->hdcTrack) {
        DeleteDC(Object->hdcTrack);
    }

    SdkFree(Object);
    SdkSetObject(hWnd, NULL);
    return 0L;
}

LRESULT CALLBACK 
TrackProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    switch (uMsg) {

    case WM_CREATE:
        TrackOnCreate(hWnd, uMsg, wp, lp);
        break;

    case WM_DESTROY:
        TrackOnDestroy(hWnd, uMsg, wp, lp);
        break;

    case WM_SIZE:
        TrackOnSize(hWnd, uMsg, wp, lp);
        break;

    case WM_PAINT:
        TrackOnPaint(hWnd, uMsg, wp, lp); 
        break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_MOUSEMOVE:
        TrackOnMouseMove(hWnd, uMsg, wp, lp);
        break;

	case WM_LBUTTONDOWN:
		TrackOnLButtonDown(hWnd, uMsg, wp, lp);
		break;
	
	case WM_LBUTTONUP:
		TrackOnLButtonUp(hWnd, uMsg, wp, lp);
		break;

	case WM_NOTIFY:
		TrackOnNotify(hWnd, uMsg, wp, lp);
		break;

    case WM_MOUSELEAVE:
        TrackOnMouseLeave(hWnd, uMsg, wp, lp);
        break;

	case WM_HSCROLL:
        TrackOnScroll(hWnd, uMsg, wp, lp);
        break;
    }

    return DefWindowProc(hWnd, uMsg, wp, lp);
}

VOID
TrackInvalidateSliderRect(
    __in PTRACK_CONTROL Object,
	__in LPRECT Rect
    )
{
	RECT rc;
	RECT rcClient;

	GetClientRect(Object->hWnd, &rcClient);

	rc = *Rect;

	if (rc.left == 0) {
		rc.left = 2;
	}

	if (rc.right >= rcClient.right) {
		rc.right = rcClient.right - 2;
	}

	InflateRect(&rc, 2, 2);
	InvalidateRect(Object->hWnd, &rc, TRUE);
}

BOOLEAN
TrackCreateBitmap(
	__in PTRACK_CONTROL Object,
	__in int Height,
	__in int VertEdge
	)
{
    HDC hdc;
    HDC hdcTrack;
    HBITMAP hbmp;
    HBITMAP hbmpOld;
    HBRUSH hBrush;
    HGDIOBJ hOldBrush;
    RECT Rect;
	PCPU_HISTORY History;
    ULONG Count;

    hdc = GetDC(Object->hWnd);
	History = Object->History;

	ASSERT(History != NULL);
	if (!History || !History->Count) {
		return FALSE;
	}

	//
	// Set history bitmap rect size
	//
	
	Count = History->Count / Object->MergeSteps;
	if (History->Count / Object->MergeSteps != 0) {
		Count += 1;
	}

	Object->Width = Count;
	Object->Height = Height;
	Object->VertEdge = VertEdge;

	Rect.left = 0;
	Rect.top = 0;
	Rect.right = Object->Width;
	Rect.bottom = Object->Height;

	//
	// Create compatible dc and bitmap, fill the background
	//

	hdcTrack = CreateCompatibleDC(hdc);
	ASSERT(hdcTrack != NULL);

	if (!hdcTrack) {
		ReleaseDC(Object->hWnd, hdc);
		return FALSE;
	}

	hbmp = CreateCompatibleBitmap(hdc, Object->Width, Object->Height);
	ASSERT(hbmp != NULL);
	if (!hbmp) {
		ReleaseDC(Object->hWnd, hdc);
		return FALSE;
	}

	ReleaseDC(Object->hWnd, hdc);
	hbmpOld = (HBITMAP)SelectObject(hdcTrack, hbmp);
	ASSERT(hbmpOld != NULL);

	hBrush = CreateSolidBrush(Object->BackColor);
	hOldBrush = SelectObject(Object->hdcTrack, hBrush);

	FillRect(hdcTrack, &Rect, hBrush);

	SelectObject(Object->hdcTrack, hOldBrush);
	DeleteObject(hBrush);

	Object->hdcTrack = hdcTrack;
	Object->hbmpTrack = hbmp;
	Object->hbmpTrackOld = hbmpOld;
	Object->bmpRect = Rect;

	return TRUE;
}

VOID
TrackDrawHistory(
    __in PTRACK_CONTROL Object,
	__in LPRECT Rect,
	__in int VertEdge
	)
{
	int Height;
	
    TrackSetSliderSize(Object, 2, 10, 40);

	Height = Rect->bottom - Rect->top - VertEdge * 2;
	ASSERT(Height > 0);

	TrackCreateBitmap(Object, Height, VertEdge);
	TrackDrawBitmap(Object);

	InvalidateRect(Object->hWnd, Rect, FALSE);
	UpdateWindow(Object->hWnd);
}

VOID
TrackDrawBitmap(
    __in PTRACK_CONTROL Object
    )
{
	RECT Rect;
    HDC hdc;
    HBRUSH hBrush;
    HBRUSH hBrushOld;
    HPEN hPen, hPenOld;
    HRGN hPolygon;
    LONG Width, Height;
    POINT *Point;
    PCPU_HISTORY History;
    FLOAT Data;
    LONG Count;
    LONG i, j;
	LONG Number;
    FLOAT *Value;
    FLOAT Limits;
    LONG Step;
	LONG x;

	//
	// Ensure memory dc and bitmap is ready
	//

	ASSERT(Object->hdcTrack != NULL);
    if(!Object->hdcTrack) {
        return;
    }

	ASSERT(Object->History != NULL);
    History = Object->History;
    if (!History) {
        return;
    }

    GetClientRect(Object->hWnd, &Rect);

	Width = Object->Width;
	Height = Object->Height;

    //
    // N.B. Although for most cases 1 is the horizonal step, however, in some special cases,
    // user did not collect enough samples, the step can be greater than 1, we ensure that
    // under any condition, the step is correctly computed.
    //

    Step = ApsComputeClosestLong((FLOAT)Width/(FLOAT)History->Count);
	Object->HoriStep = 1;
    Object->VertStep = Height / 10;

    //
    // Translate the values into bounded values
    //

    Value = History->Value;
    Limits = History->Limits;
    Count = History->Count;

	//
	// Round down to multiples of MergeSteps
	//

	Number = 0;
	x = Rect.left;
	Count = Count - Count % Object->MergeSteps;
    Point = (POINT *)SdkMalloc(sizeof(POINT) * (Count + 4));

	//
	// Merge the values by MergeSteps
	//

	for (i = 0; i < Count; ) {

        Point[Number].x = x;	
		Data = 0.0;

		for(j = 0; j < Object->MergeSteps; j += 1, i += 1) {
			Data += Value[i];
		}

		DebugTrace("# %d value: %.2f", i, Data / (Limits * Object->MergeSteps));
		Data = (FLOAT)Height * (Data / (Limits * Object->MergeSteps));
        Point[Number].y = Height - min(Height, ApsComputeClosestLong(Data));

		Number += 1;
		x += 1;
    }

	//
	// If there's remainder, merge the remained values
	//

	if (History->Count - i != 0) {

	    Point[Number].x = x;	
		Count = History->Count - i;
		Data = 0.0;

		for(; i < (LONG)History->Count; i += 1) {
			Data += Value[i];
		}

		Data = (FLOAT)Height * ((FLOAT)1.0 - (Data / (Limits * Count)));
	    Point[Number].y = min(Height, ApsComputeClosestLong(Data));

		Number += 1;
	}

	Count = Number;

	//
	// N.B. The following codes need be checked
	//

	Point[Count].x = Point[Count - 1].x;
    Point[Count].y = Rect.bottom;
    Point[Count + 1].x = Point[0].x;
    Point[Count + 1].y = Rect.bottom;

    //
    // Fill the ploygon region combined by sample points
    //

    hdc = Object->hdcTrack;
    hBrush = CreateSolidBrush(Object->FillColor);
    hBrushOld = (HBRUSH)SelectObject(hdc, hBrush);

    hPolygon = CreatePolygonRgn(Point, Count + 2, ALTERNATE);
    FillRgn(hdc, hPolygon, hBrush);

    SelectObject(hdc, hBrushOld);
    DeleteObject(hBrush);
    DeleteObject(hPolygon);

    //
    // Draw the edge use deep color pen
    //

    hPen = CreatePen(PS_SOLID, 1, Object->EdgeColor);
    hPenOld = (HPEN)SelectObject(hdc, hPen);

    Point[Count].x = Rect.right - 1;
    Polyline(hdc, Point, Count);

    SdkFree(Point);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

#if defined(_DEBUG)
	SdkSaveBitmap(Object->hbmpTrack, L"trackbar.bmp", 8);
#endif
}

BOOLEAN
TrackDrawSlider(
    __in PTRACK_CONTROL Object,
	__in LPPOINT pt
    )
{
	RECT rcSlider;
    RECT rcClient;
    BOOLEAN Reposition = FALSE;
	BOOLEAN MouseHover = FALSE;
    int Delta;

	MouseHover = (Object->hWnd == GetFocus()) ? TRUE : FALSE;
    rcClient = Object->bmpRect;

	Object->rcOldSlider = Object->rcSlider;

    //
    // Add scroll delta, we always track the slider position relative to track bitmap
    //

    Delta = GetScrollPos(Object->hWnd, SB_HORZ);
    pt->x = pt->x + Delta;

    //
    // If the slider width is bigger than bitmap width,
    // adjust slider width to cover the whole bitmap
    //

    if (Object->Width <= Object->SliderWidth) {
        pt->x = Object->SliderCursorEdge;
        rcSlider.left = 0;
        rcSlider.top = Object->SliderVertEdge;
        rcSlider.right = Object->Width;
        rcSlider.bottom = rcSlider.top + Object->SliderHeight;

        Reposition = TRUE;
    }
    else if (pt->x >= 0 && pt->x <= Object->SliderCursorEdge) {

        //
        // If the x falls into (0, SliderCursorEdge), reposition 
        // the mouse to SliderCursorEdge.
        //

        pt->x = Object->SliderCursorEdge;

        rcSlider.left = 0;
        rcSlider.top = Object->SliderVertEdge;
        rcSlider.right = rcSlider.left + Object->SliderWidth;
        rcSlider.bottom = rcSlider.top + Object->SliderHeight;

        Reposition = TRUE;
    }
    else if (pt->x >= rcClient.right - Object->SliderWidth + Object->SliderCursorEdge) {

        //
        // If the x falls into (ClientRect.right - SliderWidth + SliderCursorEdge, ClientRect.right),
        // reposition the mouse to SliderWidth - SliderCursorEdge.
        //

        pt->x = rcClient.right - Object->SliderWidth + Object->SliderCursorEdge;
        rcSlider.left = rcClient.right - Object->SliderWidth;
        rcSlider.top = Object->SliderVertEdge;
        rcSlider.right = rcClient.right - 1;
        rcSlider.bottom = rcSlider.top + Object->SliderHeight;

        Reposition = TRUE;
    }
    else {

        //
        // x falls into middle of track control, compute the appropriate position,
        // mouse position is not reposition
        //

        rcSlider.left = pt->x - Object->SliderCursorEdge;
        rcSlider.top = Object->SliderVertEdge;
        rcSlider.right = rcSlider.left + Object->SliderWidth;
        rcSlider.bottom = rcSlider.top + Object->SliderHeight;
    }

    Object->rcSlider = rcSlider;

	//
	// Need to reposition mouse pointer?
	//

    if (Reposition && MouseHover) {
		return TRUE;
    }

	return FALSE;
}

VOID
TrackSetSliderSize(
    __in PTRACK_CONTROL Object,
    __in int VertEdge,
    __in int CursorEdge,
    __in int Width
    )
{
    RECT rc;
    HBRUSH hBrush;

    GetClientRect(Object->hWnd, &rc);

    Object->SliderVertEdge = VertEdge;
    Object->SliderCursorEdge = CursorEdge;
    Object->SliderWidth = Width;
    Object->SliderHeight = rc.bottom - rc.top - VertEdge * 2;

    Object->hSliderPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    hBrush = CreateSolidBrush(RGB(255, 0, 0));
    Object->hBrushSlider = hBrush;
    Object->rcSlider.left = 0; 
    Object->rcSlider.top = 0; 
    Object->rcSlider.right = 0; 
    Object->rcSlider.bottom = 0; 

    Object->MouseCaptured = FALSE;
}

LRESULT
TrackOnLButtonDown(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PTRACK_CONTROL Object;
    RECT Rect;
    POINT pt;
	BOOLEAN Reposition;

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);

    pt.x = GET_X_LPARAM(lp);
    pt.y = GET_Y_LPARAM(lp);

    Reposition = TrackDrawSlider(Object, &pt);

    //
    // N.B. The following are required, but I think it's not necessary,
    // we need investigate to elimite the incurred paint operation 
    //

	GetClientRect(hWnd, &Rect);
	InvalidateRect(hWnd, &Rect, FALSE);
	UpdateWindow(hWnd);

	if (Reposition) {

		//
		// N.B. TrackDrawSlider already fixed the point values.
		//

		MapWindowPoints(hWnd, HWND_DESKTOP, &pt, 1);
		SetCursorPos(pt.x, pt.y);
	}

	Object->MouseCaptured = TRUE;
	return 0L;
}

VOID
TrackSetScrollBar(
    __in HWND hWnd,
    __in int MaximumSize, 
    __in int PageSize 
    )
{
    SCROLLINFO si;

	if (PageSize < MaximumSize) {

		si.cbSize = sizeof(SCROLLINFO);
		si.fMask  = SIF_RANGE | SIF_PAGE;
		si.nMin   = 0;
		si.nMax   = MaximumSize - 1;
		si.nPage  = PageSize;
		si.nPos = 0;

		EnableScrollBar(hWnd, SB_HORZ, ESB_ENABLE_BOTH);
		SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
	}
	else {
		SetScrollPos(hWnd, SB_HORZ, 0, FALSE);
		EnableScrollBar(hWnd, SB_HORZ, ESB_DISABLE_BOTH);
	}
}

LRESULT
TrackOnScroll(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PTRACK_CONTROL Object;
    int ScrollCode;
    int Position;
    int PageSize;
    int Distance;
    int Minimum;
    int Maximum;
	SCROLLINFO si;

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    ScrollCode = LOWORD(wp);
    Position = HIWORD(wp);

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_PAGE;
	GetScrollInfo(hWnd, SB_HORZ, &si);
	PageSize = si.nPage;

	switch (ScrollCode) {

		case SB_LINEDOWN:
            Distance =  1;
			break;

		case SB_LINEUP:
            Distance = -1;
			break;

		case SB_PAGEDOWN:
			Distance = PageSize;
			break;

		case SB_PAGEUP:
			Distance = - PageSize;
			break;

		case SB_THUMBPOSITION:
			Distance = Position - GetScrollPos(hWnd, SB_HORZ);
			break;

		default:
			Distance = 0;
			break;
    }

	if (Distance) {

		GetScrollRange(hWnd, SB_HORZ, &Minimum, &Maximum);

		Position = GetScrollPos(hWnd, SB_HORZ) + Distance;
		Position = max(Minimum, Position);
		Position = min(Maximum - PageSize, Position);

		Distance = Position - GetScrollPos(hWnd, SB_HORZ);
		if (Distance) {
			SetScrollPos(hWnd, SB_HORZ, Position, TRUE);
			ScrollWindow(hWnd, -Distance, 0, NULL, NULL);
        }
	}

    return 0L;
}

LRESULT
TrackOnMouseLeave(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PTRACK_CONTROL Object;

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    if (Object->IsTooltipActivated) {
        TrackActivateTooltip(Object, FALSE);
	}
    
	Object->IsTracking = FALSE;
    return 0;
}

LRESULT
TrackOnLButtonUp(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PTRACK_CONTROL Object;
    PCPU_HISTORY History;

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
	if (Object->MouseCaptured) {
		Object->MouseCaptured = FALSE;
	}

    //
    // Compute the sample range by current slider position
    //

    History = Object->History;
    Object->FirstSample = Object->rcSlider.left * Object->MergeSteps;
    Object->LastSample = Object->rcSlider.right * Object->MergeSteps;

    //
    // The last sample is required to be bounded into legal sample range
    //

    Object->LastSample = min(Object->LastSample, History->Count - 1);

    //
    // Notify the selected sample range to parent window
    //

    PostMessage(GetParent(hWnd), WM_TRACK_SET_RANGE, (WPARAM)Object->FirstSample, (LPARAM)Object->LastSample);
    return 0;
}

LRESULT
TrackOnNotify(
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
			return TrackOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
	}

	return Status;
}

//
// Global track tooltip buffer
//

WCHAR TrackTipBuffer[1024];

HWND
TrackCreateTooltip(
	__in PTRACK_CONTROL Object
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

BOOLEAN
TrackActivateTooltip(
	__in PTRACK_CONTROL Object,
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

VOID
TrackSetCpuCounters(
	__in PTRACK_CONTROL Object,
	__in double CpuMin,
	__in double CpuMax,
	__in double CpuAverage
	)
{
	Object->CpuMin = CpuMin;
	Object->CpuMax = CpuMax;
	Object->CpuAverage = CpuAverage;
}

void
TrackGetCpuCountersByRange(
	__in PTRACK_CONTROL Object,
	__in ULONG First,
	__in ULONG Last,
	__out double *Min,
	__out double *Max,
	__out double *Average
	)
{
	PCPU_HISTORY History;
	double Total;
	double Usage;
	ULONG Count;
	ULONG i, j;
	ULONG Step;

	History = Object->History;

	*Min = *Max = *Average = 0.0;
	Total = 0.0;

	Count = (Last - First + 1) / Object->MergeSteps;

	if (Count == 0) {
		Count = 1;
		Step = Last - First + 1;
	}
	else {
		Step = Object->MergeSteps;
	}

	for(i = 0; i < Count; i += 1) {

		//
		// Compute the merged CPU usage
		//

		Usage = 0.0;
		for(j = 0; j < Step; j++) {
			Usage += History->Value[First + i * Step + j];
		}
		Usage = Usage / Step;
		
		//
		// Update CPU counters
		//

		*Min = min(*Min, Usage);
		*Max = max(*Max, Usage);
		Total += Usage;
	}

	*Average = Total / Count;
}

double
TrackGetCpuUsageByIndex(
	__in PTRACK_CONTROL Object,
	__in ULONG Index
	)
{
	PCPU_HISTORY History;
	ULONG Start, End;
	ULONG Number;
	double Usage;

	History = Object->History;
	Start = Index * Object->MergeSteps;
	End = min(Start + Object->MergeSteps - 1, History->Count - 1);

	Usage = 0.0;
	for(Number = Start; Number <= End; Number += 1) {
		Usage += History->Value[Number];
	}

	return Usage / (End - Start + 1);
}

LRESULT
TrackOnTtnGetDispInfo(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PTRACK_CONTROL Object;
    PCPU_HISTORY History;
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;
	double Min, Max, Average;
	double Usage;
	POINT pt;

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
    History = Object->History;
	if (!History) {
		return 0;
	}

	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);

    //
    // Adjust the point relative to in memory bitmap
    //

    pt.x += GetScrollPos(hWnd, SB_HORZ);

	//
	// If the x fall into slider, show sample range, CPU usage range
	//

	if (pt.x >= Object->rcSlider.left && pt.x < Object->rcSlider.right) {

		TrackGetCpuCountersByRange(Object, Object->FirstSample, Object->LastSample,
								   &Min, &Max, &Average);

		StringCchPrintf(TrackTipBuffer, 1024,
						L"Sample range: (%u, %u)\r\nCPU min: %.2f %%\r\nCPU max: %.2f %%\r\nCPU average: %.2f %%",
						Object->FirstSample, Object->LastSample, 
						Min, Max, Average);
		SendMessage(Object->hWndTooltip, TTM_SETMAXTIPWIDTH, 0, 200);
		lpnmtdi->lpszText = TrackTipBuffer;
	}

	//
	// If the x fall into CPU usage history, show average CPU range 
	//

	else if (pt.x < Object->bmpRect.right) {

		Usage = TrackGetCpuUsageByIndex(Object, (ULONG)pt.x);
		StringCchPrintf(TrackTipBuffer, 1024, L"CPU: %.2f %%", Usage);

		lpnmtdi->lpszText = TrackTipBuffer;
	}

	else {
	}

	return 0;
}

LRESULT
TrackOnMouseMove(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PTRACK_CONTROL Object;
    RECT Rect;
    POINT pt;
	int x, y;

    x = GET_X_LPARAM(lp);
    y = GET_Y_LPARAM(lp);

    Object = (PTRACK_CONTROL)SdkGetObject(hWnd);
	if (!Object->MouseCaptured) {

		pt.x = x;
		pt.y = y;
		ClientToScreen(hWnd, &pt);

        //
        // Adjust point to avoid click issue
        //

		pt.y += 20;

		if (!Object->IsTooltipActivated) {
			TrackActivateTooltip(Object, TRUE);
		}

		SendMessage(Object->hWndTooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

		if (!Object->IsTracking) {
			TRACKMOUSEEVENT Event = {0};
			Event.cbSize = sizeof(Event);
			Event.dwFlags = TME_LEAVE;
			Event.hwndTrack = Object->hWnd;
			TrackMouseEvent(&Event);
			Object->IsTracking = TRUE;
		}

		return 0;
	}

    TrackActivateTooltip(Object, FALSE);

    pt.x = x;
    pt.y = y;
    TrackDrawSlider(Object, &pt);

	GetClientRect(hWnd, &Rect);
	InvalidateRect(hWnd, &Rect, FALSE);
	UpdateWindow(hWnd);

	return 0L;
}

VOID
TrackSetSliderPosition(
	__in PTRACK_CONTROL Object,
	__in int x
	)
{
	POINT pt;
    RECT rc;

   GetClientRect(Object->hWnd, &rc);

    //
    // Simulate mouse click action to position initial slider
    //

    pt.x = 1;
    pt.y = (rc.bottom - rc.top) / 2;
    TrackOnLButtonDown(Object->hWnd, WM_LBUTTONDOWN, 0, MAKELONG(pt.x, pt.y));
    TrackOnLButtonUp(Object->hWnd, WM_LBUTTONUP, 0, MAKELONG(pt.x, pt.y));

    //
    // Set initial scrollbar position
    //

    TrackSetScrollBar(Object->hWnd, Object->Width, rc.right - rc.left);
}