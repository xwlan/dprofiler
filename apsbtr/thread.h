//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
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

//
// N.B. For IO profiling, If APC_IO_COMPLETED is set,
// the intercepted APC must not queue a completion packet 
// into request table, since its request has never been queued.
//

typedef enum _BTR_APC_FLAG {
	APC_STANDARD_APC        = 1,
	APC_FILE_COMPLETION     = 1 << 1,
	APC_SOCKET_COMPLETION   = 1 << 2,
	APC_IO_COMPLETED		= 1 << 3,
	APC_IO_FALSE_POSITIVE	= 1 << 4,  
} BTR_APC_FLAG;

typedef struct _BTR_USER_APC {
	LIST_ENTRY ListEntry;
	ULONG Flag;
	PVOID Context;
	union {
		PAPCFUNC ApcRoutine;
		LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
		LPWSAOVERLAPPED_COMPLETION_ROUTINE SkCompletionRoutine;
	};
} BTR_USER_APC, *PBTR_USER_APC;

#pragma pack(push, 16)
typedef struct _BTR_THREAD_OBJECT {

	union {
		SLIST_ENTRY SListEntry;
		LIST_ENTRY ListEntry;
	};

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

	BTR_SPINLOCK IrpLock;
	LIST_ENTRY IrpList;
	SLIST_HEADER IrpLookaside;
	LIST_ENTRY ApcList;

	//
	// Pointer to lock cache of CCR profiling
	//

	struct _CCR_LOCK_CACHE *CcrLockCache;

	//
	// Per thread stack database
	//

	PSLIST_HEADER StackRecordLookaside;
	PBTR_STACK_RECORD_PAGE StackPage;
	LIST_ENTRY StackPageRetireList;
	PBTR_STACK_TABLE_PER_THREAD StackTable;

	struct _CCR_STACKTRACE_PAGE *TracePage;
	LIST_ENTRY TracePageRetireList;

} BTR_THREAD_OBJECT, *PBTR_THREAD_OBJECT;
#pragma pack(pop)

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

typedef struct _BTR_SUSPENDEE {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	ULONG SuspendCount;
	HANDLE ThreadHandle;
	PTEB Teb;
	PBTR_THREAD_OBJECT Object;
	PCONTEXT Context;

	//
	// N.B. Currently we only support MAX_STACK_DEPTH frames, technically
	// we need walk the frame until RtlUserThreadStart is reached to safely
	// determine whether there's left runtime frame on stack, however it's
	// expected that 60 frames are enough since our interception callbacks
	// are low level routines which won't incur complex code path. we may
	// walk up to RtlUserThreadStart if we hit any issue.
	//

	ULONG Depth;
	PVOID Frame[MAX_STACK_DEPTH];

} BTR_SUSPENDEE, *PBTR_SUSPENDEE;

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

VOID
BtrClearThreadStackByTeb(
	IN PTEB Teb 
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

//
// Suspend all threads except message runtime thread
// (the thread call BtrSuspendProcess)
//

ULONG
BtrSuspendProcess(
	_Out_ PLIST_ENTRY SuspendList,
	_Out_ PULONG Count
	);

VOID
BtrResumeProcess(
	_Inout_ PLIST_ENTRY SuspendList,
	_In_ BOOLEAN Clean
	);

VOID
BtrWalkSuspendeeStack(
	_In_ PLIST_ENTRY SuspendeeList
	);

BOOLEAN
BtrScanFrameFromSuspendeeList(
	_In_ PLIST_ENTRY SuspendeeList
	);

#ifdef __cplusplus
}
#endif
#endif