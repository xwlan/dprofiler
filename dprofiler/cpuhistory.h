//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _CPU_HISTORY_H_
#define _CPU_HISTORY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "treelist.h"
#include "statebar.h"
    
typedef struct _CPU_STATE_UI_CONTEXT {
	HDC hdc;
	HBITMAP hbmp;
	HBITMAP hbmpOld;
	RECT bmpRect;
} CPU_STATE_UI_CONTEXT, *PCPU_STATE_UI_CONTEXT;

typedef struct _CPU_HISTORY_FORM_CONTEXT {

    CPU_FORM_CONTEXT Base;

	HBRUSH hbrRun;
	HBRUSH hbrWait;
	HBRUSH hbrIo;
	HBRUSH hbrSleep;
	HPEN hBorderPen;

	double TrackScale;
	double StateScale;

	//
	// Cell Metrics
	//

	ULONG Scale;
	int MinCellWidth;
	int MaxCellWidth;
	int CellHeight;
	int CellBorder;
	int HoriEdge;
	int VertEdge;

    PBTR_CPU_RECORD Record;
	PCPU_THREAD_TABLE ThreadTable;

	//
	// Copy of thread table's thread array
	//

	PCPU_THREAD *Thread;
	ULONG NumberOfThreads;

	//
	// Track selected thread index in thread object array
	//

	PULONG ThreadIndex;
	ULONG NumberOfIndexes;

	ULONG FirstSample;
	ULONG LastSample;

	double CpuUsageMin;
	double CpuUsageMax;
	double CpuUsageAverage;

	//
	// CPU utilization history
	//

    PCPU_HISTORY History;

    BOOLEAN AllowHeaderChange;
    int SamplePickerPos;
    int CurrentItem;
    ULONG CurrentSample;
    ULONG OldSample;
    int OldItem;
    HPEN hPickerPen;

	BOOLEAN IsSampleHover;
	HPEN hHoverPen;

    //
    // State bar object
    //

    PSTATEBAR_OBJECT StateBar;

} CPU_HISTORY_FORM_CONTEXT, *PCPU_HISTORY_FORM_CONTEXT;


HWND
CpuHistoryCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
CpuHistoryOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT CALLBACK 
CpuHistoryHeaderProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuHistoryOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
CpuHistoryProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnDrawItem(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnCustomDraw(
	__in PDIALOG_OBJECT Object, 
	__in LPNMHDR lpnmhdr
	);

LRESULT
CpuHistoryOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT 
CpuHistoryOnTtnGetDispInfo(
    __in PDIALOG_OBJECT Object,
	__in LPNMTTDISPINFO lpnmtdi
	);

LRESULT 
CpuHistoryOnNmClick(
    __in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

LRESULT 
CpuHistoryOnNmHover(
    __in PDIALOG_OBJECT Object,
	__in LPNMHDR lpnmhdr
	);

LRESULT 
CpuHistoryOnItemChanged(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
	);

LRESULT 
CpuHistoryOnColumnClick(
    __in PDIALOG_OBJECT Object,
	__in NMLISTVIEW *lpNmlv
    );

int CALLBACK
CpuHistorySortCallback(
	__in LPARAM First, 
	__in LPARAM Second,
	__in LPARAM Param
	);

VOID
CpuHistoryInsertData(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
	);

VOID
CpuHistoryInsertThreads(
	__in HWND hWnd,
	__in PPF_REPORT_HEAD Head
    );

ULONG
CpuHistoryGetStackIdByThread(
    __in PDIALOG_OBJECT Object,
    __in PCPU_THREAD Thread
    );

VOID
CpuHistorySetCurrentSample(
    __in PDIALOG_OBJECT Object,
    __in int Item,
    __in ULONG Number
    );

VOID
CpuHistorySetCurrentRecord(
    __in HWND hWnd,
    __in ULONG Number
    );

VOID
CpuHistoryInsertBackTrace(
	__in HWND hWnd,
    __in ULONG StackTraceId 
	);

LRESULT
CpuHistoryOnGraphSetRecord(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnParentNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnLButtonDown(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnGraphSetRecord(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnTrackSetRange(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
CpuHistoryOnCommand(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT
CpuHistoryOnScroll(
    __in HWND hWnd, 
    __in UINT uMsg, 
    __in WPARAM wp, 
    __in LPARAM lp
    );

LRESULT CALLBACK 
CpuHistoryListProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
CpuHistoryOnCtlColorStatic(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    );

int
CpuHistoryPositionStateBar(
    __in PDIALOG_OBJECT Object,
    __in LPRECT rcTrack
    );

int
CpuHistoryPositionStateBar2(
    __in PDIALOG_OBJECT Object,
    __in PSTATEBAR_OBJECT StateBar,
    __in LPRECT rcTrack
    );

VOID
CpuHistorySetRecordRange(
    __in PDIALOG_OBJECT Object,
    __in ULONG First,
	__in ULONG Last
    );

PCPU_THREAD_TABLE
CpuHistoryGetThreadTable(
    VOID
	);

VOID
CpuHistoryComputeStateRect(
	__in PCPU_HISTORY_FORM_CONTEXT Context,
	__in PCPU_THREAD Thread,
	__in LPRECT lprcBound,
	__out LPRECT lprcDraw,
	__out PULONG From,
	__out PULONG To
	);

BOOLEAN
CpuHistoryDrawThreadStateInMemory(
	__in PCPU_HISTORY_FORM_CONTEXT Context,
	__in HWND hWndCtrl,
	__in PCPU_THREAD Thread
	);

LRESULT
CpuHistoryDrawThreadStateLite(
	__in PDIALOG_OBJECT Object,
	__in HWND hWndList,
	__in LPNMLVCUSTOMDRAW lvcd
	);

BOOLEAN
CpuHistoryRedrawSampleRect(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in PCPU_THREAD Thread,
    __in HWND hWndList,
    __in HDC hdc,
    __in int Item,
    __in ULONG Sample
    );

LRESULT
CpuHistoryDrawThreadState(
	__in PDIALOG_OBJECT Object,
	__in HWND hWndList,
	__in LPNMLVCUSTOMDRAW lvcd
	);

LRESULT
CpuHistorySetStateColumnWidth(
	__in PDIALOG_OBJECT Object,
	__in HWND hWndList,
	__in ULONG First,
	__in ULONG Last
	);

int
CpuHistoryHitTestSample(
    __in PCPU_HISTORY_FORM_CONTEXT Context,
    __in HWND hWndCtrl,
    __in LPPOINT pt,
    __out PULONG SampleIndex,
    __out LPRECT rcSample
    );

LRESULT
CpuHistoryOnSampleForward(
	__in PDIALOG_OBJECT Object
	);

LRESULT
CpuHistoryOnSampleBackward(
	__in PDIALOG_OBJECT Object
	);

LRESULT
CpuHistoryOnSampleUpward(
	__in PDIALOG_OBJECT Object
	);

LRESULT
CpuHistoryOnSampleDownward(
	__in PDIALOG_OBJECT Object
	);

LRESULT
CpuHistoryOnSampleFullStack(
	__in PDIALOG_OBJECT Object
	);

LRESULT
CpuHistoryOnSampleReport(
	__in PDIALOG_OBJECT Object
	);

LRESULT 
CpuHistoryOnDbClick(
	__in PDIALOG_OBJECT Object,
	__in LPNMITEMACTIVATE lpnmitem
	);

#ifdef __cplusplus
}
#endif
#endif
