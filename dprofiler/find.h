//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2014
//

#ifndef _FIND_H_
#define _FIND_H_

#include "dprofiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FIND_NOT_FOUND (ULONG)-1

struct _FIND_CONTEXT;

typedef ULONG 
(*FIND_CALLBACK)(
   __in struct _FIND_CONTEXT *Object
);

typedef struct _FIND_CONTEXT {
	HWND hWndFrame;
	BOOLEAN FindForward;
	ULONG PreviousIndex;
	WCHAR FindString[MAX_PATH];
    FIND_CALLBACK FindCallback;
} FIND_CONTEXT, *PFIND_CONTEXT;

BOOLEAN
FindIsComplete(
	__in struct _FIND_CONTEXT *Context,
	__in ULONG Current
	);

#ifdef __cplusplus
}
#endif

#endif