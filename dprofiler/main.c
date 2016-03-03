//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#include "sdk.h"
#include "resource.h"
#include "main.h"
#include "frame.h"

HWND hWndMain;
FRAME_OBJECT MainFrame = {0};

LPCWSTR APP_TITLE = L"D Profile"; 

BOOLEAN
MainInitialize(
	VOID
	)
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	return TRUE;
}

int
MainMessagePump(
	VOID
	)
{
	HACCEL hAccel;
	MSG msg;

	//
	// Create main window
	//

	hWndMain = FrameCreate(&MainFrame);
	ASSERT(hWndMain != NULL);

	hAccel = LoadAccelerators(SdkInstance, MAKEINTRESOURCE(IDC_DPROFILE));
	ASSERT(hAccel != NULL);

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWndMain, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

struct _FRAME_OBJECT*
MainGetFrame(
	VOID
	)
{
	return &MainFrame;
}