//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _TREELIST_H_
#define _TREELIST_H_

#include "sdk.h"
#include "treelistdata.h"
#include "listview.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDK_TREELIST_CLASS  L"SdkTreeList"

#define TREELIST_TREE_ID	1
#define TREELIST_HEADER_ID  2

typedef int 
(CALLBACK *TREELIST_COMPARE_CALLBACK)(
	__in LPARAM First, 
	__in LPARAM Second, 
	__in LPARAM Current 
	);

#define MAX_HEADER_NAME 32

typedef struct _HEADER_COLUMN {
	RECT Rect;
	LONG Width;
	LONG Align;
	BOOL Hide;
	BOOL ExpandTab;
	WCHAR Name[MAX_HEADER_NAME];
} HEADER_COLUMN, *PHEADER_COLUMN;

typedef struct _TREELIST_OBJECT {

	HWND hWnd;
	BOOLEAN IsInDialog;
	INT_PTR MenuOrId;

	HWND hWndHeader;
	LONG HeaderId;
	LONG HeaderWidth;
	LONG ColumnCount;
	HEADER_COLUMN Column[16];

	HWND hWndTree;
	LONG TreeId;
	COLORREF clrBack;
	COLORREF clrText;
	COLORREF clrHighlight;
	HBRUSH hBrushBack;
	HBRUSH hBrushNormal;
	HBRUSH hBrushBar;

	LONG ScrollOffset;

	LONG SplitbarLeft;
	LONG SplitbarBorder;

	LIST_SORT_ORDER SortOrder;
	int LastClickedColumn;
	HWND hWndSort;

	TREELIST_FORMAT_CALLBACK FormatCallback;

	RECT ItemRect;
	BOOLEAN ItemFocus;
	PVOID Context;

	RECT MinRect;
	BOOLEAN IsHScrolling;

} TREELIST_OBJECT, *PTREELIST_OBJECT;

BOOLEAN
TreeListInitialize(
	VOID
	);

PTREELIST_OBJECT
TreeListCreate(
	__in HWND hWndParent,
	__in BOOLEAN IsInDialog,
	__in INT_PTR MenuOrId,
	__in RECT *Rect,
	__in HWND hWndSort,
	__in TREELIST_FORMAT_CALLBACK Format,
	__in ULONG ColumnCount
	);

LRESULT CALLBACK 
TreeListProcedure(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

LRESULT CALLBACK 
TreeListTreeProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
	);

ULONG
TreeListCreateControls(
	__in PTREELIST_OBJECT Object
	);

LRESULT
TreeListOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
TreeListOnDestroy(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TreeListOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TreeListOnCustomDraw(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnBeginTrack(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnTrack(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnEndTrack(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnDividerDbclk(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnItemClick(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

VOID
TreeListGetTreeRectMostRight(
	__in PTREELIST_OBJECT Object,
	__out PLONG MostRight
	);

VOID
TreeListGetColumnTextExtent(
	__in PTREELIST_OBJECT Object,
	__in ULONG Column,
	__out PLONG Extent 
	);

LONG 
TreeListDrawVerticalLine(
	__in PTREELIST_OBJECT Object,
	__in LONG x
	);

VOID
TreeListDrawSplitBar(
	__in PTREELIST_OBJECT Object,
	__in LONG x,
	__in LONG y,
	__in BOOLEAN EndTrack
	);

VOID 
TreeListDrawItem(
	__in PTREELIST_OBJECT Object,
	__in HDC hdc, 
	__in HTREEITEM hTreeItem,
	__in LPARAM lp
	);

LRESULT
TreeListOnDeleteItem(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnItemExpanding(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	);

LRESULT
TreeListOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TreeListOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TreeListTreeOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

LRESULT
TreeListOnTimer(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

VOID
TreeListOnHScroll(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

VOID
TreeListResetScrollBar(
	__in PTREELIST_OBJECT Object
	);

LONG
TreeListInsertColumn(
	__in PTREELIST_OBJECT Object,
	__in LONG Column, 
	__in LONG Align,
	__in LONG Width,
	__in PWSTR Title
	);

#ifdef __cplusplus
}
#endif
#endif