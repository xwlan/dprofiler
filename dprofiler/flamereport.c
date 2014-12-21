//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"
#include "split.h"
#include "flamereport.h"
#include "calltree.h"
#include "flamegraph.h"

DIALOG_SCALER_CHILD FlameReportChildren[] = {
	{ IDC_FLAME_REPORT, AlignRight, AlignBottom },
	{ IDOK, AlignBoth, AlignBoth },
};

DIALOG_SCALER FlameReportScaler = {
	{0,0}, {0,0}, {0,0}, 2, FlameReportChildren
};

typedef struct _FLAME_REPORT_CONTEXT {
    CPU_FORM_CONTEXT Base;
    PBTR_STACK_RECORD Record;
    ULONG First;
    ULONG Last;
    PCALL_GRAPH Graph;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_LINE_ENTRY LineEntry;
    PBTR_TEXT_TABLE TextTable;
} FLAME_REPORT_CONTEXT, *PFLAME_REPORT_CONTEXT;

VOID
FlameReportCreate(
	__in HWND hWndParent,
    __in PPF_REPORT_HEAD Head,
    __in PBTR_STACK_RECORD Record,
    __in ULONG First,
    __in ULONG Last
	)
{
    DIALOG_OBJECT Object = {0};
    FLAME_REPORT_CONTEXT Context = {0};
	
    Context.Base.Head = Head;
    Context.First = First;
    Context.Last = Last;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_FLAME_REPORT;
    Object.Procedure = FlameReportProcedure;

    DialogCreate(&Object);
}

LRESULT
FlameReportOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFLAME_REPORT_CONTEXT Context;
	PDIALOG_OBJECT Object;
	HWND hWndCtrl;
    PFLAME_CONTROL Control;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FLAME_REPORT_CONTEXT);
	
    //
    // Initialize the flame control
    //

	hWndCtrl = GetDlgItem(hWnd, IDC_FLAME_REPORT);
    Control = FlameInitializeControl(hWndCtrl, 1, 0, 0, RGB(0xEE, 0xEE, 0xC7), 0);

	//
	// Position flame control to fill full client area
	//

    FlameSetSize(hWndCtrl, 1024, 1024, 1, 1, TRUE);
    FlameReportInsertData(hWnd, Context->Base.Head);

	//
	// Register dialog scaler
	//

	Object->Scaler = &FlameReportScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT 
FlameReportOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FlameReportOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FlameReportOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FlameReportOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PDIALOG_OBJECT Object;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);

    //
    // Clean up allocated resources
    //

	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
FlameReportOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	switch (LOWORD(wp)) {

		case IDOK:
		case IDCANCEL:
			return FlameReportOnOk(hWnd, uMsg, wp, lp);

	}

    return 0;
}

INT_PTR CALLBACK
FlameReportProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return FlameReportOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return FlameReportOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return FlameReportOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return FlameReportOnNotify(hWnd, uMsg, wp, lp);

    case WM_COMMAND:
		return FlameReportOnCommand(hWnd, uMsg, wp, lp);

    case WM_FLAME_QUERYNODE:
        return FlameReportOnQueryNode(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
FlameReportOnNotify(
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
			return FlameReportOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			break;
		}
	}


	return Status;
}

LRESULT
FlameReportOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FlameReportOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0;
    return Status;
}

VOID
FlameReportInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
    PDIALOG_OBJECT Object;
    PFLAME_REPORT_CONTEXT Context;
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
    Context = (PFLAME_REPORT_CONTEXT)Object->Context;
    Context->Base.Head = Head;
    Context->LineEntry = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE); 

    TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
    TextTable = ApsBuildSymbolTable(TextFile, 4093);
    Context->TextTable = TextTable;

    DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
    DllEntry = &DllFile->Dll[0];
    Context->DllEntry = DllEntry;

    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
    Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

    FuncTable = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);
    ApsCreatePcTableFromStream(Head, &PcTable);

    //
    // Create CPU call graph
    //
	
	//ThreadTable = CpuTreeCreateThreadedGraph();
    //Thread = ThreadTable->Thread[0];
    //ApsCreateCallGraphCpuPerThread(&Graph, Thread, Record, 
    //                               PcTable, FuncTable, TextTable);

    ApsCreateCallGraph(&Graph, PROFILE_CPU_TYPE, Record, Count, PcTable, FuncTable, TextTable);
    Context->Graph = Graph;

    //
    // Set the graph to flame control
    //

    FlameReportSetGraph(Object, Graph);
}

VOID
FlameReportSetGraph(
    __in PDIALOG_OBJECT Object,
    __in PCALL_GRAPH Graph
    )
{
    HWND hWndCtrl;
    PFLAME_CONTROL Control;

    ASSERT(Graph != NULL);

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_FLAME_REPORT);
    ASSERT(hWndCtrl != NULL);

    Control = (PFLAME_CONTROL)SdkGetObject(hWndCtrl);
    ASSERT(Control != NULL);

    Control->Graph = Graph;
    FlameBuildGraph(Control);
}

LRESULT
FlameReportOnQueryNode(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{ 
    PDIALOG_OBJECT Object;
    PFLAME_REPORT_CONTEXT Context;
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
    Context = (PFLAME_REPORT_CONTEXT)Object->Context;

    //
    // Lookup symbol name for specified call node
    //

    TextTable = Context->TextTable;
    TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Node->Address);
    if (!TextEntry) {
        StringCchPrintf(Symbol, MAX_PATH, L"0x%x", (PVOID)Node->Address);
    } else {
        StringCchPrintf(Symbol, MAX_PATH, L"%S", TextEntry->Text); 
    } 

    //
    // Lookup module name
    //

    DllEntry = Context->DllEntry;
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
        Graph = Context->Graph;
        Percent = (Node->Cpu.Inclusive * 100.0) / Graph->Inclusive;
        StringCchPrintf(QueryNode->Text, MAX_PATH, L"%s!%s, %u samples, %.2f %%", 
                        Module, Symbol, Node->Cpu.Inclusive, Percent);
    }

	return TRUE;
}