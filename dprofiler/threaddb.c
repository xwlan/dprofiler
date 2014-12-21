//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "bsp.h"
#include "list.h"
#include <psapi.h>
#include "dprofiler.h"
#include "threaddb.h"

ULONG
ThreadCreateDatabase(
	IN ULONG ProcessId,
	IN PTHREAD_DATABASE *Database
	)
{
	PTHREAD_DATABASE Thread;
	LIST_ENTRY ListHead;
	PTHREAD_ENTRY Entry;
	SYSTEM_THREAD_INFORMATION *Info;
	ULONG Number;
	ULONG Bucket;
	ULONG Status;
	HANDLE Handle;
	ULONG ThreadId;
	PBSP_PROCESS Process;

	InitializeListHead(&ListHead);
	Status = BspQueryThreadList(ProcessId, &Process);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Thread = (PTHREAD_DATABASE)BspMalloc(sizeof(THREAD_DATABASE));
	Thread->ProcessId = ProcessId;
	
	for(Number = 0; Number < MAX_THREAD_BUCKET; Number += 1) {
		InitializeListHead(&Thread->ListHead[Number]);
	}

	for(Number = 0; Number < Process->ThreadCount; Number += 1) {

		Info = &Process->Thread[Number];
		ThreadId = HandleToUlong(Info->ClientId.UniqueThread);

		//
		// Skip the thread if failed to access it
		//

		Handle = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadId);
		if (!Handle) {
			continue;
		}

		Entry = (PTHREAD_ENTRY)BspMalloc(sizeof(THREAD_ENTRY));
		Entry->ThreadId = ThreadId;
		Entry->ThreadHandle = Handle;
		QueryPerformanceCounter(&Entry->CreationTime);

		Bucket = THREAD_HASH_BUCKET(ThreadId);
		InsertTailList(&Thread->ListHead[Bucket], &Entry->ListEntry);
	}

	BspFreeProcess(Process);
	*Database = Thread;
	return PF_STATUS_OK;
}

PTHREAD_ENTRY
ThreadLookupDatabase(
	IN PTHREAD_DATABASE Database,
	IN ULONG ThreadId 
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PTHREAD_ENTRY Entry;

	Bucket = THREAD_HASH_BUCKET(ThreadId);
	ListHead = &Database->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, THREAD_ENTRY, ListEntry);
		if (Entry->ThreadId == ThreadId) {
			return Entry;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PTHREAD_ENTRY
ThreadInsertDatabase(
	IN PTHREAD_DATABASE Database,
	IN ULONG ThreadId 
	)
{
	ULONG Bucket;
	PTHREAD_ENTRY Entry;
	HANDLE Handle;

#ifdef _DEBUG
	Entry = ThreadLookupDatabase(Database, ThreadId);
	ASSERT(Entry == NULL);
#endif

	Handle = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadId);
	if (!Handle) {
		return NULL;
	}

	Entry = (PTHREAD_ENTRY)BspMalloc(sizeof(THREAD_ENTRY));
	Entry->ThreadId = ThreadId;
	Entry->ThreadHandle = Handle;
	QueryPerformanceCounter(&Entry->CreationTime);

	Bucket = THREAD_HASH_BUCKET(ThreadId);
	InsertTailList(&Database->ListHead[Bucket],
		           &Entry->ListEntry);
	return Entry;
}