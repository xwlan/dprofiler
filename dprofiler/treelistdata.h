//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _TREELISTDATA_H_
#define _TREELISTDATA_H_

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TREELIST_DATA_ENTRY {
	LIST_ENTRY ListEntry;
	HTREEITEM hTreeItem;
	PVOID Value;
} TREELIST_DATA_ENTRY, *PTREELIST_DATA_ENTRY;

typedef BOOLEAN 
(CALLBACK *TREELIST_FREE_CALLBACK)(
	IN PVOID Value
	);

typedef VOID
(CALLBACK *TREELIST_DATA_CALLBACK)(
	IN PTREELIST_DATA_ENTRY Data,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

typedef VOID
(CALLBACK *TREELIST_FORMAT_CALLBACK)(
	IN struct _TREELIST_OBJECT *TreeList,
	IN HTREEITEM hTreeItem,
	IN PVOID Value,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

typedef struct _TREELIST_HASH_TABLE {
	ULONG ColumnCount;  
	ULONG BucketCount;
	TREELIST_FREE_CALLBACK FreeCallback;
	TREELIST_DATA_CALLBACK DataCallback;
	LIST_ENTRY ListHead[ANYSIZE_ARRAY];
} TREELIST_HASH_TABLE, *PTREELIST_HASH_TABLE;

//
// Simple modulo hash to locate item's data bucket
// _H, hTreeItem
// _T, pointer to hash table 
//

#define TREELIST_HASH_BUCKET(_H, _T) \
	((ULONG_PTR)_H % _T->BucketCount)

PTREELIST_HASH_TABLE
TreeListInitData(
	IN TREELIST_FREE_CALLBACK FreeCallback,
	IN TREELIST_DATA_CALLBACK DataCallback,
	IN ULONG ColumnCount,
	IN ULONG BucketCount
	);
	
VOID
TreeListDestroyData(
	IN PTREELIST_HASH_TABLE Table
	);

PTREELIST_DATA_ENTRY
TreeListLookupData(
	IN PTREELIST_HASH_TABLE Table,
	IN HTREEITEM hTreeItem
	);

PTREELIST_DATA_ENTRY
TreeListInsertData(
	IN PTREELIST_HASH_TABLE Table,
	IN HTREEITEM hTreeItem,
	IN PVOID Value
	);

PVOID
TreeListRemoveData(
	IN PTREELIST_HASH_TABLE Table,
	IN HTREEITEM hTreeItem,
	IN BOOLEAN Free
	);

VOID
TreeListFreeData(
	IN PTREELIST_HASH_TABLE Table,
	IN PTREELIST_DATA_ENTRY Data
	);

VOID
TreeListFormatData(
	IN PTREELIST_HASH_TABLE Table,
	IN PTREELIST_DATA_ENTRY Data,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

VOID
TreeListFormatValue(
	IN struct _TREELIST_OBJECT *Object,
	IN TREELIST_FORMAT_CALLBACK Callback,
	IN HTREEITEM hTreeItem,
	IN PVOID Value,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	SIZE_T Length
	);

#ifdef __cplusplus
}
#endif

#endif 