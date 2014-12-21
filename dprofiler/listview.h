//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2011
// 

#ifndef _LISTVIEW_H_
#define _LISTVIEW_H_

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _LIST_DATA_TYPE {
    DataTypeInt,
    DataTypeUInt,
    DataTypeInt64,
    DataTypeUInt64,
    DataTypeFloat,
    DataTypeBool,
    DataTypeTime,
    DataTypeText
} LIST_DATA_TYPE;

typedef enum _LIST_SORT_ORDER {
    SortOrderDescendent = 0,
    SortOrderAscendent  = 1,
    SortOrderNone = -1
} LIST_SORT_ORDER;

typedef struct _LISTVIEW_COLUMN {
	ULONG Width;
	PWSTR Title;
	LONG Align;
	LONG Image;
	BOOLEAN Sortable;
    BOOLEAN Selected;  
    COLORREF ForeColor;
    COLORREF BackColor;
	COLORREF FontColor;
	LIST_DATA_TYPE DataType;
} LISTVIEW_COLUMN, *PLISTVIEW_COLUMN;

typedef LRESULT
(*MESSAGE_CALLBACK)(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

typedef struct _LISTVIEW_OBJECT {

	HWND hWnd;
	ULONG_PTR CtrlId;
	PLISTVIEW_COLUMN Column;
	ULONG Count;

	HIMAGELIST hImlSmallIcon;
	HIMAGELIST hImlNormalIcon;
	HIMAGELIST hImlStateIcon;

	LIST_SORT_ORDER SortOrder;
	int LastClickedColumn;

	MESSAGE_CALLBACK NotifyCallback;

	PVOID Context;

} LISTVIEW_OBJECT, *PLISTVIEW_OBJECT;

typedef struct _LISTVIEW_EDIT_CONTEXT {
	HWND hWndParent;
	HWND hWndListCtrl;
	LONG Item;
	LONG SubItem;
	RECT SubItemRect;
	BOOL Editing;
} LISTVIEW_EDIT_CONTEXT, *PLISTVIEW_EDIT_CONTEXT;

//
// from commctrl.h
// #define INDEXTOSTATEIMAGEMASK(i) ((i) << 12)
//

#define STATEIMAGEMASKTOINDEX(_S)   ((_S) >> 12)

#define CHECKED   2 
#define UNCHECKED 1

#define ListViewIsChecked(_S) \
	(((_S) >> 12) == CHECKED ? TRUE : FALSE)


PLISTVIEW_EDIT_CONTEXT
ListViewAllocateEditContext(
	IN HWND hWndParent,
	IN HWND hWndListCtrl
	);

LRESULT
ListViewSelectSingle(
	IN HWND hWnd,
	IN LONG Item
	);

LONG
ListViewGetFirstSelected(
	IN HWND hWnd
	);

LONG
ListViewGetNextSelected(
	IN HWND hWnd,
	IN LONG From
	);

BOOLEAN
ListViewGetCheck(
	IN HWND hWnd,
	IN LONG Item
	);

BOOLEAN
ListViewIsSelected(
	IN HWND hWnd,
	IN LONG Index
	);

BOOLEAN
ListViewIsFocused(
	IN HWND hWnd,
	IN LONG Index
	);

BOOLEAN 
ListViewSetCheck(
	IN HWND hWnd,
	IN LONG Item, 
	IN BOOLEAN Check
	);

BOOLEAN 
ListViewGetParam(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LPARAM *Param 
	);

BOOLEAN
ListViewSetParam(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LPARAM Param 
	);
	  
HWND 
ListViewCreateReport(
	IN HWND hWndParent,
	IN PLISTVIEW_OBJECT Object	
	);


#ifdef __cplusplus
}
#endif

#endif