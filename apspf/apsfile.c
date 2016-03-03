//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
// 

#include "apsfile.h"
#include "aps.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include <intrin.h>
#include <dbghelp.h>
#include "list.h"
#include "aps.h"
#include "apsctl.h"

HANDLE FsSharedDataHandle;
PBTR_SHARED_DATA FsSharedData;

FS_CACHE_OBJECT FsCacheObject;
HANDLE FsIndexProcedureHandle;
HANDLE FsStopIndexEvent;

ULONG
FsValidatePath(
	IN PWSTR Path
	)
{
	HANDLE Handle;

	Handle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
						OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, NULL);
	if (Handle == INVALID_HANDLE_VALUE) {
		return APS_STATUS_INVALID_PARAMETER;
	}

	CloseHandle(Handle);
	return APS_STATUS_OK;
}

ULONG
FsCreateFileObject(
	IN PWSTR FullPathName,
	IN FS_FILE_TYPE Type,
	OUT PFS_FILE_OBJECT *Object
	)
{
	ULONG Status;
	HANDLE Handle;
	PFS_FILE_OBJECT File;
	PFS_MAPPING_OBJECT Mapping;
	PVOID MappedVa;

	Handle = CreateFile(FullPathName, GENERIC_READ|GENERIC_WRITE, 
		                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
						CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE) {
		return APS_STATUS_CREATEFILE;	
	}

	__try {

		File = (PFS_FILE_OBJECT)ApsMalloc(sizeof(FS_FILE_OBJECT));
		RtlZeroMemory(File, sizeof(*File));

		File->FileObject = Handle;
		Status = SetFilePointer(File->FileObject, FS_FILE_INCREMENT, NULL, FILE_BEGIN);
		if (Status == INVALID_SET_FILE_POINTER) {
			Status = APS_STATUS_SETFILEPOINTER;
			__leave;
		}

		Status = SetEndOfFile(File->FileObject);
		if (!Status) {
			Status = APS_STATUS_SETFILEPOINTER;
			__leave;
		}

		File->Type = Type;
		File->FileLength = FS_FILE_INCREMENT;
		StringCchCopy(File->Path, MAX_PATH, FullPathName);

		Handle = CreateFileMapping(File->FileObject, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (!Handle) {
			Status = APS_STATUS_CREATEFILEMAPPING;
			__leave;
		}

		Mapping = (PFS_MAPPING_OBJECT)ApsMalloc(sizeof(FS_MAPPING_OBJECT));
		RtlZeroMemory(Mapping, sizeof(*Mapping));

		Mapping->MappedObject = Handle;
		MappedVa = MapViewOfFile(Mapping->MappedObject, PAGE_READWRITE, 0, 0, FS_MAP_INCREMENT);	
		if (!MappedVa) {
			Status = APS_STATUS_MAPVIEWOFFILE;
			__leave;
		}

		Mapping->Type = DiskFileBacked;
		Mapping->MappedVa = MappedVa;
		Mapping->MappedOffset = 0;
		Mapping->MappedLength = FS_MAP_INCREMENT;

		if (Type == FileGlobalIndex) {
			Mapping->FirstSequence = 0;
			Mapping->LastSequence = FS_MAP_INCREMENT / sizeof(BTR_FILE_INDEX) - 1;
		} else {
			Mapping->FirstSequence = 0;
			Mapping->LastSequence = 0;
		}

		File->Mapping = Mapping;
		Mapping->FileObject = File;

		Status = APS_STATUS_OK;
	}
	__finally {

		if (Status != APS_STATUS_OK) {

			if (Mapping != NULL) {
				if (Mapping->MappedObject != NULL) {
					CloseHandle(Mapping->MappedObject);
				}
				ApsFree(Mapping);
			}

			if (File != NULL) {
				if (File->FileObject != NULL) {
					CloseHandle(File->FileObject);
				}
				ApsFree(File);
			}
		}
	}

	return Status;
}

VOID
FsCloseMappingObject(
	IN PFS_MAPPING_OBJECT Mapping
	)
{
	if (Mapping != NULL) {

		if (Mapping->MappedVa != NULL) {
			UnmapViewOfFile(Mapping->MappedVa);
			Mapping->MappedVa = NULL;
		}

		if (Mapping->MappedObject != NULL) {
			CloseHandle(Mapping->MappedObject);
			Mapping->MappedObject = NULL;
		}

		Mapping->MappedOffset = 0;
		Mapping->MappedLength = 0;
		Mapping->FirstSequence = 0;
		Mapping->LastSequence = 0;
	}
}

PFS_MAPPING_OBJECT
FsCreateMappingObject(
	IN PFS_FILE_OBJECT File,
	IN ULONG64 Offset,
	IN ULONG Size
	)
{
	HANDLE Handle;
	PVOID Address;
	PFS_MAPPING_OBJECT Mapping;

	if (!ApsIsUlong64Aligned(Offset, FS_MAP_INCREMENT)) {
		ASSERT(0);
		return NULL;
	}

	if (Offset + Size > File->FileLength) {
		ASSERT(0);
		return NULL;
	}

	Handle = CreateFileMapping(File->FileObject, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!Handle) {
		return NULL;
	}

	Address = MapViewOfFile(Handle, FILE_MAP_READ, (DWORD)(Offset >> 32), 
				            (DWORD)Offset, Size);
	if (!Address) {
		return NULL;
	}

	Mapping = File->Mapping;

	if (!Mapping) {
		Mapping = (PFS_MAPPING_OBJECT)ApsMalloc(sizeof(FS_MAPPING_OBJECT));
		RtlZeroMemory(Mapping, sizeof(*Mapping));
	}

	Mapping->MappedObject = Handle;
	Mapping->MappedVa = Address;
	Mapping->MappedOffset = Offset;
	Mapping->MappedLength = Size;
	Mapping->FirstSequence = 0;
	Mapping->LastSequence = 0;

	return Mapping;
}

ULONG
FsLookupIndexBySequence(
	IN PFS_FILE_OBJECT IndexObject,
	IN ULONG Sequence,
	OUT PBTR_FILE_INDEX Index
	)
{
	ULONG64 Offset;
	PFS_MAPPING_OBJECT MappingObject;
	PBTR_FILE_INDEX Entry;

	MappingObject = IndexObject->Mapping;

	if (Sequence < IndexObject->NumberOfRecords) {

		if (FsIsSequenceMapped(MappingObject, Sequence)) {
			Entry = FsIndexFromSequence(MappingObject, Sequence);		

			if (Entry->Committed) {
				*Index = *Entry;
				return APS_STATUS_OK;
			} else {
				return APS_STATUS_NO_SEQUENCE;
			}
		}
		
		if (MappingObject->MappedVa) {
			FlushViewOfFile(MappingObject->MappedVa, 0);
			UnmapViewOfFile(MappingObject->MappedVa);
			MappingObject->MappedVa = NULL;
		}

		Offset = ApsUlong64RoundDown(FsIndexOffsetFromSequence(Sequence), FS_MAP_INCREMENT);
		
		if (Offset < IndexObject->FileLength) {
			MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, PAGE_READWRITE, 
				                                    (DWORD)(Offset >> 32), (DWORD)Offset, 
													FS_MAP_INCREMENT );
			if (!MappingObject->MappedVa) {
				return APS_STATUS_MAPVIEWOFFILE;
			}
			
			MappingObject->MappedOffset = Offset;
			MappingObject->MappedLength = FS_MAP_INCREMENT;
			MappingObject->FirstSequence = (ULONG)(Offset / sizeof(BTR_FILE_INDEX));
			MappingObject->LastSequence = MappingObject->FirstSequence + SEQUENCES_PER_INCREMENT - 1;
			
			return FsLookupIndexBySequence(IndexObject, Sequence, Index);
		}

		//
		// Index file length has been extended, we need reopen the file mapping object
		// to read the extended content.
		//

		ApsAcquireSpinLock(&FsSharedData->SpinLock);
		IndexObject->FileLength = FsSharedData->IndexFileLength;
		ApsReleaseSpinLock(&FsSharedData->SpinLock);

		ASSERT(IndexObject->FileLength > Offset);

		CloseHandle(MappingObject->MappedObject);
		MappingObject->MappedObject = CreateFileMapping(IndexObject->FileObject, NULL, 
			                                            PAGE_READWRITE, 0, 0, NULL);

		if (!MappingObject->MappedObject) {
			return APS_STATUS_CREATEFILEMAPPING;
		}

		return FsLookupIndexBySequence(IndexObject, Sequence, Index);
	}

	return APS_STATUS_NO_SEQUENCE;
}

ULONG
FsDestroyMappingObject(
	IN PFS_MAPPING_OBJECT Mapping
	)
{
	ASSERT(Mapping != NULL);

	if (!Mapping) {
		return APS_STATUS_OK;
	}

	if (Mapping->MappedVa != NULL) {
		UnmapViewOfFile(Mapping->MappedVa);
	}

	if (Mapping->MappedObject != NULL) {
		CloseHandle(Mapping->MappedObject);
	}

	ApsFree(Mapping);
	return APS_STATUS_OK;
}

ULONG CALLBACK
FsIndexProcedure(
	IN PVOID Context
	)
{
	HANDLE TimerHandle;
	ULONG TimerPeriod;
	LARGE_INTEGER DueTime;
	ULONG Status;
	HANDLE Handles[2];
	PFS_FILE_OBJECT FileObject;
	PFS_FILE_OBJECT IndexObject;
	PFS_FILE_OBJECT MasterFileObject;
	PFS_MAPPING_OBJECT MappingObject;
	ULONG NextSequence = 0;

	//
	// Clone a file object from global index file object 
	// since it's only used in index procedure, keep it private.
	//

	MasterFileObject = (PFS_FILE_OBJECT)Context;
	FileObject = (PFS_FILE_OBJECT)ApsMalloc(sizeof(FS_FILE_OBJECT));
	FileObject->Type = MasterFileObject->Type;
	FileObject->FileLength = MasterFileObject->FileLength;

	//
	// Create file object based on master file object
	//

	IndexObject = &FsCacheObject.IndexObject;
	StringCchCopy(FileObject->Path, MAX_PATH, MasterFileObject->Path);
	FileObject->FileObject = CreateFile(IndexObject->Path, GENERIC_READ|GENERIC_WRITE, 
		                                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileObject->FileObject == INVALID_HANDLE_VALUE) {
		ApsFree(FileObject);
		return APS_STATUS_CREATEFILE;
	}

	MappingObject = (PFS_MAPPING_OBJECT)ApsMalloc(sizeof(FS_MAPPING_OBJECT));
	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, 
		                                            PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		CloseHandle(FileObject->FileObject);
		ApsFree(MappingObject);
		ApsFree(FileObject);
		return APS_STATUS_ERROR;
	}

	MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
		                                    FILE_MAP_READ|FILE_MAP_WRITE, 
										    0, 0, FS_MAP_INCREMENT);
	if (!MappingObject->MappedVa) {
		CloseHandle(MappingObject->MappedObject);
		CloseHandle(FileObject->FileObject);
		ApsFree(MappingObject);
		ApsFree(FileObject);
		return APS_STATUS_ERROR;
	}

	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = FS_MAP_INCREMENT;
	MappingObject->FirstSequence = 0;
	MappingObject->LastSequence = SEQUENCES_PER_INCREMENT - 1;

	MappingObject->FileObject = FileObject;
	FileObject->Mapping = MappingObject;

	//
	// Create work loop timer, half second expires
	//

	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	TimerPeriod = 500;

	DueTime.QuadPart = -10000 * TimerPeriod;
	Status = SetWaitableTimer(TimerHandle, &DueTime, TimerPeriod, 
		                      NULL, NULL, FALSE);	
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	Handles[0] = TimerHandle;
	Handles[1] = FsStopIndexEvent;

	Status = APS_STATUS_OK;

	while (Status == APS_STATUS_OK) {
	
		ULONG Signal;
		Signal = WaitForMultipleObjects(2, Handles, FALSE, INFINITE);

		switch (Signal) {

			case WAIT_OBJECT_0:

				Status = FsUpdateNextSequence(FileObject, MappingObject, &NextSequence);
				if (Status == APS_STATUS_OK) {

					//
					// Update sequence to master index file object 
					//

					MasterFileObject->NumberOfRecords = NextSequence;
				}
				
				break;

			case WAIT_OBJECT_0 + 1:
				Status = APS_STATUS_STOPPED;
				break;

			default:
				Status = APS_STATUS_ERROR;
				break;
		} 
		
	}

	if (MappingObject->MappedVa) {
		UnmapViewOfFile(MappingObject->MappedVa);
	}
	if (MappingObject->MappedObject) {
		CloseHandle(MappingObject->MappedObject);
	}
	if (FileObject->FileObject) {
		CloseHandle(FileObject->FileObject);
	}

	ApsFree(MappingObject);
	ApsFree(FileObject);

	CancelWaitableTimer(TimerHandle);
	CloseHandle(TimerHandle);
	return Status;
}

ULONG
FsUpdateNextSequence(
	IN PFS_FILE_OBJECT FileObject,
	IN PFS_MAPPING_OBJECT MappingObject,
	IN PULONG NextSequence
	)
{
	ULONG Status;
	ULONG Number;
	ULONG Sequence;
	ULONG Maximum;
	ULONG64 Start;
	ULONG64 Length;
	PBTR_FILE_INDEX Index;

	//
	// Compute whether current view is not yet finished scan.
	//

	Sequence = *NextSequence;
	Index = (PBTR_FILE_INDEX)MappingObject->MappedVa;

	if (FsIsSequenceMapped(MappingObject, Sequence)) {

		ASSERT(Sequence <= MappingObject->LastSequence);
		Number = Sequence - MappingObject->FirstSequence;
		for ( ; Number < SEQUENCES_PER_INCREMENT; Number += 1) {
			if (Index[Number].Committed == 1) {
				Sequence += 1;
			} else {
				break;
			}
		}
	}
	
	//
	// If scaned current sequence is within current view limits, return
	//

	if (Sequence <= MappingObject->LastSequence) {
		*NextSequence = Sequence;
		return APS_STATUS_OK;
	}

	//
	// The current view is fully scanned, slide to next region, first check
	// file length limits.
	//

	Maximum = (ULONG)(FileObject->FileLength / sizeof(BTR_FILE_INDEX) - 1);	
	if (Sequence < Maximum) {
		
		Start = MappingObject->MappedOffset + MappingObject->MappedLength,
		ASSERT(Start + FS_MAP_INCREMENT <= FileObject->FileLength);

		if (MappingObject->MappedVa) {
			UnmapViewOfFile(MappingObject->MappedVa);
		}

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
			                                    FILE_MAP_READ|FILE_MAP_WRITE,
												(DWORD)(Start >> 32), (DWORD)Start, 
												FS_MAP_INCREMENT);
		if (!MappingObject->MappedVa) {
			return APS_STATUS_OUTOFMEMORY;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = FS_MAP_INCREMENT;
		MappingObject->FirstSequence = Sequence;
		MappingObject->LastSequence = Sequence + SEQUENCES_PER_INCREMENT - 1;

		*NextSequence = Sequence;
		return FsUpdateNextSequence(FileObject, MappingObject, NextSequence);
	}

	//
	// Current file length is scanned over, let's check whether the file length is 
	// already extended.
	//

	ApsAcquireSpinLock(&FsSharedData->SpinLock);
	Length = FsSharedData->IndexFileLength;
	ApsReleaseSpinLock(&FsSharedData->SpinLock);

	if (Length == FileObject->FileLength) {

		ApsAcquireSpinLock(&FsSharedData->SpinLock);

		Length = FsSharedData->IndexFileLength;
		if (Length != FileObject->FileLength) {
			
			//
			// Other thread already extended the file length, do nothing here
			//

		} else {

			Length += FS_FILE_INCREMENT;
			Status = FsExtendFileLength(FileObject->FileObject, Length);
			if (Status == APS_STATUS_OK) {
				FsSharedData->IndexFileLength = Length;
			}
		}

		ApsReleaseSpinLock(&FsSharedData->SpinLock);

		if (Status != APS_STATUS_OK) {
			return Status;
		}
	}

	//
	// The index file is already extended, use the new length to create
	// new file mapping object, note that we only unmap/null old view and keep
	// all other members unchanged to support recursive calls.
	//

	UnmapViewOfFile(MappingObject->MappedVa);
	MappingObject->MappedVa = NULL;

	CloseHandle(MappingObject->MappedObject);
	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, 
													PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		return APS_STATUS_OUTOFMEMORY;
	}

	//
	// Update current file length
	//

	FileObject->FileLength = Length;
	*NextSequence = Sequence;

	return FsUpdateNextSequence(FileObject, MappingObject, NextSequence);
}

ULONG
FsExtendFileLength(
	IN HANDLE FileObject,
	IN ULONG64 Length
	)
{
	ULONG Status;
	LARGE_INTEGER NewLength;

	NewLength.QuadPart = Length;

	Status = SetFilePointerEx(FileObject, NewLength, NULL, FILE_BEGIN);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	Status = SetEndOfFile(FileObject);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	return APS_STATUS_OK;
}

ULONG
FsCloseFileObject(
	IN PFS_FILE_OBJECT File,
	IN BOOLEAN Free
	)
{
	ASSERT(File != NULL);

	if (File->Mapping != NULL) {
		FsDestroyMappingObject(File->Mapping);
		File->Mapping = NULL;
	}

	CloseHandle(File->FileObject);
	File->FileObject = NULL;

	if (Free) {
		ApsFree(File);
	}

	return APS_STATUS_OK;
}

ULONG
FsStartIndexProcedure(
	IN PFS_FILE_OBJECT FileObject
	)
{
	FsIndexProcedureHandle = CreateThread(NULL, 0, FsIndexProcedure, FileObject, 0, NULL);
	if (!FsIndexProcedureHandle) {
		return APS_STATUS_ERROR;
	}

	return APS_STATUS_OK;
}

ULONG
FsCreateSharedMemory(
	IN PWSTR ObjectName,
	IN SIZE_T Length,
	IN ULONG ViewOffset,
	IN SIZE_T ViewSize,
	OUT PFS_MAPPING_OBJECT *Object
	)
{
	HANDLE Handle;
	PVOID Address;
	PFS_MAPPING_OBJECT Mapping;

	Handle = CreateFileMapping(NULL, NULL, PAGE_READWRITE, ((ULONG64)Length >> 32),
							   (ULONG)Length, ObjectName);
	if (!Handle) {
		return GetLastError();
	}

	Address = MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, ((ULONG64)ViewOffset >> 32), 
		                    (ULONG)ViewOffset, ViewSize );

	if (!Address) {
		CloseHandle(Handle);
		return GetLastError();
	}

	Mapping = (PFS_MAPPING_OBJECT)ApsMalloc(sizeof(FS_MAPPING_OBJECT));
	RtlZeroMemory(Mapping, sizeof(*Mapping));

	InitializeListHead(&Mapping->ListEntry);

	Mapping->Type = PageFileBacked;
	Mapping->MappedVa = Address;
	Mapping->MappedLength = (ULONG)ViewSize;
	Mapping->MappedOffset = ViewOffset;
	Mapping->FileLength = Length;

	if (ObjectName != NULL) {
		//StringCchCopy(Mapping->ObjectName, 64, ObjectName);
	}

	*Object = Mapping;
	return APS_STATUS_OK;
}