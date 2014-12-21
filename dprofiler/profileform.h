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
	CPU_FORM_FUNCTION,
	CPU_FORM_MODULE,
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
// UI macro 
//

#define CPU_UI_INCLUSIVE_RATIO_THRESHOLD  0.001

#ifdef __cplusplus
}
#endif
#endif