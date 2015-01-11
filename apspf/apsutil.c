//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2013
//

#include "apsbtr.h"
#include "aps.h"
#include <stdlib.h>
#include <math.h>
#include "calltree.h"

//
// _P2 should be greater than _P1
//

//
// Compute number of _C between _P1 and _P2
//

#define COUNT_BETWEEN_PTR(_P1, _P2, _C) \
	(SIZE_T)((((ULONG_PTR)_P2 - (ULONG_PTR)_P1) / sizeof(_C)))

//
// Compute number of bytes between _P1 and _P2
//

#define BYTE_BETWEEN_PTR(_P1, _P2) \
	(SIZE_T)((ULONG_PTR)_P2 - (ULONG_PTR)_P1)

ULONG
ApsSplitUnicodeString(
	__in PWSTR MultiSz,
	__in WCHAR Delimiter,
	__out PLIST_ENTRY ListHead
	)
{
	wchar_t *Ptr1;
	wchar_t *Ptr2;
	USHORT Bytes;
	ULONG Count;
	PSTRING_ENTRY StringEntry;

	InitializeListHead(ListHead);	

	Ptr1 = MultiSz;
	Ptr2 = Ptr1;
	Count = 0;

	//
	// Ensure it's not an empty string
	//

	if (!wcslen(MultiSz)) {
		return 0;
	}

	while (Ptr2 = wcschr(Ptr1, Delimiter)) {

		Bytes = (USHORT)(BYTE_BETWEEN_PTR(Ptr1, Ptr2));
		StringEntry = (PSTRING_ENTRY)ApsMalloc(sizeof(STRING_ENTRY));
		StringEntry->Bytes = Bytes;
		StringEntry->IsAnsi = FALSE;

		StringEntry->Buffer = (PCHAR)ApsMalloc(Bytes + sizeof(wchar_t));
		memcpy(StringEntry->Buffer, Ptr1, Bytes);

		Count += 1;
		Ptr1 = Ptr2 + 1;

		InsertTailList(ListHead, &StringEntry->ListEntry);
	}

	if (*Ptr1) {
		
		//
		// Only one string
		//

		Bytes = (USHORT)wcslen(Ptr1) * sizeof(wchar_t);
		StringEntry = (PSTRING_ENTRY)ApsMalloc(sizeof(STRING_ENTRY));
		StringEntry->Bytes = Bytes; 
		StringEntry->IsAnsi = FALSE;

		StringEntry->Buffer = (PCHAR)ApsMalloc(Bytes + sizeof(wchar_t));
		memcpy(StringEntry->Buffer, Ptr1, Bytes);

		Count += 1;
		InsertTailList(ListHead, &StringEntry->ListEntry);
	}

	return Count;
}

ULONG
ApsSplitAnsiString(
	__in PSTR MultiSz,
	__in CHAR Delimiter,
	__out PLIST_ENTRY ListHead
	)
{
	char *Ptr1;
	char *Ptr2;
	USHORT Bytes;
	ULONG Count;
	PSTRING_ENTRY StringEntry;

	InitializeListHead(ListHead);	

	Ptr1 = MultiSz;
	Ptr2 = Ptr1;
	Count = 0;

	//
	// Ensure it's not an empty string
	//

	if (!strlen(MultiSz)) {
		return 0;
	}

	while (Ptr2 = strchr(Ptr1, Delimiter)) {

		Bytes = (USHORT)(BYTE_BETWEEN_PTR(Ptr1, Ptr2));
		StringEntry = (PSTRING_ENTRY)ApsMalloc(sizeof(STRING_ENTRY));
		StringEntry->Bytes = Bytes;
		StringEntry->IsAnsi = FALSE;

		StringEntry->Buffer = (PCHAR)ApsMalloc(Bytes + sizeof(CHAR));
		memcpy(StringEntry->Buffer, Ptr1, Bytes);

		Count += 1;
		Ptr1 = Ptr2 + 1;

		InsertTailList(ListHead, &StringEntry->ListEntry);
	}

	if (*Ptr1) {
		
		//
		// Only one string
		//
		
		Bytes = (USHORT)strlen(Ptr1) * sizeof(CHAR);
		StringEntry = (PSTRING_ENTRY)ApsMalloc(sizeof(STRING_ENTRY));
		StringEntry->Bytes = Bytes;
		StringEntry->IsAnsi = FALSE;

		StringEntry->Buffer = (PCHAR)ApsMalloc(Bytes + sizeof(CHAR));
		memcpy(StringEntry->Buffer, Ptr1, Bytes);

		Count += 1;
		InsertTailList(ListHead, &StringEntry->ListEntry);
	}

	return Count;
}

ULONG
ApsCreateRandomNumber(
    __in ULONG Minimum,
    __in ULONG Maximum
    )
{
    ULONG Result;
    
    Result = (ULONG)(((double)rand()/(double)RAND_MAX) * Maximum + Minimum);
    return Result;
}

//
// N.B. Compute the closest LONG value to specified FLOAT
//

LONG
ApsComputeClosestLong(
    __in FLOAT Value
    )
{
    LONG Result;

    if (ceil(Value) - Value < 0.5) {
        Result = (ULONG)ceil(Value);
    } else {
        Result = (ULONG)floor(Value);
    }

    return Result;
}

VOID
ApsNormalizeDuration(
	__in ULONG Duration,
	__out PULONG Millisecond,
	__out PULONG Second,
	__out PULONG Minute
	)
{
	Duration = min(Duration, 1000 * 60 * 60);
	*Minute = Duration / (1000 * 60);
	*Second = Duration % (1000 * 60);
	*Millisecond = Duration % 1000; 
}

VOID
ApsFormatDuration(
	__in ULONG Millisecond,
	__in ULONG Second,
	__in ULONG Minute,
	__in PWCHAR Buffer,
	__in ULONG Length
	)
{
	StringCchPrintf(Buffer, Length, L"%02d:%02d:%03d", Minute, Second, Millisecond);
}

//
// Report Deduction
//

BOOLEAN ApsDeductionState;
ULONG ApsInclusiveThreshold;

BOOLEAN
ApsIsDeductionEnabled(
	VOID
	)
{
	return ApsDeductionState;
}

VOID
ApsEnableDeduction(
	__in BOOLEAN Enable
	)
{
	ApsDeductionState = Enable;
}

VOID
ApsSetDeductionThreshold(
	__in DEDUCTION_KEY Key,
	__in ULONG Value
	)
{
	ASSERT(Key == DEDUCTION_INCLUSIVE_PERCENT);
	ASSERT(Value < 100);

	ApsInclusiveThreshold = Value;
}

ULONG
ApsGetDeductionThreshold(
	__in DEDUCTION_KEY Key
	)
{
	return ApsInclusiveThreshold;
}

BOOLEAN
ApsIsCallNodeAboveThreshold(
	__in struct _CALL_GRAPH *Graph,
	__in struct _CALL_NODE *Node
	)
{
	if (!ApsDeductionState) {
		return TRUE;
	}

	if (Graph->Type == PROFILE_CPU_TYPE) {
		return (Node->Cpu.Inclusive * 100 / Graph->Inclusive) >= ApsInclusiveThreshold;
	}
	if (Graph->Type == PROFILE_MM_TYPE) {
		return (Node->Mm.InclusiveBytes * 100 / Graph->Inclusive) >= ApsInclusiveThreshold;
	}

	return FALSE;
}

BOOLEAN
ApsIsCallTreeAboveThreshold(
	__in struct _CALL_GRAPH *Graph,
	__in struct _CALL_TREE *Node
	)
{
	if (!ApsDeductionState) {
		return TRUE;
	}

	if (Graph->Type == PROFILE_CPU_TYPE) {
		return (Node->RootNode->Cpu.Inclusive * 100 / Graph->Inclusive) >= ApsInclusiveThreshold;
	}
	if (Graph->Type == PROFILE_MM_TYPE) {
		return (Node->RootNode->Mm.InclusiveBytes * 100 / Graph->Inclusive) >= ApsInclusiveThreshold;
	}

	return FALSE;
}

BOOLEAN
ApsValidatePath(
	__in PWSTR Path
	)
{
	HANDLE Handle;

	Handle = CreateFile(Path, GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 
						FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	
	CloseHandle(Handle);
	return TRUE;
}

SIZE_T
ApsGetBaseName(
	__in PWSTR FullPath,
	__out PWSTR BaseName,
	__in SIZE_T Length
	)
{
	WCHAR Base[MAX_PATH];
	WCHAR Extent[32];

	_wsplitpath(FullPath, NULL, NULL, Base, Extent);
	if (Extent[0] != 0) {
		wcscat(Base, Extent);
	}

	wcscpy_s(BaseName, Length, Base);
	return wcslen(BaseName);
}

//
// N.B. SourceFolder can contain mulitple strings
//

HANDLE
ApsOpenSourceFile(
	__in PWSTR SourceFolder,
	__in PWSTR SpecifiedPath,
	__out PWSTR *RealPath
	)
{
	HANDLE FileHandle;
	size_t Size;
	size_t Count;
	BOOL Status;
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PSTRING_ENTRY Entry;
	PWSTR Buffer;
	WCHAR BaseName[MAX_PATH];
	WCHAR ExtentName[16];

	*RealPath = NULL;
	FileHandle = INVALID_HANDLE_VALUE;

	ASSERT(SpecifiedPath != NULL);
	ASSERT(RealPath != NULL);

	//
	// Source folder is possible ""
	//

	if (!SourceFolder || !wcslen(SourceFolder)) {

		//
		// If source folder is not specified, directly open
		// the file by given path
		//

		FileHandle = CreateFile(SpecifiedPath, GENERIC_READ, 
								FILE_SHARE_READ, NULL, OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL, NULL);

		if (FileHandle == INVALID_HANDLE_VALUE) {
			return INVALID_HANDLE_VALUE;
		}

		//
		// Duplicate specified string to real path
 		//

		Count = wcslen(SpecifiedPath) + 1;
		Size = Count * sizeof(WCHAR);
		*RealPath = (PWSTR)ApsMalloc(Size);
		wcscpy_s(*RealPath, Count, SpecifiedPath);

		return FileHandle;
	}

	//
	// Build source file base name
	//

	BaseName[0] = 0;
	ExtentName[0] = 0;

	_wsplitpath(SpecifiedPath, NULL, NULL, BaseName, ExtentName);
	if (ExtentName[0] != 0) {
		wcscat(BaseName, ExtentName);
	}

	Buffer = (PWSTR)ApsMalloc(APS_PAGESIZE);

	//
	// Build source folder list
	//

	InitializeListHead(&ListHead);
	ApsSplitUnicodeString(SourceFolder, L';', &ListHead);

	//
	// Enum each source folder to scan the file
	//

	Status = FALSE;
	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Entry = CONTAINING_RECORD(ListEntry, STRING_ENTRY, ListEntry);
		Status = SearchTreeForFileW((PCWSTR)Entry->Buffer, BaseName, Buffer);

		if (Status) {
			*RealPath = Buffer;
			break;
		}

		ApsFree(Entry->Buffer);
		ApsFree(Entry);
	}

	//
	// Clean other folder strings
	//

	while (IsListEmpty(&ListHead) != TRUE) {
		ListEntry = RemoveHeadList(&ListHead);
		Entry = CONTAINING_RECORD(ListEntry, STRING_ENTRY, ListEntry);
		ApsFree(Entry->Buffer);
		ApsFree(Entry);
	}

	if (Status) {

		//
		// Open the source file
		//

		FileHandle = CreateFile(Buffer, GENERIC_READ, 
								FILE_SHARE_READ, NULL, OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL, NULL);

		if (FileHandle == INVALID_HANDLE_VALUE) {
			ApsFree(Buffer);
			*RealPath = NULL;
		}
	}
	
	return FileHandle;
}

BOOLEAN
ApsIsProcessLive(
	__in HANDLE ProcessHandle
	)
{
	ULONG Status;

	Status = WaitForSingleObject(ProcessHandle, 0);
	if (Status == WAIT_OBJECT_0) {
		return FALSE;
	}

	return TRUE;
}
