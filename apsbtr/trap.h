//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _TRAP_H_
#define _TRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "decode.h"

#pragma pack(push, 1)

#if defined(_M_IX86) 

typedef struct _BTR_TRAP_OBJECT {

	ULONG TrapFlag;
	PVOID Probe;                

	union {
		PVOID TrapProcedure;
		PVOID Callback;
	};

	PVOID BackwardAddress;
	ULONG HijackedLength;
	CHAR OriginalCopy[16];
	CHAR HijackedCode[32];

	UCHAR MovEsp4[4];
	ULONG Zero;
	UCHAR MovEsp8[4];
	PVOID Object;
	UCHAR Jmp[2];
	PVOID Procedure;
	UCHAR Ret;

} BTR_TRAP_OBJECT, *PBTR_TRAP_OBJECT;

#elif defined (_M_X64) 

typedef struct _BTR_TRAP_OBJECT{

	ULONG TrapFlag;
	PVOID Probe;                

	union {
		PVOID TrapProcedure;
		PVOID Callback;
	};

	PVOID BackwardAddress;
	ULONG HijackedLength;
	CHAR OriginalCopy[16];
	CHAR HijackedCode[32];

	UCHAR MovRsp8[5];
	ULONG Zero1;
	UCHAR MovRsp10[5];
	ULONG Zero2;
	UCHAR MovRsp18[4];
	ULONG ObjectLowPart;
	CHAR MovRsp1C[4];
	ULONG ObjectHighPart;
	UCHAR Jmp[2];
	ULONG Rip;
	PVOID Procedure;
	UCHAR Ret;

} BTR_TRAP_OBJECT, *PBTR_TRAP_OBJECT;		

#endif

#pragma pack(pop)

typedef struct _BTR_TRAP_PAGE {
	LIST_ENTRY ListEntry;
	PVOID StartVa;
	PVOID EndVa;
	BTR_BITMAP BitMap;
	BTR_TRAP_OBJECT Object[ANYSIZE_ARRAY];
} BTR_TRAP_PAGE, *PBTR_TRAP_PAGE;


ULONG
BtrInitializeTrap(
	__in PBTR_PROFILE_OBJECT Object	
	);

VOID
BtrUninitializeTrap(
	VOID
	);

PBTR_TRAP_PAGE
BtrAllocateTrapPage(
	IN ULONG_PTR SourceAddress
	);

VOID
BtrInitializeTrapPage(
	IN PBTR_TRAP_PAGE TrapPage 
	);

PBTR_TRAP_PAGE
BtrScanTrapPageList(
	IN ULONG_PTR SourceAddress
	);

BOOLEAN
BtrIsTrapPage(
	IN PVOID Address
	);

PBTR_TRAP_OBJECT
BtrAllocateTrap(
	IN ULONG_PTR SourceAddress
	);

VOID
BtrFreeTrap(
	IN PBTR_TRAP_OBJECT Trap
	);

ULONG
BtrFuseTrap(
	IN PVOID Address, 
	IN PBTR_DECODE_CONTEXT Decode,
	IN PBTR_TRAP_OBJECT Trap
	);

ULONG
BtrFuseTrapLite(
	IN PVOID Address, 
	IN PVOID Destine,
	IN PBTR_TRAP_OBJECT Trap
	);

PBTR_ADDRESS_RANGE
BtrGetTrapRange(
	OUT PULONG Count
	);

extern LIST_ENTRY BtrTrapPageList;
extern BTR_TRAP_OBJECT BtrTrapTemplate;

#ifdef __cplusplus
}
#endif
#endif