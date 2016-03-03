//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _APS_BTR_H_
#define _APS_BTR_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma once
#define WIN32_LEAN_AND_MEAN 

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifdef _IPCORE_
#include <WinSock2.h>
#include <MSWSock.h>
#endif

#include <windows.h> 
#include <tchar.h>

//
// N.B. disable all prefast warnings from strsafe.h
//

#include <codeanalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <strsafe.h>
#pragma warning(pop)

#pragma warning(disable : 4996)
#pragma warning(disable : 4995)

#include <crtdbg.h>
#include <dbghelp.h>
#include <psapi.h>
#include <intrin.h>
#include <emmintrin.h>
#include "status.h"
#include "list.h"
#include "bitmap.h"

#define FlagOn(F,SF)  ((F) & (SF))     
#define SetFlag(F,SF)   { (F) |= (SF);  }
#define ClearFlag(F,SF) { (F) &= ~(SF); }

#ifndef ASSERT
#define ASSERT _ASSERT 
#endif

typedef enum _BTR_RECORD_TYPE {
	RECORD_BUILDIN,
	RECORD_CALLBACK,
	RECORD_PROFILE,
} BTR_RECORD_TYPE, *PBTR_RECORD_TYPE;

typedef enum _BTR_OBJECT_FLAG {
	BTR_FLAG_EXCEPTION     = 0x00000001,
	BTR_FLAG_FPU_RETURN    = 0x00000002,
	BTR_FLAG_ON            = 0x00000010,
	BTR_FLAG_GC            = 0x00000020,
	BTR_FLAG_DECODED       = 0x00000100,
	BTR_FLAG_FUSELITE      = 0x00001000,
	BTR_FLAG_EXEMPTION     = 0x00010000,
} BTR_OBJECT_FLAG, *PBTR_OBJECT_FLAG;

typedef enum {
	NullThread,
	SystemThread,
	NormalThread,
} BTR_THREAD_TYPE, *PBTR_THREAD_TYPE;

typedef struct _BTR_RECORD_HEADER {
	ULONG TotalLength;
	ULONG Sequence;
	ULONG Type    : 4;
	ULONG StackId : 28;
	ULONG ThreadId;
	ULONG ObjectId;
	ULONG Duration;
	FILETIME Timestamp;
	PVOID CallerPc;
} BTR_RECORD_HEADER, *PBTR_RECORD_HEADER;

typedef enum _BTR_CALLBACK_FLAG {
	CALLBACK_OFF,
	CALLBACK_ON,
} BTR_CALLBACK_FLAG;

typedef enum _BTR_CALLBACK_TYPE {

	BuildinCallbackType = 0x00000001,
	
	CpuCallbackType    = 0x00000010,
	MmCallbackType     = 0x00000020,
	IoCallbackType     = 0x00000040,
	CcrCallbackType    = 0x00000080,

	HeapCallbackType   = 0x00000100,
	PageCallbackType   = 0x00000200, 
	HandleCallbackType = 0x00000400,
	GdiCallbackType    = 0x00000800,

	NetCallbackType    = 0x00010000,
	FileCallbackType   = 0x00020000,
	DeviceCallbackType = 0x00040000,

} BTR_CALLBACK_TYPE;

typedef enum _BTR_HOTPATCH_ACTION {
	HOTPATCH_COMMIT,
	HOTPATCH_DECOMMIT,
} BTR_HOTPATCH_ACTION, *PBTR_HOTPATCH_ACTION;

typedef struct _BTR_HOTPATCH_ENTRY {
	PVOID Address;
	ULONG Length;
	UCHAR Code[16];
} BTR_HOTPATCH_ENTRY, *PBTR_HOTPATCH_ENTRY;

typedef struct _BTR_CALLBACK {
	ULONG Type;
	BTR_CALLBACK_FLAG Flag;
	ULONG References;
	ULONG Spare0;
	PVOID Address;
	PVOID Callback;
	PSTR DllName;
	PSTR ApiName;
	struct _BTR_TRAP_OBJECT *Trap;
	BTR_HOTPATCH_ENTRY Hotpatch;
	PVOID PatchAddress;
} BTR_CALLBACK, *PBTR_CALLBACK;

typedef struct _BTR_CALLBACK_RECORD {
	BTR_RECORD_HEADER Base;
	ULONG CallbackType;
	ULONG EventType;
	CHAR Data[ANYSIZE_ARRAY];
} BTR_CALLBACK_RECORD, *PBTR_CALLBACK_RECORD;

typedef struct _BTR_SPINLOCK {
	DECLSPEC_ALIGN(16) volatile ULONG Acquired;
	ULONG ThreadId;
	ULONG SpinCount;
	USHORT Flag;
	USHORT Spare0;
} BTR_SPINLOCK, *PBTR_SPINLOCK;

typedef CRITICAL_SECTION BTR_LOCK, *PBTR_LOCK;
typedef SRWLOCK BTR_SRWLOCK, *PBTR_SRWLOCK;

typedef struct _BTR_ADDRESS_RANGE {
	LIST_ENTRY ListEntry;
	ULONG_PTR Address;
	ULONG_PTR Size;
} BTR_ADDRESS_RANGE, *PBTR_ADDRESS_RANGE;

typedef struct _BTR_CONTEXT {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Registers;
} BTR_CONTEXT, *PBTR_CONTEXT;

//
// Bucket number of stack trace database
//

#define MAX_STACK_DEPTH  60 

#pragma pack(push, 1)

typedef struct _BTR_FILE_INDEX {
	ULONG64 Committed : 1;
	ULONG64 Excluded  : 1;
	ULONG64 Length    : 16;
	ULONG64 Reserved  : 10;
	ULONG64 Offset    : 36;
} BTR_FILE_INDEX, *PBTR_FILE_INDEX;

#pragma pack(pop)

C_ASSERT(sizeof(BTR_FILE_INDEX) == 8);

//
// Maximum data file size is 64 GB
// Maximum index file size is 8 GB
//

#define BTR_MAXIMUM_DATA_FILE_SIZE   ((ULONG64)0x1000000000)
#define BTR_MAXIMUM_INDEX_FILE_SIZE  ((ULONG64)0x0800000000)
#define BTR_MAXIMUM_INDEX_NUMBER     ((ULONG)0xffffffff)

//
// Data file is per thread based.
//

#define BTR_FILE_INCREMENT     (1024 * 1024 * 4)
#define BTR_MAP_INCREMENT      (1024 * 64)

typedef enum _BTR_STATE {
	STATE_INVALID,
	STATE_LOADED,
	STATE_STARTING,
	STATE_RUNNING,
	STATE_STOP_STAGE0,
	STATE_STOP_STAGE1,
	STATE_UNLOAD,
} BTR_STATE, *PBTR_STATE;

#define ENTRY_COUNT_PER_STACK_PAGE (1024 * 64 * 2)

#define STACK_RECORD_SIZE          sizeof(BTR_STACK_RECORD)
#define STACK_FILE_INCREMENT       (1024 * 1024 * 4)
#define STACK_INVALID_ID           (ULONG)(-1)

//
// Caller's return address
//

#define CALLER ((ULONG_PTR)_ReturnAddress())

//
// N.B. All the following structure can be potential serialized
// to file, we need explicitly pack it as 1 byte aligned.
//

#pragma pack(push, 1) 

typedef struct _BTR_SPIN_HASH {
	BTR_SPINLOCK SpinLock;
	LIST_ENTRY ListHead;
	union {
		ULONG64 MissCount;
	} u;
} BTR_SPIN_HASH, *PBTR_SPIN_HASH;

//
// Stack Record
//

#define STACK_RECORD_BUCKET 23831
#define STACK_RECORD_BUCKET_PER_THREAD 61 

//
// Hash a stack hash to its bucket in stack table
//

#define STACK_ENTRY_BUCKET(_H) \
 (_H % STACK_RECORD_BUCKET)

#define GET_STACK_RECORD_BUCKET(_H) \
 (_H % STACK_RECORD_BUCKET_PER_THREAD)


typedef struct _BTR_STACK_ENTRY {

	LIST_ENTRY ListEntry;

	ULONG Hash;
	ULONG Depth;
	ULONG Count;
	ULONG StackId;

	union {
		ULONG64 SizeOfAllocs;
		ULONG64 Alignment;
	};

	PVOID Context;
	PVOID Frame[2];

} BTR_STACK_ENTRY, *PBTR_STACK_ENTRY;


typedef struct _BTR_STACK_PAGE {
	LIST_ENTRY ListEntry;
	ULONG Count;
	ULONG Next;
	BTR_STACK_ENTRY Buckets[ANYSIZE_ARRAY];
} BTR_STACK_PAGE, *PBTR_STACK_PAGE;

typedef struct _BTR_STACK_RECORD {

	union {
		SLIST_ENTRY ListEntry;
		LIST_ENTRY ListEntry2;
	};

	ULONG Committed : 1;
	ULONG Heap      : 1;
	ULONG Depth     : 6;
	ULONG Count     : 24;
	ULONG StackId;
	ULONG Hash;

	union {
		ULONG64 SizeOfAllocs;
		ULONG64 Alignment;
	};

	PVOID Frame[MAX_STACK_DEPTH];	

} BTR_STACK_RECORD, *PBTR_STACK_RECORD;

typedef struct _BTR_STACK_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Hash[STACK_RECORD_BUCKET];
} BTR_STACK_TABLE, *PBTR_STACK_TABLE;

//
// Count of per thread BTR_STACK_PAGE, minus 1 to fix offset of Buckets[ANYSIZE_ARRAY]
//

#define RECORD_COUNT_PER_STACK_RECORD_PAGE_PER_THREAD (4096 / sizeof(BTR_STACK_RECORD) - 1)

typedef struct _BTR_STACK_RECORD_PAGE {
	LIST_ENTRY ListEntry;
	ULONG Count;
	ULONG Next;
	BTR_STACK_RECORD Buckets[ANYSIZE_ARRAY];
} BTR_STACK_RECORD_PAGE, *PBTR_STACK_RECORD_PAGE;

typedef struct _BTR_STACK_TABLE_PER_THREAD {
	ULONG Count;
	LIST_ENTRY ListHead[STACK_RECORD_BUCKET_PER_THREAD];
} BTR_STACK_TABLE_PER_THREAD, *PBTR_STACK_TABLE_PER_THREAD;

//
// PC 
//

#define STACK_PC_BUCKET    4093

typedef struct _BTR_PC_ENTRY {

	LIST_ENTRY ListEntry;

	PVOID Address;
	ULONG64 SizeOfAllocs;

	ULONG Count;
	ULONG Exclusive;
	ULONG Inclusive;

	USHORT UseAddress;
	ULONG DllId;

	ULONG FunctionId;
	ULONG LineId;

} BTR_PC_ENTRY, *PBTR_PC_ENTRY;

typedef struct _BTR_PC_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Hash[STACK_PC_BUCKET];
} BTR_PC_TABLE, *PBTR_PC_TABLE;

//
// PC Per Thread 
// BTR_THREAD_PC_ENTRY has only thread specific counters,
// Lookup BTR_PC_ENTRY by address for other counters
//

typedef struct _BTR_THREAD_PC_ENTRY{

    LIST_ENTRY ListEntry;

	PVOID Address;
    ULONG ThreadId;

	ULONG Exclusive;
	ULONG Inclusive;

} BTR_THREAD_PC_ENTRY, *PBTR_THREAD_PC_ENTRY;

//
// Function
//

#define STACK_FUNCTION_BUCKET 4093

typedef struct _BTR_FUNCTION_ENTRY {

	LIST_ENTRY ListEntry;

	ULONG64 Address;
	ULONG FunctionId;
	ULONG DllId;
	ULONG Inclusive;

	union {
		ULONG Exclusive;
		ULONG Count;
	};

} BTR_FUNCTION_ENTRY, *PBTR_FUNCTION_ENTRY;

typedef struct _BTR_FUNCTION_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Hash[STACK_FUNCTION_BUCKET];
} BTR_FUNCTION_TABLE, *PBTR_FUNCTION_TABLE;

//
// Dll 
//

#define STACK_DLL_BUCKET 283 
 
typedef struct _CV_INFO_PDB70 {
	ULONG CvSignature;
	GUID Signature;
	ULONG Age;
	CHAR PdbName[MAX_PATH];
} CV_INFO_PDB70, *PCV_INFO_PDB70;

//
// N.B. BTR_CV_RECORD is reversed from dbgeng.dll, this is the
// key data structure to load a matching pdb file without using
// an image file, debugger rely on codeview record to uniquely
// identiy a matching pdb file. PdbName is variable length, however,
// we simply make it 64 bytes fixed length, a full qualified pdb 
// path name should be splitted into short file name.
// 

typedef struct _BTR_CV_RECORD {
	ULONG_PTR CvRecordOffset;
	ULONG_PTR SizeOfCvRecord;
	ULONG_PTR Reserved0;
	ULONG_PTR Reserved1;
	ULONG Timestamp;
	ULONG SizeOfImage;
	ULONG CvSignature; // 'RSDS'
	GUID  Signature;
	ULONG Age;
	CHAR  PdbName[64];
} BTR_CV_RECORD, *PBTR_CV_RECORD;

typedef struct _BTR_DLL_ENTRY {

	union {
		LIST_ENTRY ListEntry;
		LIST_ENTRY ListHead;
	};

	ULONG_PTR BaseVa;
	SIZE_T Size;

    VS_FIXEDFILEINFO VersionInfo;

	ULONG CheckSum;
	ULONG Timestamp;
	ULONG DllId;

	union {
		ULONG Inclusive;
		ULONG CountOfAllocs;
	};

	union {
		ULONG Exclusive;
		ULONG Count;
	};

	ULONG64 SizeOfAllocs;
	BTR_CV_RECORD CvRecord;

	WCHAR Path[MAX_PATH];

} BTR_DLL_ENTRY, *PBTR_DLL_ENTRY;

#define STACK_DLL_HASH(_V) \
	(ULONG)((ULONG_PTR)_V % STACK_DLL_BUCKET)

typedef struct _BTR_DLL_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Hash[STACK_DLL_BUCKET];
} BTR_DLL_TABLE, *PBTR_DLL_TABLE;

typedef struct _BTR_DLL_FILE {
	ULONG Count;
	BTR_DLL_ENTRY Dll[ANYSIZE_ARRAY];
} BTR_DLL_FILE, *PBTR_DLL_FILE;

//
// Symbol Text
//

#define STACK_TEXT_BUCKET 4093
#define STACK_TEXT_LIMIT  64 

typedef struct _BTR_TEXT_ENTRY {
	LIST_ENTRY ListEntry;
	ULONG64 Address;
	ULONG FunctionId;
	ULONG LineId;
	CHAR Text[STACK_TEXT_LIMIT];
} BTR_TEXT_ENTRY, *PBTR_TEXT_ENTRY;

typedef struct _BTR_TEXT_TABLE {
	HANDLE Process;
	HANDLE Object;
	ULONG Buckets;
	ULONG Count;
	BTR_SPIN_HASH Hash[ANYSIZE_ARRAY];
} BTR_TEXT_TABLE, *PBTR_TEXT_TABLE;

typedef struct _BTR_TEXT_FILE {
	ULONG Count;
	BTR_TEXT_ENTRY Text[ANYSIZE_ARRAY];
} BTR_TEXT_FILE, *PBTR_TEXT_FILE;

//
// Source Line
//

#define STACK_LINE_BUCKET 4093

typedef struct _BTR_LINE_ENTRY {  
	LIST_ENTRY ListEntry;
	ULONG64 Address; 
	ULONG Line;  
	ULONG Id;
	CHAR File[MAX_PATH];
} BTR_LINE_ENTRY, *PBTR_LINE_ENTRY; 

typedef struct _BTR_LINE_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Hash[STACK_LINE_BUCKET];
} BTR_LINE_TABLE, *PBTR_LINE_TABLE;

//
// Leak Entry, for D Profile memory profiling
//

#define STACK_LEAK_BUCKET 4093

typedef struct _BTR_LEAK_CONTEXT {
	ULONG Type;
	PVOID Context;
} BTR_LEAK_CONTEXT, *PBTR_LEAK_CONTEXT;

typedef struct _BTR_LEAK_ENTRY {

	union {
		LIST_ENTRY ListEntry;
		BTR_LEAK_CONTEXT Auxiliary;
	};

	ULONG Allocator;
	ULONG StackId;
	ULONG Count;
	ULONG64 Size;
} BTR_LEAK_ENTRY, *PBTR_LEAK_ENTRY;

typedef struct _BTR_LEAK_TABLE {
	ULONG Count;
	BTR_SPIN_HASH Hash[STACK_LEAK_BUCKET];
} BTR_LEAK_TABLE, *PBTR_LEAK_TABLE;

typedef struct _BTR_LEAK_FILE {

	union {
		LIST_ENTRY ListEntry;
		BTR_LEAK_CONTEXT Auxiliary;
	};

	BTR_CALLBACK_TYPE Type;
	ULONG Count;
	ULONG LeakCount;
    ULONG64 Bytes;
	PVOID Context;
	BTR_LEAK_ENTRY Leak[ANYSIZE_ARRAY];
} BTR_LEAK_FILE, *PBTR_LEAK_FILE;

//
// IO 
//

typedef enum _IO_OPERATION {
	IO_OP_INVALID = 0,
	IO_OP_CREATE,
	IO_OP_CONNECT,
	IO_OP_ACCEPT,
	IO_OP_IOCONTROL,
	IO_OP_FSCONTROL,
	IO_OP_READ, 
	IO_OP_RECV = IO_OP_READ, 
	IO_OP_WRITE,
	IO_OP_SEND = IO_OP_WRITE,
	IO_OP_CLOSE,
	IO_OP_NUMBER,
} IO_OPERATION;

typedef enum _IO_COMPLETION_MODE {
	IO_MODE_REQUESTOR,
	IO_MODE_COMPLETOR,
} IO_COMPLETION_MODE;

//
// IO_IRP describe an I/O request
//

#define IO_IRP_TAG  'prI'

#pragma pack(push, 1)

typedef struct _IO_FLAGS {
	BOOLEAN File		: 1;
	BOOLEAN Socket		: 1;
	BOOLEAN Synchronous : 1;
	BOOLEAN UserApc     : 1;
	BOOLEAN Completed   : 1;
	BOOLEAN Orphaned    : 1;
} IO_FLAGS, *PIO_FLAG;

typedef enum _IO_OBJECT_FLAG {
	OF_INVALID		= 0,
	OF_FILE			= 1 << 2,
	OF_SOCKET       = 1 << 3,
	OF_SKIPV4		= 1 << 4,
	OF_SKIPV6		= 1 << 5,
	OF_SKTCP		= 1 << 6,
	OF_SKUDP		= 1 << 7,
	OF_OVERLAPPED	=  1 << 8,
	OF_SKIPONSUCCESS = 1 << 9,
	OF_SKIPSETEVENT  = 1 << 10,
	OF_IOCPASSOCIATE = 1 << 11,
	OF_LOCAL_VALID = 1 << 12,
	OF_REMOTE_VALID = 1 << 13,
} IO_OBJECT_FLAG;

#pragma pack(pop)

//
// CCR definition shared by runtime and analyzer 
//

typedef enum _CCR_PROBE_TYPE {
	CCR_PROBE_INVALID = -1,
	CCR_PROBE_TRY_ENTER_CS,
	CCR_PROBE_ENTER_CS,
	CCR_PROBE_LEAVE_CS,
	CCR_PROBE_TRY_ACQUIRE_SRW_SHARED,
	CCR_PROBE_ACQUIRE_SRW_SHARED,
	CCR_PROBE_RELEASE_SRW_SHARED,
	CCR_PROBE_TRY_ACQUIRE_SRW_EXCLUSIVE,
	CCR_PROBE_ACQUIRE_SRW_EXCLUSIVE,
	CCR_PROBE_RELEASE_SRW_EXCLUSIVE,
	CCR_PROBE_SLEEP_ON_CONDITION_SRW,
	CCR_PROBE_WAIT_FOR_KEYED_EVENT,
	CCR_PROBE_WAIT_FOR_SINGLE_OBJECT,
	CCR_PROBE_HEAP_ALLOC,
	CCR_PROBE_HEAP_FREE,
	CCR_PROBE_COUNT,
} CCR_PROBE_TYPE;

typedef enum _CCR_LOCK_TYPE {
	CCR_LOCK_INVALID,
	CCR_LOCK_CS,
	CCR_LOCK_SRW,
} CCR_LOCK_TYPE;

//
// Stack trace counter track each lock's acquisition
//

typedef struct _CCR_STACKTRACE {
	struct _CCR_STACKTRACE *Next;
	union {
		ULONG StackId;
		PBTR_STACK_RECORD Record;
	} u;
	ULONG Count;
} CCR_STACKTRACE, *PCCR_STACKTRACE;

typedef struct _CCR_STACKTRACE_PAGE {
	LIST_ENTRY ListEntry;
	ULONG Count;
	ULONG Next;
	CCR_STACKTRACE Entries[ANYSIZE_ARRAY];
} CCR_STACKTRACE_PAGE, *PCCR_STACKTRACE_PAGE;

#define CCR_COUNT_OF_STACKTRACE_PER_PAGE  (4096 / sizeof(CCR_STACKTRACE) - 1)

typedef struct DECLSPEC_ALIGN(16) _CCR_LOCK_TRACK {

	union {
		SLIST_ENTRY SListEntry;
		LIST_ENTRY ListEntry;
	};

	PCCR_STACKTRACE StackTrace;
	ULONG StackTraceCount;

	CCR_LOCK_TYPE Type;
	union {
		PVOID LockPtr;
		PCRITICAL_SECTION CsPtr;
		PSRWLOCK SrwPtr;
	};

	//
	// N.B. This field is used when this structure is inserted into
	// global lock table, to chain all threads ever acquired this lock.
	//

	struct _CCR_LOCK_TRACK *SiblingAcquirers;

	ULONG TryAcquire;
	ULONG Acquire;
	ULONG TryAcquireShared;
	ULONG TryAcquireExclusive;
	ULONG AcquireShared;
	ULONG AcquireExclusive;
	ULONG TryFailure;
	ULONG Release;
	ULONG KernelWait;
	ULONG HeapAllocAcquire;
	ULONG HeapFreeAcquire;

	ULONG RecursiveCount;
	ULONG LockOwner;

	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	LARGE_INTEGER MaximumAcquireLatency;  
	LARGE_INTEGER MaximumHoldingDuration; 

	ULONG TrackThreadId;
	ULONG CreatorThreadId;
	ULONG CreatorStackId;

} CCR_LOCK_TRACK, *PCCR_LOCK_TRACK;

#define CCR_LOCK_CACHE_BUCKET 17

typedef struct _CCR_LOCK_CACHE {
	ULONG LockTrackCount;
	ULONG StackTraceCount;
	PCCR_LOCK_TRACK Current;
	BOOLEAN StackInHeapAlloc	: 1;
	BOOLEAN StackInHeapFree		: 1;
	BOOLEAN StackInAcquire		: 1;
	BOOLEAN StackInKernelWait	: 1;
	LIST_ENTRY LockList[CCR_LOCK_CACHE_BUCKET];
} CCR_LOCK_CACHE, *PCCR_LOCK_CACHE;

typedef struct _CCR_CALLER_RANGE {
	LIST_ENTRY ListEntry;
	ULONG_PTR Base;
	ULONG_PTR Size;
} CCR_CALLER_RANGE, *PCCR_CALLER_RANGE;

//
// Global lock table used after stopping profile,
// to merge all per thread lock cache.
//

#define CCR_LOCK_TALBE_BUCKET 1023
#define GET_CCR_LOCK_TABLE_BUCKET(_P) \
	((ULONG_PTR)_P % CCR_LOCK_TALBE_BUCKET)

typedef struct _CCR_LOCK_TABLE {
	ULONG Count;
	LIST_ENTRY LockList[CCR_LOCK_TALBE_BUCKET];
} CCR_LOCK_TABLE, *PCCR_LOCK_TABLE;

typedef struct _CCR_LOCK_INDEX {
	ULONG ThreadId;
	ULONG Offset;
	ULONG Size;
} CCR_LOCK_INDEX, *PCCR_LOCK_INDEX;


#pragma pack(pop)

//
// Profile callback routines
//

typedef ULONG
(*BTR_START_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_STOP_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_PAUSE_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_RESUME_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_UNLOAD_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_EXIT_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_QUERY_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef ULONG
(*BTR_QUERY_HOTPATCH_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object,
	__in BTR_HOTPATCH_ACTION Action,
	__out PBTR_HOTPATCH_ENTRY *Entry,
	__out PULONG Count
	);

typedef ULONG 
(*BTR_THREAD_ATTACH_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);
	
typedef ULONG 
(*BTR_THREAD_DETACH_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object
	);

typedef BOOLEAN
(*BTR_ISRUNTIMETHREAD_ROUTINE)(
	__in struct _BTR_PROFILE_OBJECT *Object,
	__in ULONG ThreadId
	);
	
//
// Profile Structures
//

typedef enum _BTR_PROFILE_TYPE {
	PROFILE_NONE_TYPE,
	PROFILE_CPU_TYPE,
	PROFILE_MM_TYPE,
	PROFILE_IO_TYPE,
	PROFILE_CCR_TYPE,
} BTR_PROFILE_TYPE;

typedef enum _BTR_PROFILE_MODE {
	DIGEST_MODE  = 0x00000000,
	RECORD_MODE  = 0x00000001,
} BTR_PROFILE_MODE;

typedef enum _BTR_PROFILE_OPTION {
	ENABLE_HEAP   = 0x00000001,
	ENABLE_PAGE   = 0x00000002,
	ENABLE_HANDLE = 0x00000004,
	ENABLE_GDI    = 0x00000008,
	ENABLE_NET    = 0x00000100,
	ENABLE_FILE   = 0x00000200,
} BTR_PROFILE_OPTION;

//
// Minimum CPU sample period is 10 Milliseconds
//

#define MINIMUM_SAMPLE_PERIOD 10 

typedef struct _BTR_PROFILE_ATTRIBUTE {

	BTR_PROFILE_TYPE Type;
	BTR_PROFILE_MODE Mode;

	//
	// CPU Profile
	//

	struct {
		
		LARGE_INTEGER TotalTime;
		LARGE_INTEGER KernelTime;
		LARGE_INTEGER UserTime;

		ULONG SamplingPeriod;
		ULONG StackDepth;
		BOOLEAN IncludeWait;
		BOOLEAN Paused;
		
		ULONG SamplesDepth;
		ULONG Inclusive;
		ULONG Exclusive;
	};

	//
	// MM Profile 
	//

	struct {

		ULONG EnableHeap    : 1;
		ULONG EnablePage    : 1;
		ULONG EnableHandle  : 1;
		ULONG EnableGdi     : 1;
		ULONG Spare0        : 28;

		ULONG PrivateBytes;
		ULONG VirtualBytes;
		ULONG KernelHandles;
		ULONG GdiHandles;
		ULONG NumberOfHeapAllocs;
		ULONG NumberOfHeapFrees;
		ULONG64 SizeOfHeapAllocs;
		ULONG64 SizeOfHeapFrees;
		ULONG NumberOfPageAllocs;
		ULONG NumberOfPageFrees;
		ULONG64 SizeOfPageAllocs;
		ULONG64 SizeOfPageFrees;
		ULONG NumberOfHandleAllocs;
		ULONG NumberOfHandleFrees;
		ULONG NumberOfGdiAllocs;
		ULONG NumberOfGdiFrees;

	};

	//
	// I/O Profile 
	//

	struct {

		ULONG EnableNet     : 1;
		ULONG EnableFile    : 1;
		ULONG EnableDevice  : 1;
		ULONG Spare1        : 30;

		ULONG NumberOfIoRead;
		ULONG NumberOfIoWrite;
		ULONG NumberOfIoCtl;
		ULONG64 SizeOfIoRead;
		ULONG64 SizeOfIoWrite;
		ULONG64 SizeOfIoCtl;

	};

	//
	// CCR Profile
	//

	struct {
		BOOLEAN TrackSystemLock;
	};

} BTR_PROFILE_ATTRIBUTE, *PBTR_PROFILE_ATTRIBUTE;

//
// N.B. BTR_SHARED_DATA must be cached aligned, all its fields
// are shared between profiler and target process, and accessed
// in atomic way or protected by spinlock
//

typedef struct _BTR_SHARED_DATA {

	DECLSPEC_CACHEALIGN
	BTR_SPINLOCK SpinLock;

	DECLSPEC_CACHEALIGN
	ULONG Sequence;

	DECLSPEC_CACHEALIGN
	ULONG SequenceFull;

	DECLSPEC_CACHEALIGN
	ULONG PendingReset;
	
	DECLSPEC_CACHEALIGN
	ULONG StackId;

	DECLSPEC_CACHEALIGN
	ULONG ObjectId;

	DECLSPEC_CACHEALIGN
	ULONG64 IndexFileLength;

	DECLSPEC_CACHEALIGN
	ULONG64 IndexValidLength;

	DECLSPEC_CACHEALIGN
	ULONG64 DataFileLength;
	
	DECLSPEC_CACHEALIGN
	ULONG64 DataValidLength;
	
	DECLSPEC_CACHEALIGN
	ULONG64 StackFileLength;

	DECLSPEC_CACHEALIGN
	ULONG64 StackValidLength;

	DECLSPEC_CACHEALIGN
	ULONG64 StreamFileLength;
	
	DECLSPEC_CACHEALIGN
	ULONG64 StreamValidLength;

	//
	// Profile object pointer, profiler read its 
	// performance counters via ReadProcessMemory
	//

	PVOID ProfileObject;

	FILETIME StartTime;
	FILETIME EndTime;

} BTR_SHARED_DATA, *PBTR_SHARED_DATA;

//
// Profile Message Structures
//

#define BTR_BUFFER_LENGTH   (1024 * 64)

typedef enum _BTR_MESSAGE_TYPE {
	MESSAGE_START,
	MESSAGE_START_ACK,
	MESSAGE_START_STATUS,
	MESSAGE_STOP,
	MESSAGE_STOP_ACK,
	MESSAGE_STOP_STATUS,
	MESSAGE_PAUSE,
	MESSAGE_PAUSE_ACK,
	MESSAGE_RESUME,
	MESSAGE_RESUME_ACK,
	MESSAGE_QUERY,
	MESSAGE_QUERY_ACK,
    MESSAGE_MARK,
    MESSAGE_MARK_ACK,
	MESSAGE_LAST,
} BTR_MESSAGE_TYPE, *PBTR_MESSAGE_TYPE;

typedef struct _BTR_MESSAGE_HEADER {
	ULONG Length;
	BTR_MESSAGE_TYPE Type;
} BTR_MESSAGE_HEADER, *PBTR_MESSAGE_HEADER;

typedef struct _BTR_MESSAGE_START { 
	BTR_MESSAGE_HEADER Header;
	HANDLE SharedData;
	HANDLE IndexFile;
	HANDLE DataFile;
	HANDLE StackFile;
	HANDLE ReportFile;
	HANDLE UnloadEvent;
	HANDLE ExitEvent;
	HANDLE ExitAckEvent;
	HANDLE ControlEnd;
	HANDLE IoObjectFile;
	HANDLE IoNameFile;
	HANDLE IoIrpFile;
	BTR_PROFILE_ATTRIBUTE Attribute;
} BTR_MESSAGE_START, *PBTR_MESSAGE_START;

typedef struct _BTR_MESSAGE_START_ACK { 
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
	ULONG Count;
	BTR_HOTPATCH_ENTRY Hotpatch[ANYSIZE_ARRAY];
} BTR_MESSAGE_START_ACK, *PBTR_MESSAGE_START_ACK;

typedef struct _BTR_MESSAGE_START_STATUS {
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_START_STATUS, *PBTR_MESSAGE_START_STATUS;

typedef struct _BTR_MESSAGE_STOP { 
	BTR_MESSAGE_HEADER Header;
} BTR_MESSAGE_STOP, *PBTR_MESSAGE_STOP;

typedef struct _BTR_MESSAGE_STOP_ACK { 
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
	ULONG Count;
	BTR_HOTPATCH_ENTRY Hotpatch[ANYSIZE_ARRAY];
} BTR_MESSAGE_STOP_ACK, *PBTR_MESSAGE_STOP_ACK;

typedef struct _BTR_MESSAGE_STOP_STATUS {
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_STOP_STATUS, *PBTR_MESSAGE_STOP_STATUS;

typedef struct _BTR_MESSAGE_PAUSE { 
	BTR_MESSAGE_HEADER Header;
	ULONG Duration;
} BTR_MESSAGE_PAUSE, *PBTR_MESSAGE_PAUSE;

typedef struct _BTR_MESSAGE_PAUSE_ACK { 
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_PAUSE_ACK, *PBTR_MESSAGE_PAUSE_ACK;

typedef struct _BTR_MESSAGE_RESUME { 
	BTR_MESSAGE_HEADER Header;
} BTR_MESSAGE_RESUME, *PBTR_MESSAGE_RESUME;

typedef struct _BTR_MESSAGE_RESUME_ACK { 
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_RESUME_ACK, *PBTR_MESSAGE_RESUME_ACK;

typedef struct _BTR_MESSAGE_QUERY { 
	BTR_MESSAGE_HEADER Header;
} BTR_MESSAGE_QUERY, *PBTR_MESSAGE_QUERY;

typedef struct _BTR_MESSAGE_QUERY_ACK { 
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_QUERY_ACK, *PBTR_MESSAGE_QUERY_ACK;

//
// Time Marker 
//

typedef enum _BTR_MARK_REASON {
	MARK_NONE,
	MARK_USER = 1,
	MARK_START,
	MARK_PAUSE,
	MARK_RESUME,
	MARK_ABORT,
	MARK_STOP,
} BTR_MARK_REASON, *PBTR_MARK_REASON;

typedef struct _BTR_MARK {
	LIST_ENTRY ListEntry;
    FILETIME Timestamp;
	BTR_MARK_REASON Reason;
    ULONG Index;
} BTR_MARK, *PBTR_MARK;

typedef struct _BTR_MESSAGE_MARK {
    BTR_MESSAGE_HEADER Header;
} BTR_MESSAGE_MARK, *PBTR_MESSAGE_MARK;

typedef struct _BTR_MESSAGE_MARK_ACK {
    BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_MARK_ACK, *PBTR_MESSAGE_MARK_ACK;


#ifdef __cplusplus
} 
#endif 
#endif