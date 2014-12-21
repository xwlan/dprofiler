//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_PAGE_H_
#define _MM_PAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mmprof.h"

//
// VirtualAlloc
//

LPVOID WINAPI 
VirtualAllocEnter(
	__in  LPVOID lpAddress,
	__in  SIZE_T dwSize,
	__in  DWORD flAllocationType,
	__in  DWORD flProtect
	);

typedef LPVOID 
(WINAPI *VIRTUALALLOC)(
	__in  LPVOID lpAddress,
	__in  SIZE_T dwSize,
	__in  DWORD flAllocationType,
	__in  DWORD flProtect
	);

//
// VirtualFree
//

BOOL WINAPI 
VirtualFreeEnter(
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD dwFreeType
	);

typedef BOOL 
(WINAPI *VIRTUALFREE)(
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD dwFreeType
	);

//
// VirtualAllocEx
//

LPVOID WINAPI 
VirtualAllocExEnter(
	__in HANDLE hProcess,
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD flAllocationType,
	__in DWORD flProtect
	);

typedef LPVOID 
(WINAPI *VIRTUALALLOCEX)(
	__in HANDLE hProcess,
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD flAllocationType,
	__in DWORD flProtect
	);

//
// VirtualFreeEx
//

BOOL WINAPI 
VirtualFreeExEnter(
	__in HANDLE hProcess,
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD dwFreeType
	);

typedef BOOL 
(WINAPI *VIRTUALFREEEX)(
	__in HANDLE hProcess,
	__in LPVOID lpAddress,
	__in SIZE_T dwSize,
	__in DWORD dwFreeType
	);

//
// External Delcaration
//

extern BTR_CALLBACK MmPageCallback[];
extern ULONG MmPageCallbackCount;

#ifdef __cplusplus
}
#endif
#endif