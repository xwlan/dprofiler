//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "mmmodule.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "treelist.h"
#include "sdk.h"
#include "dialog.h"
#include "profileform.h"

typedef enum _MmModuleColumnType {
	MmModuleName,
	MmModuleCount,
	MmModuleSize,
	MmModuleModule,
	MmModuleColumnCount,
} MmModuleColumnType;

HEADER_COLUMN 
MmModuleColumn[MmModuleColumnCount] = {
	{ {0}, 200, HDF_LEFT, FALSE, FALSE, L" Name" },
	{ {0}, 100,  HDF_LEFT, FALSE, FALSE, L" Count" },
	{ {0}, 120,  HDF_LEFT, FALSE, FALSE, L" Size" },
	{ {0}, 120,  HDF_LEFT, FALSE, FALSE, L" Module" },
};

#define MODULE_TREELIST_ID 1

DIALOG_SCALER_CHILD MmModuleChildren[1] = {
	{ MODULE_TREELIST_ID, AlignRight, AlignBottom}
};

DIALOG_SCALER MmModuleScaler = {
	{0,0}, {0,0}, {0,0}, 1, MmModuleChildren
};

HWND
MmModuleCreate(
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
	Object->ResourceId = IDD_FORMVIEW_MM_MODULE;
	Object->Procedure = MmModuleProcedure;

	return DialogCreateModeless(Object);
}

LRESULT
MmModuleOnInitDialog(
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

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
	
	//
	// Create treelist control
	//

	GetClientRect(hWnd, &Rect);
	TreeList = TreeListCreate(hWnd, TRUE, MODULE_TREELIST_ID, 
							  &Rect, hWnd, 
							  MmModuleFormatCallback, 
							  MmModuleColumnCount);

	for(Number = 0; Number < MmModuleColumnCount; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 MmModuleColumn[Number].Align, 
							 MmModuleColumn[Number].Width, 
							 MmModuleColumn[Number].Name );
	}

	ASSERT(TreeList->hWnd != NULL);

	Context->TreeList = TreeList;

	//
	// Register dialog scaler
	//

	Object->Scaler = &MmModuleScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

VOID
MmModuleInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Report
	)
{
	PTREELIST_OBJECT TreeList;
	TVINSERTSTRUCT tvi = {0};
	HWND hWndTree;
	ULONG Count;
	RECT rect;
	PDIALOG_OBJECT Object;
	PMM_FORM_CONTEXT FormContext;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	FormContext = SdkGetContext(Object, MM_FORM_CONTEXT);

	TreeList = FormContext->TreeList;
	ASSERT(TreeList != NULL);

	hWndTree = TreeList->hWndTree;
	Count = 0;

	GetClientRect(hWnd, &rect);
	InvalidateRect(hWnd, &rect, TRUE);
	UpdateWindow(hWnd);
}

LRESULT 
MmModuleOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmModuleOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
MmModuleOnClose(
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
MmModuleProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return MmModuleOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return MmModuleOnClose(hWnd, uMsg, wp, lp);

	case WM_TREELIST_SORT:
		MmModuleOnTreeListSort(hWnd, uMsg, wp, lp);
		break;
	}

	return Status;
}

VOID CALLBACK
MmModuleFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	)
{
}

int CALLBACK
MmModuleCompareCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
	return 0;
}

LRESULT
MmModuleOnTreeListSort(
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
	HTREEITEM hItemRoot;
	TVITEM tvi = {0};
	PMM_FORM_CONTEXT Context;
	MM_SORT_CONTEXT SortContext;

	Column = (ULONG)wp;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, MM_FORM_CONTEXT);
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
		Tsc.lpfnCompare = MmModuleCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

		hItemGroup = TreeView_GetNextSibling(TreeList->hWndTree, hItemGroup);
	}

	return 0;
}