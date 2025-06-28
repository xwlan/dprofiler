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
#include "cpupcstack.h"


static
DIALOG_SCALER_CHILD CpuPcStackChildren[] = {
	{ IDC_LIST_PCSTACK_LEFT, AlignNone, AlignBottom },
	{ IDC_LIST_PCSTACK_RIGHT, AlignRight, AlignBottom },
	{ IDC_BUTTON_PCSTACK_EXPORT, AlignBoth, AlignBoth },
	{ IDOK, AlignBoth, AlignBoth }
};

static
DIALOG_SCALER CpuPcStackScaler = {
	{0,0}, {0,0}, {0,0}, 4, CpuPcStackChildren
};

//
// Left pane show all threads' properties
//

static
LISTVIEW_COLUMN CpuPcStackColumn[] = {
	{ 80,  L"Stack ID", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Count",    LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Time %",   LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define PCSTACK_LEFT_COLUMN_NUM  (sizeof(CpuPcStackColumn)/sizeof(LISTVIEW_COLUMN))

//
// Right pane show PC statistics per thread
//

static
LISTVIEW_COLUMN CpuPcStackPcColumn[] = {
	{ 40,	L"#",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Frame",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Module",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240,	L"Line",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define PCSTACK_RIGHT_COLUMN_NUM  (sizeof(CpuPcStackPcColumn)/sizeof(LISTVIEW_COLUMN))


HWND
CpuPcStackCreate(
	IN HWND hWndParent,
	IN ULONG CtrlId,
	IN PPF_REPORT_HEAD Head,
	IN PCPU_PC_ENTRY Pc,
	IN ULONG ThreadId
	)
{
	DIALOG_OBJECT Object = { 0 };
	CPU_FORM_CONTEXT Context = { 0 };
	CPU_PCSTACK_CONTEXT PcContext;

	PcContext.Pc = Pc;
	PcContext.ThreadId = ThreadId;

	Context.CtrlId = CtrlId;
	Context.Head = Head;
	Context.Path[0] = 0;
	Context.Context = &PcContext;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_PCSTACK;
	Object.Procedure = CpuPcStackProcedure;

	DialogCreate(&Object);
	return 0;
}

LRESULT
CpuPcStackOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = { 0 };
	LVITEM lvi = { 0 };
	ULONG i;
	PLISTVIEW_OBJECT ListView;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	Context->hBrushBack = CreateSolidBrush(RGB(255, 255, 255));

	//
	// Create listview object wraps list control
	//

	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = CpuPcStackColumn;
	ListView->Count = PCSTACK_LEFT_COLUMN_NUM;
	ListView->NotifyCallback = CpuPcStackOnNotify;

	Context->ListView = ListView;

	//
	// Initialize left pane
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PCSTACK_LEFT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT,
		LVS_EX_FULLROWSELECT);

	for (i = 0; i < PCSTACK_LEFT_COLUMN_NUM; i++) {
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = i;
		lvc.pszText = CpuPcStackColumn[i].Title;
		lvc.cx = CpuPcStackColumn[i].Width;
		lvc.fmt = CpuPcStackColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
	}

	//
	// Initialize right pane 
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PCSTACK_RIGHT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT,
		LVS_EX_FULLROWSELECT);

	for (i = 0; i < PCSTACK_RIGHT_COLUMN_NUM; i++) {
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = i;
		lvc.pszText = CpuPcStackPcColumn[i].Title;
		lvc.cx = CpuPcStackPcColumn[i].Width;
		lvc.fmt = CpuPcStackPcColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
	}

	//
	// Register dialog scaler
	//

	Object->Scaler = &CpuPcStackScaler;
	DialogRegisterScaler(Object);

	//
	// Insert stacktrace into list controls
	//

	CpuPcStackInsertPcCount(hWnd, Context->Head);
	CpuPcStackInsertBackTrace(hWnd, 0);

	SdkCenterWindow(NULL);
	return TRUE;
}

LRESULT CALLBACK
CpuPcStackHeaderProcedure(
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
CpuPcStackOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
)
{
	return 0;
}

LRESULT
CpuPcStackOnClose(
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
CpuPcStackProcedure(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuPcStackOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuPcStackOnClose(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuPcStackOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		return CpuPcStackOnCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuPcStackOnOk(
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
CpuPcStackOnExport(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
)
{
	return 0;
}

LRESULT
CpuPcStackOnCommand(
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
		return CpuPcStackOnOk(hWnd, uMsg, wp, lp);
	case IDC_BUTTON_PCSTACK_EXPORT:
		return CpuPcStackOnExport(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
CpuPcStackOnNotify(
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
		Status = CpuPcStackOnColumnClick(Object, (NM_LISTVIEW*)lp);
		break;

	case LVN_ITEMCHANGED:
		if (IDC_LIST_PCSTACK_LEFT == pNmhdr->idFrom) {
			Status = CpuPcStackOnItemChanged(Object, (LPNMLISTVIEW)lp);
		}
		break;
	}

	return Status;
}

LRESULT
CpuPcStackOnItemChanged(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW* lpNmlv
)
{
	if (lpNmlv->uNewState & LVIS_SELECTED) {
		CpuPcStackInsertBackTrace(Object->hWnd, lpNmlv->iItem);
	}

	return 0L;
}

LRESULT
CpuPcStackOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW* lpNmlv
)
{
	HWND hWndHeader;
	int nColumnCount;
	HDITEM hdi;
	LISTVIEW_OBJECT* ListView;
	PCPU_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;
	int i;

	if (lpNmlv->hdr.idFrom != IDC_LIST_PCSTACK_LEFT) {
		return 0;
	}

	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	ListView = Context->ListView;

	if (ListView->SortOrder == SortOrderNone) {
		return 0;
	}

	if (ListView->LastClickedColumn == lpNmlv->iSubItem) {
		ListView->SortOrder = (LIST_SORT_ORDER)!ListView->SortOrder;
	}
	else {
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
			if (ListView->SortOrder == SortOrderAscendent) {
				hdi.fmt |= HDF_SORTUP;
			}
			else {
				hdi.fmt |= HDF_SORTDOWN;
			}
		}
		else {
			hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		}

		Header_SetItem(hWndHeader, i, &hdi);
	}

	ListView->LastClickedColumn = lpNmlv->iSubItem;
	ListView_SortItemsEx(hWndCtrl, CpuPcStackSortCallback, (LPARAM)hWnd);

	return 0L;
}

int CALLBACK
CpuPcStackSortCallback(
	__in LPARAM First,
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	WCHAR FirstData[MAX_PATH + 1];
	WCHAR SecondData[MAX_PATH + 1];
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	LISTVIEW_OBJECT* ListView;
	HWND hWnd;
	int Result;
	HWND hWndList;
	int I1, I2;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_PCSTACK_LEFT);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	ListView = Context->ListView;

	ListView_GetItemText(hWndList, First, ListView->LastClickedColumn, FirstData, MAX_PATH);
	ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);

	I1 = wcstol(FirstData, NULL, 10);
	I2 = wcstol(SecondData, NULL, 10);
	Result = I1 - I2;

	return ListView->SortOrder ? Result : -Result;
}

VOID
CpuPcStackInsertPcCount(
	IN HWND hWnd,
	IN PPF_REPORT_HEAD Head
	)
{
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVITEM lvi = { 0 };
	WCHAR Buffer[MAX_PATH];
	PCPU_PC_STACKTRACE Stack;
	PCPU_PCSTACK_CONTEXT PcContext;
	ULONG i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCCR_FORM_CONTEXT)Object->Context;
	Context->Head = Head;
	PcContext = (PCPU_PCSTACK_CONTEXT)Context->Context;

	//
	// Build stracktrace list for specified Pc entry
	//

	Stack = CpuBuildStackTraceListForPc(Head, PcContext->ThreadId, 
										PcContext->Pc, TRUE);
	ASSERT(Stack != NULL);
	PcContext->Stack = Stack;

	//
	// Fill the threads into listview
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PCSTACK_LEFT);
	ASSERT(hWndCtrl != NULL);

	for (i = 1; i < Stack->Count + 1; i++) { 

		//
		// StackId  
		//

		lvi.iItem = i - 1;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.lParam = (LPARAM)&Stack[i];

		StringCchPrintf(Buffer, MAX_PATH, L"%u", Stack[i].StackId);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);

		//
		// Count
		//

		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%d", Stack[i].Count);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);
	}
	
	//
	// Set focus to select item 0 and trigger a data update into
	// right pane
	//

	SetFocus(hWndCtrl);
	ListViewSelectSingle(hWndCtrl, 0);
}

VOID
CpuPcStackInsertBackTrace(
	__in HWND hWnd,
	__in int Index
)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT ObjectContext;
	PPF_REPORT_HEAD Report;
	PBTR_STACK_RECORD Record;
	HWND hWndCtrl;
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_DLL_FILE DllFile;
	PCPU_PC_STACKTRACE Trace;
	WCHAR Buffer[MAX_PATH];
	ULONG StackTraceId;
	ULONG i;
	LVITEM lvi = { 0 };

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PCSTACK_LEFT);
	ListViewGetParam(hWndCtrl, Index, (LPARAM*)&Trace);
	StackTraceId = Trace->StackId;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ObjectContext = SdkGetContext(Object, CPU_FORM_CONTEXT);
	Report = ObjectContext->Head;

	if (!Report) {
		return;
	}

	//
	// Clear old list items
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PCSTACK_RIGHT);
	ListView_DeleteAllItems(hWndCtrl);

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Report, STREAM_DLL);
	Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Report, STREAM_STACK);
	Record = &Record[StackTraceId];

	if (Report->Streams[STREAM_LINE].Offset != 0 &&
		Report->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Report + Report->Streams[STREAM_LINE].Offset);
	}
	else {
		Line = NULL;
	}

	Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;

		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID)Table;
	}

	for (i = 0; i < Record->Depth; i++) {

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
		}
		else {
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
		}
		else {
			lvi.pszText = L"";
		}

		ListView_SetItem(hWndCtrl, &lvi);

	}
}
