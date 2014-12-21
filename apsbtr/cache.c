//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012 
// 

#include "apsbtr.h"
#include "btr.h"
#include "global.h"
#include "heap.h"
#include "cache.h"
#include "hal.h"
#include "util.h"
#include "thread.h"

//
// File Mapping Macros
//

#define IS_MAPPED_OFFSET(_M, _S, _E) \
	(_S >= _M->MappedOffset && _E <= _M->MappedOffset + _M->MappedLength)

#define IS_FILE_OFFSET(_M, _E) \
	(_E <= _M->FileLength)

ULONG
BtrInitializeCache(
	__in PBTR_PROFILE_OBJECT Object	
	)
{
	return S_OK;
}

VOID
BtrUninitializeCache(
	VOID
	)
{
}

VOID
BtrCloseCache(
	VOID
	)
{
	LARGE_INTEGER Size;

	//
	// Close index and data files at their appropriate size
	//

	if (BtrProfileObject->IndexFileObject != NULL) {

		Size.QuadPart = BtrSharedData->IndexValidLength;
		SetFilePointerEx(BtrProfileObject->IndexFileObject, Size, NULL, FILE_BEGIN);
		SetEndOfFile(BtrProfileObject->IndexFileObject);

		CloseHandle(BtrProfileObject->IndexFileObject);
		BtrProfileObject->IndexFileObject = NULL;
	}

	if (BtrProfileObject->DataFileObject != NULL) {

		Size.QuadPart = BtrSharedData->DataValidLength;
		SetFilePointerEx(BtrProfileObject->DataFileObject, Size, NULL, FILE_BEGIN);
		SetEndOfFile(BtrProfileObject->DataFileObject);

		CloseHandle(BtrProfileObject->DataFileObject);
		BtrProfileObject->DataFileObject = NULL;
	}
}

ULONG
BtrCloseMappingPerThread(
	__in PBTR_THREAD_OBJECT Thread
	)
{
	PBTR_MAPPING_OBJECT DataObject;
	PBTR_MAPPING_OBJECT IndexObject;

	DataObject = Thread->DataMapping;
	IndexObject = Thread->IndexMapping;

	if (IndexObject != NULL) {

		if (IndexObject->MappedVa != NULL) {
			FlushViewOfFile(IndexObject->MappedVa, 0);
			UnmapViewOfFile(IndexObject->MappedVa);
		}

		if (IndexObject->Object != NULL) {
			CloseHandle(IndexObject->Object);
		}

		BtrFree(IndexObject);
		Thread->IndexMapping = NULL;
	}
	
	if (DataObject != NULL) {

		if (DataObject->MappedVa != NULL) {
			FlushViewOfFile(DataObject->MappedVa, 0);
			UnmapViewOfFile(DataObject->MappedVa);
		}

		if (DataObject->Object != NULL) {
			CloseHandle(DataObject->Object);
		}

		BtrFree(DataObject);
		Thread->DataMapping = NULL;
	}

	return S_OK;
}

ULONG
BtrCreateMappingPerThread(
	__in PBTR_THREAD_OBJECT Thread
	)
{
	ULONG Status;
	PBTR_MAPPING_OBJECT IndexObject;
	PBTR_MAPPING_OBJECT DataObject;

	IndexObject = (PBTR_MAPPING_OBJECT)BtrMalloc(sizeof(BTR_MAPPING_OBJECT));
	if (!IndexObject) {
		return BTR_E_OUTOFMEMORY;
	}

	DataObject = (PBTR_MAPPING_OBJECT)BtrMalloc(sizeof(BTR_MAPPING_OBJECT));
	if (!DataObject) {
		BtrFree(IndexObject);
		return BTR_E_OUTOFMEMORY;
	}

	Status = S_OK;

	__try {

		DataObject->Object = CreateFileMapping(BtrProfileObject->DataFileObject, NULL, 
			                                   PAGE_READWRITE, 0, 0, NULL);
		if (!DataObject->Object) {
			Status = GetLastError();
			__leave;
		}

		DataObject->MappedVa = NULL;
		DataObject->MappedOffset = 0;
		DataObject->MappedLength = 0;
		DataObject->FileLength = ReadForWriteAccess(&BtrSharedData->DataFileLength);
	

		IndexObject->Object = CreateFileMapping(BtrProfileObject->IndexFileObject, NULL, 
			                                    PAGE_READWRITE, 0, 0, NULL);	
		if (!IndexObject->Object) {
			Status = GetLastError();
			__leave;
		}

		IndexObject->MappedVa = NULL;
		IndexObject->MappedOffset = 0; 
		IndexObject->MappedLength = 0;
		IndexObject->FileLength = ReadForWriteAccess(&BtrSharedData->IndexFileLength);

		Thread->DataMapping = DataObject;
		DataObject->Thread = Thread;
		Thread->IndexMapping = IndexObject;
		IndexObject->Thread = Thread;

	}
	__finally {

		if (Status != S_OK) {

			ASSERT(0);

			if (DataObject != NULL) {

				if (DataObject->MappedVa != NULL) {
					UnmapViewOfFile(DataObject->MappedVa);
				}

				if (DataObject->Object) {
					CloseHandle(DataObject->Object);
				}

				BtrFree(DataObject);
			}

			if (IndexObject != NULL) {

				if (IndexObject->MappedVa != NULL) {
					UnmapViewOfFile(IndexObject->MappedVa);
				}

				if (IndexObject->Object) {
					CloseHandle(IndexObject->Object);
				}

				BtrFree(IndexObject);
			}
		}
	}
	
	return Status;

}

ULONG
BtrCopyMemory(
	__in PVOID Destine,
	__in PVOID Source,
	__in ULONG Length 
	)
{
	__try {

		RtlCopyMemory(Destine, Source, Length);
		return S_OK;

	} 
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}

	return BTR_E_EXCEPTION;
}
ULONG
BtrExtendFileLength(
	__in HANDLE File,
	__in ULONG64 Length
	)
{
	ULONG Status;
	LARGE_INTEGER NewLength;

	NewLength.QuadPart = Length;

	Status = SetFilePointerEx(File, NewLength, NULL, FILE_BEGIN);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	Status = SetEndOfFile(File);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	return S_OK;
}

ULONG
BtrAcquireWritePosition(
	__in ULONG BytesToWrite,
	__out PULONG64 Position
	)
{
	ULONG64 Start;
	ULONG64 End;
	ULONG64 Current;
	ULONG64 FileLength;
	ULONG64 CurrentLength;
	ULONG64 NewLength;
	ULONG Status;

	//
	// Caller get invalid write position on any failure
	//

	*Position = (ULONG64)-1;

	while (1) {

		//
		// Read current file length and its valid length
		//

		FileLength = ReadForWriteAccess(&BtrSharedData->DataFileLength);
		Start = ReadForWriteAccess(&BtrSharedData->DataValidLength);

		//
		// Compute the end length after our write
		//

		End = Start + BytesToWrite;

		if (End <= FileLength) {
			Current = _InterlockedCompareExchange64(&BtrSharedData->DataValidLength, 
													End, Start);
			if (Current == Start) {

				//
				// We get the position, return here
				//

				*Position = Start;
				return S_OK;
			}
		}
		else {
		
			break;
		}
	}

	//
	// Extend file length
	//

	Status = S_OK;
	NewLength = FileLength + BTR_FILE_INCREMENT;

	BtrAcquireSpinLock(&BtrSharedData->SpinLock);

	//
	// Read the length values again to check whether file length is already
	// changed before we acquire the lock
	//

	CurrentLength = BtrSharedData->DataFileLength;
	if (CurrentLength > FileLength) {

		//
		// The file length is extended already, do nothing and update file length
		//

	} else {

		Status = BtrExtendFileLength(BtrProfileObject->DataFileObject, NewLength);
		if (Status == S_OK) {
			BtrSharedData->DataFileLength = NewLength;
		}
	}

	BtrReleaseSpinLock(&BtrSharedData->SpinLock);

	if (Status != S_OK) {
		return Status;
	}

	//
	// Update our writer map's file length
	//

	return BtrAcquireWritePosition(BytesToWrite, Position);
}

ULONG
BtrWriteRecord(
	__in PBTR_THREAD_OBJECT Thread,
	__in PVOID Record,
	__in ULONG Length,
	__out PULONG64 Offset
	)
{
	ULONG Status;
	PBTR_MAPPING_OBJECT MappingObject;
	ULONG64 Start;
	ULONG64 End;
	ULONG64 MappedStart;
	ULONG64 MappedEnd;
	PVOID Address;

	Status = BtrAcquireWritePosition(Length, &Start);
	if (Status != S_OK) {
		return Status;
	}

	MappingObject = Thread->DataMapping;
	End = Start + Length; 

	if (IS_MAPPED_OFFSET(MappingObject, Start, End)) {
		Address = (PUCHAR)MappingObject->MappedVa + (ULONG)(Start - MappingObject->MappedOffset);
		Status = BtrCopyMemory(Address, Record, Length);
		*Offset = Start;
		return Status;
	}

	if (MappingObject->MappedVa != NULL) {
		FlushViewOfFile(MappingObject->MappedVa, 0);
		UnmapViewOfFile(MappingObject->MappedVa);
		MappingObject->MappedVa = NULL;
		MappingObject->MappedOffset = 0;
		MappingObject->MappedLength = 0;
	}

repeat:

	if (IS_FILE_OFFSET(MappingObject, End)) {

		//
		// Adjust file mapping view to hold this position,
		// note that we must ensure the mapped end is within
		// current file mapping object's length.
		//

		MappedStart = BtrUlong64RoundDown(Start, BTR_MAP_INCREMENT);
		MappedEnd = BtrUlong64RoundUp(End, BTR_MAP_INCREMENT);
		MappedEnd = min(MappedEnd, MappingObject->FileLength);

		MappingObject->MappedVa = MapViewOfFile(MappingObject->Object, FILE_MAP_READ|FILE_MAP_WRITE, 
			                                    (DWORD)(MappedStart>>32), (DWORD)MappedStart, 
												(SIZE_T)(MappedEnd - MappedStart));
		if (!MappingObject->MappedVa) {
			ASSERT(0);
			return BTR_E_MAPVIEWOFFILE;
		}
		
		MappingObject->MappedOffset = MappedStart;
		MappingObject->MappedLength = (ULONG)(MappedEnd - MappedStart);

		Address = (PUCHAR)MappingObject->MappedVa + (ULONG)(Start - MappedStart);
		Status = BtrCopyMemory(Address, Record, Length);
		*Offset = Start;
		return Status;
	}

	//
	// The acquired write position is beyond the limit of file mapping object,
	// note that the current file length is guarantted to be larger than file 
	// mapping object's length, this is ensured by BtrAcquireWritePosition()
	//

	CloseHandle(MappingObject->Object);
	MappingObject->Object = CreateFileMapping(BtrProfileObject->DataFileObject, NULL, 
		                                      PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->Object) {
		ASSERT(0);
		return BTR_E_CREATEFILEMAPPING;
	}

	MappingObject->FileLength = ReadForWriteAccess(&BtrSharedData->DataFileLength);
	goto repeat;

	return S_FALSE;
}

ULONG
BtrWriteIndex(
	__in PBTR_THREAD_OBJECT Thread,
	__in ULONG64 RecordOffset,
	__in ULONG64 Location,
	__in ULONG Size
	)
{
	ULONG Status;
	ULONG Distance;
	ULONG64 CurrentLength;
	ULONG64 ExtendedLength;
	ULONG64 MappedOffset;
	ULONG64 Start;
	ULONG64 End;
	PBTR_FILE_INDEX Index;
	PBTR_MAPPING_OBJECT MappingObject;

	MappingObject = Thread->IndexMapping;
	Start = Location;
	End = Location + sizeof(BTR_FILE_INDEX);

	if (IS_MAPPED_OFFSET(MappingObject, Start, End)) {
		Distance = (ULONG)(Start - (ULONG64)MappingObject->MappedOffset);
		Index = (PBTR_FILE_INDEX)((PUCHAR)MappingObject->MappedVa + Distance);
		Index->Length = Size;
		Index->Offset = RecordOffset;
		Index->Committed = 1;
		return S_OK;
	} 

	//
	// Whether it falls into legal file length
	//

	if (MappingObject->MappedVa) {
		FlushViewOfFile(MappingObject->MappedVa, 0);
		UnmapViewOfFile(MappingObject->MappedVa);
		MappingObject->MappedVa = NULL;
		MappingObject->MappedOffset = 0;
		MappingObject->MappedLength = 0;
	}


	if (IS_FILE_OFFSET(MappingObject, End)) {

		//
		// N.B. Ensure that view get started from an increment boundary, also note
		// that round down operation meanwhile ensure that the view is in legal file length
		//

		MappedOffset = BtrUlong64RoundDown(Start, BTR_MAP_INCREMENT);
		MappingObject->MappedVa = MapViewOfFile(MappingObject->Object, FILE_MAP_WRITE, 
			                                    (DWORD)(MappedOffset >> 32), (DWORD)MappedOffset, 
											    BTR_MAP_INCREMENT);
		if (!MappingObject->MappedVa) {
			return BTR_E_MAPVIEWOFFILE;
		}
		
		MappingObject->MappedOffset = MappedOffset;
		MappingObject->MappedLength = BTR_MAP_INCREMENT;

		return BtrWriteIndex(Thread, RecordOffset, Location, Size);
	}

	//
	// It's beyond the limit of current file length, extend the file length
	//
	
	CloseHandle(MappingObject->Object);

	BtrAcquireSpinLock(&BtrSharedData->SpinLock);
	CurrentLength = BtrSharedData->IndexFileLength;
	BtrReleaseSpinLock(&BtrSharedData->SpinLock);

	if (Location < CurrentLength) {

		//
		// Length is already extended, we can directly create new mapping.
		//

		MappingObject->Object = CreateFileMapping(BtrProfileObject->IndexFileObject, NULL, 
												  PAGE_READWRITE, 0, 0, NULL);
		if (!MappingObject->Object) {
			return BTR_E_CREATEFILEMAPPING;
		}
		
		MappingObject->FileLength = CurrentLength;
		return BtrWriteIndex(Thread, RecordOffset, Location, Size);
	}

	//
	// This is the only one slow path with a spinlock
	// we must explicitly extend the index file length
	//

	BtrAcquireSpinLock(&BtrSharedData->SpinLock);
	
	if (BtrSharedData->SequenceFull) {
		BtrReleaseSpinLock(&BtrSharedData->SpinLock);
		return BTR_E_INDEX_FULL;
	}

	if (Location < BtrSharedData->IndexFileLength) {
		
		//
		// This indicates some other thread extended file length before
		// we acquire the lock
		//

		Status = S_OK;

	} else {

		ExtendedLength = BtrSharedData->IndexFileLength + BTR_FILE_INCREMENT;
		if (ExtendedLength > BTR_MAXIMUM_INDEX_FILE_SIZE) {

			//
			// File size is overflow, stop any following operations.
			//

			BtrSharedData->SequenceFull = TRUE;
			Status = BTR_E_INDEX_FULL;

		} else {

			Status = BtrExtendFileLength(BtrProfileObject->IndexFileObject, ExtendedLength);
			if (Status == S_OK) {

				//
				// Length is successfully extended, update the file length
				//

				BtrSharedData->IndexFileLength = ExtendedLength;

			} else {
				Status = BTR_E_INDEX_FULL;	
			}
		}
	}

	//
	// Release spinlock
	//

	BtrReleaseSpinLock(&BtrSharedData->SpinLock);

	if (Status != S_OK) {
		return Status; 
	}

	MappingObject->Object = CreateFileMapping(BtrProfileObject->IndexFileObject, NULL, 
											  PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->Object) {
		return BTR_E_CREATEFILEMAPPING;
	}

	MappingObject->FileLength = ExtendedLength;
	return BtrWriteIndex(Thread, RecordOffset, Location, Size);
}

ULONG
BtrQueueRecord(
	__in PBTR_THREAD_OBJECT Thread,
	__in PBTR_RECORD_HEADER Header
	)
{
	ULONG Status;
	ULONG64 Offset;
	ULONG64 Location;

	ASSERT(Thread->IndexMapping && Thread->DataMapping);

	//
	// Acquire the record's sequence number
	//

	Header->Sequence = BtrAcquireSequence();
	Status = BtrWriteRecord(Thread, Header, Header->TotalLength, &Offset);
	if (Status != S_OK) {
		return Status;
	}

	Location = Header->Sequence * sizeof(BTR_FILE_INDEX);
	Status = BtrWriteIndex(Thread, Offset, Location, Header->TotalLength);
	if (Status != S_OK) {
		return Status;
	}

	return Status;
}

ULONG
BtrAcquireSequence(
	VOID
	)
{
	ULONG Sequence;

	Sequence = _InterlockedIncrement(&BtrSharedData->Sequence);
	return Sequence;
}

ULONG
BtrCurrentSequence(
	VOID
	)
{
	ULONG Sequence;

	Sequence = ReadForWriteAccess(&BtrSharedData->Sequence);
	return Sequence;
}

ULONG
BtrAcquireObjectId(
	VOID
	)
{
	ULONG ObjectId;

	ObjectId = InterlockedIncrement(&BtrSharedData->ObjectId);
	return ObjectId;
}

ULONG 
BtrAcquireStackId(
	VOID
	)
{
	ULONG StackId;

	StackId = InterlockedIncrement(&BtrSharedData->StackId);
	return StackId;
}