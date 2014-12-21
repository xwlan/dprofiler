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
	{ 40,  L"TID",      LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
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
	{ 80,  L"Inclusive",	 LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Inclusive %",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Exclusive",    LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Exclusive %",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
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
	}

	return Status;
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
	}

	return Status;
}

LRESULT 
CpuThreadOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
    PCPU_THREAD Thread;
    PCPU_THREAD_PC_TABLE PcTable;
    HWND hWndCtrl;
    LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	PBTR_TEXT_TABLE TextTable;
	PBTR_TEXT_FILE TextFile;
	PBTR_TEXT_ENTRY TextEntry;
	PCPU_THREAD_PC PcEntry;
	ULONG Exclusive;
    PPF_REPORT_HEAD Head;
    PCPU_FORM_CONTEXT Form;
	ULONG i;
    ULONG Number;
	PBTR_LINE_ENTRY Line;
	PBTR_LINE_ENTRY LineEntry;
	PBTR_DLL_FILE DllFile;
	PBTR_DLL_ENTRY DllEntry;
	double Percent;

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
        Thread = (PCPU_THREAD)lpNmlv->lParam;

        PcTable = Thread->Table;
        ASSERT(PcTable != NULL);

        //
        // Build symbol table to parse PC address 
        //

        TextFile = (PBTR_TEXT_FILE)((PUCHAR)Head + Head->Streams[STREAM_SYMBOL].Offset);
        TextTable = ApsBuildSymbolTable(TextFile, 4093);

        Exclusive = Thread->Exclusive;
        Number = 0;

		DllFile = (PBTR_DLL_FILE)ApsGetStreamPointer(Head, STREAM_DLL);
		DllEntry = &DllFile->Dll[0];

		Line = (PBTR_LINE_ENTRY)ApsGetStreamPointer(Head, STREAM_LINE);
		LineEntry = Line;

        for(i = 0; i < CPU_PC_BUCKET; i++) {

            PLIST_ENTRY ListEntry;
            PLIST_ENTRY ListHead;

            ListHead = &PcTable->ListHead[i];
            ListEntry = ListHead->Flink;

            while (ListEntry != ListHead) {

                PcEntry = CONTAINING_RECORD(ListEntry, CPU_THREAD_PC, ListEntry);

				Percent = (PcEntry->Inclusive * 1.0) / (Exclusive * 1.0);
				if (Percent < CPU_UI_INCLUSIVE_RATIO_THRESHOLD) {
	                ListEntry = ListEntry->Flink;
					continue;
				}

                //
                // Symbol name
                //

                lvi.iItem = Number;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_TEXT|LVIF_PARAM;
                lvi.lParam = (LPARAM)PcEntry;

                ASSERT(PcEntry->Pc != NULL);

                TextEntry = ApsLookupSymbol(TextTable, (ULONG64)PcEntry->Pc);
                if (TextEntry) {
                    StringCchPrintf(Buffer, MAX_PATH, L"%S", TextEntry->Text);
                } else {
                    ApsFormatAddress(Buffer, MAX_PATH, PcEntry->Pc, TRUE);
                }

                lvi.pszText = Buffer;
                ListView_InsertItem(hWndCtrl, &lvi);

                //
                // Inclusive
                //

                lvi.iSubItem = 1;
                lvi.mask = LVIF_TEXT;
                StringCchPrintf(Buffer, MAX_PATH, L"%u", PcEntry->Inclusive);
                lvi.pszText = Buffer;
                ListView_SetItem(hWndCtrl, &lvi);

               
                //
                // Inclusive %
                //

                lvi.iSubItem = 2;
                lvi.mask = LVIF_TEXT;
                StringCchPrintf(Buffer, MAX_PATH, L"%.2f", (PcEntry->Inclusive * 100.0) / (Exclusive * 1.0));
                lvi.pszText = Buffer;
                ListView_SetItem(hWndCtrl, &lvi);
                
                //
                // Exclusive
                //

                lvi.iSubItem = 3;
                lvi.mask = LVIF_TEXT;
                StringCchPrintf(Buffer, MAX_PATH, L"%u", PcEntry->Exclusive);
                lvi.pszText = Buffer;
                ListView_SetItem(hWndCtrl, &lvi);

                //
                // Exclusive %
                //

                lvi.iSubItem = 4;
                lvi.mask = LVIF_TEXT;
                StringCchPrintf(Buffer, MAX_PATH, L"%.2f", (PcEntry->Exclusive * 100.0) / (Exclusive * 1.0));
                lvi.pszText = Buffer;
                ListView_SetItem(hWndCtrl, &lvi);

                //
                // Dll 
                //

                lvi.iSubItem = 5;
                lvi.mask = LVIF_TEXT;
				
				ApsGetDllBaseNameById(Head, PcEntry->DllId, Buffer, MAX_PATH);
                lvi.pszText = Buffer;
                ListView_SetItem(hWndCtrl, &lvi);

				//
				// Line
				//

				if (PcEntry->LineId != -1) {

					ASSERT(Line != NULL);

					LineEntry = Line + PcEntry->LineId;
					StringCchPrintf(Buffer, MAX_PATH, L"%S:%u", LineEntry->File, LineEntry->Line);

					lvi.iSubItem = 6;
					lvi.mask = LVIF_TEXT;
					lvi.pszText = Buffer; 
					ListView_SetItem(hWndCtrl, &lvi);

				} else {

					lvi.iSubItem = 6;
					lvi.mask = LVIF_TEXT;
					lvi.pszText = L""; 
					ListView_SetItem(hWndCtrl, &lvi);
				}

                //
                // Move to next PC
                //

                Number += 1;
                ListEntry = ListEntry->Flink;
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
    PCPU_THREAD Thread1, Thread2;
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

	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Thread1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Thread2);

	if (ListView->LastClickedColumn == 0) {
        Result = Thread1->ThreadId - Thread2->ThreadId;
	}
	
	if (ListView->LastClickedColumn == 1 || ListView->LastClickedColumn == 2) {
        Result = (Thread1->KernelTime + Thread1->UserTime) - (Thread2->KernelTime + Thread2->UserTime);
	}
	
	if (ListView->LastClickedColumn == 3 || ListView->LastClickedColumn == 4) {
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
	PCPU_THREAD_PC Pc1, Pc2;
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

	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Pc1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Pc2);

	if (ListView->LastClickedColumn == 0 || 
		ListView->LastClickedColumn == 5 ||
		ListView->LastClickedColumn == 6) {

        ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	    ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);
		Result = wcsicmp(FirstData, SecondData);

	}
	
    if (ListView->LastClickedColumn == 1 || ListView->LastClickedColumn == 2) {
        Result = Pc1->Inclusive - Pc2->Inclusive;
		if (Result == 0) {
			Result = Pc1->Depth - Pc2->Depth;
		}
	}
	
    if (ListView->LastClickedColumn == 3 || ListView->LastClickedColumn == 4) {
        Result = Pc1->Exclusive - Pc2->Exclusive;
	}
	
	return ListView->SortOrder ? Result : -Result;
}

PCPU_THREAD_TABLE
CpuThreadGetTable(
    __in HWND hWnd
    )
{  
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_FORM_CONTEXT)Object->Context;
    return (PCPU_THREAD_TABLE)Context->Context;
}

PCPU_THREAD
CpuThreadGetMostBusyThread(
    __in HWND hWnd
    )
{
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;
    PCPU_THREAD_TABLE Table;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_FORM_CONTEXT)Object->Context;
    Table = (PCPU_THREAD_TABLE)Context->Context;

    ASSERT(Table != NULL);


}

VOID
CpuThreadInsertThreads(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
    )
{
    PCPU_THREAD_TABLE ThreadTable;
    PDIALOG_OBJECT Object;
    PCPU_FORM_CONTEXT Context;
    ULONG Number;
    HWND hWndCtrl;
    ULONG i;
    PCPU_THREAD Thread;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
    double Milliseconds;
    double TotalTimes;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCPU_FORM_CONTEXT)Object->Context;

    //
    // N.B. Must track head pointer herer
    //

    Context->Head = Head;

    //
    // Scan the CPU sample records to generate threaded PC stream
    //

    ThreadTable = CpuCreateThreadTable();
    CpuScanThreadedPc(Head, ThreadTable);

    TotalTimes = ApsNanoUnitToMilliseconds(
		(ULONG)(ThreadTable->KernelTime + ThreadTable->UserTime));

	//
	// Ensure not divide by zero
	//

	TotalTimes = max(0.0001, TotalTimes);

    //
    // Save the thread table in CPU_FORM_CONTEXT
    //

    Context->Context = ThreadTable;

    //
    // Fill the threads into listview
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CPU_THREAD_THREAD);
    ASSERT(hWndCtrl != NULL); 

    Number = 0;

    //
    // Walk the hash table to insert all threads, each listview item
    // attached thread object as LPARAM 
    //

    for(i = 0; i < CPU_THREAD_BUCKET; i++) {

        ListHead = &ThreadTable->ListHead[i];
        ListEntry = ListHead->Flink;

        while (ListHead != ListEntry) {

            Thread = CONTAINING_RECORD(ListEntry, CPU_THREAD, ListEntry);

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
            // Time (ms)
            //

            lvi.iSubItem = 1;
            lvi.mask = LVIF_TEXT;

            Milliseconds = ApsNanoUnitToMilliseconds(Thread->KernelTime + Thread->UserTime);
            StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Milliseconds);

            lvi.pszText = Buffer;
            ListView_SetItem(hWndCtrl, &lvi);
            
            //
            // Time (%)
            //

            lvi.iSubItem = 2;
            lvi.mask = LVIF_TEXT;
            StringCchPrintf(Buffer, MAX_PATH, L"%.3f", (Milliseconds * 100.0) / TotalTimes);
            lvi.pszText = Buffer;
            ListView_SetItem(hWndCtrl, &lvi);

            //
            // Priority 
            //

            lvi.iSubItem = 3;
            lvi.mask = LVIF_TEXT;

            CpuTranslateThreadPriority(Buffer, MAX_PATH, Thread->Priority);
            lvi.pszText = Buffer;
            ListView_SetItem(hWndCtrl, &lvi);

            //
            // State 
            //

            lvi.iSubItem = 4;
            lvi.mask = LVIF_TEXT;

            if (Thread->State == CPU_THREAD_STATE_RUNNING) {
                lvi.pszText = L"Live";
            } 
            else if (Thread->State == CPU_THREAD_STATE_RETIRED) {
                lvi.pszText = L"Retired";
            }
            else {
                ASSERT(0);
            }

            ListView_SetItem(hWndCtrl, &lvi);

            //
            // Move to next item share same bucket
            //

            Number += 1;
            ListEntry = ListEntry->Flink;
        }
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
        StringCchCopy(Buffer, Length, L"Unknown");
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
