//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _BUILDIN_H_
#define _BUILDIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "callback.h"

typedef enum _BTR_BUILDIN_CALLBACK_TYPE {
	_ExitProcess,
	BUILDIN_CALLBACK_COUNT,
} BTR_BUILDIN_CALLBACK_TYPE;

ULONG
BtrRegisterBuildinCallbacks(
	VOID
	);

ULONG
BtrUnregisterBuildinCallbacks(
	VOID
	);

//
// ExitProcess
//

VOID WINAPI 
ExitProcessEnter(
	__in UINT ExitCode
	);

typedef VOID 
(WINAPI *EXITPROCESS)(
	__in UINT ExitCode
	);

extern BTR_CALLBACK BuildinCallback[];
extern ULONG BuildinCallbackCount;

#ifdef __cplusplus
}
#endif
#endif
