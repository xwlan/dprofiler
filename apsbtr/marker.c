//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "apsbtr.h"
#include "marker.h"
#include "stream.h"
#include "heap.h"

BTR_LOCK BtrMarkLock;
LIST_ENTRY BtrMarkList;
ULONG BtrMarkCount;

VOID
BtrInitializeMarkList(
	VOID
	)
{
	InitializeListHead(&BtrMarkList);
	BtrMarkCount = 0;

	BtrInitLock(&BtrMarkLock);
}

VOID
BtrInsertMark(
	__in ULONG Index,
	__in BTR_MARK_REASON Reason
	)
{
	PBTR_MARK Mark;

	Mark = (PBTR_MARK)BtrMalloc(sizeof(BTR_MARK));
	Mark->Index = Index;
	Mark->Reason = Reason;

	GetSystemTimeAsFileTime(&Mark->Timestamp);

	BtrAcquireLock(&BtrMarkLock);

	InsertTailList(&BtrMarkList, &Mark->ListEntry);
	BtrMarkCount += 1;

	BtrReleaseLock(&BtrMarkLock);
}

ULONG
BtrWriteMarkStream(
	__in PPF_REPORT_HEAD Head,
	__in HANDLE FileHandle,
	__in LARGE_INTEGER Start,
	__out PLARGE_INTEGER End
	)
{
	ULONG Count;
	ULONG Size;
	ULONG Complete;
	ULONG Status;
	PBTR_MARK Mark;
	PBTR_MARK Entry;
	PLIST_ENTRY ListEntry;

	ASSERT(BtrMarkCount != 0);
	ASSERT(!IsListEmpty(&BtrMarkList));

	Count = 0;
	Size = sizeof(BTR_MARK) * BtrMarkCount;
	Mark = (PBTR_MARK)BtrMalloc(Size);

	BtrAcquireLock(&BtrMarkLock);

	//
	// Serialize all marks into stream in memory 
	//

	while (!IsListEmpty(&BtrMarkList)) {

		ListEntry = RemoveHeadList(&BtrMarkList);
		Entry = CONTAINING_RECORD(ListEntry, BTR_MARK, ListEntry);

		Mark[Count] = *Entry;
		BtrFree(Entry);

		Count += 1;
	}

	//
	// Verify mark count match
	//

	ASSERT(Count == BtrMarkCount);
	BtrMarkCount = 0;

	BtrReleaseLock(&BtrMarkLock);

	//
	// Write mark stream to file
	//

	Status = WriteFile(FileHandle, Mark, Size, &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
	} else {
		End->QuadPart = Start.QuadPart + Size;
		Status = S_OK;
	}

	BtrFree(Mark);

	//
	// Destroy mark lock
	//

	BtrDeleteLock(&BtrMarkLock);
	return Status;
}