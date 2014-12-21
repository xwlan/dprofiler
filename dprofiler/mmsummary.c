//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "mmsummary.h"
#include "profileform.h"
#include "apsbtr.h"
#include "apsrpt.h"

DIALOG_SCALER_CHILD MmSummaryChildren[] = {
	{ IDC_LIST_MM_SUMMARY, AlignRight, AlignBottom },
	{ IDC_BUTTON_MM_SUMMARY, AlignBoth, AlignBoth },
};

DIALOG_SCALER MmSummaryScaler = {
	{0,0}, {0,0}, {0,0}, 2, MmSummaryChildren
};

LISTVIEW_COLUMN MmSummaryColumn[] = {
	{ 120,  L"Name",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 300,  L"Value", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }
};


HWND
MmSummaryCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	)
{
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PMM_FORM_CONTEXT)SdkMalloc(sizeof(MM_FORM_CONTEXT));
	Context->CtrlId = CtrlId;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_MM_SUMMARY;
	Object->Procedure = MmSummaryProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
MmSummaryOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	ULONG i;
	LONG Style;
	int Status;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
	
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_MM_SUMMARY);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//
	// Enable group of list view control
	//

	Style = GetWindowLong(hWndCtrl, GWL_STYLE);
	SetWindowLong(hWndCtrl, GWL_STYLE, Style | LVS_ALIGNTOP);
	Status = ListView_EnableGroupView(hWndCtrl, TRUE);
	ASSERT(Status == 1);

	for (i = 0; i < 2; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = MmSummaryColumn[i].Title;	
		lvc.cx = MmSummaryColumn[i].Width;     
		lvc.fmt = MmSummaryColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	
	//
	// Register dialog scaler
	//

	Object->Scaler = &MmSummaryScaler;
	DialogRegisterScaler(Object);

	ShowWindow(GetDlgItem(hWnd, IDC_BUTTON_MM_SUMMARY), SW_HIDE);
	return TRUE;
}

LRESULT
MmSummaryOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return TRUE;
}

INT_PTR CALLBACK
MmSummaryProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return MmSummaryOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return MmSummaryOnClose(hWnd, uMsg, wp, lp);

	case WM_ERASEBKGND:
		return MmSummaryOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return MmSummaryOnNotify(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
MmSummaryOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	RECT Rect;

	GetClientRect(hWnd, &Rect);
	FillRect((HDC)wp, &Rect, GetStockBrush(WHITE_BRUSH));
	return TRUE;
}

LRESULT
MmSummaryOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0L;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lpnmhdr;
	PMM_FORM_CONTEXT Context;

	Context = Object->Context;

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW;
            break;
        
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 

			if (lvcd->iSubItem == 0) { 
				SelectObject(lvcd->nmcd.hdc, Context->hFontBold);
			} 
			else {
				SelectObject(lvcd->nmcd.hdc, Context->hFontThin);
			}

			Status = CDRF_NEWFONT;
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

LRESULT
MmSummaryOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status = 0;
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);

	if(IDC_LIST_MM_SUMMARY == pNmhdr->idFrom) {

		switch (pNmhdr->code) {
		case NM_CUSTOMDRAW:
			return MmSummaryOnCustomDraw(Object, pNmhdr);
		}

	}

	return Status;
}

VOID
MmSummaryInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{	
	LVITEM lvi = {0};
	LVGROUP Group = {0};
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT Context;
	HWND hWndCtrl;
	int GroupId;
	int Index = 0;
	PPF_STREAM_SYSTEM Info;
	PBTR_MARK Mark;
	WCHAR Buffer[MAX_PATH];
	int Count;
	FILETIME FileTime;
	SYSTEMTIME Time;
	wchar_t *Ptr;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
	Context->Head = Head;

	Info = (PPF_STREAM_SYSTEM)ApsGetStreamPointer(Head, STREAM_SYSTEM);
	ASSERT(Info != NULL);

	Mark = (PBTR_MARK)ApsGetStreamPointer(Head, STREAM_MARK);
	ASSERT(Mark != NULL);

	Count = ApsGetStreamLength(Head, STREAM_MARK)/sizeof(BTR_MARK);
	ASSERT(Count >= 0);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_MM_SUMMARY);

	// 
	// Environment group
	//

	Group.cbSize = sizeof(LVGROUP);
	Group.mask = LVGF_HEADER | LVGF_GROUPID;
	Group.pszHeader = L"Profile Environment";
	Group.iGroupId = 0;
	GroupId = ListView_InsertGroup(hWndCtrl, -1, &Group); 
	ASSERT(GroupId != -1);

	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Operating System";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	lvi.pszText = Info->System;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);
		
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Processor";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	lvi.pszText = Info->Processor;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Memory(RAM)";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	lvi.pszText = Info->Memory;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Computer Name";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	lvi.pszText = Info->Computer;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Configuration group
	//

	Group.cbSize = sizeof(LVGROUP);
	Group.mask = LVGF_HEADER | LVGF_GROUPID;
	Group.pszHeader = L"Profile Configuration";
	Group.iGroupId = 1;
	GroupId = ListView_InsertGroup(hWndCtrl, -1, &Group); 
	ASSERT(GroupId != -1);

	Index += 1;

	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Profile Type";
	lvi.iSubItem = 0;
	lvi.iGroupId = 1;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	lvi.pszText = L"Memory";
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);
		
	// Heap
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Heap";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	if (Info->Attr.EnableHeap) {
		lvi.pszText = L"Yes";
	} else {
		lvi.pszText = L"No";
	}
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	// Page
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Page";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	if (Info->Attr.EnablePage) {
		lvi.pszText = L"Yes";
	} else {
		lvi.pszText = L"No";
	}
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	// Handle
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Handle";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	if (Info->Attr.EnableHandle) {
		lvi.pszText = L"Yes";
	} else {
		lvi.pszText = L"No";
	}
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	// GDI
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"GDI";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	if (Info->Attr.EnableGdi) {
		lvi.pszText = L"Yes";
	} else {
		lvi.pszText = L"No";
	}
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	// process
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Process";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;

	Ptr = wcsrchr(Info->ImagePath, L'\\');
	Ptr += 1;
	StringCchPrintf(Buffer, MAX_PATH, L"%s (PID %u)", Ptr, Info->ProcessId);

	lvi.pszText = Buffer;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Counter group
	//

	Group.cbSize = sizeof(LVGROUP);
	Group.mask = LVGF_HEADER | LVGF_GROUPID;
	Group.pszHeader = L"Profile Counter";
	Group.iGroupId = 2;
	GroupId = ListView_InsertGroup(hWndCtrl, -1, &Group); 
	ASSERT(GroupId != -1);

	Index += 1;

	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Start Time";
	lvi.iSubItem = 0;
	lvi.iGroupId = 2;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	FileTimeToLocalFileTime(&Mark->Timestamp, &FileTime);
	FileTimeToSystemTime(&FileTime, &Time);
	StringCchPrintf(Buffer, MAX_PATH, L"%04d/%02d/%02d, %02d:%02d:%02d",
					Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);
	lvi.pszText = Buffer;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);
		
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"End Time";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	FileTimeToLocalFileTime(&Mark[Count - 1].Timestamp, &FileTime);
	FileTimeToSystemTime(&FileTime, &Time);
	StringCchPrintf(Buffer, MAX_PATH, L"%04d/%02d/%02d, %02d:%02d:%02d",
					Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);
	lvi.pszText = Buffer;
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);
}