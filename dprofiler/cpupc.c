//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "cpupc.h"
#include "profileform.h"
#include "apspdb.h"
#include "aps.h"
#include "apsrpt.h"
#include "frame.h"
#include "sdk.h"

DIALOG_SCALER_CHILD CpuPcChildren[] = {
	{ IDC_LIST_PC, AlignRight, AlignBottom }
};

DIALOG_SCALER CpuPcScaler = {
	{0,0}, {0,0}, {0,0}, 1, CpuPcChildren
};

LISTVIEW_COLUMN CpuPcColumn[] = {
	{ 160, L"Name",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120, L"Module",  LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Sample",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Sample %",LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240, L"Line",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

ULONG CpuPcColumnCount = ARRAYSIZE(CpuPcColumn);


PCPU_PC_CONTEXT FORCEINLINE
CpuPcGetContext(
	__in PDIALOG_OBJECT Object
	)
{
	PCPU_FORM_CONTEXT Context;
	Context = (PCPU_FORM_CONTEXT)Object->Context;
	return (PCPU_PC_CONTEXT)Context->Context;
}

int
CpuPcComputePercentSpace(
	__in PCPU_PC_CONTEXT Context,
	__in HDC hdc
	)
{
	SIZE Size;

	if (Context->PercentSpace != 0) {
		return Context->PercentSpace;
	}

	GetTextExtentPoint32(hdc, L"100.00", (int)wcslen(L"100.00"), &Size);
	Context->PercentSpace = Size.cx;
	return Context->PercentSpace;
}

HWND
CpuPcCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	PCPU_PC_CONTEXT PcContext;
	HWND hWnd;
	
	PcContext = (PCPU_PC_CONTEXT)SdkMalloc(sizeof(CPU_PC_CONTEXT));
	PcContext->PercentSpace = 0;
	PcContext->PercentMinimum = CpuPcColumn[CpuPcPercentColumn].Width;

	Context = (PCPU_FORM_CONTEXT)SdkMalloc(sizeof(CPU_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;
	Context->Context = PcContext;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_CPU_PC;
	Object->Procedure = CpuPcProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CpuPcOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	LVITEM lvi = {0};
	ULONG i;
	PLISTVIEW_OBJECT ListView;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	
	//
	// Create background brush for static controls
	//

	Context->hBrushBack = CreateSolidBrush(RGB(255, 255, 255));

	//
	// Create listview object wraps list control
	//

	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = CpuPcColumn;
	ListView->Count = CpuPcColumnCount;
	ListView->NotifyCallback = CpuPcOnNotify;
	
	Context->ListView = ListView;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PC);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

	for (i = 0; i < CpuPcColumnCount; i++) {
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
		lvc.iSubItem = i;
		lvc.pszText = CpuPcColumn[i].Title;
		lvc.cx = CpuPcColumn[i].Width;
		lvc.fmt = CpuPcColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
	}

	//
	// Create bold font for symbol name
	//

	Context->hFontBold = SdkCreateListBoldFont();

	GetClientRect(hWnd, &Rect);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
				Rect.right - Rect.left, 
				Rect.bottom - Rect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &CpuPcScaler;
	DialogRegisterScaler(Object);
	
	SetWindowSubclass(hWndCtrl, CpuPcListProcedure, 0, (DWORD_PTR)Object);
	return TRUE;
}

LRESULT CALLBACK 
CpuPcListProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	LRESULT Status;

	if (uMsg == WM_NOTIFY) { 
		if (pNmhdr->code == HDN_ITEMCHANGING) {
			Status = CpuPcListOnHdnItemChanging((PDIALOG_OBJECT)dwData, 
											    (LPNMHEADER)pNmhdr);
			if (Status) {
				return Status;
			}
		}
	}
	
    return DefSubclassProc(hWnd, uMsg, wp, lp);
}

LRESULT
CpuPcOnClose(
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
CpuPcProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuPcOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuPcOnClose(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuPcOnNotify(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuPcOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR pNmhdr
	)
{
	LRESULT Status = CDRF_DODEFAULT;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;
	PCPU_FORM_CONTEXT Context;

	Context = (PCPU_FORM_CONTEXT)Object->Context;
	ASSERT(Context != NULL);

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT;
            break;
        
        case CDDS_SUBITEM|CDDS_ITEMPREPAINT: 

			if (lvcd->iSubItem != CpuPcPercentColumn) { 
				Status = CDRF_DODEFAULT;
			}
			else {

				//
				// Only require erase background, we draw text when
				// receive CDDS_ITEMPOSTPAINT
				//
				
				if (lvcd->iSubItem == CpuPcPercentColumn) {
					CpuPcFillPercentRect((PCPU_PC_CONTEXT)Context->Context, pNmhdr->hwndFrom, lvcd->nmcd.hdc,
									lvcd->nmcd.dwItemSpec, &lvcd->nmcd.rc,
									lvcd->nmcd.uItemState & CDIS_FOCUS);
				}
				Status = CDRF_SKIPDEFAULT;
			}
			break;
        
		case CDDS_SUBITEM|CDDS_ITEMPOSTPAINT:
			Status = CDRF_SKIPDEFAULT;
			break;

        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

LRESULT
CpuPcOnNotify(
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

	if(IDC_LIST_PC == pNmhdr->idFrom) {

		switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return CpuPcOnCustomDraw(Object, pNmhdr);
		case NM_DBLCLK:
			Status = CpuPcOnDbClick(Object, (LPNMITEMACTIVATE)lp);
			break;
		case LVN_COLUMNCLICK:
			Status = CpuPcOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;
		case HDN_ENDTRACK:
			break;
		}
	}


	return Status;
}

LRESULT
CpuPcListOnHdnItemChanging(
    __in PDIALOG_OBJECT Object,
	__in LPNMHEADER lpnmhdr 
	)
{
	PCPU_PC_CONTEXT Context;
	Context = CpuPcGetContext(Object);

	if (lpnmhdr->iItem == CpuPcPercentColumn) {
		if (lpnmhdr->pitem->mask & HDI_WIDTH) {
			if (lpnmhdr->pitem->cxy < Context->PercentMinimum) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

LRESULT 
CpuPcOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int nColumnCount;
	int i;
	HDITEM hdi;
	LISTVIEW_OBJECT *ListView;
	PCPU_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;

	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	ListView = Context->ListView;

    if (ListView->SortOrder == SortOrderNone){
        return 0;
    }

	if (ListView->LastClickedColumn == lpNmlv->iSubItem) {
		ListView->SortOrder = (LIST_SORT_ORDER)!ListView->SortOrder;
    } else {
		ListView->SortOrder = SortOrderAscendent;
    }
    
	hWnd = Object->hWnd;
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PC);
    hWndHeader = ListView_GetHeader(hWndCtrl);
    ASSERT(hWndHeader);

    nColumnCount = Header_GetItemCount(hWndHeader);
    
    for (i = 0; i < nColumnCount; i++) {
        hdi.mask = HDI_FORMAT;
        Header_GetItem(hWndHeader, i, &hdi);
        
        if (i == lpNmlv->iSubItem) {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
            if (ListView->SortOrder == SortOrderAscendent){
                hdi.fmt |= HDF_SORTUP;
            } else {
                hdi.fmt |= HDF_SORTDOWN;
            }
        } else {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        } 
        
        Header_SetItem(hWndHeader, i, &hdi);
    }
    
	ListView->LastClickedColumn = lpNmlv->iSubItem;
    ListView_SortItemsEx(hWndCtrl, CpuPcSortCallback, (LPARAM)hWnd);

    return 0L;
}

int CALLBACK
CpuPcSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	PBTR_PC_ENTRY Pc1, Pc2;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	HWND hWnd;
    int Result;
	HWND hWndList;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_PC);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Pc1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Pc2);

    //
    // Symbol or Module name or Line
    //

    if (ListView->LastClickedColumn == CpuPcNameColumn || 
		ListView->LastClickedColumn == CpuPcModuleColumn ||
        ListView->LastClickedColumn == CpuPcLineColumn ) {
	    ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);
		Result = wcsicmp(FirstData, SecondData);
	}
	
	if (ListView->LastClickedColumn == CpuPcSampleColumn || 
		ListView->LastClickedColumn == CpuPcPercentColumn ) {
		Result = Pc1->Exclusive - Pc2->Exclusive;
	}
	
	return ListView->SortOrder ? Result : -Result;
}

VOID
CpuPcInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{
	LVITEM lvi = {0};
	ULONG i, j;
	ULONG Count = 0;
	ULONG Total = 0;
	WCHAR Buffer[MAX_PATH];
	HWND hWndCtrl;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;
	PBTR_PC_ENTRY PcEntry;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;
	ULONG Exclusive;
	ULONG Inclusive;
	PWSTR Unicode;
	double Percent;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PC);

	Count = (ULONG)(Head->Streams[STREAM_PC].Length / sizeof(BTR_PC_ENTRY));
	PcEntry = (PBTR_PC_ENTRY)((PUCHAR)Head + Head->Streams[STREAM_PC].Offset);
	
	ApsGetCpuSampleCounters(Head, &Inclusive, &Exclusive);

	TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);

	if (Head->Streams[STREAM_LINE].Offset != 0 && Head->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Head + Head->Streams[STREAM_LINE].Offset);
	} else {
		Line = NULL;
	}

	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	for(i = 0, j = 0; i < Count; i++) {

		if (!PcEntry->Exclusive) {
			PcEntry += 1;
			continue;
		}
		
		Percent = (PcEntry->Exclusive * 1.0) / (Exclusive * 1.0);
		if (Percent < CPU_UI_INCLUSIVE_RATIO_THRESHOLD) {
			PcEntry += 1;
			continue;
		}
			
		//
		// Symbol name
		//

		lvi.iItem = j;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.lParam = (LPARAM)PcEntry;

		TextEntry = ApsLookupSymbol(TextTable, (ULONG64)PcEntry->Address);
		if (TextEntry) {
			StringCchPrintf(Buffer, MAX_PATH, L"%S", TextEntry->Text);
		} else {
            ApsFormatAddress(Buffer, MAX_PATH, PcEntry->Address, TRUE);
		}

		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);

        //
        // Module
        //

        lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;

        ApsGetDllBaseNameById(Head, PcEntry->DllId, Buffer, MAX_PATH);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Sample 
		//

		lvi.iSubItem = 2;
		StringCchPrintf(Buffer, MAX_PATH, L"%u", PcEntry->Exclusive);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Sample %
		//

		lvi.iSubItem = 3;
		StringCchPrintf(Buffer, MAX_PATH, L"%.2f", Percent * 100.0);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Line
		//
		
		lvi.iSubItem = 4;

		if (PcEntry->LineId != -1) {
			ASSERT(Line != NULL);
			LineEntry = Line + PcEntry->LineId;
			StringCchPrintf(Buffer, MAX_PATH, L"%S:%u", LineEntry->File, LineEntry->Line);
			lvi.pszText = Buffer; 
		} else {
			lvi.pszText = L""; 
		}
		
		ListView_SetItem(hWndCtrl, &lvi);

		j += 1;
		PcEntry += 1;
	}

	ApsDestroySymbolTable(TextTable);

	//
	// Update status bar of frame window
	//

	Unicode = (PWSTR)ApsMalloc(MAX_PATH * 2);
	StringCchPrintf(Unicode, MAX_PATH, L"Total %u IP samples", Exclusive);
	PostMessage(GetParent(hWnd), WM_USER_STATUSBAR, 0, (LPARAM)Unicode);
}

LRESULT 
CpuPcOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	)
{
	HWND hWndList;
	PWCHAR Ptr;
	WCHAR Buffer[MAX_PATH];
	size_t Length;
	ULONG Line;

	//
	// Check whether source column is clicked
	//

	if (lpnmitem->iSubItem != CpuPcLineColumn) {
		return 0;
	}

	//
	// Check whether there's any source information
	//

	hWndList = lpnmitem->hdr.hwndFrom;

	Buffer[0] = 0;
	ListView_GetItemText(hWndList, lpnmitem->iItem, CpuPcLineColumn, Buffer, MAX_PATH);

	Length = wcslen(Buffer);
	if (!Length) {

		//
		// there's no source information
		//

		return 0;
	}

	Ptr = wcsrchr(Buffer, L':');
	ASSERT(Ptr != NULL);

	Line = _wtoi(Ptr + 1);
	Ptr[0] = 0;

	FrameShowSource(Object->hWnd, Buffer, Line);
	return 0;
}


int
CpuPcFillPercentRect(
	__in PCPU_PC_CONTEXT Context,
	__in HWND hWnd,
	__in HDC hdc,
	__in int index,
	__in LPRECT lpRect,
	__in BOOLEAN Focus
	)
{
	WCHAR Buffer[16];
	RECT rcColor;
	RECT rcPercent;
	HBRUSH hBrush;
	double Percent;
	int width;
	int space;

	space = CpuPcComputePercentSpace(Context, hdc);

	ListView_GetItemText(hWnd, index, CpuPcPercentColumn, Buffer, 16);
	Percent = _wtof(Buffer);

	ListView_GetSubItemRect(hWnd, index, CpuPcPercentColumn, LVIR_BOUNDS, &rcColor);
	rcPercent = rcColor;

	if (Focus) {
		FillRect(hdc, &rcColor, GetSysColorBrush(COLOR_HIGHLIGHT));
		SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	} else {
		SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	}

	rcColor.right -= space;
	width = rcColor.right - rcColor.left;
	
    //
    // Need format percent right now
    //

    StringCchPrintf(Buffer, MAX_PATH, L"%.2f", Percent);
	if (Percent < 0.01) {
		Percent = 0.01;
	}

	width = (int)floor(width * (100.0 - Percent)/100.0);
	rcColor.right -= width;
	rcColor.top += 2;
	rcColor.bottom -= 2;

	//
	// Fill the percent color region
	//

	hBrush = CreateSolidBrush(RGB(0x00, 0x8A, 0x52));
	FillRect(hdc, &rcColor, hBrush);
	DeleteObject(hBrush);

	//
	// Draw the percent
 	//

	rcPercent.left = rcPercent.right - space;
	
	//SetBkMode(hdc, TRANSPARENT);
	DrawText(hdc, Buffer, -1, &rcPercent, DT_SINGLELINE|DT_RIGHT);
	return 0;
}