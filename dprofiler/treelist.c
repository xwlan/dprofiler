//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#include "treelist.h"
#include "treelistdata.h"

//
// if you use treelist as custom control in dialog template,
// fill the following values:
// style 0x54100000, extended style 0x00010200 
//

#define TREELIST_TREE_STYLE  WS_CHILD|WS_HSCROLL|TVS_HASLINES|TVS_HASBUTTONS|\
							 TVS_LINESATROOT|TVS_SHOWSELALWAYS

#define TREELIST_HEADER_STYLE WS_VISIBLE|WS_CHILD|HDS_BUTTONS|HDS_HORZ|HDS_FULLDRAG



//
// N.B. 17 is standard listview control header's height
//

#define STD_LIST_HEADER_HEIGHT 17

BOOLEAN
TreeListInitialize(
	VOID
	)
{
	WNDCLASSEX wc;
	ATOM Atom;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);

	SdkMainIcon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_SMALL));
    
	wc.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = TreeListProcedure;
    wc.lpszClassName  = SDK_TREELIST_CLASS;

    Atom = RegisterClassEx(&wc);
	ASSERT(Atom != 0);

	return Atom ? TRUE : FALSE;
}

PTREELIST_OBJECT
TreeListCreate(
	__in HWND hWndParent,
	__in BOOLEAN IsInDialog,
	__in INT_PTR MenuOrId,
	__in RECT *Rect,
	__in HWND hWndSort,
	__in TREELIST_FORMAT_CALLBACK Format,
	__in ULONG ColumnCount
	)
{
	HWND hWnd;
	PTREELIST_OBJECT Object;
	LONG Style;
	LONG ExStyle;

	Object = (PTREELIST_OBJECT)SdkMalloc(sizeof(TREELIST_OBJECT));
	ZeroMemory(Object, sizeof(TREELIST_OBJECT));

	Object->HeaderId = TREELIST_HEADER_ID;
	Object->TreeId = TREELIST_TREE_ID;
	Object->ColumnCount = 0;
	Object->SplitbarLeft = -1;
	Object->SplitbarBorder = 1;
	Object->MenuOrId = MenuOrId;
	Object->hWndSort = hWndSort;
	Object->FormatCallback = Format;

	//
	// If it's in dialog template, disable its horizonal scrollbar,
	// since we use treeview's scrollbar
	//

	if (IsInDialog) {

		hWnd = GetDlgItem(hWndParent, MenuOrId);
		ASSERT(hWnd != NULL);

		SdkModifyStyle(hWnd, WS_HSCROLL, 0, FALSE);
		ShowScrollBar(hWnd, SB_HORZ, FALSE);

		return Object;
	}

	Style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS;
	ExStyle = WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT;

    hWnd = CreateWindowEx(ExStyle, SDK_TREELIST_CLASS, L"", Style,
						  Rect->left, Rect->top, Rect->right - Rect->left, 
						  Rect->bottom - Rect->top, hWndParent, (HMENU)MenuOrId, 
						  SdkInstance, (PVOID)Object);

	//
	// remove horizonal scrollbar
	//

	SdkModifyStyle(hWnd, WS_HSCROLL, 0, FALSE);
	ShowScrollBar(hWnd, SB_HORZ, FALSE);

	Object->hWnd = hWnd;
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	return Object;
}

LRESULT CALLBACK 
TreeListProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	switch (uMsg) {

		case WM_NOTIFY:
		    return TreeListOnNotify(hWnd, uMsg, wp, lp);

		case WM_CREATE:
			TreeListOnCreate(hWnd, uMsg, wp, lp);
			break;

		case WM_SIZE:
			return TreeListOnSize(hWnd, uMsg, wp, lp);

		case WM_DESTROY:
			TreeListOnDestroy(hWnd, uMsg, wp, lp);
			break;

		case WM_HSCROLL:
	        TreeListOnHScroll(hWnd, uMsg, wp, lp);
			break;

		case WM_TREELIST_DBLCLK:
			PostMessage(GetParent(hWnd), uMsg, wp, lp);
			return 0;
	}

	return DefWindowProc(hWnd, uMsg, wp, lp);
}

int 
TreeListHitTestColumn(
	__in PTREELIST_OBJECT Object,
	__in LPTVHITTESTINFO hti
	)
{
	int i;

	for(i = 0; i < Object->ColumnCount; i++) {
		if (hti->pt.x >= Object->Column[i].Rect.left && 
			hti->pt.x < Object->Column[i].Rect.right) {
			return i;
		}
	}

	return -1;
}

LRESULT CALLBACK 
TreeListTreeProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	TVHITTESTINFO hti;
	WORD x, y;
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	PTREELIST_OBJECT Object = (PTREELIST_OBJECT)dwData;
	int Column;

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK || 
		uMsg == WM_RBUTTONDOWN) {

		x = GET_X_LPARAM(lp); 
		y = GET_Y_LPARAM(lp);
		hti.pt.x = x;
		hti.pt.y = y;

		TreeView_HitTest(hWnd, &hti);
		if(hti.flags == TVHT_ONITEMRIGHT) {

			//
			// Select current row
			//

            TreeView_Select(hWnd, hti.hItem, TVGN_CARET);

			//
			// Post notification to parent window
			//

			Column = TreeListHitTestColumn(Object, &hti);
			if(Column != -1) {
				PostMessage(Object->hWnd, WM_TREELIST_DBLCLK, 
					       (WPARAM)hti.hItem, (LPARAM)Column);	
			}
		}
	}

	if (uMsg == WM_HSCROLL) {
        TreeListOnHScroll(hWnd, uMsg, wp, lp);
		return 0;
	}

	/*
	if (uMsg == WM_NOTIFY) {
		
		if(Object->hWndHeader == pNmhdr->hwndFrom) {

			switch (pNmhdr->code) {

			case HDN_BEGINTRACK:
				return TreeListOnBeginTrack(Object, pNmhdr);

			case HDN_TRACK:
				return TreeListOnTrack(Object, pNmhdr);

			case HDN_ENDTRACK:
				return TreeListOnEndTrack(Object, pNmhdr);

			case HDN_DIVIDERDBLCLICK:
				return TreeListOnDividerDbclk(Object, pNmhdr);

			case HDN_ITEMCLICK:
				return TreeListOnItemClick(Object, pNmhdr);
			}
		}
	}
	

	if (uMsg == WM_SIZE) {
		return TreeListTreeOnSize(hWnd, uMsg, wp, lp);
	}*/

    return DefSubclassProc(hWnd, uMsg, wp, lp);
} 

ULONG
TreeListCreateControls(
	__in PTREELIST_OBJECT Object
	)
{
	HWND hWnd;
	HWND hWndTree;
	HWND hWndHeader;
	HFONT hFont;

	ASSERT(Object->hWnd != NULL);
	hWnd = Object->hWnd;
	
	//
	// Create treeview control
	//

	hWndTree = CreateWindowEx(0, WC_TREEVIEW, NULL, TREELIST_TREE_STYLE, 
							  0, 0, 0, 0, 
                              hWnd, (HMENU)TREELIST_TREE_ID,
							  SdkInstance, NULL);
	Object->hWndTree = hWndTree;
	TreeView_SetUnicodeFormat(hWndTree, TRUE);

	// 
	// Enable scrollbar for treeview
	//

	//SdkModifyStyle(hWndTree, 0, WS_HSCROLL, FALSE);

	//
	// Create header control
	//

	hWndHeader = CreateWindowEx(0, WC_HEADER, NULL, TREELIST_HEADER_STYLE,
								0, 0, 0, 0, hWnd, (HMENU)TREELIST_HEADER_ID, 
								SdkInstance, NULL);
	Object->hWndHeader = hWndHeader;
	Header_SetUnicodeFormat(hWndHeader, TRUE);

	//
	// Set header's font type as treeview's 
	//

	hFont = (HFONT)SendMessage(hWndTree, WM_GETFONT, 0, 0);
	SendMessage(hWndHeader, WM_SETFONT, (WPARAM)hFont, TRUE);

    //
    // Subclass tree procedure
    //

	SetWindowSubclass(hWndTree, TreeListTreeProcedure, 0, (DWORD_PTR)Object);

	Object->ScrollOffset = 0;
	Object->clrBack = GetSysColor(COLOR_WINDOW);
	Object->clrText = GetSysColor(COLOR_WINDOWTEXT);
	Object->clrHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
	Object->hBrushBack = GetSysColorBrush(COLOR_HIGHLIGHT);
	Object->hBrushNormal = GetSysColorBrush(COLOR_WINDOW);
	Object->hBrushBar = (HBRUSH)GetStockObject(BLACK_BRUSH);	

	ShowWindow(hWndTree, SW_SHOW);
	return 0;
}

LRESULT
TreeListOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
TreeListOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	PTREELIST_OBJECT Object;

	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);

    //
    // Tree notification
    //

	if(Object->hWndTree == pNmhdr->hwndFrom) {

		switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return TreeListOnCustomDraw(Object, pNmhdr);

		case NM_DBLCLK:
			PostMessage(GetParent(Object->hWnd), uMsg, wp, lp);
			break;

		case TVN_DELETEITEM:
			return TreeListOnDeleteItem(Object, pNmhdr);

		//case TVN_ITEMEXPANDING :
		//	return TreeListOnItemExpanding(Object, pNmhdr);

		case TVN_ITEMEXPANDED :
			return TreeListOnItemExpanding(Object, pNmhdr);

		case TVN_SELCHANGED:
			{
				LPNMTREEVIEW lpnmtv;
				lpnmtv = (LPNMTREEVIEW)SdkMalloc(sizeof(NMTREEVIEW));
				memcpy(lpnmtv, (PVOID)lp, sizeof(NMTREEVIEW));
				SendMessage(GetParent(hWnd), WM_TREELIST_SELCHANGED, 0, (LPARAM)lpnmtv);
			}
			return 0;
		}
	}

    //
    // Header notification
    //

	else if(Object->hWndHeader == pNmhdr->hwndFrom) {

		switch (pNmhdr->code) {

		case HDN_BEGINTRACK:
			return TreeListOnBeginTrack(Object, pNmhdr);

		case HDN_TRACK:
			return TreeListOnTrack(Object, pNmhdr);

		case HDN_ENDTRACK:
			return TreeListOnEndTrack(Object, pNmhdr);

		case HDN_DIVIDERDBLCLICK:
			return TreeListOnDividerDbclk(Object, pNmhdr);

		case HDN_ITEMCLICK:
			return TreeListOnItemClick(Object, pNmhdr);
		}
	}

	return 0;
}

LRESULT
TreeListOnCustomDraw(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	LRESULT Status;
	LPNMTVCUSTOMDRAW lpcd = (LPNMTVCUSTOMDRAW)lp;

	Status = CDRF_DODEFAULT;

	switch (lpcd->nmcd.dwDrawStage) {

		case CDDS_PREPAINT:

			//SetViewportOrgEx(lpcd->nmcd.hdc, Object->ScrollOffset, 0, NULL);
			Status = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			
            //
            // Track item information for custom drawing after post painting stage
            //

			lpcd->clrText = Object->clrText;
			lpcd->clrTextBk = Object->clrBack;

			Object->ItemFocus = FALSE;
			if(lpcd->nmcd.uItemState & CDIS_FOCUS) {
				Object->ItemFocus = TRUE;
			}

			TreeView_GetItemRect(Object->hWndTree, (HTREEITEM)lpcd->nmcd.dwItemSpec, 
				                 &Object->ItemRect, TRUE);

			Object->ItemRect.right = min(lpcd->nmcd.rc.right, Object->HeaderWidth);
			Status = CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPOSTPAINT:

			TreeListDrawItem(Object, lpcd->nmcd.hdc, 
                             (HTREEITEM)lpcd->nmcd.dwItemSpec, 
                             lpcd->nmcd.lItemlParam);

            Status = CDRF_SKIPDEFAULT; 
			break;
	}

	return Status;
}

VOID 
TreeListDrawItem(
	__in PTREELIST_OBJECT Object,
	__in HDC hdc, 
	__in HTREEITEM hTreeItem,
	__in LPARAM lp
	)
{
	RECT rc;
	RECT Rect;
	LONG i;
	UINT Format;
	WCHAR Buffer[MAX_PATH];

	if (!lp) {
		return;
	}

	Rect = Object->ItemRect;

	if(Object->ItemFocus) {
		FillRect(hdc, &Rect, Object->hBrushBack);
		DrawFocusRect(hdc, &Rect);
	}
	else {
		FillRect(hdc, &Rect, Object->hBrushNormal);
	}

	//
	// Always write text as transparent 
	//

	SetBkMode(hdc, TRANSPARENT);

	if (Object->ItemFocus) {
		SetTextColor(hdc, Object->clrHighlight);
	} else {
		SetTextColor(hdc, Object->clrText);
	}

	rc = Rect;

	for(i = 0; i < Object->ColumnCount; i++) {

		if(i != 0) {
			rc.left = Object->Column[i].Rect.left;
		}
		rc.right = Object->Column[i].Rect.right;

		/*
		if (i == 0) { 
			if (rc.right > Object->Column[0].Rect.right) {
				rc.right = Object->Column[0].Rect.right;
				continue;
			}
			if (rc.left > rc.right) {
				rc.left = Object->Column[0].Rect.right;
				continue;
			}
		}
		*/

		TreeListFormatValue(Object, Object->FormatCallback, hTreeItem, (PVOID)lp, i, Buffer, MAX_PATH);
		if (!Object->Column[i].ExpandTab) {
			Format = DT_SINGLELINE|DT_WORD_ELLIPSIS|Object->Column[i].Align;
		} else {
			//Format = DT_SINGLELINE|DT_EXPANDTABS|Object->Column[i].Align;
			Format = DT_SINGLELINE|DT_EXPANDTABS;
		}
		DrawText(hdc, Buffer, -1, &rc, Format);
	}
}

VOID
TreeListDrawSplitBar(
	__in PTREELIST_OBJECT Object,
	__in LONG x,
	__in LONG y,
	__in BOOLEAN EndTrack
	)
{
    HDC hdc;
	RECT Rect;
	HBRUSH hBrushOld;

	hdc = GetDC(Object->hWnd);
	if (!hdc) {
        return;
	}

	GetClientRect(Object->hWnd, &Rect);
	hBrushOld = SelectObject(hdc, Object->hBrushBar);

    //
    // Clean the splibar we drew last time 
    //

	if (Object->SplitbarLeft > 0) {
		PatBlt(hdc, Object->SplitbarLeft, y, 
			   Object->SplitbarBorder, 
			   Rect.bottom - Rect.top - y, 
			   PATINVERT );
    }

    if (!EndTrack) {

        Rect.left += x;
		PatBlt(hdc, Rect.left, y, Object->SplitbarBorder, 
			   Rect.bottom - Rect.top - y, PATINVERT);

		Object->SplitbarLeft = Rect.left;

    }
    else {

		Object->SplitbarLeft = 0xffff8001;
    }

	SelectObject(hdc, hBrushOld);
	ReleaseDC(Object->hWnd, hdc);
}

LRESULT
TreeListOnBeginTrack(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	RECT Rect;
	LONG x;

	phdn = (LPNMHEADER)lp;
	Header_GetItemRect(Object->hWndHeader, phdn->iItem, &Rect);

	x = Rect.left + phdn->pitem->cxy;

	TreeListDrawVerticalLine(Object, x);
	Object->SplitbarLeft = x;
	return 0;
}

LRESULT
TreeListOnTrack(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	RECT Rect;
	LONG x;
	LONG y;

	phdn = (LPNMHEADER)lp;
	Header_GetItemRect(Object->hWndHeader, phdn->iItem, &Rect);

	x = Rect.left + phdn->pitem->cxy;
	y = Rect.bottom - Rect.top;

	if (Object->SplitbarLeft != -1) {
		TreeListDrawVerticalLine(Object, Object->SplitbarLeft);
		TreeListDrawVerticalLine(Object, x);
		Object->SplitbarLeft = x;
	}

	return 0;
}

LRESULT
TreeListOnEndTrack(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	if (Object->SplitbarLeft != -1) {
		TreeListDrawVerticalLine(Object, Object->SplitbarLeft);
		PostMessage(Object->hWnd, WM_SIZE, 0, 0);
	}

	return 0;
}

VOID
TreeListRedrawColumnArea(
    __in PTREELIST_OBJECT Object
    )
{
    //
    // Draw only column area
    //
}

LONG 
TreeListDrawVerticalLine(
	__in PTREELIST_OBJECT Object,
	__in LONG x
	)
{
	HWND hWnd;
	RECT Rect;
	HDC hdc;
	LONG y;
	HGDIOBJ hOld;

	hWnd = Object->hWnd;
	hdc = GetDC(hWnd);
	GetClientRect(hWnd, &Rect);

	y = Rect.bottom - Rect.top;

	SetROP2(hdc, R2_NOTXORPEN);
	hOld = SelectObject(hdc, GetStockObject(BLACK_PEN));

	MoveToEx(hdc, x, STD_LIST_HEADER_HEIGHT, NULL);
	LineTo(hdc, x, y);

	SelectObject(hdc, hOld);
	ReleaseDC(hWnd, hdc);

	return x;
}

LRESULT
TreeListOnDividerDbclk(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	LONG x;
	HDITEM Item = {0};
	int Length;
	SIZE Size;
	HDC hdc;
    int cxEdge;
    int cxIcon;
    
	phdn = (LPNMHEADER)lp;
    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cxIcon = GetSystemMetrics(SM_CXSMICON);

	if (phdn->iItem == 0) {
		TreeListGetTreeRectMostRight(Object, &x);
	}
	else {

		TreeListGetColumnTextExtent(Object, phdn->iItem, &x);

		//
		// Compute header's text extent length
		//
		
		hdc = GetDC(Object->hWndTree);
		Length = (int)wcslen(Object->Column[phdn->iItem].Name);
		GetTextExtentPoint32(hdc, Object->Column[phdn->iItem].Name, (int)Length, &Size);
		x = max(x, Size.cx + cxEdge * 2 + cxIcon);
		ReleaseDC(Object->hWndTree, hdc);
	}

	Item.mask = HDI_ORDER | HDI_WIDTH;
	Item.iOrder = phdn->iItem;
	Item.cxy = x;

	Header_SetItem(Object->hWndHeader, phdn->iItem, &Item);
	PostMessage(Object->hWnd, WM_SIZE, 0, 0);

	return 0;
}

LRESULT
TreeListOnItemClick(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	ULONG ColumnCount;
	ULONG Current, i;
	HDITEM hdi = {0};

	if (!Object->hWndSort) {
		return 0;
	}

	phdn = (LPNMHEADER)lp;
	Current = phdn->iItem;

	if (Object->LastClickedColumn == Current) {
		Object->SortOrder = (LIST_SORT_ORDER)!Object->SortOrder;
    } else {
		Object->SortOrder = SortOrderAscendent;
    }
    
    ColumnCount = Header_GetItemCount(Object->hWndHeader);
    
    for (i = 0; i < ColumnCount; i++) {

        hdi.mask = HDI_FORMAT;
        Header_GetItem(Object->hWndHeader, i, &hdi);
        
        hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);

        if (i == Current) {

            if (Object->SortOrder == SortOrderAscendent){
                hdi.fmt |= HDF_SORTUP;
            } else {
                hdi.fmt |= HDF_SORTDOWN;
            }

        } 
        
        Header_SetItem(Object->hWndHeader, i, &hdi);
    }
    
	Object->LastClickedColumn = Current;
	PostMessage(Object->hWndSort, WM_TREELIST_SORT, (WPARAM)Current, 0);
	return 0;
}

VOID
TreeListGetColumnTextExtent(
	__in PTREELIST_OBJECT Object,
	__in ULONG Column,
	__out PLONG Extent 
	)
{
	HTREEITEM hTreeItem;
	HWND hWndTree;
	SIZE Size;
	SIZE_T Length;
	HDC hdc;
	TVITEM tvi = {0};
    int cxEdge;
	WCHAR Buffer[MAX_PATH];

	*Extent = 0;
    cxEdge = GetSystemMetrics(SM_CXEDGE);

	hWndTree = Object->hWndTree;
	hdc = GetDC(Object->hWndTree);

	hTreeItem = TreeView_GetFirstVisible(hWndTree);

	while (hTreeItem != NULL) {

		tvi.mask = TVIF_PARAM;
		tvi.hItem = hTreeItem;
		TreeView_GetItem(hWndTree, &tvi);

		TreeListFormatValue(Object, Object->FormatCallback, hTreeItem, 
			               (PVOID)tvi.lParam, Column, Buffer, MAX_PATH);

		StringCchLength(Buffer, MAX_PATH, &Length);
		GetTextExtentPoint32(hdc, Buffer, (int)Length, &Size);

		*Extent = max(*Extent, Size.cx + cxEdge * 2);

		hTreeItem = TreeView_GetNextVisible(hWndTree, hTreeItem);
	}
	
	ReleaseDC(Object->hWndTree, hdc);
}

VOID
TreeListGetTreeRectMostRight(
	__in PTREELIST_OBJECT Object,
	__out PLONG MostRight
	)
{
	HTREEITEM hTreeItem;
	HWND hWndTree;
	RECT Rect;

	hWndTree = Object->hWndTree;
	hTreeItem = TreeView_GetFirstVisible(hWndTree);
	*MostRight = 0;

	while (hTreeItem != NULL) {
		TreeView_GetItemRect(hWndTree, hTreeItem, &Rect, TRUE);
		*MostRight = max(Rect.right, *MostRight);
		hTreeItem = TreeView_GetNextVisible(hWndTree, hTreeItem);
	}
}

LRESULT
TreeListOnDeleteItem(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
	NMTREEVIEW* lptv = (NMTREEVIEW*)lp;
	return TRUE;
}

LRESULT
TreeListOnItemExpanding(
	__in PTREELIST_OBJECT Object,
	__in LPNMHDR lp
	)
{
//#if defined (_M_X64)

	//
	// N.B. 64 bits tree custom draw has problem, we work around
	// by forcing redraw client area, 32 bits has no such problem,
	// it's under low priority investigation.
	//

	RECT Rect;
	LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lp;

	if (lpnmtv->action = TVE_EXPAND) {
	}

	GetClientRect(Object->hWndTree, &Rect);
	InvalidateRect(Object->hWndTree, &Rect, TRUE);

//#endif

	return FALSE;	
}

LRESULT
TreeListOnDestroy(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	return 0;
}

VOID
TreeListComputeClientArea(
	__in RECT *Base,
	__in RECT *Tree,
	__in RECT *Header
	)
{
	//
	// N.B. We may dynamically compute the header's height
	// when initialize by create a fake report view and get its
	// header height, this ensure that all views's header has
	// same height.
	//

	*Header = *Base;
	Header->bottom = Header->top + STD_LIST_HEADER_HEIGHT;

	*Tree = *Base;
	Tree->top = Header->bottom + 1;
}

VOID
TreeListUpdateColumn(
	__in PTREELIST_OBJECT Object	
	)
{
	HDITEM hdi;
	LONG i;
	
	Object->HeaderWidth = 0;

	for(i = 0; i < Object->ColumnCount; i++) {

		hdi.mask = HDI_WIDTH | HDI_LPARAM | HDI_FORMAT;
		Header_GetItem(Object->hWndHeader, i, &hdi);
		Object->Column[i].Width = hdi.cxy;

		if(i == 0) {
			//Object->Column[i].Rect.left = 2;
			Object->Column[i].Rect.left = 0;
		}
		else {
			//Object->Column[i].Rect.left = Object->Column[i - 1].Rect.right + 2;
			Object->Column[i].Rect.left = Object->Column[i - 1].Rect.right;
		}

		Object->Column[i].Rect.right = Object->Column[i].Rect.left  + 
			                           //Object->Column[i].Width - 2;
			                           Object->Column[i].Width;

		switch(hdi.fmt & HDF_JUSTIFYMASK) {

		case HDF_RIGHT:
			Object->Column[i].Align = DT_RIGHT;
			break;

		case HDF_CENTER:
			Object->Column[i].Align = DT_CENTER;
			break;
		case HDF_LEFT:
			Object->Column[i].Align = DT_LEFT;
			break;

		default:
			Object->Column[i].Align = DT_LEFT;
		}

		Object->HeaderWidth += Object->Column[i].Width;
	}

	//
	// Adjust right column width according to header's bounding rect
	//

	/*GetWindowRect(Object->hWndHeader, &rc);
	Delta = rc.right - rc.left - Object->HeaderWidth;
	if (Delta > 0) {
		i -= 1;
		Object->Column[i].Width += Delta;
		Object->HeaderWidth += Delta;
		hdi.mask = HDI_WIDTH;
		hdi.cxy = Object->Column[i].Width;
		Header_SetItem(Object->hWndHeader, i, &hdi);
	}*/
}

LRESULT
TreeListOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PTREELIST_OBJECT Object;
	RECT Base;
	RECT Tree;
	RECT Header;
	WPARAM Code;
	LONG Offset;
	int x = LOWORD(lp);
	int y = HIWORD(lp);


	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);
	if (!Object) {
		return 0;
	}
	GetClientRect(hWnd, &Base);

	Offset = Object->ScrollOffset;


	//
	// N.B. scrollbar API can cause WM_SIZE to be triggered,
	// we need avoid this by postpone the API calls after
	// WM_SIZE is processed.
	//


    //
    // Compute the new positions of header and tree control 
    //

	TreeListComputeClientArea(&Base, &Tree, &Header);

	SetWindowPos(Object->hWndHeader, HWND_TOP, 
		         Header.left, Header.top,
				 Header.right - Header.left,
				 Header.bottom - Header.top, 
				 SWP_NOZORDER);

	SetWindowPos(Object->hWndTree, HWND_TOP, 
		         Tree.left, Tree.top, 
				 Tree.right - Tree.left,
				 Tree.bottom - Tree.top,
				 SWP_NOZORDER);
	
	TreeListResetScrollBar(Object);
	TreeListUpdateColumn(Object);

    //
    // Set scrollbar position
    //

	Code = (Offset << 16)  | (SB_THUMBPOSITION);
	SendMessage(Object->hWnd, WM_HSCROLL, Code, 0);
	
	GetClientRect(Object->hWnd, &Base); 
	InvalidateRect(Object->hWnd, &Base, TRUE);
	return 0;
}

LRESULT
TreeListTreeOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PTREELIST_OBJECT Object;
	RECT Base;
	RECT Tree;
	RECT Header;
	WPARAM Code;
	LONG Offset;
	
	Object = (PTREELIST_OBJECT)SdkGetObject(GetParent(hWnd));
	if (!Object) {
		return 0;
	}

	Offset = Object->ScrollOffset;

	GetClientRect(Object->hWndTree, &Tree); 
	InvalidateRect(Object->hWndTree, &Tree, TRUE);

	TreeListResetScrollBar(Object);
	TreeListUpdateColumn(Object);

    //
    // Compute the new positions of header and tree control 
    //

	GetClientRect(Object->hWnd, &Base);
	TreeListComputeClientArea(&Base, &Tree, &Header);

	SetWindowPos(Object->hWndHeader, HWND_TOP, 
		         Header.left, Header.top,
				 Header.right - Header.left,
				 Header.bottom - Header.top, 
				 SWP_NOZORDER);

    //
    // Set scrollbar position
    //

	Code = (Offset << 16)  | (SB_THUMBPOSITION);
	SendMessage(Object->hWndTree, WM_HSCROLL, Code, 0);
	return 0;
}

LRESULT
TreeListOnTimer(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	return 0;
}

void
TreeListGetTextMetrics(
	__in HWND hWnd,
	__out int *CharWidth,
	__out int *CharHeight,
	__out int *UpperWidth
	)
{ 
	HDC hdc;
	TEXTMETRIC tm; 

	hdc = GetDC(hWnd); 
 
	//
	// refer to MSDN SCROLLBAR
	//

	GetTextMetrics(hdc, &tm); 

	if (CharWidth) {
		*CharWidth = tm.tmAveCharWidth; 
	}

	if (CharHeight) {
		*CharHeight = tm.tmHeight + tm.tmExternalLeading; 
	}

	if (UpperWidth) {
		*UpperWidth = (tm.tmPitchAndFamily & 1 ? 3 : 2) * tm.tmAveCharWidth / 2; 
	}

	ReleaseDC(hWnd, hdc);
}

int
TreeListGetHeaderWidth(
	__in PTREELIST_OBJECT Object
	)
{
	int i;
	int Count;
	int Width;
	HDITEM Item;
	RECT Rect;
	
	ASSERT(Object != NULL);

	Item.mask = HDI_WIDTH;
	Width = 0;

	Count = Header_GetItemCount(Object->hWndHeader);
	for(i = 0; i < Count; i++) {
		Header_GetItem(Object->hWndHeader, i, &Item);
		Width += Item.cxy;
	}

	//
	// N.B. Compare header's column total width with header's rect width
	// this require more evaluation.
	//

	GetClientRect(Object->hWndHeader, &Rect);
	Width = max(Rect.right - Rect.left, Width);
	return Width;
}

LONG
TreeListInsertColumn(
	__in PTREELIST_OBJECT Object,
	__in LONG Column, 
	__in LONG Align,
	__in LONG Width,
	__in PWSTR Title
	)
{
	HDITEM hdi = {0};

	hdi.mask = HDI_WIDTH | HDI_TEXT | HDI_FORMAT | HDI_LPARAM;
	hdi.cxy = Width;
	hdi.fmt = Align;
	hdi.pszText = Title;
	hdi.cchTextMax = MAX_PATH;
	hdi.lParam = (LPARAM)&Object->Column[Column];

	Object->Column[Column].Align = Align;
	Object->Column[Column].Width = Width;
	wcscpy_s(Object->Column[Column].Name, MAX_HEADER_NAME, Title);

	Header_InsertItem(Object->hWndHeader, Column, &hdi);
	Object->ColumnCount += 1;

	TreeListUpdateColumn(Object);
	return Column;
}

LONG
TreeListRoundUpPosition(
	__in LONG Position,
	__in LONG Unit
	)
{
	return (Position / Unit + 1) * Unit;
}

VOID
TreeListResetScrollBar(
	__in PTREELIST_OBJECT Object
	)
{
	RECT Rect;
	LONG ClientWidth;
	LONG HeaderWidth;
	SCROLLINFO si = {0};

	GetClientRect(Object->hWnd, &Rect);
	ClientWidth = Rect.right - Rect.left;
	HeaderWidth = TreeListGetHeaderWidth(Object);

	//
	// if the width of header is beyond the limit of treelist's client area
	// scrollbar is enabled
	//

	if(HeaderWidth > ClientWidth) {
		si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE; 
        si.nMin   = 0; 
        si.nMax   = HeaderWidth; 
        si.nPage  = ClientWidth; 
		SetScrollInfo(Object->hWnd, SB_HORZ, &si, TRUE);
		ShowScrollBar(Object->hWnd, SB_HORZ, TRUE);
		Object->ScrollOffset = 0;
	}
	else {
		si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE; 
        si.nMin   = 0; 
        si.nMax   = 0; 
        si.nPage  = 0; 
		SetScrollInfo(Object->hWnd, SB_HORZ, &si, TRUE);
		ShowScrollBar(Object->hWnd, SB_HORZ, FALSE);
		Object->ScrollOffset = 0;
	}

}

VOID
TreeListOnHScroll(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	RECT Rect;
	RECT TreeRect;
	RECT HeaderRect; 
	LONG Previous;
	PTREELIST_OBJECT Object;
	SCROLLINFO si = {0};

	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);
	if (!Object) {
		
		//
		// Object must be validated
		//

		return;
	}

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;

	GetScrollInfo(hWnd, SB_HORZ, &si);
	Previous = si.nPos;

	switch(LOWORD(wp))
	{
	case SB_LINELEFT:							
			si.nPos -= 1;
			break;
		
	case SB_LINERIGHT:	
			si.nPos += 1;
			break;
		
	case SB_PAGELEFT:
			si.nPos -= si.nPage;
			break;
		
	case SB_PAGERIGHT:
			si.nPos += si.nPage;
			break;

	case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;

	case SB_THUMBPOSITION:
			Object->ScrollOffset = si.nPos;
			break;
	};

	si.fMask = SIF_POS;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	si.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_HORZ, &si);

	if (si.nPos != Previous){
		ScrollWindow(hWnd, Previous - si.nPos, 0, NULL, NULL);
		GetClientRect(hWnd, &Rect);
		InvalidateRect(hWnd, &Rect, TRUE);
	}

	GetClientRect(Object->hWndTree, &TreeRect);
	GetClientRect(Object->hWnd, &Rect);
	GetClientRect(Object->hWndHeader, &HeaderRect);

	if(TreeRect.right - TreeRect.left != 0) {

		LONG TreeWidth;

		TreeWidth = TreeRect.right - TreeRect.left;
		TreeWidth = TreeListRoundUpPosition(Object->HeaderWidth, TreeWidth);

		SetWindowPos(Object->hWndHeader, 
			         HWND_TOP, 
					 //Object->ScrollOffset, 0, 
					 -si.nPos, 0,
					 //max(TreeWidth, si.nPage), 
					 HeaderRect.right - HeaderRect.left + si.nPos,
					 HeaderRect.bottom - HeaderRect.top, 
					 SWP_SHOWWINDOW);
		
		SetWindowPos(Object->hWndTree, 
			         Object->hWndHeader, 
					 //Object->ScrollOffset, 0, 
					 -si.nPos, STD_LIST_HEADER_HEIGHT,
					 HeaderRect.right - HeaderRect.left + si.nPos, 
					 TreeRect.bottom - TreeRect.top, 
					 SWP_SHOWWINDOW);
	}
}

//
// N.B.
// 1, the header must have its max length, hence the resize operation is
//    limited to be within the length the header. otherwise the scrollbar
//    can not be handled correctly.
// 2, 
//
  //// if Horizontal scrolling, we should update the location of the
  //      // left hand edge of the window...
  //      //
  //      if (dx != 0)
  //      {
  //          RECT rcHdr;
  //          GetWindowRect(plv->hwndHdr, &rcHdr);
  //          MapWindowRect(HWND_DESKTOP, plv->ci.hwnd, &rcHdr);
  //          SetWindowPos(plv->hwndHdr, NULL, rcHdr.left - dx, rcHdr.top,
  //                  rcHdr.right - rcHdr.left + dx,
  //                  rcHdr.bottom - rcHdr.top,
  //                  SWP_NOZORDER | SWP_NOACTIVATE);
  //      }
  //
