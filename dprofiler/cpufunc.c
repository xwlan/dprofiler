//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "cpufunc.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "cpufunc.h"
#include "dialog.h"
#include "profileform.h"
#include "aps.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "frame.h"
#include "srcdlg.h"

DIALOG_SCALER_CHILD CpuFuncChildren[1] = {
	{ IDC_LIST_FUNCTION, AlignRight, AlignBottom }
};

DIALOG_SCALER CpuFuncScaler = {
	{0,0}, {0,0}, {0,0}, 1, CpuFuncChildren
};

LISTVIEW_COLUMN CpuFuncColumn[] = {
	{ 160, L"Name",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Inclusive",	 LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Inclusive %",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Exclusive",    LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Exclusive %",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Module",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240, L"Line",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

ULONG CpuFuncColumnCount = ARRAYSIZE(CpuFuncColumn);

HWND
CpuFuncCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PCPU_FORM_CONTEXT)SdkMalloc(sizeof(CPU_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_CPU_FUNCTION;
	Object->Procedure = CpuFuncProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CpuFuncOnInitDialog(
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

	ListView->Column = CpuFuncColumn;
	ListView->Count = CpuFuncColumnCount;
	ListView->NotifyCallback = CpuFuncOnNotify;
	
	Context->ListView = ListView;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_FUNCTION);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	for (i = 0; i < CpuFuncColumnCount; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CpuFuncColumn[i].Title;	
		lvc.cx = CpuFuncColumn[i].Width;     
		lvc.fmt = CpuFuncColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	GetClientRect(hWnd, &Rect);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &CpuFuncScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT CALLBACK 
CpuFuncHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	return 0;
}

LRESULT
CpuFuncOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuFuncOnClose(
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
CpuFuncProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuFuncOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuFuncOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return CpuFuncOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuFuncOnNotify(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuFuncOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuFuncOnNotify(
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

	if(IDC_LIST_FUNCTION == pNmhdr->idFrom) {

		switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return CpuFuncOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			Status = CpuFuncOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;

		case NM_DBLCLK:
			Status = CpuFuncOnDbClick(Object, (LPNMITEMACTIVATE)lp);
			break;
		}
	}


	return Status;
}

LRESULT
CpuFuncOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0L;
    return Status;
}

LRESULT 
CpuFuncOnDbClick(
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

	if (lpnmitem->iSubItem != 6) {
		return 0;
	}

	//
	// Check whether there's any source information
	//

	hWndList = lpnmitem->hdr.hwndFrom;

	Buffer[0] = 0;
	ListView_GetItemText(hWndList, lpnmitem->iItem, 6, Buffer, MAX_PATH);

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

LRESULT 
CpuFuncOnColumnClick(
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
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_FUNCTION);
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
    ListView_SortItemsEx(hWndCtrl, CpuFuncSortCallback, (LPARAM)hWnd);

    return 0L;
}

int CALLBACK
CpuFuncSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	PBTR_FUNCTION_ENTRY Pc1, Pc2;
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

	if (ListView->LastClickedColumn == 0 || 
        ListView->LastClickedColumn == 5 ||
        ListView->LastClickedColumn == 6 )  {
	    ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);
		Result = wcsicmp(FirstData, SecondData);
	}
	
	if (ListView->LastClickedColumn == 3 || ListView->LastClickedColumn == 4) {
		Result = Pc1->Exclusive - Pc2->Exclusive;
	}
	
	if (ListView->LastClickedColumn == 1 || ListView->LastClickedColumn == 2) {
		Result = Pc1->Inclusive - Pc2->Inclusive;
	}

	return ListView->SortOrder ? Result : -Result;
}

VOID
CpuFuncInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{
	LVITEM lvi = {0};
	ULONG i, j;
	ULONG Count;
	WCHAR Buffer[MAX_PATH];
	HWND hWndCtrl;
	PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;
	PBTR_PC_ENTRY PcEntry;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;
	ULONG Inclusive;
	ULONG Exclusive;
	PWSTR Unicode;
	double Percent;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_FUNCTION);
	Count = (ULONG)(Head->Streams[STREAM_FUNCTION].Length / sizeof(BTR_FUNCTION_ENTRY));
	FuncEntry = (PBTR_FUNCTION_ENTRY)((PUCHAR)Head + Head->Streams[STREAM_FUNCTION].Offset);
	PcEntry = (PBTR_PC_ENTRY)((PUCHAR)Head + Head->Streams[STREAM_PC].Offset);
	TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);

    if (Head->Streams[STREAM_LINE].Offset != 0 && Head->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Head + Head->Streams[STREAM_LINE].Offset);
	} else {
		Line = NULL;
	}

	//
	// Total sample count as initial exclusive/inclusive
	//

	ApsGetCpuSampleCounters(Head, &Inclusive, &Exclusive);
	//Inclusive = Exclusive;

	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	for(i = 0, j = 0; i < Count; i++) {
		
		Percent = (FuncEntry->Inclusive * 1.0) / (Inclusive * 1.0);
		if (Percent < CPU_UI_INCLUSIVE_RATIO_THRESHOLD) {
			FuncEntry += 1;
			continue;
		}

		//
		// Symbol name
		//

		lvi.iItem = j;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.lParam = (LPARAM)FuncEntry;

		TextEntry = ApsLookupSymbol(TextTable, FuncEntry->Address);
        ASSERT(TextEntry != NULL);

        StringCchPrintf(Buffer, MAX_PATH, L"%S", TextEntry->Text);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);

		//
		// Inclusive
		//

		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%u", FuncEntry->Inclusive);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);
		
		
		//
		// Inclusive %
		//

		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%.2f", (FuncEntry->Inclusive * 100.0) / (Inclusive * 1.0));
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);
        
        //
		// Exclusive
		//

		lvi.iSubItem = 3;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%u", FuncEntry->Exclusive);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Exclusive %
		//

		lvi.iSubItem = 4;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%.2f", (FuncEntry->Exclusive * 100.0) / (Exclusive * 1.0));
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

        //
        // Module
        //

        lvi.iSubItem = 5;
		lvi.mask = LVIF_TEXT;

        ApsGetDllBaseNameById(Head, FuncEntry->DllId, Buffer, MAX_PATH);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

        //
		// Line
		//

		if (Line != NULL && TextEntry->LineId != -1) {

			ASSERT(Line != NULL);

			LineEntry = Line + TextEntry->LineId;
			StringCchPrintf(Buffer, MAX_PATH, L"%S:%u", LineEntry->File, LineEntry->Line);

			lvi.iSubItem = 6;
			lvi.mask = LVIF_TEXT;
			lvi.pszText = Buffer; 
			ListView_SetItem(hWndCtrl, &lvi);

		} else {

			lvi.iSubItem = 6;
			lvi.mask = LVIF_TEXT;
			lvi.pszText = L""; 
			ListView_SetItem(hWndCtrl, &lvi);
		}

		FuncEntry += 1;
		j += 1;
	}

	ApsDestroySymbolTable(TextTable);

	Unicode = (PWSTR)ApsMalloc(MAX_PATH * 2);
	StringCchPrintf(Unicode, MAX_PATH, L"Total %u Function samples", Count);
	PostMessage(GetParent(hWnd), WM_USER_STATUSBAR, 0, (LPARAM)Unicode);
}