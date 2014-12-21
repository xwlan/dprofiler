//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#include "aboutdlg.h"
#include "sdk.h"
#include "dialog.h"
#include "listview.h"
#include "split.h"

typedef struct _ABOUT_CONTEXT {
	HFONT hFontBold;
	HBRUSH hBrush;
} ABOUT_CONTEXT, *PABOUT_CONTEXT;

LISTVIEW_COLUMN AboutColumn[] = {
	{ 120, L"Key",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 0,  L"Value",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

ULONG AboutColumnCount = ARRAYSIZE(AboutColumn);


VOID
AboutDialog(
	__in HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	ABOUT_CONTEXT Context = {0};

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_ABOUTBOX;
	Object.Procedure = AboutProcedure;
	
	DialogCreate(&Object);
}

INT_PTR CALLBACK
AboutProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return AboutOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
				return AboutOnOk(hWnd, uMsg, wp, lp);
			}
		
		case WM_CTLCOLORSTATIC:
			return AboutOnCtlColorStatic(hWnd, uMsg, wp, lp);

	}
	
	return Status;
}

LRESULT
AboutOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HWND hWndChild;
	PDIALOG_OBJECT Object;
	PABOUT_CONTEXT Context;
	LVCOLUMN Column = {0};
	LVITEM Item = {0};
	RECT Rect;
	ULONG i;
	WCHAR Buffer[100];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PABOUT_CONTEXT)Object->Context;

	//
	// Create font and brush
	//

	Context->hBrush = CreateSolidBrush(RGB(255, 255, 255));

	hWndChild = GetDlgItem(hWnd, IDC_STATIC_PRODUCT);

	//
	// Set about text
	//

	SdkGetProductName(Buffer, 100);
	SetWindowText(hWndChild, Buffer);
	
	hWndChild = GetDlgItem(hWnd, IDC_STATIC_COPYRIGHT);
	SdkGetCopyright(Buffer, 100);
	SetWindowText(hWndChild, Buffer); 
	
	hWndChild = GetDlgItem(hWnd, IDC_LIST_VERSION);
	GetClientRect(hWndChild, &Rect);

	//
	// Adjust second column width to fill the full row
	//

	AboutColumn[1].Width = Rect.right - Rect.left - AboutColumn[0].Width
		                   + GetSystemMetrics(SM_CXBORDER);

	ListView_SetExtendedListViewStyleEx(hWndChild, 
										LVS_EX_FULLROWSELECT,  
		                                LVS_EX_FULLROWSELECT);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

	for (i = 0; i < AboutColumnCount; i++) { 
        Column.iSubItem = i;
		Column.pszText = AboutColumn[i].Title;	
		Column.cx = AboutColumn[i].Width;     
		Column.fmt = AboutColumn[i].Align;
		ListView_InsertColumn(hWndChild, i, &Column);
    } 

#ifndef _LITE
	Item.iItem = 0;
	Item.mask = LVIF_TEXT;

	Item.iSubItem = 0;
	Item.pszText = L"License";
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = L"Single User";
	ListView_SetItem(hWndChild, &Item);

	Item.iItem = i;
	Item.mask = LVIF_TEXT;

	Item.iItem = 1;
	Item.iSubItem = 0;
	Item.pszText = L"Registration";
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = L"John Lan";
	ListView_SetItem(hWndChild, &Item);

	Item.iItem = 2;
	Item.iSubItem = 0;
	Item.pszText = L"Contact";
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = L"lan.john@gmail.com";
	ListView_SetItem(hWndChild, &Item);

	Item.iItem = 3;
	Item.iSubItem = 0;
	Item.pszText = L"Expiration";
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = L"2013-12-31";
	ListView_SetItem(hWndChild, &Item);

	Item.iItem = 4;
	Item.iSubItem = 0;
	Item.pszText = L"Product Key";
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = L"5e048971048b3e158971088b4e043b2d";
	ListView_SetItem(hWndChild, &Item);

#else
	Item.mask = LVIF_TEXT;
	Item.iItem = 0;
	Item.iSubItem = 0;
	Item.pszText = L"Author";
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = L"lan.john@gmail.com";
	ListView_SetItem(hWndChild, &Item);
#endif

	SetWindowText(hWnd, L"About");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
AboutOnCtlColorStatic(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PABOUT_CONTEXT Context;
	LOGFONT LogFont;
	HFONT hFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PABOUT_CONTEXT)Object->Context;

	if (GetDlgCtrlID((HWND)lp) == IDC_STATIC_PRODUCT) {

		if (!Context->hFontBold) {

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);

			LogFont.lfWeight = FW_BOLD;
			LogFont.lfQuality = CLEARTYPE_QUALITY;
			LogFont.lfCharSet = ANSI_CHARSET;

			Context->hFontBold = CreateFontIndirect(&LogFont);
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)Context->hFontBold, TRUE);

			//
			// N.B. It's required to force a redraw 
			//

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}

	}

	return FALSE;
}

LRESULT
AboutOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PABOUT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, ABOUT_CONTEXT);

	if (Context->hFontBold) {
		DeleteFont(Context->hFontBold);
	}

	if (Context->hBrush) {
		DeleteBrush(Context->hBrush);
	}

	EndDialog(hWnd, IDOK);
	return TRUE;
}