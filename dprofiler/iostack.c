//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#include "aps.h"
#include "apsdefs.h"
#include "apsprofile.h"
#include "treelist.h"
#include "profileform.h"
#include "split.h"
#include "apspdb.h"
#include "frame.h"
#include "apsrpt.h"
#include "resource.h"
#include "iostack.h"
#include "apsbtr.h"
#include "apsio.h"


static
DIALOG_SCALER_CHILD IoStackChildren[] = {
	{ IDC_LIST_IO_STACK, AlignNone, AlignBottom },
	{ IDC_LIST_IO_STACK_PC, AlignRight, AlignBottom },
	{ IDC_BUTTON_IO_EXPORT, AlignBoth, AlignBoth },
	{ IDOK, AlignBoth, AlignBoth }
};

static
DIALOG_SCALER IoStackScaler = {
	{0,0}, {0,0}, {0,0}, 4, IoStackChildren
};

enum _IoStackColumn {
	_IoStackId,
	_IoStackCount,
	_IoStackMethod,
	_IoStackBytes,
};

static
LISTVIEW_COLUMN IoStackColumn[] = {
	{ 80,  L"Stack ID", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Count", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Method", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 146, L"IO Bytes", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define IO_STACK_COLUMN_NUM  (sizeof(IoStackColumn)/sizeof(LISTVIEW_COLUMN))

//
// Right pane show PC statistics per thread
//

static 
LISTVIEW_COLUMN IoStackPcColumn[] = {
	{ 40,	L"#",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Frame",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Module",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240,	L"Line",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define IO_STACK_PC_COLUMN_NUM  (sizeof(IoStackPcColumn)/sizeof(LISTVIEW_COLUMN))


HWND
IoStackCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId,
	__in PPF_REPORT_HEAD Head,
	__in PIO_OBJECT_ON_DISK IoObject 
	)
{
	DIALOG_OBJECT Object = {0};
	IO_FORM_CONTEXT Context = {0};
	
	Context.CtrlId = CtrlId;
	Context.Head = Head;
	Context.Path[0] = 0;
	Context.TreeList = NULL;
	Context.IoObject = IoObject;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_IO_STACK;
	Object.Procedure = IoStackProcedure;

	DialogCreate(&Object);
	return 0;
}

LRESULT
IoStackOnInitDialog(
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
	LVITEM lvi = {0};
	ULONG i;
	PLISTVIEW_OBJECT ListView;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	
	Context->hBrushBack = CreateSolidBrush(RGB(255, 255, 255));

	//
	// Create listview object wraps list control
	//

	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = IoStackColumn;
    ListView->Count = IO_STACK_COLUMN_NUM;
	ListView->NotifyCallback = IoStackOnNotify;
	
	Context->ListView = ListView;

    //
    // Initialize left pane
    //

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_STACK);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

	for (i = 0; i < IO_STACK_COLUMN_NUM; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = IoStackColumn[i].Title;	
		lvc.cx = IoStackColumn[i].Width;     
		lvc.fmt = IoStackColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Initialize right pane 
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_STACK_PC);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

	for (i = 0; i < IO_STACK_PC_COLUMN_NUM; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = IoStackPcColumn[i].Title;	
		lvc.cx = IoStackPcColumn[i].Width;     
		lvc.fmt = IoStackPcColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Register dialog scaler
	//

	Object->Scaler = &IoStackScaler;
	DialogRegisterScaler(Object);

	IoStackInsertObject(hWnd, Context->Head, Context->IoObject);
	IoInsertBackTrace(hWnd, 0);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT CALLBACK 
IoStackHeaderProcedure(
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
IoStackOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoStackOnClose(
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
IoStackProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return IoStackOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return IoStackOnClose(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return IoStackOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		return IoStackOnCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
IoStackOnOk(
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
IoStackOnExport(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoStackOnCommand(
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
			return IoStackOnOk(hWnd, uMsg, wp, lp);
		case IDC_BUTTON_IO_EXPORT:           
			return IoStackOnExport(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
IoStackOnNotify(
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
			Status = IoStackOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;

		case LVN_ITEMCHANGED:
			if(IDC_LIST_IO_STACK == pNmhdr->idFrom) {
				Status = IoStackOnItemChanged(Object, (LPNMLISTVIEW)lp);
			}
			break;
	}

	return Status;
}

LRESULT 
IoStackOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
    if (lpNmlv->uNewState & LVIS_SELECTED) {
		IoInsertBackTrace(Object->hWnd, lpNmlv->iItem);
    }

    return 0L;
}

LRESULT 
IoStackOnColumnClick(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int nColumnCount;
	HDITEM hdi;
	LISTVIEW_OBJECT *ListView;
	PIO_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;
	int i;

	if (lpNmlv->hdr.idFrom != IDC_LIST_IO_STACK){
		return 0;
	}

	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
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
	ListView_SortItemsEx(hWndCtrl, IoStackSortCallback, (LPARAM)hWnd);

    return 0L;
}

int CALLBACK
IoStackSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	HWND hWnd;
    int Result;
	HWND hWndList;
	int I1, I2;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_IO_STACK);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);

	ListView = Context->ListView;

	ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);

	switch (ListView->LastClickedColumn) {
	case _IoStackId:
	case _IoStackCount:
	case _IoStackBytes:
		I1 = wcstol(FirstData, NULL, 10);
		I2 = wcstol(SecondData, NULL, 10);
		Result = I1 - I2;
		break;
	case _IoStackMethod:
		Result = wcscmp(FirstData, SecondData);
		break;
	default:
		ASSERT(0);
		Result = 0;
	}

	return ListView->SortOrder ? Result : -Result;
}

VOID
IoStackInsertObject(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head,
	__in PIO_OBJECT_ON_DISK IoObject 
    )
{
    PDIALOG_OBJECT Object;
    PIO_FORM_CONTEXT Context;
    ULONG Number;
    HWND hWndCtrl;
    LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	PIO_STACKTRACE Trace;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PIO_FORM_CONTEXT)Object->Context;
    Context->Head = Head;

    //
    // Fill the stack trace counters into listview
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_STACK);
    ASSERT(hWndCtrl != NULL); 

	Number = ApsIoBuildObjectStackTrace(Head, IoObject);
	if (!Number) {
		return;
	}

	Trace = IoObject->Trace;
	ASSERT(Trace != NULL);

	Number = 0;
	while (Trace) {

		//
		// Stack ID
		//

		lvi.iItem = Number;
		lvi.iSubItem = _IoStackId;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.lParam = (LPARAM)Trace;

		StringCchPrintf(Buffer, MAX_PATH, L"%u", Trace->StackId);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);

		//
		// Count 
		//

		lvi.iSubItem = _IoStackCount;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%d", Trace->Count);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Method
		//

		lvi.iSubItem = _IoStackMethod;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%S", ApsIoGetOpString(Trace->Operation));
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Complete Bytes 
		//

		lvi.iSubItem = _IoStackBytes;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%d", Trace->CompleteBytes);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Move to next item 
		//

		Number += 1;
		if (Trace->Next != IO_INVALID_STACTRACE){
			Trace = IoObject->Trace + Trace->Next;
		} else {
			Trace = NULL;
		}
	}

    //
    // Set focus to select item 0 and trigger a data update into
    // right pane
    //

    SetFocus(hWndCtrl);
    ListViewSelectSingle(hWndCtrl, 0);
}

VOID
IoInsertBackTrace(
	__in HWND hWnd,
    __in int Index 
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT ObjectContext;
	PPF_REPORT_HEAD Report;
	PBTR_STACK_RECORD Record;
	HWND hWndCtrl;
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_DLL_FILE DllFile;
	WCHAR Buffer[MAX_PATH];
	PIO_STACKTRACE Trace;
	ULONG i;
	LVITEM lvi = {0};

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_STACK); 
	ListViewGetParam(hWndCtrl, Index, (LPARAM *)&Trace);
	if (!Trace){
		return;
	}

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ObjectContext = SdkGetContext(Object, IO_FORM_CONTEXT);
	Report = ObjectContext->Head;

	if (!Report) {
		return;
	}

	//
	// Clear old list items
	//

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_STACK_PC); 
	ListView_DeleteAllItems(hWndCtrl);

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Report, STREAM_DLL);
    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Report, STREAM_STACK);
	Record = &Record[Trace->StackId];

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
