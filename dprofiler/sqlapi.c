//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "sqlapi.h"
#include "aps.h"
#include <strsafe.h>
#include <assert.h>

ULONG SqlProfileThreadId;
SQL_PROFILE SqlProfile;
CRITICAL_SECTION SqlProfileLock;

PCSTR
SqlVersion(
	VOID
	)
{
	PCSTR Version;
	Version = sqlite3_libversion();
	return Version;
}

ULONG
SqlInitialize(
	__in ULONG Flag 
	)
{
	HANDLE Handle;

	InitializeCriticalSection(&SqlProfileLock);

    if (FlagOn(Flag, SQL_ENABLE_PROFILE)) {
        Handle = CreateThread(NULL, 0, SqlProfileThread, NULL,
                              0, &SqlProfileThreadId);
        CloseHandle(Handle);
    }

	return SQLITE_OK;
}

ULONG
SqlOpen(
	__in PCSTR Path,
	__out PSQL_DATABASE *Database
	)
{
	PSQL_DATABASE Object;
	sqlite3 *SqlHandle;
    SIZE_T Length;
	int Status;

    *Database = NULL;
	Status = sqlite3_open(Path, &SqlHandle);

	if (Status == SQLITE_OK) {

		Object = (PSQL_DATABASE)ApsMalloc(sizeof(SQL_DATABASE));
		Object->SqlHandle = SqlHandle;

        Length = strlen(Path) + 1;
        Object->Name = (PSTR)ApsMalloc(Length);
        StringCchCopyA(Object->Name, Length, Path); 
		*Database = Object;
	}

    //
    // N.B. Even if open failed, the SqlHandle can be valid,
    // we must explicitly close the connection
    //

    else {
        if (SqlHandle) {
            sqlite3_close(SqlHandle);
        }
    }

	return Status;
}

ULONG
SqlClose(
	__in PSQL_DATABASE Database
	)
{
	int Status;

    ASSERT(Database != NULL);
    ASSERT(Database->SqlHandle != NULL);

    //
    // N.B. If error occurs when close SQLITE,
    // we free the database object anyway
    //

	Status = sqlite3_close(Database->SqlHandle);
	ApsFree(Database);
	return Status;
}

ULONG
SqlShutdown(
	VOID
	)
{
	ULONG Status;

	Status = sqlite3_shutdown();
	return Status;
}

ULONG
SqlExecute(
	__in PSQL_DATABASE Database,
	__in PSTR Statement
	)
{
	PCHAR Message;
	ULONG Status;

	ASSERT(SqlIsOpen(Database));

	Message = NULL;
	Status = sqlite3_exec(Database->SqlHandle, Statement, NULL, NULL, &Message);
	Database->LastErrorStatus = Status;

	if (Message != NULL) {
		sqlite3_free(Message);
	}

	return Status;
}

BOOLEAN
SqlIsOpen(
	__in PSQL_DATABASE Database
	)
{
    ASSERT(Database != NULL);

	if (!Database->SqlHandle) {
		Database->LastErrorStatus = SQLITE_ERROR;
		return FALSE;
	}

	return TRUE;
}

ULONG
SqlQuery(
	__in PSQL_DATABASE Database,
	__in PSTR Statement
	)
{
	int Status;
	int ColumnNumber;
	int RowNumber;
	sqlite3_stmt *stmt;
	const UCHAR *Text;
	int i;
	BOOLEAN Abort;

	ASSERT(SqlIsOpen(Database));

	Status = sqlite3_prepare_v2(Database->SqlHandle, Statement, -1, &stmt, NULL);
	if(Status != SQLITE_OK){
		Database->LastErrorStatus = Status;
		return Status;
	}
		
	Abort = FALSE;
	RowNumber = 0;
	ColumnNumber = sqlite3_column_count(stmt);
	
	do{
		Status = sqlite3_step(stmt);

		switch (Status) {
		   case SQLITE_DONE:
			   break;

		   case SQLITE_ROW:
			   for(i = 0; i < ColumnNumber; i++){
				   
				   //
				   // N.B. Data pointer will be invalid for next round,
				   // when callback return, the Data pointer is considered
				   // invalid.
				   //

				   Text = (const UCHAR *)sqlite3_column_text(stmt, i);
				   Abort = (*Database->QueryCallback)(RowNumber, i, Text, Database->QueryContext);
			   }
			   break;

		   default:
			   break;
		}

	} while (Status == SQLITE_ROW && Abort != TRUE);

	sqlite3_finalize(stmt);
	return Status;
}

BOOLEAN
SqlBeginTransaction(
	__in PSQL_DATABASE Database
	)
{
	ULONG Status;

	Status = SqlExecute(Database, "BEGIN TRANSACTION");
	return (Status == SQLITE_OK) ? TRUE : FALSE;
}

BOOLEAN
SqlCommitTransaction(
	__in PSQL_DATABASE Database
	)
{
	ULONG Status;

	Status = SqlExecute(Database, "COMMIT TRANSACTION");
	return (Status == SQLITE_OK) ? TRUE : FALSE;
}

BOOLEAN
SqlRollbackTransaction(
	__in PSQL_DATABASE Database
	)
{
	ULONG Status;

	Status = SqlExecute(Database, "ROLLBACK TRANSACTION");
	return (Status == SQLITE_OK) ? TRUE : FALSE;
}

ULONG
SqlGetTable(
	__in PSQL_DATABASE Database,
	__in PSTR Statement,
	__out PSQL_TABLE *Table
	)
{
	CHAR **Result; 
	PCHAR Message;
	int NumberOfRows;
	int NumberOfColumns;
	PSQL_TABLE Object;
	int Status;

	Message = NULL;
	Status = sqlite3_get_table(Database->SqlHandle, Statement, &Result, 
		                       &NumberOfRows, &NumberOfColumns, &Message);

	if (Status != SQLITE_OK) {
		Database->LastErrorStatus = Status;
		return Status;
	}

	Object = (PSQL_TABLE)ApsMalloc(sizeof(SQL_TABLE));
	Object->Table = Result;
	Object->NumberOfRows = NumberOfRows;
	Object->NumberOfColumns = NumberOfColumns;

	*Table = Object;
    return SQLITE_OK;
}

ULONG
SqlFreeTable(
	__in PSQL_DATABASE Database,
	__in PSQL_TABLE Table
	)
{
	sqlite3_free_table(Table->Table);
	ApsFree(Table);
	return SQLITE_OK;
}

ULONG
SqlRegisterQueryCallback(
	__in PSQL_DATABASE Database,
	__in SQL_QUERY_CALLBACK Callback,
	__in PVOID Context
	)
{
	Database->QueryCallback = Callback;
	Database->QueryContext = Context;
	return SQLITE_OK;
}

ULONG
SqlUnregisterQueryCallback(
	__in PSQL_DATABASE Database,
	__in SQL_QUERY_CALLBACK Callback
	)
{
	Database->QueryCallback = NULL;
	return SQLITE_OK;
}

ULONG
SqlRegisterProfileCallback(
	__in PSQL_DATABASE Database,
	__in SQL_PROFILE_CALLBACK ProfileCallback,
	__in PVOID Context
	)
{
	sqlite3_profile(Database->SqlHandle, ProfileCallback, Context);
	return SQLITE_OK;
}

ULONG
SqlQueryProfile(
	__out PSQL_PROFILE Profile 
	)
{
	EnterCriticalSection(&SqlProfileLock);
	RtlCopyMemory(Profile, &SqlProfile, sizeof(SqlProfile));
	LeaveCriticalSection(&SqlProfileLock);

	return SQLITE_OK;
}

ULONG CALLBACK
SqlProfileThread(
	__in PVOID Context
	)
{
	PSQL_DATABASE Database;
	
	Database = (PSQL_DATABASE)Context;
	UNREFERENCED_PARAMETER(Database);

	return SQLITE_OK;
}