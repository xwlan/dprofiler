//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _EXEMPT_H_
#define _EXEMPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "hal.h"

#pragma pack(push, 1)

typedef struct _BTR_ADDRESS {
	ULONG_PTR StartVa;
	ULONG_PTR EndVa;
} BTR_ADDRESS, *PBTR_ADDRESS;

#pragma pack(pop)

struct _BTR_THREAD_OBJECT*
BtrIsExemptedCall(
	IN ULONG_PTR Caller 
	);

struct _BTR_THREAD_OBJECT*
BtrIsExemptedCallLite(
	IN ULONG_PTR Caller 
	);

BOOLEAN
BtrIsSystemDllAddress(
	IN PVOID Address
	);

VOID
BtrInitSystemDllAddress(
	VOID
	);

//
// Forward declare to enable inline
//

extern BOOLEAN BtrIgnoreSystemDll;
extern PWSTR BtrSystemDllName[];
extern ULONG BtrSystemDllCount;
extern ULONG BtrSystemDllFilledCount;

extern DECLSPEC_CACHEALIGN 
BTR_ADDRESS BtrSystemDllAddress[];

extern DECLSPEC_CACHEALIGN 
ULONG BtrStopped;

extern ULONG_PTR BtrDllBase;
extern ULONG_PTR BtrDllSize;

FORCEINLINE
VOID
BtrSetExemptionCookie(
	IN ULONG_PTR Cookie,
	OUT PULONG_PTR Value
	)
{
	PULONG_PTR StackBase;

	StackBase = BtrGetCurrentStackBase();
	*Value = *StackBase;
	*StackBase = Cookie;
}

FORCEINLINE
VOID
BtrClearExemptionCookie(
	IN ULONG_PTR Value 
	)
{
	PULONG_PTR StackBase;

	StackBase = BtrGetCurrentStackBase();
	*StackBase = Value;
}

FORCEINLINE
BOOLEAN
BtrIsExemptionCookieSet(
	IN ULONG_PTR Cookie
	)
{
	PULONG_PTR StackBase;

	StackBase = BtrGetCurrentStackBase();
	return (*StackBase == Cookie) ? TRUE : FALSE;
}

FORCEINLINE
ULONG
BtrIsStopped(
	VOID
	)
{
	ULONG Stopped;

	Stopped = ReadForWriteAccess(&BtrStopped);
	return Stopped;
}

#ifdef __cplusplus
}
#endif
#endif