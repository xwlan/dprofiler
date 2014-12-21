//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "srcdlg.h"
#include "list.h"
#include "listview.h"
#include "apsrpt.h"
#include "main.h"
#include "frame.h"

LISTVIEW_COLUMN SourceColumn[2] = {
	{ 60, L"Line",  LVCFMT_RIGHT,0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 400, L"Code",	LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

DIALOG_SCALER_CHILD SourceChildren[] = {
    { IDC_TAB_FILE, AlignRight, AlignBottom }
};

DIALOG_SCALER SourceScaler = {
	{0,0}, {0, 0}, {0,0},1, SourceChildren
};

HWND
SourceCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	HWND hWnd;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	ZeroMemory(Object, sizeof(*Object));

	Context = (PSOURCE_CONTEXT)SdkMalloc(sizeof(SOURCE_CONTEXT));
	ZeroMemory(Context, sizeof(*Context));

	InitializeListHead(&Context->FileListHead);

	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_DIALOG_SOURCE;
	Object->Procedure = SourceProcedure;
	
	hWnd = DialogCreateModeless(Object);
	return hWnd;
}

BOOLEAN
SourceOpenFile(
	__in HWND hWnd,
	__in PWSTR FullPath,
	__in ULONG Line
	)
{
	PDIALOG_OBJECT Object;
	PFILE_ITEM FileItem;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);

	FileItem = NULL;
	if (SourceIsOpened(hWnd, FullPath, &FileItem)) {
		ASSERT(FileItem != NULL);
		SourceSetTab(Object, FileItem);
		SourceSetLine(Object, FileItem, Line);
		return TRUE;
	}

	//
	// Load the file
	//

	FileItem = SourceLoadFile(Object, FullPath);
	if (!FileItem) {
		return FALSE;
	}

	SourceInsertTab(Object, FileItem);
	SourceSetTab(Object, FileItem);
	SourceSetLine(Object, FileItem, Line);
	return TRUE;
}

VOID
SourceSetTab(
	__in PDIALOG_OBJECT Object,
	__in PFILE_ITEM Item
	)
{
	PSOURCE_CONTEXT Context;
	PLIST_ENTRY ListEntry;
	PFILE_ITEM Entry;
	int Number = 0;

	Context = (PSOURCE_CONTEXT)Object->Context;

	//
	// Hide other tabs, show current tab
	//

	ListEntry = Context->FileListHead.Flink;
	while (ListEntry != &Context->FileListHead) {

		Entry = CONTAINING_RECORD(ListEntry, FILE_ITEM, ListEntry);

		if (Entry != Item) {
			ShowWindow(Entry->hWndList, SW_HIDE);
		}
		else {
			Context->Current = Item;
			TabCtrl_SetCurSel(Context->hWndTab, Number);
			TabCtrl_SetCurFocus(Context->hWndTab, Number);
			SourceAdjustView(Context, Entry);
			ShowWindow(Entry->hWndList, SW_SHOW);
			SetFocus(Entry->hWndList);
			return;
		}

		Number += 1;
		ListEntry = ListEntry->Flink;
	}

}

VOID
SourceSetLine(
	__in PDIALOG_OBJECT Object,
	__in PFILE_ITEM Item,
	__in ULONG Line
	)
{
	LONG Mask;
	
	UNREFERENCED_PARAMETER(Object);

	ASSERT(Item != NULL);
	ASSERT(Item->hWndList != NULL);

	Mask = LVIS_FOCUSED | LVIS_SELECTED;
	ListView_SetItemState(Item->hWndList, Line, Mask, Mask);

	SetFocus(Item->hWndList);
	ListView_EnsureVisible(Item->hWndList, Line, TRUE);
}

BOOLEAN
SourceInsertTab(
	__in PDIALOG_OBJECT Object,
	__in PFILE_ITEM FileItem 
	)
{
	LPTCITEM TabItem;
	PSOURCE_CONTEXT Context;
	RECT rc;
	HWND hWndTab;

	TabItem = &FileItem->Tab;
	TabItem->mask = TCIF_TEXT;

	ApsGetBaseName(FileItem->FullPath, FileItem->BaseName, MAX_PATH);
	TabItem->pszText = FileItem->BaseName;

	Context = (PSOURCE_CONTEXT)Object->Context;
	TabCtrl_InsertItem(Context->hWndTab, Context->FileCount, TabItem);

	//
	// Insert tab into file list 
	//

	InsertTailList(&Context->FileListHead, &FileItem->ListEntry);
	Context->FileCount += 1;

	hWndTab = Context->hWndTab;
	GetClientRect(hWndTab, &rc);
	TabCtrl_AdjustRect(hWndTab, FALSE, &rc);

	MoveWindow(FileItem->hWndList, rc.left, rc.top, rc.right - rc.left,
			   rc.bottom - rc.top, TRUE);
	return TRUE;
}

VOID
SourceAdjustView(
	__in PSOURCE_CONTEXT Context,
	__in PFILE_ITEM FileItem
	)
{
	HWND hWndTab;
	RECT rc;

	ASSERT(FileItem->hWndList != NULL);

	hWndTab = Context->hWndTab;
	GetClientRect(hWndTab, &rc);
	TabCtrl_AdjustRect(hWndTab, FALSE, &rc);

	MoveWindow(FileItem->hWndList, rc.left, rc.top, rc.right - rc.left,
			   rc.bottom - rc.top, TRUE);
}

PFILE_ITEM
SourceCurrentTab(
	__in PDIALOG_OBJECT Object
	)
{
	return NULL;
}

VOID
SourceCloseTab(
	__in PDIALOG_OBJECT Object,
	__in int index
	)
{
	PSOURCE_CONTEXT Context;
	PFILE_ITEM FileItem;

	//
	// Map index to file item
	//

	Context = (PSOURCE_CONTEXT)Object->Context;
	FileItem = SourceIndexToTab(Context, index);
	if (!FileItem) {
		return;
	}

	//
	// Remove tab item
	//

	DestroyWindow(FileItem->hWndList);
	RemoveEntryList(&FileItem->ListEntry);
	Context->FileCount -= 1;
	SdkFree(FileItem);

	TabCtrl_DeleteItem(Context->hWndTab, index);
}

PFILE_ITEM
SourceIndexToTab(
	__in PSOURCE_CONTEXT Context,
	__in int Index
	)
{
	PLIST_ENTRY ListEntry;
	int Number;
	PFILE_ITEM Item;

	Number = 0;
	ListEntry = Context->FileListHead.Flink;
	while (ListEntry != &Context->FileListHead) {
		Item = CONTAINING_RECORD(ListEntry, FILE_ITEM, ListEntry);
		if (Index == Number) {
			return Item;
		}
		ListEntry = ListEntry->Flink;
		Number += 1;
	}

	return NULL;
}

BOOLEAN
SourceIsOpened(
	__in HWND hWnd,
	__in PWSTR FullPath,
	__out PFILE_ITEM *File
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	PLIST_ENTRY ListEntry;
	PFILE_ITEM FileItem;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	ListEntry = Context->FileListHead.Flink;
	while (ListEntry != &Context->FileListHead) {

		FileItem = CONTAINING_RECORD(ListEntry, FILE_ITEM, ListEntry);	
		if (!_wcsicmp(FileItem->FullPath, FullPath)) {
			*File = FileItem;
			return TRUE;
		}

		ListEntry = ListEntry->Flink;
	}

	*File = NULL;
	return FALSE;
}

PFILE_ITEM
SourceLoadFile(
	__in PDIALOG_OBJECT Object,
	__in PWSTR FullPath
	)
{
	FILE *fp;
	PWCHAR Buffer;
	HWND hWndList;
	PFILE_ITEM FileItem;
	ULONG Number = 0;
	LVITEM lvi = {0};
	WCHAR LineBuffer[10];
	LONG Maximum = 0;
	SIZE Size = {0, 0};
	HDC hdc;

	//
	// Open the source file
	//

	if(_wfopen_s(&fp, FullPath, L"rt,ccs=UNICODE") != 0) {
		MessageBox(Object->hWnd, L"Failed to open file!", L"D Profile", MB_ICONERROR|MB_OK);
		return NULL;
	}

	//
	// Create listview to display the source file
	//

	hWndList = SourceCreateView(Object);
	ASSERT(hWndList != NULL);

	hdc = GetDC(hWndList);

	//
	// Read each source line and insert into listview
	//

	Buffer = (PWCHAR)SdkMalloc(PAGESIZE);
	while(!feof(fp)) {

		fgetws(Buffer, PAGESIZE, fp);

		//
		// Line
		//

		lvi.iItem = Number;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(LineBuffer, 10, L"%u", Number);
		lvi.pszText = LineBuffer;
		ListView_InsertItem(hWndList, &lvi);

		//
		// Code 
		//

		lvi.iSubItem = 1;
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);

		Number += 1;

		//
		// Compute the maximum column width
 		//

		GetTextExtentPoint32(hdc, Buffer, (int)wcslen(Buffer), &Size);
		Maximum = max(Size.cx, Maximum);
	}

	//
	// Compute maximum column width with icon spacing
	//

	Maximum = max(Maximum + 6, (LONG)SourceColumn[1].Width);
	ListView_SetColumnWidth(hWndList, 1, Maximum);

	//
	// Close the source file
	//

	SdkFree(Buffer);
	fclose(fp);

	//
	// Allocate new file tab item
	//

	FileItem = (PFILE_ITEM)SdkMalloc(sizeof(FILE_ITEM));
	StringCchCopy(FileItem->FullPath, MAX_PATH, FullPath);
	FileItem->LineCount = Number;
	FileItem->hWndList = hWndList;

	return FileItem; 
}

HWND
SourceCreateView(
	__in PDIALOG_OBJECT Object
	)
{
	PSOURCE_CONTEXT Context;
	HWND hWnd;
	DWORD dwStyle;
	ULONG i;
	LVCOLUMN lvc = {0}; 
	
	Context = (PSOURCE_CONTEXT)Object->Context;

	//
	// Create listview
	//

	dwStyle = WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_CLIPSIBLINGS|\
			  LVS_SINGLESEL|LVS_REPORT|LVS_NOCOLUMNHEADER|LVS_SHOWSELALWAYS;

	hWnd = CreateWindowEx(0, WC_LISTVIEW, 0, dwStyle, 
						  0, 0, 0, 0, Context->hWndTab, 
						  (HMENU)UlongToPtr(Context->FileCount), 
						  SdkInstance, NULL);
	ASSERT(hWnd != NULL);
	
    ListView_SetUnicodeFormat(hWnd, TRUE);
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER); 

	//
	// Insert 2 columns, line number and text
	//

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 

	for(i = 0; i < 2; i++) {
	    lvc.iSubItem = i;
		lvc.pszText = SourceColumn[i].Title;	
		lvc.cx = SourceColumn[i].Width;     
		lvc.fmt = SourceColumn[i].Align;
		ListView_InsertColumn(hWnd, i, &lvc);
	}

	return hWnd;
}

LRESULT
SourceOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HWND hWndTab;
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	LVCOLUMN Column = {0};
	LVITEM Item = {0};
	RECT rc;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	hWndTab = GetDlgItem(hWnd, IDC_TAB_FILE);
	Context->hWndTab = hWndTab;

	GetClientRect(hWnd, &rc);
	MoveWindow(hWndTab, rc.left, rc.top, rc.right - rc.left,
			   rc.bottom - rc.top - 16, FALSE);

	//
	// Subclass tab's window procedure
	//

	SetWindowSubclass(Context->hWndTab, SourceTabProcedure, 0, (DWORD_PTR)Object);

	//
	// Register dialog scaler
	//

	Object->Scaler = &SourceScaler;
	DialogRegisterScaler(Object);
	
	Context->clrText = GetSysColor(COLOR_WINDOWTEXT);
	Context->clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
	Context->hBrushHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);

	SetWindowText(hWnd, L"Source");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	ShowWindow(hWnd, SW_SHOW);
	return TRUE;
}

LRESULT CALLBACK 
SourceTabProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	if (uMsg == WM_NOTIFY) {
		return SourceTabOnNotify(hWnd, uMsg, wp, lp);
	}
	
	if (uMsg == WM_SIZE) {
		return SourceTabOnSize(hWnd, uMsg, wp, lp);
	}

    return DefSubclassProc(hWnd, uMsg, wp, lp);
}

LRESULT
SourceTabOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	RECT rc;
	HWND hWndTab;
	PFILE_ITEM FileItem;

	Object = (PDIALOG_OBJECT)SdkGetObject(GetParent(hWnd));
	Context = (PSOURCE_CONTEXT)Object->Context;

	FileItem = Context->Current;
	ASSERT(FileItem != NULL);

	hWndTab = Context->hWndTab;
	GetClientRect(hWndTab, &rc);
	TabCtrl_AdjustRect(hWndTab, FALSE, &rc);

	MoveWindow(FileItem->hWndList, rc.left, rc.top, rc.right - rc.left,
			   rc.bottom - rc.top, TRUE);
	return TRUE;
}

LRESULT
SourceOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	PFILE_ITEM FileItem;
	PLIST_ENTRY ListEntry;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	//
	// Destroy all open file items
	//

	while(IsListEmpty(&Context->FileListHead) != TRUE) {
		ListEntry = RemoveHeadList(&Context->FileListHead);
		FileItem = CONTAINING_RECORD(ListEntry, FILE_ITEM, ListEntry);	
		SdkFree(FileItem);
	}

	DestroyWindow(hWnd);

	//
	// Notify mainframe that the source window is closed
	//

	PostMessage(MainGetFrame()->hWnd, WM_SOURCE_CLOSE, 0, 0);
	return 0;
}

LRESULT
SourceOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
SourceProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return SourceOnInitDialog(hWnd, uMsg, wp, lp);			
		case WM_CLOSE:
			return SourceOnClose(hWnd, uMsg, wp, lp);
		case WM_NOTIFY:
			return SourceOnNotify(hWnd, uMsg, wp, lp);
		case WM_COMMAND:
			return SourceOnCommand(hWnd, uMsg, wp, lp);
		case WM_SYSCOMMAND:
			return SourceOnSysCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
SourceOnSysCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	switch (wp) {
		case SC_RESTORE:
			ShowWindow(hWnd, SW_SHOWNORMAL);
			break;

	}

	return 0;
}

LRESULT
SourceOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	UINT CommandId;

	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	CommandId = LOWORD(wp);

	switch (CommandId) {

		case ID_SOURCE_CLOSE:
			return SourceOnCloseFile(hWnd, uMsg, wp, lp);

		case ID_SOURCE_OPENFROMSHELL:
			return SourceOnOpenFromShell(hWnd, uMsg, wp, lp);

		case ID_SOURCE_OPENFOLDER:
			return SourceOnOpenFolder(hWnd, uMsg, wp, lp);

		case ID_SOURCE_COPYPATH:
			return SourceOnCopyPath(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
SourceOnCloseFile(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	HWND hWndTab;
	PFILE_ITEM FileItem;
	int index;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	//
	// Get current tab item
	//

	hWndTab = Context->hWndTab;
	index = TabCtrl_GetCurSel(hWndTab);
	SourceCloseTab(Object, index);

	if (!Context->FileCount) {
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		return 0;
	}
	
	if (index == 0) {
		index = 0;
	}
	else if(index == Context->FileCount) {
		index = Context->FileCount - 1;
	}
	else {
		index -= 1;
	}

	//
	// Set new tab item
	//

	FileItem = SourceIndexToTab(Context, index);
	SourceSetTab(Object, FileItem);
	return 0;
}

LRESULT
SourceOnOpenFromShell(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	HWND hWndTab;
	PFILE_ITEM FileItem;
	int index;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	hWndTab = Context->hWndTab;
	index = TabCtrl_GetCurSel(hWndTab);
	FileItem = SourceIndexToTab(Context, index);

	//
	// Open from shell 
	//

	ShellExecute(hWnd, L"open", FileItem->FullPath, NULL, NULL, SW_SHOWNORMAL);
	return 0;
}

LRESULT
SourceOnOpenFolder(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	HWND hWndTab;
	PFILE_ITEM FileItem;
	PWCHAR Ptr;
	WCHAR Buffer[MAX_PATH];
	int index;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	hWndTab = Context->hWndTab;
	index = TabCtrl_GetCurSel(hWndTab);
	FileItem = SourceIndexToTab(Context, index);

	wcscpy_s(Buffer, MAX_PATH, FileItem->FullPath);
	Ptr = wcsrchr(Buffer, L'\\');
	*(Ptr + 1) = 0;
	
	//
	// Open the folder
	//

	ShellExecute(hWnd, L"explore", Buffer, NULL, NULL, SW_SHOWNORMAL);
	return 0;
}

LRESULT
SourceOnCopyPath(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	HWND hWndTab;
	PFILE_ITEM FileItem;
	int index;
	size_t Length;	
	size_t cch;
	HGLOBAL hglbCopy;
	LPTSTR lptstrCopy;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	hWndTab = Context->hWndTab;
	index = TabCtrl_GetCurSel(hWndTab);
	FileItem = SourceIndexToTab(Context, index);

	//
	// Copy full path name to clipboard
	//

	cch = wcslen(FileItem->FullPath);
	Length = (cch + 1) * sizeof(WCHAR);

	hglbCopy = GlobalAlloc(GMEM_MOVEABLE, Length);
	lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
	memcpy(lptstrCopy, FileItem->FullPath, cch * sizeof(WCHAR));
	lptstrCopy[cch] = 0;
	GlobalUnlock(hglbCopy);

	if (OpenClipboard(hWnd)) {
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hglbCopy);
		CloseClipboard();
	}

	return TRUE;
}

LRESULT
SourceOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status;
	LPNMHDR pNmhdr = (LPNMHDR)lp;

	Status = 0;

	switch (pNmhdr->code) {
		
		case TCN_SELCHANGE:
			Status = SourceOnTcnSelChange(hWnd, uMsg, wp, lp);
			break;
		case NM_RCLICK:
			Status = SourceOnContextMenu(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
SourceOnTcnSelChange(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	int Current;
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	PFILE_ITEM Item;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	Current = TabCtrl_GetCurSel(pNmhdr->hwndFrom);
	Item = SourceIndexToTab(Context, Current);
	ASSERT(Item != NULL);

	SourceSetTab(Object, Item);
	return 0;
}

LRESULT
SourceTabOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status;
	LPNMHDR pNmhdr = (LPNMHDR)lp;

	Status = 0;

	switch (pNmhdr->code) {
		
		case NM_CUSTOMDRAW:
			Status = SourceTabOnCustomDraw(hWnd, uMsg, wp, lp);
			break;
		case TTN_GETDISPINFO:
			return SourceOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
SourceTabDrawText(
	__in PSOURCE_CONTEXT Context,
	__in HWND hWndList,
	__in LPNMLVCUSTOMDRAW lvcd
	)
{
	WCHAR Buffer[MAX_PATH];
	LRESULT Status = CDRF_DODEFAULT;

	ListView_GetItemText(hWndList, lvcd->nmcd.dwItemSpec, 
						 1, Buffer, MAX_PATH);

	if(lvcd->nmcd.uItemState & CDIS_FOCUS) {
		FillRect(lvcd->nmcd.hdc, &lvcd->nmcd.rc, Context->hBrushHighlight);
		SetTextColor(lvcd->nmcd.hdc, Context->clrTextHighlight);
	}

	DrawText(lvcd->nmcd.hdc, Buffer, -1, &lvcd->nmcd.rc, DT_SINGLELINE|DT_EXPANDTABS);
	return Status;
}

LRESULT
SourceTabOnCustomDraw(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	LRESULT Status = CDRF_DODEFAULT;
	LPNMHDR pNmhdr = (LPNMHDR)lp; 
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(GetParent(hWnd));
	ASSERT(Object != NULL);

	Context = (PSOURCE_CONTEXT)Object->Context;
	ASSERT(Context != NULL);

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT ;
            break;
        
        case CDDS_SUBITEM|CDDS_ITEMPREPAINT: 

			if (lvcd->iSubItem == 0) { 
				lvcd->clrTextBk = RGB(220, 220, 220);
				Status = CDRF_NEWFONT;
			} 
			else {

				//
				// Only require erase background, we draw text when
				// receive CDDS_ITEMPOSTPAINT
				//

				lvcd->clrTextBk = RGB(255, 255, 255);
				Status = CDRF_NEWFONT|CDRF_DOERASE|CDRF_NOTIFYPOSTPAINT;
			}
			break;
        
		case CDDS_SUBITEM|CDDS_ITEMPOSTPAINT:
			if (lvcd->iSubItem != 0) {
				SourceTabDrawText(Context, pNmhdr->hwndFrom, lvcd);
				Status = CDRF_SKIPDEFAULT;
			}
			break;

        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

LRESULT
SourceOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	POINT pt;
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	HMENU hPopupMenu = NULL;
	HMENU hMenuLoaded;
	PFILE_ITEM FileItem;
	TCHITTESTINFO info;
	int index;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSOURCE_CONTEXT)Object->Context;

	GetCursorPos(&pt);

	//
	// Map the cursor to tab coordinates and determine
	// which tab is hit
	//

	info.pt = pt;
	info.flags = 0;
	MapWindowPoints(HWND_DESKTOP, Context->hWndTab, &info.pt, 1);

	index = TabCtrl_HitTest(Context->hWndTab, &info);
	if (index == -1) {
		return 0;
	}

	//
	// Switch to the new tab if hit change to another tab
	//

	if (index != TabCtrl_GetCurSel(Context->hWndTab)) {
		FileItem = SourceIndexToTab(Context, index);
		SourceSetTab(Object, FileItem);
	}

	//
	// Display the context menu
	//

	hMenuLoaded = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_SOURCE)); 
	hPopupMenu = GetSubMenu(hMenuLoaded, 0);

	//
	// The menu command must be routed to dialog, not tabctrl
	//

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,               
				   pt.x, pt.y, 0, GetParent(Context->hWndTab), NULL); 
	DestroyMenu(hMenuLoaded);	
	return 0;
}

LRESULT
SourceOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFILE_ITEM FileItem;
	PDIALOG_OBJECT Object;
	PSOURCE_CONTEXT Context;
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;

	Object = (PDIALOG_OBJECT)SdkGetObject(GetParent(hWnd));
	Context = (PSOURCE_CONTEXT)Object->Context;

	//
	// hdr.idFrom is the tab index
	//

	FileItem = SourceIndexToTab(Context, lpnmtdi->hdr.idFrom);
	lpnmtdi->lpszText = FileItem->FullPath;

	return 0;
}