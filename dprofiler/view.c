//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "sdk.h"
#include "view.h"

PWSTR SdkViewClass = L"SdkView";

BOOLEAN
ViewInitialize(
	VOID
	)
{
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);

	SdkMainIcon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_APSARA));
    
	wc.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = ViewProcedure;
    wc.lpszClassName  = SdkViewClass;
	wc.lpszMenuName   = MAKEINTRESOURCE(IDC_DPROFILER);

    RegisterClassEx(&wc);
    return TRUE;
}

LRESULT
ViewOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT 
ViewOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
ViewOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
ViewOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
ViewOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
ViewOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT CALLBACK
ViewProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}