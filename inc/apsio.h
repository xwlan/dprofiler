//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2015
//

#ifndef _APS_IO_H_
#define _APS_IO_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "apsctl.h"
#include "apspdb.h"

#define IO_INVALID_STACTRACE  ((ULONG)-1)

typedef struct _IO_STACKTRACE{
	ULONG StackId;
	ULONG Count;
	ULONG Next;
	IO_OPERATION Operation;
	ULONG64 CompleteBytes;
} IO_STACKTRACE, *PIO_STACKTRACE;

typedef struct _IO_THREAD {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	ULONG FirstIrp;
	ULONG IrpCount;
	union {
		PVOID Context;
		PIO_STACKTRACE Trace;
		ULONG CurrentIrp;
	};
	IO_COUNTER_METHOD File[IO_OP_NUMBER];
	IO_COUNTER_METHOD Socket[IO_OP_NUMBER];
} IO_THREAD, *PIO_THREAD;

#define IO_THREAD_BUCKET 233

typedef struct _IO_THREAD_TABLE {
	ULONG Count;
	LIST_ENTRY ThreadList[IO_THREAD_BUCKET];
} IO_THREAD_TABLE, *PIO_THREAD_TABLE;

typedef struct _IO_HISTORY{
	FLOAT *Value;
	FLOAT MinimumValue;
	FLOAT MaximumValue;
	FLOAT Limits; 
    ULONG Count;
} IO_HISTORY, *PIO_HISTORY;

ULONG
ApsIoNormalizeRecords(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle
	);

PCSTR
ApsIoGetOpString(
	_In_ int Op
	);

ULONG
ApsIoCreateCounters(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	);

PIO_OBJECT_ON_DISK
ApsIoLookupObjectById(
	_In_ PIO_OBJECT_ON_DISK Base,
	_In_ ULONG Count,
	_In_ ULONG Id
	);

PIO_IRP_ON_DISK
ApsIoLookupIrpById(
	_In_ PIO_IRP_ON_DISK Base,
	_In_ ULONG Count,
	_In_ ULONG Id
	);

VOID
ApsIoInsertThreadedIrp(
	_In_ PIO_THREAD_TABLE Table,
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ PIO_OBJECT_ON_DISK Object
	);

PIO_THREAD
ApsIoLookupThread(
	_In_ PIO_THREAD_TABLE Table,
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ BOOLEAN Allocate
	);

VOID
ApsIoCreateThreadTable(
	_Inout_ PIO_THREAD_TABLE *Table
	);

PIO_THREAD
ApsIoAllocateThread(
	_In_ ULONG ThreadId
	);

VOID
ApsIoUpdateObjectCounters(
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ PIO_OBJECT_ON_DISK Object
	);

VOID
ApsIoUpdateThreadCounters(
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ PIO_OBJECT_ON_DISK Object,
	_In_ PIO_THREAD Thread
	);

ULONG
ApsIoBuildObjectStackTrace(
	_In_ PPF_REPORT_HEAD Head,
	_In_ PIO_OBJECT_ON_DISK Object
	);

ULONG
ApsIoBuildThreadStackTrace(
	_In_ PPF_REPORT_HEAD Head,
	_In_ PIO_THREAD Thread
	);

VOID
ApsIoFreeAttachedStackTrace(
	_In_ PIO_STACKTRACE Trace
	);

#ifdef __cplusplus 
}
#endif
#endif