//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#include "sdk.h"
#include "help.h"
#include "dialog.h"
#include <richedit.h>

DIALOG_SCALER_CHILD HelpChildren[] = {
	{ IDC_RICHEDIT, AlignRight, AlignBottom },
	{ IDOK, AlignBoth, AlignBoth }
};

DIALOG_SCALER HelpScaler = {
	{0,0}, {0,0}, {0,0}, 2, HelpChildren
};

INT_PTR
HelpDialog(
	__in HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	INT_PTR Return;

	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_HELP;
	Object.Procedure = HelpProcedure;
	
	Return = DialogCreate(&Object);
	return Return;
}

INT_PTR CALLBACK
HelpProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return HelpOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK) { 
				EndDialog(hWnd, IDOK);
			}
			else if (LOWORD(wp) == IDCANCEL) { 
				EndDialog(hWnd, IDCANCEL);
			}
			break;
	}
	
	return Status;
}

LRESULT
HelpOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HWND hWndEdit;
	HRSRC hrsrc;
	HGLOBAL hHelp;
	EDITSTREAM Stream = {0};
	PDIALOG_OBJECT Object;

	hrsrc = FindResource(SdkInstance, MAKEINTRESOURCE(IDR_HELP), L"HELP");
	ASSERT(hrsrc != NULL);

	hHelp = LoadResource(SdkInstance, hrsrc);
	ASSERT(hHelp != NULL);

	hWndEdit = GetDlgItem(hWnd, IDC_RICHEDIT);

	Stream.dwCookie = (DWORD_PTR)hHelp;
	Stream.dwError = 0;
	Stream.pfnCallback = HelpStreamCallback;
	SendMessage(hWndEdit, EM_STREAMIN, SF_RTF, (LPARAM)&Stream);

	//
	// Register dialog scaler
	//

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Object->Scaler = &HelpScaler;
	DialogRegisterScaler(Object);

	SetWindowText(hWnd, L"Help");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

DWORD CALLBACK
HelpStreamCallback(
	__in DWORD_PTR dwCookie,
    __in LPBYTE pbBuff,
    __in LONG cb,
    __out LONG *pcb
	)
{
	PCHAR Help;

	Help = (PCHAR)dwCookie;
	memcpy(pbBuff, Help, cb);

	*pcb = cb;
	return 0;
}