//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"
#include "split.h"
#include "cputree.h"
#include "calltree.h"
#include "frame.h"
#include "main.h"
#include "cputhread.h"

typedef enum _CPU_TREE_COLUMN_KEY {
	CpuTreeNameKey,
	CpuTreeInclusiveKey,
	CpuTreeInclusivePercentKey,
	CpuTreeExclusiveKey,
	CpuTreeExclusivePercentKey,
	CpuTreeModuleKey,
	CpuTreeLineKey,
} CPU_TREE_COLUMN_KEY;

DIALOG_SCALER_CHILD CpuTreeChildren[1] = {
	{ IDC_TREELIST_CPU_TREE, AlignRight, AlignBottom }
};

DIALOG_SCALER CpuTreeScaler = {
	{0,0}, {0,0}, {0,0}, 1, CpuTreeChildren
};

HEADER_COLUMN CpuTreeColumn[] = {
	{ {0}, 120, HDF_LEFT,  FALSE, FALSE, L"Name" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Inclusive" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Inclusive %" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Exclusive" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Exclusive %" },
	{ {0}, 120, HDF_RIGHT, FALSE, FALSE, L"Module" },
	{ {0}, 240, HDF_LEFT,  FALSE, TRUE,  L"Line" },
};

#define CPU_TREE_COLUMN_NUMBER  (sizeof(CpuTreeColumn)/sizeof(HEADER_COLUMN))

HWND
CpuTreeCreate(
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
	Object->ResourceId = IDD_FORMVIEW_CPU_TREE;
	Object->Procedure = CpuTreeProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CpuTreeOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PTREELIST_OBJECT TreeList;
	PCPU_FORM_CONTEXT Context;
	PDIALOG_OBJECT Object;
	ULONG Number;
	RECT Rect;
	HWND hWndCtrl;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	
	//
	// Create treelist control
	//
	
	GetClientRect(hWnd, &Rect);
	Rect.right = 10;
	Rect.bottom = 10;

	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST_CPU_TREE);
	TreeList = TreeListCreate(hWnd, TRUE, IDC_TREELIST_CPU_TREE, 
							  &Rect, hWnd, 
							  CpuTreeFormatCallback, 
							  CPU_TREE_COLUMN_NUMBER);

	//
	// We need explicitly bind the treelist object to its hwnd
	// if it's hosted in dialog as custom control.
	//

	TreeList->hWnd = hWndCtrl;
	SdkSetObject(hWndCtrl, TreeList);
	TreeListCreateControls(TreeList);

	for(Number = 0; Number < CPU_TREE_COLUMN_NUMBER; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 CpuTreeColumn[Number].Align, 
							 CpuTreeColumn[Number].Width, 
							 CpuTreeColumn[Number].Name);
	}

	ASSERT(TreeList->hWnd != NULL);
	Context->TreeList = TreeList;

	//
	// Position treelist to fill full client area
	//

	GetClientRect(hWnd, &Rect);
	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST_CPU_TREE);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);

	//
	// Register dialog scaler
	//

	CpuTreeScaler.MinClient.cx = Rect.right - Rect.left;
	CpuTreeScaler.MinClient.cy = Rect.bottom - Rect.top;
	Object->Scaler = &CpuTreeScaler;

	DialogRegisterScaler(Object);

	//
	// Subclass treelist's default procedure, this is to handle
	// NM_DBLCLK notification
	//

	SetWindowSubclass(TreeList->hWnd, CpuTreeListProcedure, 0, (DWORD_PTR)Object);
	return TRUE;
}

LRESULT CALLBACK 
CpuTreeListProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	return DefSubclassProc(hWnd, uMsg, wp, lp);
}

LRESULT 
CpuTreeOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuTreeOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HWND hWndCtrl;
	RECT Rect;

	GetClientRect(hWnd, &Rect);
	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST_CPU_TREE);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);
	return 0;
}

LRESULT
CpuTreeOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
CpuTreeProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuTreeOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuTreeOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return CpuTreeOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuTreeOnNotify(hWnd, uMsg, wp, lp);
	
	case WM_TREELIST_SORT:
		return CpuTreeOnTreeListSort(hWnd, uMsg, wp, lp);

	case WM_TREELIST_DBLCLK:
		return CpuTreeOnDblclk(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuTreeOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuTreeOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuTreeOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
    return 0;
}

LRESULT
CpuTreeOnDblclk(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	LRESULT Status = 0;
	TVITEM tvi = {0};
	HTREEITEM hTreeItem = (HTREEITEM)wp;
	HTREEITEM hTreeParentItem;
	int Column = (int)lp;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
	PCPU_TREE_CONTEXT TreeContext;
	PCALL_NODE Node;
	PBTR_LINE_ENTRY LineEntry;
	WCHAR Buffer[MAX_PATH];

	//
	// If it's not source column, return
	//

	if (lp != 6) {
		return 0;
	}

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCPU_FORM_CONTEXT)Object->Context;
	TreeList = Context->TreeList;

	//
	// If it's root item, return
	//

	hTreeParentItem = TreeView_GetParent(TreeList->hWndTree, hTreeItem);
	if (!hTreeParentItem) {
		return 0;
	}

	tvi.mask = TVIF_PARAM;
	tvi.hItem = hTreeItem;
	TreeView_GetItem(TreeList->hWndTree, &tvi);

	Node = (PCALL_NODE)tvi.lParam;
	ASSERT(Node != NULL);

	//
	// Query and format line information
	//

	TreeContext = (PCPU_TREE_CONTEXT)TreeList->Context;
	LineEntry = TreeContext->LineEntry;

	if (LineEntry != NULL && Node->LineId != -1) {
		LineEntry = LineEntry + Node->LineId;

		//
		// prefix space to padding the left edge
		//

		StringCchPrintf(Buffer, MAX_PATH, L"%S", LineEntry->File);
		FrameShowSource(hWnd, Buffer, LineEntry->Line);
	}

    return 0;
}

LRESULT
CpuTreeOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

//
// If the each node remember its tree pointer, this 
// routine can be dropped
//

PCALL_TREE
CpuTreeGetTree(
    __in HWND hWndTree,
    __in HTREEITEM hTreeItem
    )
{
    HTREEITEM hItemParent;
    TVITEM Item = {0};

    while (hItemParent = TreeView_GetParent(hWndTree, hTreeItem)) {
        hTreeItem = hItemParent;
    }
    
    Item.mask = TVIF_PARAM;
    Item.hItem = hTreeItem;
    TreeView_GetItem(hWndTree, &Item);

    return (PCALL_TREE)Item.lParam;
}

VOID CALLBACK
CpuTreeFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	)
{
	HWND hWndTree;
	HTREEITEM hTreeParentItem;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_ENTRY TextEntry;
	PBTR_LINE_ENTRY LineEntry;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
    PCPU_TREE_CONTEXT TreeContext;
	HWND hWnd;
    WCHAR BaseName[MAX_PATH];
    WCHAR ExtName[16];
    PCALL_GRAPH Graph;
    PCALL_NODE Node;
    PCALL_TREE Tree;

	hWnd = GetParent(TreeList->hWnd);
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	hWndTree = TreeList->hWndTree;
	TreeContext = (PCPU_TREE_CONTEXT)TreeList->Context;
    DllEntry = TreeContext->DllEntry;
    TextTable = TreeContext->TextTable;
	LineEntry = TreeContext->LineEntry;
    Graph = TreeContext->Graph;

	hTreeParentItem = TreeView_GetParent(hWndTree, hTreeItem);

	if (hTreeParentItem != NULL) {

        Node = (PCALL_NODE)Value;
        Tree = CpuTreeGetTree(hWndTree, hTreeItem); 

		switch (Column) {

            case CpuTreeNameKey:
				TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Node->Address);
				if (!TextEntry) {
					StringCchPrintf(Buffer, Length, L"0x%x", (PVOID)Node->Address);
				} else {
                    StringCchPrintf(Buffer, Length, L"%S", TextEntry->Text); 
				}
				break;

            case CpuTreeInclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Cpu.Inclusive);
				break;

            case CpuTreeInclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", Node->Cpu.Inclusive * 100.0 / Graph->Inclusive);
				break;

            case CpuTreeExclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Cpu.Exclusive);
				break;

			case CpuTreeExclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", Node->Cpu.Exclusive * 100.0 / Graph->Exclusive);
				break;

			case CpuTreeModuleKey:
                if (Node->DllId != -1) {
                    _wsplitpath(DllEntry[Node->DllId].Path, NULL, NULL, BaseName, ExtName);
                    if (ExtName[0] != 0) {
                        StringCchPrintf(Buffer, Length, L"%s%s", BaseName, ExtName);
                    } else {
                        StringCchPrintf(Buffer, Length, L"%s", BaseName);
                    }
                }
				break;

			case CpuTreeLineKey:

				if (LineEntry != NULL && Node->LineId != -1) {
					LineEntry = LineEntry + Node->LineId;

					//
					// prefix space to padding the left edge
					//

					StringCchPrintf(Buffer, MAX_PATH, L"    %S:%u", LineEntry->File, LineEntry->Line);
				}
		}

	} else {

        PCALL_TREE CallTree;
		CallTree = (PCALL_TREE)Value;
		Node = CallTree->RootNode;

        Buffer[0] = 0;

		switch (Column) {

            case CpuTreeNameKey:
				StringCchPrintf(Buffer, Length, L"Hot Path %d", CallTree->Number);
				break;

            case CpuTreeInclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Cpu.Inclusive);
				break;

            case CpuTreeInclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", 
					            Node->Cpu.Inclusive * 100.0 / Graph->Inclusive);
				break;

            case CpuTreeExclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Cpu.Exclusive);
				break;

            case CpuTreeExclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", 
					            Node->Cpu.Exclusive * 100.0 / Graph->Exclusive);
				break;

			case CpuTreeModuleKey:
                break;

            case CpuTreeLineKey:
                break;
		}
	}
}

VOID
CpuTreeInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
    PCPU_TREE_CONTEXT TreeContext;
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;
    PTREELIST_OBJECT TreeList;
    PBTR_DLL_FILE DllFile;
    PBTR_DLL_ENTRY DllEntry;
    PBTR_TEXT_TABLE TextTable;
    PBTR_TEXT_FILE TextFile;
    PBTR_FUNCTION_ENTRY FuncTable;
    PBTR_PC_TABLE PcTable;
    PBTR_STACK_RECORD Record;
    ULONG Count;
    PCALL_GRAPH Graph;
    PCPU_THREAD Thread;
    PCPU_THREAD_TABLE ThreadTable;
     
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_FORM_CONTEXT)Object->Context;
    TreeList = Context->TreeList;

    TreeContext = (PCPU_TREE_CONTEXT)ApsMalloc(sizeof(CPU_TREE_CONTEXT));
    TreeContext->Head = Head;
    TreeContext->LineEntry = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE); 

    TextFile = (PBTR_TEXT_FILE)ApsGetStreamPointer(Head, STREAM_SYMBOL);
    TextTable = ApsBuildSymbolTable(TextFile, 4093);
    TreeContext->TextTable = TextTable;

    DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
    DllEntry = &DllFile->Dll[0];
    TreeContext->DllEntry = DllEntry;

    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
    Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);

    FuncTable = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);
    ApsCreatePcTableFromStream(Head, &PcTable);

    //
    // Create CPU call graph
    //
    
    ApsCreateCallGraph(&Graph, PROFILE_CPU_TYPE, Record, Count, PcTable, FuncTable, TextTable);
    /*ThreadTable = CpuTreeCreateThreadedGraph();
    Thread = ThreadTable->Thread[0];
    ApsCreateCallGraphCpuPerThread(&Graph, Thread, Record, 
                                   PcTable, FuncTable, TextTable);
	Thread->Graph = Graph;*/
    TreeContext->Graph = Graph;

    TreeList->Context = TreeContext;
    CpuTreeInsertTree(TreeList->hWndTree, Head, Graph, TreeList);
}

VOID
CpuTreeInsertTree(
    __in HWND hWndTree,
    __in PPF_REPORT_HEAD Head,
    __in PCALL_GRAPH Graph,
	__in PTREELIST_OBJECT TreeList
    )
{
    TVINSERTSTRUCT tvi = {0};
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCALL_TREE CallTree;
    PWSTR Buffer;
    HTREEITEM hItemCallTree;
    LONG Number;
	PCPU_TREE_CONTEXT TreeContext = NULL;

	TreeContext = (PCPU_TREE_CONTEXT)TreeList->Context;
	ASSERT(TreeContext != NULL);

    Buffer = (PWSTR)ApsMalloc(sizeof(WCHAR) * MAX_PATH);

    Number = 0;
    ListHead = &Graph->TreeListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {

        CallTree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);
		CallTree->Number = Number;

		if (!ApsIsCallTreeAboveThreshold(Graph, CallTree)) {
	        ListEntry = ListEntry->Flink;
			continue;
		}

        tvi.hParent = TVI_ROOT; 
        tvi.hInsertAfter = TVI_LAST; 
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 

        StringCchPrintf(Buffer, MAX_PATH, L"Hot Path %d", Number);
        tvi.item.pszText = Buffer;
        tvi.item.lParam = (LPARAM)CallTree; 

        hItemCallTree = TreeView_InsertItem(hWndTree, &tvi);

        //
        // Recusively insert tree nodes
        //

        CpuTreeInsertNode(hWndTree, hItemCallTree, CallTree->RootNode, TreeContext);

        //
        // Move to next call tree
        //

        ListEntry = ListEntry->Flink;
        Number += 1;
    }

    ApsFree(Buffer);
}

HTREEITEM
CpuTreeInsertNode(
    __in HWND hWndTree,
    __in HTREEITEM ParentItem,
    __in PCALL_NODE Node,
	__in PCPU_TREE_CONTEXT Context
    )
{ 
    HTREEITEM hItemCallNode;
    TVINSERTSTRUCT tvi = {0};
    PCALL_NODE Child;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
	PWCHAR Buffer;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_ENTRY TextEntry;

	TextTable = Context->TextTable;
	Buffer = (PWCHAR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
	TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Node->Address);
	if (!TextEntry) {
		StringCchPrintf(Buffer, MAX_PATH, L"0x%x", (PVOID)Node->Address);
	} else {
        StringCchPrintf(Buffer, MAX_PATH, L"%S", TextEntry->Text); 
	}

    tvi.hParent = ParentItem; 
    tvi.hInsertAfter = TVI_LAST; 
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
    tvi.item.pszText = Buffer;
    tvi.item.lParam = (LPARAM)Node; 

    hItemCallNode = TreeView_InsertItem(hWndTree, &tvi);
	ApsFree(Buffer);

    ListHead = &Node->ChildListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {
        Child = CONTAINING_RECORD(ListEntry, CALL_NODE, ListEntry);
		
		if (!ApsIsCallNodeAboveThreshold(Context->Graph, Child)) {
	        ListEntry = ListEntry->Flink;
			continue;
		}

        CpuTreeInsertNode(hWndTree, hItemCallNode, Child, Context);
        ListEntry = ListEntry->Flink;
    }

    return hItemCallNode;
}

VOID
CpuTreeOnDeduction(
	__in HWND hWnd 
	)
{ 
	PCPU_TREE_CONTEXT TreeContext;
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;
    PTREELIST_OBJECT TreeList;
     
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_FORM_CONTEXT)Object->Context;
    TreeList = Context->TreeList;
    TreeContext = (PCPU_TREE_CONTEXT)TreeList->Context;

	//
	// Remove all nodes
	//

	TreeView_DeleteAllItems(TreeList->hWndTree);

	//
	// Insert nodes filtered by deduction threshold
	//

    CpuTreeInsertTree(TreeList->hWndTree, TreeContext->Head, 
					  TreeContext->Graph, TreeList);
}

PCPU_THREAD_TABLE
CpuTreeCreateThreadedGraph(
    VOID
    )
{
    PFRAME_OBJECT Frame;
    HWND hWndForm;
    PCPU_THREAD_TABLE Table;

    Frame = MainGetFrame();

    //
    // N.B. We rely on thread form to create per thread data,
    // just hide it
    //

    hWndForm = Frame->CpuForm.hWndForm[CPU_FORM_THREAD];

    if (!hWndForm) {
        hWndForm = CpuThreadCreate(Frame->hWnd, CPU_FORM_THREAD);
        Frame->CpuForm.hWndForm[CPU_FORM_THREAD] = hWndForm;
        CpuThreadInsertData(hWndForm, Frame->Head);
    }
    ShowWindow(hWndForm, SW_HIDE);

    Table = CpuThreadGetTable(hWndForm);
    return Table;
}