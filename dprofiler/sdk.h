//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _SDK_H_
#define _SDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <strsafe.h>
#include <math.h>
#include <assert.h>
#include "resource.h"

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

//
// Profile Control Messages
//

#define WM_USER_PROFILE_START        WM_USER + 1000
#define	WM_USER_PROFILE_START_PAUSED WM_USER + 1001
#define	WM_USER_PROFILE_TERMINATE    WM_USER + 1002
#define WM_USER_PROFILE_ANALYZED     WM_USER + 1003
#define WM_USER_PROFILE_OPENED       WM_USER + 1004
#define WM_USER_PROFILE_COMPLETED    WM_USER + 1005
#define WM_USER_PROFILE_TERMINATED   WM_USER + 1006

//
// StatusBar Message
//

#define WM_USER_STATUSBAR            WM_USER + 1010

//
// TreeList Control Message
//

#define WM_TREELIST_SORT             WM_USER + 1020
#define WM_TREELIST_SELCHANGED       WM_USER + 1021

//
// wp, hTreeItem
// lp, column index
//

#define WM_TREELIST_DBLCLK           WM_USER + 1022 


//
// Graph Control Message
//

#define WM_GRAPH_SET_RECORD          WM_USER + 1030

//
// Noise Deduction Message
//

#define WM_DEDUCTION_UPDATE          WM_USER + 1040

//
// Source Message
//

#define WM_SOURCE_CLOSE              WM_USER + 1041

//
// FlameGraph Message
//

#define WM_FLAME_QUERYNODE           WM_USER + 1042

//
// Trackbar Message
//

#define WM_TRACK_SET_RANGE           WM_USER + 1043

//
// Used to define invalid value, e.g. invalid index
// invalid pointer
//

#define INVALID_VALUE  ((ULONG)-1)
#define INVALID_PTR    ((ULONG_PTR)-1)

extern HINSTANCE SdkInstance;
extern HANDLE SdkHeapHandle;
extern HICON SdkMainIcon;

ULONG
SdkInitialize(
	IN HINSTANCE Instance	
	);

PVOID
SdkMalloc(
	IN ULONG Length
	);

VOID
SdkFree(
	IN PVOID BaseVa
	);

LONG_PTR
SdkSetObject(
	IN HWND hWnd,
	IN PVOID Object
	);

LONG_PTR
SdkGetObject(
	IN HWND hWnd
	);

VOID
SdkModifyStyle(
	IN HWND hWnd,
	IN ULONG Remove,
	IN ULONG Add,
	IN BOOLEAN ExStyle
	);

HIMAGELIST
SdkCreateGrayImageList(
	IN HIMAGELIST himlNormal
	);

VOID
SdkFillSolidRect(
	IN HDC hdc, 
	IN COLORREF cr, 
	IN RECT *rect
	);

VOID
SdkFillGradient(
	IN HDC hdc,
	IN COLORREF cr1,
	IN COLORREF cr2,
	IN RECT *Rect
	);

VOID
SdkCenterWindow(
	IN HWND hWnd
	);

HICON
SdkBitmapToIcon(
	IN HINSTANCE Instance,
	IN INT BitmapId
	);

BOOL
SdkSetMainIcon(
    IN HWND hWnd
    );

COLORREF 
SdkBrightColor(
    IN COLORREF Color,
    IN UCHAR Increment
    );

VOID 
SdkCopyControlRect(
    IN HWND hWndParent,
    IN HWND hWndFrom,
    IN HWND hWndTo
    );

HFONT
SdkSetDefaultFont(
	IN HWND hWnd
	);

HFONT
SdkSetBoldFont(
	IN HWND hWnd
	);

BOOLEAN
SdkCopyFile(
	IN PWSTR Destine,
	IN PWSTR Source
	);

BOOLEAN
SdkCreateIconFile(
	IN HICON hIcon, 
	IN LONG Bits,
	IN PWSTR Path
	);

BOOLEAN
SdkSaveBitmap(
	IN HBITMAP hbitmap, 
	IN PWSTR filename, 
	IN int nColor
	);

VOID
SdkGetSymbolPath(
    __in PWCHAR Buffer,
    __in ULONG Length
    );

HFONT
SdkCreateListBoldFont(
	VOID
	);

VOID
SdkGetProductName(
	__in PWSTR Buffer,
	__in SIZE_T Length
	);

VOID
SdkGetCopyright(
	__in PWSTR Buffer,
	__in SIZE_T Length
	);

//
// MessageBox Flags 
//

#define SDK_MB_YES    0x01
#define SDK_MB_NO     0x02
#define SDK_MB_OK     0x04
#define SDK_MB_CANCEL 0x08
#define SDK_MB_CLOSE  0x10

ULONG
SdkMessageBox(
	__in HWND hWndParent,
	__in PWSTR Message,
	__in ULONG Style
	);

//
// Access Macros
//

#define SdkGetContext(_O, _T) \
	((_T *)_O->Context)

#define SdkSetContext(_O, _C) \
	(_O->Context = _C)


//
// Colors for font or background, foreground rendering
//

#define WHITE  (RGB(255, 255, 255))
#define BLACK  (RGB(0, 0, 0))
#define GREEN  (RGB(0, 200, 0))
#define RED    (RGB(200, 0, 0))
#define BLUE   (RGB(0, 0, 200))

VOID
DebugTrace(
	__in PSTR Format,
	__in ...
	);

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(_E) assert(_E)
#else
#define ASSERT(_E)
#endif
#endif

#if defined(_AMD64_)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#if defined(_X86_)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#pragma comment(lib, "comctl32.lib")

#ifdef __cplusplus
}
#endif

//
// The followings use C++ linkage
//

#endif