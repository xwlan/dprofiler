//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _APS_CCR_H_
#define _APS_CCR_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "apsctl.h"
#include "apspdb.h"

ULONG
ApsCcrNormalizeRecords(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle
	);

ULONG
ApsCcrCreateCounters(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

#ifdef __cplusplus 
}
#endif
#endif