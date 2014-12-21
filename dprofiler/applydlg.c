//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "bsp.h"
#include "dialog.h"
#include "applydlg.h"

typedef struct _APPLY_CONTEXT {
	PBSP_PROCESS Process;
	struct _PF_COMMAND *Command;
	HFONT hBoldFont;
} APPLY_CONTEXT, *PAPPLY_CONTEXT;

INT_PTR
ApplyDialog(
	IN HWND hWndParent,
	IN PBSP_PROCESS Process,
	IN struct _PF_COMMAND *Command
	)
{
	INT_PTR Return;
	DIALOG_OBJECT Object = {0};
	APPLY_CONTEXT Context = {0};

	Context.Process = Process;
	Context.Command = Command;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_APPLY;
	Object.Procedure = ApplyProcedure;
	Object.Scaler = NULL;

	Return = DialogCreate(&Object);
	return Return;
}

LRESULT
ApplyOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
	HWND hWndCancel;
	LONG Style;
	WCHAR Buffer[MAX_PATH];
	PDIALOG_OBJECT Object;
	PAPPLY_CONTEXT Context;
	PBSP_PROCESS Process;
	PPF_COMMAND Command;

	hWndCancel = GetDlgItem(hWnd, IDCANCEL);
	ShowWindow(hWndCancel, SW_HIDE);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, APPLY_CONTEXT);
	Process = Context->Process;
	Command = Context->Command;

	if (Command->CommandType == CommandStartProfile) {
		StringCchPrintf(Buffer, MAX_PATH, L"Applying profiling to %s...", Process->Name);
	}
	
	if (Command->CommandType == CommandStopProfile) {
		StringCchPrintf(Buffer, MAX_PATH, L"Stopping profiling of %s...", Process->Name);
	}

	SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);

	//
	// Enable MARQUEE mode, walk around in 1 second
	//

	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
	Style = GetWindowLong(hWndBar, GWL_STYLE);
	SetWindowLong(hWndBar, GWL_STYLE,  Style | PBS_MARQUEE);
	SendMessage(hWndBar, PBM_SETMARQUEE, 1, 20); 

	SetWindowText(hWnd, L"D Profiler");
	SdkCenterWindow(hWnd);

	if (Command != NULL) {
		/*Command->Callback = ApplyCallback;
		Command->CallbackContext = (PVOID)hWnd;
		MspQueueUserCommand(Command);*/
	}
	
	return 0;
}

//
// Packet is a pointer to PF_COMMAND
// Context is HWND, handle of progress bar
//

ULONG CALLBACK
ApplyCallback(
	IN PVOID Packet,
	IN PVOID Context
	)
{
	HWND hWnd;

	hWnd = (HWND)Context;
	PostMessage(hWnd, WM_CLOSE, 0, 0);
	return 0;
}

LRESULT
ApplyOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT CALLBACK
ApplyProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (uMsg) {

		case WM_INITDIALOG: 
			return ApplyOnInitDialog(hWnd, uMsg, wp, lp);
		
		case WM_CLOSE:
			return ApplyOnClose(hWnd, uMsg, wp, lp);

		case WM_CTLCOLORSTATIC:
			return ApplyOnCtlColorStatic(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
ApplyOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PAPPLY_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PAPPLY_CONTEXT)Object->Context;
	hFont = Context->hBoldFont;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_STATIC) {
		if (!hFont) {

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&LogFont);

			Context->hBoldFont = hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}
	}

	return (LRESULT)0;
}