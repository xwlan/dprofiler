//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#include "ccrcontention.h"
#include "profileform.h"
#include "apsbtr.h"
#include "apsrpt.h"
#include "resource.h"
#include "apsbtr.h"
#include "ccrstack.h"


DIALOG_SCALER_CHILD CcrContentionChildren[] = {
	{ IDC_LIST_CCR_CONTENTION, AlignRight, AlignBottom },
};

DIALOG_SCALER CcrContentionScaler = {
	{0,0}, {0,0}, {0,0}, 1, CcrContentionChildren
};

typedef enum _CCR_UI_LOCK_PROPERTY{
	CcrUiAddress,
	CcrUiType,
	CcrUiOwner,
	CcrUiAcquire,
	CcrUiTryAcquire,
	CcrUiTryFailure,
	CcrUiAcquirePath,
	CcrUiAcquireThread,
	CcrUiMaxLatency,
	CcrUiMaxDuration,
	CcrUiColumnCount,
} CCR_UI_LOCK_PROPERTY;

LISTVIEW_COLUMN CcrContentionColumn[CcrUiColumnCount] = {
	{ 140,  L"Address",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Type", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Owner", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Acquire", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
	{ 120,  L"Try Acquire", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
	{ 120,  L"Try Failure", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
	{ 120,  L"Try/Acquire Path", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
	{ 140,  L"Try/Acquire Thread", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
	{ 200,  L"Max Acquire Latency(ms)", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
	{ 200,  L"Max Holding Duration(ms)", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },  
};

HWND
CcrContentionCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId
	)
{
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PCCR_FORM_CONTEXT)SdkMalloc(sizeof(CCR_FORM_CONTEXT));
	Context->CtrlId = CtrlId;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_CCR_CONTENTION;
	Object->Procedure = CcrContentionProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CcrContentionOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	ULONG i;
	RECT Rect;
	PLISTVIEW_OBJECT ListView;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
	
	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = CcrContentionColumn;
	ListView->Count = CcrUiColumnCount;
	ListView->NotifyCallback = CcrContentionOnNotify;
	Context->ListView = ListView;

	GetClientRect(hWnd, &Rect);
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_CONTENTION);
	MoveWindow(hWndCtrl, 0, 0, Rect.right, Rect.bottom, TRUE);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	for (i = 0; i < CcrUiColumnCount; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CcrContentionColumn[i].Title;	
		lvc.cx = CcrContentionColumn[i].Width;     
		lvc.fmt = CcrContentionColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Register dialog scaler
	//

	Object->Scaler = &CcrContentionScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT
CcrContentionOnClose(
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
CcrContentionProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CcrContentionOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CcrContentionOnClose(hWnd, uMsg, wp, lp);

	case WM_ERASEBKGND:
		return CcrContentionOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CcrContentionOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		return CcrContentionOnCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CcrContentionOnEraseBkgnd(
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
CcrContentionOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0L;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lpnmhdr;
	PCCR_FORM_CONTEXT Context;

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
CcrContentionOnStackTrace(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PCCR_LOCK_TRACK Track;
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	HWND hWndList;
	LONG Index;

	hWndList = GetDlgItem(hWnd, IDC_LIST_CCR_CONTENTION);
	Index = ListViewGetFirstSelected(hWndList);
	ASSERT(Index != -1);

	ListViewGetParam(hWndList, Index, (LPARAM *)&Track);
	ASSERT(Track != NULL);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
	ASSERT(Object->Context != NULL);

	CcrStackCreate(hWnd, 0, Context->Head, Track);
	return 0;
}

LRESULT
CcrContentionOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	UINT CommandId;

	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	CommandId = LOWORD(wp);

	switch (CommandId) {

		case ID_CCR_STACKTRACE:
			return CcrContentionOnStackTrace(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
CcrContentionOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	POINT pt;
	PDIALOG_OBJECT Object;
	HMENU hPopupMenu = NULL;
	HMENU hMenuLoaded;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	GetCursorPos(&pt);

	//
	// Display the context menu
	//

	hMenuLoaded = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_CCR)); 
	hPopupMenu = GetSubMenu(hMenuLoaded, 0);

	//
	// The menu command must be routed to dialog, not tabctrl
	//

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,               
				   pt.x, pt.y, 0, hWnd, NULL); 
	DestroyMenu(hMenuLoaded);	
	return 0;
}

LRESULT
CcrContentionOnNotify(
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

	if(IDC_LIST_CCR_CONTENTION == pNmhdr->idFrom) {

		switch (pNmhdr->code) {
		case NM_CUSTOMDRAW:
			return CcrContentionOnCustomDraw(Object, pNmhdr);
		case LVN_COLUMNCLICK: 
			Status = CcrContentionOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;
		case NM_RCLICK:
			Status = CcrContentionOnContextMenu(hWnd, uMsg, wp, lp);
			break;
		}
	}

	return Status;
}

int __cdecl 
CcrContentionStackIdSortCallback(
	__in const void *First, 
	__in const void *Second
	)
{
	return *(PULONG)First - *(PULONG)Second;
}

int CALLBACK
CcrContentionSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	PCCR_LOCK_TRACK Track1, Track2;
	HWND hWnd;
    int Result;
	HWND hWndList;
	UINT64 I1, I2;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_CCR_CONTENTION);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);

	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Track1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Track2);

    //
    // Symbol or Module name or Line
    //

	ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);

	switch(ListView->LastClickedColumn) {

	case CcrUiAddress:
		// skip '0x'
		I1 = _wcstoui64(FirstData + 2, NULL, 16);
		I2 = _wcstoui64(SecondData + 2, NULL, 16);
		Result = I1 >= I2 ? 1 : -1;
		break;

	case CcrUiAcquire:
	case CcrUiTryAcquire:
	case CcrUiTryFailure:
	case CcrUiAcquirePath:
	case CcrUiAcquireThread:
		I1 = _wcstoui64(FirstData, NULL, 16);
		I2 = _wcstoui64(SecondData, NULL, 16);
		Result = I1 >= I2 ? 1 : -1;
		break;

	case CcrUiType:
		Result = Track1->Type - Track2->Type;
		break;

	case CcrUiOwner:
		break;

	case CcrUiMaxLatency:
	case CcrUiMaxDuration:
		{
			double F1, F2;
			F1 = _wtof(FirstData);
			F2 = _wtof(SecondData);
			Result = F1 >= F2 ? 1 : -1;
			break;
		}
	}
	
	return ListView->SortOrder ? Result : -Result;
}

LRESULT 
CcrContentionOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int nColumnCount;
	int i;
	HDITEM hdi;
	LISTVIEW_OBJECT *ListView;
	PCCR_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;

	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
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
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_CONTENTION);
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
    ListView_SortItemsEx(hWndCtrl, CcrContentionSortCallback, (LPARAM)hWnd);

    return 0L;
}

VOID
CcrContentionInsertListItem(
	_In_ HWND hWndCtrl,
	_In_ int Index,
	_In_ PCCR_LOCK_TRACK Track 
	)
{
	LVITEM lvi = {0};
	LONG Number;
	PCCR_LOCK_TRACK Sibling;
	PCCR_STACKTRACE Trace;
	LARGE_INTEGER Time;
	double Milliseconds;
	PULONG Array;
	WCHAR Buffer[MAX_PATH];
	int i, j;

	//
	// Address
	//

	lvi.iItem = Index;
	lvi.iSubItem = CcrUiAddress;
	lvi.mask = LVIF_TEXT|LVIF_PARAM;
	lvi.lParam = (LPARAM)Track;
	StringCchPrintf(Buffer, MAX_PATH, L"0x%p", Track->LockPtr);
	lvi.pszText = Buffer;
	ListView_InsertItem(hWndCtrl, &lvi);

	//
	// Type 
	//

	lvi.iSubItem = CcrUiType;
	lvi.mask = LVIF_TEXT;
	if (Track->Type == CCR_LOCK_CS){
		StringCchCopy(Buffer, MAX_PATH, L"CS");
	} else if (Track->Type == CCR_LOCK_SRW) {
		StringCchCopy(Buffer, MAX_PATH, L"SRW");
	} else {
		StringCchCopy(Buffer, MAX_PATH, L"N/A");
	}
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Owner 
	//

	lvi.iSubItem = CcrUiOwner;
	lvi.mask = LVIF_TEXT;
	StringCchCopy(Buffer, MAX_PATH, L"N/A");
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Acquire 
	//

	lvi.iSubItem = CcrUiAcquire;
	lvi.mask = LVIF_TEXT;

	Number = 0;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		Number += Sibling->Acquire;
		Sibling = Sibling->SiblingAcquirers;
	}
	Number += Track->Acquire;

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Number);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Try Acquire 
	//

	lvi.iSubItem = CcrUiTryAcquire;
	lvi.mask = LVIF_TEXT;

	Number = 0;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		Number += Sibling->TryAcquire;
		Sibling = Sibling->SiblingAcquirers;
	}
	Number += Track->TryAcquire;

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Number);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Try Failure 
	//

	lvi.iSubItem = CcrUiTryFailure;
	lvi.mask = LVIF_TEXT;

	Number = 0;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		Number += Sibling->TryFailure;
		Sibling = Sibling->SiblingAcquirers;
	}
	Number += Track->TryFailure;

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Number);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Acquire Path 
	//

	lvi.iSubItem = CcrUiAcquirePath;
	lvi.mask = LVIF_TEXT;

	Number = 0;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		Number += Sibling->StackTraceCount;
		Sibling = Sibling->SiblingAcquirers;
	}
	Number += Track->StackTraceCount;

	if (Number * sizeof(ULONG) < sizeof(Buffer)){
		Array = (PULONG)Buffer;
	} else {
		Array = (PULONG)ApsMalloc(Number * sizeof(ULONG));
	}

	Number = 0;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		Trace = Sibling->StackTrace;
		while (Trace) {
			Array[Number] = Trace->u.StackId;
			Trace = Trace->Next;
			Number += 1;
		}
		Sibling = Sibling->SiblingAcquirers;
	}

	qsort(Array, Number, sizeof(ULONG), CcrContentionStackIdSortCallback);
	for(i = 0, j = 0; i < Number - 1; i++){
		if (Array[i] == Array[i + 1]){
			j += 1;
		}
	}

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Number - j);
	DebugTrace("DUPLICATED : %d", j);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	if (Array != (PULONG)Buffer) {
		ApsFree(Array);
	}

	//
	// Acquire Thread 
	//

	lvi.iSubItem = CcrUiAcquireThread;
	lvi.mask = LVIF_TEXT;

	Number = 0;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		Number += 1;
		Sibling = Sibling->SiblingAcquirers;
	}
	Number += 1;

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Number);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Max Acquire Latency 
	//

	lvi.iSubItem = CcrUiMaxLatency;
	lvi.mask = LVIF_TEXT;

	Time = Track->MaximumAcquireLatency;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		if (Sibling->MaximumAcquireLatency.QuadPart > Time.QuadPart){
			Time = Sibling->MaximumAcquireLatency;
		}
		Sibling = Sibling->SiblingAcquirers;
	}

	Milliseconds = ApsComputeMilliseconds((ULONG)Time.QuadPart);
	StringCchPrintf(Buffer, MAX_PATH, L"%.06f", Milliseconds);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);

	//
	// Max Holding Duration 
	//

	lvi.iSubItem = CcrUiMaxDuration;
	lvi.mask = LVIF_TEXT;

	Time = Track->MaximumHoldingDuration;
	Sibling = Track->SiblingAcquirers;
	while (Sibling) {
		if (Sibling->MaximumHoldingDuration.QuadPart > Time.QuadPart){
			Time = Sibling->MaximumHoldingDuration;
		}
		Sibling = Sibling->SiblingAcquirers;
	}

	Milliseconds = ApsComputeMilliseconds((ULONG)Time.QuadPart);
	StringCchPrintf(Buffer, MAX_PATH, L"%.06f", Milliseconds);
	lvi.pszText = Buffer;
	ListView_SetItem(hWndCtrl, &lvi);
}

VOID
CcrContentionInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{	
	LVITEM lvi = {0};
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	HWND hWndCtrl;
	int Index = 0;
	int i;
	PCCR_LOCK_TABLE Table;
	PCCR_LOCK_TRACK Track;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
	Context->Head = Head;

	Table = CcrBuildLockTable(Head);
	ASSERT(Table != NULL);
	Context->Table = Table;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_CONTENTION);
	Index = 0;

	for(i = 0; i < CCR_LOCK_TALBE_BUCKET; i++) {

		ListHead = &Table->LockList[i];
		ListEntry = ListHead->Flink;

		while (ListHead != ListEntry) {
			Track = CONTAINING_RECORD(ListEntry, CCR_LOCK_TRACK, ListEntry);
			CcrContentionInsertListItem(hWndCtrl, Index, Track);
			Index += 1;
			ListEntry = ListEntry->Flink;
		}

	}
}

VOID
CcrInsertLockTrackBucket(
	_In_ PCCR_LOCK_TABLE Table,
	_In_ PLIST_ENTRY ListHead,
	_In_ PCCR_LOCK_TRACK Track
	)
{
	PLIST_ENTRY ListEntry;
	PCCR_LOCK_TRACK Entry;

	//
	// Walk the bucket, and chain the sibling lock track if they
	// have same lock ptr 
	//

	Track->SiblingAcquirers = NULL;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, CCR_LOCK_TRACK, ListEntry);
		if (Entry->LockPtr == Track->LockPtr) {
			Track->SiblingAcquirers = Entry->SiblingAcquirers;
			Entry->SiblingAcquirers = Track;
			return;
		}
		ListEntry = ListEntry->Flink;
	}

	InsertHeadList(ListHead, &Track->ListEntry);
	Table->Count += 1;
}


#define GET_FIRST_LOCKTRACK(_I)\
	((PCCR_LOCK_TRACK)(_I + 1))

#define GET_LOCKTRACK_LIMIT(_I, _L)\
	((ULONG_PTR)_L + _I->Size - sizeof(CCR_LOCK_INDEX))

#define GET_NEXT_LOCKTRACK(_L) \
	((PCCR_LOCK_TRACK)((ULONG_PTR)(_L + 1) + sizeof(CCR_STACKTRACE) * _L->StackTraceCount))

VOID
CcrInsertLockPerThread(
	_In_ PCCR_LOCK_INDEX Index,
	_In_ PCCR_LOCK_TABLE Table
	)
{
	PCCR_LOCK_TRACK Track;
	PCCR_STACKTRACE Trace;
	ULONG_PTR Limit;
	ULONG Bucket;
	ULONG Number;

	Track = GET_FIRST_LOCKTRACK(Index);
	Limit = GET_LOCKTRACK_LIMIT(Index, Track);

	while ((ULONG_PTR)Track < Limit) {

		Track->TrackThreadId = Index->ThreadId;

		//
		// Fix the stacktrace pointers
		//
		
		Trace = (PCCR_STACKTRACE)(Track + 1);
		Track->StackTrace = Trace;

		for(Number = 0; Number < Track->StackTraceCount - 1; Number += 1) {
			Trace[Number].Next = &Trace[Number + 1];
		}
		Trace[Number].Next = NULL;


		//
		// Insert lock track into its bucket
		//

		Bucket = GET_CCR_LOCK_TABLE_BUCKET(Track->LockPtr);
		CcrInsertLockTrackBucket(Table, &Table->LockList[Bucket], Track);

		//
		// Move to next lock track entry
		//

		Track = GET_NEXT_LOCKTRACK(Track);
	}
}

PCCR_LOCK_TABLE
CcrBuildLockTable(
	_In_ PPF_REPORT_HEAD Head
	)
{
	PCCR_LOCK_TABLE Table;
	PCCR_LOCK_INDEX Index;
	ULONG Length;
	PUCHAR Base;
	ULONG Number;

	Table = (PCCR_LOCK_TABLE)ApsMalloc(sizeof(CCR_LOCK_TABLE));
	Table->Count = 0;

	for(Number = 0; Number < CCR_LOCK_TALBE_BUCKET; Number += 1) {
		InitializeListHead(&Table->LockList[Number]);
	}

	Base = (PUCHAR)ApsGetStreamPointer(Head, STREAM_CCR);
	Length = ApsGetStreamLength(Head, STREAM_CCR);
	ASSERT(Length != 0);

	Index = (PCCR_LOCK_INDEX)Base;

	do {
		CcrInsertLockPerThread(Index, Table);
		Index = (PCCR_LOCK_INDEX)((PUCHAR)Index + Index->Size);
	} while ((ULONG_PTR)Index < (ULONG_PTR)Base + Length);

	return Table;
}