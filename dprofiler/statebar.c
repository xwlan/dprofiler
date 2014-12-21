//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

//
// N.B. This is for REBARINFO structure, it has different size
// for XP and VISTA, band insertion will fail if we fill wrong
// size of REBARINFO.
//

#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include "list.h"
#include "sdk.h"
#include "statebar.h"
#include "statecolor.h"

PCWSTR ButtonStrings[] = {
	L"Forward",
	L"Backward"
};

static TBBUTTON CommandBarButtons[] = {
	{ 0, IDM_SAMPLE_FORWARD,  TBSTATE_ENABLED, BTNS_BUTTON, 0, IDS_SAMPLE_RIGHT },
	{ 1, IDM_SAMPLE_BACKWARD,  TBSTATE_ENABLED, BTNS_BUTTON, 0, IDS_SAMPLE_LEFT },
	{ 2, IDM_SAMPLE_UPWARD,  TBSTATE_ENABLED, BTNS_BUTTON, 0, IDS_SAMPLE_UP },
	{ 3, IDM_SAMPLE_DOWNWARD,  TBSTATE_ENABLED, BTNS_BUTTON, 0, IDS_SAMPLE_DOWN },
	{ 4, IDM_SAMPLE_FULLSTACK, TBSTATE_ENABLED, BTNS_BUTTON, 0, IDS_SAMPLE_FULLSTACK },
	{ 5, IDM_SAMPLE_REPORT,  TBSTATE_ENABLED, BTNS_BUTTON, 0, IDS_SAMPLE_REPORT },
};

int StateBarCommandCount = sizeof(CommandBarButtons) / sizeof(TBBUTTON);

PSTATEBAR_OBJECT
StateBarCreateObject(
	__in HWND hWndParent, 
	__in UINT Id,
    __in BOOLEAN IsDynamic
	)
{
	HWND hWndStateBar;
	PSTATEBAR_OBJECT Object;

    if (IsDynamic) {
        hWndStateBar = CreateWindowEx(0, REBARCLASSNAME, NULL,
                                   WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_FIXEDORDER |
                                   WS_CLIPSIBLINGS | CCS_NORESIZE | CCS_NODIVIDER | RBS_AUTOSIZE| WS_GROUP,
                                   0, 0, 0, 0, hWndParent, (HMENU)Id, SdkInstance, NULL);
        if (!hWndStateBar) {
            return NULL;
        }
    }
    else {
        hWndStateBar = GetDlgItem(hWndParent, Id);
        ASSERT(hWndStateBar != NULL);
    }

	Object = (PSTATEBAR_OBJECT)SdkMalloc(sizeof(STATEBAR_OBJECT));
	ZeroMemory(Object, sizeof(STATEBAR_OBJECT));

	Object->hWndStateBar = hWndStateBar;
	Object->StateBarInfo.cbSize  = sizeof(REBARINFO);

	SendMessage(hWndStateBar, RB_SETUNICODEFORMAT, (WPARAM)TRUE, (LPARAM)0);
	InitializeListHead(&Object->BandList);
	return Object;
}

HWND
StateBarCreateCommand(
	__in PSTATEBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndStateBar;
	TBBUTTON Tb = {0};
	TBBUTTONINFO Tbi = {0};

	hWndStateBar = Object->hWndStateBar;
	ASSERT(hWndStateBar != NULL);

	hWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		                  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
	                      TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | 
						  CCS_NODIVIDER | CCS_TOP | CCS_NORESIZE,
						  0, 0, 0, 0, 
		                  hWndStateBar, (HMENU)UlongToHandle(STATEBAR_BAND_COMMAND), SdkInstance, NULL);

    ASSERT(hWnd != NULL);

    SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);
	SendMessage(hWnd, TB_ADDBUTTONS, sizeof(CommandBarButtons) / sizeof(TBBUTTON), (LPARAM)CommandBarButtons);
	SendMessage(hWnd, TB_AUTOSIZE, 0, 0);
	return hWnd;
}

HWND
StateBarCreateColor(
	__in PSTATEBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndStateBar;

	hWndStateBar = Object->hWndStateBar;
	ASSERT(hWndStateBar != NULL);

	hWnd = StateColorCreateControl(hWndStateBar, STATEBAR_BAND_COLOR);
    ASSERT(hWnd != NULL);
	return hWnd;
}

HWND
StateBarCreateZoom(
	__in PSTATEBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndStateBar;

	hWndStateBar = Object->hWndStateBar;
	ASSERT(hWndStateBar != NULL);

    //
    // N.B. Set statebar's parent as zoom's parent
    //

	hWnd = CreateWindowEx(0, TRACKBAR_CLASS, L"Zoom", 
                          WS_CHILD | WS_VISIBLE|TBS_NOTICKS,
                          0, 0, 0, 0,  GetParent(Object->hWndStateBar), 
                          (HMENU)STATEBAR_BAND_ZOOM, SdkInstance,
                          NULL); 

    //
    // Set scale range 1~10
    //

    SendMessage(hWnd, TBM_SETRANGE, FALSE, MAKELONG(1, 10));
	SendMessage(hWnd, TBM_SETPOS, FALSE, 1);
	return hWnd;
}

BOOLEAN
StateBarSetImageList(
	__in HWND hWndCommand,
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

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAMPLERIGHT));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAMPLELEFT));
	ImageList_AddIcon(himlNormal, hicon);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAMPLEUP));
	ImageList_AddIcon(himlNormal, hicon);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAMPLEDOWN));
	ImageList_AddIcon(himlNormal, hicon);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAMPLESTACK));
	ImageList_AddIcon(himlNormal, hicon);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAMPLEREPORT));
	ImageList_AddIcon(himlNormal, hicon);

    SendMessage(hWndCommand, TB_SETIMAGELIST, 0, (LPARAM)himlNormal);
    SendMessage(hWndCommand, TB_SETHOTIMAGELIST, 0, (LPARAM)himlNormal);

	himlGrayed = SdkCreateGrayImageList(himlNormal);
    SendMessage(hWndCommand, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himlGrayed);

	*Normal = himlNormal;
	*Grayed = himlGrayed;

	//
	// Set rebar's imagelist
	//

	Info.cbSize = sizeof(Info);
	Info.fMask = RBIM_IMAGELIST;
	Info.himl = himlNormal;
    SendMessage(GetParent(hWndCommand), RB_SETBARINFO, 0, (LPARAM)&Info);
	return TRUE;
}

BOOLEAN
StateBarInsertBands(
	__in PSTATEBAR_OBJECT Object
	)
{
	HWND hWndColor;
	HWND hWndCommand;
	HWND hWndStateBar;
    HWND hWndZoom;
	PSTATEBAR_BAND Band;
	HIMAGELIST Normal;
	HIMAGELIST Grayed;
	LRESULT Result;
	SIZE Size;

	hWndStateBar = Object->hWndStateBar;

	//
	// Create color band
	//

	hWndColor = StateBarCreateColor(Object);
	if (!hWndColor) {
		return FALSE;
	}

	Band = (PSTATEBAR_BAND)SdkMalloc(sizeof(STATEBAR_BAND));
	ZeroMemory(Band, sizeof(STATEBAR_BAND));

	Band->hWndBand = hWndColor;
	Band->BandId = STATEBAR_BAND_COLOR;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_TEXT |
		                   RBBIM_COLORS | RBBIM_STYLE;

	Band->BandInfo.cxMinChild = 24 * CPU_STATE_COUNT;
	Band->BandInfo.cyMinChild = 22; 
	Band->BandInfo.cx = 24 * CPU_STATE_COUNT;
    Band->BandInfo.lpText = L"Thread State";
	Band->BandInfo.clrFore = GetSysColor(COLOR_BTNTEXT);
	Band->BandInfo.clrBack = GetSysColor(COLOR_BTNFACE);
	Band->BandInfo.fStyle = RBBS_NOGRIPPER | RBBS_TOPALIGN | RBBS_FIXEDSIZE;
	Band->BandInfo.wID = STATEBAR_BAND_COLOR;
	Band->BandInfo.hwndChild = hWndColor;

	Result = SendMessage(hWndStateBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}
	
	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;

	//
	// Create command band
	//

    hWndCommand = StateBarCreateCommand(Object);
	if (!hWndCommand) {
		return FALSE;
	}

	StateBarSetImageList(hWndCommand, 0, &Normal, &Grayed);
	SendMessage(hWndCommand, TB_AUTOSIZE, 0, 0);
	SendMessage(hWndCommand, TB_GETMAXSIZE, 0, (LPARAM)&Size);

	Band = (PSTATEBAR_BAND)SdkMalloc(sizeof(STATEBAR_BAND));
	ZeroMemory(Band, sizeof(STATEBAR_BAND));

	Band->hWndBand = hWndCommand;
	Band->BandId = STATEBAR_BAND_COMMAND;
	Band->himlNormal = Normal;
	Band->himlGrayed = Grayed;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID |
						   RBBIM_STYLE | RBBIM_HEADERSIZE;

	Band->BandInfo.cxMinChild = StateBarCommandCount * 22;
	Band->BandInfo.cyMinChild = 22;
	Band->BandInfo.cx = Band->BandInfo.cxMinChild + 22;
	Band->BandInfo.cxHeader = 8;
	Band->BandInfo.fStyle = RBBS_NOGRIPPER| RBBS_TOPALIGN;
	Band->BandInfo.wID = STATEBAR_BAND_COMMAND;
	Band->BandInfo.hwndChild = hWndCommand;

	Result = SendMessage(hWndStateBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}

	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;

    //
    // Create zoom band
    //

    hWndZoom = StateBarCreateZoom(Object);
    ASSERT(hWndZoom != NULL);

    Band = (PSTATEBAR_BAND)SdkMalloc(sizeof(STATEBAR_BAND));
	ZeroMemory(Band, sizeof(STATEBAR_BAND));

	Band->hWndBand = hWndZoom;
	Band->BandId = STATEBAR_BAND_ZOOM;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_TEXT |
		                   RBBIM_COLORS | RBBIM_STYLE;

	Band->BandInfo.cxMinChild = 160;
	Band->BandInfo.cyMinChild = 0; 
	Band->BandInfo.cx = 160;
    Band->BandInfo.lpText = L"Zoom";
	Band->BandInfo.clrFore = GetSysColor(COLOR_BTNTEXT);
	Band->BandInfo.clrBack = GetSysColor(COLOR_BTNFACE);
	Band->BandInfo.fStyle = RBBS_NOGRIPPER | RBBS_TOPALIGN | RBBS_FIXEDSIZE;
	Band->BandInfo.wID = STATEBAR_BAND_ZOOM;
	Band->BandInfo.hwndChild = hWndZoom;

	Result = SendMessage(hWndStateBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}

	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;
	return TRUE;
}

VOID
StateBarAdjustPosition(
	__in PSTATEBAR_OBJECT Object
	)
{
	HWND hWndParent;
	RECT ClientRect;
	RECT Rect;

	hWndParent = GetParent(Object->hWndStateBar);
	GetClientRect(hWndParent, &ClientRect);
	GetWindowRect(Object->hWndStateBar, &Rect);

	MoveWindow(Object->hWndStateBar, 0, 0, 
		       ClientRect.right - ClientRect.left, 
			   Rect.bottom - Rect.top, TRUE);
}

PSTATEBAR_BAND
StateBarGetBand(
	__in PSTATEBAR_OBJECT Object,
	__in STATEBAR_BAND_ID Id
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PSTATEBAR_BAND Band;

	ListHead = &Object->BandList;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Band = CONTAINING_RECORD(ListEntry, STATEBAR_BAND, ListEntry);
		if (Band->BandId == Id) {
			return Band;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

VOID
StateBarDisableControls(
	__in PSTATEBAR_OBJECT Object
	)
{
}