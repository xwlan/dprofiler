//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
//

#ifndef _SQLAPI_H_
#define _SQLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "sqlite3.h"
#include "aps.h"

typedef ULONG SQL_STATUS;

//
// Flag to enable SQL profiling 
//

#define SQL_ENABLE_PROFILE  (ULONG)0x00000001 

//
// N.B. If the callback returns TRUE, the query process
// will be aborted.
//

typedef BOOLEAN 
(*SQL_QUERY_CALLBACK)(
	__in ULONG Row,
	__in ULONG Column,
	__in const UCHAR *Data,
	__in PVOID Context
	);

typedef VOID
(*SQL_PROFILE_CALLBACK)(
	__in PVOID Context,
	__in PCSTR Statement,
	__in ULONG64 Time 
	);

typedef struct _SQL_DATABASE {
	PSTR Name;	
	sqlite3 *SqlHandle;
	int LastErrorStatus;
	SQL_QUERY_CALLBACK QueryCallback;
	PVOID QueryContext;
} SQL_DATABASE, *PSQL_DATABASE;

//
// N.B. Table is composed of a M*N UTF8 string matrix,
// the first row is column names, and NumberOfRows does
// not include first row.
//

typedef struct _SQL_TABLE {
	PSTR *Table;
	ULONG NumberOfRows;
	ULONG NumberOfColumns;
} SQL_TABLE, *PSQL_TABLE;

typedef struct _SQL_PROFILE {
	ULONG64 MemoryAllocated;
	ULONG64 MemoryAllocatedMaximum;
	ULONG64 SqlExecuteAverage;
	ULONG64 SqlExecuteMaximum;
	PSTR SqlStatementMaximum;
} SQL_PROFILE, *PSQL_PROFILE;


PCSTR
SqlVersion(
	VOID
	);

ULONG
SqlInitialize(
	__in ULONG Flag 
	);

ULONG
SqlOpen(
	__in PCSTR Path,
	__out PSQL_DATABASE *Database
	);

ULONG
SqlClose(
	__in PSQL_DATABASE Database
	);

ULONG
SqlShutdown(
	VOID
	);

ULONG
SqlExecute(
	__in PSQL_DATABASE Database,
	__in PSTR Statement
	);

ULONG
SqlQuery(
	__in PSQL_DATABASE Database,
	__in PSTR Statement
	);

BOOLEAN
SqlIsOpen(
	__in PSQL_DATABASE Database
	);

BOOLEAN
SqlBeginTransaction(
	__in PSQL_DATABASE Database
	);

BOOLEAN
SqlCommitTransaction(
	__in PSQL_DATABASE Database
	);

BOOLEAN
SqlRollbackTransaction(
	__in PSQL_DATABASE Database
	);

//
// N.B. SqlGetTable is primarily for small size table,
// because it fetch all data into memory, it can have
// huge overhead if the table is big. For big table,
// register a sqlquery callback and call SqlQuery, the
// callback will get invoked for every column data,
// after usage, SqlFreeTable must be called to release
// allocated resources.
//

ULONG
SqlGetTable(
	__in PSQL_DATABASE Database,
	__in PSTR Statement,
	__out PSQL_TABLE *Table
	);

ULONG
SqlFreeTable(
	__in PSQL_DATABASE Database,
	__in PSQL_TABLE Table
	);

ULONG
SqlRegisterProfileCallback(
	__in PSQL_DATABASE Database,
	__in SQL_PROFILE_CALLBACK ProfileCallback,
	__in PVOID Context
	);

ULONG
SqlRegisterQueryCallback(
	__in PSQL_DATABASE Database,
	__in SQL_QUERY_CALLBACK Callback,
	__in PVOID Context
	);

ULONG
SqlUnregisterQueryCallback(
	__in PSQL_DATABASE Database,
	__in SQL_QUERY_CALLBACK Callback
	);

ULONG
SqlQueryProfile(
	__out PSQL_PROFILE Profile 
	);

ULONG CALLBACK
SqlProfileThread(
	__in PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif