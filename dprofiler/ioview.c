#include "sdk.h"
#include "dialog.h"
#include "ioview.h"

//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "ioview.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"

typedef enum _IoNodeType {
	IO_NODE_INVALID,
	IO_NODE_BACKTRACE,
} IoNodeType;

typedef enum _IoColumnType {
	IoName,
	IoCount,
	IoSize,
	IoModule,
	IoColumnCount,
} IoColumnType;

HEADER_COLUMN 
IoViewColumn[IoColumnCount] = {
	{ {0}, 200, HDF_LEFT, FALSE, FALSE, L" Name" },
	{ {0}, 100,  HDF_LEFT, FALSE, FALSE, L" Count" },
	{ {0}, 120,  HDF_LEFT, FALSE, FALSE, L" Size" },
	{ {0}, 120,  HDF_LEFT, FALSE, FALSE, L" Module" },
};

#define IO_TREELIST_ID 1

DIALOG_SCALER_CHILD IoChildren[1] = {
	{ IO_TREELIST_ID, AlignRight, AlignBottom}
};

DIALOG_SCALER IoScaler = {
	{0,0}, {0,0}, {0,0}, 1, IoChildren
};

typedef struct _IO_FORM_CONTEXT {
	PTREELIST_OBJECT TreeList;
	UINT_PTR CtrlId;
	PPF_REPORT_HEAD Head;
	WCHAR Path[MAX_PATH];
} IO_FORM_CONTEXT, *PIO_FORM_CONTEXT;

HWND
IoFormCreate(
	__in HWND hWndParent,
	__in UINT_PTR Id
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	
	Context = (PIO_FORM_CONTEXT)SdkMalloc(sizeof(IO_FORM_CONTEXT));
	Context->CtrlId = Id;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = NULL;
	Object->ResourceId = IDD_FORMVIEW_IO;
	Object->Procedure = IoViewProcedure;

	return DialogCreateModeless(Object);
}

LRESULT
IoViewOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PTREELIST_OBJECT TreeList;
	PIO_FORM_CONTEXT Context;
	PDIALOG_OBJECT Object;
	ULONG Number;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	
	//
	// Create treelist control
	//

	GetClientRect(hWnd, &Rect);
	TreeList = TreeListCreate(hWnd, TRUE, IO_TREELIST_ID, 
							  &Rect, hWnd, 
							  IoFormatCallback, 
							  IoColumnCount);

	for(Number = 0; Number < IoColumnCount; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 IoViewColumn[Number].Align, 
							 IoViewColumn[Number].Width, 
							 IoViewColumn[Number].Name );
	}

	ASSERT(TreeList->hWnd != NULL);

	Context->TreeList = TreeList;

	//
	// Register dialog scaler
	//

	Object->Scaler = &IoScaler;
	DialogRegisterScaler(Object);

	IoInsertData(hWnd);
	return TRUE;
}

HTREEITEM
IoInsertLevel1Data(
	__in HWND hWndTree,
	__in HTREEITEM hTreeItem
	);

HTREEITEM
IoInsertLevel2Data(
	__in HWND hWndTree,
	__in HTREEITEM hTreeItem
	);

VOID
IoInsertData(
	__in HWND hWnd
	)
{
	ULONG i;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItem;
	PIO_FORM_CONTEXT  Context;
	PDIALOG_OBJECT Object;
	PTREELIST_OBJECT TreeList;
	HWND hWndTree;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	TreeList = Context->TreeList;
	hWndTree = TreeList->hWndTree;

	for(i = 0; i < 10; i++) {

		tvi.hParent = TVI_ROOT; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"Level 0";
		tvi.item.lParam = (LPARAM)L"Level 0"; 
		hItem = TreeView_InsertItem(hWndTree, &tvi);	

		IoInsertLevel1Data(hWndTree, hItem);
	}
}

HTREEITEM
IoInsertLevel1Data(
	__in HWND hWndTree,
	__in HTREEITEM hTreeItem
	)
{
	ULONG i;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItem;

	for(i = 0; i < 10; i++) {

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"Level 1";
		tvi.item.lParam = (LPARAM)L"Level 1"; 
		hItem = TreeView_InsertItem(hWndTree, &tvi);	

		IoInsertLevel2Data(hWndTree, hItem);
	}

	return hItem;
}

HTREEITEM
IoInsertLevel2Data(
	__in HWND hWndTree,
	__in HTREEITEM hTreeItem
	)
{
	ULONG i;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItem;

	for(i = 0; i < 10; i++) {

		tvi.hParent = hTreeItem; 
		tvi.hInsertAfter = TVI_LAST; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"Level 2";
		tvi.item.lParam = (LPARAM)L"Level 2"; 
		hItem = TreeView_InsertItem(hWndTree, &tvi);	

	}

	return hItem;
}

LRESULT 
IoOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
IoOnClose(
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
IoViewProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return IoViewOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return IoOnClose(hWnd, uMsg, wp, lp);

	case WM_TREELIST_SORT:
		IoOnTreeListSort(hWnd, uMsg, wp, lp);
		break;
	}

	return Status;
}

VOID CALLBACK
IoFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	)
{
	StringCchCopy(Buffer, Length, L"TreeList");
}

typedef struct _IO_SORT_CONTEXT {
	LPARAM Context;
	LIST_SORT_ORDER Order;
	ULONG Column;
} IO_SORT_CONTEXT, *PIO_SORT_CONTEXT;

int CALLBACK
IoCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	return 0;
}

LRESULT
IoOnTreeListSort(
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
	HTREEITEM hItemRoot;
	TVITEM tvi = {0};
	PIO_FORM_CONTEXT Context;
	IO_SORT_CONTEXT SortContext;

	Column = (ULONG)wp;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	TreeList = Context->TreeList;
	
	hItemRoot = TreeView_GetRoot(TreeList->hWndTree);
	hItemGroup = TreeView_GetChild(TreeList->hWndTree, hItemRoot);

	while (hItemGroup != NULL) {
		
		tvi.mask = TVIF_PARAM;
		tvi.hItem = hItemGroup;
		TreeView_GetItem(TreeList->hWndTree, &tvi);

		SortContext.Context = tvi.lParam;
		SortContext.Column = Column;
		SortContext.Order = TreeList->SortOrder;

		Tsc.hParent = hItemGroup;
		Tsc.lParam = (LPARAM)&SortContext;
		Tsc.lpfnCompare = IoCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

		hItemGroup = TreeView_GetNextSibling(TreeList->hWndTree, hItemGroup);
	}

	return 0;
}