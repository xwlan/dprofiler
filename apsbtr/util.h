//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "lock.h"
#include <crtdbg.h>

#define ASSERT _ASSERT

#if defined (_M_IX86)
#define GOLDEN_RATIO 0x9E3779B9

#elif defined (_M_X64)
#define GOLDEN_RATIO 0x9E3779B97F4A7C13
#endif

ULONG_PTR
BtrUlongPtrRoundDown(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	);

ULONG_PTR
BtrUlongPtrRoundUp(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	);

ULONG
BtrUlongRoundDown(
	__in ULONG Value,
	__in ULONG Align
	);

ULONG
BtrUlongRoundUp(
	__in ULONG Value,
	__in ULONG Align
	);

ULONG64
BtrUlong64RoundDown(
	__in ULONG64 Value,
	__in ULONG64 Align
	);

ULONG64
BtrUlong64RoundUp(
	__in ULONG64 Value,
	__in ULONG64 Align
	);

BOOLEAN
BtrIsExecutableAddress(
	__in PVOID Address
	);

BOOLEAN
BtrIsExecutingRange(
	__in PLIST_ENTRY ListHead,
	__in PVOID Address,
	__in ULONG Length
	);

BOOLEAN
BtrIsValidAddress(
	__in PVOID Address
	);

ULONG
BtrGetByteOffset(
	__in PVOID Address
	);

ULONG 
BtrCompareMemoryPointer(
	__in PVOID Source,
	__in PVOID Destine,
	__in ULONG NumberOfUlongPtr
	);

ULONG
BtrComputeAddressHash(
	__in PVOID Address,
	__in ULONG Limit
	);

ULONG
BtrGetImageInformation(
	__in PVOID ImageBase,
	__out PULONG Timestamp,
	__out PULONG CheckSum
	);

ULONG
BtrWriteMinidump(
	__in PWSTR Path,
	__in BOOLEAN Full,
	__out PHANDLE DumpHandle 
	);

VOID
BtrZeroSmallMemory(
	__in PUCHAR Ptr,
	__in ULONG Size
	);

VOID
BtrCopySmallMemory(
	__in PUCHAR Destine,
	__in PUCHAR Source,
	__in ULONG Size
	);

BOOLEAN
BtrProbePtrPtr(
	_In_ PVOID Buffer
	);

BOOLEAN
BtrProbeBuffer(
	__in PVOID Buffer,
	__in ULONG Size
	);

BOOLEAN
BtrValidateHandle(
	__in HANDLE Handle
	);

int
BtrPrintfA(
	__out char *buffer,
	__in size_t count,
	__in const char *format,
	... 
	);

int
BtrPrintfW(
	__out wchar_t *buffer,
	__in size_t count,
	__in const wchar_t *format,
	... 
	);

VOID
DebugTrace(
	__in PSTR Format,
	...
	);

VOID 
DebugTrace2(
	__in PSTR Format,
	...
	);

VOID 
BtrTrace(
	__in PSTR Format,
	...
	);

VOID 
BtrTrace2(
	__in PSTR Format,
	...
	);

ULONG
BtrCreateRandomNumber(
    __in ULONG Minimum,
    __in ULONG Maximum
    );

struct _BTR_SPINLOCK;

ULONG64
BtrInterlockedAdd64(
	_In_ volatile ULONG64 *Destine,
	_In_ ULONG Increment,
	_In_ struct _BTR_SPINLOCK *Lock
	);

#ifdef __cplusplus
}
#endif

#endif