#include "listview.h"

PLISTVIEW_EDIT_CONTEXT
ListViewAllocateEditContext(
	IN HWND hWndParent,
	IN HWND hWndListCtrl
	)
{
	PLISTVIEW_EDIT_CONTEXT EditContext = 
		(PLISTVIEW_EDIT_CONTEXT)SdkMalloc(sizeof(LISTVIEW_EDIT_CONTEXT));

	EditContext->hWndParent = hWndParent;
	EditContext->hWndListCtrl = hWndListCtrl;
	EditContext->Item = -1;
	EditContext->SubItem = -1;
	EditContext->Editing = FALSE;

	EditContext->SubItemRect.left = 
	EditContext->SubItemRect.right = 
	EditContext->SubItemRect.top = 
	EditContext->SubItemRect.bottom = 0; 

	return EditContext;
}

LRESULT
ListViewSelectSingle(
	IN HWND hWnd,
	IN LONG Item
	)
{
	LONG Mask; 
	
	Mask = LVIS_FOCUSED | LVIS_SELECTED;
	ListView_SetItemState(hWnd, Item, Mask, Mask);
	return 0;
}

LONG
ListViewGetFirstSelected(
	IN HWND hWnd
	)
{
	LONG Mask;
	LONG Index;

	Mask = LVNI_SELECTED;
	Index = ListView_GetNextItem(hWnd, -1, Mask);

	return Index;
}

LONG
ListViewGetNextSelected(
	IN HWND hWnd,
	IN LONG From
	)
{
	LONG Mask;
	LONG Index;

	Mask = LVNI_SELECTED;
	Index = ListView_GetNextItem(hWnd, From, Mask);

	return Index;
}

BOOLEAN
ListViewIsSelected(
	IN HWND hWnd,
	IN LONG Index
	)
{
	LONG Mask;
	Mask = ListView_GetItemState(hWnd, Index, LVIS_SELECTED);
	return (Mask & LVIS_SELECTED) ? TRUE : FALSE;
}

BOOLEAN
ListViewIsFocused(
	IN HWND hWnd,
	IN LONG Index
	)
{
	LONG Mask;
	Mask = ListView_GetItemState(hWnd, Index, LVIS_FOCUSED);
	return (Mask & LVIS_FOCUSED) ? TRUE : FALSE;
}

BOOLEAN
ListViewGetCheck(
	IN HWND hWnd,
	IN LONG Item
	)
{
    int State;
	
	State = (int)SendMessage(hWnd, 
		                     LVM_GETITEMSTATE, 
							 (WPARAM)Item, 
							 (LPARAM)LVIS_STATEIMAGEMASK);

    // 
	// Return zero if it's not checked, or nonzero otherwise.
	//

    return ((BOOLEAN)(State >> 12) -1);
}

BOOLEAN 
ListViewSetCheck(
	IN HWND hWnd,
	IN LONG Item, 
	IN BOOLEAN Check
	)
{
    LVITEM lvi;
    lvi.stateMask = LVIS_STATEIMAGEMASK;

	// 
    // Since state images are one-based, 1 in this macro turns the check off, and
    // 2 turns it on.
    //

    lvi.state = INDEXTOSTATEIMAGEMASK((Check ? 2 : 1));
    return (BOOLEAN)SendMessage(hWnd, LVM_SETITEMSTATE, Item, (LPARAM)&lvi);
}

BOOLEAN
ListViewGetParam(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LPARAM *Param 
	)
{
	LVITEM lvi = {0};
  
	lvi.mask = LVIF_PARAM;
	lvi.iItem = Item;
  
	if (ListView_GetItem(hWnd, &lvi)) {
		*Param = lvi.lParam;
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
ListViewSetParam(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LPARAM Param 
	)
{
	LVITEM lvi = {0};
  
	lvi.mask = LVIF_PARAM;
	lvi.iItem = Item;
	lvi.lParam = Param;

	return (BOOLEAN)ListView_SetItem(hWnd, &lvi);
}

LONG
ListViewInsertItem(
	IN HWND hWnd, 
	IN LVITEM *Item
	)
{
	LONG Status;

	Status = ListView_InsertItem(hWnd, Item);
	return Status;
}

LRESULT 
ListViewOnCustomDraw(
	IN HWND hWnd,
	IN NMHDR *pNmhdr
	)
{
    LRESULT Status = 0L;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;
	PLISTVIEW_OBJECT Object;
	PLISTVIEW_COLUMN Column;

	Object = (PLISTVIEW_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Column = Object->Column;

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW;
            break;
        
        case CDDS_SUBITEM | CDDS_ITEM | CDDS_PREPAINT: 
          
            lvcd->clrText = Column[lvcd->iSubItem].ForeColor;
            lvcd->clrTextBk = Column[lvcd->iSubItem].BackColor;
            Status = CDRF_NEWFONT;
	      
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}