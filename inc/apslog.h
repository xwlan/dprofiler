//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
//

#ifndef _APS_LOG_H_
#define _APS_LOG_H_

#include "aps.h"

#ifdef __cplusplus
extern "C" {
#endif 

//
// APS Logging Defs
//

#define APS_FLAG_LOGGING	0x1
#define APS_LOG_TEXT_LIMIT  (MAX_PATH * 2)

typedef enum _APS_LOG_LEVEL {
	LogLevelSuccess,
	LogLevelWarning,
	LogLevelFailure,
} APS_LOG_LEVEL, *PAPS_LOG_LEVEL;

typedef struct _APS_LOG_ENTRY {
	LIST_ENTRY ListEntry;
	SYSTEMTIME TimeStamp;
	ULONG ProcessId;
	APS_LOG_LEVEL Level;
	CHAR Text[APS_LOG_TEXT_LIMIT];
} APS_LOG_ENTRY, *PAPS_LOG_ENTRY;

typedef struct _APS_LOG { 
	CRITICAL_SECTION Lock;
	LIST_ENTRY ListHead;
	ULONG Flag;
	ULONG ListDepth;
	HANDLE FileObject;
	HANDLE StopObject;
	HANDLE ThreadObject;
	CHAR Path[MAX_PATH];
} APS_LOG, *PAPS_LOG;

//
// Logging Routines
//

ULONG
ApsInitializeLog(
	__in ULONG Flag	
	);

ULONG
ApsUninitializeLog(
	VOID
	);

VOID
ApsWriteLogEntry(
	__in ULONG ProcessId,
	__in APS_LOG_LEVEL Level,
	__in PSTR Format,
	...
	);

ULONG
ApsCurrentLogListDepth(
	VOID
	);

ULONG
ApsFlushLogQueue(
	VOID
	);

ULONG CALLBACK
ApsLogProcedure(
	__in PVOID Context
	);

BOOLEAN
ApsIsLoggingEnabled(
	VOID
	);

VOID
ApsGetLogFilePath(
	__out PCHAR Buffer,
	__in ULONG Length
	);

#ifdef __cplusplus
}
#endif

#endif
