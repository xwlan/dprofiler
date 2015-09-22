//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
// 

#include "find.h"

BOOLEAN
FindIsComplete(
	__in struct _FIND_CONTEXT *Context,
	__in ULONG Current
	)
{
	if (Context->FindForward) {
		return TRUE;
	}

	if (Current != -1) {
		return FALSE;
	}
	
	return TRUE;
}