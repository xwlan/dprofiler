//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

PWSTR
UtilGetSymbolPath(
	VOID
	);

extern PWSTR UtilSymbolPath;


#ifdef __cplusplus
}
#endif

#endif