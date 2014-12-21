//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_CRT_H_
#define _MM_CRT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mmprof.h"

//
// malloc
//

void* __cdecl
CrtMalloc(
	size_t size
	);

void* __cdecl
CrtMallocD(
	size_t size
	);

void* __cdecl
CrtMalloc71(
	size_t size
	);

void* __cdecl
CrtMalloc71D(
	size_t size
	);

void* __cdecl
CrtMalloc80(
	size_t size
	);

void* __cdecl
CrtMalloc80D(
	size_t size
	);

void* __cdecl
CrtMalloc90(
	size_t size
	);

void* __cdecl
CrtMalloc90D(
	size_t size
	);

void* __cdecl
CrtMalloc100(
	size_t size
	);

void* __cdecl
CrtMalloc100D(
	size_t size
	);

// 
// calloc
//

void* __cdecl
CrtCalloc( 
	size_t num,
	size_t size 
	);
 
void* __cdecl
CrtCallocD( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc71( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc71D( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc80( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc80D( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc90( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc90D( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc100( 
	size_t num,
	size_t size 
	);

void* __cdecl
CrtCalloc100D( 
	size_t num,
	size_t size 
	);

//
// realloc
//

void* __cdecl
CrtRealloc(
	void *ptr,
	size_t size 
	);
 
void* __cdecl
CrtReallocD(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc71(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc71D(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc80(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc80D(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc90(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc90D(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc100(
	void *ptr,
	size_t size 
	);

void* __cdecl
CrtRealloc100D(
	void *ptr,
	size_t size 
	);

//
// free
//

void __cdecl
CrtFree(
	void *ptr
	);

void __cdecl
CrtFreeD(
	void *ptr
	);

void __cdecl
CrtFree71(
	void *ptr
	);

void __cdecl
CrtFree71D(
	void *ptr
	);

void __cdecl
CrtFree80(
	void *ptr
	);

void __cdecl
CrtFree80D(
	void *ptr
	);

void __cdecl
CrtFree90(
	void *ptr
	);

void __cdecl
CrtFree90D(
	void *ptr
	);

void __cdecl
CrtFree100(
	void *ptr
	);

void __cdecl
CrtFree100D(
	void *ptr
	);

//
// operator new
//

void * __cdecl 
CrtNew(
	size_t size
	);

void * __cdecl 
CrtNewD(
	size_t size
	);

void * __cdecl 
CrtNew71(
	size_t size
	);

void * __cdecl 
CrtNew71D(
	size_t size
	);

void * __cdecl 
CrtNew80(
	size_t size
	);

void * __cdecl 
CrtNew80D(
	size_t size
	);

void * __cdecl 
CrtNew90(
	size_t size
	);

void * __cdecl 
CrtNew90D(
	size_t size
	);

void * __cdecl 
CrtNew100(
	size_t size
	);

void * __cdecl 
CrtNew100D(
	size_t size
	);

//
// operator delete
//

void __cdecl 
CrtDelete(
	void *ptr
	);

void __cdecl 
CrtDeleteD(
	void *ptr
	);

void __cdecl 
CrtDelete71(
	void *ptr
	);

void __cdecl 
CrtDelete71D(
	void *ptr
	);

void __cdecl 
CrtDelete80(
	void *ptr
	);

void __cdecl 
CrtDelete80D(
	void *ptr
	);

void __cdecl 
CrtDelete90(
	void *ptr
	);

void __cdecl 
CrtDelete90D(
	void *ptr
	);

void __cdecl 
CrtDelete100(
	void *ptr
	);

void __cdecl 
CrtDelete100D(
	void *ptr
	);

//
// operator new[]
//

void * __cdecl 
CrtNewArray(
	size_t size
	);

void * __cdecl 
CrtNewArrayD(
	size_t size
	);

void * __cdecl 
CrtNewArray71(
	size_t size
	);

void * __cdecl 
CrtNewArray71D(
	size_t size
	);

void * __cdecl 
CrtNewArray80(
	size_t size
	);

void * __cdecl 
CrtNewArray80D(
	size_t size
	);

void * __cdecl 
CrtNewArray90(
	size_t size
	);

void * __cdecl 
CrtNewArray90D(
	size_t size
	);

void * __cdecl 
CrtNewArray100(
	size_t size
	);

void * __cdecl 
CrtNewArray100D(
	size_t size
	);

//
// operator delete[]
//

void __cdecl 
CrtDeleteArray(
	void *ptr
	);

void __cdecl 
CrtDeleteArrayD(
	void *ptr
	);

void __cdecl 
CrtDeleteArray71(
	void *ptr
	);

void __cdecl 
CrtDeleteArray71D(
	void *ptr
	);

void __cdecl 
CrtDeleteArray80(
	void *ptr
	);

void __cdecl 
CrtDeleteArray80D(
	void *ptr
	);

void __cdecl 
CrtDeleteArray90(
	void *ptr
	);

void __cdecl 
CrtDeleteArray90D(
	void *ptr
	);

void __cdecl 
CrtDeleteArray100(
	void *ptr
	);

void __cdecl 
CrtDeleteArray100D(
	void *ptr
	);

typedef void* (__cdecl *MALLOC)(size_t size);
typedef void* (__cdecl *CALLOC)(size_t num, size_t size);
typedef void* (__cdecl *REALLOC)(void *ptr, size_t size);
typedef void (__cdecl *FREE)(void *ptr);

typedef void* (__cdecl *OP_NEW)(size_t size);
typedef void (__cdecl *OP_DELETE)(void *ptr);
typedef void* (__cdecl *OP_NEWARRAY)(size_t size);
typedef void (__cdecl *OP_DELETEARRAY)(void *ptr);

typedef intptr_t (__cdecl *CRT_GETHEAPHANDLE)(void);
 
extern CRT_GETHEAPHANDLE CrtGetHeapHandle;
extern CRT_GETHEAPHANDLE CrtGetHeapHandleD;
extern PVOID CrtHeapHandle;
extern PVOID CrtHeapHandleD;

extern CRT_GETHEAPHANDLE CrtGetHeapHandle71;
extern CRT_GETHEAPHANDLE CrtGetHeapHandle71D;
extern PVOID CrtHeapHandle71;
extern PVOID CrtHeapHandle71D;

extern CRT_GETHEAPHANDLE CrtGetHeapHandle80;
extern CRT_GETHEAPHANDLE CrtGetHeapHandle80D;
extern PVOID CrtHeapHandle80;
extern PVOID CrtHeapHandle80D;

extern CRT_GETHEAPHANDLE CrtGetHeapHandle90;
extern CRT_GETHEAPHANDLE CrtGetHeapHandle90D;
extern PVOID CrtHeapHandle90;
extern PVOID CrtHeapHandle90D;

extern CRT_GETHEAPHANDLE CrtGetHeapHandle100;
extern CRT_GETHEAPHANDLE CrtGetHeapHandle100D;
extern PVOID CrtHeapHandle100;
extern PVOID CrtHeapHandle100D;

//
// Mangled name for new/delete operators
//

#if defined(_M_IX86)

#define CrtNewName         "??2@YAPAXI@Z"
#define CrtDeleteName      "??3@YAXPAX@Z"
#define CrtNewArrayName    "??_U@YAPAXI@Z"
#define CrtDeleteArrayName "??_V@YAXPAX@Z"

#elif defined(_M_X64)

#define CrtNewName         "??2@YAPEAX_K@Z"
#define CrtDeleteName      "??3@YAXPEAX@Z"
#define CrtNewArrayName    "??_U@YAPEAX_K@Z"
#define CrtDeleteArrayName "??_V@YAXPEAX@Z"

#endif


#ifdef __cplusplus
}
#endif
#endif
