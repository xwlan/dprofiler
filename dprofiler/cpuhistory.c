//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "sdk.h"
#include "aps.h"
#include "apsdefs.h"
#include "apsprofile.h"
#include "apscpu.h"
#include "apspdb.h"
#include "profileform.h"
#include "split.h"
#include "frame.h"
#include "cpuhistory.h"
#include "graphctrl.h"
#include "cputhread.h"
#include "apsrpt.h"
#include "trackbar.h"
#include "statebar.h"
#include "main.h"
#include "fullstack.h"
#include "flamereport.h"

//
// CPU Sample State Color 
//

COLORREF crRun = RGB(0x9E, 0xCA, 0x9E);
COLORREF crWait = RGB(0xFF, 0x79, 0x71);
COLORREF crIo = RGB(0xCB, 0x98, 0xB6);
COLORREF crBorder = RGB(0x2C, 0x84, 0x04);
COLORREF crSleep = RGB(0x89, 0xAB, 0xBD);

//
// Dialog scaler children
//

static
DIALOG_SCALER_CHILD CpuHistoryChildren[] = {
    { IDC_CPU_HISTORY_GRAPH, AlignRight, AlignNone },
    { IDC_CPU_HISTORY_REBAR, AlignRight, AlignNone },
//    { IDC_ZOOMOUT, AlignBoth, AlignNone },
//    { IDC_SLIDER_ZOOM, AlignBoth, AlignNone },
//    { IDC_ZOOMIN, AlignBoth, AlignNone },
    { IDC_LIST_CPU_HISTORY_THREAD, AlignRight, AlignNone },
    { IDC_LIST_CPU_HISTORY_STACK, AlignRight, AlignBottom }
};

#define CPU_HISTORY_CHILDREN_NUM  (sizeof(CpuHistoryChildren)/sizeof(DIALOG_SCALER_CHILD))

static 
DIALOG_SCALER CpuHistoryScaler = {
    {0,0}, {0,0}, {0,0}, CPU_HISTORY_CHILDREN_NUM, CpuHistoryChildren
};

//
// Thread list pane
//

typedef enum _CpuHistoryThreadColumn {
    CPU_HISTORY_TID,
    CPU_HISTORY_STATE,
    CPU_HISTORY_TIME,
    CPU_HISTORY_CYCLES,
    CPU_HISTORY_TIME_PERCENT,
} CpuHistoryThreadColumn;


static
LISTVIEW_COLUMN CpuHistoryColumn[] = {
    { 40,  L"TID",    LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
    { 200, L"History",LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	/*
    { 80,  L"Time (ms)",LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
    { 80,  L"Cycles",   LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
    { 80,  L"Time %",   LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
    { 80,  L"State",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }, 
	*/
};

#define CPU_HISTORY_COLUMN_NUM  (sizeof(CpuHistoryColumn)/sizeof(LISTVIEW_COLUMN))

//
// Stack trace list pane
//

static 
LISTVIEW_COLUMN CpuStackColumn[] = {
    { 20,  L"#",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
    { 200, L"Call Site", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
    { 200, L"Line", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define CPU_STACK_COLUMN_NUM  (sizeof(CpuStackColumn)/sizeof(LISTVIEW_COLUMN))

//
// Horizonal splitter 
//

static
SPLIT_INFO CpuHistoryHoriSplitInfo[] = {
    { IDC_LIST_CPU_HISTORY_THREAD, SPLIT_BOTTOM },
    { IDC_LIST_CPU_HISTORY_STACK,  SPLIT_TOP }
};

static
SPLIT_OBJECT CpuHistoryVertSplitObject = {
    FALSE, CpuHistoryHoriSplitInfo, 2, 20 
};

HWND
CpuHistoryCreate(
    __in HWND hWndParent,
    __in ULONG CtrlId 
    )
{
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;
    HWND hWnd;

    Context = (PCPU_FORM_CONTEXT)SdkMalloc(sizeof(CPU_HISTORY_FORM_CONTEXT));
    Context->CtrlId = CtrlId;
    Context->Head = NULL;
    Context->Path[0] = 0;
    Context->TreeList = NULL;

    Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
    Object->Context = Context;
    Object->hWndParent = hWndParent;
    Object->ResourceId = IDD_FORMVIEW_CPU_HISTORY;
    Object->Procedure = CpuHistoryProcedure;

    hWnd = DialogCreateModeless(Object);
    ShowWindow(hWnd, SW_SHOW);
    return hWnd;
}

LRESULT
CpuHistoryOnInitDialog(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    PDIALOG_OBJECT Object;
    PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    LVCOLUMN lvc = {0};
    LVITEM lvi = {0};
    ULONG i;
    PLISTVIEW_OBJECT ListView;
    RECT Rect;
    RECT CtrlRect;

    PTRACK_CONTROL Track;
    int Height;
	int cxScreen;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = SdkGetContext(Object, CPU_HISTORY_FORM_CONTEXT);

    Context->Base.hBrushBack = CreateSolidBrush(RGB(255, 255, 255));
	Context->hbrRun = CreateSolidBrush(crRun);
	Context->hbrWait = CreateSolidBrush(crWait);
	Context->hbrIo = CreateSolidBrush(crIo);
	Context->hbrSleep = CreateSolidBrush(crSleep);
	Context->hBorderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	Context->hHoverPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

	//
	// Initialize statebar metrics, test purpose ONLY
	//

	Context->CellBorder = 0;
	Context->Scale = 1;
	Context->MinCellWidth = 1;
	Context->MaxCellWidth = 6;
	Context->VertEdge = 1;
	Context->HoriEdge = 0;
    Context->OldItem = INVALID_VALUE;
    Context->OldSample = INVALID_VALUE;
    Context->CurrentItem = INVALID_VALUE;
    Context->CurrentSample = INVALID_VALUE;

    //
    // Initialize the graph control
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_CPU_HISTORY_GRAPH);
    ASSERT(hWndCtrl != 0);

    TrackInitializeControl(hWndCtrl, IDC_CPU_HISTORY_GRAPH, 
                           RGB(238, 238, 199), RGB(0x82, 0x82, 0x82), 
                           RGB(0x9e, 0xca, 0x9e), RGB(0x3c, 0x94, 0x3c),
						   RGB(255, 0, 0));


    SdkModifyStyle(hWndCtrl, 0, WS_EX_CLIENTEDGE, TRUE);
    Track = (PTRACK_CONTROL)SdkGetObject(hWndCtrl);

    //
    // Generate random test data
    //

	cxScreen = GetSystemMetrics(SM_CXSCREEN);

    //
    // Create thread listview object wraps list control
    //

    ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
    ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

    ListView->Column = CpuHistoryColumn;
    ListView->Count = CPU_HISTORY_COLUMN_NUM;
    ListView->NotifyCallback = CpuHistoryOnNotify;

    Context->Base.ListView = ListView;

    //
    // Initialize thread pane
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_THREAD);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_TRACKSELECT, 
                                        LVS_EX_TRACKSELECT);

    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_DOUBLEBUFFER , 
                                        LVS_EX_DOUBLEBUFFER );

    for (i = 0; i < CPU_HISTORY_COLUMN_NUM; i++) { 
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
        lvc.pszText = CpuHistoryColumn[i].Title;	
        lvc.cx = CpuHistoryColumn[i].Width;     
        lvc.fmt = CpuHistoryColumn[i].Align;
        ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

    //
    // Initialize stack trace pane 
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_STACK);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

    for (i = 0; i < CPU_STACK_COLUMN_NUM; i++) { 
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
        lvc.pszText = CpuStackColumn[i].Title;	
        lvc.cx = CpuStackColumn[i].Width;     
        lvc.fmt = CpuStackColumn[i].Align;
        ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

    //
    // Initialize horizonal splitbar
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_CPU_HISTORY_HORIZON);
    SplitSetObject(hWndCtrl, &CpuHistoryVertSplitObject);

    //
    // Position controls 
    //

    GetClientRect(hWnd, &Rect);

    //
    // Position CPU graph control
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_CPU_HISTORY_GRAPH);
    CtrlRect.top = 0;
    CtrlRect.left = 0;
    CtrlRect.right = Rect.right;
    CtrlRect.bottom = 100;
    MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
               CtrlRect.right - CtrlRect.left, 
               CtrlRect.bottom - CtrlRect.top, TRUE);

    //
    // Register dialog scaler
    //

    Object->Scaler = &CpuHistoryScaler;
    DialogRegisterScaler(Object);

	//
	// Position trackbar to initial position
	//

    Context->StateBar = StateBarCreateObject(hWnd, IDC_CPU_HISTORY_REBAR, FALSE);
    ASSERT(Context->StateBar != NULL);

    Height = CpuHistoryPositionStateBar2(Object, Context->StateBar, &CtrlRect); 

	//hWndCtrl = GetDlgItem(hWnd, IDC_SLIDER_ZOOM);
	//SendMessage(hWndCtrl, TBM_SETBUDDY, TRUE, (LPARAM)GetDlgItem(hWnd, IDC_ZOOMOUT));
	//SendMessage(hWndCtrl, TBM_SETBUDDY, FALSE, (LPARAM)GetDlgItem(hWnd, IDC_ZOOMIN));
	//SendMessage(hWndCtrl, TBM_SETRANGE, FALSE, MAKELONG(1, 6));
	//SendMessage(hWndCtrl, TBM_SETPOS, FALSE, 1);
    
    //
    // Position thread listview
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_THREAD);
    CtrlRect.top = Height;
    CtrlRect.left = 0;
    CtrlRect.right = Rect.right;
    CtrlRect.bottom = CtrlRect.top + 150;
    MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
               CtrlRect.right - CtrlRect.left, 
               CtrlRect.bottom - CtrlRect.top, TRUE);

    SetWindowSubclass(hWndCtrl, CpuHistoryListProcedure, 0, (DWORD_PTR)Object);

    //
    // Position horizonal splitbar
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_CPU_HISTORY_HORIZON);
    CtrlRect.top = CtrlRect.bottom;
    CtrlRect.left = 0;
    CtrlRect.right = Rect.right;
    CtrlRect.bottom = CtrlRect.top + 2;
    MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
               CtrlRect.right - CtrlRect.left, 
               CtrlRect.bottom - CtrlRect.top, TRUE);

    //
    // Position stack listview
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_STACK);
    CtrlRect.top = CtrlRect.bottom;
    CtrlRect.left = 0;
    CtrlRect.right = Rect.right;
    CtrlRect.bottom = Rect.bottom;
    MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
               CtrlRect.right - CtrlRect.left, 
               CtrlRect.bottom - CtrlRect.top, TRUE);

  
    return TRUE;
}

VOID
CpuHistoryBuildStateBar(
	__in PDIALOG_OBJECT Object
	)
{
}

LRESULT CALLBACK 
CpuHistoryHeaderProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp,
    __in UINT_PTR uIdSubclass, 
    __in DWORD_PTR dwData
    )
{
    return 0;
}

LRESULT
CpuHistoryOnSize(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    return 0;
}

LRESULT
CpuHistoryOnClose(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    return 0;
}

INT_PTR CALLBACK
CpuHistoryProcedure(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    INT_PTR Status = FALSE;

    switch (uMsg) {

    case WM_INITDIALOG:
        return CpuHistoryOnInitDialog(hWnd, uMsg, wp, lp);

    case WM_CLOSE:
        return CpuHistoryOnClose(hWnd, uMsg, wp, lp);

    case WM_DRAWITEM:
        return CpuHistoryOnDrawItem(hWnd, uMsg, wp, lp);

    case WM_NOTIFY:
        return CpuHistoryOnNotify(hWnd, uMsg, wp, lp);

	case WM_LBUTTONDOWN:
		return CpuHistoryOnLButtonDown(hWnd, uMsg, wp, lp);

	case WM_PARENTNOTIFY:
		return CpuHistoryOnParentNotify(hWnd, uMsg, wp, lp);

	case WM_CTLCOLORSTATIC:
		return CpuHistoryOnCtlColorStatic(hWnd, uMsg, wp, lp);

    case WM_GRAPH_SET_RECORD:
        return CpuHistoryOnGraphSetRecord(hWnd, uMsg, wp, lp);

    case WM_TRACK_SET_RANGE:
        return CpuHistoryOnTrackSetRange(hWnd, uMsg, wp, lp);

    case WM_HSCROLL:
        return CpuHistoryOnScroll(hWnd, uMsg, wp, lp);

    case WM_COMMAND:
        return CpuHistoryOnCommand(hWnd, uMsg, wp, lp);
    }

    return Status;
}

LRESULT
CpuHistoryOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    PDIALOG_OBJECT Object;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    if (!Object)  {
        return 0;
    }

	switch(LOWORD(wp)) {
		case IDM_SAMPLE_FORWARD:
            CpuHistoryOnSampleForward(Object);
			break;
		case IDM_SAMPLE_BACKWARD:
            CpuHistoryOnSampleBackward(Object);
			break;
		case IDM_SAMPLE_UPWARD:
            CpuHistoryOnSampleUpward(Object);
			break;
		case IDM_SAMPLE_DOWNWARD:
            CpuHistoryOnSampleDownward(Object);
			break;
		case IDM_SAMPLE_FULLSTACK:
            CpuHistoryOnSampleFullStack(Object);
			break;
		case IDM_SAMPLE_REPORT:
            CpuHistoryOnSampleReport(Object);
			break;
	}

    return 0;
}

BOOLEAN
CpuHistoryIsThreadClipped(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in PCPU_THREAD Thread
    )
{
    PCPU_THREAD_STATE State;

    State = Thread->SampleState;
    if (State->First > Context->LastSample || (State->First + State->Count - 1) < Context->FirstSample) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
CpuHistoryIsSampleClipped(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in PCPU_THREAD Thread,
    __in ULONG Sample
    )
{
    PCPU_THREAD_STATE State;
    ULONG First, Last;

    State = Thread->SampleState;

    //
    // Compute the intersection of context sample range and thread sample range
    //

    First = max(State->First, Context->FirstSample);
    Last = min(State->First + State->Count - 1, Context->LastSample);

    if (First <= Sample && Last >= Sample) {
        return TRUE;
    }

    return FALSE;
}

ULONG
CpuHistoryFirstClippedSample(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in PCPU_THREAD Thread
    )
{
    ULONG First;
    PCPU_THREAD_STATE State;

    State = Thread->SampleState;
    ASSERT(State != NULL);

    First = max(State->First, Context->FirstSample);
    return First;
}

ULONG
CpuHistoryLastClippedSample(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in PCPU_THREAD Thread
    )
{
    ULONG Last;
    PCPU_THREAD_STATE State;

    State = Thread->SampleState;
    ASSERT(State != NULL);

	Last = min(State->First + State->Count - 1, Context->LastSample);
    return Last;
}

LRESULT
CpuHistoryOnSampleForward(
	__in PDIALOG_OBJECT Object
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    PCPU_THREAD Thread;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_HISTORY_THREAD);

    //
    // If not set current item, set to thread 0's first sample 
    //

    if (Context->CurrentItem == INVALID_VALUE || Context->CurrentSample == INVALID_VALUE) {
        ListViewSelectSingle(hWndCtrl, 0);
        Context->CurrentItem = 0;
        ListViewGetParam(hWndCtrl, 0, (LPARAM *)&Thread);
        ASSERT(Thread != NULL);
        Context->CurrentSample = CpuHistoryFirstClippedSample(Context, Thread);
    }
    else {

        ULONG Last;

        Context->OldItem = Context->CurrentItem;
        Context->OldSample = Context->CurrentSample;
        Context->CurrentItem = Context->CurrentItem;
        Context->CurrentSample = Context->CurrentSample + 1;
        
        //
        // Bound the current sample into legal range
        //

        ListViewGetParam(hWndCtrl, Context->CurrentItem, (LPARAM *)&Thread);
        Last = CpuHistoryLastClippedSample(Context, Thread);
        Context->CurrentSample = min(Context->CurrentSample, Last);
    }

    ListView_RedrawItems(hWndCtrl, Context->CurrentItem, Context->CurrentItem);
    CpuHistorySetCurrentSample(Object, Context->CurrentItem, Context->CurrentSample);
    return 0;
}

LRESULT
CpuHistoryOnSampleBackward(
	__in PDIALOG_OBJECT Object
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    PCPU_THREAD Thread;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_HISTORY_THREAD);

    //
    // If not set current item, set to thread 0's first sample 
    //

    if (Context->CurrentItem == INVALID_VALUE || Context->CurrentSample == INVALID_VALUE) {
        ListViewSelectSingle(hWndCtrl, 0);
        Context->CurrentItem = 0;
        ListViewGetParam(hWndCtrl, 0, (LPARAM *)&Thread);
        ASSERT(Thread != NULL);
        Context->CurrentSample = CpuHistoryFirstClippedSample(Context, Thread);
    }
    else {

        ULONG First;

        Context->OldItem = Context->CurrentItem;
        Context->OldSample = Context->CurrentSample;
        Context->CurrentItem = Context->CurrentItem;
        Context->CurrentSample = Context->CurrentSample - 1;
        
        //
        // Bound the current sample into legal range
        //

        ListViewGetParam(hWndCtrl, Context->CurrentItem, (LPARAM *)&Thread);
        First = CpuHistoryFirstClippedSample(Context, Thread);
        Context->CurrentSample = max((int)Context->CurrentSample, (int)First);
    }

    ListView_RedrawItems(hWndCtrl, Context->CurrentItem, Context->CurrentItem);
    CpuHistorySetCurrentSample(Object, Context->CurrentItem, Context->CurrentSample);
    return 0;
}

LRESULT
CpuHistoryOnSampleUpward(
	__in PDIALOG_OBJECT Object
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    PCPU_THREAD Thread;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_HISTORY_THREAD);

    if (Context->CurrentItem == 0) {
        return 0;
    }

    //
    // If not set current item, set to thread 0's first sample 
    //

    if (Context->CurrentItem == INVALID_VALUE || Context->CurrentSample == INVALID_VALUE) {
        ListViewSelectSingle(hWndCtrl, 0);
        Context->CurrentItem = 0;
        ListViewGetParam(hWndCtrl, 0, (LPARAM *)&Thread);
        ASSERT(Thread != NULL);
        Context->CurrentSample = CpuHistoryFirstClippedSample(Context, Thread);
    }
    else {
        Context->OldItem = Context->CurrentItem;
        Context->OldSample = Context->CurrentSample;
        Context->CurrentItem = Context->CurrentItem - 1;
        Context->CurrentSample = Context->CurrentSample;
        ListViewSelectSingle(hWndCtrl, Context->CurrentItem);
    }

    ListView_RedrawItems(hWndCtrl, Context->CurrentItem, Context->CurrentItem + 1);
    ListView_EnsureVisible(hWndCtrl, Context->CurrentItem, FALSE);
    CpuHistorySetCurrentSample(Object, Context->CurrentItem, Context->CurrentSample);
    return 0;
}

LRESULT
CpuHistoryOnSampleDownward(
	__in PDIALOG_OBJECT Object
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    PCPU_THREAD Thread;
    int LastItem;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_HISTORY_THREAD);
    
    LastItem = ListView_GetItemCount(hWndCtrl) - 1;
    if (Context->CurrentItem == LastItem) {
        return 0;
    }

    //
    // If not set current item, set to thread 0's first sample 
    //

    if (Context->CurrentItem == INVALID_VALUE || Context->CurrentSample == INVALID_VALUE) {
        ListViewSelectSingle(hWndCtrl, 0);
        Context->CurrentItem = 0;
        ListViewGetParam(hWndCtrl, 0, (LPARAM *)&Thread);
        ASSERT(Thread != NULL);
        Context->CurrentSample = CpuHistoryFirstClippedSample(Context, Thread);
    }
    else {
        Context->OldItem = Context->CurrentItem;
        Context->OldSample = Context->CurrentSample;
        Context->CurrentItem = Context->CurrentItem + 1;
        Context->CurrentSample = Context->CurrentSample;
        ListViewSelectSingle(hWndCtrl, Context->CurrentItem);
    }

    ListView_RedrawItems(hWndCtrl, Context->CurrentItem - 1, Context->CurrentItem);
    ListView_EnsureVisible(hWndCtrl, Context->CurrentItem, FALSE);
    CpuHistorySetCurrentSample(Object, Context->CurrentItem, Context->CurrentSample);
    return 0;
}

LRESULT
CpuHistoryOnSampleFullStack(
	__in PDIALOG_OBJECT Object
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	if (!Context) {
		return 0;
	}

	if (Context->CurrentItem == -1 || Context->CurrentSample == INVALID_VALUE) {
		return 0;
	}

    ASSERT(Context->Record != NULL);
    FullStackDialog(Object->hWnd, Context->Base.Head, Context->Record);
	return 0;
}

LRESULT
CpuHistoryOnSampleReport(
	__in PDIALOG_OBJECT Object
	)
{
    PCPU_HISTORY_FORM_CONTEXT Context;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	if (!Context) {
		return 0;
	}

	if (Context->CurrentItem == -1 || Context->CurrentSample == INVALID_VALUE) {
		return 0;
	}

    FlameReportCreate(Object->hWnd, Context->Base.Head, NULL, 0, 0);
	return 0;
}


LRESULT
CpuHistoryOnScroll(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    HWND hWndCtrl;
    int CtrlId;
    int Code;
    int Position;
    PDIALOG_OBJECT Object;
    PCPU_HISTORY_FORM_CONTEXT Context;

    hWndCtrl = (HWND)lp;
    CtrlId = GetDlgCtrlID(hWndCtrl);
    if (CtrlId != STATEBAR_BAND_ZOOM) {
        return 0;
    }

    Code = LOWORD(wp);
    if (Code == TB_THUMBPOSITION || Code == TB_THUMBTRACK) {
        Position = HIWORD(wp);
    }
    else {
        Position = (int)SendMessage(hWndCtrl, TBM_GETPOS, 0, 0);
    }

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;

    //
    // Update current scale and redraw thread state if it's changed
    //

    if (Context->Scale != Position) {

        Context->Scale = Position;
        hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_THREAD);

        CpuHistorySetStateColumnWidth(Object, hWndCtrl, Context->FirstSample, Context->LastSample);
        ListView_RedrawItems(hWndCtrl, 0, ListView_GetItemCount(hWndCtrl) - 1);
    }

    return 0;
}

LRESULT
CpuHistoryOnDrawItem(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    return 0;
}

LRESULT
CpuHistoryListOnHdnItemChanging(
    __in PDIALOG_OBJECT Object,
	__in LPNMHEADER lpnmhdr 
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
    Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;

	if (lpnmhdr->iItem == CPU_HISTORY_STATE) {
        if (lpnmhdr->pitem->mask & HDI_WIDTH && !Context->AllowHeaderChange) {
			return TRUE;
		}
	}

	return FALSE;
}

LRESULT CALLBACK 
CpuHistoryListProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	LRESULT Status;

	if (uMsg == WM_NOTIFY) { 
		if (pNmhdr->code == HDN_ITEMCHANGING) {
			Status = CpuHistoryListOnHdnItemChanging((PDIALOG_OBJECT)dwData, 
											    (LPNMHEADER)pNmhdr);
			if (Status) {
				return Status;
			}
		}
	}
	
    return DefSubclassProc(hWnd, uMsg, wp, lp);
}

VOID
CpuHistoryComputeStateRect(
	__in PCPU_HISTORY_FORM_CONTEXT Context,
	__in PCPU_THREAD Thread,
	__in LPRECT lprcBound,
	__out LPRECT lprcDraw,
	__out PULONG From,
	__out PULONG To
	)
{
	PCPU_THREAD_STATE State;
	int CellWidth;
	int Offset;
	ULONG First;
	ULONG Last;
	
	State = Thread->SampleState;
	CellWidth = Context->Scale;

	//
	// Compute left offset of the bounding rect 
	//

	First = max(State->First, Context->FirstSample);
	Last = min(State->First + State->Count - 1, Context->LastSample);

	Offset = (First - Context->FirstSample) * (CellWidth + Context->CellBorder);
	lprcDraw->left = lprcBound->left + Context->HoriEdge + Offset;
	lprcDraw->top = lprcBound->top + Context->VertEdge;

	//
	// Compute the right offset of the bounding rect
	//

	Offset = (Last - First + 1) * (CellWidth + Context->CellBorder);
	lprcDraw->right = lprcDraw->left + Offset;
	lprcDraw->bottom = lprcBound->bottom - Context->VertEdge;

	*From = First;
	*To = Last;
}

BOOLEAN
CpuHistoryDrawThreadStateInMemory(
	__in PCPU_HISTORY_FORM_CONTEXT Context,
	__in HWND hWndCtrl,
	__in PCPU_THREAD Thread
	)
{
	PCPU_STATE_UI_CONTEXT UiContext;
	HDC hdc, hdcCtrl;
	HBITMAP hbmp, hbmpOld;
	PCPU_THREAD_STATE State;
	ULONG Width, Height;
	ULONG First, Last;
	ULONG Number;
	HBRUSH hbrCell;
	RECT rc;

    if (!Context->Thread || !Context->ThreadIndex || !Context->NumberOfIndexes) {
        return FALSE;
    }

	UiContext = (PCPU_STATE_UI_CONTEXT)Thread->Context;
	if (!UiContext) {
		UiContext = (PCPU_STATE_UI_CONTEXT)SdkMalloc(sizeof(CPU_STATE_UI_CONTEXT));
		memset(UiContext, 0, sizeof(*UiContext));
        Thread->Context = UiContext;
	}

	UiContext = (PCPU_STATE_UI_CONTEXT)Thread->Context;

	//
	// Clip the sample range to draw
	//
	
    State = Thread->SampleState;
    First = max(State->First, Context->FirstSample);
	Last = min(State->First + State->Count - 1, Context->LastSample);

	//
	// Compute the bitmap dimension, 1 pixel + border (0, currently)
	//
    
	Width = (1 + Context->CellBorder) * (Last - First + 1);
	ListView_GetItemRect(hWndCtrl, 0, &rc, LVIR_BOUNDS);
	Height = rc.bottom - rc.top - Context->VertEdge * 2;

	if (!UiContext->hdc) {

		hdcCtrl = GetDC(hWndCtrl);
		hdc = CreateCompatibleDC(hdcCtrl); 
		hbmp = CreateCompatibleBitmap(hdcCtrl, Width, Height);
		hbmpOld = (HBITMAP)SelectObject(hdc, hbmp);
		ReleaseDC(hWndCtrl, hdcCtrl);

		UiContext->hdc = hdc;
		UiContext->hbmp = hbmp;
		UiContext->hbmpOld = hbmpOld;
		UiContext->bmpRect.left = 0;
		UiContext->bmpRect.right = Width;
		UiContext->bmpRect.top = 0;
		UiContext->bmpRect.bottom = Height;

    } else {
    
        //
        // This is a draw for new range, we need clear up earier GDI resource
        //

        DeleteObject(UiContext->hbmp);
        SelectObject(UiContext->hdc, UiContext->hbmpOld);
        DeleteDC(UiContext->hdc);

		hdcCtrl = GetDC(hWndCtrl);
		hdc = CreateCompatibleDC(hdcCtrl); 
		hbmp = CreateCompatibleBitmap(hdcCtrl, Width, Height);
		hbmpOld = (HBITMAP)SelectObject(hdc, hbmp);
		ReleaseDC(hWndCtrl, hdcCtrl);

		UiContext->hdc = hdc;
		UiContext->hbmp = hbmp;
		UiContext->hbmpOld = hbmpOld;
		UiContext->bmpRect.left = 0;
		UiContext->bmpRect.right = Width;
		UiContext->bmpRect.top = 0;
		UiContext->bmpRect.bottom = Height;
    
    }

	//
	// Draw each cell and its border
	//

    rc.left = 0;
	rc.top = 0;
	rc.right = 1;
	rc.bottom = UiContext->bmpRect.bottom;
	
	for(Number = First; Number <= Last; Number += 1) {

		switch ((UINT)(State->State[Number - State->First].Type)) {
			case CPU_STATE_RUN:
				hbrCell = Context->hbrRun;
				break;
			case CPU_STATE_WAIT:
				hbrCell = Context->hbrWait;
				break;
			case CPU_STATE_IO:
				hbrCell = Context->hbrIo;
				break;
			case CPU_STATE_SLEEP:
				hbrCell = Context->hbrSleep;
				break;

            default:
                
                //
                // Default as run
                //
                ASSERT(0);
                hbrCell = Context->hbrRun;
		}

		FillRect(UiContext->hdc, &rc, hbrCell);
		rc.left = rc.right + Context->CellBorder;
		rc.right = rc.left + 1;
	}

	return TRUE;
}

BOOLEAN
CpuHistoryRedrawSampleRect(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in PCPU_THREAD Thread,
    __in HWND hWndList,
    __in HDC hdc,
    __in int Item,
    __in ULONG Sample
    )
{
    RECT rcBound;
    RECT rcSample;
    int Width;
    PCPU_THREAD_STATE State;
    HBRUSH hbrCell;

    //
    // Check whether the sample index is bounded
    //

    if (Sample < Context->FirstSample || Sample > Context->LastSample) {
        return FALSE;
    }

    //
    // Compute the sample's bounding rect
    //

    Width = Context->Scale;
	ListView_GetSubItemRect(hWndList, Item, CPU_HISTORY_STATE, LVIR_BOUNDS, &rcBound);

    rcSample.left = rcBound.left + Context->HoriEdge + (Sample - Context->FirstSample) * (Width + Context->CellBorder);
    rcSample.right = rcSample.left + Width;
    rcSample.top = rcBound.top + Context->VertEdge;
    rcSample.bottom = rcBound.bottom - Context->VertEdge;

    //
    // Fill the sample rect with state color
    //

    State = Thread->SampleState;
    ASSERT(State != NULL);

    switch ((UINT)(State->State[Sample - State->First].Type)) {
			case CPU_STATE_RUN:
				hbrCell = Context->hbrRun;
				break;
			case CPU_STATE_WAIT:
				hbrCell = Context->hbrWait;
				break;
			case CPU_STATE_IO:
				hbrCell = Context->hbrIo;
				break;
			case CPU_STATE_SLEEP:
				hbrCell = Context->hbrSleep;
				break;
	}

	FillRect(hdc, &rcSample, hbrCell);
    return TRUE;
}

LRESULT
CpuHistoryDrawThreadStateLite(
	__in PDIALOG_OBJECT Object,
	__in HWND hWndList,
	__in LPNMLVCUSTOMDRAW lvcd
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
	PCPU_STATE_UI_CONTEXT UiContext;
	PCPU_THREAD Thread;
	RECT rcBound;
	RECT rcDraw;
    RECT rcHover;
	int index;
	PCPU_THREAD_STATE State;
	ULONG First, Last;
    HBRUSH hbrBorder;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    if (!Context->Thread || !Context->ThreadIndex || !Context->NumberOfIndexes) {
        return CDRF_DODEFAULT;
    }

	//
	// Get attached thread object to listview item
	//
	
	index = (int)lvcd->nmcd.dwItemSpec;
	ListViewGetParam(hWndList, index, (LPARAM *)&Thread);
	ASSERT(Thread != NULL);

	State = Thread->SampleState;
	ASSERT(State != NULL);

	//
	// Compute the bounding rect to draw state
	//

	ListView_GetSubItemRect(hWndList, index, CPU_HISTORY_STATE, LVIR_BOUNDS, &rcBound);
	CpuHistoryComputeStateRect(Context, Thread, &rcBound, &rcDraw, &First, &Last);

	//
	// Stretch the in memory bitmap to destine DC
	//

	UiContext = (PCPU_STATE_UI_CONTEXT)Thread->Context;
	if (!UiContext) {
		CpuHistoryDrawThreadStateInMemory(Context, hWndList, Thread);
		UiContext = (PCPU_STATE_UI_CONTEXT)Thread->Context;
	}

	StretchBlt(lvcd->nmcd.hdc, rcDraw.left, rcDraw.top, rcDraw.right - rcDraw.left,
		rcDraw.bottom - rcDraw.top, UiContext->hdc, 0, 0, 
		UiContext->bmpRect.right - UiContext->bmpRect.left,
		UiContext->bmpRect.bottom - UiContext->bmpRect.top, SRCCOPY);

    //
    // Draw the hovered sample rect
    //

    if (Context->CurrentItem == index) {
        if (Context->IsSampleHover) {

            rcHover.left = rcDraw.left + (Context->Scale + Context->CellBorder) * (Context->CurrentSample - First);
            rcHover.right = rcHover.left + Context->Scale;
            rcHover.top = rcDraw.top;
            rcHover.bottom = rcDraw.bottom;

            hbrBorder = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FrameRect(lvcd->nmcd.hdc, &rcHover, hbrBorder);
        }
    }
    return CDRF_SKIPDEFAULT;
}

LRESULT
CpuHistoryDrawThreadState(
	__in PDIALOG_OBJECT Object,
	__in HWND hWndList,
	__in LPNMLVCUSTOMDRAW lvcd
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
	PCPU_THREAD Thread;
	RECT rcBound;
	RECT rcDraw;
	RECT rcCell;
	int index;
	PCPU_THREAD_STATE State;
	int CellWidth;
	HDC hdc;
	ULONG Number;
	HBRUSH hbrBorder;
	ULONG First, Last;
	HBRUSH hbrCell;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    if (!Context->Thread || !Context->ThreadIndex || !Context->NumberOfIndexes) {
        return CDRF_DODEFAULT;
    }

	//
	// Get attached thread object to listview item
	//
	
	index = (int)lvcd->nmcd.dwItemSpec;
	ListViewGetParam(hWndList, index, (LPARAM *)&Thread);
	ASSERT(Thread != NULL);

	State = Thread->SampleState;
	ASSERT(State != NULL);

	ListView_GetSubItemRect(hWndList, index, CPU_HISTORY_STATE, LVIR_BOUNDS, &rcBound);

	CellWidth = Context->Scale;

	//
	// Compute the bounding rect to draw state
	//

	CpuHistoryComputeStateRect(Context, Thread, &rcBound, &rcDraw, &First, &Last);

	//
	// Draw the frame border
	//

	hdc = lvcd->nmcd.hdc;
	
	hbrBorder = (HBRUSH)GetStockObject(BLACK_BRUSH);
	FrameRect(hdc, &rcDraw, hbrBorder);

	//
	// Draw each cell and its border
	//

    rcCell.left = rcDraw.left;
	rcCell.top = rcDraw.top;
	rcCell.right = rcCell.left + CellWidth;
	rcCell.bottom = rcDraw.bottom;

	for(Number = First; Number < Last; Number += 1) {

		switch ((UINT)(State->State[Number - State->First].Type)) {
			case CPU_STATE_RUN:
				hbrCell = Context->hbrRun;
				break;
			case CPU_STATE_WAIT:
				hbrCell = Context->hbrWait;
				break;
			case CPU_STATE_IO:
				hbrCell = Context->hbrIo;
				break;
			case CPU_STATE_SLEEP:
				hbrCell = Context->hbrSleep;
				break;
		}

		FillRect(hdc, &rcCell, hbrCell);
		rcCell.left = rcCell.right + Context->CellBorder;
		rcCell.right = rcCell.left + CellWidth;
	}
	
	
    return CDRF_SKIPDEFAULT;
}

LRESULT
CpuHistorySetStateColumnWidth(
	__in PDIALOG_OBJECT Object,
	__in HWND hWndList,
	__in ULONG First,
	__in ULONG Last
	)
{
	PCPU_HISTORY_FORM_CONTEXT Context;
	int CellWidth;
	int Required;
    HWND hWndCtrl;
    HDITEM hdi = {0};
    LRESULT Result;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	if (!Context){
		return 0;
	}

	//
	// Compute the required column width to hold the samples
	//

	CellWidth = Context->Scale;
	Required = (Last - First) * (CellWidth + Context->CellBorder) + Context->HoriEdge * 2;

    //
    // Adjust header column width to be same as listview's column width
    //
    
	ListView_SetColumnWidth(hWndList, CPU_HISTORY_STATE, Required);

    hWndCtrl = ListView_GetHeader(hWndList);
    ASSERT(hWndCtrl != NULL);

    hdi.mask = HDI_WIDTH;
	hdi.cxy = Required;

    //
    // N.B. Only allow change the header column width here,
    // ban all others, especially user input.
    //

    Context->AllowHeaderChange = TRUE;
    Result = Header_SetItem(hWndCtrl, CPU_HISTORY_STATE, &hdi);
    Context->AllowHeaderChange = FALSE;

    return Result;
}

LRESULT
CpuHistoryOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR pNmhdr
	)
{
	LRESULT Status = CDRF_DODEFAULT;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;
	PCPU_HISTORY_FORM_CONTEXT Context;

	if (pNmhdr->idFrom != IDC_LIST_CPU_HISTORY_THREAD) {
		return 0;
	}

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	ASSERT(Context != NULL);

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT;
            break;
        
        case CDDS_SUBITEM|CDDS_ITEMPREPAINT: 

			if (lvcd->iSubItem != CPU_HISTORY_STATE) { 
				Status = CDRF_DODEFAULT;
			}
			else {

				//
				// Only require erase background, we draw text when
				// receive CDDS_ITEMPOSTPAINT
				//
				
				Status = CpuHistoryDrawThreadStateLite(Object, pNmhdr->hwndFrom, lvcd);
				Status |= CDDS_ITEMPOSTPAINT;
			}
			break;
        
		case CDDS_ITEM|CDDS_ITEMPOSTPAINT:
			Status = CDRF_SKIPDEFAULT;
			break;

        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

LRESULT
CpuHistoryOnNotify(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    )
{
    LRESULT Status = 0;
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);

	switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return CpuHistoryOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			Status = CpuHistoryOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;

		case LVN_ITEMCHANGED:
			if(IDC_LIST_CPU_HISTORY_THREAD == pNmhdr->idFrom) {
				Status = CpuHistoryOnItemChanged(Object, (LPNMLISTVIEW)lp);
			}
			break;

        case NM_CLICK:
            Status = CpuHistoryOnNmClick(Object, (LPNMITEMACTIVATE)lp);
            break;
        
        case NM_HOVER:
            Status = CpuHistoryOnNmHover(Object, (LPNMHDR)lp);
            break;

		case TTN_GETDISPINFO:
			Status = CpuHistoryOnTtnGetDispInfo(Object, (LPNMTTDISPINFO)lp);
			break;

		case NM_DBLCLK:
			Status = CpuHistoryOnDbClick(Object, (LPNMITEMACTIVATE)lp);
			break;
	}

	return Status;
}

LRESULT 
CpuHistoryOnTtnGetDispInfo(
    __in PDIALOG_OBJECT Object,
	__in LPNMTTDISPINFO lpnmtdi
	)
{
	lpnmtdi->uFlags = TTF_DI_SETITEM;

	switch (lpnmtdi->hdr.idFrom) {
		case IDM_SAMPLE_BACKWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Sample Backward");
			break;
		case IDM_SAMPLE_FORWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Sample Forward");
			break;
		case IDM_SAMPLE_DOWNWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Sample Downward");
			break;
		case IDM_SAMPLE_UPWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Sample Upward");
			break;
		case IDM_SAMPLE_FULLSTACK:
			StringCchCopy(lpnmtdi->szText, 80, L"Full Stack");
			break;
		case IDM_SAMPLE_REPORT:
			StringCchCopy(lpnmtdi->szText, 80, L"Flame Report");
			break;
	}

	return 0;
}

LRESULT 
CpuHistoryOnNmClick(
    __in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	)
{
    PCPU_HISTORY_FORM_CONTEXT Context;
    LVHITTESTINFO info = {0};
    int index;
    int offset;
    RECT rcBound;
	RECT rcSample;
    HWND hWndCtrl;
    POINT pt;
	PCPU_THREAD Thread;
	HDC hdc;
    BOOLEAN Clipped;

    if (lpnmitem->hdr.idFrom != IDC_LIST_CPU_HISTORY_THREAD) {
        return 0;
    }

    if (!(lpnmitem->iItem != -1 && lpnmitem->iSubItem == 1)) {
        return 0;
    }

    hWndCtrl = lpnmitem->hdr.hwndFrom;
    index = lpnmitem->iItem;
    pt = lpnmitem->ptAction;

    //
    // If the click falls out of listview's bounding rect, do nothing
    //

	ListView_GetSubItemRect(hWndCtrl, index, CPU_HISTORY_STATE, LVIR_BOUNDS, &rcBound);
    if (!PtInRect(&rcBound, pt)) {
        return 0;
    }

    //
    // If the click fall into edges of state column, do nothing
    //

    Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    ASSERT(Context != NULL);

    if (pt.x < rcBound.left + Context->HoriEdge || pt.x > rcBound.right - Context->HoriEdge) {
        return 0;
    }

    //
    // Compute the offset to the first sample
    //

    offset = (pt.x - rcBound.left - Context->HoriEdge) / (Context->Scale + Context->CellBorder);

    //
    // Ensure the sample is clipped into visible range
    //

    ListViewGetParam(hWndCtrl, index, (LPARAM *)&Thread);
    ASSERT(Thread != NULL);
    Clipped = CpuHistoryIsSampleClipped(Context, Thread, Context->FirstSample + offset);
    if (!Clipped) {
        return -1;
    }

    //
    // Track and set picked sample
    //

    Context->SamplePickerPos = pt.x;
    Context->OldItem = Context->CurrentItem;
    Context->OldSample = Context->CurrentSample;
    Context->CurrentItem = index;
    Context->CurrentSample = Context->FirstSample + offset;

	rcSample.left = rcBound.left + Context->HoriEdge + offset * (Context->Scale + Context->CellBorder);
    rcSample.right = rcSample.left + Context->Scale;
    rcSample.top = rcBound.top + Context->VertEdge;
    rcSample.bottom = rcBound.bottom - Context->VertEdge;
	Context->IsSampleHover = TRUE;

    //
    // Redraw the old sample
    //

	if (Context->OldItem != -1) {
	    ListViewGetParam(hWndCtrl, Context->OldItem, (LPARAM *)&Thread);
		ASSERT(Thread != NULL);
	    hdc = GetDC(hWndCtrl);
	    CpuHistoryRedrawSampleRect(Context, Thread, hWndCtrl, hdc, Context->OldItem, Context->OldSample);
	    ReleaseDC(hWndCtrl, hdc);
	}

    //
    // Force redraw the current sample
    //

	InvalidateRect(hWndCtrl, &rcSample, TRUE);
    CpuHistorySetCurrentSample(Object, index, Context->CurrentSample);
    return 0;
}

int
CpuHistoryHitTestSample(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in HWND hWndCtrl,
    __in LPPOINT pt,
    __out PULONG SampleIndex,
    __out LPRECT rcSample
    )
{
    LVHITTESTINFO info;
    int index;
    RECT rcBound;

    //
    // Only check subitem, the state column
    //

    info.pt = *pt;
    index =  ListView_SubItemHitTest(hWndCtrl, &info);
    if (index == -1 || info.iSubItem != (int)CPU_HISTORY_STATE) {
        return -1;
    }

    //
    // If the click falls out of listview's bounding rect, do nothing
    //

    ListView_GetSubItemRect(hWndCtrl, info.iItem, CPU_HISTORY_STATE, LVIR_BOUNDS, &rcBound);
    if (!PtInRect(&rcBound, *pt)) {
        return -1;
    }

    //
    // If the click fall into edges of state column, do nothing
    //

    if (pt->x < rcBound.left + Context->HoriEdge || pt->x > rcBound.right - Context->HoriEdge) {
        return -1;
    }

    //
    // Compute the cell's offset to the first sample
    //

    index = (pt->x - rcBound.left - Context->HoriEdge) / (Context->Scale + Context->CellBorder);
    *SampleIndex = index;

    //
    // Compute the cell's bounding rect
    //

    rcSample->left = rcBound.left + Context->HoriEdge + index * (Context->Scale + Context->CellBorder);
    rcSample->right = rcSample->left + Context->Scale;
    rcSample->top = rcBound.top + Context->VertEdge;
    rcSample->bottom = rcBound.bottom - Context->VertEdge;

    //
    // Return index of listview item
    //

    return info.iItem;
}

LRESULT 
CpuHistoryOnNmHover(
    __in PDIALOG_OBJECT Object,
	__in LPNMHDR lpnmhdr
	)
{
    PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    ULONG index;
    int item;
    RECT rcSample;
    POINT pt;
    PCPU_THREAD Thread = NULL;
    HDC hdc;
    BOOLEAN Clipped;

    if (lpnmhdr->idFrom != IDC_LIST_CPU_HISTORY_THREAD) {
        return 0;
    }
    
    hWndCtrl = lpnmhdr->hwndFrom;
	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;

    GetCursorPos(&pt);
    ScreenToClient(hWndCtrl, &pt);
	
	item = CpuHistoryHitTestSample(Context, hWndCtrl, &pt, &index, &rcSample);
	if (item == -1) {
		return -1;
	}

    //
    // Ensure the sample is clipped into visible range
    //

    ListViewGetParam(hWndCtrl, item, (LPARAM *)&Thread);
    ASSERT(Thread != NULL);
    Clipped = CpuHistoryIsSampleClipped(Context, Thread, Context->FirstSample + index);
    if (!Clipped) {
        return -1;
    }

    //
    // Track and set picked sample
    //

    Context->SamplePickerPos = pt.x;
    Context->OldItem = Context->CurrentItem;
    Context->OldSample = Context->CurrentSample;
    Context->CurrentItem = item;
    Context->CurrentSample = Context->FirstSample + index;
	Context->IsSampleHover = TRUE;

    //
    // Redraw the old sample
    //

	if (Context->OldItem != -1) {
        ListViewGetParam(hWndCtrl, Context->OldItem, (LPARAM *)&Thread);
        ASSERT(Thread != NULL);
	    hdc = GetDC(hWndCtrl);
	    CpuHistoryRedrawSampleRect(Context, Thread, hWndCtrl, hdc, Context->OldItem, Context->OldSample);
	    ReleaseDC(hWndCtrl, hdc);
	}

    //
    // Force redraw the current sample
    //

	InvalidateRect(hWndCtrl, &rcSample, TRUE);
    CpuHistorySetCurrentSample(Object, item, Context->CurrentSample);
    return 0;
}

LRESULT 
CpuHistoryOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
    PCPU_THREAD Thread;
    ULONG StackId;

    if (lpNmlv->lParam == 0) {
        return 0;
    }

    if (lpNmlv->uNewState & LVIS_SELECTED) {

        Thread = (PCPU_THREAD)lpNmlv->lParam;
        StackId = CpuHistoryGetStackIdByThread(Object, Thread);
        CpuHistoryInsertBackTrace(Object->hWnd, StackId);

#ifdef _DEBUG
        {
            PCPU_HISTORY_FORM_CONTEXT Context;
            Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
            DebugTrace("CPU: tid %u, sample %u", Thread->ThreadId, Context->CurrentSample);
        }
#endif
    }

    return 0L;
}

LRESULT 
CpuHistoryOnColumnClick(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
    )
{
    HWND hWndHeader;
	int nColumnCount;
	int i;
	HDITEM hdi;
	LISTVIEW_OBJECT *ListView;
	PCPU_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;
    BOOLEAN IsThreadSort;

	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	ListView = Context->ListView;

    hWnd = Object->hWnd;
	hWndCtrl = lpNmlv->hdr.hwndFrom; 
    IsThreadSort = (lpNmlv->hdr.idFrom == IDC_LIST_CPU_HISTORY_THREAD) ? TRUE : FALSE;

    //
    // Sort only thread listview, skip stack trace listview
    //

    if (!IsThreadSort) {
        return 0;
    }

    if (ListView->SortOrder == SortOrderNone){
        return 0;
    }

	if (ListView->LastClickedColumn == lpNmlv->iSubItem) {
		ListView->SortOrder = (LIST_SORT_ORDER)!ListView->SortOrder;
    } else {
		ListView->SortOrder = SortOrderAscendent;
    }
	
    hWndHeader = ListView_GetHeader(hWndCtrl);
    ASSERT(hWndHeader);

    nColumnCount = Header_GetItemCount(hWndHeader);
    
    for (i = 0; i < nColumnCount; i++) {
        hdi.mask = HDI_FORMAT;
        Header_GetItem(hWndHeader, i, &hdi);
        
        if (i == lpNmlv->iSubItem) {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
            if (ListView->SortOrder == SortOrderAscendent){
                hdi.fmt |= HDF_SORTUP;
            } else {
                hdi.fmt |= HDF_SORTDOWN;
            }
        } else {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        } 
        
        Header_SetItem(hWndHeader, i, &hdi);
    }
    
	ListView->LastClickedColumn = lpNmlv->iSubItem;
    ListView_SortItemsEx(hWndCtrl, CpuHistorySortCallback, (LPARAM)hWnd);
    return 0L;
}

int CALLBACK
CpuHistorySortCallback(
    __in LPARAM First, 
    __in LPARAM Second,
    __in LPARAM Param
    )
{
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
    PBTR_CPU_SAMPLE Thread1;
    PBTR_CPU_SAMPLE Thread2;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	HWND hWnd;
    int Result;
	HWND hWndList;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_THREAD);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Thread1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Thread2);

    if (ListView->LastClickedColumn == CPU_HISTORY_TID) {
        Result = Thread1->ThreadId - Thread2->ThreadId;
	}
	
	if (ListView->LastClickedColumn == CPU_HISTORY_TIME) { 
        Result = (Thread1->KernelTime + Thread1->UserTime) - (Thread2->KernelTime + Thread2->UserTime);
	}
	
    if (ListView->LastClickedColumn == CPU_HISTORY_TIME_PERCENT) {

        double f1, f2;

	    ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);

        f1 = _wtof(FirstData);
        f2 = _wtof(SecondData);

        if (f1 >= f2) {
            Result = 1;
        } else {
            Result = -1;
        }
	}
	
	if (ListView->LastClickedColumn == CPU_HISTORY_CYCLES) {
        Result = Thread1->Cycles - Thread2->Cycles;
	}

	if (ListView->LastClickedColumn == CPU_HISTORY_STATE) {
	    ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);
		Result = wcsicmp(FirstData, SecondData);
	}
	
	return ListView->SortOrder ? Result : -Result;
}

VOID
CpuHistoryInsertData(
    __in HWND hWnd,
    __in PPF_REPORT_HEAD Head
    )
{
	HWND hWndCtrl;
	PTRACK_CONTROL Track;
	PDIALOG_OBJECT Object;
	PCPU_HISTORY_FORM_CONTEXT Context;
	RECT Rect;
	PPF_STREAM_SYSTEM System;
	LONG MergeSteps;

    CpuHistoryInsertThreads(hWnd, Head);
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;

	//
	// Set history data to trackbar
	//
	
	hWndCtrl = GetDlgItem(hWnd, IDC_CPU_HISTORY_GRAPH);
    ASSERT(hWndCtrl != 0);

	GetClientRect(hWndCtrl, &Rect);
	Track = (PTRACK_CONTROL)SdkGetObject(hWndCtrl);

	//
	// Compute the merge steps, maximum 10
	//

	System = (PPF_STREAM_SYSTEM)ApsGetStreamPointer(Head, STREAM_SYSTEM);
	MergeSteps = 1000 / System->Attr.SamplingPeriod;
	//MergeSteps = min(MergeSteps, 10);

	TrackSetHistoryData(Track, Context->ThreadTable->History, MergeSteps);
	TrackDrawHistory(Track, &Rect, 16);
	
	SetFocus(hWndCtrl);
	TrackSetSliderPosition(Track, 1);
}

VOID
CpuHistorySetCurrentRecord(
    __in HWND hWnd,
    __in ULONG Number
    )
{
    PDIALOG_OBJECT Object;
    PPF_REPORT_HEAD Head;
    PBTR_CPU_RECORD Record;
    PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    LVITEM lvi = {0};
    PBTR_CPU_SAMPLE Thread;
	WCHAR Buffer[MAX_PATH];

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    ASSERT(Object != NULL);

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	Head = Context->Base.Head;
    ASSERT(Head != NULL);

    Record = CpuGetRecordByNumber(Head, Number);
    if (!Record) {
        return;
    }

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_THREAD);
    ASSERT(hWndCtrl != NULL);
     
    //
    // Clear current current record's threads
    //

    ListView_DeleteAllItems(hWndCtrl);

    //
    // Track the new frame and CPU record
    //

    Context->Record = Record;

    for(Number = 0; Number < Record->ActiveCount + Record->RetireCount; Number += 1) {

        Thread = &Record->Sample[Number];

        //
        // TID
        //

        lvi.iItem = Number;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT|LVIF_PARAM;
        lvi.lParam = (LPARAM)Thread;

        StringCchPrintf(Buffer, MAX_PATH, L"%u", Thread->ThreadId);
        lvi.pszText = Buffer;
        ListView_InsertItem(hWndCtrl, &lvi);

		//
		// History, we will custom draw this subitem
		//

		lvi.iSubItem = 1;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = L"";
        ListView_SetItem(hWndCtrl, &lvi);

		/*
        //
        // Time (ms)
        //

        lvi.iSubItem = 1;
        lvi.mask = LVIF_TEXT;

        Milliseconds = ApsNanoUnitToMilliseconds(Thread->KernelTime + Thread->UserTime);
        StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Milliseconds);

        lvi.pszText = Buffer;
        ListView_SetItem(hWndCtrl, &lvi);

        //
        // Cycles
        //

        lvi.iSubItem = 2;
        lvi.mask = LVIF_TEXT;
        StringCchPrintf(Buffer, MAX_PATH, L"%u", Thread->Cycles);
        lvi.pszText = Buffer; 
        ListView_SetItem(hWndCtrl, &lvi);

        //
        // Time (%)
        //

        lvi.iSubItem = 3;
        lvi.mask = LVIF_TEXT;
        
        //
        // If thread cycles is available, use it since it's more accurate,
        // this also avoid divide zero
        //

        if (Record->Cycles != 0) {
            Percent = (Thread->Cycles * 100.0f) / Record->Cycles;
        } 

        //
        // If thread cycles is not available, use time instead
        // this also avoid divide zero
        //

        else if (Record->KernelTime + Record->UserTime != 0) {
            Percent = ((Thread->KernelTime + Thread->UserTime) * 100.0f) / 
                       (Record->KernelTime + Record->UserTime);
        }

        else {
            Percent = 0.0;
        }

        StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Percent);
        lvi.pszText = Buffer;
        ListView_SetItem(hWndCtrl, &lvi);

        //
        // State 
        //

        lvi.iSubItem = 4;
        lvi.mask = LVIF_TEXT;

        if (Number < Record->ActiveCount) {
            lvi.pszText = L"Live";
        } 
        else {
            lvi.pszText = L"Retired";
        }

        ListView_SetItem(hWndCtrl, &lvi);
		*/

    }

	/*
    //
    // Sort the threads by its time percent
    //

	ListView = Context->ListView;
    ListView->SortOrder = SortOrderDescendent;
    ListView->LastClickedColumn = CPU_HISTORY_TIME_PERCENT;
    ListView_SortItemsEx(hWndCtrl, CpuHistorySortCallback, (LPARAM)hWnd);
	*/

    //
    // Set focus to select item 0 and trigger a data update into
    // right pane
    //

    SetFocus(hWndCtrl);
    ListViewSelectSingle(hWndCtrl, 0);
}

ULONG
CpuHistoryGetStackIdByThread(
    __in PDIALOG_OBJECT Object,
    __in PCPU_THREAD Thread
    )
{ 
    PCPU_HISTORY_FORM_CONTEXT Context;
    PBTR_CPU_RECORD Record;
    ULONG StackId;
    ULONG Number;
    PBTR_CPU_SAMPLE Sample;

    Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    if (!Context->Record) {
        return 0;
    }

    Record = Context->Record;
    StackId = -1;

    for(Number = 0; Number < Record->ActiveCount + Record->RetireCount; Number += 1) {
        Sample = &Record->Sample[Number];
        if (Sample->ThreadId == Thread->ThreadId) {
            StackId = Sample->StackId;
            break;
        }
    }

    return StackId;
}

VOID
CpuHistorySetCurrentSample(
    __in PDIALOG_OBJECT Object,
    __in int Item,
    __in ULONG Number
    )
{
    PPF_REPORT_HEAD Head;
    PBTR_CPU_RECORD Record;
    PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    LVITEM lvi = {0};
    PCPU_THREAD Thread;
    ULONG StackId;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	Head = Context->Base.Head;
    ASSERT(Head != NULL);

    Record = CpuGetRecordByNumber(Head, Number);
    if (!Record) {
        return;
    }
    
    Context->CurrentSample = Number;
    Context->Record = Record;

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_HISTORY_THREAD);
    ASSERT(hWndCtrl != NULL);
     
    //
    // Get selected thread
    //

    ListViewGetParam(hWndCtrl, Item, (LPARAM *)&Thread);
    ASSERT(Thread != NULL);

    //
    // Track the new frame and CPU record
    //

    StackId = CpuHistoryGetStackIdByThread(Object, Thread);
    CpuHistoryInsertBackTrace(Object->hWnd, StackId);

    DebugTrace("CPU: tid %u, sample %u", Thread->ThreadId, Number);
}

VOID
CpuHistoryInsertThreads(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
    )
{
    PDIALOG_OBJECT Object;
    PCPU_HISTORY_FORM_CONTEXT Context;
	PCPU_THREAD_TABLE Table;
	PCPU_THREAD *Thread;
	ULONG Size;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	Context->Base.Head = Head;

	Table = CpuHistoryGetThreadTable();
	ASSERT(Table != NULL);

	Context->ThreadTable = Table;

	//
	// Make our own copy of thread object array to speed up access
	//
	
	Size = sizeof(PVOID) * Table->Count;
	Thread = (PCPU_THREAD *)ApsMalloc(Size);
	memcpy_s(Thread, Size, Table->Thread, Size);

    Context->Thread = Thread;
	Context->NumberOfThreads = Table->Count;

    Size = sizeof(ULONG) * Table->Count;
	Context->ThreadIndex = (PULONG)ApsMalloc(Size);
	Context->NumberOfIndexes = 0;
}

VOID
CpuHistoryInsertBackTrace(
	__in HWND hWnd,
    __in ULONG StackTraceId 
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT ObjectContext;
	PPF_REPORT_HEAD Report;
	PBTR_STACK_RECORD Record;
	HWND hWndCtrl;
	PBTR_TEXT_TABLE Table;
	PBTR_TEXT_ENTRY Text;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_LINE_ENTRY Line;
	WCHAR Buffer[MAX_PATH];
	ULONG i;
	LVITEM lvi = {0};

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_HISTORY_STACK); 

    if (StackTraceId == -1) {
        ListView_DeleteAllItems(hWndCtrl);
        return;
    }

	ObjectContext = SdkGetContext(Object, CPU_FORM_CONTEXT);
	Report = ObjectContext->Head;

	if (!Report) {
		return;
	}

	//
	// Clear old list items
	//

	ListView_DeleteAllItems(hWndCtrl);

    Record = (PBTR_STACK_RECORD)ApsGetStreamPointer(Report, STREAM_STACK);
	Record = &Record[StackTraceId];

	if (Report->Streams[STREAM_LINE].Offset != 0 && 
		Report->Streams[STREAM_LINE].Length != 0) {
		Line = (PBTR_LINE_ENTRY)((PUCHAR)Report + Report->Streams[STREAM_LINE].Offset);
	} else {
		Line = NULL;
	}

	Table = (PBTR_TEXT_TABLE)Report->Context;
	if (!Table) {

		PBTR_TEXT_FILE TextFile;
		
		TextFile = (PBTR_TEXT_FILE)((PUCHAR)Report + Report->Streams[STREAM_SYMBOL].Offset);
		Table = ApsBuildSymbolTable(TextFile, 4093);
		Report->Context = (PVOID) Table;
	}

	for(i = 0; i < Record->Depth; i++) {

		Text = ApsLookupSymbol(Table, (ULONG64)Record->Frame[i]);

		//
		// frame number
		//

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%02u", i);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);
		
		//
		// symbol name
		//

		lvi.iItem = i;
		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;

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

		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// line information
		//

		lvi.iItem = i;
		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;

		if (Text->LineId != -1) {
			ASSERT(Line != NULL);
			LineEntry = Line + Text->LineId;
			StringCchPrintf(Buffer, MAX_PATH, L"%S:%u", LineEntry->File, LineEntry->Line);
			lvi.pszText = Buffer; 
		} else {
			lvi.pszText = L""; 
		}
		
		ListView_SetItem(hWndCtrl, &lvi);

	}
}

LRESULT
CpuHistoryOnGraphSetRecord(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    //
    // WPARAM contains current record number
    //

    CpuHistorySetCurrentRecord(hWnd, (ULONG)wp);
	return 0;
}

LRESULT
CpuHistoryOnTrackSetRange(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PDIALOG_OBJECT Object;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    ASSERT(Object != NULL);

    CpuHistorySetRecordRange(Object, (ULONG)wp, (ULONG)lp);
	return 0;
}

LRESULT
CpuHistoryOnParentNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuHistoryOnLButtonDown(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

int
CpuHistoryPositionStateBar2(
    __in PDIALOG_OBJECT Object,
    __in PSTATEBAR_OBJECT StateBar,
    __in LPRECT rcTrack
    )
{
    RECT rc;
    PCPU_HISTORY_FORM_CONTEXT Context;

    Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
    ASSERT(StateBar != NULL);
    
    StateBarInsertBands(StateBar);

    rc = *rcTrack;
    rc.top = rcTrack->bottom + 4;
    rc.bottom = rc.top + 22;

    MoveWindow(StateBar->hWndStateBar, rc.left, rc.top, rc.right - rc.left,
               rc.bottom - rc.top, TRUE);

    return rc.bottom + 6;
}

int
CpuHistoryPositionStateBar(
    __in PDIALOG_OBJECT Object,
    __in LPRECT rcTrack
    )
{
    HWND hWndCtrl;
    RECT rcCtrl;
    int Height;
    int dx, dy;

    //
    // Compute move delta and move Run color
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC_RUN_COLOR);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);

    Height = rcCtrl.bottom - rcCtrl.top + 6 * 2;

    dx = 6 - rcCtrl.left;
    dy = rcTrack->bottom + 6 + rcCtrl.bottom - rcCtrl.top - rcCtrl.bottom;

    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    //
    // Move Run text
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC_RUN_TEXT);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);
    
    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    //
    // Move Wait color 
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC_WAIT_COLOR);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);
    
    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    //
    // Move Wait text
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC_WAIT_TEXT);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);
    
    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    //
    // Move IO color 
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC_IO_COLOR);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);
    
    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    //
    // Move IO text 
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC_IO_TEXT);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);
    
    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    //
    // Move ZOOM bar 
    //

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_SLIDER_ZOOM);
    GetWindowRect(hWndCtrl, &rcCtrl);
    MapWindowRect(HWND_DESKTOP, Object->hWnd, &rcCtrl);
    
    OffsetRect(&rcCtrl, dx, dy);
    MoveWindow(hWndCtrl, rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left,
        rcCtrl.bottom - rcCtrl.top, TRUE);

    return rcTrack->bottom + Height;
}

LRESULT
CpuHistoryOnCtlColorStatic(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
    PDIALOG_OBJECT Object;
	PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
	LRESULT Result;
	RECT rc;
	HDC hdc;
	int Id;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	if (!Object) {
		return 0;
	}

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	ASSERT(Context != NULL);

	hdc = (HDC)wp;
	hWndCtrl = (HWND)lp;
	Id = GetDlgCtrlID(hWndCtrl);
	Result = 0;

	switch (Id) {
		case IDC_STATIC_RUN_COLOR:
			GetClientRect(hWndCtrl, &rc);
			FillRect(hdc, &rc, Context->hbrRun);
			break;
		case IDC_STATIC_RUN_TEXT:
			break;
		case IDC_STATIC_WAIT_COLOR:
			GetClientRect(hWndCtrl, &rc);
			FillRect(hdc, &rc, Context->hbrWait);
			break;
		case IDC_STATIC_WAIT_TEXT:
			break;
		case IDC_STATIC_IO_COLOR:
			GetClientRect(hWndCtrl, &rc);
			FillRect(hdc, &rc, Context->hbrIo);
			break;
		case IDC_STATIC_IO_TEXT:
			break;
	}

	return Result;
}

VOID
CpuHistorySetRecordRange(
    __in PDIALOG_OBJECT Object,
    __in ULONG First,
	__in ULONG Last
    )
{
    PPF_REPORT_HEAD Head;
    PCPU_HISTORY_FORM_CONTEXT Context;
    HWND hWndCtrl;
    LVITEM lvi = {0};
    PCPU_THREAD Thread;
    PLISTVIEW_OBJECT ListView;
	WCHAR Buffer[MAX_PATH];
	ULONG Number = 0;
	ULONG Count;
    ULONG Index;
    ULONG LastThreadId;
    ULONG LastThreadIndex; 
    static BOOLEAN FirstRun = TRUE;

	Context = (PCPU_HISTORY_FORM_CONTEXT)Object->Context;
	Head = Context->Base.Head;
    ASSERT(Head != NULL);

	//
	// Ensure the index is not overflow sample depth
	//

	if (CpuGetSampleDepth(Head) < Last + 1) {
		return;
	}

	//
	// Adjust required column width
	//

    hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_HISTORY_THREAD);
    ASSERT(hWndCtrl != NULL);
     
    LastThreadIndex = -1;
    LastThreadId = ListViewGetFirstSelected(hWndCtrl);
    if (LastThreadId != -1) {
        ListViewGetParam(hWndCtrl, LastThreadId, (LPARAM *)&Thread);
        ASSERT(Thread != NULL);
        LastThreadId = Thread->ThreadId;
    } 

    ListView_DeleteAllItems(hWndCtrl);

	CpuHistorySetStateColumnWidth(Object, hWndCtrl, First, Last);

	//
	// Pick up the threads that fall into the sample period
	//

	Count = CpuBuildThreadListByRange(Context->Thread, Context->NumberOfThreads,
									First, Last, Context->ThreadIndex);
	ASSERT(Count != 0);

	Context->NumberOfIndexes = Count;
	Context->FirstSample = First;
	Context->LastSample = Last;
    Context->CurrentItem = INVALID_VALUE;
    Context->CurrentSample = INVALID_VALUE;
    Context->OldItem = INVALID_VALUE;
    Context->OldSample = INVALID_VALUE;

    //
    // Insert the thread list
    //

    for(Number = 0; Number < Count; Number += 1) {

        Index = Context->ThreadIndex[Number];
        Thread = Context->Thread[Index];

        if (Thread->ThreadId == LastThreadId) {

            //
            // The thread is clipped, still valid, we will
            // focus on it after insertion
            //

            LastThreadIndex = Number;
        }

        //
        // TID
        //

        lvi.iItem = Number;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT|LVIF_PARAM;
        lvi.lParam = (LPARAM)Thread;

        StringCchPrintf(Buffer, MAX_PATH, L"%u", Thread->ThreadId);
        lvi.pszText = Buffer;
        ListView_InsertItem(hWndCtrl, &lvi);

		//
		// History, we will custom draw this subitem
		//

		lvi.iSubItem = 1;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = L"";
        ListView_SetItem(hWndCtrl, &lvi);

        //
        // Draw the thread state in memory DC
        //

        CpuHistoryDrawThreadStateInMemory(Context, hWndCtrl, Thread);
        ListView_RedrawItems(hWndCtrl, Number, Number);
    }

    //
    // Sort the threads by its time percent
    //

	ListView = Context->Base.ListView;
    ListView->SortOrder = SortOrderDescendent;
    ListView->LastClickedColumn = CPU_HISTORY_TID;
    //ListView_SortItemsEx(hWndCtrl, CpuHistorySortCallback, (LPARAM)hWnd);

    //
    // Select the first thread
    //

    if (LastThreadIndex == (ULONG)-1) {
        LastThreadIndex = 0;
    }

    ListViewSelectSingle(hWndCtrl, LastThreadIndex);
    CpuHistorySetCurrentSample(Object, LastThreadIndex, First);
    
    SetFocus(hWndCtrl);
}

PCPU_THREAD_TABLE
CpuHistoryGetThreadTable(
    VOID
    )
{
    PFRAME_OBJECT Frame;
    HWND hWndForm;
    PCPU_THREAD_TABLE Table;

    Frame = MainGetFrame();

    //
    // N.B. We rely on thread form to create per thread data,
    // just hide it
    //

    hWndForm = Frame->CpuForm.hWndForm[CPU_FORM_THREAD];

    if (!hWndForm) {
        hWndForm = CpuThreadCreate(Frame->hWnd, CPU_FORM_THREAD);
        Frame->CpuForm.hWndForm[CPU_FORM_THREAD] = hWndForm;
        CpuThreadInsertData(hWndForm, Frame->Head);
    }
    ShowWindow(hWndForm, SW_HIDE);

    Table = CpuThreadGetTable(hWndForm);
    return Table;
}

LRESULT 
CpuHistoryOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	)
{
	HWND hWndList;
	PWCHAR Ptr;
	WCHAR Buffer[MAX_PATH];
	size_t Length;
	ULONG Line;

	//
	// Check whether source column is clicked
	//

	if (lpnmitem->iSubItem != 2) {
		return 0;
	}

	//
	// Ensure it targets stack listview
	//

	if (lpnmitem->hdr.idFrom != IDC_LIST_CPU_HISTORY_STACK) {
		return 0;
	}

	//
	// Check whether there's any source information
	//

	hWndList = lpnmitem->hdr.hwndFrom;

	Buffer[0] = 0;
	ListView_GetItemText(hWndList, lpnmitem->iItem, 2, Buffer, MAX_PATH);

	Length = wcslen(Buffer);
	if (!Length) {

		//
		// there's no source information
		//

		return 0;
	}

	Ptr = wcsrchr(Buffer, L':');
	ASSERT(Ptr != NULL);

	Line = _wtoi(Ptr + 1);
	Ptr[0] = 0;

	FrameShowSource(Object->hWnd, Buffer, Line);
	return 0;
}