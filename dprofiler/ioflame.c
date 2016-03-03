//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"
#include "ioflame.h"
#include "calltree.h"
#include "flamegraph.h"
#include "resource.h"

DIALOG_SCALER_CHILD IoFlameChildren[1] = {
	{ IDC_IO_FLAME, AlignRight, AlignBottom }
};

DIALOG_SCALER IoFlameScaler = {
	{0,0}, {0,0}, {0,0}, 1, IoFlameChildren
};

typedef struct _IO_FLAME_CONTEXT {
    PPF_REPORT_HEAD Head;
    PCALL_GRAPH Graph;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_LINE_ENTRY LineEntry;
    PBTR_TEXT_TABLE TextTable;
} IO_FLAME_CONTEXT, *PIO_FLAME_CONTEXT;

HWND
IoFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR CtrlId
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PIO_FORM_CONTEXT)SdkMalloc(sizeof(IO_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_IO_FLAME;
	Object->Procedure = IoFlameProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
IoFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PIO_FORM_CONTEXT Context;
	PDIALOG_OBJECT Object;
	RECT Rect;
	HWND hWndCtrl;
    PFLAME_CONTROL Control;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	
    //
    // Initialize the flame control
    //

	hWndCtrl = GetDlgItem(hWnd, IDC_IO_FLAME);
    Control = FlameInitializeControl(hWndCtrl, 1, 0, 0, RGB(0xEE, 0xEE, 0xC7), 0);

	//
	// Position flame control to fill full client area
	//

	GetClientRect(hWnd, &Rect);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);

    FlameSetSize(hWndCtrl, 1024, 1024, 1, 1, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &IoFlameScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT 
IoFlameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
IoFlameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return IoFlameOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return IoFlameOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return IoFlameOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return IoFlameOnNotify(hWnd, uMsg, wp, lp);

    case WM_FLAME_QUERYNODE:
        return IoFlameOnQueryNode(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
IoFlameOnNotify(
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

	if(IDC_TREELIST == pNmhdr->idFrom) {

		switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return IoFlameOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			break;
		}
	}


	return Status;
}

LRESULT
IoFlameOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0;
    return Status;
}

VOID
IoFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
    PDIALOG_OBJECT Object;
    PIO_FORM_CONTEXT Context;
    PIO_FLAME_CONTEXT FlameContext;
    PBTR_DLL_FILE DllFile;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_TEXT_TABLE TextTable;
    PBTR_TEXT_FILE TextFile;
    PBTR_FUNCTION_ENTRY FuncTable;
    PBTR_PC_TABLE PcTable;
    PBTR_STACK_RECORD Record;
    ULONG Count;
    PCALL_GRAPH Graph;
     
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PIO_FORM_CONTEXT)Object->Context;

    FlameContext = (PIO_FLAME_CONTEXT)ApsMalloc(sizeof(IO_FLAME_CONTEXT));
    FlameContext->Head = Head;
    FlameContext->LineEntry = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE); 

    TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
    TextTable = ApsBuildSymbolTable(TextFile, 4093);
    FlameContext->TextTable = TextTable;

    DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
    DllEntry = &DllFile->Dll[0];
    FlameContext->DllEntry = DllEntry;

    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
    Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

    FuncTable = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);
    ApsCreatePcTableFromStream(Head, &PcTable);

    //
    // Create IO call graph
    //
	
    ApsCreateCallGraph(&Graph, PROFILE_IO_TYPE, Record, Count, PcTable, FuncTable, TextTable);
    FlameContext->Graph = Graph;
    Context->Context = FlameContext;

    //
    // Set the graph to flame control
    //

    IoFlameSetGraph(Object, Graph);
}

VOID
IoFlameSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    )
{
    HWND hWndCtrl;
    PFLAME_CONTROL Control;

    ASSERT(Graph != NULL);

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_IO_FLAME);
    ASSERT(hWndCtrl != NULL);

    Control = (PFLAME_CONTROL)SdkGetObject(hWndCtrl);
    ASSERT(Control != NULL);

    Control->Graph = Graph;
    FlameBuildGraph(Control);
}

LRESULT
IoFlameOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{ 
    PDIALOG_OBJECT Object;
    PIO_FORM_CONTEXT Context;
    PIO_FLAME_CONTEXT FlameContext;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_TEXT_ENTRY TextEntry;
    PBTR_TEXT_TABLE TextTable;
    PCALL_GRAPH Graph;
    PNM_FLAME_QUERYNODE QueryNode;
    PCALL_NODE Node;
    WCHAR Module[64];
    WCHAR Symbol[MAX_PATH];
    double Percent;
     
    QueryNode = (PNM_FLAME_QUERYNODE)lp;
    if (!QueryNode) {
        return FALSE;
    }

    Node = QueryNode->Node;
    ASSERT(FlagOn(QueryNode->Flags, FLAME_QUERY_SYMBOL));

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PIO_FORM_CONTEXT)Object->Context;
    FlameContext = (PIO_FLAME_CONTEXT)Context->Context;

    //
    // Lookup symbol name for specified call node
    //

    TextTable = FlameContext->TextTable;
    TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Node->Address);
    if (!TextEntry) {
        StringCchPrintf(Symbol, MAX_PATH, L"0x%x", (PVOID)Node->Address);
    } else {
        StringCchPrintf(Symbol, MAX_PATH, L"%S", TextEntry->Text); 
    } 

    //
    // Lookup module name
    //

    DllEntry = FlameContext->DllEntry;
    if (Node->DllId != -1) {
        _wsplitpath(DllEntry[Node->DllId].Path, NULL, NULL, Module, NULL);
    } else {
        wcscpy_s(Module, 64, L"??");
    }

    //
    // Compute the percent of inclusive samples, and format the output string
    //

    if (!FlagOn(QueryNode->Flags, FLAME_QUERY_PERCENT)) {
        StringCchPrintf(QueryNode->Text, MAX_PATH, L"%s!%s", Module, Symbol);
    } else {
        Graph = FlameContext->Graph;
        Percent = (Node->Io.InclusiveBytes * 100.0) / Graph->Inclusive;
        StringCchPrintf(QueryNode->Text, MAX_PATH, L"%s!%s, %u samples, %.2f %%", 
                        Module, Symbol, Node->Io.InclusiveBytes, Percent);
    }

	return TRUE;
}