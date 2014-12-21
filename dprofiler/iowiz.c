//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "wizard.h"
#include "iowiz.h"
#include "listview.h"


#define GET_CONTEXT(_H) \
	((PWIZARD_CONTEXT)GetWindowLongPtr(_H, GWL_USERDATA))

LISTVIEW_COLUMN IoColumn[3] = {
	{ 120, L"Name", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"PID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeUInt },
	{ 360, L"Path", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

VOID
IoWizard(
	__in HWND hWndParent,
	__in PWIZARD_CONTEXT Context
	)
{
	PROPSHEETPAGE   psp = {0}; 
    HPROPSHEETPAGE  hpsp[4] = {0}; 
    PROPSHEETHEADER psh = {0}; 
    NONCLIENTMETRICS ncm = {0};
    LOGFONT TitleLogFont;
    HDC hdc; 
    INT FontSize;

    //
	// I/O 
	//

	psp.pszTitle = L"D Profile";
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE;
    psp.pszHeaderTitle = L"I/O Profiling";
    psp.pszHeaderSubTitle = L"D Profile profiles I/O via instrumenting I/O methods";
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_IO);
    psp.pfnDlgProc = IoProcedure;
	psp.lParam = (LPARAM)Context;
    hpsp[0] = CreatePropertySheetPage(&psp);

    // 
	// Attach 
	//

	psp.pszTitle = L"D Profile";
    psp.dwFlags = PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_ATTACH);
    psp.pszHeaderTitle = L"I/O Profiling";
    psp.pszHeaderSubTitle = L"Select the target process to attach to for profiling";
	psp.lParam = (LPARAM)Context;
    psp.pfnDlgProc = IoTaskProcedure;
    hpsp[1] = CreatePropertySheetPage(&psp);

    // 
	// Run
	//

	psp.pszTitle = L"D Profile";
    psp.dwFlags = PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_RUN);
    psp.pszHeaderTitle = L"I/O Profiling";
    psp.pszHeaderSubTitle = L"Fill the parameters to launch the target application";
	psp.lParam = (LPARAM)Context;
    psp.pfnDlgProc = IoRunProcedure;
    hpsp[2] = CreatePropertySheetPage(&psp);

	//
    // Create the property sheet
	//

    psh.dwSize = sizeof(psh);
    psh.hInstance = SdkInstance;
    psh.hwndParent = hWndParent;
    psh.phpage = hpsp;
    psh.dwFlags = PSH_WIZARD97|PSH_HEADER|PSH_PROPTITLE;

	//
	// Wizard bitmap size should be 49x49 
	//

    psh.pszbmHeader = MAKEINTRESOURCE(IDB_BITMAP_WIZARD);
    psh.nStartPage = 0;
    psh.nPages = 3;

	//
    // Set up the font for the titles on the intro and ending pages
	//

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    TitleLogFont = ncm.lfMessageFont;
    TitleLogFont.lfWeight = FW_BOLD;
	StringCchCopy(TitleLogFont.lfFaceName, 32, TEXT("Verdana Bold"));

    hdc = GetDC(NULL); 
    FontSize = 12;

    TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
    Context->hTitleFont = CreateFontIndirect(&TitleLogFont);
    ReleaseDC(NULL, hdc);

    PropertySheet(&psh);
    DeleteObject(Context->hTitleFont);
}

INT_PTR CALLBACK
IoProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    switch (uMsg) {

    case WM_INITDIALOG :
		return IoOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_NOTIFY :
		return IoOnNotify(hWnd, uMsg, wp, lp);

	default:
		break;
    }

    return 0;
}

LRESULT
IoOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PWIZARD_CONTEXT Context; 
	LVCOLUMN lvc = {0};

	//
	// Attach page context
	//

	Context = (PWIZARD_CONTEXT)((LPPROPSHEETPAGE)lp)->lParam;
	SetWindowLongPtr(hWnd, GWL_USERDATA, (DWORD_PTR)Context);

	//
	// Default check all types
	//

	CheckDlgButton(hWnd, IDC_CHECK_SOCKET, TRUE);
	CheckDlgButton(hWnd, IDC_CHECK_FILE, TRUE);
	CheckDlgButton(hWnd, IDC_CHECK_DEVICE, TRUE);

	//
	// Default set attach mode
	//

	CheckDlgButton(hWnd, IDC_RADIO_ATTACH, TRUE);

	SdkSetMainIcon(GetParent(hWnd));
	SdkCenterWindow(GetParent(hWnd));
	return TRUE;
}

LRESULT
IoOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LPNMHDR lpnm = (LPNMHDR)lp;
	PWIZARD_CONTEXT Context; 

	switch (lpnm->code) {

	case PSN_SETACTIVE: 
		PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_NEXT);
		break;

	case PSN_WIZNEXT:
		
		if (IsDlgButtonChecked(hWnd, IDC_RADIO_ATTACH)) {
			SetWindowLongPtr(hWnd, DWL_MSGRESULT, IDD_PROPPAGE_ATTACH);
		} else {
			SetWindowLongPtr(hWnd, DWL_MSGRESULT, IDD_PROPPAGE_RUN);
		}
		break;

	case PSN_RESET:
		Context = GET_CONTEXT(hWnd);
		Context->Cancel = TRUE;
		break;

	default :
		break;
	}

	return TRUE;
}

INT_PTR CALLBACK
IoTaskProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	switch (uMsg) {

    case WM_INITDIALOG :
		return IoTaskOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_NOTIFY :
		return IoTaskOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		if (LOWORD(wp) == IDC_BUTTON_REFRESH) {
			return IoTaskOnRefresh(hWnd, uMsg, wp, lp);
		}

	default:
		break;
    }

    return 0;
}

LRESULT
IoTaskOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	int i;
	LIST_ENTRY ListHead;
	PWIZARD_CONTEXT Context;

	Context = (PWIZARD_CONTEXT)((LPPROPSHEETPAGE)lp)->lParam;
	SetWindowLongPtr(hWnd, GWL_USERDATA, (DWORD_PTR)Context);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_TASK);
	ListView_SetUnicodeFormat(hWndCtrl, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < 3; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = IoColumn[i].Title;	
		lvc.cx = IoColumn[i].Width;     
		lvc.fmt = IoColumn[i].Align;

		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	SdkModifyStyle(hWndCtrl, 0, LVS_SORTASCENDING, FALSE);

	InitializeListHead(&ListHead);
	ApsQueryProcessList(&ListHead);
	IoInsertTask(hWndCtrl, &ListHead);

	SetFocus(hWndCtrl);
	return TRUE;
}

VOID
IoTaskOnFinish(
	__in HWND hWnd,
	__in PWIZARD_CONTEXT Context
	)
{
	HWND hWndSheet;
	HWND hWndPage;
	HWND hWndCtrl;
	PAPS_PROCESS Process;
	int index;

	if (Context->Cancel) {
		IoDeleteTask(hWnd);
		return;
	}

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_TASK);
	index = ListViewGetFirstSelected(hWndCtrl);

	if (index == -1) {
		MessageBox(hWnd, L"No process is selected!", L"D Profile", MB_OK|MB_ICONWARNING);
		SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
		return;
	}

	ListViewGetParam(hWndCtrl, index, (LPARAM *)&Process);
	ListView_DeleteItem(hWndCtrl, index);

	Context->Attach = TRUE;
	Context->Process = Process;
		
	hWndSheet = GetParent(hWnd);
	hWndPage = PropSheet_IndexToHwnd(hWndSheet, 0);
	hWndCtrl = GetDlgItem(hWndPage, IDC_COMBO_PERIOD);

	IoDeleteTask(hWnd);
}

LRESULT
IoTaskOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LPNMHDR lpnm = (LPNMHDR)lp;
	PWIZARD_CONTEXT Context;

	switch (lpnm->code) {

	case PSN_SETACTIVE: 
		PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_FINISH|PSWIZB_BACK);
		break;

	case PSN_WIZFINISH:
		Context = GET_CONTEXT(hWnd);
		Context->Cancel = FALSE;
		IoTaskOnFinish(hWnd, Context);
		break;

	case PSN_RESET:
		Context = GET_CONTEXT(hWnd);
		Context->Cancel = TRUE;
		IoTaskOnFinish(hWnd, Context);
		break;

	case PSN_WIZBACK:
		SetWindowLongPtr(hWnd, DWL_MSGRESULT, IDD_PROPPAGE_IO);
		break;

	default :
		break;
	}

	return TRUE;
}

VOID
IoInsertTask(
	__in HWND hWnd,
	__in PLIST_ENTRY ListHead
	)
{
	PAPS_PROCESS Process;
	PLIST_ENTRY ListEntry;
	LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	HANDLE ProcessHandle;
	ULONG CurrentId;
	int i;	

	CurrentId = GetCurrentProcessId();
	i = 0;

	while (IsListEmpty(ListHead) != TRUE) {

		ListEntry = RemoveHeadList(ListHead);
		Process = CONTAINING_RECORD(ListEntry, APS_PROCESS, ListEntry);

		if (Process->ProcessId == 0 || Process->ProcessId == 4 || 
			Process->ProcessId == CurrentId ) {
			ApsFreeProcess(Process);	
			continue;
		}
		
		memset(&lvi, 0, sizeof(lvi));

		if (ApsIs64Bits) {

			//
			// N.B. D Probe x64 version only list 64 bits process. 
			//

			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Process->ProcessId);
			if (!ProcessHandle) {
				ApsFreeProcess(Process);	
				continue;
			}

			if (ApsIsWow64Process(ProcessHandle)) {
				CloseHandle(ProcessHandle);
				ApsFreeProcess(Process);	
				continue;
			}

		} 

		else if (ApsIsWow64) {

			//
			// N.B. D Probe x86 version only list 32 bits process on 64 bits Windows. 
			//

			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Process->ProcessId);
			if (!ProcessHandle) {
				ApsFreeProcess(Process);	
				continue;
			}

			if (!ApsIsWow64Process(ProcessHandle)) {
				CloseHandle(ProcessHandle);
				ApsFreeProcess(Process);	
				continue;
			}
		}
		
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.pszText = Process->Name;
		lvi.lParam = (LPARAM)Process;

		ListView_InsertItem(hWnd, &lvi);
		i += 1;
	}

	for(i = 0; i < ListView_GetItemCount(hWnd); i += 1) {
		
		ListViewGetParam(hWnd, i, (LPARAM *)&Process);

		lvi.iItem = i;
		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWnd, &lvi);

		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = Process->FullPath;
		ListView_SetItem(hWnd, &lvi);
	}
}

LRESULT
IoTaskOnRefresh(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	HWND hWndCtrl;
	LIST_ENTRY ListHead;
	int index;
	
	IoDeleteTask(hWnd);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_TASK);
	index = ListViewGetFirstSelected(hWndCtrl);

	ListView_DeleteAllItems(hWndCtrl);
	
	InitializeListHead(&ListHead);
	ApsQueryProcessList(&ListHead);
	IoInsertTask(hWndCtrl, &ListHead);

	ListViewSelectSingle(hWndCtrl, index);
	ListView_EnsureVisible(hWndCtrl, index, FALSE);
	SetFocus(hWndCtrl);
	return TRUE;
}

VOID
IoDeleteTask(
	__in HWND hWnd 
	)
{
	HWND hWndCtrl;
	PAPS_PROCESS Process;
	int Count, i;
	
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_TASK);
	Count = ListView_GetItemCount(hWndCtrl);

	for(i = 0; i < Count; i++) {
		ListViewGetParam(hWndCtrl, i, (LPARAM *)&Process);
		if (Process != NULL) {
			ApsFreeProcess(Process);
			ListViewSetParam(hWndCtrl, i, (LPARAM)NULL);
		}
	}
}

INT_PTR CALLBACK
IoRunProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	switch (uMsg) {

    case WM_INITDIALOG :
		return IoRunOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_NOTIFY :
		return IoRunOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		if (LOWORD(wp) == IDC_BUTTON_PATH) {
			return IoRunOnPath(hWnd, uMsg, wp, lp);
		}

	default:
		break;
    }

    return 0;
}

LRESULT
IoRunOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PWIZARD_CONTEXT Context; 
	
	Context = (PWIZARD_CONTEXT)((LPPROPSHEETPAGE)lp)->lParam;
	SetWindowLongPtr(hWnd, GWL_USERDATA, (DWORD_PTR)Context);
	SdkCenterWindow(GetParent(hWnd));
	return TRUE;
}

LRESULT
IoRunOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LPNMHDR lpnm = (LPNMHDR)lp;
	PWIZARD_CONTEXT Context;

	switch (lpnm->code) {

	case PSN_SETACTIVE: 
		PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_FINISH|PSWIZB_BACK);
		break;

	case PSN_WIZFINISH:
		Context = GET_CONTEXT(hWnd);
		Context->Cancel = FALSE;
		IoRunOnFinish(hWnd, Context);
		break;

	case PSN_RESET:
		Context = GET_CONTEXT(hWnd);
		Context->Cancel = TRUE;
		IoRunOnFinish(hWnd, Context);
		break;

	case PSN_WIZBACK:
		SetWindowLongPtr(hWnd, DWL_MSGRESULT, IDD_PROPPAGE_IO);
		break;

	default :
		break;
	}

	return TRUE;
}

VOID
IoRunOnFinish(
	__in HWND hWnd,
	__in PWIZARD_CONTEXT Context
	)
{
	HWND hWndSheet;
	HWND hWndPage;
	HWND hWndCtrl;
	WCHAR Drive[MAX_PATH];
	WCHAR Folder[MAX_PATH];

	//
	// N.B. we must delete attach page's process list, because
	// user can first go to attach page and then back to first page,
	// then go to run page, during the process, attach page is valid
	// and its list control hold running process objects.
	//

	hWndSheet = GetParent(hWnd);
	hWndPage = PropSheet_IndexToHwnd(hWndSheet, 1);
	IoDeleteTask(hWndPage);

	if (Context->Cancel) {
		return;
	}

	if (!IoRunCheckPath(hWnd)) {
		MessageBox(hWnd, L"Invalid image file path!", L"D Profile", MB_OK|MB_ICONERROR);
		SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
		return;
	}

	Context->Attach = FALSE;
	Context->Process = NULL;

	//
	// Get image path (mandatory)
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_IMAGEPATH);
	GetWindowText(hWndCtrl, Context->ImagePath, MAX_PATH);
	
	//
	// Get argument of image (optional)
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_ARGUMENT);
	GetWindowText(hWndCtrl, Context->Argument, MAX_PATH);
		
	//
	// Get work path of image (optional)
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_WORKPATH);
	GetWindowText(hWndCtrl, Context->WorkPath, MAX_PATH);

	if (wcslen(Context->WorkPath) == 0) {

		//
		// Set image path as work path
		//

		_wsplitpath(Context->ImagePath, Drive, Folder, NULL, NULL);
		StringCchPrintf(Context->WorkPath, MAX_PATH, L"%s%s", Drive, Folder);
	}

	hWndPage = PropSheet_IndexToHwnd(hWndSheet, 0);
}

BOOLEAN
IoRunCheckPath(
	__in HWND hWnd
	)
{
	HWND hWndCtrl;
	HMODULE Handle;

	WCHAR Buffer[MAX_PATH];

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_IMAGEPATH);
	GetWindowText(hWndCtrl, Buffer, MAX_PATH);

	Handle = LoadLibraryEx(Buffer, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (Handle) {
		FreeLibrary(Handle);
		return TRUE;
	}

	return FALSE;
}

LRESULT
IoRunOnPath(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	OPENFILENAME Ofn;
	BOOL Status;
	WCHAR Path[MAX_PATH];	

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"Executable File (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Profile";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (Status == FALSE) {
		return 0;
	}

	SetWindowText(GetDlgItem(hWnd, IDC_EDIT_IMAGEPATH), Ofn.lpstrFile);
	return 0;
}