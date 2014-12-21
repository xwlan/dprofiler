//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _CACHE_H_
#define _CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "hal.h"
#include "lock.h"
#include "thread.h"

typedef struct _BTR_MAPPING_OBJECT {
	HANDLE Object;
	PVOID MappedVa;
	ULONG MappedLength;
	ULONG64 MappedOffset;
	ULONG64 FileLength;
	USHORT ObjectFlag;
	PBTR_THREAD_OBJECT Thread;
} BTR_MAPPING_OBJECT, *PBTR_MAPPING_OBJECT;

ULONG
BtrInitializeCache(
	__in PBTR_PROFILE_OBJECT Object	
	);

VOID
BtrUninitializeCache(
	VOID
	);

VOID
BtrCloseCache(
	VOID
	);

ULONG
BtrCreateMappingPerThread(
	__in PBTR_THREAD_OBJECT Thread
	);

ULONG
BtrCloseMappingPerThread(
	__in PBTR_THREAD_OBJECT Thread
	);

ULONG
BtrQueueRecord(
	__in PBTR_THREAD_OBJECT Thread,
	__in PBTR_RECORD_HEADER Header
	);

ULONG
BtrAcquireWritePosition(
	__in ULONG BytesToWrite,
	__out PULONG64 Position
	);

ULONG
BtrWriteRecord(
	__in PBTR_THREAD_OBJECT Thread,
	__in PVOID Record,
	__in ULONG Length,
	__out PULONG64 Offset
	);

ULONG
BtrWriteIndex(
	__in PBTR_THREAD_OBJECT Thread,
	__in ULONG64 RecordOffset,
	__in ULONG64 Location,
	__in ULONG Size
	);

ULONG
BtrCopyMemory(
	__in PVOID Destine,
	__in PVOID Source,
	__in ULONG Length 
	);

ULONG
BtrExtendFileLength(
	__in HANDLE File,
	__in ULONG64 Length
	);

ULONG
BtrAcquireSequence(
	VOID
	);

ULONG
BtrCurrentSequence(
	VOID
	);

ULONG
BtrAcquireObjectId(
	VOID
	);

ULONG 
BtrAcquireStackId(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif 