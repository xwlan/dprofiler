//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _CONDITION_H_
#define _CONDITION_H_

#include "dprofiler.h"
#include "apsbtr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_DTL_OBJECT;

typedef enum _ConditionObjectType {
    ConditionObjectProcess,
    ConditionObjectPID,
    ConditionObjectTID,
    ConditionObjectTime,
    ConditionObjectProbe,
	ConditionObjectProvider,
    ConditionObjectDuration,
    ConditionObjectReturn,
    ConditionObjectDetail,
    ConditionObjectNumber,
} ConditionObjectType;

typedef enum _ConditionRelationType {
    ConditionRelationEqual,
    ConditionRelationNotEqual,
    ConditionRelationLessThan,
    ConditionRelationMoreThan,
    ConditionRelationContain,
    ConditionRelationNotContain,
	ConditionRelationEarlyThan,
	ConditionRelationLaterThan,
    ConditionRelationNumber,
} ConditionRelationType;

typedef enum _ConditionOpType {
	ConditionOpAnd,
	ConditionOpOr,
} ConditionOpType;

typedef enum _ConditionActionType {
    ConditionActionInclude,
    ConditionActionExclude,
    ConditionActionNumber,
} ConditionActionType;

typedef enum _ConditionDataType {
	ConditionDataUlong,
	ConditionDataUlong64,
	ConditionDataDouble,
	ConditionDataString,
} ConditionDataType;

//
// LIST_ENTRY must be padded o work for both 32 and 64 bits
//

typedef struct _CONDITION_ENTRY {

	union {
	    LIST_ENTRY ListEntry;
		ULONG64 Padding[2];
	};

    ConditionObjectType Object;
    ConditionRelationType Relation;
    ConditionActionType Action;
	ConditionDataType Type;

    union {
        ULONG UlongValue;
		ULONG64 Ulong64Value;
        double DoubleValue;
        WCHAR StringValue[MAX_PATH];
    };

} CONDITION_ENTRY, *PCONDITION_ENTRY;

//
// dprobe filter condition file format
//

#define DFC_FILE_SIGNATURE  'cfd'

typedef struct _DFC_FILE {
	ULONG Signature;
	ULONG CheckSum;
	ULONG Size;  
	ULONG Flag;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ConditionOpType Type;
	ULONG NumberOfConditions;
	CONDITION_ENTRY Conditions[ANYSIZE_ARRAY];
} DFC_FILE, *PDFC_FILE;

typedef struct _CONDITION_OBJECT {
    LIST_ENTRY ListEntry;
    LIST_ENTRY ConditionListHead;
    ULONG NumberOfConditions;
    struct _MSP_DTL_OBJECT *DtlObject;
} CONDITION_OBJECT, *PCONDITION_OBJECT;

PCONDITION_OBJECT
ConditionClone(
	IN PCONDITION_OBJECT Object
	);

VOID
ConditionFree(
	IN PCONDITION_OBJECT Object
	);

BOOLEAN
ConditionIsIdentical(
	IN PCONDITION_OBJECT New,
	IN PCONDITION_OBJECT Old
	);

BOOLEAN
ConditionIsSuperSet(
	IN PCONDITION_OBJECT New,
	IN PCONDITION_OBJECT Old
	);

BOOLEAN
ConditionFindCondition(
	IN PCONDITION_OBJECT Condition,
	IN PCONDITION_ENTRY Entry
	);

typedef BOOLEAN
(*CONDITION_HANDLER)(
	IN struct _MSP_DTL_OBJECT * DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionProcessHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionTimeHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionProcessIdHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionThreadIdHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionProbeHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionReturnHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionDurationHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionDetailHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

BOOLEAN
ConditionProviderHandler(
	IN struct _MSP_DTL_OBJECT *DtlObject,
    IN PCONDITION_ENTRY Condition,
    IN PBTR_RECORD_HEADER Record
    );

ULONG
ConditionExportFile(
	IN PCONDITION_OBJECT Object,
	IN PWSTR Path
	);

ULONG
ConditionImportFile(
	IN PWSTR FileName,
	OUT PCONDITION_OBJECT *Condition
	);

ULONG
ConditionComputeCheckSum(
	IN PDFC_FILE Header
	);

VOID
ConditionSetValue(
	IN ConditionDataType Type,
	IN PWSTR StringValue,
	IN PCONDITION_ENTRY Entry
	);

VOID
ConditionGetValue(
	IN PCONDITION_ENTRY Entry,
	OUT PWCHAR Buffer,
	IN ULONG Length 
	);

extern CONDITION_HANDLER ConditionHandlers[ConditionObjectNumber];
extern ConditionDataType ConditionDataTypeMap[ConditionObjectNumber];

#ifdef __cplusplus
}
#endif

#endif