//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2015
//

#include "aps.h"
#include "apsdefs.h"
#include "apspri.h"
#include "apsctl.h"
#include "apsbtr.h"
#include "apsfile.h"
#include "ntapi.h"
#include "apspdb.h"
#include "apsrpt.h"
#include "apsio.h"

ULONG
ApsCheckIoAttribute(
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	return APS_STATUS_OK;
}

ULONG
ApsCreateIoProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	PAPS_PROFILE_OBJECT Profile;

	*Object = NULL;

	Attribute->Type = PROFILE_IO_TYPE;
	Attribute->Mode = Mode;

	//
	// Validate profiling attributes
	//

	Status = ApsCheckIoAttribute(Mode, Attribute);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Open target process object
	//

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}

	//
	// Create profile object
	//

	Status = ApsCreateProfile(&Profile, Mode, PROFILE_IO_TYPE, 
		                      ProcessId, ProcessHandle, ReportPath);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Copy attribute to local profile object
	//

	ASSERT(Profile != NULL);
	memcpy(&Profile->Attribute, Attribute, sizeof(BTR_PROFILE_ATTRIBUTE));

	*Object = Profile;
	return Status;
}

int __cdecl 
ApsIoObjectSortCallback(
	__in const void *Entry1,
	__in const void *Entry2 
	)
{
    //
    // N.B. we need a increasing order
    //

	return ((PIO_OBJECT_ON_DISK)Entry1)->ObjectId - ((PIO_OBJECT_ON_DISK)Entry2)->ObjectId;
}

int __cdecl 
ApsIoIrpSortCallback(
	__in const void *Entry1,
	__in const void *Entry2 
	)
{
    //
    // N.B. we need a increasing order
    //

	return ((PIO_IRP_ON_DISK)Entry1)->RequestId - ((PIO_IRP_ON_DISK)Entry2)->RequestId;
}

int __cdecl
ApsIoSortStackTraceCallback(
	__in const void *Entry1,
	__in const void *Entry2 
	)
{
    //
    // N.B. we need a increasing order
    //

	return ((PIO_STACKTRACE)Entry1)->StackId - ((PIO_STACKTRACE)Entry2)->StackId;
}

VOID
ApsIoDumpObject(
	_In_ int Count,
	_In_ PIO_OBJECT_ON_DISK Object
	)
{
	int i;

	for(i = 0; i < Count; i++) {
		if (Object[i].Flags & OF_FILE){
			ApsTrace("OBJECT ID: #%d => %d, FILE",  i, Object[i].ObjectId);
		}
		else if (Object[i].Flags & (OF_SKIPV4|OF_SKIPV6)){
			ApsTrace("OBJECT ID: #%d => %d, SOCKET",  i, Object[i].ObjectId);
		}
		else {
			ApsTrace("OBJECT ID: #%d => %d, TYPE N/A",  i, Object[i].ObjectId);
		}
	}
}

#pragma message(__LOW__"Ensure IO_OPERATION enum match ApsIoOpString!")

PCSTR ApsIoOpString[] = {
	"Invalid",
	"Create",
	"Connect",
	"Accept",
	"IoControl",
	"FsControl",
	"Read", 
	"Write",
	"Close",
	"Invalid",
};

PCSTR
ApsIoGetOpString(
	_In_ int Op
	)
{
	if (Op < (int)IO_OP_INVALID || Op >= (int)IO_OP_NUMBER) {
		return "Invalid";
	}
	return ApsIoOpString[Op];
}

VOID
ApsIoDumpIrp(
	_In_ int Count,
	_In_ PIO_IRP_ON_DISK Irp 
	)
{
	int i;

	for(i = 0; i < Count; i++) {
		ApsDebugTrace("IRP ID: #%d => %d, OP=%s",  i, 
						Irp[i].RequestId, 
						ApsIoOpString[Irp->Operation]);
	}
}

ULONG
ApsIoNormalizeRecords(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle
	)
{
	ULONG Count;
	PIO_IRP_ON_DISK Irp;
	PIO_OBJECT_ON_DISK Object;

	Object = (PIO_OBJECT_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_OBJECT);
	Count = ApsGetStreamRecordCount(Head, STREAM_IO_OBJECT, IO_OBJECT_ON_DISK);
	qsort(Object, Count, sizeof(IO_OBJECT_ON_DISK), ApsIoObjectSortCallback); 

#ifdef _DEBUG
	ApsIoDumpObject(Count, Object);
#endif

	//
	// N.B. There maybe some missing irps.
	//

	Irp = (PIO_IRP_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_IRP);
	Count = ApsGetStreamRecordCount(Head, STREAM_IO_IRP, IO_IRP_ON_DISK);
	qsort(Irp, Count, sizeof(IO_IRP_ON_DISK), ApsIoIrpSortCallback); 
	
#ifdef _DEBUG
	ApsIoDumpIrp(Count, Irp);
#endif

	return S_OK;
}

//
// Object ID is zero based
//

PIO_OBJECT_ON_DISK
ApsIoLookupObjectById(
	_In_ PIO_OBJECT_ON_DISK Base,
	_In_ ULONG Count,
	_In_ ULONG Id
	)
{
	ULONG Number;

	if (Id < Count) {

		if (Base[Id].ObjectId == Id) {
			return &Base[Id];
		}

		else if (Base[Id].ObjectId < Id) {

			//
			// This indicate there's duplicated objects
			//

			ASSERT(0);
			for (Number = Id + 1; Number < Count; Number += 1) {
				if (Base[Number].ObjectId == Id) {
					return &Base[Number];
				}
			}
		}
		else {

			//
			// BaseId[Id].ObjectId > Id, this indicate there's missed object
			//

			ASSERT(0);
			if (Id != 0) { 
				for(Number = Id - 1; Number != (ULONG)-1; Number -= 1) {
					if (Base[Number].ObjectId == Id) {
						return &Base[Number];
					}
				}
			} 
		}
	}
	else {
	
		//
		// The object's id is beyond the number of objects, scan reversely
		// to do best effort
		//
	
		ASSERT(0);
		for(Number = Count - 1; Number != (ULONG)-1; Number -= 1) {
			if (Base[Number].ObjectId == Id) {
				return &Base[Number];
			}
		}
	}

	return NULL;
}

PIO_IRP_ON_DISK
ApsIoLookupIrpById(
	_In_ PIO_IRP_ON_DISK Base,
	_In_ ULONG Count,
	_In_ ULONG Id
	)
{
	return NULL;
}

VOID
ApsIoCreateThreadTable(
	_Inout_ PIO_THREAD_TABLE *ThreadTable
	)
{
	ULONG Number;
	PIO_THREAD_TABLE Table;

	Table = (PIO_THREAD_TABLE)ApsMalloc(sizeof(IO_THREAD_TABLE));
	Table->Count = 0;

	for(Number = 0; Number < IO_THREAD_BUCKET; Number += 1) { 
		InitializeListHead(&Table->ThreadList[Number]);
	}

	*ThreadTable = Table;
}

VOID
ApsIoDestroyThreadTable(
	_Inout_ PIO_THREAD_TABLE ThreadTable
	)
{
	ASSERT(ThreadTable->Count == 0);
	ApsFree(ThreadTable);
}

PIO_THREAD
ApsIoAllocateThread(
	_In_ ULONG ThreadId
	)
{
	PIO_THREAD Thread;
	Thread = (PIO_THREAD)ApsMalloc(sizeof(IO_THREAD));
	ZeroMemory(Thread, sizeof(IO_THREAD));
	Thread->ThreadId = ThreadId;
	Thread->FirstIrp = IO_INVALID_IRP_INDEX;
	Thread->CurrentIrp = IO_INVALID_IRP_INDEX;
	return Thread;
}

VOID
ApsIoUpdateObjectCounters(
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ PIO_OBJECT_ON_DISK Object
	)
{
	PIO_COUNTER_METHOD Method;

	ASSERT(Irp->ObjectId == Object->ObjectId);
	ASSERT(Irp->Operation > IO_OP_INVALID && Irp->Operation < IO_OP_NUMBER);

	Method = &Object->Counters[Irp->Operation];
	Method->RequestCount += 1;

	if (Irp->Complete) {

		Method->CompleteSize += Irp->CompletionBytes;
		Method->CompleteCount += 1;

		if (Method->RequestCount > 1) {
			Method->MinimumLatency = min((ULONG)Irp->Duration.QuadPart, Method->MinimumLatency); 
			Method->MaximumLatency = max((ULONG)Irp->Duration.QuadPart, Method->MaximumLatency); 
			Method->AverageLatency = (Method->AverageLatency * (Method->RequestCount - 1) + (ULONG)Irp->Duration.QuadPart) / Method->RequestCount;
			Method->MinimumSize = min((ULONG)Irp->CompletionBytes, Method->MinimumSize);
			Method->MaximumSize = max((ULONG)Irp->CompletionBytes, Method->MaximumSize);
			Method->AverageSize = (Method->AverageSize * (Method->RequestCount - 1) + Irp->CompletionBytes) / Method->RequestCount;
		} else {
			Method->MinimumLatency = (ULONG)Irp->Duration.QuadPart;
			Method->MaximumLatency = Method->MinimumLatency;
			Method->AverageLatency = Method->MinimumLatency;
			Method->MinimumSize = Irp->CompletionBytes;
			Method->MaximumSize = Irp->CompletionBytes;
			Method->AverageSize = Irp->CompletionBytes;
		}

	}
	else if (Irp->Aborted) {
		Method->FailureSize += Irp->RequestBytes;
		Method->FailureCount += 1;
	}
	if (Irp->Asynchronous) {
		Method->AsynchronousCount += 1;
	} else {
		Method->SynchronousCount += 1;
	}
}

VOID
ApsIoUpdateThreadCounters(
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ PIO_OBJECT_ON_DISK Object,
	_In_ PIO_THREAD Thread
	)
{
	PIO_COUNTER_METHOD Method;

	if (FlagOn(Object->Flags, OF_FILE)) {
		Method = &Thread->File[Irp->Operation];
	}
	else if (FlagOn(Object->Flags, (OF_SKIPV4|OF_SKIPV6))) {
		Method = &Thread->Socket[Irp->Operation];
	}
	else {
		ASSERT(0);
		return;
	}
	
	Method->RequestCount += 1;
	ASSERT(Thread->IrpCount >= 1);

	if (Irp->Complete) {
		Method->CompleteSize += Irp->CompletionBytes;
		Method->CompleteCount += 1;

		if (Thread->IrpCount > 1) {
			Method->MinimumLatency = min((ULONG)Irp->Duration.QuadPart, Method->MinimumLatency); 
			Method->MaximumLatency = max((ULONG)Irp->Duration.QuadPart, Method->MaximumLatency); 
			Method->AverageLatency = (Method->AverageLatency * Thread->IrpCount - 1 + (ULONG)Irp->Duration.QuadPart) / Thread->IrpCount;
			Method->MinimumSize = min((ULONG)Irp->CompletionBytes, Method->MinimumSize);
			Method->MaximumSize = max((ULONG)Irp->CompletionBytes, Method->MaximumSize);
			Method->AverageSize = (Method->AverageSize * Thread->IrpCount - 1 + Irp->CompletionBytes) / Thread->IrpCount;
		} else {
			Method->MinimumLatency = (ULONG)Irp->Duration.QuadPart;
			Method->MaximumLatency = Method->MinimumLatency;
			Method->AverageLatency = Method->MinimumLatency;
			Method->MinimumSize = Irp->CompletionBytes;
			Method->MaximumSize = Irp->CompletionBytes;
			Method->AverageSize = Irp->CompletionBytes;
		}

	}
	else if (Irp->Aborted) {
		Method->FailureSize += Irp->RequestBytes;
		Method->FailureCount += 1;
	}
	if (Irp->Asynchronous) {
		Method->AsynchronousCount += 1;
	} else {
		Method->SynchronousCount += 1;
	}
}

PIO_THREAD
ApsIoLookupThread(
	_In_ PIO_THREAD_TABLE Table,
	_In_ PIO_IRP_ON_DISK Irp,
	_In_ BOOLEAN Allocate
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PIO_THREAD Thread;

	Bucket = Irp->RequestThreadId % IO_THREAD_BUCKET;
	ListHead = &Table->ThreadList[Bucket];
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Thread = CONTAINING_RECORD(ListEntry, IO_THREAD, ListEntry);	
		if (Thread->ThreadId == Irp->RequestThreadId) {
			return Thread;
		}
		ListEntry = ListEntry->Flink;
	}

	if (!Allocate) {
		return NULL;
	}

	//
	// If thread is not found, allocate and insert into bucket
	//

	Thread = ApsIoAllocateThread(Irp->RequestThreadId);
	InsertHeadList(ListHead, &Thread->ListEntry);
	Table->Count += 1;
	return Thread;
}

ULONG
ApsIoCreateCounters(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG Status;
	ULONG Count;
	ULONG NumberOfObject;
	PIO_IRP_ON_DISK Irp;
	PIO_OBJECT_ON_DISK Object;
	PIO_OBJECT_ON_DISK Target;
	PIO_THREAD_TABLE Table;
	ULONG i, j;
	PLIST_ENTRY ListEntry;
	PIO_THREAD Thread;
	PIO_THREAD Thread2;
	ULONG ThreadSize;
	ULONG Complete;

	Object = (PIO_OBJECT_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_OBJECT);
	NumberOfObject = ApsGetStreamRecordCount(Head, STREAM_IO_OBJECT, IO_OBJECT_ON_DISK);

	ASSERT(Object != NULL);
	ASSERT(NumberOfObject != 0);
	
	ApsIoCreateThreadTable(&Table);
	ASSERT(Table != NULL);

	Irp = (PIO_IRP_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_IRP);
	Count = ApsGetStreamRecordCount(Head, STREAM_IO_IRP, IO_IRP_ON_DISK);

	for(i = 0; i < Count; i += 1) {

		Target = ApsIoLookupObjectById(Object, NumberOfObject, Irp[i].ObjectId);
		if (!Target) {
			ASSERT(0);
			continue;
		}

		Target->IrpCount += 1;

		//
		// Build irp chain of target object
		//

		Irp[i].NextIrp = IO_INVALID_IRP_INDEX;

		if (Target->IrpCount > 1) {
			Irp[Target->CurrentIrp].NextIrp = i;
		}
		else {
			Target->FirstIrp = i;
		}

		Target->CurrentIrp = i;
		ApsIoUpdateObjectCounters(&Irp[i], Target);

		//
		// Chian the irp by its requestor thread id
		//

		Irp->ThreadedNextIrp = IO_INVALID_IRP_INDEX;
		Thread = ApsIoLookupThread(Table, &Irp[i], TRUE);
		Thread->IrpCount += 1;

		if (Thread->IrpCount > 1) {
			ASSERT(Thread->FirstIrp != IO_INVALID_IRP_INDEX);
			ASSERT(Thread->CurrentIrp != IO_INVALID_IRP_INDEX);
			Irp[Thread->CurrentIrp].ThreadedNextIrp = i;
		} else {
			Thread->FirstIrp = i;
		}

		Thread->CurrentIrp = i;
		ApsIoUpdateThreadCounters(&Irp[i], Target, Thread);
	}

	//
	// Serialize the threads
	//

	ThreadSize = Table->Count * sizeof(IO_THREAD);
	Thread2 = (PIO_THREAD)ApsMalloc(ThreadSize);

	for(i = 0, j = 0; i < IO_THREAD_BUCKET; i += 1) {

		while (!IsListEmpty(&Table->ThreadList[i])) {
			ListEntry = RemoveHeadList(&Table->ThreadList[i]);
			Thread = CONTAINING_RECORD(ListEntry, IO_THREAD, ListEntry);
			Thread2[j] = *Thread;
			j += 1;
		}
	}

	ASSERT(Table->Count == j);

	//
	// Destroy thread table
	//

	Table->Count = 0;
	ApsIoDestroyThreadTable(Table);

	Status = WriteFile(FileHandle, Thread2, ThreadSize, &Complete, NULL);
	if (Status) {
		Head->Streams[STREAM_IO_THREAD].Offset = Start.QuadPart;
		Head->Streams[STREAM_IO_THREAD].Length = ThreadSize;
		End->QuadPart = Start.QuadPart + ThreadSize;
		Status = S_OK;
	} else {
		Status = GetLastError();
	}

	ApsFree(Thread2);
	return Status;
}

ULONG
ApsIoBuildObjectStackTrace(
	_In_ PPF_REPORT_HEAD Head,
	_In_ PIO_OBJECT_ON_DISK Object
	)
{
	PIO_IRP_ON_DISK IrpStream;
	ULONG Count;
	PIO_IRP_ON_DISK Irp;
	PIO_STACKTRACE Trace;
	ULONG Number;
	ULONG i;

	IrpStream = (PIO_IRP_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_IRP);
	Count = ApsGetStreamRecordCount(Head, STREAM_IO_IRP, IO_IRP_ON_DISK);

	if (!Object->IrpCount) {
		Object->Trace = NULL;
		return 0;
	}

	//
	// Attach stack trace array to io object
	//

	Trace = (PIO_STACKTRACE)ApsMalloc(sizeof(IO_STACKTRACE) * Object->IrpCount);
	Object->Trace = Trace;

	Irp = &IrpStream[Object->FirstIrp];
	Number = 0;

	do {

		Trace[Number].StackId = Irp->StackId;
		Trace[Number].Count = 1;
		Trace[Number].Operation = (IO_OPERATION)((ULONG)Irp->Operation);
		Trace[Number].Next = IO_INVALID_STACTRACE;
		Trace[Number].CompleteBytes = Irp->CompletionBytes;
		Number += 1;

		if (Irp->NextIrp != IO_INVALID_IRP_INDEX) {
			Irp = &IrpStream[Irp->NextIrp];
		} else {
			Irp = NULL;
		}
	
	} while (Irp != NULL);

	ASSERT(Number == Object->IrpCount);

	//
	// Sort all trace its stack id
	//

	qsort(Trace, Number, sizeof(*Trace), ApsIoSortStackTraceCallback);

	//
	// Scan the trace stream to merge duplicated ones
	//

	for (i = 0; i < Number - 1; ) {
		if (Trace[i].StackId != Trace[i + 1].StackId){
			Trace[i].Next = i + 1;
			i = i + 1;
		}
		else {

			//
			// Merge the same stack id traces
			//

			int j;
			j = i;

			do{
				Trace[j].Count += 1;
				Trace[j].CompleteBytes += Trace[i + 1].CompleteBytes;
				i += 1;
			}
			while ((i < Number - 1) && (Trace[j].StackId == Trace[i + 1].StackId));

			if (i == Number - 1) {
				break;
			}

			ASSERT(Trace[j].StackId != Trace[i + 1].StackId); 
			Trace[j].Next = i + 1;
			i = i + 1;
		}
	}

	return Number; 
}

ULONG
ApsIoBuildThreadStackTrace(
	_In_ PPF_REPORT_HEAD Head,
	_In_ PIO_THREAD Thread
	)
{
	PIO_IRP_ON_DISK IrpStream;
	ULONG Count;
	PIO_IRP_ON_DISK Irp;
	PIO_STACKTRACE Trace;
	ULONG Number;
	ULONG i;

	IrpStream = (PIO_IRP_ON_DISK)ApsGetStreamPointer(Head, STREAM_IO_IRP);
	Count = ApsGetStreamRecordCount(Head, STREAM_IO_IRP, IO_IRP_ON_DISK);

	if (!Thread->IrpCount) {
		Thread->Trace = NULL;
		return 0;
	}

	//
	// Attach stack trace array to thread object
	//

	Trace = (PIO_STACKTRACE)ApsMalloc(sizeof(IO_STACKTRACE) * Thread->IrpCount);
	Thread->Trace = Trace;

	Irp = &IrpStream[Thread->FirstIrp];
	Number = 0;

	do {

		Trace[Number].StackId = Irp->StackId;
		Trace[Number].Count = 1;
		Trace[Number].Operation = (IO_OPERATION)((ULONG)Irp->Operation);
		Trace[Number].Next = IO_INVALID_STACTRACE;
		Trace[Number].CompleteBytes = Irp->CompletionBytes;
		Number += 1;

		if (Irp->NextIrp != IO_INVALID_IRP_INDEX) {
			Irp = &IrpStream[Irp->NextIrp];
		} else {
			Irp = NULL;
		}
	
	} while (Irp != NULL);

	ASSERT(Number == Thread->IrpCount);

	//
	// Sort all trace its stack id
	//

	qsort(Trace, Number, sizeof(*Trace), ApsIoSortStackTraceCallback);

	//
	// Scan the trace stream to merge duplicated ones
	//

	for (i = 0; i < Number - 1; ) {
		if (Trace[i].StackId != Trace[i + 1].StackId){
			Trace[i].Next = i + 1;
			i = i + 1;
		}
		else {

			//
			// Merge the same stack id traces
			//

			int j;
			j = i;

			do{
				Trace[j].Count += 1;
				Trace[j].CompleteBytes += Trace[i + 1].CompleteBytes;
				i += 1;
			}
			while ((i < Number - 1) && (Trace[j].StackId == Trace[i + 1].StackId));

			if (Trace[j].StackId != Trace[i + 1].StackId) {
				Trace[j].Next = i + 1;
				i = i + 1;
			}
		}
	}

	return Number; 
}

VOID
ApsIoFreeAttachedStackTrace(
	_In_ PIO_STACKTRACE Trace
	)
{
	ApsFree(Trace);
}