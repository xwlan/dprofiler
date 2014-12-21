//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "apsrpt.h"
#include "apspdb.h"
#include "sdk.h"
#include "treelist.h"
#include "dialog.h"
#include "profileform.h"
#include "split.h"
#include "mmtree.h"
#include "calltree.h"

typedef enum _MM_TREE_COLUMN_KEY {
	MmTreeNameKey,
	MmTreeInclusiveKey,
	MmTreeInclusivePercentKey,
	MmTreeExclusiveKey,
	MmTreeExclusivePercentKey,
	MmTreeModuleKey,
	MmTreeLineKey,
} MM_TREE_COLUMN_KEY;

DIALOG_SCALER_CHILD MmTreeChildren[1] = {
	{ IDC_TREELIST_MM_TREE, AlignRight, AlignBottom }
};

DIALOG_SCALER MmTreeScaler = {
	{0,0}, {0,0}, {0,0}, 1, MmTreeChildren
};

HEADER_COLUMN MmTreeColumn[] = {
	{ {0}, 120, HDF_LEFT,  FALSE, FALSE, L"Name" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Inclusive" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Inclusive %" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Exclusive" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Exclusive %" },
	{ {0}, 120, HDF_RIGHT, FALSE, FALSE, L"Module" },
	{ {0}, 240, HDF_LEFT, FALSE, FALSE, L"Line" },
};

#define MM_TREE_COLUMN_NUMBER  (sizeof(MmTreeColumn)/sizeof(HEADER_COLUMN))


HWND
MmTreeCreate(
	__in HWND hWndParent,
	__in UINT_PTR CtrlId
	)
{
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PMM_FORM_CONTEXT)SdkMalloc(sizeof(MM_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_MM_TREE;
	Object->Procedure = MmTreeProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
MmTreeOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PTREELIST_OBJECT TreeList;
	PMM_FORM_CONTEXT Context;
	PDIALOG_OBJECT Object;
	ULONG Number;
	RECT Rect;
	HWND hWndCtrl;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
	
	//
	// Create treelist control
	//
	
	GetClientRect(hWnd, &Rect);
	Rect.right = 10;
	Rect.bottom = 10;

	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST_MM_TREE);
	TreeList = TreeListCreate(hWnd, TRUE, IDC_TREELIST_MM_TREE, 
							  &Rect, hWnd, 
							  MmTreeFormatCallback, 
							  MM_TREE_COLUMN_NUMBER);

	//
	// We need explicitly bind the treelist object to its hwnd
	// if it's hosted in dialog as custom control.
	//

	TreeList->hWnd = hWndCtrl;
	SdkSetObject(hWndCtrl, TreeList);
	TreeListCreateControls(TreeList);

	for(Number = 0; Number < MM_TREE_COLUMN_NUMBER; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 MmTreeColumn[Number].Align, 
							 MmTreeColumn[Number].Width, 
							 MmTreeColumn[Number].Name);
	}

	ASSERT(TreeList->hWnd != NULL);
	Context->TreeList = TreeList;

	//
	// Position treelist to fill full client area
	//

	GetClientRect(hWnd, &Rect);
	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST_MM_TREE);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &MmTreeScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT 
MmTreeOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmTreeOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmTreeOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
MmTreeProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return MmTreeOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return MmTreeOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return MmTreeOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return MmTreeOnNotify(hWnd, uMsg, wp, lp);
	
	case WM_TREELIST_SORT:
		return MmTreeOnTreeListSort(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
MmTreeOnNotify(
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
			return MmTreeOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			break;
		}
	}


	return Status;
}

LRESULT
MmTreeOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmTreeOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0;
    return Status;
}

LRESULT
MmTreeOnTreeListSort(
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
MmTreeGetTree(
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
MmTreeFormatCallback(
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
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT Context;
    PMM_TREE_CONTEXT TreeContext;
	HWND hWnd;
    WCHAR BaseName[MAX_PATH];
    WCHAR ExtName[16];
    PCALL_GRAPH Graph;
    PCALL_NODE Node;
    PCALL_TREE Tree;

	hWnd = GetParent(TreeList->hWnd);
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, MM_FORM_CONTEXT);

	hWndTree = TreeList->hWndTree;
	TreeContext = (PMM_TREE_CONTEXT)TreeList->Context;
    DllEntry = TreeContext->DllEntry;
    TextTable = TreeContext->TextTable;
    Graph = TreeContext->Graph;

	hTreeParentItem = TreeView_GetParent(hWndTree, hTreeItem);

	if (hTreeParentItem != NULL) {

        Node = (PCALL_NODE)Value;
        Tree = MmTreeGetTree(hWndTree, hTreeItem); 

		switch (Column) {

            case MmTreeNameKey:
				TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Node->Address);
				if (!TextEntry) {
					StringCchPrintf(Buffer, Length, L"0x%x", (PVOID)Node->Address);
				} else {
                    StringCchPrintf(Buffer, Length, L"%S", TextEntry->Text); 
				}
				break;

            case MmTreeInclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Mm.InclusiveBytes);
				break;

            case MmTreeInclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", Node->Mm.InclusiveBytes * 100.0 / Graph->InclusiveBytes);
				break;

            case MmTreeExclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Mm.ExclusiveBytes);
				break;

			case MmTreeExclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", Node->Mm.ExclusiveBytes * 100.0 / Graph->ExclusiveBytes);
				break;

			case MmTreeModuleKey:
                if (Node->DllId != -1) {
                    _wsplitpath(DllEntry[Node->DllId].Path, NULL, NULL, BaseName, ExtName);
                    if (ExtName[0] != 0) {
                        StringCchPrintf(Buffer, Length, L"%s%s", BaseName, ExtName);
                    } else {
                        StringCchPrintf(Buffer, Length, L"%s", BaseName);
                    }
                }
				break;

            case MmTreeLineKey:
                break;
		}

	} else {

        PCALL_TREE CallTree;
		CallTree = (PCALL_TREE)Value;
		Node = CallTree->RootNode;

        Buffer[0] = 0;

		switch (Column) {

            case MmTreeNameKey:
				StringCchPrintf(Buffer, Length, L"Hot Path %d", 0);
				break;

            case MmTreeInclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Mm.InclusiveBytes);
				break;

            case MmTreeInclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", 
					            Node->Mm.InclusiveBytes * 100.0 / Graph->InclusiveBytes);
				break;

            case MmTreeExclusiveKey:
				StringCchPrintf(Buffer, Length, L"%u", Node->Mm.ExclusiveBytes);
				break;

            case MmTreeExclusivePercentKey:
				StringCchPrintf(Buffer, Length, L"%.2f", 
					            Node->Mm.ExclusiveBytes * 100.0 / Graph->ExclusiveBytes);
				break;

			case MmTreeModuleKey:
                break;

            case MmTreeLineKey:
                break;
		}
	}
}

VOID
MmTreeInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
    PMM_TREE_CONTEXT TreeContext;
    PDIALOG_OBJECT Object;
    PMM_FORM_CONTEXT Context;
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
     
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PMM_FORM_CONTEXT)Object->Context;
    TreeList = Context->TreeList;

    TreeContext = (PMM_TREE_CONTEXT)ApsMalloc(sizeof(MM_TREE_CONTEXT));
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
    // Create MM call graph
    //

    ApsCreateCallGraph(&Graph, PROFILE_MM_TYPE, Record, Count, PcTable, FuncTable, TextTable);
    TreeContext->Graph = Graph;

    TreeList->Context = TreeContext;
    MmTreeInsertTree(TreeList->hWndTree, Head, Graph, TreeList);
}

VOID
MmTreeInsertTree(
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
	PMM_TREE_CONTEXT TreeContext = NULL;

	TreeContext = (PMM_TREE_CONTEXT)TreeList->Context;
	ASSERT(TreeContext != NULL);

    Buffer = (PWSTR)ApsMalloc(sizeof(WCHAR) * MAX_PATH);

    Number = 0;
    ListHead = &Graph->TreeListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {

        CallTree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);

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

        MmTreeInsertNode(hWndTree, hItemCallTree, CallTree->RootNode, TreeContext);

        //
        // Move to next call tree
        //

        ListEntry = ListEntry->Flink;
        Number += 1;
    }

    ApsFree(Buffer);
}

HTREEITEM
MmTreeInsertNode(
    __in HWND hWndTree,
    __in HTREEITEM ParentItem,
    __in PCALL_NODE Node,
	__in PMM_TREE_CONTEXT Context
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
        MmTreeInsertNode(hWndTree, hItemCallNode, Child, Context);
        ListEntry = ListEntry->Flink;
    }

    return hItemCallNode;
}

VOID
MmTreeOnDeduction(
	__in HWND hWnd 
	)
{
}