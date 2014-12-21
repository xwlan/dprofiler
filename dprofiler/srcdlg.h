//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _SRCDLG_H_
#define _SRCDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"

//
// Each tab item specify one source file,
// each source file is contained by a listview.
//

typedef struct _FILE_ITEM {

	LIST_ENTRY ListEntry;
	TCITEM Tab;
	HWND hWndList;

	WCHAR BaseName[MAX_PATH];
	WCHAR FullPath[MAX_PATH];
	ULONG LineCount;

} FILE_ITEM, *PFILE_ITEM;

typedef struct _SOURCE_CONTEXT {

	HWND hWndTab;
	LIST_ENTRY FileListHead;
	ULONG FileCount;

	PFILE_ITEM Current;

	COLORREF clrText;
	COLORREF clrTextHighlight;
	HBRUSH hBrushHighlight;

} SOURCE_CONTEXT, *PSOURCE_CONTEXT;

HWND
SourceCreate(
	__in HWND hWndParent,
	__in ULONG CtrlId 
	);

LRESULT
SourceOnInitDialog(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
SourceOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
SourceOnClose(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK
SourceProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnSysCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnContextMenu(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
SourceOnCommand(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnCloseFile(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnOpenFromShell(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnOpenFolder(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnCopyPath(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceOnTcnSelChange(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT CALLBACK 
SourceTabProcedure(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp,
	__in UINT_PTR uIdSubclass, 
	__in DWORD_PTR dwData
	);

LRESULT
SourceTabOnCustomDraw(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
SourceTabOnNotify(
	__in HWND hWnd, 
	__in UINT uMsg, 
	__in WPARAM wp, 
	__in LPARAM lp
	);

LRESULT
SourceTabOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

BOOLEAN
SourceOpenFile(
	__in HWND hWnd,
	__in PWSTR FullPath,
	__in ULONG Line
	);

BOOLEAN
SourceIsOpened(
	__in HWND hWnd,
	__in PWSTR FullPath,
	__out PFILE_ITEM *File
	);

VOID
SourceSetTab(
	__in PDIALOG_OBJECT Object,
	__in PFILE_ITEM Item
	);

BOOLEAN
SourceInsertTab(
	__in PDIALOG_OBJECT Object,
	__in PFILE_ITEM FileItem 
	);

PFILE_ITEM
SourceCurrentTab(
	__in PDIALOG_OBJECT Object
	);

VOID
SourceAdjustView(
	__in PSOURCE_CONTEXT Context,
	__in PFILE_ITEM FileItem
	);

VOID
SourceCloseTab(
	__in PDIALOG_OBJECT Object,
	__in int index
	);

PFILE_ITEM
SourceIndexToTab(
	__in PSOURCE_CONTEXT Context,
	__in int Index
	);

PFILE_ITEM
SourceLoadFile(
	__in PDIALOG_OBJECT Object,
	__in PWSTR FullPath
	);

HWND
SourceCreateView(
	__in PDIALOG_OBJECT Object
	);

VOID
SourceSetLine(
	__in PDIALOG_OBJECT Object,
	__in PFILE_ITEM Item,
	__in ULONG Line
	);

LRESULT
SourceTabDrawText(
	__in PSOURCE_CONTEXT Context,
	__in HWND hWndList,
	__in LPNMLVCUSTOMDRAW lvcd
	);

LRESULT
SourceOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

#ifdef __cplusplus
}
#endif
#endif
