//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

//
// N.B. This is for REBARINFO structure, it has different size
// for XP and VISTA, band insertion will fail if we fill wrong
// size of REBARINFO.
//

#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include "rebar.h"
#include "list.h"
#include "sdk.h"
#include "apsprofile.h"

TBBUTTON CommandBarButtons[] = {
	{ 0, IDM_DEDUCTION,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0},
};

int RebarCommandCount = sizeof(CommandBarButtons) / sizeof(TBBUTTON);

PREBAR_OBJECT
RebarCreateObject(
	__in HWND hWndParent, 
	__in UINT Id 
	)
{
	HWND hWndRebar;
	LRESULT Result;
	PREBAR_OBJECT Object;

	hWndRebar = CreateWindowEx(0, REBARCLASSNAME, NULL,
							   WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_FIXEDORDER |
							   WS_CLIPSIBLINGS | CCS_TOP | CCS_NODIVIDER | RBS_AUTOSIZE| WS_GROUP,
							   0, 0, 0, 0, hWndParent, UlongToHandle(Id), SdkInstance, NULL);
	if (!hWndRebar) {
		return NULL;
	}

	Object = (PREBAR_OBJECT)SdkMalloc(sizeof(REBAR_OBJECT));
	ZeroMemory(Object, sizeof(REBAR_OBJECT));

	Object->hWndRebar = hWndRebar;
	Object->RebarInfo.cbSize  = sizeof(REBARINFO);

	Result = SendMessage(hWndRebar, RB_SETBARINFO, 0, (LPARAM)&Object->RebarInfo);
	if (!Result) {
		SdkFree(Object);
		return NULL;
	}
	
	SendMessage(hWndRebar, RB_SETUNICODEFORMAT, (WPARAM)TRUE, (LPARAM)0);

	InitializeListHead(&Object->BandList);
	return Object;
}

HWND
RebarCreateBar(
	__in PREBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndRebar;
	TBBUTTON Tb = {0};
	TBBUTTONINFO Tbi = {0};

	hWndRebar = Object->hWndRebar;
	ASSERT(hWndRebar != NULL);

	hWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		                  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
	                      TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | 
						  CCS_NODIVIDER | CCS_TOP | CCS_NORESIZE,
						  0, 0, 0, 0, 
		                  hWndRebar, (HMENU)UlongToHandle(REBAR_BAND_COMMAND), SdkInstance, NULL);

    ASSERT(hWnd != NULL);

    SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);
	SendMessage(hWnd, TB_ADDBUTTONS, sizeof(CommandBarButtons) / sizeof(TBBUTTON), (LPARAM)CommandBarButtons);
	SendMessage(hWnd, TB_AUTOSIZE, 0, 0);
	return hWnd;
}

BOOLEAN
RebarSetBarImage(
	__in HWND hWndBar,
	__in COLORREF Mask,
	__out HIMAGELIST *Normal,
	__out HIMAGELIST *Grayed
	)
{
	ULONG Number;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGrayed;
	HICON hicon;
	REBARINFO Info = {0};

	//
	// Create imagelist for toolbar band
	//

	Number = sizeof(CommandBarButtons) / sizeof(TBBUTTON);

	himlNormal = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, Number, Number);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_DEDUCTION));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SEARCH));
	ImageList_AddIcon(himlNormal, hicon);
	
    SendMessage(hWndBar, TB_SETIMAGELIST, 0, (LPARAM)himlNormal);
    SendMessage(hWndBar, TB_SETHOTIMAGELIST, 0, (LPARAM)himlNormal);

	himlGrayed = SdkCreateGrayImageList(himlNormal);
    SendMessage(hWndBar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himlGrayed);

	*Normal = himlNormal;
	*Grayed = himlGrayed;

	//
	// Set rebar's imagelist
	//

	Info.cbSize = sizeof(Info);
	Info.fMask = RBIM_IMAGELIST;
	Info.himl = himlNormal;
    SendMessage(GetParent(hWndBar), RB_SETBARINFO, 0, (LPARAM)&Info);
	return TRUE;
}

HWND
RebarCreateCombo(
	__in PREBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndRebar;

	hWndRebar = Object->hWndRebar;
	ASSERT(hWndRebar != NULL);

	hWnd = CreateWindowEx(0, L"COMBOBOX", NULL, 
                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | ES_LEFT | CBS_DROPDOWNLIST, 
						  0, 0, 0, 0, hWndRebar, (HMENU)REBAR_BAND_SWITCH, SdkInstance, NULL);
    ASSERT(hWnd != NULL);
	return hWnd;
}

VOID
RebarSetType(
	__in PREBAR_OBJECT Object,
	__in BTR_PROFILE_TYPE Type
	)
{
	HWND hWnd;

	hWnd = GetDlgItem(Object->hWndRebar, REBAR_BAND_SWITCH);

	switch (Type) {

		case PROFILE_CPU_TYPE:

			SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Summary");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"IPs On CPU");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Function");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Module");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Thread");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Call Tree");
#ifndef _LITE
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Flame Graph");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"History");
#endif
			SendMessage(hWnd, CB_SETCURSEL, 0, 0);
			break;

		case PROFILE_MM_TYPE:

			SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Summary");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Outstanding Allocation");
#ifndef _LITE
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Heap Allocation By Module");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Heap Allocation Call Tree");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Heap Allocation Flame Graph");
#endif
			SendMessage(hWnd, CB_SETCURSEL, 0, 0);
			break;

		case PROFILE_IO_TYPE:
			
			SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Network");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"File System");
			SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Device");
			SendMessage(hWnd, CB_SETCURSEL, 0, 0);
			break;

		case PROFILE_NONE_TYPE:
			SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
			break;
	}
}

VOID
RebarSelectForm(
	__in PREBAR_OBJECT Object,
	__in ULONG Form
	)
{
	HWND hWnd;

	hWnd = GetDlgItem(Object->hWndRebar, REBAR_BAND_SWITCH);
	SendMessage(hWnd, CB_SETCURSEL, (WPARAM)Form, 0);
}

HWND
RebarCreateLabel(
	__in PREBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndRebar;

	hWndRebar = Object->hWndRebar;
	ASSERT(hWndRebar != NULL);

	hWnd = CreateWindowEx(0, L"Static", NULL, 
                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | SS_LEFT, 
						  0, 0, 0, 0, hWndRebar, (HMENU)REBAR_BAND_LABEL, SdkInstance, NULL);
    ASSERT(hWnd != NULL);
	return hWnd;
}

HWND
RebarCreateFindEdit(
	__in PREBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndRebar;

	hWndRebar = Object->hWndRebar;
	ASSERT(hWndRebar != NULL);

	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", NULL, 
                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | ES_LEFT, 
						  0, 0, 0, 0, hWndRebar, (HMENU)REBAR_BAND_FIND, SdkInstance, NULL);
    ASSERT(hWnd != NULL);
	Edit_SetCueBannerText(hWnd, L"<Search>");
	return hWnd;
}

BOOLEAN
RebarInsertBands(
	__in PREBAR_OBJECT Object
	)
{
	HWND hWndCombo;
	HWND hWndBar;
	HWND hWndRebar;
	PREBAR_BAND Band;
	HIMAGELIST Normal;
	HIMAGELIST Grayed;
	LRESULT Result;
	SIZE Size;

	hWndRebar = Object->hWndRebar;

	hWndBar = RebarCreateBar(Object);
	if (!hWndBar) {
		return FALSE;
	}

	RebarSetBarImage(hWndBar, 0, &Normal, &Grayed);

	SendMessage(hWndBar, TB_AUTOSIZE, 0, 0);
	SendMessage(hWndBar, TB_GETMAXSIZE, 0, (LPARAM)&Size);

	//
	// Create combobox to help switch views
	//

	hWndCombo = RebarCreateCombo(Object);
	if (!hWndCombo) {
		return FALSE;
	}

	//
	// Create combo band
	//

	Band = (PREBAR_BAND)SdkMalloc(sizeof(REBAR_BAND));
	ZeroMemory(Band, sizeof(REBAR_BAND));

	Band->hWndBand = hWndCombo;
	Band->BandId = REBAR_BAND_SWITCH;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_TEXT |
		                   RBBIM_COLORS | RBBIM_STYLE;

	Band->BandInfo.cxMinChild = 240;
	Band->BandInfo.cyMinChild = Size.cy; ; 
	Band->BandInfo.cx = 240;
	Band->BandInfo.cyMaxChild = 240;
	Band->BandInfo.cyIntegral = 0;
	Band->BandInfo.lpText = L"Current View";
	Band->BandInfo.clrFore = GetSysColor(COLOR_BTNTEXT);
	Band->BandInfo.clrBack = GetSysColor(COLOR_BTNFACE);
	Band->BandInfo.fStyle = RBBS_NOGRIPPER | RBBS_TOPALIGN | RBBS_FIXEDSIZE;
	Band->BandInfo.wID = REBAR_BAND_SWITCH;
	Band->BandInfo.hwndChild = hWndCombo;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}
	
	SendMessage(hWndCombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));
	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;

/*
	//
	// Create find band
	//

	hWndEdit = RebarCreateFindEdit(Object);
	if (!hWndEdit) {
		return FALSE;
	}

	Band = (PREBAR_BAND)SdkMalloc(sizeof(REBAR_BAND));
	ZeroMemory(Band, sizeof(REBAR_BAND));

	Band->hWndBand = hWndEdit;
	Band->BandId = REBAR_BAND_FIND;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | 
						   RBBIM_STYLE | RBBIM_HEADERSIZE;

	Band->BandInfo.cxMinChild = 55;
	Band->BandInfo.cyMinChild = Size.cy - 1; 
	Band->BandInfo.cx = 160;
	Band->BandInfo.cxHeader = 8;
	Band->BandInfo.fStyle = RBBS_NOGRIPPER;

	Band->BandInfo.wID = REBAR_BAND_FIND;
	Band->BandInfo.hwndChild = hWndEdit;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}
	
	SendMessage(hWndEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));
	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;
*/
	//
	// Create command band
	//

	Band = (PREBAR_BAND)SdkMalloc(sizeof(REBAR_BAND));
	ZeroMemory(Band, sizeof(REBAR_BAND));

	Band->hWndBand = hWndBar;
	Band->BandId = REBAR_BAND_COMMAND;
	Band->himlNormal = Normal;
	Band->himlGrayed = Grayed;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID |
						   RBBIM_STYLE | RBBIM_HEADERSIZE;

	Band->BandInfo.cxMinChild = RebarCommandCount * 22;
	Band->BandInfo.cyMinChild = Size.cy;
	Band->BandInfo.cx = Band->BandInfo.cxMinChild + 22;
	Band->BandInfo.cxHeader = 8;
	Band->BandInfo.fStyle = RBBS_NOGRIPPER| RBBS_TOPALIGN;
	Band->BandInfo.wID = REBAR_BAND_COMMAND;
	Band->BandInfo.hwndChild = hWndBar;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}

	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;
	return TRUE;
}

VOID
RebarAdjustPosition(
	__in PREBAR_OBJECT Object
	)
{
	HWND hWndParent;
	RECT ClientRect;
	RECT Rect;

	hWndParent = GetParent(Object->hWndRebar);
	GetClientRect(hWndParent, &ClientRect);
	GetWindowRect(Object->hWndRebar, &Rect);

	MoveWindow(Object->hWndRebar, 0, 0, 
		       ClientRect.right - ClientRect.left, 
			   Rect.bottom - Rect.top, TRUE);
}

PREBAR_BAND
RebarGetBand(
	__in PREBAR_OBJECT Object,
	__in REBAR_BAND_ID Id
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PREBAR_BAND Band;

	ListHead = &Object->BandList;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Band = CONTAINING_RECORD(ListEntry, REBAR_BAND, ListEntry);
		if (Band->BandId == Id) {
			return Band;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

VOID
RebarCurrentEditString(
	__in PREBAR_OBJECT Object,
	__out PWCHAR Buffer,
	__in ULONG Length
	)
{
	PREBAR_BAND Band;

	Band = RebarGetBand(Object, REBAR_BAND_FIND);
	ASSERT(Band != NULL);

	SendMessage(Band->hWndBand,  WM_GETTEXT, (WPARAM)Length, (LPARAM)Buffer);
}

VOID
RebarDisableControls(
	__in PREBAR_OBJECT Object
	)
{
}