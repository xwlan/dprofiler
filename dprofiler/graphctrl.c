//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#include "graphctrl.h"
#include "aps.h"

#define SDK_GRAPH_CLASS   L"SdkGraph"

BOOLEAN 
GraphInitialize(
    VOID
    )
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = GraphProcedure;
    wc.hInstance = SdkInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = SDK_GRAPH_CLASS;

    if (!RegisterClassEx(&wc)) {
        return FALSE;
    }

    return TRUE;
}

PGRAPH_CONTROL
GraphCreateControl(
    __in HWND hWndParent,
    __in INT_PTR CtrlId,
    __in LPRECT Rect,
    __in COLORREF crBack,
    __in COLORREF crGrid,
    __in COLORREF crFill,
    __in COLORREF crEdge
    )
{
    HWND hWnd;
    PGRAPH_CONTROL Object;

    Object = (PGRAPH_CONTROL)SdkMalloc(sizeof(GRAPH_CONTROL));
    ZeroMemory(Object, sizeof(GRAPH_CONTROL));

    Object->CtrlId = CtrlId;
    Object->BackColor = crBack;
    Object->GridColor = crGrid;
    Object->FillColor = crFill;
    Object->EdgeColor = crEdge;
    Object->FirstPos = 12;

    hWnd = CreateWindowEx(0, SDK_GRAPH_CLASS, L"", 
                          WS_VISIBLE | WS_CHILD, 
                          Rect->left, Rect->top, 
                          Rect->right - Rect->left, 
                          Rect->bottom - Rect->top, 
                          hWndParent, (HMENU)CtrlId, 
                          SdkInstance, Object);
    return Object;
}

VOID
GraphInitializeControl(
    __in HWND hWnd,
    __in INT_PTR CtrlId,
    __in COLORREF crBack,
    __in COLORREF crGrid,
    __in COLORREF crFill,
    __in COLORREF crEdge
    )
{
    PGRAPH_CONTROL Object;

    Object = (PGRAPH_CONTROL)SdkMalloc(sizeof(GRAPH_CONTROL));
    ZeroMemory(Object, sizeof(GRAPH_CONTROL));

    Object->CtrlId = CtrlId;
    Object->BackColor = crBack;
    Object->GridColor = crGrid;
    Object->FillColor = crFill;
    Object->EdgeColor = crEdge;
    Object->FirstPos = 12;

    Object->hWnd = hWnd;
    SdkSetObject(hWnd, Object);
}

VOID
GraphSetHistoryData(
    __in PGRAPH_CONTROL Control,
    __in PGRAPH_HISTORY History
    )
{
    Control->History = History; 
}


LRESULT
GraphOnCreate(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PGRAPH_CONTROL Object;
    LPCREATESTRUCT lpCreate;

    lpCreate = (LPCREATESTRUCT)lp;
    Object = (PGRAPH_CONTROL)lpCreate->lpCreateParams;

    if (Object != NULL) {
        Object->hWnd = hWnd;
        SdkSetObject(hWnd, Object);
    }

    return 0;
}

LRESULT
GraphOnPaint(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PAINTSTRUCT Paint;
    PGRAPH_CONTROL Object;
    HDC hdc;
    RECT Rect;

    Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    GetClientRect(hWnd, &Rect);

    //
    // Draw in memory dc 
    //

    GraphDraw(Object);

    hdc = BeginPaint(hWnd, &Paint);
    BitBlt(hdc, 0, 0, Rect.right, Rect.bottom, Object->hdcGraph, 0, 0, SRCCOPY);
    EndPaint(hWnd, &Paint);

    return 0;
}

LRESULT
GraphOnSize(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PGRAPH_CONTROL Object;

    Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    GraphInvalidate(hWnd);
    return 0;
}

LRESULT
GraphOnDestroy(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PGRAPH_CONTROL Object;

    Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
    ASSERT(Object != NULL);

    if (Object->hbmpGraph) {
        DeleteObject(Object->hbmpGraph);
    }

    if (Object->hdcGraph) {
        DeleteDC(Object->hdcGraph);
    }

    SdkFree(Object);
    SdkSetObject(hWnd, NULL);
    return 0L;
}

LRESULT CALLBACK 
GraphProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    switch (uMsg) {

    case WM_CREATE:
        GraphOnCreate(hWnd, uMsg, wp, lp);
        break;

    case WM_DESTROY:
        GraphOnDestroy(hWnd, uMsg, wp, lp);
        break;

    case WM_SIZE:
        GraphOnSize(hWnd, uMsg, wp, lp);
        break;

    case WM_PAINT:
        GraphOnPaint(hWnd, uMsg, wp, lp); 
        break;

    case WM_ERASEBKGND:
        return TRUE;

	case WM_LBUTTONDOWN:
		GraphOnLButtonDown(hWnd, uMsg, wp, lp);
		break;
    }

    return DefWindowProc(hWnd, uMsg, wp, lp);
}

BOOLEAN
GraphInvalidate(
    __in HWND hWnd
    )
{
    HDC hdc;
    HDC hdcGraph;
    HBITMAP hbmp;
    HBITMAP hbmpOld;	
    HBRUSH hBrush;
    PGRAPH_CONTROL Object;
    RECT Rect;
    LONG Width, Height;
    POINT Point;
    HPEN hPen;
    LONG Grid;
    int i;

    Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
    ASSERT(Object != NULL);

    if (Object->hbmpGraph) {
        ASSERT(Object->hdcGraph != NULL);
        DeleteObject(Object->hbmpGraph);
        SelectObject(Object->hdcGraph, Object->hbmpGraphOld);
    }

    if (Object->hdcGraph) {
        DeleteDC(Object->hdcGraph);
    }

    hdc = GetDC(hWnd);
    hdcGraph = CreateCompatibleDC(hdc);

    GetClientRect(hWnd, &Rect);
    Width = Rect.right - Rect.left;
    Height = Rect.bottom - Rect.top;

    hbmp = CreateCompatibleBitmap(hdc, Width, Height);
    hbmpOld = (HBITMAP)SelectObject(hdcGraph, hbmp);

    hBrush = CreateSolidBrush(Object->BackColor);
    FillRect(hdcGraph, &Rect, hBrush);
    DeleteObject(hBrush);

    hPen = CreatePen(PS_SOLID, 1, Object->GridColor);
    SelectObject(hdcGraph, hPen);

    Grid = Height / 10;
    for(i = Rect.top; i < Rect.bottom; i += Grid) {
        MoveToEx(hdcGraph, Rect.left, i, &Point);
        LineTo(hdcGraph, Rect.right, i);
    }

    MoveToEx(hdcGraph, 0, 0, NULL);
    LineTo(hdcGraph, 0, Rect.bottom);

    MoveToEx(hdcGraph, Rect.right - 1, 0, NULL);
    LineTo(hdcGraph, Rect.right - 1, Rect.bottom - 1);

    DeleteObject(hPen);

    Object->hdcGraph = hdcGraph;
    Object->hbmpGraph = hbmp;
    Object->hbmpGraphOld = hbmpOld;

    
    InvalidateRect(hWnd, &Rect, TRUE);
    return TRUE;
}

VOID
GraphDraw(
    __in PGRAPH_CONTROL Object
    )
{
    RECT Rect;
    HDC hdc;
    HBRUSH hBrush;
    HBRUSH hBrushOld;
    HPEN hPen, hPenOld;
    HRGN hPolygon;
    LONG Width, Height;
    LONG Grid;
    POINT *Point;
    PGRAPH_HISTORY History;
    FLOAT Data;
    LONG Count;
    LONG Number;
    FLOAT *Value;
    FLOAT Limits;
    LONG Step;

    if (!Object || !Object->hdcGraph) {
        return;
    }

    GetClientRect(Object->hWnd, &Rect);

    Width = Rect.right - Rect.left;
    Height = Rect.bottom - Rect.top;

    //
    // N.B. Although for most cases 1 is the horizonal step, however, in some special cases,
    // user did not collect enough samples, the step can be greater than 1, we ensure that
    // under any condition, the step is correctly computed.
    //

    History = Object->History;
    if (!History) {
        return;
    }

    Step = ApsComputeClosestLong((FLOAT)Width/(FLOAT)History->Count);
    Object->HoriStep = Step;

    Grid = Width / 10;
    Object->VertStep = Grid;

    hdc = Object->hdcGraph;

    
    //
    // Fill the background
    //

    hBrush = CreateSolidBrush(Object->BackColor);
    hBrushOld = (HBRUSH)SelectObject(hdc, hBrush);
    FillRect(hdc, &Rect, hBrush);

    DeleteObject(hBrush);
    SelectObject(hdc, hBrushOld);

    Object->FirstPos -= Step;
    if(Object->FirstPos < 0) {
        Object->FirstPos += Grid;
    }

    /*
    //
    // Draw the grid
    //

    hPen = CreatePen(PS_SOLID, 1, Object->GridColor);
    SelectObject(hdc, hPen);

    Grid = Height / 10;
    for(i = Rect.top; i < Rect.bottom; i += Grid) {
        MoveToEx(hdc, Rect.left, i, &Point);
        LineTo(hdc, Rect.right, i);
    }

    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, 0, Rect.bottom);

    MoveToEx(hdc, Rect.right - 1, 0, NULL);
    LineTo(hdc, Rect.right - 1, Rect.bottom - 1);

    DeleteObject(hPen);
    */

    //
    // Translate the values into bounded values
    //

    Value = History->Value;
    Limits = History->Limits;
    Count = History->Count;

    Point = (POINT *)SdkMalloc(sizeof(POINT) * (Count + 4));
    for (Number = 0; Number < Count; Number += 1) {
        Point[Number].x = Rect.left + Number * Step;	
        Data = (FLOAT)Height * ((FLOAT)1.0 - Value[Number] / Limits);
        Point[Number].y = ApsComputeClosestLong(Data);
    }

    Point[Count].x = Rect.right;
    Point[Count + 1].x = Rect.right;
    Point[Count + 1].y = Rect.bottom;
    Point[Count + 2].x = Point[0].x;
    Point[Count + 2].y = Rect.bottom;

    //
    // Fill the ploygon region combined by sample points
    //

    hBrush = CreateSolidBrush(Object->FillColor);
    hBrushOld = (HBRUSH)SelectObject(hdc, hBrush);

    hPolygon = CreatePolygonRgn(Point, Count + 3, ALTERNATE);
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
    Polyline(hdc, Point, Count + 1);

    SdkFree(Point);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

    //
    // Draw the current marker
    //

    hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    hPenOld = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, Object->Current, Rect.top + 1, NULL);
    LineTo(hdc, Object->Current, Rect.bottom - 1);
    SelectObject(hdc, hPenOld);
    DeleteObject(hPen);

    InvalidateRect(Object->hWnd, &Rect, TRUE);
}

LRESULT
GraphOnLButtonDown(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PGRAPH_CONTROL Object;
    RECT Rect;

    Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
    Object->Current = GET_X_LPARAM(lp); 

    //
    // Notify parent window the current record number
    //

    PostMessage(GetParent(hWnd), WM_GRAPH_SET_RECORD, (WPARAM)Object->Current, 0);

    GetClientRect(hWnd, &Rect);
    InvalidateRect(hWnd, &Rect, TRUE);
	return 0L;
}