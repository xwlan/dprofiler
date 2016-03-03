//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#include "iosummary.h"
#include "profileform.h"
#include "apsbtr.h"
#include "apsrpt.h"
#include "resource.h"

DIALOG_SCALER_CHILD IoSummaryChildren[] = {
	{ IDC_LIST_IO_SUMMARY, AlignRight, AlignBottom },
	{ IDC_BUTTON_IO_SUMMARY, AlignBoth, AlignBoth },
};

DIALOG_SCALER IoSummaryScaler = {
	{0,0}, {0,0}, {0,0}, 2, IoSummaryChildren
};

LISTVIEW_COLUMN IoSummaryColumn[] = {
	{ 120,  L"Name",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 300,  L"Value", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }
};


HWND
IoSummaryCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PIO_FORM_CONTEXT)SdkMalloc(sizeof(IO_FORM_CONTEXT));
	Context->CtrlId = CtrlId;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_IO_SUMMARY;
	Object->Procedure = IoSummaryProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
IoSummaryOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	ULONG i;
	LONG Style;
	int Status;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_SUMMARY);
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
		lvc.pszText = IoSummaryColumn[i].Title;	
		lvc.cx = IoSummaryColumn[i].Width;     
		lvc.fmt = IoSummaryColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	
	//
	// Register dialog scaler
	//

	Object->Scaler = &IoSummaryScaler;
	DialogRegisterScaler(Object);

	ShowWindow(GetDlgItem(hWnd, IDC_BUTTON_IO_SUMMARY), SW_HIDE);
	return TRUE;
}

LRESULT
IoSummaryOnClose(
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
IoSummaryProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return IoSummaryOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return IoSummaryOnClose(hWnd, uMsg, wp, lp);

	case WM_ERASEBKGND:
		return IoSummaryOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return IoSummaryOnNotify(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
IoSummaryOnEraseBkgnd(
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
IoSummaryOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0L;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lpnmhdr;
	PIO_FORM_CONTEXT Context;

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
IoSummaryOnNotify(
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

	if(IDC_LIST_IO_SUMMARY == pNmhdr->idFrom) {

		switch (pNmhdr->code) {
		case NM_CUSTOMDRAW:
			return IoSummaryOnCustomDraw(Object, pNmhdr);
		}

	}

	return Status;
}

VOID
IoSummaryInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{	
	LVITEM lvi = {0};
	LVGROUP Group = {0};
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
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

	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	Context->Head = Head;

	Info = (PPF_STREAM_SYSTEM)ApsGetStreamPointer(Head, STREAM_SYSTEM);
	ASSERT(Info != NULL);

	Mark = (PBTR_MARK)ApsGetStreamPointer(Head, STREAM_MARK);
	ASSERT(Mark != NULL);

	Count = ApsGetStreamLength(Head, STREAM_MARK)/sizeof(BTR_MARK);
	ASSERT(Count >= 0);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_SUMMARY);

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
	lvi.pszText = L"I/O";
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);
		
	// File 
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"File";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	if (Info->Attr.EnableFile) {
		lvi.pszText = L"Yes";
	} else {
		lvi.pszText = L"No";
	}
	lvi.iSubItem = 1;
	ListView_SetItem(hWndCtrl, &lvi);

	// Socket 
	Index += 1;
	lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_COLUMNS;
	lvi.iItem = Index;
	lvi.pszText = L"Net";
	lvi.iSubItem = 0;
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.mask = LVIF_TEXT;
	if (Info->Attr.EnableNet) {
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
	if (Ptr){
		Ptr += 1;
		StringCchPrintf(Buffer, MAX_PATH, L"%s (PID %u)", Ptr, Info->ProcessId);
	} else {
		Buffer[0] = 0;
	}

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