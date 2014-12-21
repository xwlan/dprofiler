//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2014
// 

#include "sdk.h"
#include "dialog.h"
#include "deduction.h"
#include "aps.h"

typedef struct _DEDUCTION_CONTEXT {
	BOOLEAN Enable;
	ULONG Threshold;
} DEDUCTION_CONTEXT, *PDEDUCTION_CONTEXT;

VOID
DeductionDialog(
	__in HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	DEDUCTION_CONTEXT Context = {0};

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_DEDUCTION;
	Object.Procedure = DeductionProcedure;
	
	DialogCreate(&Object);
}

INT_PTR CALLBACK
DeductionProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return DeductionOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
				return DeductionOnOk(hWnd, uMsg, wp, lp);
			}
			if (LOWORD(wp) == IDC_CHECK_TRIMMING) {
				return DeductionOnTrimming(hWnd, uMsg, wp, lp);
			}
	}
	
	return Status;
}

LRESULT
DeductionOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PDEDUCTION_CONTEXT Context;
	HWND hWndEdit;
	HWND hWndCheck;
	WCHAR Percent[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDEDUCTION_CONTEXT)Object->Context;

	//
	// Disable threshold edit
	//

	hWndCheck = GetDlgItem(hWnd, IDC_CHECK_TRIMMING);
	hWndEdit = GetDlgItem(hWnd, IDC_EDIT_THRESHOLD);
	SendMessage(hWndEdit, EM_LIMITTEXT, 2, 0);
	EnableWindow(hWndEdit, FALSE);

	if (ApsIsDeductionEnabled()) {

		Context->Enable = TRUE;
		Context->Threshold = ApsGetDeductionThreshold(DEDUCTION_INCLUSIVE_PERCENT);

		CheckDlgButton(hWnd, IDC_CHECK_TRIMMING, BST_CHECKED);
		EnableWindow(hWndEdit, TRUE);

		StringCchPrintf(Percent, MAX_PATH, L"%2d", Context->Threshold);
		SendMessage(hWndEdit, WM_SETTEXT, 0, (LPARAM)Percent);
	}
	else {
		Context->Enable = FALSE;
		Context->Threshold = 0;
	}

	SetWindowText(hWnd, L"Noise Deduction");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
DeductionOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PDEDUCTION_CONTEXT Context;
	UINT Check;
	BOOLEAN Notify = FALSE;
	ULONG Threshold;
	WCHAR Buffer[3] = {0};

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDEDUCTION_CONTEXT)Object->Context;

	Check = IsDlgButtonChecked(hWnd, IDC_CHECK_TRIMMING);
	if (Check) {

		ApsEnableDeduction(TRUE);
		if (!Context->Enable) {
			Notify = TRUE;
		}

		GetWindowText(GetDlgItem(hWnd, IDC_EDIT_THRESHOLD), Buffer, 3);	
		Threshold = _wtoi(Buffer);
		ApsSetDeductionThreshold(DEDUCTION_INCLUSIVE_PERCENT, Threshold);

		if (Context->Threshold != Threshold) {
			Notify = TRUE;
		}

	} else {

		ApsEnableDeduction(FALSE);
		ApsSetDeductionThreshold(DEDUCTION_INCLUSIVE_PERCENT, 0);
		
		if (Context->Enable) {
			Notify = TRUE;
		}
	}

	//
	// Post update message to frame window
	//

	if (Notify) {
		PostMessage(GetParent(hWnd), WM_DEDUCTION_UPDATE, 0, 0);
	}

	EndDialog(hWnd, IDOK);
	return TRUE;
}

LRESULT
DeductionOnTrimming(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	UINT Check;
	HWND hWndEdit;
	WCHAR Percent[10];

	hWndEdit = GetDlgItem(hWnd, IDC_EDIT_THRESHOLD);
	Check = IsDlgButtonChecked(hWnd, IDC_CHECK_TRIMMING);

	if (!Check) {
		EnableWindow(hWndEdit, FALSE);
	} else {
		EnableWindow(hWndEdit, TRUE);
		StringCchPrintf(Percent, 10, L"%2d", 
						ApsGetDeductionThreshold(DEDUCTION_INCLUSIVE_PERCENT));
		SendMessage(hWndEdit, WM_SETTEXT, 0, (LPARAM)Percent);
	} 

	return TRUE;
}