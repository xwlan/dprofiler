//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "dprofiler.h"
#include "treelistdata.h"

PTREELIST_HASH_TABLE
TreeListInitData(
	IN TREELIST_FREE_CALLBACK FreeCallback,
	IN TREELIST_DATA_CALLBACK DataCallback,
	IN ULONG ColumnCount,
	IN ULONG BucketCount
	)
{
	ULONG Length;
	ULONG Number;
	PTREELIST_HASH_TABLE Table;

	if (!ColumnCount || !BucketCount || !FreeCallback || !DataCallback) {
		return NULL;
	}

	Length = FIELD_OFFSET(TREELIST_HASH_TABLE, ListHead[BucketCount]);
	Table = (PTREELIST_HASH_TABLE)SdkMalloc(Length);

	if (!Table) {
		return NULL;
	}

	Table->FreeCallback = FreeCallback;
	Table->DataCallback = DataCallback;
	Table->ColumnCount = ColumnCount;
	Table->BucketCount = BucketCount;

	for(Number = 0; Number < BucketCount; Number += 1) {
		InitializeListHead(&Table->ListHead[Number]);
	}

	return Table;
}
	
VOID
TreeListDestroyData(
	IN PTREELIST_HASH_TABLE Table
	)
{
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PTREELIST_DATA_ENTRY Data;
	ULONG Number;

	for(Number = 0; Number < Table->BucketCount; Number += 1) {

		ListHead = &Table->ListHead[Number];

		while (IsListEmpty(ListHead) != TRUE) {
			ListEntry = RemoveHeadList(ListHead);
			Data = CONTAINING_RECORD(ListEntry, TREELIST_DATA_ENTRY, ListEntry);
			TreeListFreeData(Table, Data);
		}

	}
}

PTREELIST_DATA_ENTRY
TreeListLookupData(
	IN PTREELIST_HASH_TABLE Table,
	IN HTREEITEM hTreeItem
	)
{
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PTREELIST_DATA_ENTRY Data;
	ULONG Bucket;

	Bucket = TREELIST_HASH_BUCKET(hTreeItem, Table);

	ListHead = &Table->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Data = CONTAINING_RECORD(ListEntry, TREELIST_DATA_ENTRY, ListEntry);
		if (Data->hTreeItem == hTreeItem) {
			return Data;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PTREELIST_DATA_ENTRY
TreeListInsertData(
	IN PTREELIST_HASH_TABLE Table,
	IN HTREEITEM hTreeItem,
	IN PVOID Value
	)
{
	PTREELIST_DATA_ENTRY Current;
	PTREELIST_DATA_ENTRY Data;
	ULONG Bucket;

	Current = TreeListLookupData(Table, hTreeItem);
	if (Current != NULL) {
		return NULL;
	}

	Data = (PTREELIST_DATA_ENTRY)SdkMalloc(sizeof(TREELIST_DATA_ENTRY));
	RtlZeroMemory(Data, sizeof(TREELIST_DATA_ENTRY));

	Data->hTreeItem = hTreeItem;
	Data->Value = Value; 

	Bucket = TREELIST_HASH_BUCKET(hTreeItem, Table);
	InsertTailList(&Table->ListHead[Bucket], &Data->ListEntry);
	return Data;
}

PVOID
TreeListRemoveData(
	IN PTREELIST_HASH_TABLE Table,
	IN HTREEITEM hTreeItem,
	IN BOOLEAN Free
	)
{
	PTREELIST_DATA_ENTRY Current;
	PVOID Value;

	Current = TreeListLookupData(Table, hTreeItem);
	if (!Current) {
		return NULL;
	}

	RemoveEntryList(&Current->ListEntry);

	if (Free) {
		TreeListFreeData(Table, Current);
		return NULL;
	}

	Value = Current->Value;
	SdkFree(Current);

	return Value;
}

VOID
TreeListFreeData(
	IN PTREELIST_HASH_TABLE Table,
	IN PTREELIST_DATA_ENTRY Data
	)
{
	if (!Data->Value) {
		return;
	}

	__try {
		(*Table->FreeCallback)(Data->Value);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	SdkFree(Data);
}

VOID
TreeListFormatData(
	IN PTREELIST_HASH_TABLE Table,
	IN PTREELIST_DATA_ENTRY Data,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	)
{
	if (!Data->Value) {
		Buffer[0] = 0;
		return;
	}

	__try {
		(*Table->DataCallback)(Data, Column, Buffer, Length);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}
}

VOID
TreeListFormatValue(
	IN struct _TREELIST_OBJECT *Object,
	IN TREELIST_FORMAT_CALLBACK Callback,
	IN HTREEITEM hTreeItem,
	IN PVOID Value,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	SIZE_T Length
	)
{
	Buffer[0] = 0;

	if (!Value) {
		return;
	}

	__try {
		(*Callback)(Object, hTreeItem, Value, Column, Buffer, Length);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}
}