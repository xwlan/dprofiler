//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _APS_PRIVATE_H_
#define _APS_PRIVATE_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

VOID
ApsComputeHardwareFrequency(
	VOID
	);

VOID __cdecl
ApsDebugTrace(
	IN PSTR Format,
	IN ...
	);

//
// Extern Declaration
//

extern BOOLEAN ApsIs64Bits;
extern BOOLEAN ApsIsWow64;

extern ULONG ApsMajorVersion;
extern ULONG ApsMinorVersion;
extern ULONG ApsPageSize;

//
// Macros
//

#ifndef FlagOn
#define FlagOn(F,SF)  ((F) & (SF))     
#endif

#ifndef SetFlag
#define SetFlag(F,SF)   { (F) |= (SF);  }
#endif

#ifndef ClearFlag
#define ClearFlag(F,SF) { (F) &= ~(SF); }
#endif

#ifndef ASSERT
#define ASSERT assert
#endif

#ifdef __cplusplus
}
#endif
#endif