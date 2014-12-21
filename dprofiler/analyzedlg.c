//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "aps.h"
#include "dialog.h"
#include "analyzedlg.h"

typedef struct _ANALYZE_CONTEXT {
	BTR_PROFILE_TYPE Type;
	PVOID Profile;
	ULONG Percent;
	HFONT hBoldFont;
} ANALYZE_CONTEXT, *PANALYZE_CONTEXT;

VOID
AnalyzeDialog(
	IN HWND hWndParent,
	IN BTR_PROFILE_TYPE Type,
	IN PVOID Profile 
	)
{
	DIALOG_OBJECT Object = {0};
	ANALYZE_CONTEXT Context = {0};

	Context.Type = Type;
	Context.Profile = Profile;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_ANALYZE;
	Object.Procedure = AnalyzeProcedure;
	Object.Scaler = NULL;
	
	DialogCreate(&Object);
}

INT_PTR CALLBACK
AnalyzeProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {

		case WM_INITDIALOG: 
			return AnalyzeOnInitDialog(hWnd, uMsg, wp, lp);
		
		case WM_TIMER:
			return AnalyzeOnTimer(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
AnalyzeOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PANALYZE_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, ANALYZE_CONTEXT);

	SetDlgItemText(hWnd, IDC_STATIC, L"Analyzing profiler records ...");
	
	hWndCtrl = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndCtrl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hWndCtrl, PBM_SETSTEP, 10, 0); 

	if (Context->Type == PROFILE_CPU_TYPE) {
	}

	if (Context->Type == PROFILE_MM_TYPE) {

	}
	
	if (Context->Type == PROFILE_IO_TYPE) {
		
	}
	
	//
	// Set percent update timer
	//

	SetTimer(hWnd, 1, 1000, NULL);

	SetWindowText(hWnd, L"D Profile");
	SdkCenterWindow(hWnd);
	return 0;
}

LRESULT
AnalyzeOnTimer(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PANALYZE_CONTEXT Context;
	WCHAR Buffer[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, ANALYZE_CONTEXT);

	StringCchPrintf(Buffer, MAX_PATH, L"%%%u of records are analyzed...", Context->Percent);
    SetDlgItemText(hWnd, IDC_STATIC, Buffer);

	hWndCtrl = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndCtrl, PBM_SETPOS, Context->Percent, 0);

	if (Context->Percent == 100) {
		KillTimer(hWnd, 1);
		EndDialog(hWnd, IDOK);
	}

	return 0;
}

VOID CALLBACK
AnalyzeCallback(
	IN PVOID Context,
	IN ULONG Percent
	)
{
	HWND hWnd;

	hWnd = (HWND)Context;
	PostMessage(hWnd, WM_USER_ANALYZE, (WPARAM)Percent, 0);
}