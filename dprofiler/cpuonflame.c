//
// xwlan@outlook.com
// Copyright(C) 2025
//

#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"
#include "split.h"
#include "cpuonflame.h"
#include "calltree.h"
#include "flamegraph.h"

DIALOG_SCALER_CHILD CpuOnFlameChildren[1] = {
	{ IDC_CPU_ONFLAME, AlignRight, AlignBottom }
};

DIALOG_SCALER CpuOnFlameScaler = {
	{0,0}, {0,0}, {0,0}, 1, CpuOnFlameChildren
};

typedef struct _CPU_ONFLAME_CONTEXT {
	PPF_REPORT_HEAD Head;
	PCALL_GRAPH Graph;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_TEXT_TABLE TextTable;
} CPU_ONFLAME_CONTEXT, * PCPU_ONFLAME_CONTEXT;


HWND
CpuOnFlameCreate(
	__in HWND hWndParent,
	__in UINT_PTR CtrlId
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
	Object->ResourceId = IDD_FORMVIEW_CPU_ONFLAME;
	Object->Procedure = CpuOnFlameProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CpuOnFlameOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PCPU_FORM_CONTEXT Context;
	PDIALOG_OBJECT Object;
	RECT Rect;
	HWND hWndCtrl;
	PFLAME_CONTROL Control;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	//
	// Initialize the flame control
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_CPU_ONFLAME);
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

	Object->Scaler = &CpuOnFlameScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT
CpuOnFlameOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuOnFlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuOnFlameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
CpuOnFlameProcedure(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuOnFlameOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuOnFlameOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return CpuOnFlameOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuOnFlameOnNotify(hWnd, uMsg, wp, lp);

	case WM_FLAME_QUERYNODE:
		return CpuOnFlameOnQueryNode(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuOnFlameOnCustomDraw(
	__in PDIALOG_OBJECT Object,
	__in LPNMHDR lpnmhdr
	)
{
	return 0;
}

LRESULT
CpuOnFlameOnDrawItem(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuOnFlameOnQueryNode(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	PCPU_ONFLAME_CONTEXT FlameContext;
	PNM_FLAME_QUERYNODE QueryNode;

	QueryNode = (PNM_FLAME_QUERYNODE)lp;
	if (!QueryNode) {
		return FALSE;
	}

	ASSERT(FlagOn(QueryNode->Flags, FLAME_QUERY_SYMBOL));

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCPU_FORM_CONTEXT)Object->Context;
	FlameContext = (PCPU_ONFLAME_CONTEXT)Context->Context;

	FlameQueryNodeFormatTooltip(QueryNode,
		FlameContext->DllEntry,
		FlameContext->TextTable,
		FlameContext->Graph);
	return TRUE;
}

LRESULT
CpuOnFlameOnNotify(
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

	if (IDC_TREELIST == pNmhdr->idFrom) {

		switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return CpuOnFlameOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			break;
		}
	}


	return Status;
}

VOID
CpuOnFlameInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	PCPU_ONFLAME_CONTEXT FlameContext;
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
	Context = (PCPU_FORM_CONTEXT)Object->Context;

	FlameContext = (PCPU_ONFLAME_CONTEXT)ApsMalloc(sizeof(CPU_ONFLAME_CONTEXT));
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
	// Create CPU call graph
	//

	//ThreadTable = CpuTreeCreateThreadedGraph();
	//Thread = ThreadTable->Thread[0];
	//ApsCreateCallGraphCpuPerThread(&Graph, Thread, Record, 
	//                               PcTable, FuncTable, TextTable);

	ApsCreateCallGraphTopdown(&Graph, PROFILE_CPU_TYPE, Record, Count, PcTable, FuncTable, TextTable);
	FlameContext->Graph = Graph;
	Context->Context = FlameContext;

	//
	// Set the graph to flame control
	//

	CpuOnFlameSetGraph(Object, Graph);
}

VOID
CpuOnFlameSetGraph(
	__in PDIALOG_OBJECT Object,
	__in PCALL_GRAPH Graph
	)
{
	HWND hWndCtrl;
	PFLAME_CONTROL Control;

	ASSERT(Graph != NULL);

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_CPU_ONFLAME);
	ASSERT(hWndCtrl != NULL);

	Control = (PFLAME_CONTROL)SdkGetObject(hWndCtrl);
	ASSERT(Control != NULL);

	Control->Graph = Graph;
	Control->Mode = FLAME_MODE_TOPDOWN;
	FlameBuildGraph(Control);
}


