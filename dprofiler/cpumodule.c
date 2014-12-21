//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "apsbtr.h"
#include "apsprofile.h"
#include "apspdb.h"
#include "aps.h"
#include "apsrpt.h"
#include "treelist.h"
#include "cpumodule.h"
#include "dialog.h"
#include "profileform.h"
#include "frame.h"

DIALOG_SCALER_CHILD CpuModuleChildren[1] = {
	{ IDC_TREELIST, AlignRight, AlignBottom }
};

DIALOG_SCALER CpuModuleScaler = {
	{0,0}, {0,0}, {0,0}, 1, CpuModuleChildren
};

HEADER_COLUMN CpuModuleColumn[5] = {
	{ {0}, 120, HDF_LEFT,  FALSE, FALSE, L"Name" },
	{ {0}, 160, HDF_RIGHT, FALSE, FALSE, L"Inclusive" },
	{ {0}, 160, HDF_RIGHT, FALSE, FALSE, L"Inclusive %" },
	{ {0}, 160, HDF_RIGHT, FALSE, FALSE, L"Exclusive" },
	{ {0}, 160, HDF_RIGHT, FALSE, FALSE, L"Exclusive %" },
};

HWND
CpuModuleCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
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
	Object->ResourceId = IDD_FORMVIEW_CPU_MODULE;
	Object->Procedure = CpuModuleProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CpuModuleOnInitDialog(
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

	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST);
	TreeList = TreeListCreate(hWnd, TRUE, IDC_TREELIST, 
							  &Rect, hWnd, 
							  CpuModuleFormatCallback, 
							  5);

	//
	// We need explicitly bind the treelist object to its hwnd
	// if it's hosted in dialog as custom control.
	//

	TreeList->hWnd = hWndCtrl;
	SdkSetObject(hWndCtrl, TreeList);
	TreeListCreateControls(TreeList);

	for(Number = 0; Number < 5; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 CpuModuleColumn[Number].Align, 
							 CpuModuleColumn[Number].Width, 
							 CpuModuleColumn[Number].Name);
	}

	ASSERT(TreeList->hWnd != NULL);
	Context->TreeList = TreeList;

	//
	// Position treelist to fill full client area
	//

	GetClientRect(hWnd, &Rect);
	hWndCtrl = GetDlgItem(hWnd, IDC_TREELIST);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &CpuModuleScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT CALLBACK 
CpuModuleHeaderProcedure(
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
CpuModuleOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuModuleOnClose(
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
CpuModuleProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuModuleOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuModuleOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return CpuModuleOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuModuleOnNotify(hWnd, uMsg, wp, lp);
	
	case WM_TREELIST_SORT:
		return CpuModuleOnTreeListSort(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuModuleOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuModuleOnNotify(
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
			return CpuModuleOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			Status = CpuModuleOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;
		}
	}


	return Status;
}

LRESULT
CpuModuleOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0;
    return Status;
}

LRESULT 
CpuModuleOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
    return 0;
}

int CALLBACK
CpuModuleSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	return 0;	
}

VOID
CpuModuleInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	PBTR_PC_ENTRY PcEntry;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	TVINSERTSTRUCT tvi = {0};
	ULONG i, j;
	HWND hWndTree;
	PTREELIST_OBJECT TreeList;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	WCHAR Buffer[MAX_PATH];
	HTREEITEM hItemDll;
	PWSTR Unicode;
    PBTR_FUNCTION_ENTRY FuncEntry;
    ULONG FuncCount;
	double Percent;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	DllFile = (PBTR_DLL_FILE)((PUCHAR)Head + Head->Streams[STREAM_DLL].Offset);

    //
    // Get the total PC counts as computation base 
    //

	PcEntry = (PBTR_PC_ENTRY)((PUCHAR)Head + Head->Streams[STREAM_PC].Offset);

	ApsGetCpuSampleCounters(Head, &Context->Inclusive, &Context->Exclusive);
	//Context->Inclusive = ApsGetSampleCount(Head);
	//Context->Exclusive = Context->Inclusive;

    FuncEntry = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);
    if (!FuncEntry) {
        return;
    }

    FuncCount = ApsGetStreamLength(Head, STREAM_FUNCTION) / sizeof(BTR_FUNCTION_ENTRY);
    ApsInitializeDllFile(DllFile);

	//
	// Drop Pcs into each owner dll, note that dll file is assumed sorted by
	// its dll id in increasing order.
	//

	for(i = 0; i < FuncCount; i++) {
		
		if (FuncEntry->DllId != -1) {

			Percent = (FuncEntry->Inclusive * 1.0) / (Context->Inclusive * 1.0);
			if (Percent >= CPU_UI_INCLUSIVE_RATIO_THRESHOLD) {
				DllEntry = &DllFile->Dll[FuncEntry->DllId];
				InsertHeadList(&DllEntry->ListHead, &FuncEntry->ListEntry);
			}
		}

		FuncEntry += 1;
	}

	TreeList = Context->TreeList;
	hWndTree = TreeList->hWndTree;

	TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);
	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	//
	// Save text table into context of treelist object
	//

	TreeList->Context = TextTable;

	//
	// Insert each dll if its has child Pc
	//

	for(i = 0, j = 0; i < DllFile->Count; i++) {

		DllEntry = &DllFile->Dll[i];
		if (IsListEmpty(&DllEntry->ListHead)) {
			continue;
		}

		tvi.hParent = TVI_ROOT; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 

		_wsplitpath(DllEntry->Path, NULL, NULL, Buffer, NULL);
		tvi.item.pszText = Buffer;

		tvi.item.lParam = (LPARAM)DllEntry; 
		hItemDll = TreeView_InsertItem(hWndTree, &tvi);

		CpuModuleInsertPc(hWndTree, hItemDll, DllEntry, TextTable);
		j += 1;
	}

	Unicode = (PWSTR)ApsMalloc(MAX_PATH * 2);
	StringCchPrintf(Unicode, MAX_PATH, L"Total %u Module samples", j);
	PostMessage(GetParent(hWnd), WM_USER_STATUSBAR, 0, (LPARAM)Unicode);
}

HTREEITEM
CpuModuleInsertPc(
	__in HWND hWndTree,
	__in HTREEITEM hItemDll,
	__in PBTR_DLL_ENTRY DllEntry,
	__in PBTR_TEXT_TABLE TextTable
	)
{
	HTREEITEM hItemPc;
	TVINSERTSTRUCT tvi = {0};
	WCHAR Buffer[MAX_PATH];
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
    PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_TEXT_ENTRY TextEntry;
	PWSTR Unicode;

	ListHead = &DllEntry->ListHead;
	ListEntry = ListHead->Flink;
    Unicode = NULL;

	while (ListEntry != ListHead) {

		FuncEntry = CONTAINING_RECORD(ListEntry, BTR_FUNCTION_ENTRY, ListEntry);

		tvi.hParent = hItemDll; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 

		TextEntry = ApsLookupSymbol(TextTable, (ULONG64)FuncEntry->Address);
		if (!TextEntry) {
			StringCchPrintf(Buffer, MAX_PATH, L"0x%x", (PVOID)FuncEntry->Address);
			tvi.item.pszText = Buffer;
		} else {
			ApsConvertAnsiToUnicode(TextEntry->Text, &Unicode);
			tvi.item.pszText = Unicode;
		}

		tvi.item.lParam = (LPARAM)FuncEntry; 
		hItemPc = TreeView_InsertItem(hWndTree, &tvi);

        if (Unicode) {
            ApsFree(Unicode);
            Unicode = NULL;
        }

		ListEntry = ListEntry->Flink;
	}

	return hItemPc;
}

VOID CALLBACK
CpuModuleFormatCallback(
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
	PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_ENTRY TextEntry;
	PWSTR Unicode;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	HWND hWnd;

	hWnd = GetParent(TreeList->hWnd);
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	hWndTree = TreeList->hWndTree;
	TextTable = (PBTR_TEXT_TABLE)TreeList->Context;

	hTreeParentItem = TreeView_GetParent(hWndTree, hTreeItem);

	if (hTreeParentItem != NULL) {

		FuncEntry = (PBTR_FUNCTION_ENTRY)Value;

		switch (Column) {
			case 0:
				TextEntry = ApsLookupSymbol(TextTable, (ULONG64)FuncEntry->Address);
				if (!TextEntry) {
					StringCchPrintf(Buffer, Length, L"0x%x", (PVOID)FuncEntry->Address);
				} else {
					ApsConvertAnsiToUnicode(TextEntry->Text, &Unicode);
					StringCchCopy(Buffer, Length, Unicode);
					ApsFree(Unicode);
				}
				break;
			case 1:
				StringCchPrintf(Buffer, Length, L"%u", FuncEntry->Inclusive);
				break;
			case 2:
				StringCchPrintf(Buffer, Length, L"%.2f", FuncEntry->Inclusive * 100.0 / Context->Inclusive);
				break;
			case 3:
				StringCchPrintf(Buffer, Length, L"%u", FuncEntry->Exclusive);
				break;
			case 4:
				StringCchPrintf(Buffer, Length, L"%.2f", FuncEntry->Exclusive * 100.0 / Context->Exclusive);
				break;
		}

	} else {

		DllEntry = (PBTR_DLL_ENTRY)Value;

		switch (Column) {
			case 0:
				_wsplitpath(DllEntry->Path, NULL, NULL, Buffer, NULL);
				break;
			case 1:
				StringCchPrintf(Buffer, Length, L"%u", DllEntry->Inclusive);
				break;
			case 2:
				StringCchPrintf(Buffer, Length, L"%.2f", DllEntry->Inclusive * 100.0 / Context->Inclusive);
				break;
			case 3:
				StringCchPrintf(Buffer, Length, L"%u", DllEntry->Exclusive);
				break;
			case 4:
				StringCchPrintf(Buffer, Length, L"%.2f", DllEntry->Exclusive * 100.0 / Context->Exclusive);
				break;
		}
	}
}

int CALLBACK
CpuModuleCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	PBTR_FUNCTION_ENTRY F1, F2;
	PBTR_DLL_ENTRY D1, D2;
	PCPU_SORT_CONTEXT Context;
	int Result;

	Result = 0;
	Context = (PCPU_SORT_CONTEXT)Param;

	if (!Context->Context) {

		D1 = (PBTR_DLL_ENTRY)First;
		D2 = (PBTR_DLL_ENTRY)Second;

		if (Context->Column == 1 || Context->Column == 2) {
			Result = D1->Inclusive - D2->Inclusive;
		}

		if (Context->Column == 3 || Context->Column == 4) {
			Result = D1->Exclusive - D2->Exclusive;
		}
	}

	else {
		
		F1 = (PBTR_FUNCTION_ENTRY)First;
		F2 = (PBTR_FUNCTION_ENTRY)Second;

		if (Context->Column == 1 || Context->Column == 2) {
			Result = F1->Inclusive - F2->Inclusive;
		}

		if (Context->Column == 3 || Context->Column == 4) {
			Result = F1->Exclusive - F2->Exclusive;
		}
	}

	if (Context->Order == SortOrderDescendent) {
		Result = -Result;
	}

	return Result;
}

LRESULT
CpuModuleOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Column;
	TVSORTCB Tsc = {0};
	PTREELIST_OBJECT TreeList;
	PDIALOG_OBJECT Object;
	HTREEITEM hItemGroup;
	TVITEM tvi = {0};
	PCPU_FORM_CONTEXT Context;
	CPU_SORT_CONTEXT SortContext;

	Column = (ULONG)wp;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	TreeList = Context->TreeList;
	
	//
	// Sort dll items
	//

	SortContext.Context = (LPARAM)NULL;
	SortContext.Column = Column;
	SortContext.Order = TreeList->SortOrder;

	Tsc.hParent = TVI_ROOT;
	Tsc.lParam = (LPARAM)&SortContext;
	Tsc.lpfnCompare = CpuModuleCompareCallback;
	TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

	hItemGroup = TreeView_GetChild(TreeList->hWndTree, TVI_ROOT);

	while (hItemGroup != NULL) {
		
		tvi.mask = TVIF_PARAM;
		tvi.hItem = hItemGroup;
		TreeView_GetItem(TreeList->hWndTree, &tvi);

		SortContext.Context = 1;
		SortContext.Column = Column;
		SortContext.Order = TreeList->SortOrder;

		Tsc.hParent = hItemGroup;
		Tsc.lParam = (LPARAM)&SortContext;
		Tsc.lpfnCompare = CpuModuleCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

		hItemGroup = TreeView_GetNextSibling(TreeList->hWndTree, hItemGroup);
	}

	return 0;
}