//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#ifndef _DIALOG_H_
#define _DIALOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

typedef enum _DIALOG_SCALER_ALIGN {
    AlignNone   = 0, 
    AlignLeft   = 1, 
    AlignTop    = 1, 
    AlignRight  = 2, 
    AlignBottom = 2,
    AlignBoth   = 3
} DIALOG_SCALER_ALIGN;

typedef struct _DIALOG_SCALER_CHILD {
    int CtrlId;
    DIALOG_SCALER_ALIGN Horizonal;
    DIALOG_SCALER_ALIGN Vertical;
} DIALOG_SCALER_CHILD, *PDIALOG_SCALER_CHILD;

typedef struct _DIALOG_SCALER {
    SIZE Previous;
    SIZE MinTrack;
    SIZE MinClient;
    UINT Count;
    DIALOG_SCALER_CHILD *Children;
} DIALOG_SCALER, *PDIALOG_SCALER;
    
typedef struct _DIALOG_OBJECT {
	HWND hWnd;
	HWND hWndParent;
	UINT ResourceId;
	DLGPROC Procedure;
	PVOID Context;
	PVOID ObjectContext;
	PDIALOG_SCALER Scaler;
	BOOLEAN NoFrameControl;
	BOOLEAN PostWmUserSize;
} DIALOG_OBJECT, *PDIALOG_OBJECT;

typedef struct _DIALOG_RESULT {
	INT_PTR Status;
	PVOID Context;
} DIALOG_RESULT, *PDIALOG_RESULT;

INT_PTR
DialogCreate(
	IN PDIALOG_OBJECT Object
	);

HWND
DialogCreateModeless(
	IN PDIALOG_OBJECT Object
	);

INT_PTR CALLBACK 
DialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT CALLBACK 
DialogScalerProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp,
	IN UINT_PTR uIdSubclass, 
	IN DWORD_PTR dwData
	);

BOOLEAN
DialogRegisterScaler(
	IN PDIALOG_OBJECT Object
	);

LRESULT 
DialogOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
DialogOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
DialogScalerOnEraseBkgnd(
	IN PDIALOG_OBJECT Object,
	IN HDC hdc
	);

LRESULT 
DialogScalerOnNcHitTest(
	IN PDIALOG_OBJECT Object,
	IN int x, 
	IN int y
	);

LRESULT 
DialogScalerOnSizing(
	IN PDIALOG_OBJECT Object,
	IN UINT uEdge, 
	OUT RECT *lpRect
	);

LRESULT 
DialogScalerOnGetMinMaxInfo(
	IN PDIALOG_OBJECT Object,
	OUT LPMINMAXINFO lpMinMaxInfo
	);

LRESULT 
DialogScalerOnSize(
	IN PDIALOG_OBJECT Object,
	IN UINT nState, 
	IN int cx,
	IN int cy
	);

VOID
DialogScalerAdjustChildren(
	IN PDIALOG_OBJECT Object,
	IN int cx, 
	IN int cy
	);

VOID
DialogScalerAdjustChild(
	IN PDIALOG_OBJECT Object,
	IN HWND hWndCtl, 
	IN DIALOG_SCALER_CHILD *Child,
	IN int dx, 
	IN int dy, 
	OUT RECT *rc
	);

//
// N.B. If the scaler dialog box require further position
// its children, it can set PostWmUserSize to TRUE, scaler
// procedure will post a WM_USER_SIZE to its dialog procedure
// to notify it when WM_SIZE is completed.
//

extern int WM_USER_SIZE;

#ifdef __cplusplus
}
#endif

#endif