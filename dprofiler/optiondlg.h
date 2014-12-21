//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#ifndef _OPTIONDLG_H_
#define _OPTIONDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

typedef enum _OPTION_NODE_TYPE {
	OPTION_GENERAL,
    OPTION_SYMBOL_GENERAL,
    OPTION_SOURCE_GENERAL,
    OPTION_NUMBER,
} OPTION_NODE_TYPE;

typedef struct _OPTION_NODE {
    OPTION_NODE_TYPE Type; 
    DIALOG_OBJECT *Page;
} OPTION_NODE, *POPTION_NODE;

typedef struct _OPTION_CONTEXT {
	HFONT hFontOld;
	HFONT hFontBold;
	HBRUSH hBrushFocus;
	HBRUSH hBrushPane;
	DIALOG_OBJECT Pages[OPTION_NUMBER];
} OPTION_CONTEXT, *POPTION_CONTEXT;

VOID
OptionDialog(
	__in HWND hWndParent	
	);

INT_PTR CALLBACK
OptionProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
OptionOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
OptionOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
OptionOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
OptionOnSelChanged(
	__in HWND hWnd,
	__in LPNMTREEVIEW lp
	);

LRESULT
OptionOnCommand(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

VOID
OptionCommitData(
	__in HWND hWnd
	);

LRESULT
OptionCreateTree(
    __in PDIALOG_OBJECT Object
    );

VOID 
OptionPageAdjustPosition(
	__in HWND hWndParent,
	__in HWND hWndPage
	);

//
// Symbol
//

INT_PTR CALLBACK
OptionSymbolProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT
OptionSymbolOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
OptionSymbolOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
	);

LRESULT
OptionSymbolOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT
OptionSymbolOnEndLabelEdit(
    __in HWND hWndList, 
    __in NMLVDISPINFO *lpdi 
    );

LRESULT
OptionSymbolOnAdd(
	__in HWND hWnd
	);
	
LRESULT
OptionSymbolOnRemove(
	__in HWND hWnd
	);

VOID
OptionSymbolFillData(
	__in HWND hWnd
	);

VOID
OptionSymbolCommitData(
	__in HWND hWnd
	);

//
// Source
//

INT_PTR CALLBACK
OptionSourceProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT
OptionSourceOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
OptionSourceOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
	);

LRESULT
OptionSourceOnAdd(
    __in HWND hWnd
    );

LRESULT
OptionSourceOnRemove(
    __in HWND hWnd
    );

LRESULT
OptionSourceOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT
OptionSourceOnEndLabelEdit(
    __in HWND hWndList, 
    __in NMLVDISPINFO *lpdi 
    );

VOID
OptionSourceFillData(
	__in HWND hWnd
	);

VOID
OptionSourceCommitData(
	__in HWND hWnd
	);

//
// General
//

INT_PTR CALLBACK
OptionGeneralProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT
OptionGeneralOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
OptionGeneralOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
	);

LRESULT
OptionGeneralOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

VOID
OptionGeneralFillData(
	__in HWND hWnd
	);

VOID
OptionGeneralCommitData(
	__in HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif