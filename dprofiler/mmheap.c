//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "mmheap.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"
#include "split.h"

typedef enum _MmHeapColumnType {
	MmHeapName,
	MmHeapCount,
	MmHeapSize,
	MmHeapColumnCount,
} MmHeapColumnType;

HEADER_COLUMN 
MmHeapColumn[MmHeapColumnCount] = {
	{ {0}, 100,  HDF_LEFT,  FALSE, FALSE, L" Name" },
	{ {0}, 160,  HDF_RIGHT, FALSE, FALSE, L" Count" },
	{ {0}, 160,  HDF_RIGHT, FALSE, FALSE, L" Bytes" },
};

DIALOG_SCALER_CHILD MmHeapChildren[3] = {
	{ IDC_TREELIST, AlignNone, AlignBottom},
	{ IDC_SPLIT, AlignNone, AlignBottom },
	{ IDC_LIST_BACKTRACE, AlignRight, AlignBottom},
};

DIALOG_SCALER MmHeapScaler = {
	{0,0}, {0,0}, {0,0}, 3, MmHeapChildren
};

//
// Back trace list column
//

LISTVIEW_COLUMN MmHeapListColumn[] = {
	{ 20,  L"#",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Call Site", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Module", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Line", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

//
// Vertical splitter layout
//

SPLIT_INFO MmHeapSplitInfo[2] = {
	{ IDC_TREELIST, SPLIT_RIGHT },
	{ IDC_LIST_BACKTRACE, SPLIT_LEFT }
};

//
// Vertical split object
//

SPLIT_OBJECT MmHeapSplitObject = {
	TRUE, MmHeapSplitInfo, 2, 20 
};

HWND
MmHeapCreate(
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
	Object->ResourceId = IDD_FORMVIEW_MM_HEAP;
	Object->Procedure = MmHeapProcedure;

	return DialogCreateModeless(Object);
}

LRESULT
MmHeapOnInitDialog(
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
	ULONG i;
	LVCOLUMN lvc = {0};
	RECT CtrlRect;

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
							  MmHeapFormatCallback, 
							  MmHeapColumnCount);

	//
	// We need explicitly bind the treelist object to its hwnd
	// if it's hosted in dialog as custom control.
	//

	TreeList->hWnd = hWndCtrl;
	SdkSetObject(hWndCtrl, TreeList);
	TreeListCreateControls(TreeList);

	for(Number = 0; Number < MmHeapColumnCount; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 MmHeapColumn[Number].Align, 
							 MmHeapColumn[Number].Width, 
							 MmHeapColumn[Number].Name );
	}

	ASSERT(TreeList->hWnd != NULL);
	Context->TreeList = TreeList;

	//
	// Initialize splitbar
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_SPLIT);
	SplitSetObject(hWndCtrl, &MmHeapSplitObject);

	//
	// Initialize listview
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_BACKTRACE);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	for (i = 0; i < 4; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = MmHeapListColumn[i].Title;	
		lvc.cx = MmHeapListColumn[i].Width;     
		lvc.fmt = MmHeapListColumn[i].Align;
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

	Object->Scaler = &MmHeapScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

VOID
MmHeapInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head 
	)
{
	PTREELIST_OBJECT TreeList;
	HWND hWndTree;
	ULONG Count;
	RECT rect;
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT FormContext;
	TVINSERTSTRUCT tvi = {0};
	ULONG i;
	PBTR_DLL_FILE DllFile;
	PBTR_STACK_RECORD BackTrace;
	ULONG DllId;
	PBTR_DLL_ENTRY DllEntry;
	WCHAR Buffer[MAX_PATH];
	HTREEITEM hItemDll;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	FormContext = SdkGetContext(Object, MM_FORM_CONTEXT);
	FormContext->Head = Head;

	TreeList = FormContext->TreeList;
	ASSERT(TreeList != NULL);

	hWndTree = TreeList->hWndTree;
	Count = 0;

	//
	// Initialize dll file
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
	for(i = 0; i < DllFile->Count; i++) {
		InitializeListHead(&DllFile->Dll[i].ListHead);
	}

	BackTrace = (PBTR_STACK_RECORD)ApsGetStreamPointer(Head, STREAM_STACK);
	Count = ApsGetStreamRecordCount(Head, STREAM_STACK, BTR_STACK_RECORD);
	

	//
	// Scan all stack traces and chain each record into dll entry by its caller
	//

	for(i = 0; i < Count; i++) {

		if (!BackTrace->Heap) {
			BackTrace += 1;
			continue;
		}

		DllId = ApsGetDllIdByAddress(DllFile, (ULONG64)BackTrace->Frame[1]);

		if (DllId != (ULONG)-1) {
			DllEntry = &DllFile->Dll[DllId];
			InsertTailList(&DllEntry->ListHead, &BackTrace->ListEntry2);
		}

		BackTrace += 1;
	}

	//
	// Insert tree items by dll entry
	//

	for(i = 0; i < DllFile->Count; i++) {

		DllEntry = &DllFile->Dll[i];

		if (DllEntry->Count != 0) {

			tvi.hParent = TVI_ROOT; 
			tvi.hInsertAfter = TVI_LAST; 
			tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 

			_wsplitpath(DllEntry->Path, NULL, NULL, Buffer, NULL);
			tvi.item.pszText = Buffer;

			tvi.item.lParam = (LPARAM)DllEntry; 
			hItemDll = TreeView_InsertItem(hWndTree, &tvi);

			//
			// Insert back trace
			//

			ListHead = &DllEntry->ListHead;
			ListEntry = ListHead->Flink;

			while (ListEntry != ListHead) {

				BackTrace = CONTAINING_RECORD(ListEntry, BTR_STACK_RECORD, ListEntry2);
				StringCchPrintf(Buffer, MAX_PATH, L"BackTrace ID %u", BackTrace->StackId);

				tvi.hParent = hItemDll; 
				tvi.hInsertAfter = TVI_LAST; 
				tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
				tvi.item.pszText = Buffer;
				tvi.item.lParam = (LPARAM)BackTrace; 
				TreeView_InsertItem(hWndTree, &tvi);	

				ListEntry = ListEntry->Flink;
			}
		}
	}

	GetClientRect(hWnd, &rect);
	InvalidateRect(hWnd, &rect, TRUE);
	UpdateWindow(hWnd);
}

HTREEITEM
MmHeapInsertBackTrace(
	__in PPF_REPORT_HEAD Report,
	__in PTREELIST_OBJECT TreeList,
	__in HTREEITEM hTreeItem,
	__in PBTR_STACK_RECORD Record
	)
{
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	HTREEITEM hItemParent;
	HTREEITEM hItemFrame;
	TVINSERTSTRUCT tvi = {0};
	HWND hWndTree;
	ULONG i;
	WCHAR Buffer[MAX_PATH];

	hWndTree = TreeList->hWndTree;

	Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;
		
		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID) Table;
	}

	//
	// Insert stacktrace id
	//

	StringCchPrintf(Buffer, MAX_PATH, L"BackTrace ID %u", Record->StackId);

	tvi.hParent = hTreeItem; 
	tvi.hInsertAfter = TVI_LAST; 
	tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
	tvi.item.pszText = Buffer;
	tvi.item.lParam = (LPARAM)Record; 
	hItemParent = TreeView_InsertItem(hWndTree, &tvi);	

	for(i = 0; i < Record->Depth; i++) {

		Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[i]);
		if (!Text) {
            Text = ApsInsertPseudoTextEntry(Table, (ULONG64)Record->Frame[i]);
		}

		StringCchPrintf(Buffer, MAX_PATH, L"%S", Text->Text);

		tvi.hParent = hItemParent; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = Buffer;
		tvi.item.lParam = (LPARAM)Text; 
		hItemFrame = TreeView_InsertItem(hWndTree, &tvi);	
	}

	return hItemFrame;
}

LRESULT 
MmHeapOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmHeapOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmHeapOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return 0;
}

INT_PTR CALLBACK
MmHeapProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return MmHeapOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return MmHeapOnClose(hWnd, uMsg, wp, lp);

	case WM_TREELIST_SORT:
		MmHeapOnTreeListSort(hWnd, uMsg, wp, lp);
		break;

	case WM_TREELIST_SELCHANGED:
		MmHeapOnTreeListSelChanged(hWnd, uMsg, wp, lp);
		break;

	}

	return Status;
}

VOID CALLBACK
MmHeapFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	)
{
	HTREEITEM hItemParent;
	PBTR_TEXT_ENTRY TextEntry;
	PBTR_STACK_RECORD Record;
	PBTR_DLL_ENTRY DllEntry;
	HWND hWndTree;

	hWndTree = TreeList->hWndTree;
	hItemParent = TreeView_GetParent(hWndTree, hTreeItem);

	if (hItemParent == NULL) {
		
		DllEntry = (PBTR_DLL_ENTRY)Value;
		switch (Column) {
			case 0:
				_wsplitpath(DllEntry->Path, NULL, NULL, Buffer, NULL);
				break;
			case 1:
				StringCchPrintf(Buffer, Length, L"%u", DllEntry->CountOfAllocs);
				break;
			case 2:
				StringCchPrintf(Buffer, Length, L"%u", DllEntry->SizeOfAllocs);
				break;
		}
	}

	else {
	
		hItemParent = TreeView_GetParent(hWndTree, hItemParent);
		if (hItemParent == NULL) {

			Record = (PBTR_STACK_RECORD)Value;
			switch (Column) {
			case 0:
				StringCchPrintf(Buffer, Length, L"BackTrace ID %u", Record->StackId);
				break;
			case 1:
				StringCchPrintf(Buffer, Length, L"%u", Record->Count);
				break;
			case 2:
				StringCchPrintf(Buffer, Length, L"%u", Record->SizeOfAllocs);
				break;
			}

		}
		else {

			TextEntry = (PBTR_TEXT_ENTRY)Value;

			switch (Column) {
			case 0:
				StringCchPrintf(Buffer, Length, L"%S", TextEntry->Text);
				break;
			case 1:
			case 2:
				break;
			}
		}
	}
}

int CALLBACK
MmHeapCompareCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
	PBTR_STACK_RECORD Stack1, Stack2;
	PBTR_DLL_ENTRY Dll1, Dll2;
	PMM_SORT_CONTEXT Context;
	int Result;

	Result = 0;
	Context = (PMM_SORT_CONTEXT)Param;

	if (!Context->Context) {

		Dll1 = (PBTR_DLL_ENTRY)First;
		Dll2 = (PBTR_DLL_ENTRY)Second;

		if (Context->Column == 1) {
			Result = Dll1->CountOfAllocs - Dll2->CountOfAllocs;
		}

		if (Context->Column == 2) {
			Result = (int)(Dll1->SizeOfAllocs - Dll2->SizeOfAllocs);
		}
	}

	else {
		
		Stack1 = (PBTR_STACK_RECORD)First;
		Stack2 = (PBTR_STACK_RECORD)Second;

		if (Context->Column == 1) {
			Result = Stack1->Count - Stack2->Count;
		}

		if (Context->Column == 2) {
			Result = (int)(Stack1->SizeOfAllocs - Stack2->SizeOfAllocs);
		}
	}

	if (Context->Order == SortOrderDescendent) {
		Result = -Result;
	}

	return Result;
}

LRESULT
MmHeapOnTreeListSort(
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
	
	//
	// Sort dll items
	//

	SortContext.Context = (LPARAM)NULL;
	SortContext.Column = Column;
	SortContext.Order = TreeList->SortOrder;

	Tsc.hParent = TVI_ROOT;
	Tsc.lParam = (LPARAM)&SortContext;
	Tsc.lpfnCompare = MmHeapCompareCallback;
	TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

	hItemGroup = TreeView_GetChild(TreeList->hWndTree, TVI_ROOT);
	while (hItemGroup != NULL) {
		
		tvi.mask = TVIF_PARAM;
		tvi.hItem = hItemGroup;
		TreeView_GetItem(TreeList->hWndTree, &tvi);

		//
		// Sort back trace item
		//

		SortContext.Context = tvi.lParam;
		SortContext.Column = Column;
		SortContext.Order = TreeList->SortOrder;

		Tsc.hParent = hItemGroup;
		Tsc.lParam = (LPARAM)&SortContext;
		Tsc.lpfnCompare = MmHeapCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

		hItemGroup = TreeView_GetNextSibling(TreeList->hWndTree, hItemGroup);
	}

	return 0;
}

LRESULT
MmHeapOnTreeListSelChanged(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lp;
	MmHeapInsertBackTraceEx(hWnd, lpnmtv->hdr.hwndFrom,
		                    lpnmtv->itemNew.hItem);
	SdkFree(lpnmtv);
	return 0;
}

VOID
MmHeapInsertBackTraceEx(
	__in HWND hWnd,
	__in HWND hWndTree,
	__in HTREEITEM hTreeItem
	)
{
	TVITEM tvi = {0};
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT ObjectContext;
	PPF_REPORT_HEAD Report;
	PBTR_STACK_RECORD Record;
	HWND hWndCtrl;
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	WCHAR Buffer[MAX_PATH];
	ULONG i;
	LVITEM lvi = {0};
	PBTR_DLL_ENTRY DllEntry;
	PBTR_DLL_FILE DllFile;
    PBTR_LINE_ENTRY Line;
	PBTR_LINE_ENTRY LineEntry;

	//
	// Skip dll entry
	//

	if (TreeView_GetParent(hWndTree, hTreeItem) == TVI_ROOT) {
		return;
	}

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

	tvi.mask = TVIF_PARAM;
	tvi.hItem = hTreeItem;
	TreeView_GetItem(hWndTree, &tvi);
	
	Record = (PBTR_STACK_RECORD)tvi.lParam;
	ASSERT(Record != NULL);

	Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;
		
		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID) Table;
	}

	//
	// Get dll file
	//

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Report, STREAM_DLL);

	 //
    // N.B. Read source line information
    //

    if (Report->Streams[STREAM_LINE].Offset != 0 && Report->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Report + Report->Streams[STREAM_LINE].Offset);
	} else {
		Line = NULL;
	}    

	for(i = 0; i < Record->Depth; i++) {

		Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[i]);
		if (!Text) {
            Text = ApsInsertPseudoTextEntry(Table, (ULONG64)Record->Frame[i]);
		}

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
		StringCchPrintf(Buffer, MAX_PATH, L"%S", Text->Text);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);
		
		//
		// module 
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

		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// line information
		//
		
		lvi.iItem = i;
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
	}
}