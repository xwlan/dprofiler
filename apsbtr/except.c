//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"

//
// Previous exception filter
//

PTOP_LEVEL_EXCEPTION_FILTER BtrPreviousExceptionFilter;

//
// Unhandled exception filter of runtime, note that
// we need to dispatch to previous registered filter
// if there's one
//

LONG WINAPI 
BtrUnhandledExceptionFilter(
	__in struct _EXCEPTION_POINTERS* Pointers 
	)
{
	//
	// Capture a full memory dump if BTR_FLAG_DUMP is specified
	//

	return 0;
}

VOID
BtrSetUnhandledExceptionFilter(
	VOID
	)
{
	//
	// Register runtime unhandled exception filter as current top level exceptio filter,
	// and save previous exception filter
	//

	BtrPreviousExceptionFilter = SetUnhandledExceptionFilter(BtrUnhandledExceptionFilter);
	return;
}
