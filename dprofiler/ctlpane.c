//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "dprofiler.h"
#include "ctlpane.h"
#include "apscpu.h"
#include "wizard.h"
#include "frame.h"
#include "apsrpt.h"
#include "apspdb.h"
#include "apsanalysis.h"
#include "util.h"
#include "apslog.h"


typedef struct _CTL_PANE_CONTEXT {

    UINT CtrlId;
	BOOLEAN Pause;
	UINT_PTR TimerId;
	HFONT hLargeFont;
	HFONT hSmallFont;
    HBRUSH hBrushPane;

	CTL_PANE_STATE State;
    PAPS_PROFILE_OBJECT Profile;

    APS_ANALYSIS_CONTEXT Analysis;

	//
	// N.B. The following copy from profile
	// object, after profile stops, we may
	// still need access this fields
	//

	BTR_PROFILE_TYPE Type;
	ULONG ProcessId;
	WCHAR ProcessPath[MAX_PATH];
	WCHAR ReportPath[MAX_PATH];

    PPF_REPORT_HEAD Head;

} CTL_PANE_CONTEXT, *PCTL_PANE_CONTEXT;

DIALOG_SCALER_CHILD CtlPaneChildren[] = {
	{ IDC_STATIC, AlignRight, AlignBottom },
	{ IDC_ANIMATE, AlignBoth, AlignNone },
	{ IDC_BUTTON_PAUSE_RESUME, AlignBoth, AlignBoth }, 
    { IDC_BUTTON_MARK, AlignBoth, AlignBoth },
	{ IDC_BUTTON_STOP, AlignBoth, AlignBoth }
};

DIALOG_SCALER CtlPaneScaler = {
	{0,0}, {0,0}, {0,0}, 5, CtlPaneChildren
};


HWND
CtlPaneCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	)
{
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	HWND hWnd;
	
	Context = (PCTL_PANE_CONTEXT)SdkMalloc(sizeof(CTL_PANE_CONTEXT));
	Context->CtrlId = CtrlId;
	Context->State = CTL_PANE_BOOTSTRAP;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Context;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_FORMVIEW_CTL;
	Object->Procedure = CtlPaneProcedure;

	hWnd = DialogCreateModeless(Object);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT
CtlPaneOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	WCHAR Buffer[MAX_PATH];
	HFONT hFont;
	LOGFONT LogFont;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

    //
    // Create background brush 
    //

    Context->hBrushPane = CreateSolidBrush(RGB(255, 255, 255));
	ASSERT(Context->hBrushPane != NULL);

	//
	// Create fonts to draw text
	//

	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	GetObject(hFont, sizeof(LOGFONT), &LogFont);
	LogFont.lfWeight = FW_BOLD;
	LogFont.lfHeight = 16;

	Context->hLargeFont = CreateFontIndirect(&LogFont);
	ASSERT(Context->hLargeFont != NULL);

	LogFont.lfWeight = FW_NORMAL;
	LogFont.lfHeight = 12;
	Context->hSmallFont = CreateFontIndirect(&LogFont);
	ASSERT(Context->hSmallFont != NULL);

	//
	// Clear static control
	//

    Buffer[0] = 0;
	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC);
	SetWindowText(hWndCtrl, Buffer);

	//
	// N.B. Subclass static ctrl and take over its drawing
	//

	SetWindowSubclass(hWndCtrl, CtlPaneStaticProcedure, 0, (DWORD_PTR)Object);

	SetWindowText(hWnd, L"D Profile");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);

    //
	// Register dialog scaler
	//

	Object->Scaler = &CtlPaneScaler;
	DialogRegisterScaler(Object);

	return TRUE;
}

LRESULT
CtlPaneOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
    RECT Rect;

    GetClientRect(hWnd, &Rect);
    InvalidateRect(hWnd, &Rect, TRUE);
	return 0;
}

LRESULT
CtlPaneOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	return 0;
}

INT_PTR CALLBACK
CtlPaneProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	switch (uMsg) {

	case WM_INITDIALOG:
        return CtlPaneOnInitDialog(hWnd, uMsg, wp, lp);

	case WM_ERASEBKGND:
		return CtlPaneOnEraseBkgnd(hWnd, uMsg, wp, lp);

    case WM_CTLCOLORSTATIC:
		return CtlPaneOnCtlColorStatic(hWnd, uMsg, wp, lp);

    case WM_SIZE:
        return CtlPaneOnSize(hWnd, uMsg, wp, lp);

    case WM_COMMAND:
		return CtlPaneOnCommand(hWnd, uMsg, wp, lp);		

	case WM_USER_PROFILE_START:
		return CtlPaneOnProfileStart(hWnd, uMsg, wp, lp);

	case WM_USER_PROFILE_TERMINATE:
		return CtlPaneOnProfileTerminate(hWnd, uMsg, wp, lp);

    case WM_USER_PROFILE_ANALYZED:
        return CtlPaneOnProfileAnalyzed(hWnd, uMsg, wp, lp); 

    case WM_USER_PROFILE_OPENED:
        return CtlPaneOnProfileOpened(hWnd, uMsg, wp, lp); 

	case WM_NOTIFY: 
		break;

	}

	return 0;
}

LRESULT
CtlPaneOnEraseBkgnd(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	RECT Rect;
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	HDC hdc;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

	GetClientRect(hWnd, &Rect);
	hdc = GetDC(hWnd);
	FillRect(hdc, &Rect, Context->hBrushPane);
	ReleaseDC(hWnd, hdc);
	return TRUE;
}

LRESULT
CtlPaneOnCtlColorStatic(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;
	return (LRESULT)Context->hBrushPane;
}

LRESULT
CtlPaneOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

LRESULT
CtlPaneOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	)
{
	return 0;
}

LRESULT
CtlPaneOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	return 0;
}

ULONG
CtlPaneProfileCallback(
	__in PAPS_PROFILE_OBJECT Object,
	__in PVOID Context,
	__in ULONG Status
	)
{
	HWND hWnd;

	hWnd = (HWND)Context;
	PostMessage(hWnd, WM_USER_PROFILE_TERMINATE, (WPARAM)Status, 0);
	return APS_STATUS_OK;
}

ULONG
CtlPaneAnalysisCallback(
    __in PVOID Argument
    )
{
    PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    ULONG Status;

    Object = (PDIALOG_OBJECT)Argument;
    Context = (PCTL_PANE_CONTEXT)Object->Context;

    //
    // Free allocated strings
    //

    if (Context->Analysis.ReportPath) {
        ApsFree(Context->Analysis.ReportPath);
        Context->Analysis.ReportPath = NULL;
    }

    if (Context->Analysis.SymbolPath) {
        //ApsFree(Context->Analysis.SymbolPath);
        Context->Analysis.SymbolPath = NULL;
    }

    //
    // Notify the ctrl pane that the analysis is completed along with
    // its completion status
    //

    Status = Context->Analysis.Callback.Status;
    PostMessage(Object->hWnd, WM_USER_PROFILE_ANALYZED, Status, 0);
    return 0;
}

ULONG
CtlPaneOpenCallback(
    __in PVOID Argument
    )
{
    PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    ULONG Status;

    Object = (PDIALOG_OBJECT)Argument;
    Context = (PCTL_PANE_CONTEXT)Object->Context;

    //
    // Free allocated strings
    //

    if (Context->Analysis.ReportPath) {
        ApsFree(Context->Analysis.ReportPath);
        Context->Analysis.ReportPath = NULL;
    }

    //
    // Notify the ctrl pane that the open is completed along with
    // its completion status
    //

    Status = Context->Analysis.Callback.Status;
    PostMessage(Object->hWnd, WM_USER_PROFILE_OPENED, Status, 0);
    return 0;
}

LRESULT CALLBACK 
CtlPaneStaticProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	)
{
	switch (uMsg) {

	case WM_PAINT:

		//
		// dialog object ptr is passed as LPARAM
		//

		return CtlPaneStaticOnPaint(hWnd, uMsg, wp, (LPARAM)dwData);

	case WM_SIZE:
		return CtlPaneStaticOnSize(hWnd, uMsg, wp, lp);
	}

    return DefSubclassProc(hWnd, uMsg, wp, lp);
} 

LRESULT 
CtlPaneStaticOnSize(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	RECT Rect;

	GetClientRect(hWnd, &Rect);
	InvalidateRect(hWnd, &Rect, TRUE);
	return 0;
}

LRESULT 
CtlPaneStaticOnPaint(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	RECT Rect;
	HDC hdc;
	PAINTSTRUCT Paint;

	Object = (PDIALOG_OBJECT)lp;
	Context = (PCTL_PANE_CONTEXT)Object->Context;

	GetClientRect(hWnd, &Rect);

	hdc = BeginPaint(hWnd, &Paint);

	FillRect(hdc, &Rect, Context->hBrushPane);

	switch (Context->State) {

    case CTL_PANE_BOOTSTRAP:
		CtlPaneDrawBootstrap(hWnd, hdc, &Rect, Object);
        break;

    case CTL_PANE_PROFILING:
		CtlPaneDrawProfiling(hWnd, hdc, &Rect, Object);
        break;

	case CTL_PANE_INIT_PAUSED:
	case CTL_PANE_PAUSED:
		CtlPaneDrawPaused(hWnd, hdc, &Rect, Object);
		break;

    case CTL_PANE_COMPLETED:
        CtlPaneDrawCompleted(hWnd, hdc, &Rect, Object);
        break;

    case CTL_PANE_ANALYZING:
		CtlPaneDrawAnalyzing(hWnd, hdc, &Rect, Object);
        break;

    case CTL_PANE_TERMINATED:
		CtlPaneDrawTerminated(hWnd, hdc, &Rect, Object);
        break;

    case CTL_PANE_RESUME_FAILED:
		CtlPaneDrawResumeFailed(hWnd, hdc, &Rect, Object);
        break;

    case CTL_PANE_FAILED:
		CtlPaneDrawFailed(hWnd, hdc, &Rect, Object);
        break;

    default:
		ASSERT(0);

	}

    EndPaint(hWnd, &Paint);
	return 0;
} 

CTL_PANE_STATE
CtlPaneGetCurrentState(
	__in HWND hWnd
	)
{
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;
	return Context->State;
}

VOID
CtlPaneDrawBootstrap(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	PWSTR WelcomeText = L"Welcome to D Profile !";
	PWSTR QuickStartText = L"\r\nTo start CPU profiling,use:\r\nProfile -> CPU\r\n"
						   L"\r\nTo start Memory profiling,use:\r\nProfile -> Memory\r\n"
						   L"\r\nTo analyze report,use:\r\nFile -> Open.";
	HFONT hOldFont;
	HBRUSH hBrush;
	RECT TextRect;
	RECT LineRect;
	SIZE Size;
    ULONG Count;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	GetClientRect(hWnd, &TextRect);

	//
	// Display the text start from (20, 20)
	//

	TextRect.top += Rect->top + 20;
	TextRect.left += Rect->left + 20;
    
	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, WelcomeText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	
	Count = (ULONG)wcslen(WelcomeText);
	GetTextExtentPoint32(hdc, WelcomeText, Count, &Size);
	SelectObject(hdc, hOldFont);

	//
	// Draw underline width 3 
	//
	

	hBrush = CreateSolidBrush(RGB(0x00, 0x00, 0x00));
	LineRect.left = TextRect.left;
	LineRect.top  = TextRect.top + Size.cy + 2;
	LineRect.right = TextRect.left + Size.cx;
	LineRect.bottom = LineRect.top + 3;
	FillRect(hdc, &LineRect, hBrush);

	DeleteObject(hBrush);

	//
	// draw help text
	//
	
	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
	TextRect.top = LineRect.bottom;
	DrawText(hdc, QuickStartText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);

	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawProfiling(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	RECT TextRect;
	PCTL_PANE_CONTEXT Context;
	HFONT hOldFont;
	HWND hWndCtrl;
	int AnimateWidth;
	int AnimateHeight;
	RECT AnimateRect;
	SIZE Size;
    ULONG Count;
	PWSTR ProfilingText = L"Profiling ...";
	PWSTR HintText = L"D Profile is collecting data. This may take few moments.\r\n"
		             L"Closing this window will cancel this profiling session.";

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	TextRect = *Rect;
    
    Count = (ULONG)wcslen(ProfilingText);
	GetTextExtentPoint32(hdc, ProfilingText, Count, &Size);

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_ANIMATE);
	GetWindowRect(hWndCtrl, &AnimateRect);
	MapWindowPoints(GetDesktopWindow(), Object->hWnd, (LPPOINT)&AnimateRect, 2);

	AnimateWidth = AnimateRect.right - AnimateRect.left;
	AnimateHeight = AnimateRect.bottom - AnimateRect.top;

	//
	// Center the animate control
	//

	MoveWindow(hWndCtrl, Rect->left + 20, Rect->top + 20, 
		       AnimateWidth, AnimateHeight, TRUE);

	TextRect.top = Rect->top + 20 + AnimateHeight / 2 - Size.cy / 2;
	TextRect.left = Rect->left + 20 + AnimateWidth + AnimateWidth / 2;
	TextRect.bottom = Rect->bottom;
	TextRect.right = Rect->right;

	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, ProfilingText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	SelectObject(hdc, hOldFont);
	
	TextRect.top = TextRect.top + Size.cy * 2;
	
	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
	DrawText(hdc, HintText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);

	// 
	// Start animation
	//

	CtlPaneStartAnimation(Object);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawPaused(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	PWSTR PausedText = L"Paused";
	PWSTR HintText = L"Profile is currently paused, press Resume button to continue.";
	HFONT hOldFont;
	RECT TextRect;
	SIZE Size;
    ULONG Count;
	HWND hWndCtrl;
	int AnimateWidth;
	int AnimateHeight;
	RECT AnimateRect;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	TextRect = *Rect;
    
    Count = (ULONG)wcslen(PausedText);
	GetTextExtentPoint32(hdc, PausedText, Count, &Size);

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_ANIMATE);
	GetWindowRect(hWndCtrl, &AnimateRect);
	MapWindowPoints(GetDesktopWindow(), Object->hWnd, (LPPOINT)&AnimateRect, 2);

	AnimateWidth = AnimateRect.right - AnimateRect.left;
	AnimateHeight = AnimateRect.bottom - AnimateRect.top;

	//
	// Center the animate control
	//

	MoveWindow(hWndCtrl, Rect->left + 20, Rect->top + 20, 
		       AnimateWidth, AnimateHeight, TRUE);

	//
	// Draw paused text
	//

	TextRect.top = Rect->top + 20 + AnimateHeight / 2 - Size.cy / 2;
	TextRect.left = Rect->left + 20 + AnimateWidth + AnimateWidth / 2;
	TextRect.bottom = Rect->bottom;
	TextRect.right = Rect->right;

	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, PausedText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	SelectObject(hdc, hOldFont);
	
	//
	// Draw hint text
	//

	TextRect.top = TextRect.top + Size.cy * 2;
	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
	DrawText(hdc, HintText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);

	CtlPaneShowAnimation(Object, TRUE);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawCompleted(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	PWSTR CompleteText = L"Completed";
	PWSTR ProfileText = L"\r\nTo start CPU profiling,use:\r\nProfile -> CPU\r\n"
						L"\r\nTo start Memory profiling,use:\r\nProfile -> Memory\r\n"
						L"\r\nTo analyze report,use:\r\nFile -> Open.";
    WCHAR ReportText[MAX_PATH];
	HFONT hOldFont;
	RECT TextRect;
	SIZE Size;
    ULONG Count;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	GetClientRect(hWnd, &TextRect);

	//
	// Display the text start from (20, 20)
	//

	TextRect.top += Rect->top + 20;
	TextRect.left += Rect->left + 20;
    
	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, CompleteText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	
    Count = (ULONG)wcslen(CompleteText);
	GetTextExtentPoint32(hdc, CompleteText, Count, &Size);
	SelectObject(hdc, hOldFont);

	//
	// Draw report path
    //
	
    StringCchPrintf(ReportText, MAX_PATH, L"Report path: %s", Context->ReportPath);

	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
    TextRect.top = TextRect.top + Size.cy * 2;
	DrawText(hdc, ReportText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	//
	// Draw profile text
	//
	
    TextRect.top = TextRect.top + Size.cy;
	DrawText(hdc, ProfileText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawTerminated(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	PWSTR TerminatedText = L"Aborted";
    PWSTR ProfileText = L"\r\nTo start CPU profiling,use:\r\nProfile -> CPU\r\n"
						L"\r\nTo start Memory profiling,use:\r\nProfile -> Memory\r\n"
						L"\r\nTo analyze report,use:\r\nFile -> Open.";
	PWSTR MessageText;
    WCHAR BaseName[MAX_PATH];
    WCHAR Extension[MAX_PATH];
	HFONT hOldFont;
	RECT TextRect;
	SIZE Size;
    ULONG Count;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	GetClientRect(hWnd, &TextRect);

	//
	// Display the text start from (20, 20)
	//

	TextRect.top += Rect->top + 20;
	TextRect.left += Rect->left + 20;
    
	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, TerminatedText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	
    Count = (ULONG)wcslen(TerminatedText);
	GetTextExtentPoint32(hdc, TerminatedText, Count, &Size);
	SelectObject(hdc, hOldFont);

	//
	// Draw terminated message 
    //
	
    BaseName[0] = 0;
    Extension[0] = 0;
    _wsplitpath(Context->ProcessPath, NULL, NULL, BaseName, Extension);

    MessageText = (PWSTR)ApsMalloc(APS_PAGESIZE * 2);

    if (Extension[0] != 0) {
        StringCchPrintf(MessageText, MAX_PATH, L"%s.%s (%u) was terminated unexpectedly, no data collected.", 
                        BaseName, Extension, Context->ProcessId);
    } else {
        StringCchPrintf(MessageText, MAX_PATH, L"%s (%u) was terminated unexpectedly, no data collected.",
                        BaseName, Context->ProcessId);
    }

    SetTextColor(hdc, RGB(255, 0, 0));
	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
    TextRect.top = TextRect.top + Size.cy * 2;
	DrawText(hdc, MessageText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

    ApsFree(MessageText);

    SetTextColor(hdc, RGB(0, 0, 0));
    TextRect.top = TextRect.top + Size.cy;
    DrawText(hdc, ProfileText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawFailed(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	PWSTR FailedText = L"Failed";
    PWSTR ProfileText = L"\r\nTo start CPU profiling,use:\r\nProfile -> CPU\r\n"
						L"\r\nTo start Memory profiling,use:\r\nProfile -> Memory\r\n"
						L"\r\nTo analyze report,use:\r\nFile -> Open.";
	PWSTR MessageText;
    WCHAR BaseName[MAX_PATH];
    WCHAR Extension[MAX_PATH];
	HFONT hOldFont;
	RECT TextRect;
	SIZE Size;
    ULONG Count;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	GetClientRect(hWnd, &TextRect);

	//
	// Display the text start from (20, 20)
	//

	TextRect.top += Rect->top + 20;
	TextRect.left += Rect->left + 20;
    
	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, FailedText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	
    Count = (ULONG)wcslen(FailedText);
	GetTextExtentPoint32(hdc, FailedText, Count, &Size);
	SelectObject(hdc, hOldFont);

	//
	// Draw terminated message 
    //
	
    BaseName[0] = 0;
    Extension[0] = 0;
    _wsplitpath(Context->ProcessPath, NULL, NULL, BaseName, Extension);

    MessageText = (PWSTR)ApsMalloc(APS_PAGESIZE * 2);

    if (Extension[0] != 0) {
        StringCchPrintf(MessageText, MAX_PATH, L"Failed to analyze %s.%s (%u).", 
                        BaseName, Extension, Context->ProcessId);
    } else {
        StringCchPrintf(MessageText, MAX_PATH, L"Failed to analyze %s (%u).",
                        BaseName, Context->ProcessId);
    }

    SetTextColor(hdc, RGB(255, 0, 0));
	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
    TextRect.top = TextRect.top + Size.cy * 2;
	DrawText(hdc, MessageText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

    ApsFree(MessageText);

    SetTextColor(hdc, RGB(0, 0, 0));
    TextRect.top = TextRect.top + Size.cy;
    DrawText(hdc, ProfileText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawResumeFailed(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	PWSTR ResumeText = L"Resume Failed";
    PWSTR ProfileText = L"\r\nTo start CPU profiling,use:\r\nProfile -> CPU\r\n"
						L"\r\nTo start Memory profiling,use:\r\nProfile -> Memory\r\n"
						L"\r\nTo analyze report,use:\r\nFile -> Open.";
	PWSTR MessageText;
    WCHAR BaseName[MAX_PATH];
    WCHAR Extension[MAX_PATH];
	HFONT hOldFont;
	RECT TextRect;
	SIZE Size;
    ULONG Count;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	GetClientRect(hWnd, &TextRect);

	//
	// Display the text start from (20, 20)
	//

	TextRect.top += Rect->top + 20;
	TextRect.left += Rect->left + 20;
    
	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, ResumeText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	
    Count = (ULONG)wcslen(ResumeText);
	GetTextExtentPoint32(hdc, ResumeText, Count, &Size);
	SelectObject(hdc, hOldFont);

	//
	// Draw terminated message 
    //
	
    BaseName[0] = 0;
    Extension[0] = 0;
    _wsplitpath(Context->ProcessPath, NULL, NULL, BaseName, Extension);

    MessageText = (PWSTR)ApsMalloc(APS_PAGESIZE * 2);

    if (Extension[0] != 0) {
        StringCchPrintf(MessageText, MAX_PATH, L"%s.%s (%u) failed to resume profile, no data collected.", 
                        BaseName, Extension, Context->ProcessId);
    } else {
        StringCchPrintf(MessageText, MAX_PATH, L"%s (%u) failed to resume profile, no data collected.",
                        BaseName, Context->ProcessId);
    }

    SetTextColor(hdc, RGB(255, 0, 0));
	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
    TextRect.top = TextRect.top + Size.cy * 2;
	DrawText(hdc, MessageText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

    ApsFree(MessageText);

    SetTextColor(hdc, RGB(0, 0, 0));
    TextRect.top = TextRect.top + Size.cy;
    DrawText(hdc, ProfileText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneDrawAnalyzing(
	__in HWND hWnd,
	__in HDC hdc,
	__in const LPRECT Rect,
	__in PDIALOG_OBJECT Object
	)
{
    PCTL_PANE_CONTEXT Context;
	PWSTR AnalyzingText = L"Analyzing ...";
	WCHAR ReportText[MAX_PATH];
    PWSTR MessageText = L"Loading symbols ...";
	HWND hWndCtrl;
	int AnimateWidth;
	int AnimateHeight;
	RECT AnimateRect;
    HFONT hOldFont;
	SIZE Size;
    RECT TextRect;
    ULONG Count;

	Context = (PCTL_PANE_CONTEXT)Object->Context;

	TextRect = *Rect;
    
    Count = (ULONG)wcslen(AnalyzingText);
	GetTextExtentPoint32(hdc, AnalyzingText, Count, &Size);

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_ANIMATE);
	GetWindowRect(hWndCtrl, &AnimateRect);
	MapWindowPoints(GetDesktopWindow(), Object->hWnd, (LPPOINT)&AnimateRect, 2);

	AnimateWidth = AnimateRect.right - AnimateRect.left;
	AnimateHeight = AnimateRect.bottom - AnimateRect.top;

	//
	// Center the animate control
	//

	MoveWindow(hWndCtrl, Rect->left + 20, Rect->top + 20, 
		       AnimateWidth, AnimateHeight, TRUE);

    //
    // Draw analyzing text
    //

	TextRect.top = Rect->top + 20 + AnimateHeight / 2 - Size.cy / 2;
	TextRect.left = Rect->left + 20 + AnimateWidth + AnimateWidth / 2;
	TextRect.bottom = Rect->bottom;
	TextRect.right = Rect->right;

	hOldFont = (HFONT)SelectObject(hdc, Context->hLargeFont);
	DrawText(hdc, AnalyzingText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);
	SelectObject(hdc, hOldFont);
	
    //
	// Draw report path
    //
	
    StringCchPrintf(ReportText, MAX_PATH, L"%s", Context->ReportPath);
	TextRect.top = TextRect.top + Size.cy * 2;

	hOldFont = (HFONT)SelectObject(hdc, Context->hSmallFont);
	DrawText(hdc, ReportText, -1, &TextRect,DT_LEFT | DT_WORD_ELLIPSIS);

	//
	// Draw message text, message text is dynamically changed, we need
    // track the rect into object context
	//
	
    TextRect.top = TextRect.top + Size.cy;
	DrawText(hdc, MessageText, -1, &TextRect, DT_LEFT|DT_WORD_ELLIPSIS);

	SelectObject(hdc, hOldFont);

	CtlPaneStartAnimation(Object);
	CtlPaneSetButtonState(Object);
}

VOID
CtlPaneShowAnimation(
	__in PDIALOG_OBJECT Object,
	__in BOOLEAN Show
	)
{
	HWND hWndCtrl;
	int Cmd;

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_ANIMATE);

	if (Animate_IsPlaying(hWndCtrl)) {
		Animate_Stop(hWndCtrl);
		Animate_Open(hWndCtrl, MAKEINTRESOURCE(IDR_AVI3));

	} else {

		//
		// N.B. To show animation picture, it's require
		// to open it first whether or not we play it
		//

		Animate_Open(hWndCtrl, MAKEINTRESOURCE(IDR_AVI3));
	}

	Cmd = Show ? SW_SHOW : SW_HIDE;
	ShowWindow(hWndCtrl, Cmd);
}

VOID
CtlPaneStartAnimation(
	__in PDIALOG_OBJECT Object
	)
{
	HWND hWndCtrl;

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_ANIMATE);
	Animate_Open(hWndCtrl, MAKEINTRESOURCE(IDR_AVI3));
	Animate_Play(hWndCtrl, 0, -1, -1);
	ShowWindow(hWndCtrl, SW_SHOW);
}

VOID
CtlPaneStopAnimation(
	__in PDIALOG_OBJECT Object
	)
{
	HWND hWndCtrl;

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_ANIMATE);
	Animate_Stop(hWndCtrl);
	ShowWindow(hWndCtrl, SW_HIDE);
}

VOID
CtlPaneRedraw(
	__in PDIALOG_OBJECT Object
	)
{
	HWND hWndCtrl;

	//
	// N.B. Redraw only the staitc area
	//

	hWndCtrl = GetDlgItem(Object->hWnd, IDC_STATIC);
	InvalidateRect(hWndCtrl, NULL, TRUE);
}

LRESULT
CtlPaneOnProfileStart(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	PAPS_PROFILE_OBJECT Profile;
    ULONG Status;
	CHAR Buffer[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

	Profile = Context->Profile;
	ASSERT(Profile != NULL);

	//
	// Register profile status callback to get notified when
	// user stop profile, or target terminated unexpectedly
	//

	ApsRegisterCallback(Profile, CtlPaneProfileCallback, (PVOID)hWnd);
    
    if (!Profile->Attribute.Paused) {

        Status = ApsStartProfile(Profile);
        if (Status != APS_STATUS_OK) {

            MessageBox(hWnd, L"Failed to start profile!", L"D Profile", MB_OK|MB_ICONERROR);

			if (Profile->Type == PROFILE_CPU_TYPE) {
				StringCchPrintfA(Buffer, MAX_PATH, "Failed to start CPU profile for pid=%u",
								 Profile->ProcessId);
			} else {
				StringCchPrintfA(Buffer, MAX_PATH, "Failed to start MM profile for pid=%u",
								 Profile->ProcessId);
			}
			ApsWriteLogEntry(Profile->ProcessId, LogLevelFailure, Buffer);

            //
            // Notify parent window the profile start failed
            //

            PostMessage(GetParent(hWnd), WM_USER_PROFILE_START, Status, 0);
            return Status;
        }

        Context->State = CTL_PANE_PROFILING; 

		if (Profile->Type == PROFILE_CPU_TYPE) {
			StringCchPrintfA(Buffer, MAX_PATH, "Successful to start CPU profile for pid=%u",
							 Profile->ProcessId);
		} else {
			StringCchPrintfA(Buffer, MAX_PATH, "Successful to start MM profile for pid=%u",
							 Profile->ProcessId);
		}
		ApsWriteLogEntry(Profile->ProcessId, LogLevelSuccess, Buffer);

	}
    else {

        Context->State = CTL_PANE_INIT_PAUSED;
    }

	CtlPaneRedraw(Object);
    return APS_STATUS_OK;
}

ULONG
CtlPaneRunApplication(
	__in PWSTR ImagePath,
	__in PWSTR Argument,
	__in PWSTR WorkPath,
	__in BOOLEAN Suspend,
	__out PHANDLE ProcessHandle,
	__out PHANDLE ThreadHandle,
	__out PULONG ProcessId,
	__out PULONG ThreadId
	)
{
	ULONG Status;
	ULONG Action;
	STARTUPINFO StartupInfo = {0};
	PROCESS_INFORMATION ProcessInfo = {0};
	WCHAR Buffer[MAX_PATH * 2];

	ASSERT(ImagePath != NULL);

	if (!ImagePath) {
		return APS_STATUS_INVALID_PARAMETER;
	}

	//
	// If suspend is specified, thread handle can't be NULL,
	// because there's no way to conveniently resume target 
	// after this routine return
	//

	if (Suspend) {
		if (!ThreadHandle) {
			return APS_STATUS_INVALID_PARAMETER;
		}
	}

	//
	// Merge image path into command line 
	//

	if (wcslen(Argument) != 0) {
		StringCchPrintf(Buffer, MAX_PATH * 2, L"%s %s", ImagePath, Argument);
	} else {
		StringCchPrintf(Buffer, MAX_PATH * 2, L"%s", ImagePath);
	}

	//
	// Run application with suspend state
	//

	if (Suspend) {
		Action = CREATE_SUSPENDED;
	} else {
		Action = 0;
	}

	StartupInfo.cb = sizeof(StartupInfo);
	Status = CreateProcess(NULL, Buffer, NULL, 
		                   NULL, FALSE, 0, 
						   NULL, WorkPath, &StartupInfo, 
						   &ProcessInfo);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	if (ProcessHandle != NULL) {
		*ProcessHandle = ProcessInfo.hProcess;
	}

	if (ThreadHandle != NULL) {
		*ThreadHandle = ProcessInfo.hThread;
	}

	if (ProcessId != NULL) {
		*ProcessId = ProcessInfo.dwProcessId;
	}

	if (ThreadId != NULL) {
		*ThreadId = ProcessInfo.dwThreadId;
	}

	return APS_STATUS_OK;
}

LRESULT
CtlPaneOnProfileTerminate(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    PAPS_PROFILE_OBJECT Profile;
    ULONG Status;
    PWSTR Buffer;
    HANDLE ThreadHandle;
	BOOLEAN IsAutoAnalyze;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCTL_PANE_CONTEXT)Object->Context;
    Profile = Context->Profile;

	//
	// Unexpected termination
	//

	Status = (ULONG)wp;
	if (Status == APS_STATUS_TERMINATED) {

        Context->State = CTL_PANE_TERMINATED;
        PostMessage(GetParent(Object->hWnd), WM_USER_PROFILE_TERMINATED, 0, 0);
        
        CtlPaneStopAnimation(Object);
		CtlPaneRedraw(Object);
        return 0;
	}

	//
	// Normal exit
	//

	if (Status == APS_STATUS_EXITPROCESS) {

		//
		// Change pane state
		//

		IsAutoAnalyze = CtlPaneIsAutoAnalyze(Object);
		if (IsAutoAnalyze) {

			ThreadHandle = CreateThread(NULL, 0, ApsAnalysisProcedure, 
										&Context->Analysis, 
										CREATE_SUSPENDED, NULL); 
			if (!ThreadHandle) {
				ApsFailFast();
			}

			//
			// Fill analysis context and start analysis thread
			//

			ZeroMemory(&Context->Analysis, sizeof(Context->Analysis));

			Context->Analysis.Type = Context->Type;
			Context->Analysis.Option = ANALYSIS_CREATE_COUNTERS; 
			Context->Analysis.Callback.Callback = CtlPaneAnalysisCallback;
			Context->Analysis.Callback.Context = Object;

			//
			// Copy report path
			//

			Buffer = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
			StringCchCopy(Buffer, MAX_PATH, Context->ReportPath);
			Context->Analysis.ReportPath = Buffer;

			//
			// Copy symbol path
			//

			/*Buffer = (PWSTR)ApsMalloc(APS_PAGESIZE);
			ApsGetSymbolPath(Buffer, APS_PAGESIZE / sizeof(WCHAR));
			Context->Analysis.SymbolPath = Buffer;*/
			Context->Analysis.SymbolPath = UtilGetSymbolPath();

			//
			// Resume thread execution
			//

			ResumeThread(ThreadHandle);
			CloseHandle(ThreadHandle);

			Context->State = CTL_PANE_ANALYZING;

		}
		else {

			CtlPaneStopAnimation(Object);
			Context->State = CTL_PANE_COMPLETED; 

			//
			// N.B. Notify frame window the profile is completed
			//

			PostMessage(GetParent(Object->hWnd), WM_USER_PROFILE_COMPLETED, 0, 0);
		}

		//
		// Force redraw of client area
		//

		CtlPaneRedraw(Object);
	}

	else {
		ASSERT(0);
	}

	return 0;
}

LRESULT
CtlPaneOnProfileAnalyzed(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
	ULONG Status = (ULONG)wp;
    
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

    //
    // N.B. This routine is called when analysis is completed
    // notify frame window that switch to appropriate form, 
    // the ctrl pane should hide from now on
    //

	if (Status != APS_STATUS_OK) {
		Context->State = CTL_PANE_FAILED;
		CtlPaneStopAnimation(Object);
		CtlPaneRedraw(Object);
	}

    PostMessage(GetParent(hWnd), WM_USER_PROFILE_ANALYZED, wp, 0);
    return 0;
}

LRESULT
CtlPaneOnProfileOpened(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

    //
    // N.B. This routine is called when analysis is completed
    // notify frame window that switch to appropriate form, 
    // the ctrl pane should hide from now on
    //

    PostMessage(GetParent(hWnd), WM_USER_PROFILE_OPENED, wp, 0);
    return 0;
}

VOID
CtlPaneStartProfile(
	__in HWND hWndCtlPane
	)
{
	PostMessage(hWndCtlPane, WM_USER_PROFILE_START, 0, 0);
}

VOID
CtlPaneStartProfilePaused(
	__in HWND hWndCtlPane
	)
{

}

ULONG
CtlPaneEnterAnalyzing(
    __in PFRAME_OBJECT Object
    )
{
    return 0;
}

ULONG
CtlPaneEnterReporting(
    __in PFRAME_OBJECT Object
    )
{
    return 0;
}

LRESULT
CtlPaneOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	switch (LOWORD(wp)) {

		case IDC_BUTTON_PAUSE_RESUME:
			CtlPaneOnPauseResume(hWnd, uMsg, wp, lp);
			break;

		case IDC_BUTTON_STOP:
			CtlPaneOnStop(hWnd, uMsg, wp, lp);
			break;

		case IDC_BUTTON_MARK:
			CtlPaneOnMark(hWnd, uMsg, wp, lp);
			break;
	}

	return 0;
}

LRESULT
CtlPaneOnPauseResume(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    PAPS_PROFILE_OBJECT Profile;
    ULONG Status;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCTL_PANE_CONTEXT)Object->Context;
    Profile = Context->Profile;

	if (Context->State == CTL_PANE_INIT_PAUSED) {

		//
		// User request to get started to profile
		//

		Status = ApsStartProfile(Profile);
		if (Status != APS_STATUS_OK) {

			MessageBox(hWnd, L"Failed to resume paused profile!", L"D Profile", MB_OK|MB_ICONERROR);

            //
            // Notify frame window to update menu state
            //

            PostMessage(GetParent(hWnd), WM_USER_PROFILE_START, Status, 0);
            Context->State = CTL_PANE_RESUME_FAILED;
		}
        else {
            Context->State = CTL_PANE_PROFILING;
        }
	}

	else if (Context->State == CTL_PANE_PAUSED) {

		//
		// User request to resume paused profile
		//

		Status = ApsResumeProfile(Profile);
		if (Status != APS_STATUS_OK) {
			MessageBox(hWnd, L"Failed to resume paused profile!", 
				       L"D Profile", MB_OK|MB_ICONERROR);
			return Status;
		}

		Context->State = CTL_PANE_PROFILING;
	}

	else if (Context->State == CTL_PANE_PROFILING) {

		//
		// User request to pause profile
		// 
		
		Status = ApsPauseProfile(Profile);
		if (Status != APS_STATUS_OK) {
			MessageBox(hWnd, L"Failed to pause profile!", 
				       L"D Profile", MB_OK|MB_ICONERROR);
			return Status;
		}

		Context->State = CTL_PANE_PAUSED;
	}

	else {
		ASSERT(0);
	}

	CtlPaneRedraw(Object);
	return 0;
}

LRESULT
CtlPaneOnStop(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
    PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    PAPS_PROFILE_OBJECT Profile;
    ULONG Status;
    PWSTR Buffer;
    HANDLE ThreadHandle;
	BOOLEAN IsAutoAnalyze;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCTL_PANE_CONTEXT)Object->Context;
    Profile = Context->Profile;

    //
    // Stop profile, don't touch profile object
	// any more since it's freed!
    //

    Status = ApsStopProfile(Profile);
    if (Status != APS_STATUS_UNLOAD) {
        
        //
        // N.B. stop failure is ignored currently
        // we will continue to analyze profile report anyway
        //

    }
    
	Context->Profile = NULL;

    //
    // Change pane state
    //

    IsAutoAnalyze = CtlPaneIsAutoAnalyze(Object);
    if (IsAutoAnalyze) {

        ThreadHandle = CreateThread(NULL, 0, ApsAnalysisProcedure, 
                                    &Context->Analysis, 
                                    CREATE_SUSPENDED, NULL); 
        if (!ThreadHandle) {
			ApsFailFast();
        }

        //
        // Fill analysis context and start analysis thread
        //

        ZeroMemory(&Context->Analysis, sizeof(Context->Analysis));

        Context->Analysis.Type = Context->Type;
        Context->Analysis.Option = ANALYSIS_CREATE_COUNTERS; 
        Context->Analysis.Callback.Callback = CtlPaneAnalysisCallback;
        Context->Analysis.Callback.Context = Object;

        //
        // Copy report path
        //

        Buffer = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
        StringCchCopy(Buffer, MAX_PATH, Context->ReportPath);
        Context->Analysis.ReportPath = Buffer;

        //
        // Copy symbol path
        //

       /* Buffer = (PWSTR)ApsMalloc(APS_PAGESIZE);
        ApsGetSymbolPath(Buffer, APS_PAGESIZE / sizeof(WCHAR));
        Context->Analysis.SymbolPath = Buffer;*/
		Context->Analysis.SymbolPath = UtilGetSymbolPath();

        //
        // Resume thread execution
        //

        ResumeThread(ThreadHandle);
        CloseHandle(ThreadHandle);

        Context->State = CTL_PANE_ANALYZING;

    }
    else {

        CtlPaneStopAnimation(Object);
        Context->State = CTL_PANE_COMPLETED; 

        //
        // N.B. Notify frame window the profile is completed
        //

        PostMessage(GetParent(Object->hWnd), WM_USER_PROFILE_COMPLETED, 0, 0);
    }

    //
    // Force redraw of client area
    //

    CtlPaneRedraw(Object);
	return Status;
}

LRESULT
CtlPaneOnMark(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;
    PAPS_PROFILE_OBJECT Profile;
    ULONG Status;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCTL_PANE_CONTEXT)Object->Context;
    Profile = Context->Profile;

	Status = ApsMarkProfile(Profile);
	if (Status != APS_STATUS_OK) {

		//
		// N.B. Mark error is not considered as fatal issue,
		// just log it or simply ignore it here
		//
		
		MessageBox(hWnd, L"Failed to mark profile!", 
				   L"D Profile", MB_OK|MB_ICONERROR);
	}

	return Status;
}

VOID
CtlPaneSetButtonState(
	__in PDIALOG_OBJECT Object
	)
{
	PCTL_PANE_CONTEXT Context;
	HWND hWndCtrl;
	PAPS_PROFILE_OBJECT Profile;

	Context = (PCTL_PANE_CONTEXT)Object->Context;
	Profile = Context->Profile;

	switch (Context->State) {

		case CTL_PANE_BOOTSTRAP:

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_MARK);
			ShowWindow(hWndCtrl, SW_HIDE);

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_PAUSE_RESUME);
			ShowWindow(hWndCtrl, SW_HIDE);
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_STOP);
			ShowWindow(hWndCtrl, SW_HIDE);
			break;

		case CTL_PANE_PROFILING:
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_MARK);
			ShowWindow(hWndCtrl, SW_SHOW);

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_PAUSE_RESUME);
			ShowWindow(hWndCtrl, SW_SHOW);
			SetWindowText(hWndCtrl, L"Pause");

			//
			// N.B. MM profile does not support pause/resume
			//

			ASSERT(Profile != NULL);

			if (Profile->Type == PROFILE_MM_TYPE) {
				EnableWindow(hWndCtrl, FALSE);
			}
			if (Profile->Type == PROFILE_CPU_TYPE) {
				EnableWindow(hWndCtrl, TRUE);
			}
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_STOP);
			ShowWindow(hWndCtrl, SW_SHOW);
			break;

		case CTL_PANE_INIT_PAUSED:
		case CTL_PANE_PAUSED:
			
			//
			// N.B. MM profile does not support pause/resume
			//

			ASSERT(Profile->Type == PROFILE_CPU_TYPE);

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_MARK);
			ShowWindow(hWndCtrl, SW_HIDE);

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_PAUSE_RESUME);
			ShowWindow(hWndCtrl, SW_SHOW);
			SetWindowText(hWndCtrl, L"Resume");
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_STOP);
			ShowWindow(hWndCtrl, SW_SHOW);
			break;

		case CTL_PANE_COMPLETED:
		case CTL_PANE_TERMINATED:
		case CTL_PANE_RESUME_FAILED:
		case CTL_PANE_FAILED:
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_MARK);
			ShowWindow(hWndCtrl, SW_HIDE);

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_PAUSE_RESUME);
			ShowWindow(hWndCtrl, SW_HIDE);
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_STOP);
			ShowWindow(hWndCtrl, SW_HIDE);
			break;

		case CTL_PANE_ANALYZING:
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_MARK);
			ShowWindow(hWndCtrl, SW_HIDE);

			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_PAUSE_RESUME);
			ShowWindow(hWndCtrl, SW_HIDE);
			
			hWndCtrl = GetDlgItem(Object->hWnd, IDC_BUTTON_STOP);
			ShowWindow(hWndCtrl, SW_HIDE);
			break;

		default:
			ASSERT(0);
	}
}

ULONG
CtlPaneCreateProfileByAttach(
	__in HWND hWndCtlPane,
	__in PWIZARD_CONTEXT Wizard,
    __out PWSTR ReportPath
	)
{
    ULONG Status;
    PAPS_PROCESS Process;
    PAPS_PROFILE_OBJECT Profile;
    BTR_PROFILE_ATTRIBUTE Attr;
	PDIALOG_OBJECT Pane;
	PCTL_PANE_CONTEXT Context;

    ASSERT(Wizard->Process != NULL);
    Process = Wizard->Process;

    //
    // Build report name
    //

    ApsBuildReportFullName(Process->Name, Process->ProcessId, 
                           Wizard->Type, ReportPath);

	Status = APS_STATUS_OK;
	RtlZeroMemory(&Attr, sizeof(Attr));

	//
	// Fill profile attributes and create corresponding
	// profile object
	//

	if (Wizard->Type == PROFILE_CPU_TYPE) {

		Attr.Type = PROFILE_CPU_TYPE;
		Attr.Mode = RECORD_MODE;
		Attr.SamplingPeriod = Wizard->Cpu.SamplingPeriod;
		Attr.StackDepth = Wizard->Cpu.StackDepth;
		Attr.IncludeWait = Wizard->Cpu.IncludeWait;
		Attr.Paused = Wizard->InitialPause;

	    Status = ApsCreateCpuProfile(Process->ProcessId, Process->FullPath,
	                                 &Attr, ReportPath, &Profile);

	}

	else if (Wizard->Type == PROFILE_MM_TYPE) {

		Attr.Type = PROFILE_MM_TYPE;
		Attr.Mode = DIGEST_MODE;
		Attr.EnableGdi = Wizard->Mm.EnableGdi;
		Attr.EnableHandle = Wizard->Mm.EnableHandle;
		Attr.EnableHeap = Wizard->Mm.EnableHeap;
		Attr.EnablePage = Wizard->Mm.EnablePage;

		Status = ApsCreateMmProfile(Process->ProcessId, Wizard->ImagePath, 
									DIGEST_MODE, &Attr, ReportPath, &Profile);
	}

	else if (Wizard->Type == PROFILE_IO_TYPE) {
		ASSERT(0);
	}

	else if (Wizard->Type == PROFILE_CCR_TYPE) {
		ASSERT(0);
	}
	else {
		ASSERT(0);
	}

    if (Status != APS_STATUS_OK) {
        return Status;
    }

	//
	// Save profile object
	//

	Pane = (PDIALOG_OBJECT)SdkGetObject(hWndCtlPane);
	Context = (PCTL_PANE_CONTEXT)Pane->Context;
	Context->Profile = Profile;

	//
	// Back up attributes we may use after profile stops
	//

	Context->Type = Wizard->Type;
	Context->ProcessId = Process->ProcessId;
	StringCchCopy(Context->ProcessPath, MAX_PATH, Process->FullPath);
	StringCchCopy(Context->ReportPath, MAX_PATH, ReportPath);

	//
	// Save process path
	//

	StringCchCopy(Profile->ImagePath, MAX_PATH, Process->FullPath);
    return APS_STATUS_OK;
}

ULONG
CtlPaneCreateProfileByLaunch(
	__in HWND hWndCtlPane,
	__in PWIZARD_CONTEXT Wizard,
    __out PWSTR ReportPath
	)
{
    ULONG Status;
    PAPS_PROFILE_OBJECT Profile;
    BTR_PROFILE_ATTRIBUTE Attr;
	PDIALOG_OBJECT Pane;
	PCTL_PANE_CONTEXT Context;
	ULONG ProcessId;
	WCHAR BaseName[MAX_PATH];

	//
	// First run application
	//

	ProcessId = 0;

	Status = CtlPaneRunApplication(Wizard->ImagePath, Wizard->Argument,
								   Wizard->WorkPath, FALSE, NULL, 
								   NULL, &ProcessId, NULL);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	ASSERT(ProcessId != 0);

	//
	// Sleep 5 seconds to make target to finish its initialization
    //

    Sleep(5000);

	//
	// Strip out process base name and build report path
	//
	
	_wsplitpath(Wizard->ImagePath, NULL, NULL, BaseName, NULL);
    ApsBuildReportFullName(BaseName, ProcessId, Wizard->Type, ReportPath);

	Status = APS_STATUS_OK;
	RtlZeroMemory(&Attr, sizeof(Attr));

	//
	// Fill profile attributes and create corresponding
	// profile object
	//

	if (Wizard->Type == PROFILE_CPU_TYPE) {

		Attr.Type = PROFILE_CPU_TYPE;
		Attr.Mode = RECORD_MODE;
		Attr.SamplingPeriod = Wizard->Cpu.SamplingPeriod;
		Attr.StackDepth = Wizard->Cpu.StackDepth;
		Attr.IncludeWait = Wizard->Cpu.IncludeWait;
		Attr.Paused = Wizard->InitialPause;

		Status = ApsCreateCpuProfile(ProcessId, Wizard->ImagePath,
	                                 &Attr, ReportPath, &Profile);

	}

	else if (Wizard->Type == PROFILE_MM_TYPE) {

		Attr.Type = PROFILE_MM_TYPE;
		Attr.Mode = DIGEST_MODE;
		Attr.EnableGdi = Wizard->Mm.EnableGdi;
		Attr.EnableHandle = Wizard->Mm.EnableHandle;
		Attr.EnableHeap = Wizard->Mm.EnableHeap;
		Attr.EnablePage = Wizard->Mm.EnablePage;

		Status = ApsCreateMmProfile(ProcessId, Wizard->ImagePath, 
									DIGEST_MODE, &Attr, ReportPath, &Profile);
	}

	else if (Wizard->Type == PROFILE_IO_TYPE) {
		ASSERT(0);
	}

	else if (Wizard->Type == PROFILE_CCR_TYPE) {
		ASSERT(0);
	}
	else {
		ASSERT(0);
	}

    if (Status != APS_STATUS_OK) {
        return Status;
    }

	//
	// Save profile object
	//

	Pane = (PDIALOG_OBJECT)SdkGetObject(hWndCtlPane);
	Context = (PCTL_PANE_CONTEXT)Pane->Context;
	Context->Profile = Profile;

	//
	// Back up attributes we may use after profile stops
	//

	Context->Type = Wizard->Type;
	Context->ProcessId = ProcessId;
	StringCchCopy(Context->ProcessPath, MAX_PATH, Wizard->ImagePath);
	StringCchCopy(Context->ReportPath, MAX_PATH, ReportPath);

	//
	// Save process path
	//

	StringCchCopy(Profile->ImagePath, MAX_PATH, Wizard->ImagePath);
	StringCchCopy(Profile->WorkPath, MAX_PATH, Wizard->WorkPath);
    return APS_STATUS_OK;
}

VOID
CtlPaneGetProfileAttributes(
    __in HWND hWnd,
    __out BTR_PROFILE_TYPE *Type,
    __out PWCHAR ReportPath,
    __in SIZE_T Length
    )
{
    PDIALOG_OBJECT Object;
    PCTL_PANE_CONTEXT Context;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCTL_PANE_CONTEXT)Object->Context;

    *Type = Context->Type;
    StringCchCopy(ReportPath, Length, Context->ReportPath);
}

BOOLEAN
CtlPaneIsAutoAnalyze(
	__in PDIALOG_OBJECT Object
	)
{
	PMDB_DATA_ITEM Mdi;
	BOOLEAN IsAutoAnalyze;

	UNREFERENCED_PARAMETER(Object);

	Mdi = MdbGetData(MdbAutoAnalyze);
	ASSERT(Mdi != NULL);

	IsAutoAnalyze = atoi(Mdi->Value);
	return IsAutoAnalyze;
}

ULONG
CtlPaneOpenReport(
	__in HWND hWnd,
	__in PWSTR ReportPath 
	)
{
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	HANDLE ThreadHandle;
	BTR_PROFILE_TYPE Type;
	PWSTR Buffer;
    PF_REPORT_HEAD Head;
    ULONG Status;

	ASSERT(ReportPath != NULL);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

    //
    // Cache report path and check report's integrity
    //

    StringCchCopy(Context->ReportPath, MAX_PATH, ReportPath);
    Status = ApsReadReportHead(Context->ReportPath, &Head);
    if (Status != APS_STATUS_OK) {
		MessageBox(hWnd, L"It's an invalid profile report!", L"D Profile", MB_OK|MB_ICONERROR);
        return Status;
    }

    if (Head.Analyzed) {

        //
        // If it's already analyzed, notify frame window that the report is opened
        // that it can switch to report view and hide ctlpane
        //

        PostMessage(GetParent(hWnd), WM_USER_PROFILE_OPENED, 0, 0);
        return APS_STATUS_OK;
    }

	ThreadHandle = CreateThread(NULL, 0, ApsAnalysisProcedure, 
								&Context->Analysis, 
								CREATE_SUSPENDED, NULL); 
	if (!ThreadHandle) {
		return APS_STATUS_ERROR;
	}

	//
	// Fill analysis context and start analysis thread
	//

	ZeroMemory(&Context->Analysis, sizeof(Context->Analysis));

	if (Head.IncludeCpu) {
		Type = PROFILE_CPU_TYPE;
	} 
	else if (Head.IncludeMm) {
		Type = PROFILE_MM_TYPE;
	} 
	else if (Head.IncludeIo) {
		Type = PROFILE_IO_TYPE;
	} 
	else if (Head.IncludeCcr) {
		Type = PROFILE_CCR_TYPE;
	}
	else {
		ASSERT(0);
	}

    //
    // Initialize open context
    //

	Context->Analysis.Type = Type;
	Context->Analysis.Option = ANALYSIS_CREATE_COUNTERS; 
	Context->Analysis.Callback.Callback = CtlPaneOpenCallback;
	Context->Analysis.Callback.Context = Object;
    Context->Analysis.Callback.Status = APS_STATUS_OK;

	//
	// Copy report path
	//

	Buffer = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
	StringCchCopy(Buffer, MAX_PATH, ReportPath);
	Context->Analysis.ReportPath = Buffer;

	//
	// Copy symbol path
	//

	/*Buffer = (PWSTR)ApsMalloc(APS_PAGESIZE);
	ApsGetSymbolPath(Buffer, APS_PAGESIZE / sizeof(WCHAR));
	Context->Analysis.SymbolPath = Buffer;*/
	Context->Analysis.SymbolPath = UtilGetSymbolPath();

	//
	// Resume thread execution
	//

	ResumeThread(ThreadHandle);
	CloseHandle(ThreadHandle);

	Context->State = CTL_PANE_ANALYZING;

    //
    // Start animation
    //

    CtlPaneStartAnimation(Object);
    CtlPaneRedraw(Object);

	return APS_STATUS_OK;
}

//
// N.B. This routine is called only via UI
// when user select Analyze menu item
//

ULONG
CtlPaneOnAnalyze(
	__in HWND hWnd
	)
{
	ULONG Status;
	PF_REPORT_HEAD Head;
	PDIALOG_OBJECT Object;
	PCTL_PANE_CONTEXT Context;
	HANDLE ThreadHandle;
	BTR_PROFILE_TYPE Type;
	PWSTR Buffer;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCTL_PANE_CONTEXT)Object->Context;

	//
	// Read the report head
	//

    ASSERT(Context->ReportPath[0] != 0);
    
    Status = ApsReadReportHead(Context->ReportPath, &Head);
	if (Status != APS_STATUS_OK) {
		MessageBox(hWnd, L"It's an invalid profile report!", 
			       L"D Profile", MB_OK|MB_ICONERROR);
        return Status;
	}

	Status = ApsPrepareForAnalysis(Context->ReportPath);
	if (Status != APS_STATUS_OK) {
		MessageBox(hWnd, L"Failed to clear analyzed report!", 
				   L"D Profile", MB_OK|MB_ICONERROR);
		return Status;
	}

	ThreadHandle = CreateThread(NULL, 0, ApsAnalysisProcedure, 
								&Context->Analysis, 
								CREATE_SUSPENDED, NULL); 
	if (!ThreadHandle) {
		return APS_STATUS_ERROR;
	}

	//
	// Fill analysis context and start analysis thread
	//

	ZeroMemory(&Context->Analysis, sizeof(Context->Analysis));

	if (Head.IncludeCpu) {
		Type = PROFILE_CPU_TYPE;
	} 
	else if (Head.IncludeMm) {
		Type = PROFILE_MM_TYPE;
	} 
	else if (Head.IncludeIo) {
		Type = PROFILE_IO_TYPE;
	} 
	else if (Head.IncludeCcr) {
		Type = PROFILE_CCR_TYPE;
	}
	else {
		ASSERT(0);
	}

    //
    // Initialize analysis context
    //

	Context->Analysis.Type = Type;
	Context->Analysis.Option = ANALYSIS_CREATE_COUNTERS; 
	Context->Analysis.Callback.Callback = CtlPaneOpenCallback;
	Context->Analysis.Callback.Context = Object;
    Context->Analysis.Callback.Status = APS_STATUS_OK;

	//
	// Copy report path
	//

	Buffer = (PWSTR)ApsMalloc(MAX_PATH * sizeof(WCHAR));
	StringCchCopy(Buffer, MAX_PATH, Context->ReportPath);
	Context->Analysis.ReportPath = Buffer;

	//
	// Copy symbol path
	//

	//Buffer = (PWSTR)ApsMalloc(APS_PAGESIZE);
	//ApsGetSymbolPath(Buffer, APS_PAGESIZE / sizeof(WCHAR));
	//Context->Analysis.SymbolPath = Buffer;
	Context->Analysis.SymbolPath = UtilGetSymbolPath();

	//
	// Resume thread execution
	//

	ResumeThread(ThreadHandle);
	CloseHandle(ThreadHandle);

	Context->State = CTL_PANE_ANALYZING;

    //
    // Start animation
    //

    CtlPaneStartAnimation(Object);
    CtlPaneRedraw(Object);

	return APS_STATUS_OK;
}