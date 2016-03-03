//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2011
//

#include "apsbtr.h"
#include "decode.h"
#include "util.h"

ULONG
BtrDisassemble(
	IN PVOID Address,
	OUT PBTR_OPCODE Opcode
	)
{
	UINT Length;

#if defined(_M_X64)

	Length = hde64_disasm(Address, Opcode); 

#elif defined(_M_IX86)

	Length = hde32_disasm(Address, Opcode);

#endif

	if (Opcode->flags & F_ERROR) {
		return 0;
	}

	return Length; 
}

BOOLEAN
BtrIsBranchOpcode(
	IN PUCHAR Code
	)
{
	//
	// jcc, jecxz etc
	//

	if (Code[0] >= (UCHAR)0x70 && Code[0] <= (UCHAR)0x7f) {
		return TRUE;
	}

	if (Code[0] == (UCHAR)0xe3) {
		return TRUE;	
	}

	if (Code[0] == (UCHAR)0x0f) {
		if (Code[1] >= (UCHAR)0x80 && Code[1] <= (UCHAR)0x8f) {
			return TRUE;
		}
		return FALSE;
	}

	//
	// jmp rel8
	// jmp rel32
	// call rel32
	//

	if (Code[0] == (UCHAR)0xeb || Code[0] == (UCHAR)0xe9 || Code[0] == (UCHAR)0xe8) {
		return TRUE;
	}

	if (Code[0] == (UCHAR)0xff) {
	
		if ((Code[1] == 0x15) || (Code[1] == 0x25)) {
		
			// 
			// jmp  r/m32
			// jmp  r/m64
			// call r/m32
			// call r/m64
			//

			return TRUE;
		}
	}

	//
	// ret, ret imm16
	//

	if (Code[0] == (UCHAR)0xc2 || Code[0] == (UCHAR)0xc3) {
		return TRUE;
	}

	//
	// don't check far call etc
	//

	return FALSE;
}

LONG_PTR
BtrDecodeBranchDestine(
	IN PCHAR Code,
	IN LONG_PTR TopLevel
	)
{
	LONG_PTR Destine;

	//
	// N.B. This routine can be executed recursively,
	// the followings opcodes are checked:
	// a. jmp rel8, jmp rel32, jmp r/m32, jmp r/m64
	// b. call rel32, call r/m32, call r/m64

	// N.B. If TopLevel == Destine, this imply a dead ring of
	// branch opcode, we detect this kind of stuff, however,
	// theoritically, it's a malicious, rare case.
	//

	if (Code[0] == 0xeb) {
		
		//
		// eb cb, jmp rel8
		//
		
		Destine = (LONG_PTR)(Code + 2 + (LONG_PTR)Code[1]);			
		if (Destine == TopLevel) {
			return 0;
		}
		return BtrDecodeBranchDestine((PCHAR)Destine, TopLevel);
	}
	
	if (Code[0] == 0xe9) {

		//
		// e9 cd, jmp rel32
		//

		Destine = (LONG_PTR)(Code + 5 + (LONG_PTR)*(PLONG)(Code + 1));			
		if (Destine == TopLevel) {
			return 0;
		}
		return BtrDecodeBranchDestine((PCHAR)Destine, TopLevel);
	} 

	if (Code[0] == 0xff && Code[1] == 0x25) {
	
		//
		// ff /4, jmp r/m32
		//        jmp r/m64
		//
		
		Destine = (LONG_PTR)*(PLONG)&Code[2];

#if defined(_M_X64)

		//
		// RIP relative addressing
		//

		Destine = (LONG_PTR)Code + 6 + Destine;
#endif

		Destine = *(PLONG_PTR)Destine;
		if (Destine == TopLevel) {
			return 0;
		}
		return BtrDecodeBranchDestine((PCHAR)Destine, TopLevel);
	}

	if (Code[0] == 0xe8) {
		
		//
		// e8 cd, call rel32
		//
	
		Destine = (LONG_PTR)(Code + 5 + (LONG_PTR)*(PLONG)(Code + 1));			
		return BtrDecodeBranchDestine((PCHAR)Destine, TopLevel);
	}

	if (Code[0] == 0xff && Code[1] == 0x15) {
		
		//
		// ff /2, call r/m32
		//        call r/m64
		//
	
		Destine = (LONG_PTR)*(PLONG)&Code[2];

#if defined(_M_X64)
		
		//
		// RIP relative addressing
		//

		Destine = (LONG_PTR)Code + 6 + Destine;
#endif

		Destine = *(PLONG_PTR)Destine;
		if (Destine == TopLevel) {
			return 0;
		}
		return BtrDecodeBranchDestine((PCHAR)Destine, TopLevel);
	}

	return (LONG_PTR)Code;	
}

ULONG
BtrDecodeRoutine(
	IN PVOID SourceAddress,	
	OUT PBTR_DECODE_CONTEXT Decode
	)
{
	ULONG Length;
	PUCHAR Buffer;
	ULONG TotalLength;
	PBTR_OPCODE Opcode;

	TotalLength = 0;
	Buffer = (PUCHAR)SourceAddress;
	Opcode = &Decode->Opcode[0];
	
	while (TotalLength < Decode->RequiredLength) {
		
		Length = BtrDisassemble(Buffer, Opcode);
		if (!Length) {
			return S_FALSE;
		}
		
		//
		// Mark whether there's any branch opcode, we don't do probe
		// if the scanned code bytes include any branch
		//

		if (BtrIsBranchOpcode(Buffer)) {
			if (!FlagOn(Decode->DecodeFlag, BTR_FLAG_BRANCH)) {
				SetFlag(Decode->DecodeFlag, BTR_FLAG_BRANCH);
			}
			break;
		}

		Buffer += Length;
		TotalLength += Length;

#if defined(_M_X64)

		//
		// ModR/M, mod=00B, r/m=101B => RIP+Disp32
		//

		if ((Opcode->modrm & 0xC7) == 0x05) {
		
			//
			// RIP relative addressing, this imply that the last operand is
			// a 32 bits displacement
			//

			Decode->RipDisplacementOffset = TotalLength - 4;
			ASSERT(Length >= 5);
		}

#endif 

		Opcode += 1;
		Decode->NumberOfOpcodes += 1;
	}

	Decode->ResultLength = TotalLength;
	return S_OK;
}

//
// N.B. Before decode any probe's code region, we first check whether
// it's a trampoline, it's either an import stub, debug stub, or patched
// by other modules. For this case, BTR redirect the destine address to
// BTR runtime without copy any instruction for this probe object, when
// the thread's PC leave BTR runtime, it's redirected to old destine address
// by a jmp instruction.
//

BTR_DECODE_FLAG
BtrComputeDestineAddress(
	IN PVOID Source,
	OUT PLONG_PTR Destine,
	OUT PLONG_PTR PatchAddress
	)
{
	PUCHAR Code;
	LONG_PTR Displacement;

	*Destine = 0;
	*PatchAddress = (LONG_PTR)Source;

	Code = (PUCHAR)Source;

	if (Code[0] == 0xff && Code[1] == 0x25) {

#if defined(_M_IX86)

		//
		// jmp dword ptr[x], x is absolute address
		//

		Displacement = *(PLONG_PTR)&Code[2];
		*Destine = *(PLONG_PTR)Displacement;
		return BTR_FLAG_JMP_RM32;

#elif defined(_M_X64)

		//
		// jmp qword ptr[x], x is RIP offset 
		//
		
		Displacement = *(PLONG)&Code[2];
		Displacement = (LONG_PTR)Source + 6 + Displacement;
		*Destine = *(PLONG_PTR)Displacement;
		return BTR_FLAG_JMP_RM64;
#endif

	} 

	else if (Code[0] == 0xe9) {
		
		//
		// jmp rel32
		//

		Displacement = *(PLONG)&Code[1];
		*Destine = (LONG_PTR)Source + 5 + Displacement;
		return BTR_FLAG_JMP_REL32;
	} 
	
	else if (Code[0] == 0xeb) {

		LONG_PTR TempFrom;
		LONG_PTR TempTo;
		LONG_PTR TempPatch;
		BTR_DECODE_FLAG Flag;

		//
		// jmp rel8, Windows 7/ Windows Server 2008 R2 use lots of
		// jmp rel8 stub to redirect API calls,we have to support
		// this kind of encoding.
		//

		Displacement = (LONG_PTR)(CHAR)Code[1];
		TempFrom = (LONG_PTR)Source + 2 + Displacement;
		Flag = BtrComputeDestineAddress((PVOID)TempFrom, &TempTo, &TempPatch);

		ASSERT(Flag != BTR_FLAG_JMP_REL8);
		*PatchAddress = TempFrom;
		*Destine = TempTo;
		return Flag;
	}

	return BTR_FLAG_JMP_UNSUPPORTED;
}