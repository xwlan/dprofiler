//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _THREAD_H_
#define _THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "btr.h"
#include "lock.h"
#include "exempt.h"
#include "stacktrace.h"

//
// N.B. Systsem TEB size is 4K on x86, 8K on x64
// The size must be ensured by compiler assert
//

typedef struct _BTR_TEB {
	ULONG_PTR Teb[1023];
	struct _BTR_THREAD_OBJECT *Thread;
} BTR_TEB, *PBTR_TEB;

#if defined (_M_IX86)

C_ASSERT(sizeof(BTR_TEB) == 4096);

#endif

#if defined (_M_X64) 

C_ASSERT(sizeof(BTR_TEB) == (4096 * 2));

#endif

#define STACK_TLS_SIZE 10

#pragma pack(push, 16)
typedef struct _BTR_THREAD_OBJECT {

	LIST_ENTRY ListEntry;
	BTR_THREAD_TYPE Type;

	ULONG ThreadFlag;
	ULONG ThreadId;
	PULONG_PTR StackBase;
	HANDLE RundownHandle;
	PVOID Buffer;
	BOOLEAN BufferBusy;

	union {
		struct {
			struct _BTR_FILE_OBJECT *FileObject;
			struct _BTR_FILE_OBJECT *IndexObject;
		};
		struct {
			struct _BTR_MAPPING_OBJECT *DataMapping;
			struct _BTR_MAPPING_OBJECT *IndexMapping;
		};
	};

	ULONG FrameDepth;
	LIST_ENTRY FrameList;
	ULONG64 CallCount;

	//
	// stack lookup cache
	//

	ULONG StackCount;
	BTR_STACK_ENTRY StackCache[STACK_TLS_SIZE];

} BTR_THREAD_OBJECT, *PBTR_THREAD_OBJECT;
#pragma pack(pop)

typedef enum _BTR_THREAD_TYPE {
	ThreadRuntime,
	ThreadUser,
} BTR_THREAD_TYPE, *PBTR_THREAD_TYPE;

typedef struct _BTR_THREAD_STATE {
	ULONG ActiveCount;
	ULONG RundownCount;
	PULONG ActiveId;
	PULONG RundownId;
	SYSTEMTIME TimeStamp;
} BTR_THREAD_STATE, *PBTR_THREAD_STATE;

typedef struct _BTR_THREAD_PAGE {
	LIST_ENTRY ListEntry;
	PVOID StartVa;
	PVOID EndVa;
	BTR_BITMAP BitMap;
	BTR_THREAD_OBJECT Object[ANYSIZE_ARRAY];
} BTR_THREAD_PAGE, *PBTR_THREAD_PAGE;

//
// N.B. Forward declare to enable inline
//

extern LIST_ENTRY BtrThreadObjectList;
extern BTR_LOCK BtrThreadListLock;
extern LIST_ENTRY BtrThreadPageList;
extern BTR_LOCK BtrThreadPageLock;

extern PBTR_THREAD_OBJECT BtrNullThread;
extern DWORD BtrSystemThreadId[];
extern HANDLE BtrSystemThread[];
extern PTHREAD_START_ROUTINE BtrSystemProcedure[];

DECLSPEC_CACHEALIGN
extern ULONG BtrDllReferences;

PBTR_THREAD_OBJECT
BtrAllocateThread(
	__in BTR_THREAD_TYPE Type
	);

VOID
BtrFreeThread(
	__in PBTR_THREAD_OBJECT Thread
	);

BOOLEAN
BtrIsThreadTerminated(
	__in PBTR_THREAD_OBJECT Thread
	);

PBTR_THREAD_OBJECT
BtrGetCurrentThread(
	VOID
	);

BOOLEAN
BtrIsExecutingAddress(
	__in PLIST_ENTRY ContextList,
	__in PLIST_ENTRY AddressList
	);

BOOLEAN
BtrIsExecutingRuntime(
	__in PLIST_ENTRY ContextList 
	);

BOOLEAN
BtrIsExecutingTrap(
	__in PLIST_ENTRY ContextList
	);

ULONG
BtrInitializeThread(
	__in PBTR_PROFILE_OBJECT Object	
	);

VOID
BtrUninitializeThread(
	VOID
	);

BOOLEAN
BtrIsThreadTerminated(
	__in PBTR_THREAD_OBJECT Thread 
	);

BOOL
BtrOnThreadAttach(
	VOID
	);

BOOL
BtrOnThreadDetach(
	VOID
	);

BOOL
BtrOnProcessAttach(
	__in HMODULE DllHandle	
	);

BOOL
BtrOnProcessDetach(
	VOID
	);

VOID
BtrSetTlsValue(
	__in PBTR_THREAD_OBJECT Thread
	);

PBTR_THREAD_OBJECT
BtrGetTlsValue(
	VOID
	);

VOID
BtrClearThreadStack(
	__in PBTR_THREAD_OBJECT Thread
	);

PBTR_THREAD_OBJECT
BtrAllocateThreadEx(
	VOID
	);

VOID
BtrFreeThread(
	__in PBTR_THREAD_OBJECT Thread
	);

VOID
BtrInitializeThreadPage(
	__in PBTR_THREAD_PAGE Page 
	);

VOID
BtrFreeThreadPageList(
	VOID
	);

ULONG
BtrScanThreadList(
	__out PBTR_THREAD_STATE State
	);

PVOID 
BtrGetFramePointer(
	VOID
	);

VOID
BtrTrapProcedure(
	VOID
	);

BOOLEAN
BtrIsRuntimeThread(
	__in ULONG ThreadId
	);

FORCEINLINE
VOID
BtrReferenceDll(
	VOID
	)
{
	_InterlockedIncrement(&BtrDllReferences);
}

FORCEINLINE
VOID
BtrDereferenceDll(
	VOID
	)
{
	_InterlockedDecrement(&BtrDllReferences);
}

#ifdef __cplusplus
}
#endif
#endif