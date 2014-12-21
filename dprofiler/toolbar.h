//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

enum ToolBarLimits {
	ToolBarNumber = 18,
};

typedef struct _TOOLBAR_OBJECT {
	BOOLEAN Show;
	HWND hWnd;
	HWND hWndParent;
	UINT CtrlId;
	ULONG Count;
	TBBUTTON *Button;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGray;
} TOOLBAR_OBJECT, *PTOOLBAR_OBJECT;

extern TBBUTTON ToolBarButtons[];
extern TOOLBAR_OBJECT ToolBarObject;

HWND
ToolBarCreate(
	IN PTOOLBAR_OBJECT ToolBar
	);

VOID
ToolBarDestroy(
	IN PTOOLBAR_OBJECT ToolBar
	);

BOOLEAN
ToolBarSetImageList(
	IN PTOOLBAR_OBJECT ToolBar,
	IN UINT BitmapId, 
	IN COLORREF Mask
	);

BOOLEAN
ToolBarSetImageList2(
	IN PTOOLBAR_OBJECT ToolBar,
	OUT PUINT ResourceId, 
	IN ULONG Count,
	IN COLORREF Mask
	);

BOOLEAN
ToolBarEnable(
	IN PTOOLBAR_OBJECT ToolBar,
	IN ULONG Id
	);

BOOLEAN
ToolBarDisable(
	IN PTOOLBAR_OBJECT ToolBar,
	IN ULONG Id
	);

LONG
ToolBarGetState(
	IN PTOOLBAR_OBJECT ToolBar,
	IN ULONG Id
	);

#ifdef __cplusplus
}
#endif

#endif