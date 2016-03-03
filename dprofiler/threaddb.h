//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _THREADDB_H_
#define _THREADDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

#define MAX_THREAD_BUCKET 257 

#define THREAD_HASH_BUCKET(_T) \
	((_T) % (MAX_THREAD_BUCKET))

typedef struct _THREAD_ENTRY {
	LIST_ENTRY ListEntry;
	HANDLE ThreadHandle;
	ULONG ThreadId;
	ULONG SampleCount;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} THREAD_ENTRY, *PTHREAD_ENTRY;

typedef struct _THREAD_DATABASE {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	LIST_ENTRY ListHead[MAX_THREAD_BUCKET];
} THREAD_DATABASE, *PTHREAD_DATABASE;

ULONG
ThreadCreateDatabase(
	IN ULONG ProcessId,
	IN PTHREAD_DATABASE *Database
	);

PTHREAD_ENTRY
ThreadLookupDatabase(
	IN PTHREAD_DATABASE Database,
	IN ULONG ThreadId 
	);

PTHREAD_ENTRY
ThreadInsertDatabase(
	IN PTHREAD_DATABASE Database,
	IN ULONG ThreadId 
	);
	
#ifdef __cplusplus
}
#endif

#endif