//
// lan.john@gmail.com 
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "bitmap.h"
#include "sdk.h"

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
BtrDebugTrace(
	IN PSTR Format,
	IN ...
	)
{
#ifdef _DEBUG
	CHAR Buffer [512];
	va_list Arg;

	va_start(Arg, Format);

	StringCchVPrintfA(Buffer, 512, Format, Arg);
	StringCchCatA(Buffer, 512, "\n");
	OutputDebugStringA(Buffer);	

	va_end(Arg);
#endif
}

VOID
BtrDumpBitMap(
	IN PBTR_BITMAP BitMap
	)
{

#ifdef _DEBUG

    ULONG i;
    BOOLEAN AllZeros, AllOnes;

    BtrDebugTrace(" BitMap:%08lx", BitMap);

    BtrDebugTrace(" (%08x)", BitMap->SizeOfBitMap);
    BtrDebugTrace(" %08lx\n", BitMap->Buffer);

    AllZeros = FALSE;
    AllOnes = FALSE;

    for (i = 0; i < ((BitMap->SizeOfBitMap + 31) / 32); i += 1) {

        if (BitMap->Buffer[i] == 0) {

            if (AllZeros) {

            } else {

                BtrDebugTrace("%4d:", i);
                BtrDebugTrace(" %08lx\n", BitMap->Buffer[i]);
            }

            AllZeros = TRUE;
            AllOnes = FALSE;

        } else if (BitMap->Buffer[i] == 0xFFFFFFFF) {

            if (AllOnes) {

            } else {

                BtrDebugTrace("%4d:", i);
                BtrDebugTrace(" %08lx\n", BitMap->Buffer[i]);
            }

            AllZeros = FALSE;
            AllOnes = TRUE;

        } else {

            AllZeros = FALSE;
            AllOnes = FALSE;

            BtrDebugTrace("%4d:", i);
            BtrDebugTrace(" %08lx\n", BitMap->Buffer[i]);
        }
    }

#endif

}