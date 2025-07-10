//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "aps.h"
#include "apsdefs.h"
#include "apsprofile.h"
#include "treelist.h"
#include "cputhread.h"
#include "profileform.h"
#include "split.h"
#include "apspdb.h"
#include "frame.h"
#include "apscpu.h"
#include "apsrpt.h"
#include "cpupcstack.h"
#include "resource.h"

static
DIALOG_SCALER_CHILD CpuThreadChildren[3] = {
	{ IDC_LIST_CPU_THREAD_THREAD, AlignNone, AlignBottom },
	{ IDC_SPLIT, AlignNone, AlignBottom },
	{ IDC_LIST_CPU_THREAD_PC, AlignRight, AlignBottom }
};

static
DIALOG_SCALER CpuThreadScaler = {
	{0,0}, {0,0}, {0,0}, 3, CpuThreadChildren
};

//
// Left pane show all threads' properties
//

static
LISTVIEW_COLUMN CpuThreadColumn[] = {
	{ 40,  L"TID",      LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120, L"Start Address", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Time (ms)",LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Time %",   LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 50,  L"Priority", LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"State",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define CPU_THREAD_COLUMN_NUM  (sizeof(CpuThreadColumn)/sizeof(LISTVIEW_COLUMN))

//
// Right pane show PC statistics per thread
//

static 
LISTVIEW_COLUMN CpuPcColumn[] = {
	{ 160, L"Name",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Samples",	 LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Time %",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Module",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240, L"Line",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define CPU_PC_COLUMN_NUM  (sizeof(CpuPcColumn)/sizeof(LISTVIEW_COLUMN))

//
// Vertical splitter layout
//

static
SPLIT_INFO CpuThreadSplitInfo[2] = {
	{ IDC_LIST_CPU_THREAD_THREAD, SPLIT_RIGHT },
	{ IDC_LIST_CPU_THREAD_PC, SPLIT_LEFT }
};

//
// Vertical split object
//

static
SPLIT_OBJECT CpuThreadSplitObject = {
	TRUE, CpuThreadSplitInfo, 2, 20 
};

HWND
CpuThreadCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	HWND hWnd;
	
	Context = (PCPU_FORM_CONTEXT)SdkMalloc(sizeof(CPU_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_CPU_THREAD;
	Object->Procedure = CpuThreadProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CpuThreadOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	LVITEM lvi = {0};
	ULONG i;
	PLISTVIEW_OBJECT ListView;
	RECT Rect;
    RECT CtrlRect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	
	Context->hBrushBack = CreateSolidBrush(RGB(255, 255, 255));

	//
	// Create listview object wraps list control
	//

	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = CpuThreadColumn;
    ListView->Count = CPU_THREAD_COLUMN_NUM;
	ListView->NotifyCallback = CpuThreadOnNotify;
	
	Context->ListView = ListView;

    //
    // Initialize left pane
    //

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_THREAD);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

	for (i = 0; i < CPU_THREAD_COLUMN_NUM; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CpuThreadColumn[i].Title;	
		lvc.cx = CpuThreadColumn[i].Width;     
		lvc.fmt = CpuThreadColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Initialize splitbar
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_SPLIT);
	SplitSetObject(hWndCtrl, &CpuThreadSplitObject);

	//
	// Initialize right pane 
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_PC);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, 
                                        LVS_EX_FULLROWSELECT);

	for (i = 0; i < CPU_PC_COLUMN_NUM; i++) { 
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = CpuPcColumn[i].Title;	
		lvc.cx = CpuPcColumn[i].Width;     
		lvc.fmt = CpuPcColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Position controls 
	//

	GetClientRect(hWnd, &Rect);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_THREAD);

	CtrlRect.top = 0;
	CtrlRect.left = 0;
	CtrlRect.right = 400;
	CtrlRect.bottom = Rect.bottom;

	MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
		       CtrlRect.right - CtrlRect.left, 
			   CtrlRect.bottom - CtrlRect.top, TRUE);

	hWndCtrl = GetDlgItem(hWnd, IDC_SPLIT);

	CtrlRect.top = 0;
	CtrlRect.left = CtrlRect.right;
	CtrlRect.right = CtrlRect.left + 2;
	CtrlRect.bottom = Rect.bottom;

	MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
		       CtrlRect.right - CtrlRect.left, 
			   CtrlRect.bottom - CtrlRect.top, TRUE);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_PC);

	CtrlRect.top = 0;
	CtrlRect.left = CtrlRect.right;
	CtrlRect.right = Rect.right;
	CtrlRect.bottom = Rect.bottom;

	MoveWindow(hWndCtrl, CtrlRect.left, CtrlRect.top, 
		       CtrlRect.right - CtrlRect.left, 
			   CtrlRect.bottom - CtrlRect.top, TRUE);
	//
	// Register dialog scaler
	//

	Object->Scaler = &CpuThreadScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT CALLBACK 
CpuThreadHeaderProcedure(
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
CpuThreadOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuThreadOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return TRUE;
}

INT_PTR CALLBACK
CpuThreadProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return CpuThreadOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return CpuThreadOnClose(hWnd, uMsg, wp, lp);

	case WM_DRAWITEM:
		return CpuThreadOnDrawItem(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return CpuThreadOnNotify(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
		return CpuThreadOnCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
CpuThreadOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	UINT CommandId;

	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	CommandId = LOWORD(wp);

	switch (CommandId) {

	case ID_PCSTACK_STACKTRACE:
		return CpuThreadOnStackTrace(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
CpuThreadOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CpuThreadOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	LRESULT Status = 0L;
    return Status;
}

LRESULT
CpuThreadOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	POINT Screen;
	POINT Client;
	HMENU hPopupMenu = NULL;
	HMENU hMenuLoaded;
	RECT Rect;
	LPNMHDR lpnmhdr = (LPNMHDR)lp;

	if (lpnmhdr->idFrom != IDC_LIST_CPU_THREAD_PC) {
		return 0;
	}

	GetCursorPos(&Screen);

	//
	// Ensure mouse fall into right list control's client area
	//

	Client = Screen;
	ScreenToClient(lpnmhdr->hwndFrom, &Client);

	GetClientRect(lpnmhdr->hwndFrom, &Rect);
	if (!PtInRect(&Rect, Client)) {
		return 0;
	}

	//
	// ensure user has selected one item
	//

	if (!ListView_GetSelectedCount(lpnmhdr->hwndFrom)) {
		return 0;
	}

	//
	// Display the context menu
	//

	hMenuLoaded = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_PCSTACK));
	hPopupMenu = GetSubMenu(hMenuLoaded, 0);

	//
	// The menu command must be routed to dialog, not tabctrl
	//

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
				 Screen.x, Screen.y, 0, hWnd, NULL);
	DestroyMenu(hMenuLoaded);
	return TRUE;
}

LRESULT
CpuThreadOnNotify(
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
			return CpuThreadOnCustomDraw(Object, pNmhdr);

		case LVN_COLUMNCLICK:
			Status = CpuThreadOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;

		case NM_DBLCLK:
			if (IDC_LIST_CPU_THREAD_PC == pNmhdr->idFrom) {
				Status = CpuThreadOnDbClick(Object, (LPNMITEMACTIVATE)lp);
			}
			break;

		case LVN_ITEMCHANGED:
			if(IDC_LIST_CPU_THREAD_THREAD == pNmhdr->idFrom) {
				Status = CpuThreadOnItemChanged(Object, (LPNMLISTVIEW)lp);
			}
			break;
		case NM_RCLICK:
			Status = CpuThreadOnContextMenu(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT 
CpuThreadOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
    HWND hWndCtrl;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;
    PCPU_COUNTERS Thread;
    PPF_REPORT_HEAD Head;
    PCPU_FORM_CONTEXT Form;
	PBTR_LINE_ENTRY Line;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
    LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
    ULONG Number;
	double Percent;
	PCPU_FUNCTION_COUNTERS Function;
	PCPU_FUNCTION_ENTRY FuncEntry;
	PBTR_FUNCTION_ENTRY FuncTable;

    Form = (PCPU_FORM_CONTEXT)Object->Context;
    ASSERT(Form != NULL);

    Head = Form->Head;
    ASSERT(Head != NULL);

	if (lpNmlv->uNewState & LVIS_SELECTED) {

		//
		// Delete all items if any
		//

		hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_CPU_THREAD_PC);
        ListView_DeleteAllItems(hWndCtrl);

        ASSERT(lpNmlv->lParam != 0);
        Thread = (PCPU_COUNTERS)lpNmlv->lParam;

		//
		// Merge PCs into function if function id is available
		//

		CpuBuildOnCpuStatisticsEx(Head, Thread, &Function);

        //
        // Build symbol table to parse PC address 
        //

        TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);
        TextTable = ApsBuildSymbolTable(TextFile, 4093);

		DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
		DllEntry = &DllFile->Dll[0];

		Line = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE);
		LineEntry = Line;
		
		FuncTable = (PBTR_FUNCTION_ENTRY)ApsGetStreamPointer(Head, STREAM_FUNCTION);
		if (!FuncTable) {
			return 0;
		}

		for (Number = 0; Number < Function->FunctionCount; Number += 1) {

			FuncEntry = &Function->Function[Number];
			if (FuncEntry->Function.FunctionId != -1) {

				//
				// Fill real function address if function id is valid
				//

				FuncEntry->Function.Address = FuncTable[FuncEntry->Function.FunctionId].Address;
			}

			lvi.iItem = Number;
			lvi.iSubItem = 0;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.lParam = (LPARAM)FuncEntry;

			TextEntry = ApsLookupSymbol(TextTable, (ULONG64)FuncEntry->Function.Address);
			if (TextEntry) {
				StringCchPrintf(Buffer, MAX_PATH, L"%S", TextEntry->Text);
			}
			else {
				ApsFormatAddress(Buffer, MAX_PATH, (PVOID)FuncEntry->Function.Address, TRUE);
			}

			lvi.pszText = Buffer;
			ListView_InsertItem(hWndCtrl, &lvi);

			//
			// Samples 
			//

			lvi.iSubItem = 1;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%u", FuncEntry->Count);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Time %
			//

			Percent = (FuncEntry->KernelTime + FuncEntry->UserTime) * 100.0 / (Function->TotalKernelTime + Function->TotalUserTime) * 1.0;
			lvi.iSubItem = 2;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%.2f", Percent);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Dll 
			//

			lvi.iSubItem = 3;
			lvi.mask = LVIF_TEXT;

			ApsGetDllBaseNameById(Head, FuncEntry->Function.DllId, Buffer, MAX_PATH);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Line
			//

			if (TextEntry) {

				if (Line != NULL) {
					if (TextEntry->LineId != -1) {
						LineEntry = Line + TextEntry->LineId;
						StringCchPrintf(Buffer, MAX_PATH, L"%S:%u", LineEntry->File, LineEntry->Line);
					}
				}

				else {
					Buffer[0] = L'\0';
				}

				lvi.iSubItem = 4;
				lvi.mask = LVIF_TEXT;
				lvi.pszText = Buffer;
				ListView_SetItem(hWndCtrl, &lvi);
			}
			else {

				lvi.iSubItem = 4;
				lvi.mask = LVIF_TEXT;
				lvi.pszText = L"";
				ListView_SetItem(hWndCtrl, &lvi);
			}
		}

		ApsDestroySymbolTable(TextTable);
    }

    return 0L;
}

LRESULT 
CpuThreadOnColumnClick(
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

    if (ListView->SortOrder == SortOrderNone){
        return 0;
    }

	if (ListView->LastClickedColumn == lpNmlv->iSubItem) {
		ListView->SortOrder = (LIST_SORT_ORDER)!ListView->SortOrder;
    } else {
		ListView->SortOrder = SortOrderAscendent;
    }
    
	hWnd = Object->hWnd;
	hWndCtrl = lpNmlv->hdr.hwndFrom; 
    IsThreadSort = (lpNmlv->hdr.idFrom == IDC_LIST_CPU_THREAD_THREAD) ? TRUE : FALSE;

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

    if (IsThreadSort) {
        ListView_SortItemsEx(hWndCtrl, CpuThreadSortThreadCallback, (LPARAM)hWnd);
    } else {
        ListView_SortItemsEx(hWndCtrl, CpuThreadSortPcCallback, (LPARAM)hWnd);
    }

    return 0L;
}

int CALLBACK
CpuThreadSortThreadCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
    PCPU_COUNTERS Thread1, Thread2;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	HWND hWnd;
    int Result;
	HWND hWndList;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_THREAD);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	Result = 0;
	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Thread1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Thread2);


	if (ListView->LastClickedColumn == 0) {
        Result = Thread1->ThreadId - Thread2->ThreadId;
	}
	
	if (ListView->LastClickedColumn == 2 || ListView->LastClickedColumn == 3) {
        Result = (Thread1->TotalKernelTime + Thread1->TotalUserTime) - (Thread2->TotalKernelTime + Thread2->TotalUserTime);
	}
	
	if (ListView->LastClickedColumn == 4 || ListView->LastClickedColumn == 5) {
	    ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);
		Result = wcsicmp(FirstData, SecondData);
	}
	
	return ListView->SortOrder ? Result : -Result;
}

int CALLBACK
CpuThreadSortPcCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
	PCPU_PC_ENTRY Pc1, Pc2;
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	HWND hWnd;
    int Result;
	HWND hWndList;
	WCHAR FirstData[MAX_PATH];
	WCHAR SecondData[MAX_PATH];

	hWnd = (HWND)Param;

    //
    // Compare PC counters of each thread
    //

	hWndList = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_PC);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);

	Result = 0;
	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Pc1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Pc2);

	if (ListView->LastClickedColumn == 0 || 
		ListView->LastClickedColumn == 3 ||
		ListView->LastClickedColumn == 4) {

        ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);
		Result = wcsicmp(FirstData, SecondData);

	}
	
	//
	// Sort by Pc sample count
	//

    if (ListView->LastClickedColumn == 1) {
        Result = Pc1->Count - Pc2->Count;
	}
	
	//
	// Sort by Pc time percent
	//

    if (ListView->LastClickedColumn == 2) {
        Result = Pc1->KernelTime + Pc1->UserTime - (Pc2->KernelTime + Pc2->UserTime);
	}
	
	return ListView->SortOrder ? Result : -Result;
}

VOID
CpuThreadInsertThreads(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
    )
{
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;
    ULONG Number;
    HWND hWndCtrl;
    double Milliseconds;
    double TotalTimes;
	PCPU_COUNTERS Counter = NULL;
	PCPU_COUNTERS Thread;
	PBTR_CPU_THREAD BtrThread;
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	WCHAR Buffer[MAX_PATH];
	WCHAR Name[MAX_PATH];
    LVITEM lvi = {0};

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_FORM_CONTEXT)Object->Context;

    //
    // N.B. Must track head pointer herer
    //

    Context->Head = Head;

    // Scan the CPU sample records to generate threaded PC stream
    //

	CpuScanOnCpuThreaded(Head, CPU_COUNTER_ONCPU_THREADED, &Counter);
	if (!Counter->ThreadCount || !Counter->PcCount) {
		return;
	}

    TotalTimes = ApsNanoUnitToMilliseconds(
		(ULONG)(Counter->TotalKernelTime + Counter->TotalUserTime));

	//
	// Ensure not divide by zero
	//

	TotalTimes = max(0.0001, TotalTimes);

    //
    // Save the thread table in CPU_FORM_CONTEXT
    //

    Context->Context = Counter;

    TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);
	TextTable = ApsBuildSymbolTable(TextFile, 4093);

	DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
	DllEntry = &DllFile->Dll[0];

    //
    // Fill the threads into listview
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_THREAD);
    ASSERT(hWndCtrl != NULL); 

    //
    // Scan counter table to insert all threads, each listview item
    // attached a thread object as LPARAM 
    //

    Number = 0;
	for (Thread = CpuGetFirstCounter(Counter); Thread != NULL; 
		 Thread = CpuGetNextCounter(Counter, Thread)) {

		//
		// TID
		//

		ASSERT(Thread->PcCount != 0);

		lvi.iItem = Number;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.lParam = (LPARAM)Thread;

		StringCchPrintf(Buffer, MAX_PATH, L"%u", Thread->ThreadId);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndCtrl, &lvi);

		//
		// Start (Thread start address)
		//

		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;

		BtrThread = CpuGetBtrThreadObject(Head, Thread->ThreadId);
		ASSERT(BtrThread != NULL);

		TextEntry = ApsLookupSymbol(TextTable, (ULONG64)BtrThread->StartAddress);
		if (TextEntry) {
			StringCchPrintf(Name, MAX_PATH, L"%S", TextEntry->Text);
		}
		else {
			ApsFormatAddress(Name, MAX_PATH, BtrThread->StartAddress, TRUE);
		}
		
		WCHAR DllName[64];
		if (ApsGetDllBaseNameByAddress(Head, (ULONG_PTR)BtrThread->StartAddress, DllName, 63)) {
		}
		else {
			StringCchCopy(DllName, 64, L"Unknown");
		}

		StringCchPrintf(Buffer, MAX_PATH, L"%s!%s", DllName, Name);

		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Time (ms)
		//

		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;

		Milliseconds = ApsNanoUnitToMilliseconds(Thread->TotalKernelTime + Thread->TotalUserTime);
		StringCchPrintf(Buffer, MAX_PATH, L"%.2f", Milliseconds);

		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);
		
		//
		// Time (%)
		//

		lvi.iSubItem = 3;
		lvi.mask = LVIF_TEXT;
		StringCchPrintf(Buffer, MAX_PATH, L"%.2f", (Milliseconds * 100.0) / TotalTimes);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// Priority 
		//

		lvi.iSubItem = 4;
		lvi.mask = LVIF_TEXT;

		CpuTranslateThreadPriority(Buffer, MAX_PATH, CpuGetThreadPriority(Head, Thread->ThreadId));
		lvi.pszText = Buffer;
		ListView_SetItem(hWndCtrl, &lvi);

		//
		// State 
		//

		lvi.iSubItem = 5;
		lvi.mask = LVIF_TEXT;

		if (!CpuIsThreadRetired(Head, Thread->ThreadId)){
			lvi.pszText = L"Live";
		} 
		else {
			lvi.pszText = L"Retired";
		}
		ListView_SetItem(hWndCtrl, &lvi);

		Number += 1;
    }

    //
    // Set focus to select item 0 and trigger a data update into
    // right pane
    //

    SetFocus(hWndCtrl);
    ListViewSelectSingle(hWndCtrl, 0);
}

VOID
CpuThreadInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{
    CpuThreadInsertThreads(hWnd, Head);
    return;
}

VOID
CpuTranslateThreadPriority(
    __out PWCHAR Buffer,
    __in SIZE_T Length,
    __in ULONG Priority
    )
{
    switch (Priority) {
    case THREAD_PRIORITY_ABOVE_NORMAL:
        StringCchCopy(Buffer, Length, L"Above Normal");
        break;
    case THREAD_PRIORITY_BELOW_NORMAL:
        StringCchCopy(Buffer, Length, L"Below Normal");
        break;
    case THREAD_PRIORITY_HIGHEST:
        StringCchCopy(Buffer, Length, L"Highest");
        break;
    case THREAD_PRIORITY_IDLE:
        StringCchCopy(Buffer, Length, L"Idle");
        break;
    case THREAD_PRIORITY_LOWEST:
        StringCchCopy(Buffer, Length, L"Lowest");
        break;
    case THREAD_PRIORITY_NORMAL:
        StringCchCopy(Buffer, Length, L"Normal");
        break;
    case THREAD_PRIORITY_TIME_CRITICAL:
        StringCchCopy(Buffer, Length, L"Time Critical");
        break;
    default:

		//
		// For unknown value, directly print its priority value
		//

		StringCchPrintf(Buffer, Length, L"%u", Priority);
    }
}

LRESULT 
CpuThreadOnDbClick(
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

	if (lpnmitem->iSubItem != 6) {
		return 0;
	}

	//
	// Check whether there's any source information
	//

	hWndList = lpnmitem->hdr.hwndFrom;

	Buffer[0] = 0;
	ListView_GetItemText(hWndList, lpnmitem->iItem, 6, Buffer, MAX_PATH);

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


PCPU_THREAD_TABLE
CpuThreadGetTable(
	__in HWND hWnd
)
{
	return NULL;
}

LRESULT
CpuThreadOnStackTrace(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCPU_FORM_CONTEXT Context;
	PCPU_FUNCTION_ENTRY Function;
	PCPU_COUNTERS Thread;
	HWND hWndList;
	LONG Index;
	ULONG PcCount;
	FLOAT PcPercent;
	FLOAT ThreadPercent;

	//
	// Get currently selected Pc entry
	//

	hWndList = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_PC);
	Index = ListViewGetFirstSelected(hWndList);
	if (Index == -1) {
		return 0;
	}

	Function = NULL;
	ListViewGetParam(hWndList, Index, (LPARAM*)&Function);
	ASSERT(Function != NULL);
	if (!Function) {
		return 0;
	}

	WCHAR Buffer[64];
	ListView_GetItemText(hWndList, Index, 1, Buffer, 64);
	PcCount = (ULONG)_wtoi(Buffer);

	ListView_GetItemText(hWndList, Index, 2, Buffer, 64);
	PcPercent = (FLOAT)_wtof(Buffer);

	//
	// Get current selected thread id
	//

	hWndList = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_THREAD);
	Index = ListViewGetFirstSelected(hWndList);
	if (Index == -1) {
		ASSERT(0);
		return 0;
	}

	Thread = NULL;
	ListViewGetParam(hWndList, Index, (LPARAM*)&Thread);
	ASSERT(Thread != NULL);
	if (!Thread) {
		return 0;
	}

	ListView_GetItemText(hWndList, Index, 3, Buffer, 64);
	ThreadPercent = (FLOAT)_wtof(Buffer);

	//
	// Create PcStack dialog
	//

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_FORM_CONTEXT);
	ASSERT(Object->Context != NULL);

	CpuPcStackCreate(hWnd, 0, Context->Head, Function, 
					Thread->ThreadId, ThreadPercent,
					PcPercent, PcCount);
	return 0;
}