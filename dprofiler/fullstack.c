//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#include "sdk.h"
#include "cpuprof.h"
#include "apsprofile.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "dialog.h"
#include "profileform.h"
#include "treelist.h"
#include "fullstack.h"
#include "apscpu.h"


typedef enum _FULLSTACK_COLUMN_KEY {
	FullStackTID,
	FullStackState,
	FullStackPercent,
	FullStackMilliseconds,
    FullStackCycles,
	FULLSTACK_COLUMN_COUNT,
} FULLSTACK_COLUMN_KEY;

DIALOG_SCALER_CHILD FullStackChildren[] = {
	{ IDC_TREE_FULLSTACK, AlignRight, AlignBottom },
	{ IDC_BUTTON_EXPAND, AlignNone, AlignBoth },
	{ IDC_BUTTON_COLLAPSE, AlignNone, AlignBoth },
	{ IDC_BUTTON_EXPORT, AlignNone, AlignBoth },
	{ IDOK, AlignBoth, AlignBoth }
};

DIALOG_SCALER FullStackScaler = {
	{0,0}, {0,0}, {0,0}, 5, FullStackChildren
};

HEADER_COLUMN FullStackColumn[] = {
	{ {0}, 360, HDF_LEFT,  FALSE, FALSE, L"TID" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"State" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Time %" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Time (ms)" },
	{ {0}, 80,  HDF_RIGHT, FALSE, FALSE, L"Cycles)" }
};


VOID
FullStackDialog(
	__in HWND hWndParent,
	__in PPF_REPORT_HEAD Report,
    __in PBTR_CPU_RECORD Record 
	)
{
	DIALOG_OBJECT Object = {0};
	FULLSTACK_CONTEXT Context = {0};

	Context.Base.Head = Report;
	Context.Record = Record;
	Object.Context = &Context.Base;

	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_FULLSTACK;
	Object.Procedure = FullStackProcedure;
	
	DialogCreate(&Object);
}

INT_PTR CALLBACK
FullStackProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return FullStackOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			return FullStackOnCommand(hWnd, uMsg, wp, lp);
		
		case WM_CTLCOLORSTATIC:
			return FullStackOnCtlColorStatic(hWnd, uMsg, wp, lp);

        case WM_TREELIST_SORT:
            return FullStackOnTreeListSort(hWnd, uMsg, wp, lp);
	}
	
	return Status;
}

LRESULT
FullStackOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFULLSTACK_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
	ULONG Number;
	RECT Rect;
	HWND hWndCtrl;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FULLSTACK_CONTEXT);
	
	//
	// Create treelist control
	//
	
	hWndCtrl = GetDlgItem(hWnd, IDC_TREE_FULLSTACK);
	TreeList = TreeListCreate(hWnd, TRUE, IDC_TREE_FULLSTACK, 
							  &Rect, hWnd, 
							  FullStackFormatCallback, 
							  FULLSTACK_COLUMN_COUNT);

	//
	// We need explicitly bind the treelist object to its hwnd
	// if it's hosted in dialog as custom control.
	//

	TreeList->hWnd = hWndCtrl;
	SdkSetObject(hWndCtrl, TreeList);
	TreeListCreateControls(TreeList);

	for(Number = 0; Number < FULLSTACK_COLUMN_COUNT; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 FullStackColumn[Number].Align, 
							 FullStackColumn[Number].Width, 
							 FullStackColumn[Number].Name);
	}

	ASSERT(TreeList->hWnd != NULL);
	Context->Base.TreeList = TreeList;

    //
    // Insert thread list
    //

	Object->Scaler = &FullStackScaler;
	DialogRegisterScaler(Object);

    FullStackInsertData(Object, Context, TreeList);
    MoveWindow(hWnd, 0, 0, 680, 600, TRUE);

	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

VOID
FullStackInsertData(
    __in PDIALOG_OBJECT Object,
    __in PFULLSTACK_CONTEXT Context,
    __in PTREELIST_OBJECT TreeList
    )
{ 
    PBTR_CPU_RECORD Record;
    HWND hWndTree;
    PBTR_CPU_SAMPLE Sample;
    TVINSERTSTRUCT tvi = {0};
    ULONG Number;
    HTREEITEM hItem;
    WCHAR Buffer[16];
    
    Record = Context->Record;
    hWndTree = TreeList->hWndTree;

    for(Number = 0; Number < Record->ActiveCount + Record->RetireCount; Number += 1) {

        Sample = &Record->Sample[Number];
        tvi.hParent = TVI_ROOT; 
        tvi.hInsertAfter = TVI_LAST; 
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 

        StringCchPrintf(Buffer, 16, L"%u", Sample->ThreadId);
        tvi.item.pszText = Buffer;
        tvi.item.lParam = (LPARAM)Sample; 

        hItem = TreeView_InsertItem(hWndTree, &tvi);
        FullStackInsertBackTrace(Context, hWndTree, hItem, Sample);
   }
}

HTREEITEM
FullStackInsertBackTrace(
    __in PFULLSTACK_CONTEXT Context,
    __in HWND hWndTree,
    __in HTREEITEM hItemParent,
    __in PBTR_CPU_SAMPLE Sample 
    )
{
    TVINSERTSTRUCT tvi = {0};
    HTREEITEM hItem;
    WCHAR Buffer[MAX_PATH];
    PBTR_STACK_RECORD Record;
    PBTR_TEXT_TABLE Table;
    PPF_REPORT_HEAD Report;
    PBTR_TEXT_ENTRY Text;
    ULONG Length;
    PWCHAR Symbol;
    ULONG i;
   
    Report = Context->Base.Head;
    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Report, STREAM_STACK);
    Record = &Record[Sample->StackId];

    Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;
		
		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID) Table;
	}

	for(i = 0; i < Record->Depth; i++) {

		Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[i]);

		if (Text != NULL) {
			StringCchPrintf(Buffer, MAX_PATH, L"%S", Text->Text);
		} 
		
		else {

#if defined(_M_X64)
			StringCchPrintf(Buffer, MAX_PATH, L"0x%I64x", (ULONG64)Record->Frame[i]);

#elif defined(_M_IX86)
			StringCchPrintf(Buffer, MAX_PATH, L"0x%x", (ULONG)Record->Frame[i]);
#endif
		}
        
        Length = (wcslen(Buffer)  + 1 ) * sizeof(WCHAR);
        Symbol = (PWCHAR)SdkMalloc(Length);

        StringCchCopy(Symbol, Length, Buffer);
        tvi.hParent = hItemParent; 
        tvi.hInsertAfter = TVI_LAST; 
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
        tvi.item.pszText = Symbol;
        tvi.item.lParam = (LPARAM)Symbol; 

        hItem = TreeView_InsertItem(hWndTree, &tvi);
	}

    return hItem;
}

LRESULT
FullStackOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	switch (LOWORD(wp)) {

		case IDOK:
		case IDCANCEL:
			return FullStackOnOk(hWnd, uMsg, wp, lp);

		case IDC_BUTTON_EXPAND:
			return FullStackOnExpand(hWnd, uMsg, wp, lp);

		case IDC_BUTTON_COLLAPSE:
			return FullStackOnCollapse(hWnd, uMsg, wp, lp);
		
		case IDC_BUTTON_EXPORT:
			return FullStackOnExport(hWnd, uMsg, wp, lp);

	}

    return 0;
}

LRESULT
FullStackOnOk(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PDIALOG_OBJECT Object;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    FullStackCleanUp(Object);
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
FullStackOnExpand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
    PFULLSTACK_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
    HTREEITEM hItem;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = SdkGetContext(Object, FULLSTACK_CONTEXT);
    TreeList = Context->Base.TreeList;
    ASSERT(TreeList != NULL);

	hItem = TreeView_GetChild(TreeList->hWndTree, TVI_ROOT);
	while (hItem != NULL) {
        TreeView_Expand(TreeList->hWndTree, hItem, TVE_EXPAND);
		hItem = TreeView_GetNextSibling(TreeList->hWndTree, hItem);
	}

    return 0;
}

LRESULT
FullStackOnCollapse(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
    PFULLSTACK_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
    HTREEITEM hItem;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = SdkGetContext(Object, FULLSTACK_CONTEXT);
    TreeList = Context->Base.TreeList;
    ASSERT(TreeList != NULL);

	hItem = TreeView_GetChild(TreeList->hWndTree, TVI_ROOT);
	while (hItem != NULL) {
        TreeView_Expand(TreeList->hWndTree, hItem, TVE_COLLAPSE);
		hItem = TreeView_GetNextSibling(TreeList->hWndTree, hItem);
	}

    return 0;
}

LRESULT
FullStackOnExport(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FullStackOnCtlColorStatic(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    return 0;
}


VOID CALLBACK
FullStackFormatCallback(
	__in struct _TREELIST_OBJECT *TreeList,
	__in HTREEITEM hTreeItem,
	__in PVOID Value,
	__in ULONG Column,
	__out PWCHAR Buffer,
	__in SIZE_T Length
	)
{
	HWND hWnd;
	PDIALOG_OBJECT Object;
	PFULLSTACK_CONTEXT Context;
	HWND hWndTree;
	HTREEITEM hTreeParentItem;
    PBTR_CPU_SAMPLE Sample;
    PBTR_CPU_RECORD Record;
    double Time;
    double Percent;

	hWnd = GetParent(TreeList->hWnd);
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FULLSTACK_CONTEXT);
    Record = Context->Record;

	hWndTree = TreeList->hWndTree;
    Sample = (PBTR_CPU_SAMPLE)Value;
	hTreeParentItem = TreeView_GetParent(hWndTree, hTreeItem);

	if (hTreeParentItem != NULL) {
        if (Column == 0) {
            StringCchPrintf(Buffer, Length, L"%s", Value);
        } else {
            Buffer[0] = 0;
        }

	} else {
        
        switch (Column) {

        case FullStackTID:
            StringCchPrintf(Buffer, Length, L"%u", Sample->ThreadId);
            break;

        case FullStackMilliseconds:
            Time = ApsNanoUnitToMilliseconds(Sample->KernelTime + Sample->UserTime);
            StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Time);
            break;

        case FullStackCycles:
            StringCchPrintf(Buffer, MAX_PATH, L"%u", Sample->Cycles);
            break;
        
        case FullStackState:
            break;

        case FullStackPercent:

            if (Record->Cycles != 0) {
                Percent = (Sample->Cycles * 100.0f) / Record->Cycles;
            } 

            //
            // If thread cycles is not available, use time instead
            // this also avoid divide zero
            //

            else if (Record->KernelTime + Record->UserTime != 0) {
                Percent = ((Sample->KernelTime + Sample->UserTime) * 100.0f) / 
                    (Record->KernelTime + Record->UserTime);
            }

            else {
                Percent = 0.0;
            }

            StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Percent);
            break;
        }
	}
}

int CALLBACK
FullStackCompareCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
    PCPU_SORT_CONTEXT Csc;
    PBTR_CPU_SAMPLE S1, S2; 
    int Result;
    
    S1 = (PBTR_CPU_SAMPLE)First;
    S2 = (PBTR_CPU_SAMPLE)Second;
    
    Csc = (PCPU_SORT_CONTEXT)Param;

    if (Csc->Column == FullStackTID) {
        Result = (int)(S1->ThreadId - S2->ThreadId); 
    } 
    if (Csc->Column == FullStackState) {
        Result = 0;
    }
    if (Csc->Column == FullStackPercent || Csc->Column == FullStackMilliseconds ||
        Csc->Column == FullStackCycles) {
        Result = (int)(S1->Cycles - S2->Cycles);
    }

    if (Csc->Order == SortOrderDescendent) {
		Result = -Result;
	}

    return Result;
}

LRESULT
FullStackOnTreeListSort(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
    PFULLSTACK_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
	TVSORTCB Tsc = {0};
    CPU_SORT_CONTEXT SortContext;
	TVITEM tvi = {0};

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = SdkGetContext(Object, FULLSTACK_CONTEXT);
    TreeList = Context->Base.TreeList;
    ASSERT(TreeList != NULL);
	
    SortContext.Column = wp;
    SortContext.Context = (LPARAM)NULL;
    SortContext.Order = TreeList->SortOrder;

	Tsc.hParent = TVI_ROOT;
	Tsc.lParam = (LPARAM)&SortContext;
	Tsc.lpfnCompare = FullStackCompareCallback;
	TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

	return 0;
}

VOID
FullStackCleanUp(
    __in PDIALOG_OBJECT Object
    )
{
    PFULLSTACK_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
    HWND hWndTree;
    HTREEITEM hItem, hChildItem;
	TVITEM tvi = {0};

    Context = SdkGetContext(Object, FULLSTACK_CONTEXT);
    TreeList = Context->Base.TreeList;
    ASSERT(TreeList != NULL);

    hWndTree = TreeList->hWndTree;
    hItem = TreeView_GetChild(TreeList->hWndTree, TVI_ROOT);

	while (hItem != NULL) {
        
        hChildItem = TreeView_GetChild(TreeList->hWndTree, hItem);

        while (hChildItem) {
            tvi.mask = TVIF_PARAM;
            tvi.hItem = hChildItem;
            TreeView_GetItem(TreeList->hWndTree, &tvi);
            
            if (tvi.lParam) {
                SdkFree((PVOID)tvi.lParam);
            }

            hChildItem = TreeView_GetNextSibling(TreeList->hWndTree, hChildItem);
        }

		hItem = TreeView_GetNextSibling(TreeList->hWndTree, hItem);
	}
}