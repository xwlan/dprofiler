//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "perf.h"
#include "perfdlg.h"
#include "graphctrl.h"
#include "bsp.h"
#include <math.h>

typedef enum _PerfColumnType {
	PerfColumnCounter,
	PerfColumnValue,
	PerfColumnAverage,
	PerfColumnMaximum,
	PERF_COLUMN_NUMBER,
} PerfColumnType;

typedef enum _PerfCounterType {
	PerfCounterCpuTotal,
	PerfCounterPrivateBytes,
	PerfCounterWorkingSet,
	PerfCounterKernelHandles,
	PerfCounterUserHandles,
	PerfCounterGdiHandles,
	PerfCounterIoTotal,
	PERF_COUNTER_NUMBER,
} PerfCounterType;

LISTVIEW_COLUMN PerfColumn[PERF_COLUMN_NUMBER] = {
	{ 100,  L"Counter",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,  L"Current",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,  L"Average (60s)",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,  L"Maximum",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

INT_PTR
PerfDialog(
	IN HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	PERF_DIALOG_CONTEXT Context = {0};
	INT_PTR Return;
	
	InitializeListHead(&Context.TaskContextList);

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_PERFORMANCE;
	Object.Procedure = PerfProcedure;
	Object.Scaler = NULL;

	Return = DialogCreate(&Object);
	return Return;
}

INT_PTR CALLBACK
PerfProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_INITDIALOG:
			PerfOnInitDialog(hWnd, uMsg, wp, lp);
			Status = TRUE;

		case WM_COMMAND:
			PerfOnCommand(hWnd, uMsg, wp, lp);
			break;
		
		case WM_NOTIFY:
			PerfOnNotify(hWnd, uMsg, wp, lp);
			break;

		case WM_TIMER:
			PerfOnTimer(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
PerfOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	RECT Rect;
	HWND hWndCtrl;
	PGRAPH_CONTROL Graph;
	PDIALOG_OBJECT Object;
	HWND hWndDesktop;
	PPERF_DIALOG_CONTEXT Context;
	LVCOLUMN Column = {0};
	WCHAR Buffer[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, PERF_DIALOG_CONTEXT);

	hWndDesktop = GetDesktopWindow();

	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_CPU);
	GetWindowRect(hWndCtrl, &Rect);
	MapWindowRect(hWndDesktop, hWnd, &Rect);

	//
	// CPU graph
	//

	Graph = GraphCreateControl(hWnd, IDC_STATIC_CPU, &Rect,
		                       RGB(212, 208, 200), RGB(130, 130, 130), 
							   RGB(255, 128, 128), RGB(255, 0, 0));

	Graph->HistoryCallback = PerfGraphHistoryCallback;
	Graph->HistoryContext = Object;
	Context->Graph[PerfGraphCpu] = Graph;

	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_MEMORY);
	GetWindowRect(hWndCtrl, &Rect);
	MapWindowRect(hWndDesktop, hWnd, &Rect);

	//
	// Memory Graph
	//

	Graph = GraphCreateControl(hWnd, IDC_STATIC_MEMORY, &Rect,
                		       RGB(212, 208, 200), RGB(130, 130, 130), 
							   RGB(158, 202, 158), RGB(60, 148, 60));

	Graph->HistoryCallback = PerfGraphHistoryCallback;
	Graph->HistoryContext = Object;
	Context->Graph[PerfGraphMemory] = Graph;

	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_IO);
	GetWindowRect(hWndCtrl, &Rect);
	MapWindowRect(hWndDesktop, hWnd, &Rect);

	//
	// I/O Graph
	//

	Graph = GraphCreateControl(hWnd, IDC_STATIC_IO, &Rect,
		                	   RGB(212, 208, 200), RGB(130, 130, 130), 
							   RGB(144, 158, 228), RGB(30, 60, 200));
							   
	Graph->HistoryCallback = PerfGraphHistoryCallback;
	Graph->HistoryContext = Object;
	Context->Graph[PerfGraphIo] = Graph;

	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_CAPTION);
	SetWindowText(hWndCtrl, L"Performance Counter Report:");

	PerfInsertTaskList(Object, hWnd);
	PerfInsertCounterList(Object, hWnd);
	PerfRegisterCallback(PerfGraphConsumerCallback, (PVOID)Object);

	//
	// Set default task pane
	//

	LoadString(SdkInstance, IDS_PERFMON, Buffer, MAX_PATH);
	SetWindowText(hWnd, Buffer);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_PERF_PROCESS);
	SetFocus(hWndCtrl);
	ListView_SetItemState(hWndCtrl, 0, LVIS_SELECTED, LVIS_SELECTED);

	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);

	PerfStartWorkItem();
	return 0;
}

LRESULT
PerfOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCounters;
	HWND hWndObjects;
	PGRAPH_CONTROL Graph;
	PDIALOG_OBJECT Object;
	PPERF_DIALOG_CONTEXT Context;
	int i;
	int Count;

	PerfRemoveTerminatedTasks(hWnd);
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, PERF_DIALOG_CONTEXT);

	hWndObjects = GetDlgItem(hWnd, IDC_LIST_PERF_PROCESS);
	Count = ListView_GetItemCount(hWndObjects); 

	if (!Count) {
		for(i = 0; i < PERF_GRAPH_NUMBER; i++) {
			Graph = Context->Graph[i];
			GraphInvalidate(Graph->hWnd);
		}
		return 0L;
	}

	PerfUpdateRangeTitle(hWnd);

	hWndCounters = GetDlgItem(hWnd, IDC_LIST_PERF_COUNTERS);
	PerfUpdateCountersReport(hWndCounters, Context->Current);

	for(i = 0; i < PERF_GRAPH_NUMBER; i++) {
		Graph = Context->Graph[i];
		GraphOnTimer(Graph->hWnd, WM_TIMER, 0, 0);
	}

	return 0;
}

VOID
PerfUpdateRangeTitle(
	IN HWND hWnd
	)
{
	PPERF_TASK_CONTEXT Task;
	PGRAPH_HISTORY History;
	HWND hWndTitle;
	LONG Limits;
	WCHAR Buffer[MAX_PATH];
	PPERF_DIALOG_CONTEXT Context;
	PDIALOG_OBJECT Object;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, PERF_DIALOG_CONTEXT);

	Task = Context->Current;

	History = &Task->MemoryHistory;
	if (History->Limits > History->PreviousLimits) {

		hWndTitle = GetDlgItem(hWnd, IDC_STATIC_MEMORY_LIMITS);
		Limits = (LONG)(History->Limits / (1024 * 1024));
		StringCchPrintf(Buffer, MAX_PATH - 1, L"%u MB", Limits);
		SetWindowText(hWndTitle, Buffer); 

		History->PreviousLimits = History->Limits;
	}

	History = &Task->IoHistory;
	if (History->Limits > History->PreviousLimits) {

		hWndTitle = GetDlgItem(hWnd, IDC_STATIC_IO_LIMITS);
		Limits = (LONG)(History->Limits / (1024 * 1024));
		StringCchPrintf(Buffer, MAX_PATH - 1, L"%u MB/Sec", Limits);
		SetWindowText(hWndTitle, Buffer); 

		History->PreviousLimits = History->Limits;
	}
}

VOID
PerfRemoveTerminatedTasks(
	IN HWND hWnd
	)
{
	HWND hWndList;
	PPERF_TASK_CONTEXT Task;
	int Count;
	int i;
	BOOLEAN Changed = FALSE;
	PPERF_DIALOG_CONTEXT Context;
	PDIALOG_OBJECT Object;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, PERF_DIALOG_CONTEXT);
	
	hWndList = GetDlgItem(hWnd, IDC_LIST_PERF_PROCESS);
	Count = ListView_GetItemCount(hWndList);

	for(i = Count - 1; i != -1; i--) {
		ListViewGetParam(hWndList, i, (LPARAM *)&Task);
		if (Task && Task->Information.Terminated) {
			ListView_DeleteItem(hWndList, i);
			RemoveEntryList(&Task->ListEntry);
			BspFree(Task);
			Changed = TRUE;
		}
	}

	//
	// N.B. If there's no any removed task, we don't switch
	// selected item.
	//

	if (ListView_GetItemCount(hWndList) != 0 && Changed) {
		ListViewSelectSingle(hWndList, 0);
	}
}

LRESULT
PerfOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDOK:
	case IDCANCEL:
		PerfOnOk(hWnd, uMsg, wp, lp);
	    break;

	}

	return 0;
}

LRESULT
PerfOnSelChanged(
	IN HWND hWnd,
	IN HWND hWndList,
	IN LONG Current
	)
{
	PDIALOG_OBJECT Object;
	PPERF_DIALOG_CONTEXT Context;
	PPERF_TASK_CONTEXT TaskContext;
	PGRAPH_CONTROL Graph;
	HWND hWndCounters;
	HWND hWndTitle;
	LONG Limits;
	PGRAPH_HISTORY History;
	WCHAR Buffer[MAX_PATH];
	WCHAR Title[MAX_PATH];
	int i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PPERF_DIALOG_CONTEXT)Object->Context;

	ListViewGetParam(hWndList, Current, (LPARAM *)&TaskContext);
	ASSERT(TaskContext != NULL);

	Context->Current = TaskContext;
	hWndCounters = GetDlgItem(hWnd, IDC_LIST_PERF_COUNTERS);

	History = &TaskContext->MemoryHistory;
	hWndTitle = GetDlgItem(hWnd, IDC_STATIC_MEMORY_LIMITS);
	Limits = (LONG)(History->Limits / (1024 * 1024));
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u MB", Limits);
	SetWindowText(hWndTitle, Buffer); 

	History = &TaskContext->IoHistory;
	hWndTitle = GetDlgItem(hWnd, IDC_STATIC_IO_LIMITS);
	Limits = (LONG)(History->Limits / (1024 * 1024));
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u MB/Sec", Limits);
	SetWindowText(hWndTitle, Buffer); 

	PerfUpdateCountersReport(hWndCounters, TaskContext);

	for(i = 0; i < PERF_GRAPH_NUMBER; i++) {
		Graph = Context->Graph[i];
		GraphInvalidate(Graph->hWnd);
	}

	LoadString(SdkInstance, IDS_PERFMON, Title, MAX_PATH);
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%ws - %ws (%u)",
					Title, TaskContext->Information.Name,
		            TaskContext->Information.ProcessId);
	SetWindowText(hWnd, Buffer);
	return 0L;
}

LRESULT
PerfOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	NMHDR *lpnmhdr = (NMHDR *)lp;
	NMLISTVIEW *lpnmlv = (LPNMLISTVIEW)lpnmhdr;
	HWND hWndList;

	switch(LOWORD(wp)) {

	case IDC_LIST_PERF_PROCESS:
		if (lpnmhdr->code == LVN_ITEMCHANGED) {

			if ((lpnmlv->uChanged & LVIF_STATE) && (lpnmlv->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))) {
				hWndList = GetDlgItem(hWnd, IDC_LIST_PERF_PROCESS);
				PerfOnSelChanged(hWnd, hWndList, lpnmlv->iItem);
			}
		}
		break;

	case IDC_LIST_PERF_COUNTERS:
		if (lpnmhdr->code == NM_CUSTOMDRAW) {
			PerfHighlightCounters(hWnd, lpnmhdr);
		}
		break;
	}

	return 0L;
}

LRESULT
PerfHighlightCounters(
	IN HWND hWnd,
	IN NMHDR *lpnmhdr
	)
{
	return 0L;
}

LRESULT
PerfOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);

	PerfStopWorkItem();
	PerfCleanUp(Object);

	EndDialog(hWnd, IDOK);
	return 0;
}

VOID
PerfCleanUp(
	IN PDIALOG_OBJECT Object	
	)
{
	PPERF_TASK_CONTEXT TaskContext;
	PPERF_DIALOG_CONTEXT ObjectContext;
	PLIST_ENTRY ListEntry;

	//
	// N.B. Perfmon callback must be unregistered before any
	// clean up work
	//

	PerfUnregisterCallback(PerfGraphConsumerCallback);
	ObjectContext = (PPERF_DIALOG_CONTEXT)Object->Context;	
	while (IsListEmpty(&ObjectContext->TaskContextList) != TRUE) {
		ListEntry = RemoveHeadList(&ObjectContext->TaskContextList);
		TaskContext = CONTAINING_RECORD(ListEntry, PERF_TASK_CONTEXT, ListEntry);
		BspFree(TaskContext);	
	}
}

ULONG
PerfInsertTaskList(
	IN PDIALOG_OBJECT Object,
	IN HWND hWnd
	)
{
	HWND hWndList;
	RECT Rect;
	LVCOLUMN Column = {0};
	LVITEM Item = {0};
	PPERF_INFORMATION Information;
	PLIST_ENTRY ListEntry;
	WCHAR Buffer[MAX_PATH];
	ULONG Number = 0;
	PPERF_TASK_CONTEXT TaskContext;
	PPERF_DIALOG_CONTEXT PerfContext;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	PerfContext = SdkGetContext(Object, PERF_DIALOG_CONTEXT);

	hWndList = GetDlgItem(hWnd, IDC_LIST_PERF_PROCESS);
	GetClientRect(hWndList, &Rect);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
    Column.iSubItem = 0;
	Column.pszText = L"Process";	
	Column.cx = Rect.right - Rect.left;     

	ListView_InsertColumn(hWndList, 0, &Column);

	ListEntry = PerfListHead.Flink;
	while (ListEntry != &PerfListHead) {

		Information = CONTAINING_RECORD(ListEntry, PERF_INFORMATION, ListEntry);
		TaskContext = PerfCreateTaskContext(Information);

		Item.mask = LVIF_TEXT | LVIF_PARAM;
		Item.iItem = Number;
		Item.iSubItem = 0;

		StringCchPrintf(Buffer, MAX_PATH - 1, L"%ws", 
			            Information->Name, Information->ProcessId);

		Item.pszText = Buffer;
		Item.lParam = (LPARAM)TaskContext;
		ListView_InsertItem(hWndList, &Item);
		
		InsertTailList(&PerfContext->TaskContextList, &TaskContext->ListEntry);
		ListEntry = ListEntry->Flink;
	}

	return S_OK;
}

PPERF_TASK_CONTEXT
PerfCreateTaskContext(
	IN PPERF_INFORMATION Information
	)
{
	PPERF_TASK_CONTEXT TaskContext;

	TaskContext = (PPERF_TASK_CONTEXT)BspMalloc(sizeof(PERF_TASK_CONTEXT));
	RtlZeroMemory(TaskContext, sizeof(PERF_TASK_CONTEXT));

	//
	// We hold a copy of PERF_INFORMATION to avoid lock race with PerfWorkItem,
	// and we never use its ListEntry field, everytime, our callback is invoked,
	// we copy the structure to update.
	//

	RtlCopyMemory(&TaskContext->Information, Information, sizeof(PERF_INFORMATION));

	//
	// CPU has no increment, because its value frequently sparks to 100%,
	// not stable.
	//

	TaskContext->CpuHistory.Color = RGB(0x08, 0xE2, 0x10);
	TaskContext->CpuHistory.Limits = 100;
	TaskContext->CpuHistory.PreviousLimits = 100;
	TaskContext->CpuHistory.Increment = 0;

	//
	// Memory increase at 10MB/Sec, init 100M limits
	//

	TaskContext->MemoryHistory.Color = RGB(0x08, 0xE2, 0x10);
	TaskContext->MemoryHistory.Limits = 1024 * 1024 * 100;
	TaskContext->MemoryHistory.PreviousLimits = 1024 * 1024 * 100;
	TaskContext->MemoryHistory.Increment = 1024 * 1024 * 10;

	//
	// I/O increase at 1MB/Sec, init 1M/Sec limits
	//

	TaskContext->IoHistory.Color = RGB(0x08, 0xE2, 0x10);
	TaskContext->IoHistory.Limits = 1024 * 1024;
	TaskContext->IoHistory.PreviousLimits = 1024 * 1024;
	TaskContext->IoHistory.Increment = 1024 * 1024;
	
	return TaskContext;
}

ULONG
PerfInsertCounterList(
	IN PDIALOG_OBJECT Object,
	IN HWND hWnd
	)
{
	HWND hWndCounters;
	LVCOLUMN Column = {0};
	LVITEM item = {0};
	int i;

	UNREFERENCED_PARAMETER(Object);

	hWndCounters = GetDlgItem(hWnd, IDC_LIST_PERF_COUNTERS);
	ListView_SetUnicodeFormat(hWndCounters, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndCounters, 
		                                LVS_EX_FULLROWSELECT,  
										LVS_EX_FULLROWSELECT);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

	for(i = 0; i < PERF_COLUMN_NUMBER; i++) {

	    Column.iSubItem = i;
		Column.pszText = PerfColumn[i].Title;	
		Column.cx = PerfColumn[i].Width;     
		Column.fmt = PerfColumn[i].Align;     

		ListView_InsertColumn(hWndCounters, i, &Column);
	}

	//
	// PerfCounterCpuTotal
	//

	item.mask = LVIF_TEXT;
	item.iItem = PerfCounterCpuTotal;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"CPU Usage";
	ListView_InsertItem(hWndCounters, &item);

	//
	// PerfCounterPrivateBytes
	//

	item.iItem = PerfCounterPrivateBytes;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"Private Bytes";
	ListView_InsertItem(hWndCounters, &item);

	//
	// PerfCounterWorkingSet
	//

	item.iItem = PerfCounterWorkingSet;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"Working Set";
	ListView_InsertItem(hWndCounters, &item);

	//
	// PerfCounterKernelHandles
	//

	item.iItem = PerfCounterKernelHandles;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"Kernel Handles";
	ListView_InsertItem(hWndCounters, &item);

	//
	// PerfCounterUserHandles
	//

	item.iItem = PerfCounterUserHandles;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"User Handles";
	ListView_InsertItem(hWndCounters, &item);

	//
	// PerfCounterGdiHandles
	//

	item.iItem = PerfCounterGdiHandles;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"Gdi Handles";
	ListView_InsertItem(hWndCounters, &item);

	//
	// PerfCounterIoTotal
	//

	item.iItem = PerfCounterIoTotal;
	item.iSubItem = PerfColumnCounter;
	item.pszText = L"I/O Total";
	ListView_InsertItem(hWndCounters, &item);

	return S_OK;
}

VOID
PerfUpdateCountersReport(
	IN HWND hWndCounters,
	IN PPERF_TASK_CONTEXT TaskContext 
	)
{
	PPERF_INFORMATION Perf;
	LVITEM item = {0};
	WCHAR Buffer[MAX_PATH];
	double IoTotal;
	double Value;

	Perf = &TaskContext->Information;

	//
	// PerfCounterCpuTotal
	//

	item.mask = LVIF_TEXT;
	item.iItem = PerfCounterCpuTotal;

	item.iSubItem = PerfColumnValue;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f", Perf->CpuTotalUsage * 100); 
	StringCchCat(Buffer, MAX_PATH - 1, L"%");
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f", TaskContext->CpuHistory.Average); 
	StringCchCat(Buffer, MAX_PATH - 1, L"%");
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f", TaskContext->CpuHistory.MaximumValue);
	StringCchCat(Buffer, MAX_PATH - 1, L"%");
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	//
	// PerfCounterPrivateBytes
	//

	item.iItem = PerfCounterPrivateBytes;

	item.iSubItem = PerfColumnValue;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB", 
		           (Perf->PrivateDelta.Value * 1.0) / (1024 * 1024 * 1.0)); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB", 
		            (TaskContext->MemoryHistory.Average * 1.0) / (1024 * 1024 * 1.0)); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB", 
		            (TaskContext->MemoryHistory.MaximumValue * 1.0) / (1024 * 1024 * 1.0)); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	//
	// PerfCounterWorkingSet
	//

	item.iItem = PerfCounterWorkingSet;

	item.iSubItem = PerfColumnValue;
	Value = (Perf->WorkingSetDelta.Value * 1.0) / ((1024 * 1024) * 1.0);
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB", Value); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	item.pszText = L"N/A";
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	Value = (Perf->WorkingSetPeak * 1.0) / ((1024 * 1024) * 1.0);
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB", Value); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	//
	// PerfCounterKernelHandles
	//

	item.iItem = PerfCounterKernelHandles;

	item.iSubItem = PerfColumnValue;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u", Perf->KernelHandles); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	item.pszText = L"N/A";
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u", Perf->KernelHandlesPeak); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	//
	// PerfCounterUserHandles
	//

	item.iItem = PerfCounterUserHandles;

	item.iSubItem = PerfColumnValue;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u", Perf->UserHandles); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	item.pszText = L"N/A";
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u", Perf->UserHandlesPeak); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	//
	// PerfCounterGdiHandles
	//

	item.iItem = PerfCounterGdiHandles;

	item.iSubItem = PerfColumnValue;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u", Perf->GdiHandles); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	item.pszText = L"N/A";
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%u", Perf->GdiHandlesPeak); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	//
	// PerfCounterIoTotal
	//

	item.iItem = PerfCounterIoTotal;

	item.iSubItem = PerfColumnValue;
	IoTotal = (FLOAT)(Perf->IoReadDelta.Delta + Perf->IoWriteDelta.Delta + Perf->IoOtherDelta.Delta) / (1024 * 1024);
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB/Sec", IoTotal); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnAverage;
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB/Sec", TaskContext->IoHistory.Average / (1024 * 1024)); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

	item.iSubItem = PerfColumnMaximum;
	IoTotal = (FLOAT)TaskContext->IoHistory.MaximumValue / (1024 * 1024);
	StringCchPrintf(Buffer, MAX_PATH - 1, L"%.2f MB/Sec", IoTotal); 
	item.pszText = Buffer;
	ListView_SetItem(hWndCounters, &item);

}

ULONG CALLBACK
PerfGraphConsumerCallback(
	IN PPERF_INFORMATION Information,
	IN PVOID Context
	)
{
	PDIALOG_OBJECT Dialog;
	PPERF_DIALOG_CONTEXT DialogContext;
	PLIST_ENTRY ListEntry;
	PPERF_TASK_CONTEXT Task;

	Dialog = (PDIALOG_OBJECT)Context;
	DialogContext = SdkGetContext(Dialog, PERF_DIALOG_CONTEXT);

	ListEntry = DialogContext->TaskContextList.Flink;
	while (ListEntry != &DialogContext->TaskContextList) {

		Task = CONTAINING_RECORD(ListEntry, PERF_TASK_CONTEXT, ListEntry);

		if (Task->Information.ProcessId == Information->ProcessId) {

			if (Information->Terminated) {
				Task->Information.Terminated = TRUE;
			}
			else {
				RtlCopyMemory(&Task->Information, Information, sizeof(PERF_INFORMATION));
				PerfUpdateGraphHistory(Task);
			}
		}

		if (Task == DialogContext->Current) {
			PostMessage(Dialog->hWnd, WM_TIMER, 0, 0);
		}

		ListEntry = ListEntry->Flink;
	}

	return S_OK;
}

VOID
PerfComputeAverage(
	IN PPERF_TASK_CONTEXT Context
	)
{
	PGRAPH_HISTORY History;
	FLOAT Data;
	int i;
	int Count;
	
	//
	// CPU Average
	//

	History = &Context->CpuHistory;	
	if (History->Full != TRUE) {
		Count = History->LastValue + 1;
	} else {
		Count = 61;
	}

	Data = 0;
	for(i = 0; i < Count; i++) {
		Data += History->Value[i];
	}

	Data = Data / Count;
	History->Average = Data;

	//
	// Memory Average
	//

	History = &Context->MemoryHistory;	
	
	Data = 0;
	for(i = 0; i < Count; i++) {
		Data += History->Value[i];
	}

	Data = Data / Count;
	History->Average = Data;

	//
	// I/O Average
	//

	History = &Context->IoHistory;	
	
	Data = 0;
	for(i = 0; i < Count; i++) {
		Data += History->Value[i];
	}

	Data = Data / Count;
	History->Average = Data;
}

VOID
PerfUpdateGraphHistory(
	IN PPERF_TASK_CONTEXT Context
	)	
{
	PPERF_INFORMATION Perf;
	PGRAPH_HISTORY History;
	LONG LastValue;
	FLOAT Data;

	Perf = &Context->Information;
	History = &Context->CpuHistory;
	Data = Perf->CpuTotalUsage * 100;

	//
	// Update CpuTotalUsage history
	//

	if (History->Full != TRUE) {

		LastValue = History->LastValue + 1;

		if (LastValue == 61) {
			LastValue = 0;
			History->Full = TRUE;
		}

		History->Value[LastValue] = Data;
		History->LastValue = LastValue;

	} else {

		LastValue = History->LastValue + 1;
		LastValue %= 61;
		History->Value[LastValue] = Data;
		History->LastValue = LastValue;
	}

	if (Data > History->MaximumValue) {
		History->MaximumValue = Data;
	} 

	//
	// Update PrivateBytes history
	//

	History = &Context->MemoryHistory;
	Data = Perf->PrivateDelta.Value * ((FLOAT)1.0);

	if (History->Full != TRUE) {

		LastValue = History->LastValue + 1;

		if (LastValue == 61) {
			LastValue = 0;
			History->Full = TRUE;
		}

		History->Value[LastValue] = Data;
		History->LastValue = LastValue;

	} else {

		LastValue = History->LastValue + 1;
		LastValue %= 61;
		History->Value[LastValue] = Data;
		History->LastValue = LastValue;
	}

	if (Data > History->MaximumValue) {
		History->MaximumValue = Data;
	} 

	if (History->MaximumValue > History->Limits) {
		History->PreviousLimits = History->Limits;
	}

	while (History->MaximumValue > History->Limits) {
		History->Limits += History->Increment;
	}

	//
	// Update I/O total history
	//

	History = &Context->IoHistory;
	Data = (Perf->IoReadDelta.Delta + Perf->IoWriteDelta.Delta + Perf->IoOtherDelta.Delta) * ((FLOAT)1.0);

	if (History->Full != TRUE) {

		LastValue = History->LastValue + 1;

		if (LastValue == 61) {
			LastValue = 0;
			History->Full = TRUE;
		}

		History->Value[LastValue] = Data;
		History->LastValue = LastValue;

	} else {

		LastValue = History->LastValue + 1;
		LastValue %= 61;
		History->Value[LastValue] = Data;
		History->LastValue = LastValue;
	}
	
	if (Data > History->MaximumValue) {
		History->MaximumValue = Data;
	} 
	
	if (History->MaximumValue > History->Limits) {
		History->PreviousLimits = History->Limits;
	}

	while (History->MaximumValue > History->Limits) {
		History->Limits += History->Increment;
	}
	
	//
	// Computer average of last 60 values
	//

	PerfComputeAverage(Context);
}

ULONG CALLBACK
PerfGraphHistoryCallback(
	IN PGRAPH_CONTROL Object,
	OUT PGRAPH_HISTORY *Primary,
	OUT PGRAPH_HISTORY *Secondary,
	IN PVOID Context
	)
{
	HWND hWndList;
	PPERF_TASK_CONTEXT Task;
	PPERF_DIALOG_CONTEXT DialogContext;
	PDIALOG_OBJECT Dialog;
	LONG Selected;

	Dialog = (PDIALOG_OBJECT)Context;
	DialogContext = (PPERF_DIALOG_CONTEXT)Dialog->Context;
	hWndList = GetDlgItem(Dialog->hWnd, IDC_LIST_PERF_PROCESS);
	
	*Primary = NULL;
	*Secondary = NULL;


	//
	// N.B. If Selected is -1, this may be a race condition with 
	// PerfOnInitDialog, we ignore here, just return.
	//

	Selected = ListViewGetFirstSelected(hWndList);
	if (Selected == -1) {
		return 0;
	}

	ListViewGetParam(hWndList, Selected, (LPARAM *)&Task);

	ASSERT(Task != NULL);
	ASSERT(Task == DialogContext->Current); 

	if (Object->CtrlId == IDC_STATIC_CPU) {
		*Primary = &Task->CpuHistory;
	}
	
	if (Object->CtrlId == IDC_STATIC_MEMORY) {
		*Primary = &Task->MemoryHistory;
	}
	
	//
	// N.B. PerfGraphIo has 2 history, the secondary is
	// Btr I/O history, reserved for next release.
	//

	if (Object->CtrlId == IDC_STATIC_IO) {
		*Primary = &Task->IoHistory;
	}

	return S_OK;
}