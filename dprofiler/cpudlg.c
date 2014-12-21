//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#include "cpudlg.h"
#include "sdk.h"
#include "dialog.h"
#include "listview.h"
#include "split.h"

typedef struct _CPU_ATTACH_CONTEXT {
	PBSP_PROCESS Process;
	ULONG Period;
} CPU_ATTACH_CONTEXT, *PCPU_ATTACH_CONTEXT;

LISTVIEW_COLUMN CpuAttachColumn[3] = {
	{ 120, L"Name", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"PID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeUInt },
	{ 360, L"Path", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

//
// N.B. Default we use 1sec as sampling period
//

typedef enum _CPU_SAMPLING_PERIOD {
	Sampling500ms,
	Sampling1Sec,
	Sampling2Sec,
	Sampling3Sec,
	Sampling4Sec,
	Sampling5Sec,
	SamplingCount,
} CPU_SAMPLING_PERIOD;

PWSTR SamplingPeriodStr[SamplingCount] = {
	L"500 ms",
	L" 1 sec",
	L" 2 sec",
	L" 3 sec",
	L" 4 sec",
	L" 5 sec",
};

INT_PTR
CpuAttachDialog(
	IN HWND hWndParent,
	OUT PBSP_PROCESS *Process,
	OUT PULONG Period
	)
{
	DIALOG_OBJECT Object = {0};
	CPU_ATTACH_CONTEXT Context = {0};
	INT_PTR Return;

	*Process = NULL;
	*Period = 1000;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_CPU_ATTACH;
	Object.Procedure = CpuAttachProcedure;
	
	Return = DialogCreate(&Object);

	if (Return == IDOK) {
		*Process = Context.Process;
		*Period = Context.Period;
	}
	return Return;
}

INT_PTR CALLBACK
CpuAttachProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return CpuAttachOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
				return CpuAttachOnOk(hWnd, uMsg, wp, lp);
			}

			if (LOWORD(wp) == IDC_BUTTON_REFRESH) {
				return CpuAttachOnRefresh(hWnd, uMsg, wp, lp);
			}
	}
	
	return Status;
}

LRESULT
CpuAttachOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PCPU_ATTACH_CONTEXT Context;
	LVCOLUMN lvc = {0};
	int i;
	LIST_ENTRY ListHead;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCPU_ATTACH_CONTEXT)Object->Context;
	
	//
	// Insert sampling period strings
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_PERIOD);
	SdkModifyStyle(hWndCtrl, 0, CBS_DROPDOWNLIST, FALSE);

    SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)SamplingPeriodStr[0]);
    SendMessage(hWndCtrl, CB_SETITEMDATA, 0, (LPARAM)500);

	for(i = 1; i < SamplingCount; i++) {
	    SendMessage(hWndCtrl, CB_ADDSTRING, i, (LPARAM)SamplingPeriodStr[i]);
	    SendMessage(hWndCtrl, CB_SETITEMDATA, i, (LPARAM)i * 1000);
	}

	//
	// Default use 1 sec sampling period
	//

    SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)Sampling1Sec, 0);

	//
	// Insert task tree
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_TASK);
	ListView_SetUnicodeFormat(hWndCtrl, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < 3; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = CpuAttachColumn[i].Title;	
		lvc.cx = CpuAttachColumn[i].Width;     
		lvc.fmt = CpuAttachColumn[i].Align;

		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	SdkModifyStyle(hWndCtrl, 0, LVS_SORTASCENDING, FALSE);

	InitializeListHead(&ListHead);
	BspQueryProcessList(&ListHead);
	CpuAttachInsertTask(Object, &ListHead);
	SetFocus(hWndCtrl);

	SetWindowText(hWnd, L"Attach");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
CpuAttachOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCPU_ATTACH_CONTEXT Context;
	HWND hWndCtrl;
	INT_PTR Id;
	int index;

	Id = LOWORD(wp);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CPU_ATTACH_CONTEXT);

	if (Id == IDOK) {

		//
		// Get sampling target process
		//

		hWndCtrl = GetDlgItem(hWnd, IDC_LIST_TASK);
		index = ListViewGetFirstSelected(hWndCtrl);

		if (index == -1) {
			MessageBox(hWnd, L"No process is selected!", L"D Profiler", MB_OK|MB_ICONWARNING);
			return 0;
		}

		ListViewGetParam(hWndCtrl, index, (LPARAM *)&Context->Process);
		ListView_DeleteItem(hWndCtrl, index);

		//
		// Get sampling period
		//

		hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_PERIOD);
		index = SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);
		Context->Period = SendMessage(hWndCtrl, CB_GETITEMDATA, index, 0);

	}

	CpuAttachCleanUp(Object);
	EndDialog(hWnd, Id);
	return TRUE;
}

LRESULT
CpuAttachOnRefresh(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	HWND hWndCtrl;
	LIST_ENTRY ListHead;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	CpuAttachCleanUp(Object);

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_TASK);
	ListView_DeleteAllItems(hWndCtrl);
	
	InitializeListHead(&ListHead);
	BspQueryProcessList(&ListHead);
	CpuAttachInsertTask(Object, &ListHead);

	return TRUE;
}

VOID
CpuAttachCleanUp(
	IN PDIALOG_OBJECT Object
	)
{
	HWND hWndCtrl;
	PBSP_PROCESS Process;
	int Count, i;
	
	hWndCtrl = GetDlgItem(Object->hWnd, IDC_LIST_TASK);
	Count = ListView_GetItemCount(hWndCtrl);

	for(i = 0; i < Count; i++) {
		ListViewGetParam(hWndCtrl, i, (LPARAM *)&Process);
		BspFreeProcess(Process);
	}
}

VOID
CpuAttachInsertTask(
	IN PDIALOG_OBJECT Object,
	IN PLIST_ENTRY ListHead
	)
{
	PBSP_PROCESS Process;
	PLIST_ENTRY ListEntry;
	HWND hWnd;
	LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	HANDLE ProcessHandle;
	ULONG CurrentId;
	int i;	

	ASSERT(Object != NULL);

	hWnd = GetDlgItem(Object->hWnd, IDC_LIST_TASK);
	i = 0;
	CurrentId = GetCurrentProcessId();

	while (IsListEmpty(ListHead) != TRUE) {

		ListEntry = RemoveHeadList(ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		if (Process->ProcessId == 0 || Process->ProcessId == 4 || 
			Process->ProcessId == CurrentId ) {
			BspFreeProcess(Process);	
			continue;
		}
		
		memset(&lvi, 0, sizeof(lvi));

		if (BspIs64Bits) {

			//
			// N.B. D Probe x64 version only list 64 bits process. 
			//

			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Process->ProcessId);
			if (!ProcessHandle) {
				BspFreeProcess(Process);	
				continue;
			}

			if (BspIsWow64Process(ProcessHandle)) {
				CloseHandle(ProcessHandle);
				BspFreeProcess(Process);	
				continue;
			}

		} 

		else if (BspIsWow64) {

			//
			// N.B. D Probe x86 version only list 32 bits process on 64 bits Windows. 
			//

			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Process->ProcessId);
			if (!ProcessHandle) {
				BspFreeProcess(Process);	
				continue;
			}

			if (!BspIsWow64Process(ProcessHandle)) {
				CloseHandle(ProcessHandle);
				BspFreeProcess(Process);	
				continue;
			}
		}
		
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.pszText = Process->Name;
		lvi.lParam = (LPARAM)Process;

		ListView_InsertItem(hWnd, &lvi);
		i += 1;
	}

	for(i = 0; i < ListView_GetItemCount(hWnd); i += 1) {
		
		ListViewGetParam(hWnd, i, (LPARAM *)&Process);

		lvi.iItem = i;
		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWnd, &lvi);

		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = Process->FullPath;
		ListView_SetItem(hWnd, &lvi);
	}
}