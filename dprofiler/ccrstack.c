//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#include "aps.h"
#include "apsdefs.h"
#include "apsprofile.h"
#include "treelist.h"
#include "cputhread.h"
#include "profileform.h"
#include "split.h"
#include "apspdb.h"
#include "frame.h"
#include "apsrpt.h"
#include "resource.h"
#include "ccrstack.h"
#include "apsbtr.h"


static
DIALOG_SCALER_CHILD CcrStackChildren[] = {
	{ IDC_LIST_CCR_STACK_LOCK, AlignNone, AlignBottom },
	{ IDC_LIST_CCR_STACK_PC, AlignRight, AlignBottom },
	{ IDC_BUTTON_CCR_EXPORT, AlignBoth, AlignBoth },
	{ IDOK, AlignBoth, AlignBoth }
};

static
DIALOG_SCALER CcrStackScaler = {
	{0,0}, {0,0}, {0,0}, 4, CcrStackChildren
};

//
// Left pane show all threads' properties
//

static
LISTVIEW_COLUMN CcrStackColumn[] = {
	{ 80,  L"TID",	LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Stack ID", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Count", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define CCR_STACK_COLUMN_NUM  (sizeof(CcrStackColumn)/sizeof(LISTVIEW_COLUMN))

//
// Right pane show PC statistics per thread
//

static 
LISTVIEW_COLUMN CcrStackPcColumn[] = {
	{ 40,	L"#",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Frame",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Module",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240,	L"Line",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define CCR_STACK_PC_COLUMN_NUM  (sizeof(CcrStackPcColumn)/sizeof(LISTVIEW_COLUMN))

HWND
CcrStackCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId,
	__in PPF_REPORT_HEAD Head,
	__in PCCR_LOCK_TRACK Lock
	)
{
	DIALOG_OBJECT Object = {0};
	CCR_FORM_CONTEXT Context = {0};
	
	Context.CtrlId = CtrlId;
	Context.Head = Head;
	Context.Path[0] = 0;
	Context.TreeList = NULL;
	Context.Lock = Lock;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_CCR_STACK;
	Object.Procedure = CcrStackProcedure;

	DialogCreate(&Object);
	return 0;
}

LRESULT
CcrStackOnInitDialog(
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
	LVITEM lvi = {0};
	ULONG i;
	PLISTVIEW_OBJECT ListView;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
	
	Context->hBrushBack = CreateSolidBrush(RGB(255, 255, 255));

	//
	// Create listview object wraps list control
	//

	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = CcrStackColumn;
    ListView->Count = CCR_STACK_COLUMN_NUM;
	ListView->NotifyCallback = CcrStackOnNotify;
	
	Context->ListView = ListView;

    //
    // Initialize left pane
    //

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_STACK_LOCK);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

	for (i = 0; i < CCR_STACK_COLUMN_NUM; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CcrStackColumn[i].Title;	
		lvc.cx = CcrStackColumn[i].Width;     
		lvc.fmt = CcrStackColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Initialize right pane 
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_STACK_PC);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

	for (i = 0; i < CCR_STACK_PC_COLUMN_NUM; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CcrStackPcColumn[i].Title;	
		lvc.cx = CcrStackPcColumn[i].Width;     
		lvc.fmt = CcrStackPcColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Register dialog scaler
	//

	Object->Scaler = &CcrStackScaler;
	DialogRegisterScaler(Object);

	//
	// Insert lock object
	//

	CcrStackInsertLock(hWnd, Context->Head, Context->Lock);
	CcrInsertBackTrace(hWnd, 0);
	return TRUE;
}

LRESULT CALLBACK 
CcrStackHeaderProcedure(
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
CcrStackOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CcrStackOnClose(
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
CcrStackProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CcrStackOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CcrStackOnClose(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CcrStackOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		return CcrStackOnCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CcrStackOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return TRUE;
}

LRESULT
CcrStackOnExport(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CcrStackOnCommand(
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
		case IDOK:
			return CcrStackOnOk(hWnd, uMsg, wp, lp);
		case IDC_BUTTON_CCR_EXPORT:           
			return CcrStackOnExport(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
CcrStackOnNotify(
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

	switch (pNmhdr->code) {

		case LVN_COLUMNCLICK:
			Status = CcrStackOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;

		case LVN_ITEMCHANGED:
			if(IDC_LIST_CCR_STACK_LOCK == pNmhdr->idFrom) {
				Status = CcrStackOnItemChanged(Object, (LPNMLISTVIEW)lp);
			}
			break;
	}

	return Status;
}

LRESULT 
CcrStackOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
    if (lpNmlv->uNewState & LVIS_SELECTED) {
		CcrInsertBackTrace(Object->hWnd, lpNmlv->iItem);
    }

    return 0L;
}

LRESULT 
CcrStackOnColumnClick(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int nColumnCount;
	HDITEM hdi;
	LISTVIEW_OBJECT *ListView;
	PCCR_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;
	int i;

	if (lpNmlv->hdr.idFrom != IDC_LIST_CCR_STACK_LOCK){
		return 0;
	}

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
	hWndCtrl = lpNmlv->hdr.hwndFrom; 

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
	ListView_SortItemsEx(hWndCtrl, CcrStackSortCallback, (LPARAM)hWnd);

    return 0L;
}

int CALLBACK
CcrStackSortCallback(
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
	HWND hWnd;
    int Result;
	HWND hWndList;
	int I1, I2;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_CCR_STACK_LOCK);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);

	ListView = Context->ListView;

	ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);

	I1 = wcstol(FirstData, NULL, 10);
	I2 = wcstol(SecondData, NULL, 10);
	Result = I1 - I2;

	return ListView->SortOrder ? Result : -Result;
}

VOID
CcrStackInsertLock(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head,
	__in PCCR_LOCK_TRACK Lock
    )
{
    PDIALOG_OBJECT Object;
    PCCR_FORM_CONTEXT Context;
    ULONG Number;
    HWND hWndCtrl;
    ULONG i;
    LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	PCCR_STACKTRACE Trace;
	PCCR_LOCK_TRACK Sibling;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCCR_FORM_CONTEXT)Object->Context;
    Context->Head = Head;

    //
    // Fill the threads into listview
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_STACK_LOCK);
    ASSERT(hWndCtrl != NULL); 

    Number = 0;
	Sibling = Lock;

	while (Sibling) {

		Trace = Sibling->StackTrace;
		for(i = 0; i < Sibling->StackTraceCount; i++) {

			//
			// TID
			//

			lvi.iItem = Number;
			lvi.iSubItem = 0;
			lvi.mask = LVIF_TEXT|LVIF_PARAM;
			lvi.lParam = (LPARAM)Trace;

			StringCchPrintf(Buffer, MAX_PATH, L"%u", Sibling->TrackThreadId);
			lvi.pszText = Buffer;
			ListView_InsertItem(hWndCtrl, &lvi);

			//
			// Stack ID
			//

			lvi.iSubItem = 1;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", Trace->u.StackId);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Count
			//

			lvi.iSubItem = 2;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", Trace->Count);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Move to next item 
			//

			Number += 1;
			Trace = Trace->Next;
		}
		
		Sibling = Sibling->SiblingAcquirers;
	}

    //
    // Set focus to select item 0 and trigger a data update into
    // right pane
    //

    SetFocus(hWndCtrl);
    ListViewSelectSingle(hWndCtrl, 0);
}

VOID
CcrInsertBackTrace(
	__in HWND hWnd,
    __in int Index 
	)
{
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT ObjectContext;
	PPF_REPORT_HEAD Report;
	PBTR_STACK_RECORD Record;
	HWND hWndCtrl;
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_DLL_FILE DllFile;
	PCCR_STACKTRACE Trace;
	WCHAR Buffer[MAX_PATH];
    ULONG StackTraceId;
	ULONG i;
	LVITEM lvi = {0};

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_STACK_LOCK); 
	ListViewGetParam(hWndCtrl, Index, (LPARAM *)&Trace);
	StackTraceId = Trace->u.StackId;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ObjectContext = SdkGetContext(Object, CCR_FORM_CONTEXT);
	Report = ObjectContext->Head;

	if (!Report) {
		return;
	}

	//
	// Clear old list items
	//

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CCR_STACK_PC); 
	ListView_DeleteAllItems(hWndCtrl);

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Report, STREAM_DLL);
    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Report, STREAM_STACK);
	Record = &Record[StackTraceId];

	if (Report->Streams[STREAM_LINE].Offset != 0 && 
		Report->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Report + Report->Streams[STREAM_LINE].Offset);
	} else {
		Line = NULL;
	}

	Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;
		
		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID) Table;
	}

	for(i = 0; i < Record->Depth; i++) {

		Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[i]);

		//
		// frame number
		//

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%02u", i);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);
		
		//
		// symbol name
		//

		lvi.iItem = i;
		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;

		if (Text != NULL) {
			StringCchPrintf(Buffer, MAX_PATH, L"%S", Text->Text);
		} 
		
		else {

#if defined(_M_X64)
			StringCchPrintf(Buffer, MAX_PATH, L"0x%I64x", (ULONG64)Record->Frame[i]);

#elif defined(_M_IX86)
			StringCchPrintf(Buffer, MAX_PATH, L"0x%x", (ULONG)Record->Frame[i]);
#endif
		}

		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Module
		//

		lvi.iItem = i;
		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;

		DllEntry = ApsGetDllEntryByPc(DllFile, Record->Frame[i]);
		if (DllEntry) {
			ApsGetDllBaseNameById(Report, DllEntry->DllId, Buffer, MAX_PATH);
		} else {
			StringCchCopy(Buffer, MAX_PATH, L"N/A");
		}
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// line information
		//

		lvi.iItem = i;
		lvi.iSubItem = 3;
		lvi.mask = LVIF_TEXT;

		if (Text != NULL && Text->LineId != -1) {
			ASSERT(Line != NULL);
			LineEntry = Line + Text->LineId;
			StringCchPrintf(Buffer, MAX_PATH, L"%S:%u", LineEntry->File, LineEntry->Line);
			lvi.pszText = Buffer; 
		} else {
			lvi.pszText = L""; 
		}
		
		ListView_SetItem(hWndCtrl, &lvi);

	}
}
