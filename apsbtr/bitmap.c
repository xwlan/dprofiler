//
// lan.john@gmail.com 
// Apsara Labs
// Copyright(C) 2009-2010
//

#include "bitmap.h"
#include "util.h"
#include <assert.h>

#ifndef ASSERT
#define ASSERT assert
#endif

VOID
BtrInitializeBitMap(
    IN PBTR_BITMAP BitMap, 
    IN PULONG Buffer,
    IN ULONG SizeOfBitMap
    )
{
    BitMap->SizeOfBitMap = SizeOfBitMap;
    BitMap->Buffer = Buffer;
}

VOID
BtrSetBit(
    IN PBTR_BITMAP BitMapHeader,
    IN ULONG BitNumber
    )
{
    PCHAR ByteAddress;
    ULONG ShiftCount;

    ASSERT(BitNumber < BitMapHeader->SizeOfBitMap);

    ByteAddress = (PCHAR)BitMapHeader->Buffer + (BitNumber >> 3);
    ShiftCount = BitNumber & 0x7;
    *ByteAddress |= (CHAR)(1 << ShiftCount);
}

VOID
BtrClearBit(
    IN PBTR_BITMAP BitMapHeader,
    IN ULONG BitNumber
    )
{
	PCHAR ByteAddress;
    ULONG ShiftCount;

    ASSERT(BitNumber < BitMapHeader->SizeOfBitMap);

    ByteAddress = (PCHAR)BitMapHeader->Buffer + (BitNumber >> 3);
    ShiftCount = BitNumber & 0x7;
    *ByteAddress &= (CHAR)(~(1 << ShiftCount));
}

BOOLEAN
BtrTestBit(
    IN PBTR_BITMAP BitMapHeader,
    IN ULONG BitNumber
    )
{
    PCHAR ByteAddress;
    ULONG ShiftCount;

    ASSERT(BitNumber < BitMapHeader->SizeOfBitMap);

    ByteAddress = (PCHAR)BitMapHeader->Buffer + (BitNumber >> 3);
    ShiftCount = BitNumber & 0x7;
    return (BOOLEAN)((*ByteAddress >> ShiftCount) & 1);
}

ULONG
BtrFindFirstSetBit(
	IN PBTR_BITMAP BitMap,
	IN ULONG FromIndex
	)
{
	BOOLEAN Status;
	ULONG Index;

	ASSERT(FromIndex < BitMap->SizeOfBitMap);

	for (Index = FromIndex; Index < BitMap->SizeOfBitMap; Index += 1) {
		Status = BtrTestBit(BitMap, Index);
		if (Status) {
			return Index; 
		}
	}

	return (ULONG)-1;
}

ULONG
BtrFindFirstClearBit(
	IN PBTR_BITMAP BitMap,
	IN ULONG FromIndex
	)
{
	BOOLEAN Status;
	ULONG Index;

	ASSERT(FromIndex < BitMap->SizeOfBitMap);

	for (Index = FromIndex; Index < BitMap->SizeOfBitMap; Index += 1) {
		Status = BtrTestBit(BitMap, Index);
		if (Status != TRUE) {
			return Index; 
		}
	}

	return (ULONG)-1;
}

ULONG
BtrFindFirstClearBitBackward(
	IN PBTR_BITMAP BitMap,
	IN ULONG FromIndex
	)
{
	BOOLEAN Status;
	ULONG Index;

	ASSERT(FromIndex < BitMap->SizeOfBitMap);

	for (Index = FromIndex; Index != -1; Index -= 1) {
		Status = BtrTestBit(BitMap, Index);
		if (Status != TRUE) {
			return Index; 
		}
	}

	return (ULONG)-1;
}

VOID
BtrClearAllBits(
    PBTR_BITMAP BitMapHeader
    )
{
	ULONG Number;

	for(Number = 0; Number < (BitMapHeader->SizeOfBitMap + 31) / 32; Number += 1) {
		BitMapHeader->Buffer[Number] = 0;
	}
}

VOID
BtrSetAllBits(
    PBTR_BITMAP BitMapHeader
    )
{
	ULONG Number;

	for(Number = 0; Number < (BitMapHeader->SizeOfBitMap + 31) / 32; Number += 1) {
		BitMapHeader->Buffer[Number] = -1;
	}
}

BOOLEAN
BtrAreAllBitsClear(
	IN PBTR_BITMAP BitMap
	)
{
	ULONG Count;
	ULONG Number;
	PCHAR Buffer;

	Buffer = (PCHAR)BitMap->Buffer;
	Count = BitMap->SizeOfBitMap / 8;

	for(Number = 0; Number < Count; Number += 1) {
		if (Buffer[Number] != 0) {
			return FALSE;
		}
	}

	return TRUE;
}

BOOLEAN
BtrAreAllBitsSet(
	IN PBTR_BITMAP BitMap
	)
{
	ULONG Count;
	ULONG Number;
	PCHAR Buffer;

	Buffer = (PCHAR)BitMap->Buffer;
	Count = BitMap->SizeOfBitMap / 8;

	for(Number = 0; Number < Count; Number += 1) {
		if (Buffer[Number] != (CHAR)0xFF) {
			return FALSE;
		}
	}

	return TRUE;
}

VOID
BtrDumpBitMap(
	IN PBTR_BITMAP BitMap
	)
{

#ifdef _DEBUG

    ULONG i;
    BOOLEAN AllZeros, AllOnes;

    DebugTrace(" BitMap:%08lx", BitMap);

    DebugTrace(" (%08x)", BitMap->SizeOfBitMap);
    DebugTrace(" %08lx\n", BitMap->Buffer);

    AllZeros = FALSE;
    AllOnes = FALSE;

    for (i = 0; i < ((BitMap->SizeOfBitMap + 31) / 32); i += 1) {

        if (BitMap->Buffer[i] == 0) {

            if (AllZeros) {

            } else {

                DebugTrace("%4d:", i);
                DebugTrace(" %08lx\n", BitMap->Buffer[i]);
            }

            AllZeros = TRUE;
            AllOnes = FALSE;

        } else if (BitMap->Buffer[i] == 0xFFFFFFFF) {

            if (AllOnes) {

            } else {

                DebugTrace("%4d:", i);
                DebugTrace(" %08lx\n", BitMap->Buffer[i]);
            }

            AllZeros = FALSE;
            AllOnes = TRUE;

        } else {

            AllZeros = FALSE;
            AllOnes = FALSE;

            DebugTrace("%4d:", i);
            DebugTrace(" %08lx\n", BitMap->Buffer[i]);
        }
    }

#endif

}