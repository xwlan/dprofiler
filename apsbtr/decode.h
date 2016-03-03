//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _DECODE_H_
#define _DECODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"

#if defined (_M_IX86)
#include "hde32.h"
#endif

#if defined (_M_X64)
#include "hde64.h"
#endif

typedef enum _BTR_DECODE_FLAG {
	BTR_FLAG_TAIL            = 0x00000001,
	BTR_FLAG_BRANCH          = 0x00000002,
	BTR_FLAG_JMP_UNSUPPORTED = 0x00000004,
	BTR_FLAG_JMP_RM32        = 0x00000008,
	BTR_FLAG_JMP_RM64        = 0x00000010,
	BTR_FLAG_JMP_REL32       = 0x00000020,
	BTR_FLAG_JMP_REL8        = 0x00000040,
	BTR_FLAG_CALLBACK        = 0x00000080,
} BTR_DECODE_FLAG, *PBTR_DECODE_FLAG;

typedef struct _BTR_DECODE_CONTEXT {
	ULONG RequiredLength;
	ULONG ResultLength;
	ULONG DecodeFlag;
	ULONG NumberOfOpcodes;
	BTR_OPCODE Opcode[5];
	ULONG RipDisplacementOffset;
} BTR_DECODE_CONTEXT, *PBTR_DECODE_CONTEXT;

ULONG
BtrDecodeRoutine(
	IN PVOID SourceAddress,	
	OUT PBTR_DECODE_CONTEXT Decode
	);

ULONG
BtrDisassemble(
	IN PVOID Address,
	OUT PBTR_OPCODE Opcode
	);

BOOLEAN
BtrIsBranchOpcode(
	IN PUCHAR Code
	);

LONG_PTR
BtrDecodeBranchDestine(
	IN PCHAR Code,
	IN LONG_PTR TopLevel
	);

BTR_DECODE_FLAG
BtrComputeDestineAddress(
	IN PVOID Source,
	OUT PLONG_PTR Destine,
	OUT PLONG_PTR PatchAddress
	);

#ifdef __cplusplus
}
#endif

#endif