//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _REBAR_H_
#define _REBAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "apsprofile.h"

typedef enum _REBAR_BAND_ID {
	REBAR_BAND_LABEL,
	REBAR_BAND_SWITCH,
	REBAR_BAND_FIND,
	REBAR_BAND_COMMAND,
} REBAR_BAND_ID, *PREBAR_BAND_ID;

typedef struct _REBAR_BAND {
	LIST_ENTRY ListEntry;
	HWND hWndBand;
	REBAR_BAND_ID BandId;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGrayed;
	REBARBANDINFO BandInfo;
} REBAR_BAND, *PREBAR_BAND;

typedef struct _REBAR_OBJECT {
	HWND hWndRebar;
	REBARINFO RebarInfo;
	ULONG NumberOfBands;
	LIST_ENTRY BandList;
} REBAR_OBJECT, *PREBAR_OBJECT;

PREBAR_OBJECT
RebarCreateObject(
	__in HWND hWndParent, 
	__in UINT Id 
	);

HWND
RebarCreateBar(
	__in PREBAR_OBJECT Object	
	);

HWND
RebarCreateLabel(
	__in PREBAR_OBJECT Object	
	);

HWND
RebarCreateFindEdit(
	__in PREBAR_OBJECT Object	
	);

HWND
RebarCreateCombo(
	__in PREBAR_OBJECT Object	
	);

VOID
RebarSetType(
	__in PREBAR_OBJECT Object,
	__in BTR_PROFILE_TYPE Type
	);

VOID
RebarSelectForm(
	__in PREBAR_OBJECT Object,
	__in ULONG Form
	);

BOOLEAN
RebarSetBarImage(
	__in HWND hWndBar,
	__in COLORREF Mask,
	__out HIMAGELIST *Normal,
	__out HIMAGELIST *Grayed
	);

BOOLEAN
RebarInsertBands(
	__in PREBAR_OBJECT Object
	);

VOID
RebarAdjustPosition(
	__in PREBAR_OBJECT Object
	);

VOID
RebarCurrentEditString(
	__in PREBAR_OBJECT Object,
	__out PWCHAR Buffer,
	__in ULONG Length
	);

PREBAR_BAND
RebarGetBand(
	__in PREBAR_OBJECT Object,
	__in REBAR_BAND_ID Id
	);

#ifdef __cplusplus
}
#endif

#endif