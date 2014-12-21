//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _MAIN_H_
#define _MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

extern HWND hWndMain;
    
BOOLEAN
MainInitialize(
	VOID
	);

int
MainMessagePump(
	VOID
	);

struct _FRAME_OBJECT*
MainGetFrame(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif