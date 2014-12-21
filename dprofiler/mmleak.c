//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "mmleak.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"
#include "sdk.h"
#include "split.h"
#include "frame.h"

typedef enum _MmLeakNodeType {
	LEAK_NODE_INVALID = 0,
	LEAK_NODE_HEAP_ROOT,
	LEAK_NODE_HEAP_NONE,
	LEAK_NODE_PAGE_ROOT,
	LEAK_NODE_PAGE_NONE,
	LEAK_NODE_HANDLE_ROOT,
	LEAK_NODE_HANDLE_NONE,
	LEAK_NODE_GDI_ROOT,
	LEAK_NODE_GDI_NONE,
	LEAK_NODE_HEAP_FILE,
	LEAK_NODE_HEAP_ENTRY,
	LEAK_NODE_PAGE_FILE,
	LEAK_NODE_PAGE_ENTRY,
	LEAK_NODE_HANDLE_FILE,
	LEAK_NODE_HANDLE_ENTRY,
	LEAK_NODE_GDI_FILE,
	LEAK_NODE_GDI_ENTRY,
	LEAK_NODE_BACKTRACE,
} MmLeakNodeType;

typedef enum _MmLeakColumnType {
	MmLeakName,
	MmLeakCount,
	MmLeakSize,
	MmLeakModule,
	MmLeakColumnCount,
} MmLeakColumnType;

HEADER_COLUMN 
MmLeakColumn[MmLeakColumnCount] = {
	{ {0}, 100,  HDF_LEFT, FALSE, FALSE,  L" Name" },
	{ {0}, 160,  HDF_RIGHT, FALSE, FALSE, L" Count" },
	{ {0}, 160,  HDF_RIGHT, FALSE, FALSE, L" Bytes" },
	{ {0}, 120,  HDF_RIGHT, FALSE, FALSE, L" Module" },
};

DIALOG_SCALER_CHILD MmLeakChildren[3] = {
	{ IDC_TREELIST, AlignNone, AlignBottom},
	{ IDC_SPLIT, AlignNone, AlignBottom },
	{ IDC_LIST_BACKTRACE, AlignRight, AlignBottom},
};

DIALOG_SCALER MmLeakScaler = {
	{0,0}, {0,0}, {0,0}, 3, MmLeakChildren
};

//
// Back trace list column
//

LISTVIEW_COLUMN BackTraceColumn[] = {
	{ 20,  L"#", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Call Site", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Module", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Line", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

//
// Vertical splitter layout
//

SPLIT_INFO MmLeakSplitInfo[2] = {
	{ IDC_TREELIST, SPLIT_RIGHT },
	{ IDC_LIST_BACKTRACE, SPLIT_LEFT }
};

//
// Vertical split object
//

SPLIT_OBJECT MmLeakSplitObject = {
	TRUE, MmLeakSplitInfo, 2, 20 
};

HWND
MmLeakCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	)
{
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT Context;
	
	Context = (PMM_FORM_CONTEXT)SdkMalloc(sizeof(MM_FORM_CONTEXT));
	Context->CtrlId = Id;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_MM_LEAK;
	Object->Procedure = MmLeakProcedure;

	return DialogCreateModeless(Object);
}

LRESULT
MmLeakOnInitDialog(
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
	LVCOLUMN lvc = {0};
	RECT CtrlRect;
	int i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
	
	//
	// Create treelist control
	//

	GetClientRect(hWnd, &Rect);
	Rect.right = 10;
	Rect.bottom = 10;

	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST);
	TreeList = TreeListCreate(hWnd, TRUE, IDC_TREELIST, 
							  &Rect, hWnd, 
							  MmLeakFormatCallback, 
							  MmLeakColumnCount);
	//
	// We need explicitly bind the treelist object to its hwnd
	// if it's hosted in dialog as custom control.
	//

	TreeList->hWnd = hWndCtrl;
	SdkSetObject(hWndCtrl, TreeList);
	TreeListCreateControls(TreeList);

	for(Number = 0; Number < MmLeakColumnCount; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 MmLeakColumn[Number].Align, 
							 MmLeakColumn[Number].Width, 
							 MmLeakColumn[Number].Name );
	}

	ASSERT(TreeList->hWnd != NULL);
	Context->TreeList = TreeList;

	//
	// Initialize splitbar
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_SPLIT);
	SplitSetObject(hWndCtrl, &MmLeakSplitObject);

	//
	// Initialize listview
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_BACKTRACE);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	for (i = 0; i < 4; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = BackTraceColumn[i].Title;	
		lvc.cx = BackTraceColumn[i].Width;     
		lvc.fmt = BackTraceColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Position controls 
	//

	GetClientRect(hWnd, &Rect);

	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST);

	CtrlRect.top = 0;
	CtrlRect.left = 0;
    CtrlRect.right = (Rect.right - Rect.left) / 2 - 2;
	CtrlRect.bottom = Rect.bottom;

	MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
		       CtrlRect.right - CtrlRect.left, 
			   CtrlRect.bottom - CtrlRect.top, TRUE);

	hWndCtrl = GetDlgItem(hWnd, IDC_SPLIT);

	CtrlRect.top = 0;
	CtrlRect.left = CtrlRect.right;
	CtrlRect.right = CtrlRect.left + 2;
	CtrlRect.bottom = Rect.bottom;

	MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
		       CtrlRect.right - CtrlRect.left, 
			   CtrlRect.bottom - CtrlRect.top, TRUE);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_BACKTRACE);

	CtrlRect.top = 0;
	CtrlRect.left = CtrlRect.right;
	CtrlRect.right = Rect.right;
	CtrlRect.bottom = Rect.bottom;

	MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
		       CtrlRect.right - CtrlRect.left, 
			   CtrlRect.bottom - CtrlRect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &MmLeakScaler;
	DialogRegisterScaler(Object);
	
	return TRUE;
}

VOID
MmLeakInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	)
{
	PTREELIST_OBJECT TreeList;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItemRoot;
	HWND hWndTree;
	ULONG Count;
	PBTR_LEAK_CONTEXT Context;
	RECT Rect;
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT FormContext;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	FormContext = SdkGetContext(Object, MM_FORM_CONTEXT);
	FormContext->Head = Report;

	TreeList = FormContext->TreeList;
	ASSERT(TreeList != NULL);

	//
	// Attach report head to treelist
	//

	TreeList->Context = Report;
	
	hWndTree = TreeList->hWndTree;
	hItemRoot = NULL;
	Count = 0;

	if (ApsGetStreamLength(Report, STREAM_LEAK_HEAP) != 0) {

		//
		// Insert heap data
		//

		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_HEAP_ROOT;
		Context->Context = Report;

		tvi.hParent = TVI_ROOT; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"Heap";
		tvi.item.lParam = (LPARAM)Context; 
		hItemRoot = TreeView_InsertItem(hWndTree, &tvi);

		MmLeakInsertHeapData(Report, TreeList, hItemRoot);

	}

	if (ApsGetStreamLength(Report, STREAM_LEAK_PAGE) != 0) {

		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_PAGE_ROOT;
		Context->Context = Report;

		tvi.hParent = TVI_ROOT; 
		tvi.hInsertAfter = hItemRoot; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"Page";
		tvi.item.lParam = (LPARAM)Context; 
		hItemRoot = TreeView_InsertItem(hWndTree, &tvi);

		MmLeakInsertPageData(Report, TreeList, hItemRoot);

	}

	if (ApsGetStreamLength(Report, STREAM_LEAK_HANDLE) != 0) {

		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_HANDLE_ROOT;
		Context->Context = Report;

		tvi.hParent = TVI_ROOT; 
		tvi.hInsertAfter = hItemRoot; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"Handle";
		tvi.item.lParam = (LPARAM)Context; 
		hItemRoot = TreeView_InsertItem(hWndTree, &tvi);

		MmLeakInsertHandleData(Report, TreeList, hItemRoot);

	}

	if (ApsGetStreamLength(Report, STREAM_LEAK_GDI) != 0) {

		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_GDI_ROOT;
		Context->Context = Report;

		tvi.hParent = TVI_ROOT; 
		tvi.hInsertAfter = hItemRoot; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"GDI";
		tvi.item.lParam = (LPARAM)Context; 
		hItemRoot = TreeView_InsertItem(hWndTree, &tvi);

		MmLeakInsertGdiData(Report, TreeList, hItemRoot);

	}

	//
	// Force a treelist sort by Count column
	//

	MmLeakOnTreeListSort(hWnd, WM_TREELIST_SORT, (WPARAM)MmLeakCount, 0);

	//
	// Update treelist client area
	//

	GetClientRect(hWnd, &Rect);
	InvalidateRect(hWnd, &Rect, TRUE);
	UpdateWindow(hWnd);
}

HTREEITEM
MmLeakInsertPerHeapData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem,
	__in PBTR_LEAK_FILE File
	)
{
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItemHeap;
	HTREEITEM hItemEntry;
	HWND hWndTree;
	WCHAR Buffer[MAX_PATH];
	ULONG Number;
	PBTR_LEAK_ENTRY Entry;

	//
	// Insert heap item
	//

	hWndTree = TreeList->hWndTree;

#if defined(_M_IX86)
	StringCchPrintf(Buffer, MAX_PATH, L"Heap Handle 0x%08x", (ULONG_PTR)File->Context);
#endif

#if defined(_M_X64)
	StringCchPrintf(Buffer, MAX_PATH, L"Heap Handle 0x%0I64x", (ULONG_PTR)File->Context);
#endif

	File->Auxiliary.Type = LEAK_NODE_HEAP_FILE;
	File->Auxiliary.Context = NULL;

	tvi.hParent = hTreeItem; 
	tvi.hInsertAfter = TVI_LAST; 
	tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
	tvi.item.pszText = Buffer;
	tvi.item.lParam = (LPARAM)File; 
	hItemHeap = TreeView_InsertItem(hWndTree, &tvi);

	for(Number = 0; Number < File->Count; Number += 1) {

		Entry = &File->Leak[Number];
		Entry->Auxiliary.Type = LEAK_NODE_HEAP_ENTRY;
		Entry->Auxiliary.Context = File;

		StringCchPrintf(Buffer, MAX_PATH, L"Back Trace ID %u", Entry->StackId);

		tvi.hParent = hItemHeap; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = Buffer;
		tvi.item.lParam = (LPARAM)Entry; 
		hItemEntry = TreeView_InsertItem(hWndTree, &tvi);	

	}

	return hItemHeap;
}

VOID
MmLeakInsertHeapData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	)
{
	PPF_STREAM_HEAP_LEAK HeapList;
	PBTR_LEAK_FILE HeapFile;
	ULONG Number;

	HeapList = (PPF_STREAM_HEAP_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_HEAP);
	for(Number = 0; Number < HeapList->NumberOfHeaps; Number += 1) {
		HeapFile = (PBTR_LEAK_FILE)((PUCHAR)Report + HeapList->Info[Number].Offset);	
		MmLeakInsertPerHeapData(Report, TreeList, hTreeItem, HeapFile);
	}
}

VOID
MmLeakInsertPageData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	)
{
	PBTR_LEAK_FILE File;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItemEntry;
	HWND hWndTree;
	ULONG i;
	PBTR_LEAK_ENTRY Entry;
	WCHAR Buffer[MAX_PATH];

	hWndTree = TreeList->hWndTree;

	File = (PBTR_LEAK_FILE)ApsGetStreamPointer(Report, STREAM_LEAK_PAGE);
	if (!File || File->Count == 0) {

		PBTR_LEAK_CONTEXT Context;
		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_PAGE_NONE;

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"None";
		tvi.item.lParam = (LPARAM)Context; 
		TreeView_InsertItem(hWndTree, &tvi);
		return;
	}

	File->Auxiliary.Type = LEAK_NODE_PAGE_FILE;
	File->Auxiliary.Context = NULL;

	for(i = 0; i < File->Count; i++) {

		Entry = &File->Leak[i];
		Entry->Auxiliary.Type = LEAK_NODE_PAGE_ENTRY;
		Entry->Auxiliary.Context = File;

		StringCchPrintf(Buffer, MAX_PATH, L"Back Trace ID %u", Entry->StackId);

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = Buffer;
		tvi.item.lParam = (LPARAM)Entry; 
		hItemEntry = TreeView_InsertItem(hWndTree, &tvi);	

	}
}

VOID
MmLeakInsertHandleData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	)
{
	PBTR_LEAK_FILE File;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItemEntry;
	HWND hWndTree;
	ULONG i;
	PBTR_LEAK_ENTRY Entry;
	WCHAR Buffer[MAX_PATH];

	hWndTree = TreeList->hWndTree;

	File = (PBTR_LEAK_FILE)ApsGetStreamPointer(Report, STREAM_LEAK_HANDLE);
	if (!File || File->Count == 0) {

		PBTR_LEAK_CONTEXT Context;
		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_HANDLE_NONE;

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"None";
		tvi.item.lParam = (LPARAM)Context; 
		TreeView_InsertItem(hWndTree, &tvi);
		return;
	}

	File->Auxiliary.Type = LEAK_NODE_HANDLE_FILE;
	File->Auxiliary.Context = NULL;

	for(i = 0; i < File->Count; i++) {

		Entry = &File->Leak[i];
		Entry->Auxiliary.Type = LEAK_NODE_HANDLE_ENTRY;
		Entry->Auxiliary.Context = File;

		StringCchPrintf(Buffer, MAX_PATH, L"Back Trace ID %u", Entry->StackId);

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = Buffer;
		tvi.item.lParam = (LPARAM)Entry; 
		hItemEntry = TreeView_InsertItem(hWndTree, &tvi);	

	}

}

VOID
MmLeakInsertGdiData(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem
	)
{
	PBTR_LEAK_FILE File;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItemEntry;
	HWND hWndTree;
	ULONG i;
	PBTR_LEAK_ENTRY Entry;
	WCHAR Buffer[MAX_PATH];

	hWndTree = TreeList->hWndTree;

	File = (PBTR_LEAK_FILE)ApsGetStreamPointer(Report, STREAM_LEAK_GDI);
	if (!File || File->Count == 0) {

		PBTR_LEAK_CONTEXT Context;
		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_GDI_NONE;

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"None";
		tvi.item.lParam = (LPARAM)Context; 
		TreeView_InsertItem(hWndTree, &tvi);
		return;
	}

	File->Auxiliary.Type = LEAK_NODE_GDI_FILE;
	File->Auxiliary.Context = NULL;

	for(i = 0; i < File->Count; i++) {

		Entry = &File->Leak[i];
		Entry->Auxiliary.Type = LEAK_NODE_GDI_ENTRY;
		Entry->Auxiliary.Context = File;

		StringCchPrintf(Buffer, MAX_PATH, L"Back Trace ID %u", Entry->StackId);

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = NULL; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = Buffer;
		tvi.item.lParam = (LPARAM)Entry; 
		hItemEntry = TreeView_InsertItem(hWndTree, &tvi);	

	}

}

#define ApsMarkPointerMsb(_P)        \
{                                    \
	_P = (LONG_PTR)0x80000000 | _P;  \
}

#define ApsClearPointerMsb(_P)       \
{                                    \
	_P = (_P << 1) >> 1;             \
}

VOID
MmLeakInsertBackTrace(
	__in HWND hWnd,
	__in PBTR_LEAK_CONTEXT Context
	)
{
	PPF_REPORT_HEAD Report;
	PBTR_LEAK_ENTRY Entry;
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	PBTR_STACK_RECORD StackRecord;
	ULONG i, j;
	WCHAR Buffer[MAX_PATH];
	HWND hWndCtrl;
	LVITEM lvi = {0};
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT ObjectContext;

    PBTR_LINE_ENTRY Line;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_DLL_FILE DllFile;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ObjectContext = SdkGetContext(Object, MM_FORM_CONTEXT);
	Report = ObjectContext->Head;

	if (!Report) {
		return;
	}

	//
	// Clear old list items
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_BACKTRACE);
	ListView_DeleteAllItems(hWndCtrl);


	switch (Context->Type) {

	case LEAK_NODE_HEAP_ROOT:
	case LEAK_NODE_PAGE_ROOT:
	case LEAK_NODE_HANDLE_ROOT:
	case LEAK_NODE_GDI_ROOT:
	case LEAK_NODE_HEAP_NONE:
	case LEAK_NODE_PAGE_NONE:
	case LEAK_NODE_HANDLE_NONE:
	case LEAK_NODE_GDI_NONE:
	case LEAK_NODE_HEAP_FILE:
	case LEAK_NODE_BACKTRACE:
		return;

	case LEAK_NODE_HEAP_ENTRY:
	case LEAK_NODE_PAGE_ENTRY:
	case LEAK_NODE_HANDLE_ENTRY:
	case LEAK_NODE_GDI_ENTRY:
		Entry = (PBTR_LEAK_ENTRY)Context;
		break;

	}

	StackRecord = (PBTR_STACK_RECORD)ApsGetStreamPointer(Report, STREAM_STACK);
	StackRecord = &StackRecord[Entry->StackId];

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Report, STREAM_DLL);

	Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;
		
		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID) Table;
	}

    //
    // N.B. Read source line information
    //

    if (Report->Streams[STREAM_LINE].Offset != 0 && Report->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Report + Report->Streams[STREAM_LINE].Offset);
	} else {
		Line = NULL;
	}    

	for(i = 0, j = 0; i < StackRecord->Depth; i++) {

		Context = (PBTR_LEAK_CONTEXT)SdkMalloc(sizeof(BTR_LEAK_CONTEXT));
		Context->Type = LEAK_NODE_BACKTRACE;

		Text = ApsLookupSymbol(Table, (ULONG64)StackRecord->Frame[i]);
		if (!Text) {
            Text = ApsInsertPseudoTextEntry(Table, (ULONG64)StackRecord->Frame[i]);
		}

		Context->Context = Text;

		//
		// frame number
		//

		lvi.iItem = j;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.lParam = (LPARAM)Context;
		StringCchPrintf(Buffer, MAX_PATH, L"%02u", j);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);

		//
		// symbol name
		//

		lvi.iItem = j;
		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%S", Text->Text);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// module 
		//

		lvi.iItem = j;
		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;

		DllEntry = ApsGetDllEntryByPc(DllFile, StackRecord->Frame[i]);
		if (DllEntry) {
			ApsGetDllBaseNameById(Report, DllEntry->DllId, Buffer, MAX_PATH);
		} else {
			StringCchCopy(Buffer, MAX_PATH, L"N/A");
		}

		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// line information
		//

		lvi.iItem = j;
		lvi.iSubItem = 3;
		lvi.mask = LVIF_TEXT;

		if (Text->LineId != -1) {
			ASSERT(Line != NULL);
			LineEntry = Line + Text->LineId;
			StringCchPrintf(Buffer, MAX_PATH, L"%S : %u", LineEntry->File, LineEntry->Line);
			lvi.pszText = Buffer;
		}
		else {
			lvi.pszText = L""; 
		}

		ListView_SetItem(hWndCtrl, &lvi);
		j += 1;
	}
}

LRESULT 
MmLeakOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmLeakOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmLeakOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
MmLeakOnTreeListSelChanged(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lp;
	TVITEM tvi = {0};

	tvi.mask = TVIF_PARAM;
	tvi.hItem = lpnmtv->itemNew.hItem;
	TreeView_GetItem(lpnmtv->hdr.hwndFrom, &tvi);

	MmLeakInsertBackTrace(hWnd, (PBTR_LEAK_CONTEXT)tvi.lParam);
	SdkFree(lpnmtv);
	return 0;
}

LRESULT
MmLeakOnNotify(
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

	if(IDC_LIST_BACKTRACE == pNmhdr->idFrom) {

		switch (pNmhdr->code) {
		case NM_DBLCLK:
			Status = MmLeakOnDbClick(Object, (LPNMITEMACTIVATE)lp);
			break;
		}
	}


	return Status;
}

INT_PTR CALLBACK
MmLeakProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return MmLeakOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return MmLeakOnClose(hWnd, uMsg, wp, lp);

	case WM_TREELIST_SORT:
		MmLeakOnTreeListSort(hWnd, uMsg, wp, lp);
		break;

	case WM_TREELIST_SELCHANGED:
		MmLeakOnTreeListSelChanged(hWnd, uMsg, wp, lp);
		break;

	case WM_NOTIFY:
		return MmLeakOnNotify(hWnd, uMsg, wp, lp);
	}

	return Status;
}

VOID CALLBACK
MmLeakFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	)
{
	PBTR_LEAK_CONTEXT Context;
	PBTR_LEAK_FILE File;
	PBTR_LEAK_ENTRY Entry;
	PBTR_TEXT_ENTRY Text;
    PPF_REPORT_HEAD Report;
    PPF_STREAM_HEAP_LEAK HeapLeak;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;

	if (!Value) {
		return;
	}

	Report = (PPF_REPORT_HEAD)TreeList->Context;
	Context = (PBTR_LEAK_CONTEXT)Value;

	switch (Context->Type) {

	case LEAK_NODE_HEAP_ROOT:

		if (Column == 0) {
			StringCchCopy(Buffer, Length, L"Heap");
		}

        else if (Column == 1) {
            Report = (PPF_REPORT_HEAD)Context->Context;
            HeapLeak = (PPF_STREAM_HEAP_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_HEAP);
            StringCchPrintf(Buffer, Length, L"%I64u", HeapLeak->LeakCount);
        }

        else if (Column == 2) {
            Report = (PPF_REPORT_HEAD)Context->Context;
            HeapLeak = (PPF_STREAM_HEAP_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_HEAP);
            StringCchPrintf(Buffer, Length, L"%I64u", HeapLeak->Bytes);
        }

		break;

	case LEAK_NODE_PAGE_ROOT:
		if (Column == 0) {
			StringCchCopy(Buffer, Length, L"Page");
		}
		else if (Column == 1) {
            Report = (PPF_REPORT_HEAD)Context->Context;
            File = (PPF_STREAM_PAGE_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_PAGE);
            StringCchPrintf(Buffer, Length, L"%u", File->LeakCount);
        }

        else if (Column == 2) {
            Report = (PPF_REPORT_HEAD)Context->Context;
            File = (PPF_STREAM_PAGE_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_PAGE);
            StringCchPrintf(Buffer, Length, L"%I64u", File->Bytes);
        }
		break;

	case LEAK_NODE_HANDLE_ROOT:
		if (Column == 0) {
			StringCchCopy(Buffer, Length, L"Handle");
		}
		else if (Column == 1) {
            Report = (PPF_REPORT_HEAD)Context->Context;
            File = (PPF_STREAM_HANDLE_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_HANDLE);
            StringCchPrintf(Buffer, Length, L"%u", File->LeakCount);
        }

        else if (Column == 2) {

			//
			// N.B. bytes does not make sense to HANDLE
			//

			Buffer[0] = '0';
			Buffer[1] = 0;
        }
		break;

	case LEAK_NODE_GDI_ROOT:

		if (Column == 0) {
			StringCchCopy(Buffer, Length, L"GDI");
		}
		else if (Column == 1) {
            Report = (PPF_REPORT_HEAD)Context->Context;
            File = (PPF_STREAM_GDI_LEAK)ApsGetStreamPointer(Report, STREAM_LEAK_GDI);
            StringCchPrintf(Buffer, Length, L"%u", File->LeakCount);
        }

        else if (Column == 2) {

			//
			// N.B. bytes does not make sense to GDI objects 
			//

			Buffer[0] = '0';
			Buffer[1] = 0;
        }
		break;

	case LEAK_NODE_HEAP_NONE:
	case LEAK_NODE_PAGE_NONE:
	case LEAK_NODE_HANDLE_NONE:
	case LEAK_NODE_GDI_NONE:
		if (Column == 0) {
			StringCchCopy(Buffer, Length, L"None");
		}
		break;

	case LEAK_NODE_HEAP_FILE:

		File = (PBTR_LEAK_FILE)Value;
		if (Column == 0) {

		#if defined(_M_IX86)
			StringCchPrintf(Buffer, MAX_PATH, L"Heap Handle 0x%08x", (ULONG_PTR)File->Context);
		#endif
		#if defined(_M_X64)
			StringCchPrintf(Buffer, MAX_PATH, L"Heap Handle 0x%0I64x", (ULONG_PTR)File->Context);
		#endif

		}
		if (Column == 1) {
			StringCchPrintf(Buffer, Length, L"%u", File->LeakCount);
		}
		if (Column == 2) {
			StringCchPrintf(Buffer, Length, L"%I64u", File->Bytes);
		}
		if (Column == 3) {
			Buffer[0] = 0;
		}
		break;

	case LEAK_NODE_HEAP_ENTRY:
		Entry = (PBTR_LEAK_ENTRY)Value;
		if (Column == 0) {
			StringCchPrintf(Buffer, Length, L"Back Trace ID %d", Entry->StackId);
		}
		if (Column == 1) {
			StringCchPrintf(Buffer, Length, L"%u", Entry->Count);
		}
		if (Column == 2) {
			StringCchPrintf(Buffer, Length, L"%I64u", Entry->Size);
		}
		if (Column == 3) {
			Buffer[0] = 0;
		}
		break;

	case LEAK_NODE_PAGE_FILE:
		ASSERT(0);
		break;

	case LEAK_NODE_PAGE_ENTRY:
		Entry = (PBTR_LEAK_ENTRY)Value;
		if (Column == 0) {
			StringCchPrintf(Buffer, Length, L"Back Trace ID %d", Entry->StackId);
		}
		if (Column == 1) {
			StringCchPrintf(Buffer, Length, L"%u", Entry->Count);
		}
		if (Column == 2) {
			StringCchPrintf(Buffer, Length, L"%I64u", Entry->Size);
		}
		if (Column == 3) {
			Buffer[0] = 0;
		}
		break;

	case LEAK_NODE_HANDLE_FILE:
		ASSERT(0);
		break;

	case LEAK_NODE_HANDLE_ENTRY:
		Entry = (PBTR_LEAK_ENTRY)Value;
		if (Column == 0) {
			StringCchPrintf(Buffer, Length, L"Back Trace ID %d", Entry->StackId);
		}
		if (Column == 1) {
			StringCchPrintf(Buffer, Length, L"%u", Entry->Count);
		}
		if (Column == 2) {

			//
			// N.B. bytes does not make sense to GDI objects 
			//

			Buffer[0] = '0';
			Buffer[1] = 0;
		}
		if (Column == 3) {
			Buffer[0] = 0;
		}
		break;

	case LEAK_NODE_GDI_FILE:
		ASSERT(0);
		break;

	case LEAK_NODE_GDI_ENTRY:
		Entry = (PBTR_LEAK_ENTRY)Value;
		if (Column == 0) {
			StringCchPrintf(Buffer, Length, L"Back Trace ID %d", Entry->StackId);
		}
		if (Column == 1) {
			StringCchPrintf(Buffer, Length, L"%u", Entry->Count);
		}
		if (Column == 2) {

			//
			// N.B. bytes does not make sense to GDI objects 
			//

			Buffer[0] = '0';
			Buffer[1] = 0;
		}
		if (Column == 3) {
			Buffer[0] = 0;
		}
		break;

	case LEAK_NODE_BACKTRACE:

		Context = (PBTR_LEAK_CONTEXT)Value;
		Text = (PBTR_TEXT_ENTRY)Context->Context;

		if (Column == 0) {
			StringCchPrintf(Buffer, Length, L"%S", Text->Text);
		}

		if (Column == 1) {

			if (Text->LineId != -1) {

				Line = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Report, STREAM_LINE);
				ASSERT(Line != NULL);

				LineEntry = Line + Text->LineId;
				StringCchPrintf(Buffer, MAX_PATH, L"%S : %u", LineEntry->File, LineEntry->Line);
			}
			else {
				Buffer[0] = 0;
			}
		}
		break;
	}

}

int CALLBACK
MmLeakCompareCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
	PBTR_LEAK_CONTEXT C1, C2;
	PBTR_LEAK_ENTRY E1, E2;
	PBTR_LEAK_FILE F1, F2;
	PMM_SORT_CONTEXT Context;
    int Result;

    Result = 0;
	C1 = (PBTR_LEAK_CONTEXT)First;
	C2 = (PBTR_LEAK_CONTEXT)Second;
	
	Context = (PMM_SORT_CONTEXT)Param;

    //
    // Compare Heap entries
    //

	if (C1->Type == LEAK_NODE_HEAP_FILE && C2->Type == LEAK_NODE_HEAP_FILE) {

		F1 = (PBTR_LEAK_FILE)First;
		F2 = (PBTR_LEAK_FILE)Second;

		if (Context->Column == 1) {
			Result = F1->Count - F2->Count;
		}
		else if (Context->Column == 2) {
			Result = (int)(F1->Bytes - F2->Bytes);
		}
	}

	if (C1->Type == LEAK_NODE_HEAP_ENTRY && C2->Type == LEAK_NODE_HEAP_ENTRY) {

		E1 = (PBTR_LEAK_ENTRY)First;
		E2 = (PBTR_LEAK_ENTRY)Second;

		if (Context->Column == 1) {
			Result = E1->Count - E2->Count;
		}
		if (Context->Column == 2) {
			Result = (int)(E1->Size - E2->Size);
		}

	}

    //
    // Compare Page entries
    //

	if (C1->Type == LEAK_NODE_PAGE_ENTRY && C2->Type == LEAK_NODE_PAGE_ENTRY) {

		E1 = (PBTR_LEAK_ENTRY)First;
		E2 = (PBTR_LEAK_ENTRY)Second;

		if (Context->Column == 1) {
			Result = E1->Count - E2->Count;
        }
    }

    //
    // Compare Kernel handle entries
    //

    if (C1->Type == LEAK_NODE_HANDLE_ENTRY && C2->Type == LEAK_NODE_HANDLE_ENTRY) {

		E1 = (PBTR_LEAK_ENTRY)First;
		E2 = (PBTR_LEAK_ENTRY)Second;

		if (Context->Column == 1) {
			Result = E1->Count - E2->Count;
		}
	}

    //
    // Compare GDI handle entries
    //

	if (C1->Type == LEAK_NODE_GDI_ENTRY && C2->Type == LEAK_NODE_GDI_ENTRY) {

		E1 = (PBTR_LEAK_ENTRY)First;
		E2 = (PBTR_LEAK_ENTRY)Second;

		if (Context->Column == 1) {
			Result = E1->Count - E2->Count;
		}
	}
	
	if (Context->Order == SortOrderDescendent) {
		Result = -Result;
	}

	return Result;
}

LRESULT
MmLeakOnTreeListSort(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	ULONG Column;
	TVSORTCB Tsc = {0};
	PTREELIST_OBJECT TreeList;
	PDIALOG_OBJECT Object;
	HTREEITEM hItemGroup;
	TVITEM tvi = {0};
	PMM_FORM_CONTEXT Context;
	MM_SORT_CONTEXT SortContext;

	Column = (ULONG)wp;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
	TreeList = Context->TreeList;
	
	SortContext.Context = (LPARAM)NULL;
	SortContext.Column = Column;
	SortContext.Order = TreeList->SortOrder;

	Tsc.hParent = TVI_ROOT;
	Tsc.lParam = (LPARAM)&SortContext;
	Tsc.lpfnCompare = MmLeakCompareCallback;
	TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

	hItemGroup = TreeView_GetChild(TreeList->hWndTree, TVI_ROOT);

	while (hItemGroup != NULL) {
		
        //
        // Sort all heaps
        //

		tvi.mask = TVIF_PARAM;
		tvi.hItem = hItemGroup;
		TreeView_GetItem(TreeList->hWndTree, &tvi);

		SortContext.Context = tvi.lParam;
		SortContext.Column = Column;
		SortContext.Order = TreeList->SortOrder;

		Tsc.hParent = hItemGroup;
		Tsc.lParam = (LPARAM)&SortContext;
		Tsc.lpfnCompare = MmLeakCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);
        
        //
        // Sort heap entry
        //

        MmLeakSortHeapEntry(TreeList, hItemGroup, Column);

        //
        // Next sibiling's sorting cycle
        //

		hItemGroup = TreeView_GetNextSibling(TreeList->hWndTree, hItemGroup);
	}

	return 0;
}

VOID
MmLeakSortHeapEntry(
    __in PTREELIST_OBJECT TreeList,
    __in HTREEITEM Parent,
    __in ULONG Column
    )
{
	TVSORTCB Tsc = {0};
	HTREEITEM Item;
	TVITEM tvi = {0};
	MM_SORT_CONTEXT SortContext;

    Item = TreeView_GetChild(TreeList->hWndTree, Parent);

	while (Item != NULL) {
		
		tvi.mask = TVIF_PARAM;
		tvi.hItem = Item;
		TreeView_GetItem(TreeList->hWndTree, &tvi);

		SortContext.Context = tvi.lParam;
		SortContext.Column = Column;
		SortContext.Order = TreeList->SortOrder;

		Tsc.hParent = Item;
		Tsc.lParam = (LPARAM)&SortContext;
		Tsc.lpfnCompare = MmLeakCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

		Item = TreeView_GetNextSibling(TreeList->hWndTree, Item);
	}
}

LRESULT 
MmLeakOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	)
{
	HWND hWndList;
	PWCHAR Ptr;
	WCHAR Buffer[MAX_PATH];
	size_t Length;
	ULONG Line;

	//
	// Check whether source column is clicked
	//

	if (lpnmitem->iSubItem != 3) {
		return 0;
	}

	//
	// Check whether there's any source information
	//

	hWndList = lpnmitem->hdr.hwndFrom;

	Buffer[0] = 0;
	ListView_GetItemText(hWndList, lpnmitem->iItem, 3, Buffer, MAX_PATH);

	Length = wcslen(Buffer);
	if (!Length) {

		//
		// there's no source information
		//

		return 0;
	}

	Ptr = wcsrchr(Buffer, L':');
	ASSERT(Ptr != NULL);

	Line = _wtoi(Ptr + 1);
	Ptr[0] = 0;

	FrameShowSource(Object->hWnd, Buffer, Line);
	return 0;
}