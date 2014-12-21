//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "mdb.h"
#include "sql.h"
#include <stdlib.h>
#include "sdk.h"

PCSTR MdbName = "dprofile.mdb";
CHAR MdbPath[MAX_PATH];

PSQL_DATABASE MdbObject;
CRITICAL_SECTION MdbLock;

MDB_DATA_ITEM MdbData[] = {

	//
	// General
	//

	{ "MdbAutoAnalyze", NULL, MdbBoolean },

	//
	// Symbol
	//

	{ "MdbSymbolPath", NULL, MdbUtf8 },
	{ "MdbMsSymbolServer", NULL, MdbBoolean },

	//
	// Source 
	//
	
	{ "MdbSourcePath", NULL, MdbUtf8 },
	{ "MdbCodeEditor",  NULL, MdbUtf8 },

	//
	// View 
	//

	{ "MdbRecordLimit",  NULL, MdbInteger },

};

BOOLEAN 
MdbQueryCallback(
	__in ULONG Row,
	__in ULONG Column,
	__in const UCHAR *Data,
	__in PVOID Context
	)
{
	CHAR Buffer[MAX_PATH];

	if (!strcmp((PCSTR)Data, (PSTR)Context)) {
		StringCchPrintfA(Buffer, MAX_PATH, "MdbQueryCallback: (%d, %d) %s", 
			             Row, Column, Data);
		return TRUE;
	}

	return FALSE;
}

ULONG
MdbSetDefaultValues(
	__in PSTR Path
	)
{
	ULONG Status;
	CHAR Sql[MAX_PATH];

	if (!MdbObject) {
		Status = SqlOpen(Path, &MdbObject);
		if (Status != S_OK) {
			return Status;
		}
	}

	SqlBeginTransaction(MdbObject);

    //
    // Delete all values if available
    //

	StringCchCopyA(Sql, MAX_PATH, "DELETE FROM MetaData");
	SqlExecute(MdbObject, Sql);
	
	//
	// General
	//

	SqlExecute(MdbObject, "INSERT INTO MetaData VALUES(\"MdbAutoAnalyze\", \"1\")");

    //
    // Symbol
    //

	SqlExecute(MdbObject, "INSERT INTO MetaData VALUES(\"MdbSymbolPath\", \"\")");
	SqlExecute(MdbObject, "INSERT INTO MetaData VALUES(\"MdbMsSymbolServer\", \"1\")");

    //
    // Source
    //

	SqlExecute(MdbObject, "INSERT INTO MetaData VALUES(\"MdbSourcePath\", \"\")");
	SqlExecute(MdbObject, "INSERT INTO MetaData VALUES(\"MdbCodeEditor\", \"\")");

    //
    // View
    //

	SqlExecute(MdbObject, "INSERT INTO MetaData VALUES(\"MdbRecordLimit\", \"60\")");

	Status = SqlCommitTransaction(MdbObject);
    Status = Status ? S_OK : S_FALSE;
    return Status;
}

ULONG
MdbInitialize(
	__in ULONG Flag 
	)
{
	ULONG Status;

    UNREFERENCED_PARAMETER(Flag);

	GetCurrentDirectoryA(MAX_PATH, MdbPath);
	StringCchCatA(MdbPath, MAX_PATH, "\\"); 
	StringCchCatA(MdbPath, MAX_PATH, MdbName); 

	Status = SqlOpen(MdbPath, &MdbObject);
	if (Status != S_OK) {
		MdbObject = NULL;
		return STATUS_MDB_OPENFAILED;
	}

	InitializeCriticalSection(&MdbLock);

	Status = MdbLoadMetaData(MdbObject);
    if (Status != SQLITE_OK) {
        return S_FALSE;
    }

    return S_OK;
}

VOID
MdbClose(
	VOID
	)
{
	SqlClose(MdbObject);
	MdbObject = NULL;
	DeleteCriticalSection(&MdbLock);
}

ULONG
MdbLoadMetaData(
	__in PSQL_DATABASE SqlObject
	)
{
	ULONG Status;
	ULONG i;
	PSTR *Value;
	PSQL_TABLE Table; 
    SIZE_T Length;

	Status = SqlGetTable(SqlObject, "SELECT Value FROM MetaData", &Table);
	if (Status != SQLITE_OK) {
		return Status;
	}

    //
    // N.B SQLITE always return column name as first row,
    // NumberOfRows does not include the first row, we need
    // to skip it to locate the first real data row
    //

	Value = Table->Table;
    Value += 1;

	for(i = 0; i < Table->NumberOfRows; i += 1) {
        Length = strlen(Value[i]) + 1;
        MdbData[i].Value = (PSTR)ApsMalloc(Length);
        StringCchCopyA(MdbData[i].Value, Length, Value[i]);
	}

	SqlFreeTable(SqlObject, Table);

#ifdef _DEBUG
	MdbDumpMetaData();
#endif

	return Status;
}

PMDB_DATA_ITEM
MdbGetData(
	__in MDB_DATA_OFFSET Offset
	)
{
	PMDB_DATA_ITEM Item;

	EnterCriticalSection(&MdbLock);
	Item = &MdbData[Offset];
	LeaveCriticalSection(&MdbLock);

	return Item;
}

ULONG
MdbSetData(
	__in MDB_DATA_OFFSET Offset,
	__in PSTR Value
	)
{
	ULONG Status;
	PMDB_DATA_ITEM Item;
	PCHAR Buffer;
	SIZE_T Length;
	PSTR Sql;

	ASSERT(Value != NULL);

    //
    // Acquire mdb lock
    //

	EnterCriticalSection(&MdbLock);

	Item = &MdbData[Offset];
	if (Item->Value != NULL && !stricmp(Value, Item->Value)) {

        //
        // Value is identical, release lock and return
        //

		LeaveCriticalSection(&MdbLock);
		return S_OK;
	}

	Length = strlen(Value);
	Sql = (PSTR)ApsMalloc(Length + MAX_PATH);
	StringCchPrintfA(Sql, Length + MAX_PATH, "UPDATE MetaData SET Value=\"%s\" WHERE Key=\"%s\"", Value, Item->Key);

	ASSERT(MdbObject != NULL);

    //
    // Update the value in a SQL transaction
    //

	SqlBeginTransaction(MdbObject);
	Status = SqlExecute(MdbObject, Sql);
	if (Status != SQLITE_OK) {

        //
        // Transaction failed, rollback and release mdb lock
        //

		SqlRollbackTransaction(MdbObject);
		LeaveCriticalSection(&MdbLock);

		ApsFree(Sql);
		return S_FALSE;
	}

	SqlCommitTransaction(MdbObject);

#ifdef _DEBUG
	{
		SqlRegisterQueryCallback(MdbObject, MdbQueryCallback, Value);
		StringCchPrintfA(Sql, MAX_PATH, "SELECT Value FROM MetaData WHERE Key=\"%s\"", Item->Key);
		SqlQuery(MdbObject, Sql);
		SqlUnregisterQueryCallback(MdbObject, MdbQueryCallback);
	}
#endif

    //
    // Release old value buffer and assign new value
    //

	if (Item->Value != NULL) {
		ApsFree(Item->Value);
	}

	Length += 1;
	Buffer = (PCHAR)ApsMalloc(Length);
	StringCchCopyA(Buffer, Length, Value);
	Item->Value = Buffer;

	LeaveCriticalSection(&MdbLock);
	
	ApsFree(Sql);
	return S_OK;
}

VOID
MdbDumpMetaData(
	VOID
	)
{
	ULONG i;
	ULONG Number;
	CHAR Buffer[MAX_PATH * 2];

	Number = sizeof(MdbData) / sizeof(MDB_DATA_ITEM);
	EnterCriticalSection(&MdbLock);

	for(i = 0; i < Number; i++) {
		StringCchPrintfA(Buffer, MAX_PATH * 2, "#%02d %s = %s", i, MdbData[i].Key, MdbData[i].Value);	
        DebugTrace(Buffer);
	}

	LeaveCriticalSection(&MdbLock);
}

//
// Return the required buffer length
//

ULONG
MdbGetSymbolPath(
    __in PWCHAR Buffer,
    __in ULONG Length
    )
{
	PMDB_DATA_ITEM Item;
	size_t Count;
	ULONG RequiredLength;

	Buffer[0] = 0;
	Item = MdbGetData(MdbSymbolPath);

	//
	// Compute length of symbol path 
	//

	Count = strlen(Item->Value);
	if (!Count) {
		return 0;
	}

	//
	// Caller require the minimum buffer length to hold the 
	// symbol path, we need append a ';' at the end of symbol path
	//
	
	RequiredLength = (Count + 2) * sizeof(WCHAR);
	if (!Length) {
		return RequiredLength;
	}

	//
	// If the buffer is too small, return -1
	//

	if (RequiredLength > Length) {
		return (ULONG)-1;
	}

	StringCchPrintf(Buffer, Length, L"%S;", Item->Value);
	return RequiredLength;
}