//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _APS_FILE_H_
#define _APS_FILE_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "apsbtr.h"
#include "apsdefs.h"
#include <rpc.h>

typedef enum _FS_FILE_TYPE {
	FileGlobalIndex,
	FileGlobalData,
	FileStackEntry,
	FileStackText,
	FileFilterIndex,
	FileDpfReport,
	FileDtlReport,
} FS_FILE_TYPE, *PFS_FILE_TYPE;

typedef struct _FS_FILE_OBJECT {

	LIST_ENTRY ListEntry;
	FS_FILE_TYPE Type;

	HANDLE FileObject;
	ULONG64 FileLength;
	ULONG64 ValidLength;
	ULONG NumberOfRecords;

	struct _FS_MAPPING_OBJECT *Mapping;

	WCHAR Path[MAX_PATH];

} FS_FILE_OBJECT, *PFS_FILE_OBJECT;

typedef enum _FS_MAPPING_TYPE {
	DiskFileBacked,
	PageFileBacked,
} FS_MAPPING_TYPE, *PFS_MAPPING_TYPE;

typedef struct _FS_MAPPING_OBJECT {

	LIST_ENTRY ListEntry;
	FS_MAPPING_TYPE Type;
	HANDLE MappedObject;

	union {
		PVOID MappedVa;
		struct _BTR_FILE_INDEX *Index;
		PULONG FilterIndex;
	};

	ULONG64 FileLength;
	ULONG64 MappedOffset;
	ULONG MappedLength;

	ULONG FirstSequence;
	ULONG LastSequence;

	struct _FS_FILE_OBJECT *FileObject;
	struct _FS_MAPPING_OBJECT *Next;

} FS_MAPPING_OBJECT, *PFS_MAPPING_OBJECT;

typedef struct _FS_CACHE_OBJECT {

	ULONG ProcessId;

	FS_FILE_OBJECT IndexObject;
	FS_FILE_OBJECT DataObject;
	FS_FILE_OBJECT StackFile;

	WCHAR Path[MAX_PATH];

} FS_CACHE_OBJECT, *PFS_CACHE_OBJECT;

//
// N.B. Filter index is only a sequence number, we only scan
// filtered sequence number, don't require to track its full index,
// when access filtered record, we directly pass the sequence to
// FsCopyRecord().
//

typedef ULONG FS_FILTER_INDEX, *PFS_FILTER_INDEX;

//
// File and mapping adjustment macros
//

typedef enum _FS_MACRO {
	FS_MAP_INCREMENT  = 1024 * 64,
	FS_FILE_INCREMENT = 1024 * 1024 * 4,
	SEQUENCES_PER_INCREMENT = (FS_MAP_INCREMENT/sizeof(BTR_FILE_INDEX)),
} FS_MACRO;

ULONG
FsValidatePath(
	IN PWSTR Path
	);

ULONG
FsCopyRecordEx(
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size,
	OUT PBTR_FILE_INDEX Index
	);

PFS_MAPPING_OBJECT
FsCreateMappingObject(
	IN PFS_FILE_OBJECT FileObject,
	IN ULONG64 Offset,
	IN ULONG Size
	);

VOID
FsCloseMappingObject(
	IN PFS_MAPPING_OBJECT Mapping
	);

ULONG
FsDestroyMappingObject(
	IN PFS_MAPPING_OBJECT Mapping
	);

ULONG
FsLookupIndexBySequence(
	IN PFS_FILE_OBJECT IndexObject,
	IN ULONG Sequence,
	OUT PBTR_FILE_INDEX Index
	);

ULONG
FsUpdateNextSequence(
	IN PFS_FILE_OBJECT FileObject,
	IN PFS_MAPPING_OBJECT MappingObject,
	IN PULONG NextSequence
	);

ULONG
FsExtendFileLength(
	IN HANDLE FileObject,
	IN ULONG64 Length
	);

ULONG
FsCloseFileObject(
	IN PFS_FILE_OBJECT FileObject,
	IN BOOLEAN Free
	);

ULONG
FsCreateSharedMemory(
	IN PWSTR ObjectName,
	IN SIZE_T Length,
	IN ULONG ViewOffset,
	IN SIZE_T ViewSize,
	OUT PFS_MAPPING_OBJECT *Object
	);

//
// FS INLINES
//

FORCEINLINE
BOOLEAN
FsIsSequenceMapped(
	IN PFS_MAPPING_OBJECT Mapping,
	IN ULONG Sequence
	)
{
	return ((Mapping->FirstSequence <= Sequence) && (Sequence <= Mapping->LastSequence));
}

FORCEINLINE
PBTR_FILE_INDEX
FsIndexFromSequence(
	IN PFS_MAPPING_OBJECT Mapping,
	IN ULONG Sequence
	)
{
	return &Mapping->Index[Sequence - Mapping->FirstSequence];
}

FORCEINLINE
BOOLEAN
FsIsOffsetMapped(
	IN PFS_MAPPING_OBJECT Mapping,
	IN ULONG64 Start,
	IN ULONG64 End
	)
{
	return ((Start >= Mapping->MappedOffset) && (End <= Mapping->MappedOffset + Mapping->MappedLength));
}

FORCEINLINE
ULONG64
FsMaximumMappedOffset(
	IN PFS_MAPPING_OBJECT Mapping
	)
{
	return Mapping->MappedOffset + Mapping->MappedLength;
}
	
FORCEINLINE
PVOID
FsAddressFromOffset(
	IN PFS_MAPPING_OBJECT Mapping,
	IN ULONG64 Offset
	)
{
	return (PVOID)((PUCHAR)Mapping->MappedVa + Offset - Mapping->MappedOffset);
}

FORCEINLINE
ULONG64
FsIndexOffsetFromSequence(
	IN ULONG Sequence
	)
{
	return (ULONG64)(Sequence * sizeof(BTR_FILE_INDEX));
}

FORCEINLINE
BOOLEAN
FsCanOffsetBeMapped(
	IN PFS_MAPPING_OBJECT Mapping,
	IN ULONG64 Offset
	)
{
	return (Offset < Mapping->FileLength);
}


#ifdef __cplusplus
}
#endif
#endif