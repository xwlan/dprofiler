//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#define _IPCTL_
#include "iosocket.h"
#include "profileform.h"
#include "apspdb.h"
#include "aps.h"
#include "apsrpt.h"
#include "frame.h"
#include "resource.h"
#include "iostack.h"
#include "apsio.h"
#include "aps.h"

DIALOG_SCALER_CHILD IoSkChildren[] = {
	{ IDC_LIST_IO_SOCKET, AlignRight, AlignBottom }
};

DIALOG_SCALER IoSkScaler = {
	{0,0}, {0,0}, {0,0}, 1, IoSkChildren
};

typedef enum _IoSocketColumnOrdinal{
	_SkSourceAddress,
	_SkSourcePort,
	_SkDestineAddress,
	_SkDestinePort,
	_SkProtocal,
	_SkIoCount,
	_SkReadCount,
	_SkWriteCount,
	_SkFailedCount,
	_SkReadBytes,
	_SkWriteBytes,
	_SkMaxReadLatency,
	_SkMeanReadLatency,
	_SkMaxWriteLatency,
	_SkMeanWriteLatency,
	_SkMaxReadBytes,
	_SkMeanReadBytes,
	_SkMaxWriteBytes,
	_SkMeanWriteBytes,
} _IoSocketColumnOrdinal;

LISTVIEW_COLUMN IoSkColumn[] = {
	{ 200,	L"Source",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,	L"Port",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200,	L"Destine",  LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,	L"Port",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,	L"Protocal",    LVCFMT_LEFT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100,	L"IO Count",  LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100,	L"R Count",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100,	L"W Count",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100,	L"F Count",  LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"R Bytes",LVCFMT_RIGHT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"W Bytes",    LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 140,	L"Max R Latency(ms)", LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 140,	L"Mean R Latency(ms)",LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 140,	L"Max W Latency(ms)", LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 140,	L"Mean W Latency(ms)",LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Max R Bytes", LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Mean R Bytes", LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Max W Bytes", LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,	L"Mean W Bytes", LVCFMT_RIGHT,  0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

ULONG IoSkColumnCount = ARRAYSIZE(IoSkColumn);


#define IoGetSocketObjectName(_N, _O)\
	((SOCKADDR_STORAGE *)((PCHAR)_N + _O->NameOffset))


PIO_SK_CONTEXT FORCEINLINE
IoSkGetContext(
	__in PDIALOG_OBJECT Object
	)
{
	PIO_FORM_CONTEXT Context;
	Context = (PIO_FORM_CONTEXT)Object->Context;
	return (PIO_SK_CONTEXT)Context->Context;
}

HWND
IoSkCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	PIO_SK_CONTEXT SkContext;
	HWND hWnd;
	
	SkContext = (PIO_SK_CONTEXT)SdkMalloc(sizeof(IO_SK_CONTEXT));
	SkContext->PercentSpace = 0;
	SkContext->PercentMinimum = 0;

	Context = (PIO_FORM_CONTEXT)SdkMalloc(sizeof(IO_FORM_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->Head = NULL;
	Context->Path[0] = 0;
	Context->TreeList = NULL;
	Context->Context = SkContext;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_IO_SOCKET;
	Object->Procedure = IoSkProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
IoSkOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	HWND hWndCtrl;
	LVCOLUMN lvc = {0};
	LVITEM lvi = {0};
	ULONG i;
	PLISTVIEW_OBJECT ListView;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	
	//
	// Create background brush for static controls
	//

	Context->hBrushBack = CreateSolidBrush(RGB(255, 255, 255));

	//
	// Create listview object wraps list control
	//

	ListView = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ZeroMemory(ListView, sizeof(LISTVIEW_OBJECT));

	ListView->Column = IoSkColumn;
	ListView->Count = IoSkColumnCount;
	ListView->NotifyCallback = IoSkOnNotify;
	
	Context->ListView = ListView;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_SOCKET);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

	for (i = 0; i < IoSkColumnCount; i++) {
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
		lvc.iSubItem = i;
		lvc.pszText = IoSkColumn[i].Title;
		lvc.cx = IoSkColumn[i].Width;
		lvc.fmt = IoSkColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i, &lvc);
	}

	//
	// Create bold font for symbol name
	//

	Context->hFontBold = SdkCreateListBoldFont();

	GetClientRect(hWnd, &Rect);
	MoveWindow(hWndCtrl, Rect.left, Rect.top, 
				Rect.right - Rect.left, 
				Rect.bottom - Rect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &IoSkScaler;
	DialogRegisterScaler(Object);
	
	//SetWindowSubclass(hWndCtrl, IoSkListProcedure, 0, (DWORD_PTR)Object);
	return TRUE;
}

LRESULT
IoSkOnClose(
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
IoSkProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

	case WM_INITDIALOG:
		return IoSkOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_CLOSE:
		return IoSkOnClose(hWnd, uMsg, wp, lp);

	case WM_NOTIFY:
		return IoSkOnNotify(hWnd, uMsg, wp, lp);
	
	case WM_COMMAND:
		return IoSkOnCommand(hWnd, uMsg, wp, lp);

	}

	return Status;
}

LRESULT
IoSkOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
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

		case ID_IO_STACKTRACE:
			return IoSkOnStackTrace(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
IoSkOnNotify(
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

	if(IDC_LIST_IO_SOCKET == pNmhdr->idFrom) {

		switch (pNmhdr->code) {

			case LVN_COLUMNCLICK: 
			Status = IoSkOnColumnClick(Object, (NM_LISTVIEW *)lp);
			break;

			case NM_RCLICK:
			Status = IoSkOnContextMenu(hWnd, uMsg, wp, lp);
			break;
		}
	}


	return Status;
}

LRESULT
IoSkOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	POINT pt;
	PDIALOG_OBJECT Object;
	HMENU hPopupMenu = NULL;
	HMENU hMenuLoaded;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	GetCursorPos(&pt);

	//
	// Display the context menu
	//

	hMenuLoaded = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_IO)); 
	hPopupMenu = GetSubMenu(hMenuLoaded, 0);

	//
	// The menu command must be routed to dialog, not tabctrl
	//

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,               
				   pt.x, pt.y, 0, hWnd, NULL); 
	DestroyMenu(hMenuLoaded);	
	return 0;
}

LRESULT 
IoSkOnColumnClick(
	__in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int nColumnCount;
	int i;
	HDITEM hdi;
	LISTVIEW_OBJECT *ListView;
	PIO_FORM_CONTEXT Context;
	HWND hWndCtrl;
	HWND hWnd;

	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
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
	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_SOCKET);
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
    ListView_SortItemsEx(hWndCtrl, IoSkSortCallback, (LPARAM)hWnd);

    return 0L;
}

int CALLBACK
IoSkSortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	)
{
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	LISTVIEW_OBJECT *ListView;
	PIO_OBJECT_ON_DISK Object1, Object2;
	HWND hWnd;
    int Result;
	HWND hWndList;
	__int64 I1,I2;
	double F1,F2;

	hWnd = (HWND)Param;
	hWndList = GetDlgItem(hWnd, IDC_LIST_IO_SOCKET);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);

	ListView = Context->ListView;
	ListViewGetParam(hWndList, (LONG)First, (LPARAM *)&Object1);
	ListViewGetParam(hWndList, (LONG)Second, (LPARAM *)&Object2);

	ASSERT(Object1 != NULL);
	ASSERT(Object2 != NULL);

	ListView_GetItemText(hWndList, First,  ListView->LastClickedColumn, FirstData,  MAX_PATH);
	ListView_GetItemText(hWndList, Second, ListView->LastClickedColumn, SecondData, MAX_PATH);

    //
    // Symbol or Module name or Line
    //

	switch(ListView->LastClickedColumn){
	
	case _SkSourcePort:
	case _SkDestinePort:
	case _SkIoCount:
	case _SkReadCount:
	case _SkWriteCount:
	case _SkFailedCount:
	case _SkReadBytes:
	case _SkWriteBytes:
	case _SkMaxReadBytes:
	case _SkMeanReadBytes:
	case _SkMaxWriteBytes:
	case _SkMeanWriteBytes:

		I1 = _wcstoi64(FirstData, NULL, 10);
		I2 = _wcstoi64(SecondData, NULL, 10);
		Result = (int)(I1 - I2);
		break;

	case _SkSourceAddress:
	case _SkDestineAddress:
	case _SkProtocal:

		Result = wcsicmp(FirstData, SecondData);
		break;

	case _SkMaxReadLatency:
	case _SkMeanReadLatency:
	case _SkMaxWriteLatency:
	case _SkMeanWriteLatency:

		F1 = wcstod(FirstData, NULL);
		F2 = wcstod(SecondData, NULL);

		if (F1 < F2) {
			Result = -1;
		}
		else if (fabs(F1 - F2) < 0.0001f) {
			Result = 0;
		}
		else if (F1 > F2) {
			Result = 1;
		}
		else {
			Result = 0;
		}
		break;

	default:
		ASSERT(0);
		Result = 0;
	}
	
	return ListView->SortOrder ? Result : -Result;
}

VOID
IoSkInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	)
{
	LVITEM lvi = {0};
	ULONG Count = 0;
	ULONG Total = 0;
	WCHAR Buffer[MAX_PATH];
	HWND hWndCtrl;
	PWSTR Unicode;
	PDIALOG_OBJECT Object;
	PIO_FORM_CONTEXT Context;
	PIO_OBJECT_ON_DISK IoObject;
	int Number;
	int i, j;
	PWSTR Name;
	ULONG Offset;
	PPF_STREAM_SYSTEM System;
	LARGE_INTEGER Qpc;
	double Microseconds;

	ASSERT(Head != NULL);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IO_FORM_CONTEXT);
	Context->Head = Head;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_IO_SOCKET);
	
	IoObject = (PIO_OBJECT_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_OBJECT);
	Number = ApsGetStreamRecordCount(Head, STREAM_IO_OBJECT, IO_OBJECT_ON_DISK);
	Name = (PWSTR)ApsGetStreamPointer(Head, STREAM_IO_NAME);
	Offset = 0;

	System = (PPF_STREAM_SYSTEM)ApsGetStreamPointer(Head, STREAM_SYSTEM);
	Qpc = System->QpcFrequency;

	for(i = 0, j = 0; i < Number; i++, IoObject++) {

		//
		// If there's no io issued on this object, skip it
		//

		if (!IoObject->IrpCount) {
			continue;
		}

		if (IoIsSocketObject(IoObject)) {

			//
			// Source
			//
			
			SOCKADDR_STORAGE *Address;
			Address = IoGetSocketObjectName(Name, IoObject);
			if (Address->ss_family == AF_UNSPEC) {
				continue;
			}

			lvi.iItem = j;
			lvi.iSubItem = _SkSourceAddress;
			lvi.mask = LVIF_TEXT|LVIF_PARAM;
			lvi.lParam = (LPARAM)IoObject;
			ApsSockaddrToString(Address, Buffer);
			lvi.pszText = Buffer;
			ListView_InsertItem(hWndCtrl, &lvi);

			//
			// Source Port
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkSourcePort;
			lvi.mask = LVIF_TEXT;
			
			ApsSockaddrToPort(Address, Buffer, MAX_PATH);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);
			
			//
			// Destine
			//

			Address += 1;

			lvi.iItem = j;
			lvi.iSubItem = _SkDestineAddress;
			lvi.mask = LVIF_TEXT;
			ApsSockaddrToString(Address, Buffer);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);
			
			//
			// Destine Port
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkDestinePort;
			lvi.mask = LVIF_TEXT;
			ApsSockaddrToPort(Address, Buffer, MAX_PATH);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Protocal 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkProtocal;
			lvi.mask = LVIF_TEXT;
			
			if (IoIsSocketTcp(IoObject)) {
				lvi.pszText = L"TCP";
			} else if(IoIsSocketUdp(IoObject)){
				lvi.pszText = L"UDP";
			} else {
				lvi.pszText = L"UNSPECIFIED";
			}

			ListView_SetItem(hWndCtrl, &lvi);

			//
			// IO Count
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkIoCount;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->IrpCount);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// R Count
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkReadCount;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_READ].RequestCount);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// W Count
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkWriteCount;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_WRITE].RequestCount);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// F Count
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkFailedCount;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_READ].FailureCount +
														IoObject->Counters[IO_OP_WRITE].FailureCount);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// R Bytes 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkReadBytes;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_READ].CompleteSize);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// W Bytes 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkWriteBytes;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_WRITE].CompleteSize);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Maximum R Latency 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMaxReadLatency;
			lvi.mask = LVIF_TEXT;
			Microseconds = ApsComputeMicroseconds(IoObject->Counters[IO_OP_READ].MaximumLatency, Qpc.QuadPart);
			StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Microseconds/1000);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Mean R Latency 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMeanReadLatency;
			lvi.mask = LVIF_TEXT;
			Microseconds = ApsComputeMicroseconds(IoObject->Counters[IO_OP_READ].AverageLatency, Qpc.QuadPart);
			StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Microseconds/1000);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Maximum W Latency 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMaxWriteLatency;
			lvi.mask = LVIF_TEXT;
			Microseconds = ApsComputeMicroseconds(IoObject->Counters[IO_OP_WRITE].MaximumLatency, Qpc.QuadPart);
			StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Microseconds/1000);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Mean W Latency 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMeanWriteLatency;
			lvi.mask = LVIF_TEXT;
			Microseconds = ApsComputeMicroseconds(IoObject->Counters[IO_OP_WRITE].AverageLatency, Qpc.QuadPart);
			StringCchPrintf(Buffer, MAX_PATH, L"%.3f", Microseconds/1000);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Max Read Packet Bytes 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMaxReadBytes;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_READ].MaximumSize);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Mean Read Packet Bytes 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMeanReadBytes;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_READ].AverageSize);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Max Write Packet Bytes 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMaxWriteBytes;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_WRITE].MaximumSize);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Mean Write Packet Bytes 
			//

			lvi.iItem = j;
			lvi.iSubItem = _SkMeanWriteBytes;
			lvi.mask = LVIF_TEXT;
			StringCchPrintf(Buffer, MAX_PATH, L"%d", IoObject->Counters[IO_OP_WRITE].AverageSize);
			lvi.pszText = Buffer;
			ListView_SetItem(hWndCtrl, &lvi);

			//
			// Increase item number
			//

			j += 1;
		
		}
	}

	//
	// Update status bar of frame window
	//

	Unicode = (PWSTR)ApsMalloc(MAX_PATH * 2);
	StringCchPrintf(Unicode, MAX_PATH, L"Total %u socket objects traced", j);
	PostMessage(GetParent(hWnd), WM_USER_STATUSBAR, 0, (LPARAM)Unicode);
}


LRESULT 
IoSkOnDbClick(
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

	if (lpnmitem->iSubItem != 0) {
		return 0;
	}

	//
	// Check whether there's any source information
	//

	hWndList = lpnmitem->hdr.hwndFrom;

	Buffer[0] = 0;
	ListView_GetItemText(hWndList, lpnmitem->iItem, 0, Buffer, MAX_PATH);

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

LRESULT
IoSkOnStackTrace(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCCR_FORM_CONTEXT Context;
	PIO_OBJECT_ON_DISK IoObject;
	IO_OBJECT_ON_DISK Clone;
	HWND hWndList;
	LONG Index;

	hWndList = GetDlgItem(hWnd, IDC_LIST_IO_SOCKET);
	Index = ListViewGetFirstSelected(hWndList);
	ASSERT(Index != -1);

	ListViewGetParam(hWndList, Index, (LPARAM *)&IoObject);
	ASSERT(IoObject != NULL);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CCR_FORM_CONTEXT);
	ASSERT(Object->Context != NULL);

	//
	// Pass the cloned on stack object to stack trace dialog, 
	// because we don't want change io object's on disk state.
	//

	Clone = *IoObject;
	Clone.Trace = NULL;

	IoStackCreate(hWnd, 0, Context->Head, &Clone);
	if (Clone.Trace) {
		ApsIoFreeAttachedStackTrace(Clone.Trace);
	}	
	return 0;
}