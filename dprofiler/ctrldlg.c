//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "ctrldlg.h"
#include "sdk.h"
#include "dialog.h"
#include "listview.h"
#include "split.h"
#include "apscpu.h"

#define WM_USER_PROFILE     (WM_USER + 1)
#define WM_USER_TERMINATE   (WM_USER + 2)
#define ID_CTRL_TIMER 10

typedef struct _CTRL_CONTEXT {
	PAPS_PROFILE_OBJECT Profile;
	BOOLEAN Pause;
	HFONT hFontBold;
	UINT_PTR TimerId;
} CTRL_CONTEXT, *PCTRL_CONTEXT;

LISTVIEW_COLUMN CtrlColumn[2] = {
	{ 120,  L"Name", LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 360,  L"Value", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

INT_PTR
CtrlDialog(
	IN HWND hWndParent,
	IN PAPS_PROFILE_OBJECT Profile,
	IN BOOLEAN Pause
	)
{
	DIALOG_OBJECT Object = {0};
	CTRL_CONTEXT Context = {0};
	INT_PTR Return;

	Context.Profile = Profile;
	Context.Pause = Pause;
	Object.Context = &Context;

	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_CTRL;
	Object.Procedure = CtrlProcedure;
	
	Return = DialogCreate(&Object);

	if (Context.hFontBold) {
		DeleteFont(Context.hFontBold);
	}

	return Return;
}

INT_PTR CALLBACK
CtrlProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return CtrlOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
				return CtrlOnOk(hWnd, uMsg, wp, lp);
			}
			if (LOWORD(wp) == IDC_BUTTON_START) {
				return CtrlOnStart(hWnd, uMsg, wp, lp);
			}

		case WM_TIMER:
			return CtrlOnTimer(hWnd, uMsg, wp, lp);

		case WM_NOTIFY:
			if (wp == IDC_LIST_DATA) {
				return CtrlOnNotify(hWnd, uMsg, wp, lp);
			}
			break;

		case WM_USER_TERMINATE:
			CtrlOnTerminate(hWnd, uMsg, wp, lp);
			break;
	}
	
	return Status;
}

LRESULT
CtrlOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PCTRL_CONTEXT Context;
	LVCOLUMN lvc = {0};
	LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	int i;
	PAPS_PROFILE_OBJECT Profile;
	ULONG ProcessId;
	WCHAR BaseName[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTRL_CONTEXT)Object->Context;
	Profile = Context->Profile;

	//
	// Register profile callback
	//

	ApsRegisterCallback(Profile, CtrlProfileCallback, (PVOID)hWnd);

	ProcessId = Profile->ProcessId;
	_wsplitpath(Profile->ImagePath, NULL, NULL, BaseName, NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndCtrl, PBM_SETMARQUEE, 1, 20); 

	if (Context->Pause) {
		SdkModifyStyle(hWndCtrl, PBS_MARQUEE, 0, FALSE);
	}

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_DATA);
	ListView_SetUnicodeFormat(hWndCtrl, TRUE);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_DOUBLEBUFFER,  LVS_EX_DOUBLEBUFFER);


	for(i = 0; i < 2; i += 1) {
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CtrlColumn[i].Title;	
		lvc.cx = CtrlColumn[i].Width;     
		lvc.fmt = CtrlColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
	}

	if (Profile->Type == PROFILE_CPU_TYPE) {
		CtrlInsertCpuCounters(hWndCtrl);
		StringCchPrintf(Buffer, MAX_PATH, L"CPU Profiling on PID %u (%s)", 
						ProcessId, BaseName);
	}

	if (Profile->Type == PROFILE_MM_TYPE) {
		CtrlInsertMmCounters(hWndCtrl);
		StringCchPrintf(Buffer, MAX_PATH, L"Memory Profiling on PID %u (%s)", 
						ProcessId, BaseName);
	}
	
	if (Profile->Type == PROFILE_IO_TYPE) {
		CtrlInsertIoCounters(hWndCtrl);
		StringCchPrintf(Buffer, MAX_PATH, L"I/O Profiling on PID %u (%s)", 
						ProcessId, BaseName);
	}
	
	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC);
	SetWindowText(hWndCtrl, Buffer);

	SetWindowText(hWnd, L"D Profile");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);

	if (Profile->Type != PROFILE_CPU_TYPE) {
		hWndCtrl = GetDlgItem(hWnd, IDC_BUTTON_START);
		ShowWindow(hWndCtrl, SW_HIDE);
	}

	if (Profile->Type == PROFILE_CPU_TYPE) {

		if (!Context->Pause) {
			
			hWndCtrl = GetDlgItem(hWnd, IDC_BUTTON_START);
			SetWindowText(hWndCtrl, L"Pause");
			
			Profile->Attribute.Paused = FALSE;
			//ApsCpuStartProfile(Profile);
		}
		else {
			Profile->Attribute.Paused = TRUE;
		}
	}

	//
	// Activate update timer
	//

	Context->TimerId = SetTimer(hWnd, ID_CTRL_TIMER, 1000, NULL);
	return TRUE;
}

LRESULT
CtrlOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCTRL_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTRL_CONTEXT)Object->Context;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_STATIC) {

		if (!Context->hFontBold) { 

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			LogFont.lfQuality = CLEARTYPE_QUALITY;
			LogFont.lfCharSet = ANSI_CHARSET;
			hFont = CreateFontIndirect(&LogFont);

			Context->hFontBold = hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}

	}

	return 0;
}

LRESULT
CtrlOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Id;
	PDIALOG_OBJECT Object;
	PCTRL_CONTEXT Context;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CTRL_CONTEXT);

	KillTimer(hWnd, Context->TimerId);

	Id = LOWORD(wp);
	EndDialog(hWnd, Id);
	return TRUE;
}

LRESULT
CtrlOnStart(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	WCHAR Buffer[MAX_PATH];
	PAPS_PROFILE_OBJECT Profile;
	PCTRL_CONTEXT Context;
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CTRL_CONTEXT);
	Profile = Context->Profile;

	hWndCtrl = GetDlgItem(hWnd, IDC_BUTTON_START);
	GetWindowText(hWndCtrl, Buffer, MAX_PATH);

	if (wcsicmp(Buffer, L"Start") == 0) {

		Profile->Attribute.Paused = FALSE;
		ApsStartProfile(Profile);
		SetWindowText(hWndCtrl, L"Pause");
		
		hWndCtrl = GetDlgItem(hWnd, IDC_PROGRESS);
		SdkModifyStyle(hWndCtrl, 0, PBS_MARQUEE, FALSE);

	}

	else {

		Profile->Attribute.Paused = TRUE;
		SetWindowText(hWndCtrl, L"Start");

		hWndCtrl = GetDlgItem(hWnd, IDC_PROGRESS);
		SdkModifyStyle(hWndCtrl, PBS_MARQUEE, 0, FALSE);
	}

	return TRUE;
}

LRESULT
CtrlOnTimer(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCTRL_CONTEXT Context;
	HWND hWndCtrl;
	PAPS_PROFILE_OBJECT Profile;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CTRL_CONTEXT);
	Profile = Context->Profile;
	
	if (ApsIsProfileTerminated(Profile)) {
		CtrlOnOk(hWnd, WM_COMMAND, IDOK, 0);
		return 0;
	}

	if (Profile->Type == PROFILE_CPU_TYPE) {
		hWndCtrl = GetDlgItem(hWnd, IDC_LIST_DATA);
		CtrlUpdateCpuCounters(hWndCtrl, Profile);
	}

	if (Profile->Type == PROFILE_MM_TYPE) {
		hWndCtrl = GetDlgItem(hWnd, IDC_LIST_DATA);
		CtrlUpdateMmCounters(hWndCtrl, Profile);
	}

	if (Profile->Type == PROFILE_IO_TYPE) {
		hWndCtrl = GetDlgItem(hWnd, IDC_LIST_DATA);
		CtrlUpdateIoCounters(hWndCtrl, Profile);
	}

	return TRUE;
}

LRESULT
CtrlOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0;
	NMHDR *pNmhdr = (NMHDR *)lp;

	switch (pNmhdr->code) {
		case NM_CUSTOMDRAW:
			Status = CtrlOnCustomDraw(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
CtrlOnTerminate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	ULONG Status;
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PCTRL_CONTEXT Context;
	PAPS_PROFILE_OBJECT Profile;
	WCHAR Buffer[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTRL_CONTEXT)Object->Context;
	Profile = Context->Profile;

	//
	// Pause progress bar
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndCtrl, PBM_SETSTATE, (WPARAM)PBST_PAUSED, 0);

	//
	// Update text to notify user that target process terminating/terminated
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC);

	Status = (ULONG)wp;

	if (Status == APS_STATUS_TERMINATED) {
		StringCchPrintf(Buffer, MAX_PATH, L"PID %u was terminated unexpectedly, profiling is stopped.", 
			            Profile->ProcessId);
	}

	if (Status == APS_STATUS_EXITPROCESS) {
		StringCchPrintf(Buffer, MAX_PATH, L"PID %u is terminating, profiling is stopped.", 
			            Profile->ProcessId);
	}

	//
	// Wait for 3 seconds to notify user
	//

	FlashWindow(hWnd, FALSE);

	EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_START), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);

	MsgWaitForMultipleObjects(0, NULL, FALSE, 3000, QS_ALLEVENTS);

	EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
	CtrlOnOk(hWnd, IDOK, 0, 0);

	return Status;
}

LRESULT
CtrlOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0L;
	LPNMHDR pNmhdr = (LPNMHDR)lp; 
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW;
            break;
        
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 

			if (lvcd->nmcd.dwItemSpec % 2 != 0) { 
				lvcd->clrTextBk = RGB(255, 255, 255);
			} 
			else {
				lvcd->clrTextBk = RGB(212, 208, 200);
			}
			Status = CDRF_NEWFONT;
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

VOID
CtrlInsertCpuCounters(
	IN HWND hWndCtrl
	)
{
	LVITEM lvi = {0};

	lvi.mask = LVIF_TEXT;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = L"Sample Stages";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 1;
	lvi.iSubItem = 0;
	lvi.pszText = L"IP Count";
	ListView_InsertItem(hWndCtrl, &lvi);
}

VOID
CtrlUpdateCpuCounters(
	IN HWND hWndCtrl,
	IN PAPS_PROFILE_OBJECT Object
	)
{
	WCHAR Buffer[MAX_PATH];

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.SamplesDepth);
	ListView_SetItemText(hWndCtrl, 0, 1, Buffer);
}

VOID
CtrlInsertMmCounters(
	IN HWND hWndCtrl
	)
{
	LVITEM lvi = {0};

	lvi.mask = LVIF_TEXT;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = L"Heap Alloc Count";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 1;
	lvi.iSubItem = 0;
	lvi.pszText = L"Heap Free Count";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 2;
	lvi.iSubItem = 0;
	lvi.pszText = L"Page Alloc Count";
	ListView_InsertItem(hWndCtrl, &lvi);
		
	lvi.iItem = 3;
	lvi.iSubItem = 0;
	lvi.pszText = L"Page Free Count";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 4;
	lvi.iSubItem = 0;
	lvi.pszText = L"Handle Alloc Count";
	ListView_InsertItem(hWndCtrl, &lvi);
	
	lvi.iItem = 5;
	lvi.iSubItem = 0;
	lvi.pszText = L"Handle Free Count";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 6;
	lvi.iSubItem = 0;
	lvi.pszText = L"Current Handle Count";
	ListView_InsertItem(hWndCtrl, &lvi);
	
	lvi.iItem = 7;
	lvi.iSubItem = 0;
	lvi.pszText = L"GDI Alloc Count";
	ListView_InsertItem(hWndCtrl, &lvi);
	
	lvi.iItem = 8;
	lvi.iSubItem = 0;
	lvi.pszText = L"GDI Free Count";
	ListView_InsertItem(hWndCtrl, &lvi);
	
	lvi.iItem = 9;
	lvi.iSubItem = 0;
	lvi.pszText = L"Current GDI Count";
	ListView_InsertItem(hWndCtrl, &lvi);
}

VOID
CtrlUpdateMmCounters(
	IN HWND hWndCtrl,
	IN PAPS_PROFILE_OBJECT Object
	)
{
	WCHAR Buffer[MAX_PATH];
	ULONG Count;

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfHeapAllocs);
	ListView_SetItemText(hWndCtrl, 0, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfHeapFrees);
	ListView_SetItemText(hWndCtrl, 1, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfPageAllocs);
	ListView_SetItemText(hWndCtrl, 2, 1, Buffer);
	
	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfPageFrees);
	ListView_SetItemText(hWndCtrl, 3, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfHandleAllocs);
	ListView_SetItemText(hWndCtrl, 4, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfHandleFrees);
	ListView_SetItemText(hWndCtrl, 5, 1, Buffer);

	GetProcessHandleCount(Object->ProcessHandle, &Count);
	StringCchPrintf(Buffer, MAX_PATH, L"%d", Count);
	ListView_SetItemText(hWndCtrl, 6, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfGdiAllocs);
	ListView_SetItemText(hWndCtrl, 7, 1, Buffer);
	
	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfGdiAllocs);
	ListView_SetItemText(hWndCtrl, 8, 1, Buffer);
	
	Count = GetGuiResources(Object->ProcessHandle, GR_GDIOBJECTS);
	StringCchPrintf(Buffer, MAX_PATH, L"%d", Count);
	ListView_SetItemText(hWndCtrl, 9, 1, Buffer);
}

VOID
CtrlInsertIoCounters(
	IN HWND hWndCtrl
	)
{
	LVITEM lvi = {0};

	lvi.mask = LVIF_TEXT;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = L"I/O Read Count";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 1;
	lvi.iSubItem = 0;
	lvi.pszText = L"I/O Write Count";
	ListView_InsertItem(hWndCtrl, &lvi);

	lvi.iItem = 2;
	lvi.iSubItem = 0;
	lvi.pszText = L"I/O Control Count";
	ListView_InsertItem(hWndCtrl, &lvi);
}

VOID
CtrlUpdateIoCounters(
	IN HWND hWndCtrl,
	IN PAPS_PROFILE_OBJECT Object
	)
{
	WCHAR Buffer[MAX_PATH];

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfIoRead);
	ListView_SetItemText(hWndCtrl, 0, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfIoWrite);
	ListView_SetItemText(hWndCtrl, 1, 1, Buffer);

	StringCchPrintf(Buffer, MAX_PATH, L"%d", Object->Attribute.NumberOfIoCtl);
	ListView_SetItemText(hWndCtrl, 2, 1, Buffer);
}

ULONG
CtrlProfileCallback(
	__in PAPS_PROFILE_OBJECT Object,
	__in PVOID Context,
	__in ULONG Status
	)
{
	HWND hWnd;

	hWnd = (HWND)Context;
	PostMessage(hWnd, WM_USER_TERMINATE, (WPARAM)Status, 0);
	return APS_STATUS_OK;
}