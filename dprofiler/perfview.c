//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "sdk.h"
#include "frame.h"
#include "perfview.h"
#include "listview.h"
#include "aps.h"

int PerfViewColumnOrder[PerfColumnCount] = {
	LogColumnSequence,
	LogColumnProcess,
	LogColumnTimestamp,
	LogColumnPID,
	LogColumnTID,
	LogColumnProbe,
	LogColumnProvider,
	LogColumnReturn,
	LogColumnDuration,
	LogColumnDetail,
};

//
// N.B. Process and Sequence column order are swapped when displayed
//

LISTVIEW_COLUMN PerfViewColumn[PerfColumnCount] = {
	{ 80,  L"Process",		 LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"Sequence",      LVCFMT_RIGHT,0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Time",          LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"PID",           LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"TID",           LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Probe",         LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Provider",      LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"Return",        LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Duration (ms)", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 360, L"Detail",        LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }
};

LISTVIEW_OBJECT PerfViewObject = 
{ NULL, 0, PerfViewColumn, PerfColumnCount, NULL, NULL, NULL, 0, 0 };

typedef struct _PERFVIEW_OBJECT {
	LISTVIEW_OBJECT ListObject;
	HIMAGELIST himlSmall;
	LIST_ENTRY ImageListHead;
	PERFVIEW_MODE Mode;
	BOOLEAN AutoScroll;
} PERFVIEW_OBJECT, *PPERFVIEW_OBJECT;

HIMAGELIST PerfViewSmallIcon;
LIST_ENTRY PerfViewImageList;

PERFVIEW_MODE PerfViewMode = PERFVIEW_UPDATE_MODE;
BOOLEAN PerfViewAutoScroll = FALSE;
HFONT PerfViewBoldFont;
HFONT PerfViewThinFont;

//
// N.B. record buffer is constructed as a guarded region,
//      the first and the last pages are no access, the 
//      wrapped 256K is accessible.
//

PVOID PerfViewBuffer;
ULONG PerfViewBufferSize;

#define PERFVIEW_TIMER_ID  6

ULONG
PerfViewInitImageList(
	IN HWND	hWnd
	)
{
	SHFILEINFO sfi = {0};
	PPERFVIEW_IMAGE Object;
	HICON hicon;
	ULONG Index;

	PerfViewSmallIcon = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
		                                GetSystemMetrics(SM_CYSMICON), 
										ILC_COLOR32, 0, 0); 

	ListView_SetImageList(hWnd, PerfViewSmallIcon, LVSIL_SMALL);
    
	//
	// Add first icon as shell32.dll's default icon, this is
	// for those failure case when process's icon can't be extracted.
	//

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_GENERIC));
	Index = ImageList_AddIcon(PerfViewSmallIcon, hicon);
	DestroyIcon(sfi.hIcon);
	
	Object = (PPERFVIEW_IMAGE)SdkMalloc(sizeof(PERFVIEW_IMAGE));
	Object->ProcessId = -1;
	Object->ImageIndex = 0;

	InitializeListHead(&PerfViewImageList);
	InsertHeadList(&PerfViewImageList, &Object->ListEntry);

	return 0;
}

ULONG
PerfViewAddImageList(
	IN PWSTR FullPath,
	IN ULONG ProcessId
	)
{
	SHFILEINFO sfi = {0};
	PPERFVIEW_IMAGE Object;
	ULONG Index;

	Index = PerfViewQueryImageList(ProcessId);
	if (Index != 0) {
		return Index;
	}

	SHGetFileInfo(FullPath, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
	Index = ImageList_AddIcon(PerfViewSmallIcon, sfi.hIcon);
	if (Index != -1) {
		DestroyIcon(sfi.hIcon);
	}
	
	Object = (PPERFVIEW_IMAGE)SdkMalloc(sizeof(PERFVIEW_IMAGE));
	Object->ProcessId = ProcessId;

	if (Index != -1) {
		Object->ImageIndex = Index;
	} else {
		Object->ImageIndex = 0;
	}

	InsertHeadList(&PerfViewImageList, &Object->ListEntry);
	return Index;
}

ULONG
PerfViewQueryImageList(
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PPERFVIEW_IMAGE Object;

	ListEntry = PerfViewImageList.Flink;

	while (ListEntry != &PerfViewImageList) {
		
		Object = CONTAINING_RECORD(ListEntry, PERFVIEW_IMAGE, ListEntry);
		if (Object->ProcessId == ProcessId) {
			return Object->ImageIndex;
		}
		ListEntry = ListEntry->Flink;
	}

	return 0;
}

PVOID
PerfViewCreateBuffer(
	VOID
	)
{
	PVOID Address;

	//
	// N.B. The logview's buffer is built as a paged buffer,
	// the first page and last page are not committed, the
	// middle page is committed, this can help to detect buffer
	// overflow just in time.
	//

	Address = (PVOID)VirtualAlloc(NULL, ApsPageSize * 3, MEM_RESERVE, PAGE_NOACCESS);
	Address = VirtualAlloc((PCHAR)Address + ApsPageSize, ApsPageSize, MEM_COMMIT, PAGE_READWRITE);

	PerfViewBuffer = Address;
	PerfViewBufferSize = ApsPageSize;
	return Address;
}

VOID CALLBACK 
PerfViewTimerProcedure(
	IN HWND hWnd,
    UINT uMsg,
    UINT_PTR idEvent,
    DWORD dwTime
	)
{
	if (idEvent == PERFVIEW_TIMER_ID) {
		PerfViewOnTimer(hWnd, uMsg, 0, 0);
	}
}

PLISTVIEW_OBJECT
PerfViewCreate(
	IN HWND hWndParent,
	IN ULONG CtrlId
	)
{
	HWND hWnd;
	DWORD dwStyle;
	ULONG i;
	LVCOLUMN lvc = {0}; 

	dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_CLIPSIBLINGS | 
		      LVS_OWNERDATA | LVS_SINGLESEL;
	
	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, 0, dwStyle, 
						  0, 0, 0, 0, hWndParent, (HMENU)UlongToPtr(CtrlId), SdkInstance, NULL);

	ASSERT(hWnd != NULL);
	
    ListView_SetUnicodeFormat(hWnd, TRUE);

    dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
	SetWindowLongPtr(hWnd, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | LVS_REPORT | LVS_SHOWSELALWAYS);

	SdkSetObject(hWnd, &PerfViewObject);

	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_SUBITEMIMAGES , LVS_EX_SUBITEMIMAGES); 
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER); 
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_INFOTIP, LVS_EX_INFOTIP); 

	//
	// Add support to imagelist (for Process Name column).
	//

	PerfViewInitImageList(hWnd);

	//
	// N.B. Because the first column is always left aligned, we have to 
	// first insert a dummy column and then delete it to make the first
	// column text right aligned.
	//

	lvc.mask = 0; 
	ListView_InsertColumn(hWnd, 0, &lvc);

    for (i = 0; i < PerfColumnCount; i++) { 

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = PerfViewColumn[i].Title;	
		lvc.cx = PerfViewColumn[i].Width;     
		lvc.fmt = PerfViewColumn[i].Align;
		ListView_InsertColumn(hWnd, i + 1, &lvc);
    } 

	//
	// Delete dummy column
	//

	ListView_DeleteColumn(hWnd, 0);

	ListView_SetColumnOrderArray(hWnd, PerfColumnCount, PerfViewColumnOrder);

	PerfViewObject.CtrlId = CtrlId;
	PerfViewObject.hWnd = hWnd;
	
	PerfViewBuffer = PerfViewCreateBuffer();
	ASSERT(PerfViewBuffer != NULL);

	SetTimer(hWnd, PERFVIEW_TIMER_ID, 1000, PerfViewTimerProcedure);
	return &PerfViewObject;
}

VOID
PerfViewSetItemCount(
	IN ULONG Count
	)
{
	if (PerfViewMode != PERFVIEW_STALL_MODE) {

		if (!PerfViewAutoScroll) {
			ListView_SetItemCountEx(PerfViewObject.hWnd, Count, 
				                    LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
		} else {
			ListView_SetItemCount(PerfViewObject.hWnd, Count);
			ListView_EnsureVisible(PerfViewObject.hWnd, Count - 1, FALSE);
		}
	}
}

LRESULT 
PerfViewOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status;
	LPNMHDR pNmhdr = (LPNMHDR)lp;

	Status = 0L;

	switch (pNmhdr->code) {
		
		case NM_CUSTOMDRAW:
			Status = PerfViewOnCustomDraw(hWnd, uMsg, wp, lp);
			break;

		case LVN_GETDISPINFO:
			Status = PerfViewOnGetDispInfo(hWnd, uMsg, wp, lp);
			break;
		
	}

	return Status;
}

LRESULT
PerfViewOnOdCacheHint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

HFONT
PerfViewCreateFont(
	IN HDC hdc
	)
{
	LOGFONT LogFont;
	HFONT hFont;

	//
	// Create bold font
	//

	hFont = GetCurrentObject(hdc, OBJ_FONT);
	PerfViewThinFont = hFont;

	GetObject(hFont, sizeof(LOGFONT), &LogFont);
	LogFont.lfWeight = FW_BOLD;
	hFont = CreateFontIndirect(&LogFont);
	PerfViewBoldFont = hFont;

	return PerfViewBoldFont;
}

LRESULT
PerfViewOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0L;
	LPNMHDR pNmhdr = (LPNMHDR)lp; 
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW;
            break;
        
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 

			if (lvcd->iSubItem == LogColumnSequence) { 
				lvcd->clrTextBk = RGB(220, 220, 220);
			} 
			else {
				lvcd->clrTextBk = RGB(255, 255, 255);
			}
			Status = CDRF_NEWFONT;
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

ULONG
PerfViewGetItemCount(
	VOID
	)
{
	return (ULONG)ListView_GetItemCount(PerfViewObject.hWnd);
}

LRESULT 
PerfViewOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0L;
}

LRESULT
PerfViewOnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LV_DISPINFO* lpdi;
    LV_ITEM *lpvi;
	WCHAR Buffer[1024];

    lpdi = (LV_DISPINFO *)lp;
    lpvi = &lpdi->item;

	if (PerfViewIsInStallMode()) {
		return 0;
	}

    if (lpvi->mask & LVIF_TEXT){
		
		switch (lpvi->iSubItem) {

		}

		StringCchCopy(lpvi->pszText, lpvi->cchTextMax, Buffer);
    }
	
	if (lpvi->iSubItem == LogColumnProcess) {
		//lpvi->iImage = PerfViewQueryImageList(Record->ProcessId);
		lpvi->mask |= LVIF_IMAGE;
	}

    return 0L;
}


LRESULT
PerfViewOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	if (PerfViewIsInStallMode()) {
		return 0;
	}

	return 0;
}

LRESULT
PerfViewProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Result = 0;

	switch (uMsg) {

		case WM_NOTIFY:
			Result = PerfViewOnNotify(hWnd, uMsg, wp, lp);
			break;

		case WM_CONTEXTMENU:
			Result = PerfViewOnContextMenu(hWnd, uMsg, wp, lp);

		case WM_TIMER:
			PerfViewOnTimer(hWnd, uMsg, wp, lp);
			break;
	}

	return Result;
}

VOID
PerfViewEnterStallMode(
	VOID
	)
{
	RECT Rect;
	HWND hWnd;
	LONG Style;

	hWnd = PerfViewObject.hWnd;

	KillTimer(hWnd, PERFVIEW_TIMER_ID);
	PerfViewMode = PERFVIEW_STALL_MODE;

	//
	// Temporarily remove LVS_SHOWSELALWAYS
	//

	Style = GetWindowLong(hWnd, GWL_STYLE);
	Style &= ~LVS_SHOWSELALWAYS;
	SetWindowLong(hWnd, GWL_STYLE, Style);
	
	PerfViewSetItemCount(0);

	GetClientRect(hWnd, &Rect);
	InvalidateRect(hWnd, &Rect, TRUE);
	UpdateWindow(hWnd);
}

VOID
PerfViewExitStallMode(
	IN ULONG NumberOfRecords
	)
{
	HWND hWnd;
	LONG Style;

	hWnd = PerfViewObject.hWnd;
	PerfViewMode = PERFVIEW_UPDATE_MODE;

	//
	// Re-enabling LVS_SHOWSELALWAYS
	//

	Style = GetWindowLong(hWnd, GWL_STYLE);
	Style |= LVS_SHOWSELALWAYS;
	SetWindowLong(hWnd, GWL_STYLE, Style);

	//
	// N.B. according to MSDN, to force redraw immediately, we should call
	// UpdateWindow() after ListView_RedrawItems
	//

	PerfViewSetItemCount(NumberOfRecords);
	ListView_RedrawItems(hWnd, 0, NumberOfRecords);
	UpdateWindow(PerfViewObject.hWnd);
	
	SetTimer(hWnd, PERFVIEW_TIMER_ID, 1000, PerfViewTimerProcedure);
}

BOOLEAN
PerfViewIsInStallMode(
	VOID
	)
{
	return (PerfViewMode == PERFVIEW_STALL_MODE);
}

VOID
PerfViewSetAutoScroll(
	IN BOOLEAN AutoScroll
	)
{
	PerfViewAutoScroll = AutoScroll;
}

int
PerfViewGetFirstSelected(
	VOID
	)
{
	return ListViewGetFirstSelected(PerfViewObject.hWnd);
}