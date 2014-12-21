//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _STATEBAR_H_
#define _STATEBAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "apsprofile.h"

typedef enum _STATEBAR_BAND_ID {
	STATEBAR_BAND_COLOR,
	STATEBAR_BAND_COMMAND,
	STATEBAR_BAND_ZOOM,
} STATEBAR_BAND_ID, *PSTATEBAR_BAND_ID;

typedef struct _STATEBAR_BAND {
	LIST_ENTRY ListEntry;
	HWND hWndBand;
	STATEBAR_BAND_ID BandId;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGrayed;
	REBARBANDINFO BandInfo;
} STATEBAR_BAND, *PSTATEBAR_BAND;

typedef struct _STATEBAR_OBJECT {
	HWND hWndStateBar;
	REBARINFO StateBarInfo;
	ULONG NumberOfBands;
	LIST_ENTRY BandList;
} STATEBAR_OBJECT, *PSTATEBAR_OBJECT;

PSTATEBAR_OBJECT
StateBarCreateObject(
	__in HWND hWndParent, 
	__in UINT Id,
    __in BOOLEAN IsDynamic
	);

HWND
StateBarCreateCommand(
	__in PSTATEBAR_OBJECT Object	
	);

HWND
StateBarCreateColor(
	__in PSTATEBAR_OBJECT Object	
	);

HWND
StateBarCreateZoom(
	__in PSTATEBAR_OBJECT Object	
	);

BOOLEAN
StateBarSetImageList(
	__in HWND hWndBar,
	__in COLORREF Mask,
	__out HIMAGELIST *Normal,
	__out HIMAGELIST *Grayed
	);

BOOLEAN
StateBarInsertBands(
	__in PSTATEBAR_OBJECT Object
	);

VOID
StateBarAdjustPosition(
	__in PSTATEBAR_OBJECT Object
	);

PSTATEBAR_BAND
StateBarGetBand(
	__in PSTATEBAR_OBJECT Object,
	__in STATEBAR_BAND_ID Id
	);

#ifdef __cplusplus
}
#endif

#endif