#include "condition.h"
#include "mspdtl.h"
#include "mspprocess.h"

CONDITION_HANDLER 
ConditionHandlers[ConditionObjectNumber] = {
    ConditionProcessHandler,
    ConditionProcessIdHandler,
    ConditionThreadIdHandler,
    ConditionTimeHandler,
    ConditionProbeHandler,
	ConditionProviderHandler,
    ConditionDurationHandler,
    ConditionReturnHandler,
    ConditionDetailHandler,
};

ConditionDataType 
ConditionDataTypeMap[ConditionObjectNumber] = {
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
	ConditionDataString,
};

PCONDITION_OBJECT
ConditionClone(
	IN PCONDITION_OBJECT Object
	)
{
	PCONDITION_OBJECT Clone;
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Source, Destine;

	ASSERT(Object != NULL);
	ASSERT(IsListEmpty(&Object->ConditionListHead) != TRUE);

	Clone = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
	Clone->DtlObject = NULL;
	Clone->NumberOfConditions = 0;
	InitializeListHead(&Clone->ConditionListHead);

	ListEntry = Object->ConditionListHead.Flink;
	while (ListEntry != &Object->ConditionListHead) {

		Destine = (PCONDITION_ENTRY)SdkMalloc(sizeof(CONDITION_ENTRY));
		Source = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		RtlCopyMemory(Destine, Source, sizeof(CONDITION_ENTRY));
		InsertHeadList(&Clone->ConditionListHead, &Destine->ListEntry);
		Clone->NumberOfConditions += 1;

		ListEntry = ListEntry->Flink;
	}

	return Clone;
}

VOID
ConditionFree(
	IN PCONDITION_OBJECT Object
	)
{
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Entry;

	if (!Object) {
		return;
	}

	while (IsListEmpty(&Object->ConditionListHead) != TRUE) {
		ListEntry = RemoveHeadList(&Object->ConditionListHead);
		Entry = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		SdkFree(Entry);
	}

	SdkFree(Object);
}

BOOLEAN
ConditionIsIdentical(
	IN PCONDITION_OBJECT New,
	IN PCONDITION_OBJECT Old
	)
{
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Entry;

	if (New->NumberOfConditions != Old->NumberOfConditions) {
		return FALSE;
	}

	ListEntry = New->ConditionListHead.Flink;
	while (ListEntry != &New->ConditionListHead) {
		Entry = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		if (ConditionFindCondition(Old, Entry) != TRUE) {
			return FALSE;
		}
		ListEntry = ListEntry->Flink;
	}

	return TRUE;
}

//
// Check whether new filter is superset of old filter, if TRUE,
// we can reuse old filter index to apply new filter, for other
// case, rebuild the index from the very beginning. 
//

BOOLEAN
ConditionIsSuperSet(
	IN PCONDITION_OBJECT New,
	IN PCONDITION_OBJECT Old
	)
{
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Entry;

	//
	// If new filter is superset of old filter, its condition number should 
	// be greater than old's.
	//

	if (New->NumberOfConditions <= Old->NumberOfConditions) {
		return FALSE;
	}

	//
	// For each old filter condition, walk the new filter condition list
	// to check whether it exists, if not, return FALSE
	//

	ListEntry = Old->ConditionListHead.Flink;
	while (ListEntry != &Old->ConditionListHead) {
		Entry = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		if (ConditionFindCondition(New, Entry) != TRUE) {
			return FALSE;
		}
		ListEntry = ListEntry->Flink;
	}

	return TRUE;
}

BOOLEAN
ConditionFindCondition(
	IN PCONDITION_OBJECT Condition,
	IN PCONDITION_ENTRY Entry
	)
{
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Entry2;

	ListEntry = Condition->ConditionListHead.Flink;
	while (ListEntry != &Condition->ConditionListHead) {
		Entry2 = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		if (Entry2->Object == Entry->Object && Entry2->Relation == Entry->Relation &&
			Entry2->Action == Entry->Action && !wcscmp(Entry2->StringValue, Entry->StringValue)) {
			return TRUE;
		}
		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
ConditionProcessHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
    BOOLEAN Equal;
	WCHAR Buffer[MAX_PATH];

    //
	// N.B. Only ConditionRelationEqual, or ConditionRelationNotEqual are valid
	//

	MspDecodeRecord(DtlObject, Record, DECODE_PROCESS, Buffer, MAX_PATH);
	Equal = !wcscmp(Buffer, Condition->StringValue);

	if (Equal) {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }

        return FALSE;
    
    }

    else {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }

        return FALSE;
    }
}

BOOLEAN
ConditionTimeHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
    return TRUE;
}

BOOLEAN
ConditionProcessIdHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
    int ProcessId;

    ProcessId = _wtoi(Condition->StringValue);  
    if (ProcessId == Record->ProcessId) {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }

        return FALSE;
    
    }

    else {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }

        return FALSE;
    }
}

BOOLEAN
ConditionThreadIdHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
    int ThreadId;

    ThreadId = _wtoi(Condition->StringValue);  
    if (ThreadId == Record->ThreadId) {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }

        return FALSE;
    
    }

    else {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }

        return FALSE;
    }
}

BOOLEAN
ConditionProbeHandler(
	IN struct _MSP_DTL_OBJECT * DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
	BOOLEAN Equal;
	WCHAR Buffer[MAX_PATH];

    //
	// N.B. Only ConditionRelationEqual, or ConditionRelationNotEqual are valid
	//

	MspDecodeRecord(DtlObject, Record, DECODE_PROBE, Buffer, MAX_PATH);
	Equal = !wcscmp(Buffer, Condition->StringValue);

	if (Equal) {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }

        return FALSE;
    
    }

    else {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }

        return FALSE;
    }
}

BOOLEAN
ConditionReturnHandler(
	IN struct _MSP_DTL_OBJECT * DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
    ULONG_PTR ReturnValue;
	ULONG_PTR Return;
	PWCHAR Prefix;

	//
	// N.B. Only ConditionRelationEqual or ConditionRelationNotEqual are valid
	//

	Return = (ULONG_PTR)Record->Return;
	Prefix = wcschr(Condition->StringValue, L'x');
	if (Prefix) {
		ReturnValue = (ULONG_PTR)_wtoi64(Prefix + 1);
	} else {
		ReturnValue = (ULONG_PTR)_wtoi64(Condition->StringValue);
	}

	if (ReturnValue == Return) {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }

        return FALSE;
    
    }

    else {

        if (Condition->Relation == ConditionRelationEqual && 
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }  

        if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
        
        if (Condition->Relation == ConditionRelationNotEqual && 
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }  

        if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }

        return FALSE;
    }
}

BOOLEAN
ConditionDurationHandler(
	IN struct _MSP_DTL_OBJECT * DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
    double Duration;

	//
	// N.B. Only ConditionRelationMoreThan is valid
	//

	Duration = _wtof(Condition->StringValue);
	if (BspComputeMilliseconds(Record->Duration) > Duration) {

		if (Condition->Relation == ConditionRelationMoreThan &&
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }
		
		if (Condition->Relation == ConditionRelationMoreThan &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
		
        return FALSE;
    
    }

    else {

		if (Condition->Relation == ConditionRelationMoreThan &&
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }
		
		if (Condition->Relation == ConditionRelationMoreThan &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
		
        return FALSE;
    }
}

BOOLEAN
ConditionDetailHandler(
	IN struct _MSP_DTL_OBJECT * DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
	WCHAR Buffer[MAX_PATH];
	PWCHAR Found;

	MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Buffer, MAX_PATH);
	Found = wcsstr(Buffer, Condition->StringValue);

	if (Found != NULL) {

		if ((Condition->Relation == ConditionRelationContain || Condition->Relation == ConditionRelationEqual) &&
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }
		
		if (Condition->Relation == ConditionRelationNotContain &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }
		
        return FALSE;
    
    } else {

		if (Condition->Relation == ConditionRelationContain &&
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }
		
		if (Condition->Relation == ConditionRelationNotContain &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
		
        return FALSE;
    }
}

BOOLEAN
ConditionProviderHandler(
	IN struct _MSP_DTL_OBJECT * DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    )
{
	BOOLEAN Equal;
	WCHAR Buffer[MAX_PATH];

    //
	// N.B. Only ConditionRelationEqual, or ConditionRelationNotEqual are valid
	//

	MspDecodeRecord(DtlObject, Record, DECODE_PROVIDER, Buffer, MAX_PATH);
	Equal = !wcscmp(Buffer, Condition->StringValue);

	if (Equal) {

		if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }
		
		if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
			return FALSE;
		}

		if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }

		if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
		
        return FALSE;
    
    } else {

		if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionInclude) {
            return FALSE;
        }
		
		if (Condition->Relation == ConditionRelationEqual &&
            Condition->Action == ConditionActionExclude) {
            return TRUE;
        }
		
		if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionInclude) {
            return TRUE;
        }

		if (Condition->Relation == ConditionRelationNotEqual &&
            Condition->Action == ConditionActionExclude) {
            return FALSE;
        }

        return FALSE;
    }
}

ULONG
ConditionExportFile(
	IN PCONDITION_OBJECT Object,
	IN PWSTR Path
	)
{
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Condition;
	HANDLE FileHandle;
	ULONG Status;
	ULONG Number;
	ULONG Length;
	PDFC_FILE ConditionFile;

	if (!Object->NumberOfConditions) {
		return ERROR_SUCCESS;
	}

	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
	                        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}
	
	Length = FIELD_OFFSET(DFC_FILE, Conditions[Object->NumberOfConditions]);
	ConditionFile = (PDFC_FILE)_alloca(Length);
	ConditionFile->Signature = DFC_FILE_SIGNATURE;
	ConditionFile->Size = Length;
	ConditionFile->Flag = 0;
	ConditionFile->MajorVersion = 1;
	ConditionFile->MinorVersion = 0;

	//
	// Currently support only AND operator
	//

	ConditionFile->Type = ConditionOpAnd;
	ConditionFile->NumberOfConditions = Object->NumberOfConditions;

	Number = 0;
	ListEntry = Object->ConditionListHead.Flink;
	while (ListEntry != &Object->ConditionListHead) {

		Condition = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		RtlCopyMemory(&ConditionFile->Conditions[Number], Condition, sizeof(CONDITION_ENTRY));

		Number += 1;
		ListEntry = ListEntry->Flink;
	}

	ASSERT(Number == Object->NumberOfConditions);

	//
	// Compute checksum 
	//

	ConditionFile->CheckSum = ConditionComputeCheckSum(ConditionFile);
	Status = WriteFile(FileHandle, ConditionFile, Length, &Number, NULL);
	if (!Status) {
		Status = GetLastError();
	} else {
		Status = ERROR_SUCCESS;
	}

	CloseHandle(FileHandle);

	//
	// In case of error delete the junk file
	//

	if (Status != ERROR_SUCCESS) {
		DeleteFile(Path);
	}

	return Status;
}

ULONG
ConditionImportFile(
	IN PWSTR Path,
	OUT PCONDITION_OBJECT *Condition
	)
{
	ULONG Status;
	HANDLE FileHandle;
	HANDLE ViewHandle;
	ULONG Number;
	PDFC_FILE File;
	PCONDITION_OBJECT Object;
	PCONDITION_ENTRY Entry;
	ULONG CheckSum;

	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
			                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	ViewHandle = CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!ViewHandle) {
		Status = GetLastError();
		CloseHandle(FileHandle);
		return Status;
	}

	File = (PDFC_FILE)MapViewOfFile(ViewHandle, FILE_MAP_READ, 0, 0, 0);
	if (!File) {
		Status = GetLastError();
		CloseHandle(ViewHandle);
		CloseHandle(FileHandle);
		return Status;
	}

	//
	// Simple check of dfc file integrity 
	//

	if (File->Signature != (ULONG)DFC_FILE_SIGNATURE || 
		File->Size != (ULONG)GetFileSize(FileHandle, NULL)) {
		return ERROR_INVALID_DATA;
	}

	CheckSum = ConditionComputeCheckSum(File);
	if (CheckSum != File->CheckSum) {
		MessageBox(NULL, L"The dfc file is corrupted!", L"D Probe", MB_OK|MB_ICONERROR);
		CloseHandle(ViewHandle);
		CloseHandle(FileHandle);
		return ERROR_INVALID_DATA;
	}

	Object = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
	InitializeListHead(&Object->ConditionListHead);
	Object->NumberOfConditions = File->NumberOfConditions;
	
	for(Number = 0; Number < File->NumberOfConditions; Number += 1) {
		Entry = (PCONDITION_ENTRY)SdkMalloc(sizeof(CONDITION_ENTRY));
		RtlCopyMemory(Entry, &File->Conditions[Number], sizeof(CONDITION_ENTRY));
		InsertTailList(&Object->ConditionListHead, &Entry->ListEntry);
	}

	UnmapViewOfFile(File);
	CloseHandle(ViewHandle);
	CloseHandle(FileHandle);

	Object->DtlObject = NULL;
	*Condition = Object;

	return ERROR_SUCCESS;
}

ULONG
ConditionComputeCheckSum(
	IN PDFC_FILE Header
	)
{
	PULONG Buffer;
	ULONG Result;
	ULONG Seed;
	ULONG Number;
	ULONG i;

	Seed = DFC_FILE_SIGNATURE;
	Result = 0;

	//
	// N.B. Must ensure that the file is aligned with ULONG size.
	// before compute checksum, the total size must be filled appropriately.
	// the region computed excludes Signature and CheckSum fileds, include
	// all left parts of the dpc file.
	//

	ASSERT(Header->Size != 0);
	ASSERT(Header->Size % sizeof(ULONG) == 0);

	Buffer = (PULONG)&Header->Size;
	Number = (Header->Size - FIELD_OFFSET(DFC_FILE, Size)) / sizeof(ULONG);

	for(i = 0; i < Number; i += 1) {
		Result += Seed ^ Buffer[i];
	}

	DebugTrace("ConditionComputeCheckSum  0x%08x", Result);
	return Result;
}

VOID
ConditionSetValue(
	IN ConditionDataType Type,
	IN PWSTR StringValue,
	IN PCONDITION_ENTRY Entry
	)
{
	switch (Type) {

		case ConditionDataUlong:
			Entry->UlongValue = (ULONG)wcstol(StringValue, NULL, 16);
			break;

		case ConditionDataUlong64:
			Entry->Ulong64Value = (ULONG)_wcstoui64(StringValue, NULL, 16);
			break;

		case ConditionDataDouble:
		case ConditionDataString:
			StringCchCopy(Entry->StringValue, MAX_PATH, StringValue);
			break;

		default:
			StringCchCopy(Entry->StringValue, MAX_PATH, StringValue);
	}	
}

VOID
ConditionGetValue(
	IN PCONDITION_ENTRY Entry,
	OUT PWCHAR Buffer,
	IN ULONG Length 
	)
{
	switch (Entry->Type) {

		case ConditionDataUlong:
			_ultow_s(Entry->UlongValue, Buffer, Length, 16);
			break;

		case ConditionDataUlong64:
			_ui64tow_s(Entry->UlongValue, Buffer, Length, 16);
			break;

			//
			// N.B. double is encoded as string
			//

		case ConditionDataDouble:
		case ConditionDataString:
			StringCchCopy(Buffer, Length, Entry->StringValue);
			break;

		default:
			StringCchCopy(Buffer, Length, Entry->StringValue);
	}	

}