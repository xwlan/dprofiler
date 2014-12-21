// testmm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include <process.h>
 
//
// Test HeapAlloc/malloc/SysAllocString in multi-threaded environment
//

UINT CALLBACK
MallocProcedure(
	IN PVOID Context
	);

int _tmain(int argc, _TCHAR* argv[])
{
	int i;
	char buf[MAX_PATH];

	_getche();

	for(i = 0; i < 10; i++) {
		_beginthreadex(NULL, 0, MallocProcedure, NULL, 0, NULL);
	}
	
	//
	// Test handle leak
	//

	for(i = 0; i < 100; i++) {
		sprintf(buf, "c:\\%d.txt", i);
		fopen(buf, "w");
	}
	_getche();

	return 0;
}

UINT CALLBACK
MallocProcedure(
	IN PVOID Context
	)
{
	int i;
	HANDLE HeapHandle;
	PVOID Ptr;
	BSTR bstr;

	//
	// Test process heap
	//

	HeapHandle = GetProcessHeap();

	for(i = 0; i < 1000; i++) {
		Ptr = HeapAlloc(HeapHandle, 0, 10);		
		HeapFree(HeapHandle, 0, Ptr);
	}

	for(i = 0; i < 1000; i++) {
		Ptr = HeapAlloc(HeapHandle, 0, 10);		
	}

	//
	// Test crt heap
	//

	for(i = 0; i < 1000; i++) {
		Ptr = malloc(10);
		free(Ptr);
	}

	for(i = 0; i < 1000; i++) {
		Ptr = malloc(10);
	}

	//
	// Test OLE BSTR
	//

	for(i = 0; i < 1000; i++) {
		bstr = SysAllocString(L"dprofiler");
		SysFreeString(bstr);
	}

	for(i = 0; i < 1000; i++) {
		bstr = SysAllocString(L"dprofiler");
	}

	
	return 0;
}