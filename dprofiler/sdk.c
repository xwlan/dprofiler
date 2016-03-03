//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "sdk.h"
#include "graphctrl.h"
#include "treelist.h"
#include "split.h"
#include "flamegraph.h"
#include "trackbar.h"
#include "statecolor.h"
#include <shellapi.h>

HINSTANCE SdkInstance;
HANDLE SdkHeapHandle;
HICON SdkMainIcon;

ULONG
SdkInitialize(
	IN HINSTANCE Instance
	)
{
	BOOLEAN Status;
	INITCOMMONCONTROLSEX CommonCtrls;

	SdkInstance = Instance;

	CommonCtrls.dwSize = sizeof(CommonCtrls);
	CommonCtrls.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES |
						ICC_WIN95_CLASSES;

	Status = InitCommonControlsEx(&CommonCtrls);
	ASSERT(Status);

	if (Status != TRUE) {
		return GetLastError();
	}
	
	//
	// Support richedit hosted in dialog
	//

	LoadLibrary(L"riched32.dll");
	SdkHeapHandle = HeapCreate(0, 0, 0);

	if (SdkHeapHandle == NULL) {
		return GetLastError();
	}

	Status = GraphInitialize();
	if (Status != TRUE) {
		return GetLastError();
	}

	Status = TrackInitialize();
	if (Status != TRUE) {
		return GetLastError();
	}

	Status = TreeListInitialize();
	if (Status != TRUE) {
		return GetLastError();
	}

	Status = SplitInitialize(SdkInstance);
	if (Status != TRUE) {
		return GetLastError();
	}

    Status = FlameInitialize();
	if (Status != TRUE) {
		return GetLastError();
	}

    Status = StateColorInitialize();
	if (Status != TRUE) {
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

PVOID
SdkMalloc(
	IN ULONG Length
	)
{
	PVOID BaseVa;
	ASSERT(SdkHeapHandle != NULL);
	BaseVa = HeapAlloc(SdkHeapHandle, HEAP_ZERO_MEMORY, Length);
	return BaseVa;
}

VOID
SdkFree(
	IN PVOID BaseVa
	)
{
	ASSERT(SdkHeapHandle != NULL);
	HeapFree(SdkHeapHandle, 0, BaseVa);
}

LONG_PTR
SdkSetObject(
	IN HWND hWnd,
	IN PVOID Object
	) 
{
	LONG_PTR Value;

	ASSERT(IsWindow(hWnd));

#if defined (_M_IX86)
	Value = (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_USERDATA, PtrToUlong(Object));

#elif defined (_M_X64)
	Value = (LONG_PTR)SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)Object);
#endif

	return Value;
}

LONG_PTR
SdkGetObject(
	IN HWND hWnd
	)
{
	return (LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

VOID
SdkModifyStyle(
	IN HWND hWnd,
	IN ULONG Remove,
	IN ULONG Add,
	IN BOOLEAN ExStyle
	)
{
	ULONG Style;

	if (!ExStyle) {
		Style = GetWindowLong(hWnd, GWL_STYLE);
	}
	else {
		Style = GetWindowLong(hWnd, GWL_EXSTYLE);
	}

	if (Remove != 0) {
		Style &= ~Remove;
	}

	if (Add != 0) {
		Style |= Add;
	}

	if (!ExStyle) {
		SetWindowLong(hWnd, GWL_STYLE, Style);
	}
	else {
		SetWindowLong(hWnd, GWL_EXSTYLE, Style);
	}
}

VOID
SdkFillSolidRect(
	IN HDC hdc, 
	IN COLORREF cr, 
	IN RECT *rect
	)
{
    COLORREF oldColor;
	
	oldColor = SetBkColor(hdc, cr);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, rect, NULL, 0, NULL);
	SetBkColor(hdc, oldColor);
}

VOID
SdkFillGradient(
	IN HDC hdc,
	IN COLORREF cr1,
	IN COLORREF cr2,
	IN RECT *Rect
	)
{
	int r1, g1, b1;
	int r2, g2, b2;
	int width,height;
	int r,g,b;
	int i, j;

	r1 = GetRValue(cr1);
	g1 = GetGValue(cr1);
	b1 = GetBValue(cr1);
	r2 = GetRValue(cr2);
	g2 = GetGValue(cr2);
	b2 = GetBValue(cr2);

	width = Rect->right - Rect->left;
	height = Rect->bottom - Rect->top;

	for(i = 0; i < width; i++) { 

		r = r1 + (i * (r2 - r1) / width);
		g = g1 + (i * (g2 - g1) / width);
		b = b1 + (i * (b2 - b1) / width);

		for(j = 0; j < height; j++) {
			SetPixel(hdc, i, j, RGB(r,g,b));
		}
	}
}


HIMAGELIST
SdkCreateGrayImageList(
	IN HIMAGELIST himlNormal
	)
{
	int Count, i;
	int Width, Height;
	HIMAGELIST himlGray;
	HDC hdcDesktop;
	HDC hdcMem;
	RECT rc;
	COLORREF crMask;
	HPALETTE hpal;
	UINT index;
	HGDIOBJ hbm;
	HGDIOBJ hbmOld;
	COLORREF rgb;
	BYTE gray;
	HWND hWnd;
	int x, y;

	Count = ImageList_GetImageCount(himlNormal);
	if (Count == 0) {
		return NULL;
	}

	ImageList_GetIconSize(himlNormal, &Width, &Height);
	himlGray = ImageList_Create(Width, Height, ILC_COLOR24 | ILC_MASK, Count, 0);

	hdcDesktop = GetDC(NULL);
	hdcMem = CreateCompatibleDC(NULL);
	
	rc.top = rc.left = 0;
	rc.bottom = Height;
	rc.right = Width;
	crMask = RGB(200, 199, 200);

	if (GetDeviceCaps(hdcDesktop, BITSPIXEL) < 24) {
		hpal = (HPALETTE)GetCurrentObject(hdcDesktop, OBJ_PAL);
		index = GetNearestPaletteIndex(hpal, crMask);
		if (index != CLR_INVALID) { 
			crMask = PALETTEINDEX(index);
		}
	}

	for (i = 0 ; i < Count; ++i) {

		hbm = CreateCompatibleBitmap(hdcDesktop, Width, Height);
		hbmOld = SelectObject(hdcMem, hbm);

		SdkFillSolidRect(hdcMem, crMask, &rc);

		ImageList_SetBkColor(himlNormal, crMask);
		ImageList_Draw(himlNormal, i, hdcMem, 0, 0, ILD_NORMAL);

		for (x = 0 ; x < Width; ++x) {
			for (y = 0; y < Height; ++y) {
				rgb = GetPixel(hdcMem, x, y);
				if (rgb != crMask) { 
					gray = (BYTE) (95 + (GetRValue(rgb) * 3 + GetGValue(rgb) * 6 + GetBValue(rgb)) / 20);
					SetPixel(hdcMem, x, y, RGB(gray, gray, gray));
				}
			}
		}

		hbm = SelectObject(hdcMem, hbmOld);
		ImageList_AddMasked(himlGray, (HBITMAP)hbm, crMask);
		DeleteObject(hbm);
	}

	DeleteDC(hdcMem);
	hWnd = WindowFromDC(hdcDesktop);
	ReleaseDC(hWnd, hdcDesktop);

	return himlGray; 
}

VOID
SdkCenterWindow(
	IN HWND hWnd
	)
{
	HWND hWndParent;
	RECT rect, rectParent;
	int width, height;      
	int screenWidth, screenHeight;
	int x, y;

	GetWindowRect(hWnd, &rect);
	hWndParent = GetParent(hWnd);
	
	if (hWndParent == NULL) {
		
		width  = rect.right - rect.left;
		height = rect.bottom - rect.top;
		screenWidth = GetSystemMetrics(SM_CXSCREEN);
		screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
		x = (screenWidth - width) / 2;
		y = (screenHeight - height) / 2;

		MoveWindow(hWnd, x, y, width, height, FALSE);
		return;
	}

	GetWindowRect(hWndParent, &rectParent);

	width  = rect.right - rect.left;
	height = rect.bottom - rect.top;

	x = ((rectParent.right  - rectParent.left) - width)  / 2 + rectParent.left;
	y = ((rectParent.bottom - rectParent.top)  - height) / 2 + rectParent.top;

	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (x < 0) {
		x = 0;
	}

	if (y < 0) {
		y = 0;
	}

	if (x + width > screenWidth) {
		x = screenWidth - width;
	}

	if (y + height > screenHeight) {
		y = screenHeight - height;
	}

	MoveWindow(hWnd, x, y, width, height, FALSE);
}

HICON
SdkBitmapToIcon(
	IN HINSTANCE Instance,
	IN INT BitmapId
	)
{
	HRSRC hResource;
	HGLOBAL hGlobal;
	HICON hIcon;
	PVOID Bits;

	hResource = FindResource(Instance, MAKEINTRESOURCE(BitmapId), RT_BITMAP);
	hGlobal = LoadResource(Instance, hResource); 
	Bits = LockResource(hGlobal); 
 
	hIcon = CreateIconFromResourceEx(
					(PUCHAR) Bits, 
					SizeofResource(Instance, hResource), 
					TRUE, 0x00030000, 32, 32, LR_DEFAULTCOLOR); 
 
	return hIcon;

}

BOOL
SdkSetMainIcon(
    IN HWND hWnd
    )
{
    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)SdkMainIcon);
    SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)SdkMainIcon);
	return TRUE;
}

COLORREF 
SdkBrightColor(
    IN COLORREF Color,
    IN UCHAR Increment
    )
{
    UCHAR R, G, B;

    R = (UCHAR)Color;
    G = (UCHAR)(Color >> 8);
    B = (UCHAR)(Color >> 16);

    if (R <= 255 - Increment)
        R += Increment;
    else
        R = 255;

    if (G <= 255 - Increment)
        G += Increment;
    else
        G = 255;

    if (B <= 255 - Increment)
        B += Increment;
    else
        B = 255;

    return RGB(R, G, B);
} 

VOID 
SdkCopyControlRect(
    IN HWND hWndParent,
    IN HWND hWndFrom,
    IN HWND hWndTo
    )
{
	HWND hWndDesktop;
    RECT Rect;

	hWndDesktop = GetDesktopWindow();
    GetWindowRect(hWndFrom, &Rect);
	MapWindowRect(hWndDesktop, hWndParent, &Rect);
    MoveWindow(hWndTo, Rect.left, Rect.top, 
			   Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);
}

HFONT
SdkSetDefaultFont(
	IN HWND hWnd
	)
{
	HFONT hFont;	

	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
	return hFont;
}

HFONT
SdkSetBoldFont(
	IN HWND hWnd
	)
{
	HFONT hFont;	
	LOGFONT LogFont;

	hFont = (HFONT)GetStockObject(SYSTEM_FONT);
	GetObject(hFont, sizeof(LOGFONT), &LogFont);
	LogFont.lfWeight = FW_BOLD;
	hFont = CreateFontIndirect(&LogFont);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
	return hFont;
}

HFONT
SdkCreateListBoldFont(
	VOID
	)
{
	HFONT hFont;
    LOGFONT LogFont = {0};

    LogFont.lfHeight = -11;
    LogFont.lfWeight = FW_BOLD;
    wcscpy_s(LogFont.lfFaceName, LF_FACESIZE, L"MS Shell Dlg 2");

	hFont = CreateFontIndirect(&LogFont);
	return hFont;
}

BOOLEAN
SdkCopyFile(
	IN PWSTR Destine,
	IN PWSTR Source
	)
{
	SHFILEOPSTRUCT FileOp = {0};

	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
	FileOp.pFrom = Source;
	FileOp.pTo = Destine;
	FileOp.wFunc = FO_COPY;

	return !SHFileOperation(&FileOp);
}

BOOLEAN
SdkSaveBitmap(
	IN HBITMAP hBitmap, 
	IN PWSTR Path, 
	IN int Bits 
	)
{

	BITMAP Bitmap;
	HDC    hDC;                        
	DWORD  dwPaletteSize=0,dwBmBitsSize,dwDIBSize, dwWritten;    
	BITMAPFILEHEADER  bmfHdr;                  
	BITMAPINFOHEADER  bi;                          
	LPBITMAPINFOHEADER  lpbi;          
	HANDLE fh, hDib, hPal, hOldPal=NULL;  

	if (Bits <= 8) { 
		dwPaletteSize = (ULONG)((1 << Bits)) * sizeof(RGBQUAD);  
	}

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = Bits;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	dwBmBitsSize = ((Bitmap.bmWidth * Bits + 31)/32*4) * Bitmap.bmHeight; 

	hDib = GlobalAlloc(GHND,dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));  
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);  
	*lpbi = bi;

	hPal = GetStockObject(DEFAULT_PALETTE);      
	if(hPal)  
	{  
		hDC = GetDC(NULL);  
		hOldPal = SelectPalette(hDC, (HPALETTE)hPal, FALSE);  
		RealizePalette(hDC);  
	} 

	GetDIBits(hDC, hBitmap, 0, Bitmap.bmHeight,
		(LPSTR)lpbi + sizeof(BITMAPINFOHEADER)+dwPaletteSize,  
		(BITMAPINFO *)lpbi, DIB_RGB_COLORS);  

	if (hOldPal)  
	{  
		SelectPalette(hDC,   (HPALETTE)hOldPal,   TRUE);  
		RealizePalette(hDC);  
		ReleaseDC(NULL,hDC);  
	}  

	fh   =   CreateFile(Path,   GENERIC_WRITE,    
		0,//not   be   shared  
		NULL,   //cannot   be   inherited  
		CREATE_ALWAYS,  
		FILE_ATTRIBUTE_NORMAL   |   FILE_FLAG_SEQUENTIAL_SCAN,    
		NULL);  

	if   (fh   ==   INVALID_HANDLE_VALUE)  
		return   FALSE;  
	bmfHdr.bfType   =   0x4D42;     //   "BM"  
	dwDIBSize           =   sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+dwPaletteSize+dwBmBitsSize;      
	bmfHdr.bfSize   =   dwDIBSize;  
	bmfHdr.bfReserved1   =   0;  
	bmfHdr.bfReserved2   =   0;  
	bmfHdr.bfOffBits   =   (DWORD)sizeof(BITMAPFILEHEADER)    
		+   (DWORD)sizeof(BITMAPINFOHEADER)+   dwPaletteSize;  

	//write   file   header  
	WriteFile(fh,   (LPSTR)&bmfHdr,   sizeof(BITMAPFILEHEADER),   &dwWritten,   NULL);  

	//write   bmp   data  
	WriteFile(fh,   (LPSTR)lpbi,   dwDIBSize,   &dwWritten,   NULL);  

	GlobalUnlock(hDib);  
	GlobalFree(hDib);  
	CloseHandle(fh);  
	DeleteObject(hBitmap);   
	return   TRUE; 
} 

VOID
DebugTrace(
	__in PSTR Format,
	...
	)
{
#ifdef _DEBUG

	va_list arg;
	char Message[512];
	char Buffer[512];
	
	va_start(arg, Format);
	StringCchVPrintfA(Message, 512, Format, arg);
	StringCchPrintfA(Buffer, 512, "[dprofile]: %s\n", Message);
	OutputDebugStringA(Buffer);
	va_end(arg);

#endif
}

VOID
SdkGetSymbolPath(
    __in PWCHAR Buffer,
    __in ULONG Length
    )
{
	ASSERT(0);
}

VOID
SdkGetProductName(
	__in PWSTR Buffer,
	__in SIZE_T Length
	)
{
	WCHAR Product[64];
	WCHAR Version[64];

#ifdef _M_X64
	PCWSTR CPU = L"x64";
#else
	PCWSTR CPU = L"";
#endif

	LoadString(SdkInstance, IDS_PRODUCT, Product, 64);
	LoadString(SdkInstance, IDS_VERSION, Version, 64);

#ifdef _LITE
	StringCchPrintf(Buffer, Length, L"%s Lite %s %s", Product, Version, CPU);
#else
	StringCchPrintf(Buffer, Length, L"%s %s %s", Product, Version, CPU);
#endif

}

VOID
SdkGetCopyright(
	__in PWSTR Buffer,
	__in SIZE_T Length
	)
{
	LoadString(SdkInstance, IDS_COPYRIGHT, Buffer, Length);
}