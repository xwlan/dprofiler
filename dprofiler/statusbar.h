#ifndef _STATUSBAR_H_
#define _STATUSBAR_H_

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _STATUSBAR_OBJECT{
	BOOLEAN Show;
	HWND hWnd;
	HWND hWndParent;
	UINT CtrlId;
	ULONG PartCount;
	ULONG *Width;
	PWSTR *Text;
} STATUSBAR_OBJECT, *PSTATUSBAR_OBJECT;

extern STATUSBAR_OBJECT StatusBarObject;

HWND
StatusBarCreate(
	OUT PSTATUSBAR_OBJECT StatusBar
	);

BOOLEAN
StatusBarSetParts(
	IN PSTATUSBAR_OBJECT StatusBar,
	IN ULONG Count,
	IN PULONG Width
	);

ULONG
StatusBarGetParts(
	IN PSTATUSBAR_OBJECT StatusBar
	);

BOOLEAN
StatusBarSetText(
	IN PSTATUSBAR_OBJECT StatusBar,
	IN ULONG PartIndex,
	IN PWSTR Text
	);


#ifdef __cplusplus
}
#endif

#endif