// 
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#ifndef _SPLITBAR_H_
#define _SPLITBAR_H_

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SPLITBAR_OBJECT {
	BOOLEAN Show;
	BOOLEAN Moved;
	BOOLEAN DragMode;
	BOOLEAN Vertical;
	HWND hWnd[2];
	LONG Top;
	LONG Position;
	LONG Border;
} SPLITBAR_OBJECT, *PSPLITBAR_OBJECT;

extern SPLITBAR_OBJECT SplitBar;

LRESULT 
SplitBarOnMouseMove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
SplitBarOnLButtonDown(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
SplitBarDrawBar(
	IN HDC hdc, 
	IN int x1, 
	IN int y1, 
	IN int width, 
	IN int height
	);

VOID
SplitBarAdjustPosition(
	IN LONG x,
	IN LONG y
	);

#ifdef __cplusplus
}
#endif

#endif