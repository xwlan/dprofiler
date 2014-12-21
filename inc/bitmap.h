//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BITMAP_H_
#define _BITMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef struct _BTR_BITMAP {
    ULONG SizeOfBitMap;
    PULONG Buffer; 
} BTR_BITMAP, *PBTR_BITMAP;

VOID
BtrInitializeBitMap(
    IN PBTR_BITMAP BitMap, 
    IN PULONG Buffer,
    IN ULONG SizeOfBitMap
    );

VOID
BtrClearBit(
    IN PBTR_BITMAP BitMapHeader,
    IN ULONG BitNumber
    );

VOID
BtrSetBit(
    IN PBTR_BITMAP BitMapHeader,
    IN ULONG BitNumber
    );

BOOLEAN
BtrTestBit(
    IN PBTR_BITMAP BitMapHeader,
    IN ULONG BitNumber
    );

VOID
BtrClearAllBits(
    IN PBTR_BITMAP BitMapHeader
    );

VOID
BtrSetAllBits(
    IN PBTR_BITMAP BitMapHeader
    );

ULONG
BtrFindFirstSetBit(
	IN PBTR_BITMAP BitMap,
	IN ULONG FromIndex
	);

ULONG
BtrFindFirstClearBit(
	IN PBTR_BITMAP BitMap,
	IN ULONG FromIndex
	);

ULONG
BtrFindFirstClearBitBackward(
	IN PBTR_BITMAP BitMap,
	IN ULONG FromIndex
	);

BOOLEAN
BtrAreAllBitsClear(
	IN PBTR_BITMAP BitMap
	);

BOOLEAN
BtrAreAllBitsSet(
	IN PBTR_BITMAP BitMap
	);

VOID
BtrDumpBitMap(
	IN PBTR_BITMAP BitMap
	);

#ifdef __cplusplus
}
#endif

#endif