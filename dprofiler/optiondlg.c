//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
// 

#include "sdk.h"
#include "dialog.h"
#include "listview.h"
#include "optiondlg.h"
#include "mdb.h"
#include "aps.h"

#define FLAG_OPTION_NEW  1
#define FLAG_OPTION_OLD  0

VOID
OptionDialog(
    __in HWND hWndParent	
    )
{
    DIALOG_OBJECT Object = {0};
    OPTION_CONTEXT Context = {0};

    Object.Context = &Context;
    Object.hWndParent = hWndParent;
    Object.ResourceId = IDD_DIALOG_OPTION;
    Object.Procedure = OptionProcedure;

    DialogCreate(&Object);
}

INT_PTR CALLBACK
OptionProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    LRESULT Result = 0;

    switch (uMsg) {

    case WM_INITDIALOG:
        return OptionOnInitDialog(hWnd, uMsg, wp, lp);			

    case WM_COMMAND:
		return OptionOnCommand(hWnd, uMsg, wp, lp);

    case WM_NOTIFY:
        return OptionOnNotify(hWnd, uMsg, wp, lp);

    }

    return Result;
}

LRESULT
OptionOnInitDialog(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    LRESULT Result;
    PDIALOG_OBJECT Object;
    POPTION_CONTEXT Context;
    HWND hWndPage;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (POPTION_CONTEXT)Object->Context;

	//
	// Hide the placeholder static control
	//

	ShowWindow(GetDlgItem(hWnd, IDC_STATIC), SW_HIDE);

    //
    // Create solid white brush to paint background of each pane its children's background
    //

    Context->hBrushPane = CreateSolidBrush(RGB(255, 255, 255));

	//
	// General
	//

	Context->Pages[OPTION_GENERAL].Procedure = OptionGeneralProcedure;
    Context->Pages[OPTION_GENERAL].hWndParent = hWnd;
    Context->Pages[OPTION_GENERAL].ResourceId = IDD_OPTION_GENERAL;
    hWndPage = DialogCreateModeless(&Context->Pages[OPTION_GENERAL]);
    ShowWindow(hWndPage, SW_HIDE);

	//
	// Symbol
	//

    Context->Pages[OPTION_SYMBOL_GENERAL].Procedure = OptionSymbolProcedure;
    Context->Pages[OPTION_SYMBOL_GENERAL].hWndParent = hWnd;
    Context->Pages[OPTION_SYMBOL_GENERAL].ResourceId = IDD_OPTION_SYMBOL_GENERAL;
    hWndPage = DialogCreateModeless(&Context->Pages[OPTION_SYMBOL_GENERAL]);
    ShowWindow(hWndPage, SW_HIDE);

	//
	// Source
	//

    Context->Pages[OPTION_SOURCE_GENERAL].Procedure = OptionSourceProcedure;
    Context->Pages[OPTION_SOURCE_GENERAL].hWndParent = hWnd;
    Context->Pages[OPTION_SOURCE_GENERAL].ResourceId = IDD_OPTION_SOURCE_GENERAL;
    hWndPage = DialogCreateModeless(&Context->Pages[OPTION_SOURCE_GENERAL]);
    ShowWindow(hWndPage, SW_HIDE);

    SdkSetMainIcon(hWnd);
    SdkCenterWindow(hWnd);

    Result = OptionCreateTree(Object);
    return Result;
}

LRESULT
OptionOnCommand(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	switch (LOWORD(wp)) {

		case IDOK:
			OptionCommitData(hWnd);
			EndDialog(hWnd, IDOK);
			break;

		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			break;
	}

	return 0L;
}

LRESULT
OptionCreateTree(
    __in PDIALOG_OBJECT Object
    )
{
    HWND hWnd;
    HWND hWndTree;
    TVINSERTSTRUCT Item = {0};
    HTREEITEM hTreeItem;
    HTREEITEM hItemRoot;
    POPTION_CONTEXT Context;

    Context = (POPTION_CONTEXT)Object->Context;

    hWnd = Object->hWnd;
    hWndTree = GetDlgItem(hWnd, IDC_TREE_OPTION);

	//
    // General 
    //

    Item.hParent = 0; 
    Item.hInsertAfter = TVI_ROOT; 
    Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
    Item.item.pszText = L"General";
    Item.item.lParam = (LPARAM)&Context->Pages[OPTION_GENERAL];
    hTreeItem = TreeView_InsertItem(hWndTree, &Item);
    hItemRoot = hTreeItem;

	Item.hParent = hTreeItem; 
    Item.hInsertAfter = hTreeItem; 
    Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
    Item.item.pszText = L"General";
    Item.item.lParam = (LPARAM)&Context->Pages[OPTION_GENERAL];
    hTreeItem = TreeView_InsertItem(hWndTree, &Item);

    //
    // Symbol
    //

    Item.hParent = 0; 
    Item.hInsertAfter = TVI_ROOT; 
    Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
    Item.item.pszText = L"Symbol";
    Item.item.lParam = (LPARAM)&Context->Pages[OPTION_SYMBOL_GENERAL];
    hTreeItem = TreeView_InsertItem(hWndTree, &Item);

    Item.hParent = hTreeItem; 
    Item.hInsertAfter = hTreeItem; 
    Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
    Item.item.pszText = L"General";
    Item.item.lParam = (LPARAM)&Context->Pages[OPTION_SYMBOL_GENERAL];
    hTreeItem = TreeView_InsertItem(hWndTree, &Item);

	//
	// Source
	//

    Item.hParent = 0; 
    Item.hInsertAfter = TVI_ROOT; 
    Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
    Item.item.pszText = L"Source";
    Item.item.lParam = (LPARAM)&Context->Pages[OPTION_SOURCE_GENERAL];
    hTreeItem = TreeView_InsertItem(hWndTree, &Item);

    Item.hParent = hTreeItem; 
    Item.hInsertAfter = hTreeItem; 
    Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
    Item.item.pszText = L"General";
    Item.item.lParam = (LPARAM)&Context->Pages[OPTION_SOURCE_GENERAL];
    hTreeItem = TreeView_InsertItem(hWndTree, &Item);

    //
    // Focus on root item
    //

    TreeView_SelectItem(hWndTree, hItemRoot);
    return TRUE;
}

LRESULT
OptionOnNotify(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    NMHDR *lpnmhdr = (NMHDR *)lp;
    HWND hWndTree;
    TVITEMEX Item = {0};

    hWndTree = GetDlgItem(hWnd, IDC_TREE_OPTION);

    if (IDC_TREE_OPTION == LOWORD(wp)) {
        switch (lpnmhdr->code)  {
        case TVN_SELCHANGED:
            return OptionOnSelChanged(hWnd, (LPNMTREEVIEW)lp);
        }
    }

    return 0;
}

LRESULT
OptionOnSelChanged(
    __in HWND hWnd,
    __in LPNMTREEVIEW lp
    )
{
    TVITEM Item = {0}; 
    PDIALOG_OBJECT PageObject;

    ASSERT(lp->itemNew.mask & TVIF_PARAM);

    if (lp->itemNew.mask & TVIF_PARAM) {

        if (lp->itemOld.lParam) {
            PageObject = (PDIALOG_OBJECT)lp->itemOld.lParam;
            ShowWindow(PageObject->hWnd, SW_HIDE);
        }

        PageObject = (PDIALOG_OBJECT)lp->itemNew.lParam;
        ShowWindow(PageObject->hWnd, SW_SHOW);

        OptionPageAdjustPosition(hWnd, PageObject->hWnd);
    }

    return 0;
}

VOID 
OptionPageAdjustPosition(
    __in HWND hWndParent,
    __in HWND hWndPage
    )
{
    HWND hWndPos;
    HWND hWndDesktop;
    RECT Rect;

    hWndPos = GetDlgItem(hWndParent, IDC_STATIC);
    GetWindowRect(hWndPos, &Rect);

    hWndDesktop = GetDesktopWindow();
    MapWindowPoints(hWndDesktop, hWndParent, (LPPOINT)&Rect, 2);

    MoveWindow(hWndPage, Rect.left, Rect.top, 
               Rect.right - Rect.left, 
               Rect.bottom - Rect.top, TRUE);
}

VOID
OptionCommitData(
	__in HWND hWnd
	)
{
    PDIALOG_OBJECT Object;
    POPTION_CONTEXT Context;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (POPTION_CONTEXT)Object->Context;

	//
	// Commit data in turn
	//

	OptionGeneralCommitData(Context->Pages[OPTION_GENERAL].hWnd);
	OptionSymbolCommitData(Context->Pages[OPTION_SYMBOL_GENERAL].hWnd);
	OptionSourceCommitData(Context->Pages[OPTION_SOURCE_GENERAL].hWnd);
}
	
INT_PTR CALLBACK
OptionSymbolProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    LRESULT Result = 0;

    switch (uMsg) {

    case WM_INITDIALOG:
        Result = OptionSymbolOnInitDialog(hWnd, uMsg, wp, lp);

    case WM_COMMAND:
        Result = OptionSymbolOnCommand(hWnd, uMsg, wp, lp);

    case WM_NOTIFY:
        Result = OptionSymbolOnNotify(hWnd, uMsg, wp, lp);

    }

    return Result;
}

LRESULT
OptionSymbolOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    //
    // Add symbol path
    //

    if (LOWORD(wp) == IDC_BUTTON_SYMBOL_ADD){
        return OptionSymbolOnAdd(hWnd);
    }

    //
    // Remove symbol path
    //

    if (LOWORD(wp) == IDC_BUTTON_SYMBOL_REMOVE){
        return OptionSymbolOnRemove(hWnd);
    }

    return 0;
}

LRESULT
OptionSymbolOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    NMHDR *lpnmhdr = (NMHDR *)lp;
    HWND hWndList;

    hWndList = GetDlgItem(hWnd, IDC_LIST_SYMBOL);

    if (IDC_LIST_SYMBOL == LOWORD(wp)) {
        switch (lpnmhdr->code)  {
        case LVN_BEGINLABELEDIT:
            return FALSE;
        case LVN_ENDLABELEDIT:
			return OptionSymbolOnEndLabelEdit(hWndList, (NMLVDISPINFO *)lp);
        }
    }

    return 0;
}

LRESULT
OptionSymbolOnEndLabelEdit(
    __in HWND hWndList, 
    __in NMLVDISPINFO *lpdi 
    )
{
	PWCHAR Ptr;
	LVITEM lvi = {0};
    NMHDR *lphdr = (NMHDR *)lpdi;

	//
	// If it's empty string, reject edit
	//

	if (!lpdi->item.pszText) {
		if (lpdi->item.lParam == FLAG_OPTION_NEW) {
			ListView_DeleteItem(hWndList, lpdi->item.iItem);
		}
		return FALSE;
	}


	//
	// If it does not include a '\', it's not a path string,
	// reject it
	//

	Ptr = wcschr(lpdi->item.pszText, L'\\');
	if (!Ptr) {

		//
		// If it's a new added item, delete it
		//

		if (lpdi->item.lParam == FLAG_OPTION_NEW) {
			ListView_DeleteItem(hWndList, lpdi->item.iItem);
		}
		return FALSE;
	}

	if (lpdi->item.lParam == FLAG_OPTION_NEW) {

		//
		// Mark the item as old, otherwise it will be deleted
		//

		lvi.mask = LVIF_PARAM;
		lvi.iItem = lpdi->item.iItem;
		lvi.lParam = (LPARAM)FLAG_OPTION_OLD;
		ListView_SetItem(hWndList, &lvi);
	}

    return TRUE;
}


LRESULT
OptionSymbolOnAdd(
    __in HWND hWnd
    )
{
    HWND hWndCtrl;
    LVITEM Item = {0};
    int index;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_SYMBOL);

    Item.mask = LVIF_TEXT|LVIF_PARAM;
	Item.iItem = ListView_GetItemCount(hWndCtrl);
	Item.iSubItem = 0;
	Item.pszText = L"";
	Item.lParam = (LPARAM)FLAG_OPTION_NEW;
	index = ListView_InsertItem(hWndCtrl, &Item);

    //
    // In place edit the item, focus must be set
    //

    SetFocus(hWndCtrl);
    ListView_EditLabel(hWndCtrl, index);
    return 0;
}

LRESULT
OptionSymbolOnRemove(
    __in HWND hWnd
    )
{
    HWND hWndCtrl;
    int index;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_SYMBOL);
	index = ListViewGetFirstSelected(hWndCtrl);
	if (index != -1) {
		ListView_DeleteItem(hWndCtrl, index);
	}

    return 0;
}

LRESULT
OptionSymbolOnInitDialog(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    HWND hWndList;
    LVCOLUMN lvc = {0};
	RECT Rect;
	
    hWndList = GetDlgItem(hWnd, IDC_LIST_SYMBOL);

	ListView_SetUnicodeFormat(hWndList, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, 
		                                LVS_EX_FULLROWSELECT);

	GetClientRect(hWndList, &Rect);

    lvc.mask = LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
    lvc.iSubItem = 0;
	lvc.cx = Rect.right - Rect.left - 2;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hWndList, 0, &lvc);

	//
	// Fill data
	//

	OptionSymbolFillData(hWnd);
    OptionPageAdjustPosition(GetParent(hWnd), hWnd);
    return TRUE;
}

VOID
OptionSymbolFillData(
	__in HWND hWnd
	)
{
	PMDB_DATA_ITEM Mdi;
	HWND hWndList;
	LIST_ENTRY ListHead;
	PSTRING_ENTRY StringEntry;
	PLIST_ENTRY ListEntry;
	UINT Enable;
	ULONG Count;
	LVITEM lvi = {0};
	PWSTR Unicode;

	hWndList = GetDlgItem(hWnd, IDC_LIST_SYMBOL);

	//
	// Fill data
	//

	Mdi = MdbGetData(MdbSymbolPath);
	ASSERT(Mdi != NULL);

	InitializeListHead(&ListHead);
	Count = ApsSplitAnsiString(Mdi->Value, ';',  &ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		StringEntry = CONTAINING_RECORD(ListEntry, STRING_ENTRY, ListEntry);
		ApsConvertAnsiToUnicode(StringEntry->Buffer, &Unicode);

		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.iItem = ListView_GetItemCount(hWndList);
		lvi.iSubItem = 0;
		lvi.pszText = Unicode;
		lvi.lParam = (LPARAM)FLAG_OPTION_OLD;
		ListView_InsertItem(hWndList, &lvi);

		ApsFree(Unicode);
		ApsFree(StringEntry->Buffer);
		ApsFree(StringEntry);
	}

	//
	// Whether use MS symbol server
	//

	Mdi = MdbGetData(MdbMsSymbolServer);
	Enable = (UINT)atoi(Mdi->Value);
	CheckDlgButton(hWnd, IDC_CHECK_MS_SYMBOL_SERVER, Enable);
	
#ifdef _LITE
	//
	// Hide symbol server option
	//
	ShowWindow(GetDlgItem(hWnd, IDC_CHECK_MS_SYMBOL_SERVER), SW_HIDE);
#endif

}

VOID
OptionSymbolCommitData(
	__in HWND hWnd
	)
{
	HWND hWndList;
	UINT Enable;
	LVITEM lvi = {0};
	int Count;
	int i;
	PSTR Ansi;
	PSTR Buffer;
	PWSTR Text;

	hWndList = GetDlgItem(hWnd, IDC_LIST_SYMBOL);
	Count = ListView_GetItemCount(hWndList);

	//
	// N.B. Allocate a big buffer, if it's overflow,
	// let it be!
	//

	Buffer = (PSTR)ApsMalloc(1024 * 64);
	Text = (PWSTR)ApsMalloc(1024 * 4);

	for(i = 0; i < Count; i++) {

		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = Text;
		lvi.cchTextMax = 1024 * 2;
		ListView_GetItem(hWndList, &lvi);

		ApsConvertUnicodeToAnsi(lvi.pszText, &Ansi);
		strcat(Buffer, Ansi);

		if (i != Count - 1) {

			//
			// skip the last one
			//

			strcat(Buffer, ";");
		}

		ApsFree(Ansi);
	}

	MdbSetData(MdbSymbolPath, Buffer);

	//
	// Set MS symbol server
	//

	Enable = IsDlgButtonChecked(hWnd, IDC_CHECK_MS_SYMBOL_SERVER);
	itoa(Enable, Buffer, 10);
	MdbSetData(MdbMsSymbolServer, Buffer);

	//
	// Finally release the buffer
	//

	ApsFree(Text);
	ApsFree(Buffer);
}

VOID
OptionSourceFillData(
	__in HWND hWnd
	)
{
	PMDB_DATA_ITEM Mdi;
	HWND hWndList;
	LIST_ENTRY ListHead;
	PSTRING_ENTRY StringEntry;
	PLIST_ENTRY ListEntry;
	ULONG Count;
	LVITEM lvi = {0};
	PWSTR Unicode;

	hWndList = GetDlgItem(hWnd, IDC_LIST_SOURCE);

	//
	// Fill data
	//

	Mdi = MdbGetData(MdbSourcePath);
	ASSERT(Mdi != NULL);

	InitializeListHead(&ListHead);
	Count = ApsSplitAnsiString(Mdi->Value, ';',  &ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		StringEntry = CONTAINING_RECORD(ListEntry, STRING_ENTRY, ListEntry);
		ApsConvertAnsiToUnicode(StringEntry->Buffer, &Unicode);

		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.iItem = ListView_GetItemCount(hWndList);
		lvi.iSubItem = 0;
		lvi.pszText = Unicode;
		lvi.lParam = (LPARAM)FLAG_OPTION_OLD;
		ListView_InsertItem(hWndList, &lvi);

		ApsFree(Unicode);
		ApsFree(StringEntry->Buffer);
		ApsFree(StringEntry);
	}
}

INT_PTR CALLBACK
OptionSourceProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    LRESULT Result = 0;

    switch (uMsg) {

    case WM_INITDIALOG:
        Result = OptionSourceOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
        Result = OptionSourceOnCommand(hWnd, uMsg, wp, lp);

    case WM_NOTIFY:
        Result = OptionSourceOnNotify(hWnd, uMsg, wp, lp);

	}

    return Result;
}

LRESULT
OptionSourceOnInitDialog(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    HWND hWndList;
    LVCOLUMN lvc = {0};
	RECT Rect;

    hWndList = GetDlgItem(hWnd, IDC_LIST_SOURCE);

	ListView_SetUnicodeFormat(hWndList, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, 
		                                LVS_EX_FULLROWSELECT);

	GetClientRect(hWndList, &Rect);

    lvc.mask = LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
    lvc.iSubItem = 0;
	lvc.cx = Rect.right - Rect.left - 2;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hWndList, 0, &lvc);

	//
	// Fill data
	//

	OptionSourceFillData(hWnd);
    return TRUE;
}

LRESULT
OptionSourceOnAdd(
    __in HWND hWnd
    )
{
    HWND hWndCtrl;
    LVITEM Item = {0};
    int index;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_SOURCE);

    Item.mask = LVIF_TEXT|LVIF_PARAM;
	Item.iItem = ListView_GetItemCount(hWndCtrl);
	Item.iSubItem = 0;
	Item.pszText = L"";
	Item.lParam = (LPARAM)FLAG_OPTION_NEW;
	index = ListView_InsertItem(hWndCtrl, &Item);

    //
    // In place edit the item, focus must be set
    //

    SetFocus(hWndCtrl);
    ListView_EditLabel(hWndCtrl, index);
    return 0;
}

LRESULT
OptionSourceOnRemove(
    __in HWND hWnd
    )
{
    HWND hWndCtrl;
    int index;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_SOURCE);
	index = ListViewGetFirstSelected(hWndCtrl);
	if (index != -1) {
		ListView_DeleteItem(hWndCtrl, index);
	}

    return 0;
}

LRESULT
OptionSourceOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
	)
{
    if (LOWORD(wp) == IDC_BUTTON_SOURCE_ADD){
        return OptionSourceOnAdd(hWnd);
    }

    if (LOWORD(wp) == IDC_BUTTON_SOURCE_REMOVE){
        return OptionSourceOnRemove(hWnd);
    }

    return 0;
}

LRESULT
OptionSourceOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    NMHDR *lpnmhdr = (NMHDR *)lp;
    HWND hWndList;

    hWndList = GetDlgItem(hWnd, IDC_LIST_SOURCE);

    if (IDC_LIST_SOURCE == LOWORD(wp)) {
        switch (lpnmhdr->code)  {
        case LVN_BEGINLABELEDIT:
            return FALSE;
        case LVN_ENDLABELEDIT:
			return OptionSourceOnEndLabelEdit(hWndList, (NMLVDISPINFO *)lp);
        }
    }

    return 0;
}

LRESULT
OptionSourceOnEndLabelEdit(
    __in HWND hWndList, 
    __in NMLVDISPINFO *lpdi 
    )
{
	PWCHAR Ptr;
	LVITEM lvi = {0};
    NMHDR *lphdr = (NMHDR *)lpdi;

	//
	// If it's empty string, reject edit
	//

	if (!lpdi->item.pszText) {
		if (lpdi->item.lParam == FLAG_OPTION_NEW) {
			ListView_DeleteItem(hWndList, lpdi->item.iItem);
		}
		return FALSE;
	}


	//
	// If it does not include a '\', it's not a path string,
	// reject it
	//

	Ptr = wcschr(lpdi->item.pszText, L'\\');
	if (!Ptr) {

		//
		// If it's a new added item, delete it
		//

		if (lpdi->item.lParam == FLAG_OPTION_NEW) {
			ListView_DeleteItem(hWndList, lpdi->item.iItem);
		}
		return FALSE;
	}

	if (lpdi->item.lParam == FLAG_OPTION_NEW) {

		//
		// Mark the item as old, otherwise it will be deleted
		//

		lvi.mask = LVIF_PARAM;
		lvi.iItem = lpdi->item.iItem;
		lvi.lParam = (LPARAM)FLAG_OPTION_OLD;
		ListView_SetItem(hWndList, &lvi);
	}

    return TRUE;
}

VOID
OptionSourceCommitData(
	__in HWND hWnd
	)
{
	HWND hWndList;
	LVITEM lvi = {0};
	int Count;
	int i;
	PSTR Ansi;
	PSTR Buffer;
	PWSTR Text;

	hWndList = GetDlgItem(hWnd, IDC_LIST_SOURCE);
	Count = ListView_GetItemCount(hWndList);

	//
	// N.B. Allocate a big buffer, if it's overflow,
	// let it be!
	//

	Buffer = (PSTR)ApsMalloc(1024 * 64);
	Text = (PWSTR)ApsMalloc(1024 * 4);

	for(i = 0; i < Count; i++) {

		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = Text;
		lvi.cchTextMax = 1024 * 2;
		ListView_GetItem(hWndList, &lvi);

		ApsConvertUnicodeToAnsi(lvi.pszText, &Ansi);
		strcat(Buffer, Ansi);

		if (i != Count - 1) {

			//
			// skip the last one
			//

			strcat(Buffer, ";");
		}

		ApsFree(Ansi);
	}

	MdbSetData(MdbSourcePath, Buffer);

	//
	// Finally release the buffer
	//

	ApsFree(Text);
	ApsFree(Buffer);
}

INT_PTR CALLBACK
OptionGeneralProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    LRESULT Result = 0;

    switch (uMsg) {

    case WM_INITDIALOG:
        Result = OptionGeneralOnInitDialog(hWnd, uMsg, wp, lp);

    case WM_COMMAND:
        Result = OptionGeneralOnCommand(hWnd, uMsg, wp, lp);

    case WM_NOTIFY:
        Result = OptionGeneralOnNotify(hWnd, uMsg, wp, lp);

    }

    return Result;
}

LRESULT
OptionGeneralOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	OptionGeneralFillData(hWnd);
    OptionPageAdjustPosition(GetParent(hWnd), hWnd);
    return TRUE;
}

LRESULT
OptionGeneralOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
	)
{
	return 0;
}

LRESULT
OptionGeneralOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
	return 0;
}

VOID
OptionGeneralFillData(
	__in HWND hWnd
	)
{
	PMDB_DATA_ITEM Mdi;
	int Value;

	Mdi = MdbGetData(MdbAutoAnalyze);
	ASSERT(Mdi != NULL);

	Value = atoi(Mdi->Value);
	Value = Value ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_AUTOANALYZE, Value);
}

VOID
OptionGeneralCommitData(
	__in HWND hWnd
	)
{
	int Value;
	CHAR Buffer[8];

	Value = IsDlgButtonChecked(hWnd, IDC_CHECK_AUTOANALYZE);

	Value = (Value == BST_CHECKED) ? 1 : 0;
	itoa(Value, Buffer, 10);

	MdbSetData(MdbAutoAnalyze, Buffer);
}