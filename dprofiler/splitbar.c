//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#include "sdk.h"
#include "splitbar.h"

SPLITBAR_OBJECT SplitBar = {
	TRUE, FALSE, FALSE, FALSE, { 0, 0 }, -4, 100, 2
};

VOID
SplitBarDrawBar(
	IN HDC hdc, 
	IN int x1, 
	IN int y1, 
	IN int width, 
	IN int height
	)
{
	WORD Pattern[8] = { 
		0x00aa, 0x0055, 0x00aa, 0x0055, 
		0x00aa, 0x0055, 0x00aa, 0x0055
	};

	HBITMAP hBitmap;
	HBRUSH  hBrush;
	HBRUSH  hBrushOld;

	hBitmap = CreateBitmap(8, 8, 1, 1, Pattern);
	hBrush = CreatePatternBrush(hBitmap);
	
	SetBrushOrgEx(hdc, x1, y1, 0);
	hBrushOld = (HBRUSH)SelectObject(hdc, hBrush);
	
	PatBlt(hdc, x1, y1, width, height, PATINVERT);
	
	SelectObject(hdc, hBrushOld);
	
	DeleteObject(hBrush);
	DeleteObject(hBitmap);
}

LRESULT 
SplitBarOnMouseMove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HDC hdc;
	RECT rect;

	POINT pt;

	if (SplitBar.DragMode == FALSE) {
		return 0;
	}

	pt.x = (short)LOWORD(lp);
	pt.y = (short)HIWORD(lp);

	GetWindowRect(hWnd, &rect);

	ClientToScreen(hWnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;

	OffsetRect(&rect, -rect.left, -rect.top);

	if (pt.y < 0) {
		pt.y = 0;
	}

	if (pt.y > rect.bottom - 4)  {
		pt.y = rect.bottom - 4;
	}

	if(pt.y != SplitBar.Top && (wp & MK_LBUTTON)) {

		hdc = GetWindowDC(hWnd);
		
		SplitBarDrawBar(hdc, 1, SplitBar.Top - 2, rect.right - 2, 4);
		SplitBarDrawBar(hdc, 1, pt.y - 2, rect.right - 2, 4);
			
		ReleaseDC(hWnd, hdc);

		SplitBar.Top = pt.y;
	}

	return 0;
}

LRESULT 
SplitBarOnLButtonDown(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	POINT pt;
	HDC hdc;
	RECT rect;

	pt.x = (short)LOWORD(lp);
	pt.y = (short)HIWORD(lp);

	GetWindowRect(hWnd, &rect);

	ClientToScreen(hWnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;
	
	OffsetRect(&rect, -rect.left, -rect.top);
	
	if(pt.y < 0) {
		pt.y = 0;
	}

	if(pt.y > rect.bottom - 4) {
		pt.y = rect.bottom - 4;
	}

	SplitBar.DragMode = TRUE;

	SetCapture(hWnd);

	hdc = GetWindowDC(hWnd);
	SplitBarDrawBar(hdc, 1, pt.y - 2, rect.right - 2, 4);
	ReleaseDC(hWnd, hdc);
	
	SplitBar.Top = pt.y;
		
	return 0;
}

LRESULT 
SplitBarOnLButtonUp(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HDC hdc;
	RECT rect;
	POINT pt;

	pt.x = (short)LOWORD(lp);
	pt.y = (short)HIWORD(lp);

	if (SplitBar.DragMode == FALSE) {
		return 0;
	}
	
	GetWindowRect(hWnd, &rect);

	ClientToScreen(hWnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;
	
	OffsetRect(&rect, -rect.left, -rect.top);

	if(pt.y < 0) {
		pt.y = 0;
	}

	if(pt.y > rect.bottom - 4) {
		pt.y = rect.bottom - 4;
	}

	hdc = GetWindowDC(hWnd);
	SplitBarDrawBar(hdc, 1, SplitBar.Top - 2, rect.right - 2, 4);			
	ReleaseDC(hWnd, hdc);

	SplitBar.Top = pt.y;
	SplitBar.DragMode = FALSE;

	GetWindowRect(hWnd, &rect);
	pt.x += rect.left;
	pt.y += rect.top;

	ScreenToClient(hWnd, &pt);
	GetClientRect(hWnd, &rect);
	SplitBar.Position = pt.y;
	
	SplitBarAdjustPosition(rect.right, rect.bottom);

	ReleaseCapture();
	return 0;
}

VOID
SplitBarAdjustPosition(
	IN LONG x,
	IN LONG y
	)
{
	MoveWindow(SplitBar.hWnd[0], 
		       0, 
			   0, 
			   x, 
			   SplitBar.Position, 
			   TRUE);

	MoveWindow(SplitBar.hWnd[1], 
		       0, 
			   SplitBar.Position + SplitBar.Border, 
		       x, 
			   y - SplitBar.Position - SplitBar.Border, 
			   TRUE);
}