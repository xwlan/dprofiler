//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "sdk.h"
#include "aps.h"
#include "apspdb.h"
#include "mdb.h"
#include "util.h"

#define SYMPATH_SIZE (32 * 1024)

PWSTR UtilSymbolPath;

PWSTR
UtilGetSymbolPath(
	VOID
	)
{
	ULONG Length;

	if (!UtilSymbolPath) {
		UtilSymbolPath = (PWSTR)SdkMalloc(SYMPATH_SIZE);
	}

	//
	// Get user custom symbol path
	//

	Length = MdbGetSymbolPath(UtilSymbolPath, SYMPATH_SIZE);
	ASSERT(Length != -1);

	if (!Length) {
		Length = 1;
	}

	//
	// Get default symbol path
	//

	Length = (Length - 1) / sizeof(WCHAR);
	ApsGetSymbolPath(UtilSymbolPath + Length, SYMPATH_SIZE - Length * sizeof(WCHAR));
	return UtilSymbolPath;
}

