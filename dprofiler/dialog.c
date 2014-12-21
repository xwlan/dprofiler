#include "sdk.h"
#include "dialog.h"
#include "resource.h"
#include "frame.h"

int WM_USER_SIZE;

INT_PTR
DialogCreate(
	IN PDIALOG_OBJECT Object
	)
{
	INT_PTR Status;

	Status = DialogBoxParam(SdkInstance, 
							MAKEINTRESOURCE(Object->ResourceId), 
							Object->hWndParent, 
							DialogProcedure, 
							(LPARAM)Object);

	return Status;
}

HWND
DialogCreateModeless(
	IN PDIALOG_OBJECT Object
	)
{
	HWND hWnd;
	hWnd = CreateDialogParam(SdkInstance, 
							 MAKEINTRESOURCE(Object->ResourceId), 
							 Object->hWndParent, 
							 DialogProcedure, 
							 (LPARAM)Object);

	return hWnd;
}

INT_PTR CALLBACK 
DialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Result = 0;
	PDIALOG_OBJECT Object = NULL;

    if (uMsg == WM_INITDIALOG) {
    
		Object = (PDIALOG_OBJECT)lp;

		ASSERT(Object != NULL);
		ASSERT(Object->Procedure != NULL);
		ASSERT(Object->Procedure != DialogProcedure);

		Object->hWnd = hWnd;
		SdkSetObject(hWnd, Object);
		
		if (Object->Scaler != NULL) {
			DialogRegisterScaler(Object);
		}
	}
	
	if (Object == NULL) {
		Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	}

	if (Object != NULL && Object->Procedure != NULL) {
		Result = (Object->Procedure)(hWnd, uMsg, wp, lp);
#if defined (_M_IX86)
		SetWindowLongPtr(hWnd, DWLP_MSGRESULT, (LONG)Result);
#else defined (_M_X64)
		SetWindowLongPtr(hWnd, DWLP_MSGRESULT, Result);
#endif

	} else {

		//
		// N.B. Some message may be sent before WM_INITDIALOG, e.g. owner draw message
		// WM_MEASUREITEM, we need take this in special consideration
		//
	}

    return Result;
}

LRESULT CALLBACK 
DialogScalerProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp,
	IN UINT_PTR uIdSubclass, 
	IN DWORD_PTR dwData
	)
{
	PDIALOG_OBJECT Object;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);

	ASSERT(Object != NULL);

	switch (uMsg) {
        
		case WM_NCHITTEST:
            return DialogScalerOnNcHitTest(Object, LOWORD(lp), HIWORD(lp));

        case WM_SIZE:
            return DialogScalerOnSize(Object, (UINT)wp, LOWORD(lp), HIWORD(lp));

        case WM_SIZING:
            return DialogScalerOnSizing(Object, (UINT)wp,(RECT *)lp);

        case WM_ERASEBKGND:
            return DialogScalerOnEraseBkgnd(Object, (HDC)wp);

        case WM_GETMINMAXINFO:
            return DialogScalerOnGetMinMaxInfo(Object, (LPMINMAXINFO)lp);
    }

    return DefSubclassProc(hWnd, uMsg, wp, lp);
}

LRESULT 
DialogScalerOnEraseBkgnd(
	IN DIALOG_OBJECT *Object,
	IN HDC hdc
	)
{
    LRESULT Result;
	
	Result = DefSubclassProc(Object->hWnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    
	if (!(WS_CHILD & GetWindowLongPtr(Object->hWnd, GWL_STYLE))) {
        RECT rc;
        GetClientRect(Object->hWnd, &rc);
        rc.left = rc.right  - GetSystemMetrics(SM_CXVSCROLL);
        rc.top  = rc.bottom - GetSystemMetrics(SM_CXVSCROLL);
		if (!Object->NoFrameControl) {
	        DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
		}

    }

    return Result;
}

LRESULT 
DialogScalerOnNcHitTest(
	IN DIALOG_OBJECT *Object,
	IN int x, 
	IN int y
	)
{
    if (!(WS_CHILD & GetWindowLongPtr(Object->hWnd, GWL_STYLE))) {

        RECT rc;
		int cx, cy;

        GetClientRect(Object->hWnd, &rc);

        rc.left = rc.right  - GetSystemMetrics(SM_CXVSCROLL);
        rc.top  = rc.bottom - GetSystemMetrics(SM_CXVSCROLL);

        MapWindowRect(Object->hWnd, HWND_DESKTOP, &rc);

        cx = x - rc.left;
        cy = y - rc.top;
        if (cx > 0 && cy > 0) {
            if (rc.right - rc.left < cx + cy) {
                return HTBOTTOMRIGHT;
            }
        }
    }

    return DefSubclassProc(Object->hWnd, WM_NCHITTEST, 0, MAKELPARAM(x, y));
}

LRESULT 
DialogScalerOnGetMinMaxInfo(
	IN  DIALOG_OBJECT *Object,
	OUT LPMINMAXINFO lpMinMaxInfo
	)
{
    lpMinMaxInfo->ptMinTrackSize.x = Object->Scaler->MinTrack.cx;
    lpMinMaxInfo->ptMinTrackSize.y = Object->Scaler->MinTrack.cy;

    return TRUE;
}

LRESULT 
DialogScalerOnSizing(
	IN PDIALOG_OBJECT Object,
	IN UINT uEdge, 
	OUT RECT *lpRect
	)
{
    LRESULT Status;
    RECT rc;

	Status = DefSubclassProc(Object->hWnd, WM_SIZING,(WPARAM)uEdge, (LPARAM)lpRect);

    GetClientRect(Object->hWnd, &rc);
    rc.left = rc.right  - GetSystemMetrics(SM_CXVSCROLL);
    rc.top  = rc.bottom - GetSystemMetrics(SM_CXVSCROLL);
    InvalidateRect(Object->hWnd, &rc, FALSE);

    return (LRESULT)Status;
}

LRESULT 
DialogScalerOnSize(
	IN PDIALOG_OBJECT Object,
	IN UINT nState, 
	IN int cx, 
	IN int cy
	)
{
    DefSubclassProc(Object->hWnd, WM_SIZE,(WPARAM)nState, MAKELPARAM(cx, cy));

    if (nState != SIZE_MINIMIZED) {
        
		RECT rc;
        GetClientRect(Object->hWnd, &rc);
        rc.left = rc.right  - GetSystemMetrics(SM_CXVSCROLL);
        rc.top  = rc.bottom - GetSystemMetrics(SM_CXVSCROLL);
        
		DialogScalerAdjustChildren(Object, cx, cy);
        InvalidateRect(Object->hWnd, &rc, TRUE);
    }
    
	//
	// N.B. We use PostMessage to serialize the message processing
	// to avoid re-enter message procedure here.
	//

	if (Object->PostWmUserSize) {
		PostMessage(Object->hWnd, WM_USER_SIZE, 0, 0);
	}

	return 0L;
}

VOID
DialogScalerAdjustChildren(
	IN PDIALOG_OBJECT Object,
	IN int cx, 
	IN int cy
	)
{
    int dx, dy;
	UINT i;
	HWND hWndCtrl;
	DIALOG_SCALER *Scaler;
	DIALOG_SCALER_CHILD *Child;
	HDWP hdwp;
	RECT rc;

	Scaler = Object->Scaler;
    ASSERT(Scaler != NULL);

    hdwp = BeginDeferWindowPos(Scaler->Count);

    cx = max(cx, Scaler->MinClient.cx);
    cy = max(cy, Scaler->MinClient.cy);

    dx = cx - Scaler->Previous.cx;
    dy = cy - Scaler->Previous.cy;

    Scaler->Previous.cx = cx;
    Scaler->Previous.cy = cy;

    for(i = 0; i < Scaler->Count; i++) {

        Child = &Scaler->Children[i];
		hWndCtrl = GetDlgItem(Object->hWnd, Child->CtrlId);
    
		ASSERT(hWndCtrl != NULL);
        DialogScalerAdjustChild(Object, hWndCtrl, Child, dx, dy, &rc);

        hdwp = DeferWindowPos(hdwp, hWndCtrl, 0, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    }        

    EndDeferWindowPos(hdwp);
}    

VOID
DialogScalerAdjustChild(
	IN PDIALOG_OBJECT Object,
	IN HWND hWndCtl, 
	IN DIALOG_SCALER_CHILD *Child,
	IN int dx, 
	IN int dy, 
	OUT RECT *rc
	)
{
	DIALOG_SCALER *Scaler = Object->Scaler;
	ASSERT(Scaler != NULL);

    GetWindowRect(hWndCtl, rc);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, rc);

	if (Child->Horizonal & AlignLeft){
        rc->left += dx;
    }
	if (Child->Horizonal & AlignRight){
        rc->right += dx;
    }
    if (Child->Vertical & AlignTop){
        rc->top += dy;
    }
	if (Child->Vertical & AlignBottom){
        rc->bottom += dy;
    }

}

BOOLEAN
DialogRegisterScaler(
	IN DIALOG_OBJECT *Object
	)
{
	RECT rc;
	DIALOG_SCALER *Scaler;

	ASSERT(Object != NULL);
	ASSERT(IsWindow(Object->hWnd));

	//
	// N.B. WM_USER_SIZE is registered when first scaler dialog is created 
	//

	if (!WM_USER_SIZE) {
		WM_USER_SIZE = RegisterWindowMessage(L"WM_USER_SIZE");
	}

	Scaler = Object->Scaler;
    GetWindowRect(Object->hWnd, &rc);

    Scaler->MinTrack.cx = rc.right - rc.left;
    Scaler->MinTrack.cy = rc.bottom - rc.top;

    GetClientRect(Object->hWnd, &rc);
    Scaler->MinClient.cx = rc.right - rc.left;
    Scaler->MinClient.cy = rc.bottom - rc.top;

    Scaler->Previous.cx = rc.right - rc.left;
    Scaler->Previous.cy = rc.bottom - rc.top;

    return SetWindowSubclass(Object->hWnd, DialogScalerProcedure, 0, 0);
}

LRESULT 
DialogOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	DIALOG_OBJECT *Object = (PDIALOG_OBJECT)lp;
	ASSERT(Object != NULL);

	SetWindowLongPtr(Object->hWnd, GWLP_USERDATA, PtrToUlong(Object)); 

	if (Object->Scaler != NULL) {
		DialogRegisterScaler(Object);
	}

	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)SdkMainIcon);
	return (LRESULT)TRUE;
}

LRESULT
DialogOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

    switch (LOWORD(wp)) {
    
	case IDOK:
        EndDialog(hWnd, IDOK); 
        break;

    case IDCANCEL:
        EndDialog(hWnd, IDCANCEL);
        break;
    }

    return 0L;
}