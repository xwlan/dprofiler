//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _MDB_H_
#define _MDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sqlapi.h"

typedef enum _MDB_STATUS {
	STATUS_MDB_OK = S_OK,
	STATUS_MDB_OPENFAILED,
	STATUS_MDB_EXECFAILED,
	STATUS_MDB_QUERYFAILED,
	STATUS_MDB_NODATAITEM,
	STATUS_MDB_MISMATCH,
	STATUS_MDB_UNKNOWN,
} MDB_STATUS;

typedef enum _MDB_DATA_TYPE {
	MdbBoolean,
	MdbInteger,
	MdbFloat,
	MdbUtf8,
	MdbAnsi,
	MdbUnicode,
	MdbBinary,
} MDB_DATA_TYPE;

//
// N.B. The data order is strictly ordered in SQLite MetaData table
// as primary key.
//

typedef enum _MDB_DATA_OFFSET {

	//
	// General
	//

	MdbAutoAnalyze,

	//
	// Symbol
	//

	MdbSymbolPath,
    MdbMsSymbolServer,

	//
	// Source 
	//
	
	MdbSourcePath,
    MdbCodeEditor,

	//
	// View 
	//

	MdbRecordLimit,

} MDB_DATA_OFFSET;

typedef struct _MDB_DATA_ITEM {
	PSTR Key;
	PSTR Value;
	MDB_DATA_TYPE Type;
} MDB_DATA_ITEM, *PMDB_DATA_ITEM;

extern MDB_DATA_ITEM MdbData[];

ULONG
MdbInitialize(
	__in ULONG Flag 
	);

VOID
MdbClose(
	VOID
	);

ULONG
MdbLoadMetaData(
	__in PSQL_DATABASE SqlObject
	);

PMDB_DATA_ITEM
MdbGetData(
	__in MDB_DATA_OFFSET Offset
	);

ULONG
MdbSetData(
	__in MDB_DATA_OFFSET Offset,
	__in PSTR Value
	);

BOOLEAN 
MdbQueryCallback(
	__in ULONG Row,
	__in ULONG Column,
	__in const UCHAR *Data,
	__in PVOID Context
	);

ULONG
MdbSetDefaultValues(
	__in PSTR Path
	);

VOID
MdbDumpMetaData(
	VOID
	);

ULONG
MdbGetSymbolPath(
    __in PWCHAR Buffer,
    __in ULONG Length
    );

#ifdef __cplusplus
}
#endif

#endif