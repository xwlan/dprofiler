//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"

VOID
BtrUninitializeHeap(
	VOID
	);

ULONG
BtrPrepareUnload(
	VOID
	);

#ifdef __cplusplus
}
#endif
#endif
