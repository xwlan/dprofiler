//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "status.h"
#include "sdk.h"
#include "frame.h"
#include "menu.h"
#include "rebar.h"
#include "listview.h"
#include "main.h"
#include "perfview.h"
#include "aboutdlg.h"
#include "ctrldlg.h"
#include "wizard.h"
#include "cpuwiz.h"
#include "mmwiz.h"
#include "iowiz.h"
#include "analyzedlg.h"
#include "apsctl.h"
#include "apsdefs.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "ioview.h"
#include "apscpu.h"
#include "aps.h"
#include "optiondlg.h"
#include "mmsummary.h"
#include "mmleak.h"
#include "mmheap.h"
#include "mmmodule.h"
#include "mmtree.h"
#include "mmflame.h"
#include "cpusummary.h"
#include "cpupc.h"
#include "cpufunc.h"
#include "cpumodule.h"
#include "cputhread.h"
#include "cpuhistory.h"
#include "cputree.h"
#include "cpuflame.h"
#include "ctlpane.h"
#include "deduction.h"
#include "srcdlg.h"
#include "help.h"

PWSTR SdkFrameClass = L"SdkFrame";

BOOLEAN
FrameRegisterClass(
	VOID
	)
{
    WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);

	SdkMainIcon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_APSARA));
    
	wc.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = FrameProcedure;
    wc.lpszClassName  = SdkFrameClass;
	wc.lpszMenuName   = MAKEINTRESOURCE(IDC_DPROFILE);

    RegisterClassEx(&wc);
    return TRUE;
}

HWND 
FrameCreate(
	__in PFRAME_OBJECT Object 
	)
{
	BOOLEAN Status;
	HWND hWnd;

	Status = FrameRegisterClass();
	ASSERT(Status);

	if (Status != TRUE) {
		return NULL;
	}

	hWnd = CreateWindowEx(0, SdkFrameClass, L"", 
						  WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
						  0, 0,
						  800, 600,
						  0, 0, SdkInstance, (PVOID)Object);

	ASSERT(hWnd);

	if (hWnd != NULL) {
		SdkSetMainIcon(hWnd);
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		return hWnd;

	} else {
		return NULL;
	}
}

VOID
FrameGetViewRect(
	__in PFRAME_OBJECT Object,
	__out RECT *rcView
	)
{
	RECT rcClient;
	RECT rcRebar = {0};
    RECT rcStatusBar = {0};

    GetClientRect(Object->hWnd, &rcClient);
    
    if (Object->hWndStatusBar) {
        GetWindowRect(Object->hWndStatusBar, &rcStatusBar);
    }
	
    if (Object->Rebar != NULL) {
		GetWindowRect(Object->Rebar->hWndRebar, &rcRebar);
    }
	
	rcView->top    = rcClient.top + (rcRebar.bottom - rcRebar.top) + 2;
	rcView->left   = rcClient.left;
	rcView->right  = rcClient.right;
    rcView->bottom = rcClient.bottom - rcClient.top - (rcStatusBar.bottom - rcStatusBar.top);

	if ((rcView->bottom <= rcView->top) || (rcView->right <= rcView->left)){
        rcView->top = 0;
		rcView->left = 0;
		rcView->right = 0;
		rcView->left = 0;    
    }
}

LRESULT 
FrameOnContextMenu(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	int x, y;
	POINT Point;
	RECT Rect;
	PFRAME_OBJECT Object;

	x = GET_X_LPARAM(lp); 
	y = GET_Y_LPARAM(lp); 

	Point.x = x;
	Point.y = y;

	ScreenToClient(hWnd, &Point);
	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	FrameGetViewRect(Object, &Rect);

	if (Point.x >= Rect.left && Point.x <= Rect.right && 
		Point.y >= Rect.top  && Point.y <= Rect.bottom ) {

        //
        // N.B. Not implemented
        //
	}

	return 0L;
}

LRESULT
FrameOnExit(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	FrameOnClose(hWnd, uMsg, wp, lp);	
	PostQuitMessage(0);
	return 0;
}

LRESULT
FrameOnAbout(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	AboutDialog(hWnd);
	return 0;
}

LRESULT
FrameOnHelp(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HelpDialog(hWnd);
	return 0;
}

LRESULT
FrameOnGetMinMaxInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFRAME_OBJECT Object;
	MINMAXINFO *Info = (PMINMAXINFO)lp; 

    Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	if (!Object) {
		return 0;
	}

	Info->ptMinTrackSize.x = 800;
	Info->ptMinTrackSize.y = 600;

	return 0;
}

LRESULT
FrameOnOpen(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status;
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];
	PFRAME_OBJECT Frame;

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = Ofn.lpstrFilter = L"D Profile Report (*.dpf)\0*.dpf\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Profile";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (!Status) {
		return 0;
	}

    //
    // Check whether it's opening current report
    //

    Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);
    if (!wcsicmp(Path, Frame->FilePath)) {
        MessageBox(hWnd, L"It's already opened!", L"D Profile", MB_OK|MB_ICONWARNING);
        return 0;
    }

    StringCchCopy(Frame->FilePath, MAX_PATH, Path);

    //
    // Open profile report, note that this is a asynchronous operation,
    // the logic is mainly handled in FrameOnUserProfileOpened(), after
    // the report is opened, frame gets a notification
    //

    Status = CtlPaneOpenReport(Frame->hWndCtlPane, Path);
    if (Status != APS_STATUS_OK) {
        return 0;
    }

    Frame->State = FRAME_REPORTING;
    FrameSetMenuState(Frame);
	return 0;
}

LRESULT
FrameOnSave(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status;
	OPENFILENAME Ofn;
	PFRAME_OBJECT Object;
	PPF_REPORT_HEAD Head;
	PWCHAR Ptr;
	WCHAR Path[MAX_PATH];
	WCHAR Name[MAX_PATH];

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	if (!Object->Head) {
		return 0;
	}
	
	Head = Object->Head;
	Ptr = wcsrchr(Object->FilePath, L'\\');
	wcscpy_s(Name, MAX_PATH, Ptr);

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = Ofn.lpstrFilter = L"D Profile Report (*.dpf)\0*.dpf\0\0";
	Ofn.lpstrFile = Name; 
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Profile";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetSaveFileName(&Ofn);
	if (!Status) {
		return 0;
	}
	
	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	CopyFile(Object->FilePath, Path, FALSE);
	return 0;
}

LRESULT
FrameOnOption(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	OptionDialog(hWnd);
	return 0;
}

LRESULT
FrameOnAnalyze(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFRAME_OBJECT Frame;
    ULONG Status;
	PPF_REPORT_HEAD Head;
	int Continue;

	Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);
	Head = Frame->Head;

	if (Head != NULL && Head->Analyzed) {
		
		//
		// Prompt user it's analyzed and whether user want to
		// discard previous analysis and re-analyze again
		//

		Continue = MessageBox(hWnd, L"The report was analyzed, press OK to continue", 
							  L"D Profile", MB_YESNO|MB_ICONWARNING);
		if (Continue != IDYES) {
			return APS_STATUS_OK;
		}
	}
	
    FrameCloseReport(Frame);
    
    //
    // Ensure the frame state is reporting
    //

    ASSERT(Frame->State == FRAME_REPORTING);

	Status = CtlPaneOnAnalyze(Frame->hWndCtlPane);
	if (Status != APS_STATUS_OK) {
        return Status;
    }

    FrameCleanForm(Frame);
    FrameShowCtlPane(Frame, TRUE);

    Frame->State = FRAME_ANALYZING;
    FrameSetMenuState(Frame);
	return 0;
}

LRESULT
FrameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	RECT ClientRect;
	PFRAME_OBJECT Object;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	//
	// Adjust toolbar, statusbar automatically
	//

	if (Object->Rebar){
		RebarAdjustPosition(Object->Rebar);
	}
	if (Object->hWndStatusBar){
		SendMessage(Object->hWndStatusBar, WM_SIZE, 0, 0);	
	}
	
	//
	// Adjust view position after toolbar, statusbar
	//

    if (Object->hWndCtlPane && Object->ShowCtlPane) {

        FrameGetViewRect(Object, &ClientRect);
		MoveWindow(Object->hWndCtlPane, 
				   ClientRect.left, 
				   ClientRect.top, 
				   ClientRect.right - ClientRect.left, 
				   ClientRect.bottom - ClientRect.top,
				   TRUE);
    }

	if (Object->hWndCurrent) {

		FrameGetViewRect(Object, &ClientRect);
		MoveWindow(Object->hWndCtlPane, 
				   ClientRect.left, 
				   ClientRect.top, 
				   ClientRect.right - ClientRect.left, 
				   ClientRect.bottom - ClientRect.top,
				   TRUE);

		MoveWindow(Object->hWndCurrent, 
				   ClientRect.left, 
				   ClientRect.top, 
				   ClientRect.right - ClientRect.left, 
				   ClientRect.bottom - ClientRect.top,
				   TRUE);

	}

	return 0;
}

LRESULT
FrameOnCommand(
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

		case IDM_ABOUT:
			return FrameOnAbout(hWnd, uMsg, wp, lp);

		case IDM_HELP:
			return FrameOnHelp(hWnd, uMsg, wp, lp);

		case IDM_EXIT:
			return FrameOnExit(hWnd, uMsg, wp, lp);

		case IDM_CPU:
			return FrameOnCpu(hWnd, uMsg, wp, lp);

		case IDM_MM:
			return FrameOnMm(hWnd, uMsg, wp, lp);
		
		case IDM_IO:
			return FrameOnIo(hWnd, uMsg, wp, lp);

		case IDM_OPEN:
			return FrameOnOpen(hWnd, uMsg, wp, lp);

		case IDM_SAVE:
			return FrameOnSave(hWnd, uMsg, wp, lp);

		case IDM_ANALYZE:
			return FrameOnAnalyze(hWnd, uMsg, wp, lp);

		case IDM_OPTION:
			return FrameOnOption(hWnd, uMsg, wp, lp);
		
		case IDM_FIND_FORWARD:
			return FrameOnFindForward(hWnd, uMsg, wp, lp);
		
		case IDM_FIND_BACKWARD:
			return FrameOnFindBackward(hWnd, uMsg, wp, lp);

		case IDM_DEDUCTION:
			return FrameOnDeduction(hWnd, uMsg, wp, lp);

	}

	if (HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == REBAR_BAND_SWITCH) {
		return FrameOnSwitchView(hWnd, (HWND)lp);
	}

	return 0;
}

LRESULT
FrameOnSwitchView(
	__in HWND hWnd,
	__in HWND hWndCombo
	)
{
	PFRAME_OBJECT Object;
	ULONG Current;
	HWND hWndForm;

	Current = (ULONG)SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);

    if (Object->hWndCtlPane && Object->ShowCtlPane) {
        return 0;
    }
	
	ShowWindow(Object->hWndCurrent, SW_HIDE);

	if (Object->Type == PROFILE_CPU_TYPE) {

		if (Object->CpuForm.Current != Current) {

            //
            // N.B. The following paragraph of code can be reduced to
            // be single case, reserved for improvement in future
            //

			switch (Current) {
					
				case CPU_FORM_CALLTREE:

					if (Object->CpuForm.hWndForm[CPU_FORM_CALLTREE] == NULL) {
						hWndForm = CpuTreeCreate(hWnd, CPU_FORM_CALLTREE);
						Object->CpuForm.hWndForm[CPU_FORM_CALLTREE] = hWndForm;
						CpuTreeInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_CALLTREE];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

				case CPU_FORM_FLAME:

					if (Object->CpuForm.hWndForm[CPU_FORM_FLAME] == NULL) {
						hWndForm = CpuFlameCreate(hWnd, CPU_FORM_FLAME);
						Object->CpuForm.hWndForm[CPU_FORM_FLAME] = hWndForm;
						CpuFlameInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_FLAME];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

				case CPU_FORM_SUMMARY:

					if (Object->CpuForm.hWndForm[CPU_FORM_SUMMARY] == NULL) {
						hWndForm = CpuSummaryCreate(hWnd, CPU_FORM_SUMMARY);
						Object->CpuForm.hWndForm[CPU_FORM_SUMMARY] = hWndForm;
						CpuSummaryInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_SUMMARY];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

				case CPU_FORM_IP:
					
					if (Object->CpuForm.hWndForm[CPU_FORM_IP] == NULL) {
						hWndForm = CpuPcCreate(hWnd, CPU_FORM_IP);
						Object->CpuForm.hWndForm[CPU_FORM_IP] = hWndForm;
						CpuPcInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_IP];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);

					break;

				case CPU_FORM_FUNCTION:

					if (Object->CpuForm.hWndForm[CPU_FORM_FUNCTION] == NULL) {
						hWndForm = CpuFuncCreate(hWnd, CPU_FORM_FUNCTION);
						Object->CpuForm.hWndForm[CPU_FORM_FUNCTION] = hWndForm;
						CpuFuncInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_FUNCTION];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

				case CPU_FORM_MODULE:

					if (Object->CpuForm.hWndForm[CPU_FORM_MODULE] == NULL) {
						hWndForm = CpuModuleCreate(hWnd, CPU_FORM_MODULE);
						Object->CpuForm.hWndForm[CPU_FORM_MODULE] = hWndForm;
						CpuModuleInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_MODULE];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

                case CPU_FORM_THREAD:

					if (Object->CpuForm.hWndForm[CPU_FORM_THREAD] == NULL) {
						hWndForm = CpuThreadCreate(hWnd, CPU_FORM_THREAD);
						Object->CpuForm.hWndForm[CPU_FORM_THREAD] = hWndForm;
						CpuThreadInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_THREAD];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

                case CPU_FORM_HISTORY:

					if (Object->CpuForm.hWndForm[CPU_FORM_HISTORY] == NULL) {
						hWndForm = CpuHistoryCreate(hWnd, CPU_FORM_HISTORY);
						Object->CpuForm.hWndForm[CPU_FORM_HISTORY] = hWndForm;
						CpuHistoryInsertData(hWndForm, Object->Head);
					} 

					Object->CpuForm.Current = (CPU_FORM_TYPE)Current;
					Object->hWndCurrent = Object->CpuForm.hWndForm[CPU_FORM_HISTORY];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
                    break;

                default:
                    ASSERT(0);
			}
		}
		else {
			ShowWindow(Object->hWndCurrent, SW_SHOW);
		}
	}

	if (Object->Type == PROFILE_MM_TYPE) {

		if (Object->MmForm.Current != Current) {

			switch (Current) {

				case MM_FORM_SUMMARY:

					if (Object->MmForm.hWndForm[MM_FORM_SUMMARY] == NULL) {
						hWndForm = MmSummaryCreate(hWnd, MM_FORM_SUMMARY);
						Object->MmForm.hWndForm[MM_FORM_SUMMARY] = hWndForm;
						MmSummaryInsertData(hWndForm, Object->Head);
					} 

					Object->MmForm.Current = (MM_FORM_TYPE)Current;
					Object->hWndCurrent = Object->MmForm.hWndForm[MM_FORM_SUMMARY];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

				case MM_FORM_LEAK:

					if (Object->MmForm.hWndForm[MM_FORM_LEAK] == NULL) {
						hWndForm = MmLeakCreate(hWnd, MM_FORM_LEAK);
						Object->MmForm.hWndForm[MM_FORM_LEAK] = hWndForm;
						MmLeakInsertData(hWndForm, Object->Head);
					} 

					Object->MmForm.Current = (MM_FORM_TYPE)Current;
					Object->hWndCurrent = Object->MmForm.hWndForm[MM_FORM_LEAK];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;
				
				case MM_FORM_HEAP:

					if (Object->MmForm.hWndForm[MM_FORM_HEAP] == NULL) {
						hWndForm = MmHeapCreate(hWnd, MM_FORM_HEAP);
						Object->MmForm.hWndForm[MM_FORM_HEAP] = hWndForm;
						MmHeapInsertData(hWndForm, Object->Head);
					} 

					Object->MmForm.Current = (MM_FORM_TYPE)Current;
					Object->hWndCurrent = Object->MmForm.hWndForm[MM_FORM_HEAP];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

                case MM_FORM_CALLTREE:

					if (Object->MmForm.hWndForm[MM_FORM_CALLTREE] == NULL) {
						hWndForm = MmTreeCreate(hWnd, MM_FORM_CALLTREE);
						Object->MmForm.hWndForm[MM_FORM_CALLTREE] = hWndForm;
						MmTreeInsertData(hWndForm, Object->Head);
					} 

					Object->MmForm.Current = (MM_FORM_TYPE)Current;
					Object->hWndCurrent = Object->MmForm.hWndForm[MM_FORM_CALLTREE];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;

				 case MM_FORM_FLAME:

					if (Object->MmForm.hWndForm[MM_FORM_FLAME] == NULL) {
						hWndForm = MmFlameCreate(hWnd, MM_FORM_FLAME);
						Object->MmForm.hWndForm[MM_FORM_FLAME] = hWndForm;
						MmFlameInsertData(hWndForm, Object->Head);
					} 

					Object->MmForm.Current = (MM_FORM_TYPE)Current;
					Object->hWndCurrent = Object->MmForm.hWndForm[MM_FORM_FLAME];

					ShowWindow(Object->hWndCurrent, SW_SHOW);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;
			}
		}
		else {
			ShowWindow(Object->hWndCurrent, SW_SHOW);
		}
	}

	return 0;
}

LRESULT
FrameOnCreate(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	CREATESTRUCT *lpcs;
	PFRAME_OBJECT Frame;
	WCHAR Buffer[MAX_PATH];
    HWND hWndCtlPane;

	lpcs = (CREATESTRUCT *)lp;
	Frame = (PFRAME_OBJECT)lpcs->lpCreateParams;

#ifdef _LITE
#if defined(_M_X64)
	SetWindowText(hWnd, L"D Profile Lite x64");
#else
	SetWindowText(hWnd, L"D Profile Lite");
#endif
#else
	LoadString(SdkInstance, IDS_PRODUCT, Buffer, MAX_PATH);
	SetWindowText(hWnd, Buffer);
#endif

	SdkSetObject(hWnd, Frame);
	Frame->hWnd = hWnd;

	//
	// Load rebar and statusbar
	//

	FrameLoadBars(Frame);

    //
    // Create control pane
    //

    hWndCtlPane = CtlPaneCreate(hWnd, FrameCtlPaneId);
    ASSERT(hWndCtlPane != NULL);
    Frame->hWndCtlPane = hWndCtlPane;
    Frame->ShowCtlPane = TRUE;

    Frame->State = FRAME_BOOTSTRAP;
    FrameSetMenuState(Frame);

    //
    // Center frame window
    //

	SdkCenterWindow(hWnd);
	return 0;
}

LRESULT
FrameOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	MdbClose();
	ApsUninitialize();
	return 0;
}

LRESULT
FrameOnWmUserStatus(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFRAME_OBJECT Object;

	//
	// lp is a PWSTR
	//

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	SendMessage(Object->hWndStatusBar, SB_SETTEXT, SBT_NOBORDERS | 1, (LPARAM)lp);

	ApsFree((PVOID)lp);
	return 0;
}

LRESULT CALLBACK
FrameProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LRESULT Result = -1;

	switch (uMsg) {

		case WM_CREATE:
			FrameOnCreate(hWnd, uMsg, wp, lp);
			break;

		case WM_SIZE:
			FrameOnSize(hWnd, uMsg, wp, lp);
			break;

		case WM_GETMINMAXINFO:
			FrameOnGetMinMaxInfo(hWnd, uMsg, wp, lp);
			break;

		case WM_CLOSE:
			FrameOnClose(hWnd, uMsg, wp, lp);
			PostQuitMessage(0);
			break;

		case WM_COMMAND:
			FrameOnCommand(hWnd, uMsg, wp, lp);
			break;

		case WM_CONTEXTMENU:
			FrameOnContextMenu(hWnd, uMsg, wp, lp);
			break;

		case WM_NOTIFY:
			FrameOnNotify(hWnd, uMsg, wp, lp);
			break;
	}

	if (uMsg == WM_USER_STATUSBAR) {
		return FrameOnWmUserStatus(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_USER_PROFILE_START) {
		return FrameOnUserProfileStarted(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_USER_PROFILE_ANALYZED) {
		return FrameOnUserProfileAnalyzed(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_USER_PROFILE_OPENED) {
		return FrameOnUserProfileOpened(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_USER_PROFILE_COMPLETED) {
		return FrameOnUserProfileCompleted(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_USER_PROFILE_TERMINATED) {
		return FrameOnUserProfileTerminated(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_DEDUCTION_UPDATE) {
		return FrameOnDeductionUpdate(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_SOURCE_CLOSE) {
		return FrameOnSourceClose(hWnd, uMsg, wp, lp);
	}

	if (!Result) {
		return Result;
	}

	return DefWindowProc(hWnd, uMsg, wp, lp);
}

BOOLEAN
FrameLoadBars(
	__in PFRAME_OBJECT Object
	)
{
	HWND hWndFrame;
	PREBAR_OBJECT Rebar;
	HWND hWndStatusBar;
	INT StatusParts[2];

	hWndFrame = Object->hWnd;
	ASSERT(hWndFrame != NULL);

	//
	// Create rebar object and its children
	//

	Rebar = RebarCreateObject(hWndFrame, FrameRebarId);
	if (!Rebar) {
		return FALSE;
	}

	RebarInsertBands(Rebar);
	Object->Rebar = Rebar;

	//
	// Create statusbar object
	//

	hWndStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL, 
								   WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
								   SBARS_SIZEGRIP | CCS_BOTTOM, 0, 0, 0, 0, hWndFrame, 
								   (HMENU)FrameStatusBarId, SdkInstance, NULL);

	ASSERT(hWndStatusBar != NULL);

	if (!hWndStatusBar) {
		return FALSE;
	}

	Object->hWndStatusBar = hWndStatusBar;

	//
	// Initialize status bar
	//

	StatusParts[0] = -1;
	SendMessage(hWndStatusBar, SB_SETTEXT,  SBT_NOBORDERS | 0, (LPARAM)L"Ready");
	ShowWindow(hWndStatusBar, SW_SHOW);
	return TRUE;
}

LRESULT
FrameOnUserProfileStarted(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
    ULONG Status;
    PFRAME_OBJECT Frame;

    Status = (ULONG)wp;
    Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);

    if (Status != APS_STATUS_OK) {

        //
        // N.B. If profile start failed, reset frame state
        // to bootstrap to set appropriate menu state
        //

        Frame->State = FRAME_BOOTSTRAP;
        FrameSetMenuState(Frame);
    }

    return 0;
}

LRESULT
FrameOnUserProfileAnalyzed(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status;
	PFRAME_OBJECT Frame;
	WCHAR Buffer[MAX_PATH];
    BTR_PROFILE_TYPE Type;
	HWND hWndForm;
    
    Status = (ULONG)wp;
	Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);

    if (Status != APS_STATUS_OK) {

        //
        // If failed to open report, reset to bootstrap state
        //

        Frame->State = FRAME_BOOTSTRAP;
        FrameSetMenuState(Frame);
        return 0;
    }

    Frame->State = FRAME_REPORTING;
    FrameSetMenuState(Frame);

    CtlPaneGetProfileAttributes(Frame->hWndCtlPane, &Type, Buffer, MAX_PATH);

	Status = FrameOpenReport(Frame, Buffer);
	if (!Status) {
		MessageBox(hWnd, L"Failed to open report!", L"D Profile", MB_OK|MB_ICONERROR);
		return 0;
	}

	if (Type == PROFILE_CPU_TYPE) {

		Frame->Type = PROFILE_CPU_TYPE;
		RebarSetType(Frame->Rebar, PROFILE_CPU_TYPE);

		hWndForm = CpuSummaryCreate(hWnd, CPU_FORM_SUMMARY);
		Frame->CpuForm.hWndForm[CPU_FORM_SUMMARY] = hWndForm;
        Frame->CpuForm.Current = CPU_FORM_SUMMARY;
		Frame->hWndCurrent = hWndForm;
        
		CpuSummaryInsertData(hWndForm, Frame->Head);
	}

    else if (Type == PROFILE_MM_TYPE) {
		
        Frame->Type = PROFILE_MM_TYPE;
		RebarSetType(Frame->Rebar, PROFILE_MM_TYPE);

        hWndForm = MmSummaryCreate(hWnd, MM_FORM_SUMMARY);
		Frame->MmForm.hWndForm[MM_FORM_SUMMARY] = hWndForm;
        Frame->MmForm.Current = MM_FORM_SUMMARY;
		Frame->hWndCurrent = hWndForm;
        
        MmSummaryInsertData(hWndForm, Frame->Head);
    }

    else {
        ASSERT(0);
    }

	//
	// Change frame object state to reporting mode
	//

	FrameShowCtlPane(Frame, FALSE);
	Frame->State = FRAME_REPORTING;
    ShowWindow(Frame->hWndCurrent, SW_SHOW);

	PostMessage(hWnd, WM_SIZE, 0, 0);
	SendMessage(Frame->hWndStatusBar, SB_SETTEXT, SBT_NOBORDERS | 0, (LPARAM)Buffer);
	return 0;
}

LRESULT
FrameOnUserProfileOpened(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status;
	PFRAME_OBJECT Frame;
	WCHAR Buffer[MAX_PATH];
	HWND hWndForm;
    PPF_REPORT_HEAD Head;

    Status = (ULONG)wp;
	Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);

    if (Status != APS_STATUS_OK) {

        //
        // If failed to open report, reset to bootstrap state
        //

        Frame->State = FRAME_BOOTSTRAP;
        FrameSetMenuState(Frame);
        return 0;
    }

	//
	// Clean old report and forms if any
    //

    FrameCloseReport(Frame);
    FrameCleanForm(Frame);

    Frame->State = FRAME_REPORTING;
    FrameSetMenuState(Frame);

    StringCchCopy(Buffer, MAX_PATH, Frame->FilePath);

	Status = FrameOpenReport(Frame, Buffer);
	if (!Status) {
		MessageBox(hWnd, L"Failed to open report!", L"D Profile", MB_OK|MB_ICONERROR);
		return 0;
	}

    Head = Frame->Head;

    if (Head->IncludeCpu) {

		Frame->Type = PROFILE_CPU_TYPE;
		RebarSetType(Frame->Rebar, PROFILE_CPU_TYPE);

		hWndForm = CpuSummaryCreate(hWnd, CPU_FORM_SUMMARY);
		Frame->CpuForm.hWndForm[CPU_FORM_SUMMARY] = hWndForm;
        Frame->CpuForm.Current = CPU_FORM_SUMMARY;
		Frame->hWndCurrent = hWndForm;

		CpuSummaryInsertData(hWndForm, Frame->Head);
	}

    else if (Head->IncludeMm) {
		
        Frame->Type = PROFILE_MM_TYPE;
		RebarSetType(Frame->Rebar, PROFILE_MM_TYPE);

        hWndForm = MmSummaryCreate(hWnd, MM_FORM_SUMMARY);
		Frame->MmForm.hWndForm[MM_FORM_SUMMARY] = hWndForm;
        Frame->MmForm.Current = MM_FORM_SUMMARY;
		Frame->hWndCurrent = hWndForm;
        
        MmSummaryInsertData(hWndForm, Frame->Head);
    }

    else {
        ASSERT(0);
    }

	//
	// Change frame object state to reporting mode
	//

	FrameShowCtlPane(Frame, FALSE);
	Frame->State = FRAME_REPORTING;
    ShowWindow(Frame->hWndCurrent, SW_SHOW);

	PostMessage(hWnd, WM_SIZE, 0, 0);
	SendMessage(Frame->hWndStatusBar, SB_SETTEXT, SBT_NOBORDERS | 0, (LPARAM)Buffer);
	return 0;
}

LRESULT
FrameOnUserProfileCompleted(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
    PFRAME_OBJECT Frame;

    Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);
    Frame->State = FRAME_REPORTING;
    FrameSetMenuState(Frame);
    return 0;
}

LRESULT
FrameOnUserProfileTerminated(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
    PFRAME_OBJECT Frame;

    Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);
    Frame->State = FRAME_TERMINATED;
    FrameSetMenuState(Frame);
    return 0;
}

LRESULT
FrameOnCpu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status;
	PFRAME_OBJECT Frame;
	WIZARD_CONTEXT Context;
	WCHAR ReportPath[MAX_PATH];

	ZeroMemory(&Context, sizeof(Context));

	Context.Type = PROFILE_CPU_TYPE;
	CpuWizard(hWnd, &Context);

	if (Context.Cancel) {
		return 0;
	}

	Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);
	if (Frame->State == FRAME_REPORTING) {
		ApsCleanReport();
	}

	//
	// Create profile object
	//

	if (Context.Attach) {
		Status = CtlPaneCreateProfileByAttach(Frame->hWndCtlPane, &Context, ReportPath);
	}
	else {
		Status = CtlPaneCreateProfileByLaunch(Frame->hWndCtlPane, &Context, ReportPath);
	}

	if (Status != APS_STATUS_OK) {
		MessageBox(hWnd, L"Failed to create profile!", L"D Profile", MB_OK|MB_ICONERROR);
		return Status;
	}

    //
    // Start profile
    //

	CtlPaneStartProfile(Frame->hWndCtlPane);

	FrameCleanForm(Frame);
	FrameCloseReport(Frame);
    Frame->State = FRAME_PROFILING;
    FrameSetMenuState(Frame);

	//
	// Copy report path
	//

	StringCchCopy(Frame->FilePath, MAX_PATH, ReportPath);
	FrameShowCtlPane(Frame, TRUE);
	return 0;
}

VOID
FrameShowCtlPane(
	__in PFRAME_OBJECT Object,
	__in BOOLEAN Show
	)
{
	if (Show) {

		Object->ShowCtlPane = TRUE;

		//
		// The size of frame client area may have changed,
		// force pane do a resize
		//

		ShowWindow(Object->hWndCtlPane, SW_SHOW);
		PostMessage(Object->hWnd, WM_SIZE, 0, 0);

	}
	else {
		Object->ShowCtlPane = FALSE;
		ShowWindow(Object->hWndCtlPane, SW_HIDE);
	}
}

VOID
FrameResizeCtlPane(
	__in PFRAME_OBJECT Object
	)
{
	RECT ClientRect;

	FrameGetViewRect(Object, &ClientRect);
	MoveWindow(Object->hWndCtlPane, 
			   ClientRect.left, 
			   ClientRect.top, 
			   ClientRect.right - ClientRect.left, 
			   ClientRect.bottom - ClientRect.top,
			   TRUE);
}

LRESULT
FrameOnMm(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status;
	PFRAME_OBJECT Frame;
	WIZARD_CONTEXT Context;
	WCHAR ReportPath[MAX_PATH];

	ZeroMemory(&Context, sizeof(Context));

	Context.Type = PROFILE_MM_TYPE;
	MmWizard(hWnd, &Context);

	if (Context.Cancel) {
		return 0;
	}

	Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);
	if (Frame->State == FRAME_REPORTING) {
		ApsCleanReport();
	}

	//
	// Create profile object
	//

	if (Context.Attach) {
		Status = CtlPaneCreateProfileByAttach(Frame->hWndCtlPane, &Context, ReportPath);
	}
	else {
		Status = CtlPaneCreateProfileByLaunch(Frame->hWndCtlPane, &Context, ReportPath);
	}

	if (Status != APS_STATUS_OK) {
		MessageBox(hWnd, L"Failed to create profile!", L"D Profile", MB_OK|MB_ICONERROR);
		return Status;
	}

    //
    // Start profile
    //

	CtlPaneStartProfile(Frame->hWndCtlPane);

	FrameCleanForm(Frame);
	FrameCloseReport(Frame);
    Frame->State = FRAME_PROFILING;
    FrameSetMenuState(Frame);

	//
	// Copy report path
	//

	StringCchCopy(Frame->FilePath, MAX_PATH, ReportPath);
	FrameShowCtlPane(Frame, TRUE);
	return 0;
}

VOID
FrameCleanForm(
	__in PFRAME_OBJECT Object
	)
{
	ULONG i;

	//
	// Clean rebar
	//

	RebarSetType(Object->Rebar, PROFILE_NONE_TYPE);

	//
	// Clean status bar
	//

	SendMessage(Object->hWndStatusBar, SB_SETTEXT, SBT_NOBORDERS | 0, (LPARAM)L"Ready");
	if (Object->Type == PROFILE_CPU_TYPE) {
		for(i = 0; i < CPU_FORM_COUNT; i++) {
			if (Object->CpuForm.hWndForm[i] != NULL) {
				DestroyWindow(Object->CpuForm.hWndForm[i]);
				Object->CpuForm.hWndForm[i] = NULL;
			}
		}
		Object->CpuForm.Current = CPU_FORM_SUMMARY;
	}
	
	if (Object->Type == PROFILE_MM_TYPE) {
		for(i = 0; i < MM_FORM_COUNT; i++) {
			if (Object->MmForm.hWndForm[i] != NULL) {
				DestroyWindow(Object->MmForm.hWndForm[i]);
				Object->MmForm.hWndForm[i] = NULL;
			}
		}
		Object->MmForm.Current = MM_FORM_SUMMARY;
	}

	Object->hWndCurrent = NULL;

}

LRESULT
FrameOnIo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

BOOLEAN
FrameOpenReport(
	__in PFRAME_OBJECT Object,
	__in PWSTR FilePath
	)
{
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PPF_REPORT_HEAD Head;

	FileHandle = CreateFile(FilePath, GENERIC_READ|GENERIC_WRITE, 
		                    FILE_SHARE_READ|FILE_SHARE_WRITE,
			                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		return FALSE;
	}

	Head = (PPF_REPORT_HEAD)MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Head) {
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		return FALSE;
	}

	//
	// N.B. Must clear its context as it's dirty data
	//

	Head->Context = NULL;

	Object->FileHandle = FileHandle;
	Object->MappingHandle = MappingHandle;
	Object->Head = Head;
	StringCchCopy(Object->FilePath, MAX_PATH, FilePath);

    if (Head->IncludeCpu) {
        Object->Type = PROFILE_CPU_TYPE; 
    }
    else if (Head->IncludeMm) {
        Object->Type = PROFILE_MM_TYPE; 
    }
    else if (Head->IncludeIo) {
        Object->Type = PROFILE_IO_TYPE; 
    }
    else if (Head->IncludeCcr) {
        Object->Type = PROFILE_CCR_TYPE; 
    }
    else {
        ASSERT(0);
    }

	//
	// Report depend on internal states, we need initialize
	// the report context here before navigate the report.
	//

	ApsInitReportContext(Head);

    Object->State = FRAME_REPORTING;
	return TRUE;
}

VOID
FrameCloseReport(
	__in PFRAME_OBJECT Object
	)
{
	if (Object->Head != NULL) {
		UnmapViewOfFile(Object->Head);
		Object->Head = NULL;
	}

	if (Object->MappingHandle != NULL) {
		CloseHandle(Object->MappingHandle);
		Object->MappingHandle = NULL;
	}

	if (Object->FileHandle != NULL) {
		CloseHandle(Object->FileHandle);
		Object->FileHandle = NULL;
	}
}

VOID
FrameGetPaneRect(
	__in PFRAME_OBJECT Object,
	__out RECT *rcView
	)
{
	RECT rcClient;
	RECT rcRebar = {0};
    RECT rcStatusBar = {0};

    GetClientRect(Object->hWnd, &rcClient);
    
    if (Object->hWndStatusBar) {
        GetWindowRect(Object->hWndStatusBar, &rcStatusBar);
    }
	
    if (Object->Rebar != NULL) {
		GetWindowRect(Object->Rebar->hWndRebar, &rcRebar);
    }
	
	rcView->top    = rcClient.top + (rcRebar.bottom - rcRebar.top);
	rcView->left   = rcClient.left;
	rcView->right  = rcClient.right;
    rcView->bottom = rcClient.bottom;

	if ((rcView->bottom <= rcView->top) || (rcView->right <= rcView->left)){
        rcView->top = 0;
		rcView->left = 0;
		rcView->right = 0;
		rcView->left = 0;    
    }
}
 
VOID
FrameSetMenuState(
    __in PFRAME_OBJECT Object
    )
{
    HMENU hMenu;
    HMENU hSubMenu;

    hMenu = GetMenu(Object->hWnd);
    
    switch (Object->State) {

    case FRAME_BOOTSTRAP:
        hSubMenu = GetSubMenu(hMenu, 0);
        EnableMenuItem(hSubMenu, IDM_OPEN, MF_ENABLED);
        EnableMenuItem(hSubMenu, IDM_SAVE, MF_ENABLED);
        hSubMenu = GetSubMenu(hMenu, 1);
        EnableMenuItem(hSubMenu, IDM_CPU, MF_ENABLED);
        EnableMenuItem(hSubMenu, IDM_MM, MF_ENABLED);
        hSubMenu = GetSubMenu(hMenu, 2);
        EnableMenuItem(hSubMenu, IDM_ANALYZE, MF_DISABLED|MF_GRAYED);    
        break;

    case FRAME_PROFILING:
    case FRAME_ANALYZING:
        hSubMenu = GetSubMenu(hMenu, 0);
        EnableMenuItem(hSubMenu, IDM_OPEN, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, IDM_SAVE, MF_DISABLED|MF_GRAYED);
        hSubMenu = GetSubMenu(hMenu, 1);
        EnableMenuItem(hSubMenu, IDM_CPU, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, IDM_MM, MF_DISABLED|MF_GRAYED);
        hSubMenu = GetSubMenu(hMenu, 2);
        EnableMenuItem(hSubMenu, IDM_ANALYZE, MF_DISABLED|MF_GRAYED);
        break;

    case FRAME_REPORTING:
        hSubMenu = GetSubMenu(hMenu, 0);
        EnableMenuItem(hSubMenu, IDM_OPEN, MF_ENABLED);
        EnableMenuItem(hSubMenu, IDM_SAVE, MF_ENABLED);
        hSubMenu = GetSubMenu(hMenu, 1);
        EnableMenuItem(hSubMenu, IDM_CPU, MF_ENABLED);
        EnableMenuItem(hSubMenu, IDM_MM, MF_ENABLED);
        hSubMenu = GetSubMenu(hMenu, 2);
        EnableMenuItem(hSubMenu, IDM_ANALYZE, MF_ENABLED);    
        break;

    case FRAME_TERMINATED:
        hSubMenu = GetSubMenu(hMenu, 0);
        EnableMenuItem(hSubMenu, IDM_OPEN, MF_ENABLED);
        EnableMenuItem(hSubMenu, IDM_SAVE, MF_ENABLED);
        hSubMenu = GetSubMenu(hMenu, 1);
        EnableMenuItem(hSubMenu, IDM_CPU, MF_ENABLED);
        EnableMenuItem(hSubMenu, IDM_MM, MF_ENABLED);
        hSubMenu = GetSubMenu(hMenu, 2);
        EnableMenuItem(hSubMenu, IDM_ANALYZE, MF_DISABLED|MF_GRAYED);    
        break;

    default:
        ASSERT(0);
    }
}

LRESULT
FrameOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	LPNMHDR lpNmhdr;

	lpNmhdr = (LPNMHDR)lp;

	switch (lpNmhdr->code) {
		case TTN_GETDISPINFO:
			FrameOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
FrameOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;
	lpnmtdi->uFlags = TTF_DI_SETITEM;

	switch (lpnmtdi->hdr.idFrom) {
		case IDM_FIND_BACKWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Find Backward");
			break;
		case IDM_FIND_FORWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Find Forward");
			break;
		case IDM_DEDUCTION:
			StringCchCopy(lpnmtdi->szText, 80, L"Noise Deduction");
			break;
	}

	return 0L;
}

LRESULT
FrameOnFindForward(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FrameOnFindBackward(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
FrameOnDeduction(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	DeductionDialog(hWnd);
	return 0;
}

VOID
FrameRetrieveFindString(
	__in PFRAME_OBJECT Object,
	__out PWCHAR Buffer,
	__in ULONG Length
	)
{
	PREBAR_OBJECT Rebar;

	Rebar = Object->Rebar;
	RebarCurrentEditString(Rebar, Buffer, Length);
}

LRESULT
FrameOnDeductionUpdate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFRAME_OBJECT Frame;
    
	Frame = (PFRAME_OBJECT)SdkGetObject(hWnd);

	//
	// Currently, only CPU calltree or MM calltree support
	// noise deduction.
	//

	if (Frame->CpuForm.hWndForm[CPU_FORM_CALLTREE]) {
		CpuTreeOnDeduction(Frame->CpuForm.hWndForm[CPU_FORM_CALLTREE]);
	}

	if (Frame->MmForm.hWndForm[MM_FORM_CALLTREE]) {
		MmTreeOnDeduction(Frame->MmForm.hWndForm[MM_FORM_CALLTREE]);
	}

	return 0;
}

VOID
FrameShowSource(
	__in HWND hWndFrom,
	__in PWSTR FullPath,
	__in ULONG Line
	)
{
	PFRAME_OBJECT Object;
	HWND hWndSource;
	HANDLE Handle;
	PWSTR RealPath;
	PWSTR Folder;
	PMDB_DATA_ITEM Item;

	Object = MainGetFrame();
	ASSERT(Object != NULL);

	//
	// Get source code folder
	//

	Item = MdbGetData(MdbSourcePath);
	ASSERT(Item != NULL);

	ApsConvertAnsiToUnicode(Item->Value, &Folder);

	//
	// Open the source file, note that folder can be NULL
	//

	//
	// N.B. ApsOpenSourceFile internally call SearchTreeForFileW(),
	// this requires that dbghelp is initialized.
	//

	SymInitialize(GetCurrentProcess(), NULL, FALSE);

	RealPath = NULL;
	Handle = ApsOpenSourceFile(Folder, FullPath, &RealPath);
	if (Handle != INVALID_HANDLE_VALUE) {
		ASSERT(RealPath != NULL);
		CloseHandle(Handle);
	}
	else {
		MessageBox(Object->hWnd, L"Failed to find source file!", L"D Profile",
			       MB_OK|MB_ICONERROR);
		SymCleanup(GetCurrentProcess());
		return;
	}

	if (!Object->SourceForm) {

		//
		// Create modeless source dialog, don't set owner!
		//

		hWndSource = SourceCreate(NULL, IDD_DIALOG_SOURCE);
		ASSERT(hWndSource != NULL);

		Object->SourceForm = (PDIALOG_OBJECT)SdkGetObject(hWndSource);
		ASSERT(Object->SourceForm != NULL);

	} else {
		hWndSource = Object->SourceForm->hWnd;
	}

	//
	// Enforce the source window to show
	//

	ShowWindow(hWndSource, SW_RESTORE);
	SetFocus(hWndSource);

	//
	// Show the file in source dialog
	//

	SourceOpenFile(hWndSource, RealPath, Line);

	if (Folder) {
		ApsFree(Folder);
	}

	if (RealPath) {
		ApsFree(RealPath);
	}
	
	SymCleanup(GetCurrentProcess());
}

LRESULT
FrameOnSourceClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	PFRAME_OBJECT Object;

	Object = MainGetFrame();
	ASSERT(Object->SourceForm != NULL);

	SdkFree(Object->SourceForm->Context);
	SdkFree(Object->SourceForm);
	Object->SourceForm = NULL;
	return 0;
}