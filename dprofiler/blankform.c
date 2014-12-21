#include "blankform.h"

HWND
BlankCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	HWND hWnd;
	
	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = NULL;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_BLANK;
	Object->Procedure = BlankProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
BlankOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return TRUE;
}

INT_PTR CALLBACK
BlankProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return BlankOnInitDialog(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
BlankOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
BlankOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}