//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _MARKER_H_
#define _MARKER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"


VOID
BtrInitializeMarkList(
	VOID
	);

VOID
BtrInsertMark(
	__in ULONG Index,
	__in BTR_MARK_REASON Reason
	);

ULONG
BtrWriteMarkStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);


extern LIST_ENTRY BtrMarkList;
extern ULONG BtrMarkCount;


#ifdef __cplusplus
}
#endif
#endif // _MARKER_H_