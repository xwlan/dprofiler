//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PERFVIEW_H_
#define _PERFVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#include "sdk.h"
#include "listview.h"

enum PerfViewColumnType {
	LogColumnProcess,
	LogColumnSequence,
	LogColumnTimestamp,
	LogColumnPID,
	LogColumnTID,
	LogColumnProbe,
	LogColumnProvider,
	LogColumnReturn,
	LogColumnDuration,
	LogColumnDetail,
	PerfColumnCount	
};

typedef enum _PERFVIEW_MODE {
	PERFVIEW_UPDATE_MODE,
	PERFVIEW_PENDING_MODE,
	PERFVIEW_STALL_MODE,
} PERFVIEW_MODE;

typedef struct _PERFVIEW_IMAGE {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	ULONG ImageIndex;
} PERFVIEW_IMAGE, *PPERFVIEW_IMAGE;

#define WM_PERFVIEW_STALL (WM_USER + 10)

extern int PerfViewColumnOrder[PerfColumnCount];
extern LISTVIEW_COLUMN PerfViewColumn[PerfColumnCount];
extern LISTVIEW_OBJECT PerfViewObject;
extern PERFVIEW_MODE PerfViewMode;

PLISTVIEW_OBJECT
PerfViewCreate(
	IN HWND hWndParent,
	IN ULONG CtrlId
	);

ULONG
PerfViewInitImageList(
	IN HWND hWnd	
	);

ULONG
PerfViewAddImageList(
	IN PWSTR FullPath,
	IN ULONG ProcessId
	);

ULONG
PerfViewQueryImageList(
	IN ULONG ProcessId
	);

VOID
PerfViewSetItemCount(
	IN ULONG Count
	);

LRESULT
PerfViewOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfViewOnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
PerfViewOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfViewOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfViewOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
PerfViewEnterStallMode(
	VOID
	);

VOID
PerfViewExitStallMode(
	IN ULONG NumberOfRecords
	);

BOOLEAN
PerfViewIsInStallMode(
	VOID
	);

ULONG
PerfViewGetItemCount(
	VOID
	);

int
PerfViewGetFirstSelected(
	VOID
	); 

VOID
PerfViewSetAutoScroll(
	IN BOOLEAN AutoScroll
	);

#ifdef __cplusplus
}
#endif

#endif