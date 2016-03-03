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
#include "split.h"
#include "ccrflame.h"
#include "calltree.h"
#include "flamegraph.h"
#include "resource.h"

DIALOG_SCALER_CHILD CcrFlameChildren[1] = {
	{ IDC_CCR_FLAME, AlignRight, AlignBottom }
};

DIALOG_SCALER CcrFlameScaler = {
	{0,0}, {0,0}, {0,0}, 1, CcrFlameChildren
};

typedef struct _CCR_FLAME_CONTEXT {
    PPF_REPORT_HEAD Head;
    PCALL_GRAPH Graph;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_LINE_ENTRY LineEntry;
    PBTR_TEXT_TABLE TextTable;
} CCR_FLAME_CONTEXT, *PCCR_FLAME_CONTEXT;

HWND
CcrFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR CtrlId
	)
{
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PCCR_FORM_CONTEXT)SdkMalloc(sizeof(CCR_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_CCR_FLAME;
	Object->Procedure = CcrFlameProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CcrFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PCCR_FORM_CONTEXT Context;
	PDIALOG_OBJECT Object;
	RECT Rect;
	HWND hWndCtrl;
    PFLAME_CONTROL Control;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
	
    //
    // Initialize the flame control
    //

	hWndCtrl = GetDlgItem(hWnd, IDC_CCR_FLAME);
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

	Object->Scaler = &CcrFlameScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT 
CcrFlameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CcrFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CcrFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
CcrFlameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CcrFlameOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CcrFlameOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return CcrFlameOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CcrFlameOnNotify(hWnd, uMsg, wp, lp);

    case WM_FLAME_QUERYNODE:
        return CcrFlameOnQueryNode(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CcrFlameOnNotify(
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
			return CcrFlameOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			break;
		}
	}


	return Status;
}

LRESULT
CcrFlameOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CcrFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0;
    return Status;
}

VOID
CcrFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
    PDIALOG_OBJECT Object;
    PCCR_FORM_CONTEXT Context;
    PCCR_FLAME_CONTEXT FlameContext;
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
    Context = (PCCR_FORM_CONTEXT)Object->Context;

    FlameContext = (PCCR_FLAME_CONTEXT)ApsMalloc(sizeof(CCR_FLAME_CONTEXT));
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
    // Create CCR call graph
    //
	
    ApsCreateCallGraph(&Graph, PROFILE_CCR_TYPE, Record, Count, PcTable, FuncTable, TextTable);
    FlameContext->Graph = Graph;
    Context->Context = FlameContext;

    //
    // Set the graph to flame control
    //

    CcrFlameSetGraph(Object, Graph);
}

VOID
CcrFlameSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    )
{
    HWND hWndCtrl;
    PFLAME_CONTROL Control;

    ASSERT(Graph != NULL);

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_CCR_FLAME);
    ASSERT(hWndCtrl != NULL);

    Control = (PFLAME_CONTROL)SdkGetObject(hWndCtrl);
    ASSERT(Control != NULL);

    Control->Graph = Graph;
    FlameBuildGraph(Control);
}

LRESULT
CcrFlameOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{ 
    PDIALOG_OBJECT Object;
    PCCR_FORM_CONTEXT Context;
    PCCR_FLAME_CONTEXT FlameContext;
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
    Context = (PCCR_FORM_CONTEXT)Object->Context;
    FlameContext = (PCCR_FLAME_CONTEXT)Context->Context;

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
        Percent = (Node->Ccr.Inclusive * 100.0) / Graph->Inclusive;
        StringCchPrintf(QueryNode->Text, MAX_PATH, L"%s!%s, %u samples, %.2f %%", 
                        Module, Symbol, Node->Ccr.Inclusive, Percent);
    }

	return TRUE;
}