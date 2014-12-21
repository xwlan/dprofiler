////
//// Apsara Labs
//// lan.john@gmail.com
//// Copyright(C) 2009-2010
////
//
//#include "toolbar.h"
//
//#define SDK_TOOLBAR_STYLE  (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | \
//	                        TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_AUTOSIZE | TBSTYLE_LIST | CCS_TOP)
//
//TBBUTTON ToolBarButtons[ToolBarNumber] = {
//	{ 0, IDM_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 1, IDM_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
//	{ 2, IDM_FILE_EXPORT, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 3, IDM_FILE_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 4, IDM_MANUAL_PROBE,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 5, IDM_ADDON_PROBE,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 6, IDM_STOP_PROBE,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 7, IDM_RUN_PROBE,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
//	{ 8, IDM_STATISTICS,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 9, IDM_PERFORMANCE, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 10, IDM_FIND_RECORD, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
//	{ 11, ID_FILTER_EDITFILTER,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0}, 
//	{ 12, ID_FILTER_RESETFILTER,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 13, ID_FILTER_IMPORTFILTER,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//	{ 14, ID_FILTER_EXPORTFILTER,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
//}; 
//
//TOOLBAR_OBJECT ToolBarObject = {
//	TRUE, NULL, NULL, 2, ToolBarNumber, ToolBarButtons, NULL, NULL
//};
//
//HWND
//ToolBarCreate(
//	IN PTOOLBAR_OBJECT ToolBar
//	)
//{
//	HWND hWnd;
//	TBBUTTON Tb = {0};
//	TBBUTTONINFO Tbi = {0};
//	int Id;
//	ULONG ExStyle;
//
//	ASSERT(ToolBar != NULL);
//
//	hWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, SDK_TOOLBAR_STYLE,
//						  0, 0, 0, 0, ToolBar->hWndParent, (HMENU)ToolBar->CtrlId, SdkInstance, NULL);
//
//    ASSERT(hWnd != NULL);
//
//	ToolBar->hWnd = hWnd;
//    SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);
//	SendMessage(hWnd, TB_ADDBUTTONS, ToolBar->Count, (LPARAM)ToolBar->Button);
//	
//	// test
//
//	ExStyle = (ULONG)SendMessage(hWnd, TB_GETEXTENDEDSTYLE, 0, 0L);
//	ExStyle |= TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS;
//	SendMessage(hWnd, TB_SETEXTENDEDSTYLE, 0, (LPARAM)ExStyle);
//
//	SendMessage(hWnd, TB_GETBUTTON, 0, (LPARAM)&Tb);
//	Id = (int)SendMessage(hWnd, TB_ADDSTRING, 0, (LPARAM)L"Open");
//
//	Tb.iString = Id;
//	Tb.fsStyle |= BTNS_AUTOSIZE | BTNS_SHOWTEXT;
//	SendMessage(hWnd, TB_DELETEBUTTON, 0, 0L);
//	SendMessage(hWnd, TB_INSERTBUTTON, 0, (LPARAM)&Tb);
//
//	Tbi.cbSize = sizeof(Tbi);
//	Tbi.dwMask = TBIF_STYLE;
//	SendMessage(hWnd, TB_GETBUTTONINFO, 0, (LPARAM)&Tbi);
//
//	Tbi.fsStyle |= TBSTYLE_DROPDOWN;
//	SendMessage(hWnd, TB_SETBUTTONINFO, 0, (LPARAM)&Tbi);
//
//	return hWnd;
//}
//
//BOOLEAN
//ToolBarSetImageList(
//	IN PTOOLBAR_OBJECT ToolBar,
//	IN UINT BitmapId, 
//	IN COLORREF Mask
//	)
//{
//    HBITMAP hBitmap;
//	BITMAP Bitmap = {0};
//	LONG Width, Height;
//	HIMAGELIST himlNormal;
//	HIMAGELIST himlGray;
//
//	hBitmap	= LoadBitmap(SdkInstance, MAKEINTRESOURCE(BitmapId));
//    ASSERT(hBitmap != NULL);
//	if (hBitmap == NULL) {
//        return FALSE;
//	}
//    
//	if (!GetObject(hBitmap, sizeof(BITMAP), &Bitmap)) {
//        return FALSE;
//	}
//
//	Width  = Bitmap.bmWidth / ToolBar->Count;
//    Height = Bitmap.bmHeight;
//
//	himlNormal = ImageList_Create(Width, Height, ILC_COLOR32 | ILC_MASK, ToolBar->Count, 0);
//    ASSERT(himlNormal);
//	if (himlNormal == NULL) {
//        return FALSE;
//	}
//
//    ImageList_AddMasked(himlNormal, hBitmap, Mask);
//    SendMessage(ToolBar->hWnd, TB_SETIMAGELIST, 0L, (LPARAM)himlNormal);
//    SendMessage(ToolBar->hWnd, TB_SETHOTIMAGELIST, 0L, (LPARAM)himlNormal);
//
//    DeleteObject(hBitmap);
//    hBitmap = NULL;
//
//	himlGray = SdkCreateGrayImageList(himlNormal);
//    SendMessage(ToolBar->hWnd, TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlGray);
//
//	ToolBar->himlNormal = himlNormal;
//	ToolBar->himlGray = himlGray;
//
//	return TRUE;
//}
//
//BOOLEAN
//ToolBarSetImageList2(
//	IN PTOOLBAR_OBJECT ToolBar,
//	IN PUINT ResourceId, 
//	IN ULONG Count,
//	IN COLORREF Mask
//	)
//{
//	HIMAGELIST himlNormal;
//	HIMAGELIST himlGray;
//	HBITMAP hbmp;
//	HICON hicon;
//
//	ASSERT(Count == ToolBar->Count);
//
//	himlNormal = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, ToolBarNumber, ToolBarNumber);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_OPEN));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAVE));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_EXPORTPROBE));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_IMPORTPROBE));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_MANUAL));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_FILTER));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_STOP_PROBE));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_RUN));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_STATISTICS));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_INFORMATION));
//	ImageList_AddIcon(himlNormal, hicon);
//
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_QUERY_RECORD));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_QUERY_FILTER));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_RESETFILTER));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_IMPORTFILTER));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAVEFILTER));
//	ImageList_AddIcon(himlNormal, hicon);
//	
//    SendMessage(ToolBar->hWnd, TB_SETIMAGELIST, 0L, (LPARAM)himlNormal);
//    SendMessage(ToolBar->hWnd, TB_SETHOTIMAGELIST, 0L, (LPARAM)himlNormal);
//
//	himlGray = SdkCreateGrayImageList(himlNormal);
//    SendMessage(ToolBar->hWnd, TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlGray);
//
//	ToolBar->himlNormal = himlNormal;
//	ToolBar->himlGray = himlGray;
//
//	return TRUE;
//}
//
//VOID
//ToolBarDestroy(
//	IN PTOOLBAR_OBJECT ToolBar
//	)
//{
//	HIMAGELIST himlNormal = (HIMAGELIST)SendMessage(ToolBar->hWnd, TB_GETIMAGELIST, 0L, 0L);
//	HIMAGELIST himlGray = (HIMAGELIST)SendMessage(ToolBar->hWnd, TB_GETDISABLEDIMAGELIST, 0L, 0L);
//
//    if (himlNormal != NULL){
//        ImageList_Destroy(himlNormal);
//    }
//    
//	if (himlGray != NULL){
//        ImageList_Destroy(himlGray);
//    }
//
//}
//BOOLEAN
//ToolBarEnable(
//	IN PTOOLBAR_OBJECT ToolBar,
//	IN ULONG Id
//	)
//{
//	BOOLEAN Status;
//
//	Status = (BOOLEAN)SendMessage(ToolBar->hWnd, 
//								  TB_ENABLEBUTTON, 
//								  (WPARAM)Id, 
//								  (LPARAM)MAKELONG(TRUE, 0));
//
//	return Status;
//}
//
//BOOLEAN
//ToolBarDisable(
//	IN PTOOLBAR_OBJECT ToolBar,
//	IN ULONG Id
//	)
//{
//	BOOLEAN Status;
//
//	Status = (BOOLEAN)SendMessage(ToolBar->hWnd, 
//		                 TB_ENABLEBUTTON, 
//			 		     (WPARAM)Id, 
//						 (LPARAM)MAKELONG(FALSE, 0));
//
//	return Status;
//}
//
//LONG
//ToolBarGetState(
//	IN PTOOLBAR_OBJECT ToolBar,
//	IN ULONG Id
//	)
//{
//	LONG State;
//
//	State = (LONG)SendMessage(ToolBar->hWnd, TB_GETSTATE, (WPARAM)Id, 0L);
//	return State;
//}
//
//VOID
//ToolbarInsertText(
//	__in HWND hWndToolbar,
//	__in UINT Id
//	)
//{
//	WCHAR Buffer[MAX_PATH];
//
//	LoadString(SdkInstance, Id, Buffer, MAX_PATH); 
//}