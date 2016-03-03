//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
//

#ifndef _PROFILE_FORM_H_
#define _PROFILE_FORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"
#include "treelist.h"

//
// CPU Form Defs
//

typedef enum _CPU_FORM_TYPE {
	CPU_FORM_SUMMARY,
	CPU_FORM_IP,
    CPU_FORM_THREAD,
	CPU_FORM_CALLTREE,
    CPU_FORM_FLAME,
    CPU_FORM_HISTORY,
	CPU_FORM_COUNT,

} CPU_FORM_TYPE;

typedef struct _CPU_FORM {
	CPU_FORM_TYPE Current;
	HWND hWndForm[CPU_FORM_COUNT];
} CPU_FORM, *PCPU_FORM;

typedef struct _CPU_FORM_CONTEXT {

	PTREELIST_OBJECT TreeList;
	PLISTVIEW_OBJECT ListView;
	UINT_PTR CtrlId;
	PPF_REPORT_HEAD Head;
	HFONT hFontThin;
	HFONT hFontBold;
	HBRUSH hBrushBack;
	WCHAR Path[MAX_PATH];

	ULONG Inclusive;
	ULONG Exclusive;
	
	ULONG DllInclusive;
	ULONG DllExclusive;

    PVOID Context;
} CPU_FORM_CONTEXT, *PCPU_FORM_CONTEXT;

typedef struct _CPU_SORT_CONTEXT {
	LPARAM Context;
	LIST_SORT_ORDER Order;
	ULONG Column;
} CPU_SORT_CONTEXT, *PCPU_SORT_CONTEXT;

//
// MM Form Defs
//

typedef enum _MM_FORM_TYPE {
	MM_FORM_SUMMARY,
	MM_FORM_LEAK,
	MM_FORM_HEAP,
    MM_FORM_CALLTREE,
    MM_FORM_FLAME,
	MM_FORM_COUNT,
} MM_FORM_TYPE;

typedef struct _MM_FORM {
	MM_FORM_TYPE Current;
	HWND hWndForm[MM_FORM_COUNT];
} MM_FORM, *PMM_FORM;

typedef struct _MM_FORM_CONTEXT {

	PTREELIST_OBJECT TreeList;
	PLISTVIEW_OBJECT ListView;
	UINT_PTR CtrlId;
	PPF_REPORT_HEAD Head;
	HFONT hFontThin;
	HFONT hFontBold;
	HBRUSH hBrushBack;
	WCHAR Path[MAX_PATH];

	ULONG Inclusive;
	ULONG Exclusive;
	PVOID Context;

} MM_FORM_CONTEXT, *PMM_FORM_CONTEXT;

typedef struct _MM_SORT_CONTEXT {
	LPARAM Context;
	LIST_SORT_ORDER Order;
	ULONG Column;
} MM_SORT_CONTEXT, *PMM_SORT_CONTEXT;

//
// IO Form Defs
//

typedef enum _IO_FORM_TYPE {
	IO_FORM_SUMMARY,
	IO_FORM_FILE,
	IO_FORM_SOCKET,
	IO_FORM_FLAME,
	IO_FORM_THREAD,
	IO_FORM_COUNT,
} IO_FORM_TYPE;

typedef struct _IO_FORM {
	IO_FORM_TYPE Current;
	HWND hWndForm[CPU_FORM_COUNT];
} IO_FORM, *PIO_FORM;

typedef struct _IO_FORM_CONTEXT {

	PTREELIST_OBJECT TreeList;
	PLISTVIEW_OBJECT ListView;
	UINT_PTR CtrlId;
	PPF_REPORT_HEAD Head;
	HFONT hFontThin;
	HFONT hFontBold;
	HBRUSH hBrushBack;
	WCHAR Path[MAX_PATH];

	ULONG Inclusive;
	ULONG Exclusive;
	
	ULONG DllInclusive;
	ULONG DllExclusive;

    PVOID Context;
	PIO_OBJECT_ON_DISK IoObject;

} IO_FORM_CONTEXT, *PIO_FORM_CONTEXT;

//
// CCR Form Defs
//

typedef enum _CCR_FORM_TYPE {
	CCR_FORM_SUMMARY,
	CCR_FORM_CONTENTION,
	CCR_FORM_FLAME,
	CCR_FORM_COUNT,
} CCR_FORM_TYPE;

typedef struct _CCR_FORM {
	CCR_FORM_TYPE Current;
	HWND hWndForm[CPU_FORM_COUNT];
} CCR_FORM, *PCCR_FORM;

typedef struct _CCR_FORM_CONTEXT {

	PTREELIST_OBJECT TreeList;
	PLISTVIEW_OBJECT ListView;
	UINT_PTR CtrlId;
	PPF_REPORT_HEAD Head;
	HFONT hFontThin;
	HFONT hFontBold;
	HBRUSH hBrushBack;
	WCHAR Path[MAX_PATH];

	ULONG Inclusive;
	ULONG Exclusive;
	
	ULONG DllInclusive;
	ULONG DllExclusive;

    PVOID Context;
	PCCR_LOCK_TABLE Table;
	PCCR_LOCK_TRACK Lock;

} CCR_FORM_CONTEXT, *PCCR_FORM_CONTEXT;

//
// UI macro 
//

#define CPU_UI_INCLUSIVE_RATIO_THRESHOLD  0.001

#ifdef __cplusplus
}
#endif
#endif