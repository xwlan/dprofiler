//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include <windows.h>
#include "list.h"

VOID
InitializeListHead(
    IN PLIST_ENTRY ListHead
	)
{
	ListHead->Flink = ListHead->Blink = ListHead;
}

BOOLEAN
IsListEmpty(
	IN PLIST_ENTRY ListHead 
	)
{
	BOOLEAN IsEmpty;
	
	IsEmpty = (ListHead->Flink == ListHead);
	return IsEmpty;
}

BOOLEAN
RemoveEntryList(
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Flink->Blink = Blink;
    Blink->Flink = Flink;

    return (BOOLEAN)(Flink == Blink);
}

PLIST_ENTRY
RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead; 
    
    return Entry;
}

PLIST_ENTRY
RemoveTailList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;    

	return Entry;
}

VOID
InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    
    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

VOID 
InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}