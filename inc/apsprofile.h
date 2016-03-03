//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _APS_PROFILE_H_
#define _APS_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"


//
// Memory Profile 
//

typedef enum _HEAP_CALLBACK_TYPE {

	_RtlAllocateHeap,
	_RtlReAllocateHeap,
	_RtlFreeHeap,

	_Malloc,
	_Realloc,
	_Calloc,
	_Free,

	_GlobalAlloc,
	_GlobalReAlloc,
	_GlobalFree,

	_LocalAlloc,
	_LocalReAlloc,
	_LocalFree,

	_SysAllocString,
	_SysAllocStringLen,
	_SysAllocStringByteLen,
	_SysReAllocString,
	_SysReAllocStringLen,
	_SysFreeString,

	_HeapCreate,
	_HeapDestroy,
	_RtlCreateHeap,
	_RtlDestroyHeap,

} HEAP_CALLBACK_TYPE;

typedef enum _PAGE_CALLBACK_TYPE {
	_VirtualAlloc,
	_VirtualFree,
	_VirtualAllocEx,
	_VirtualFreeEx,
} PAGE_CALLBACK_TYPE;

typedef enum _HANDLE_CALLBACK_TYPE {
	_DuplicateHandle,
	_CloseHandle,
	_CreateProcessA,
	_CreateProcessW,
	_CreateProcessAsUserA,
	_CreateProcessAsUserW,
	_CreateProcessWithLogonW,
	_OpenProcess,
	_CreateThread,
	_CreateRemoteThread,
	_OpenThread,
	_CreateJobObjectW, 
	_OpenJobObjectW,
	_CreateFileW,
	_CreateFileMappingW,
	_OpenFileMappingW,
	_MapViewOfFile,
	_MapViewOfFileEx,
	_UnmapViewOfFile,
	_RegOpenCurrentUser,
	_RegConnectRegistry,
	_RegOpenUserClassesRoot,
	_RegCreateKeyA,
	_RegCreateKeyW,
	_RegCreateKeyExA,
	_RegCreateKeyExW,
	_RegOpenKeyExA,
	_RegOpenKeyExW,
	_RegCloseKey,
	_WSASocketW,
	_WSAAccept,
	_closesocket,   
	_CreateEventW,
	_CreateEventExW,
	_OpenEventW,
	_CreateMutexW,
	_CreateMutexExW,
	_OpenMutexW,
	_CreateSemaphoreW,
	_CreateSemaphoreExW,
	_OpenSemaphoreW,
	_CreateWaitableTimerExW,
	_OpenWaitableTimerW,
	_CreateIoCompletionPort,
	_CreatePipe,
	_CreateNamedPipeW,
	_CreateMailslotW,
	_CreateRestrictedToken,
	_DuplicateToken,
	_DuplicateTokenEx,
	_OpenProcessToken,
	_OpenThreadToken,
	_HandleLast,
} HANDLE_CALLBACK_TYPE;

typedef enum _GDI_CALLBACK_TYPE {
   _DeleteObject,
   _DeleteDC,
   _ReleaseDC,
   _DeleteEnhMetaFile,
   _CloseEnhMetaFile,
	_CreateCompatibleDC,
	_CreateDCA,
	_CreateDCW,
	_CreateICA,
	_CreateICW,
	_GetDC,
	_GetDCEx,
	_GetWindowDC,
	_CreatePen,
    _CreatePenIndirect,
    _ExtCreatePen,
	_CreateBrushIndirect,
	_CreateSolidBrush,
	_CreatePatternBrush,
	_CreateDIBPatternBrush,
	_CreateHatchBrush,
    _CreateBtrftonePalette,
    _CreatePalette,
	_CreateFontA,
	_CreateFontW,
	_CreateFontIndirectA,
	_CreateFontIndirectW,
	_CreateFontIndirectExA,
	_CreateFontIndirectExW,
	_LoadBitmapA,
	_LoadBitmapW,
	_CreateBitmap,
	_CreateBitmapIndirect,
	_CreateCompatibleBitmap,
	_LoadImageA,
	_LoadImageW,
	_PathToRegion,
    _CreateEllipticRgn,
    _CreateEllipticRgnIndirect,
    _CreatePolygonRgn,
    _CreatePolyPolygonRgn,
    _CreateRectRgn,
    _CreateRectRgnIndirect,
    _CreateRoundRectRgn,
    _ExtCreateRegion,
    _CreateEnhMetaFileA,
    _CreateEnhMetaFileW,
    _GetEnhMetaFileA,
    _GetEnhMetaFileW,
} GDI_CALLBACK_TYPE;

typedef enum _HANDLE_TYPE {
	HANDLE_GENERIC,
	HANDLE_PROCESS,
	HANDLE_THREAD,
	HANDLE_JOB,
	HANDLE_FILE,
	HANDLE_FILEMAPPING,
	HANDLE_KEY,
	HANDLE_SOCKET,
	HANDLE_EVENT,
	HANDLE_MUTEX,
	HANDLE_SEMAPHORE,
	HANDLE_TIMER,
	HANDLE_IOCP,
	HANDLE_PIPE,
	HANDLE_MAILSLOT,
	HANDLE_TOKEN,
	HANDLE_CONSOLE,
	HANDLE_TRANSACTION,
	HANDLE_MM_RESOURCE_NOTIFICATION,
	HANDLE_LAST,
} HANDLE_TYPE;

//
// N.B. GDI object type start from 1, it's OBJ_PEN
//

#define GDI_LAST GDI_OBJ_LAST

typedef struct _PF_TYPE_ENTRY {
	ULONG Allocs;
	ULONG Frees;
} PF_TYPE_ENTRY, *PPF_TYPE_ENTRY;

typedef struct _PF_HEAP_RECORD {

	LIST_ENTRY ListEntry;

	HANDLE HeapHandle;
	PVOID Address;
	PVOID OldAddr;
	ULONG Size;
	ULONG Duration;

	union {
		ULONG StackHash;
		ULONG StackId;
	};

	USHORT StackDepth;
	USHORT CallbackType;

} PF_HEAP_RECORD, *PPF_HEAP_RECORD;

typedef struct _PF_PAGE_RECORD {

	LIST_ENTRY ListEntry;

	PVOID Address;
	ULONG Size;
	ULONG Duration;

	union {
		ULONG StackHash;
		ULONG StackId;
	};

	USHORT StackDepth;
	USHORT CallbackType;

} PF_PAGE_RECORD, *PPF_PAGE_RECORD;

typedef struct _PF_HANDLE_RECORD {

	LIST_ENTRY ListEntry;

	HANDLE_TYPE Type;
	HANDLE Value;
	HANDLE Duplicate;
	ULONG Duration;

	union {
		ULONG StackHash;
		ULONG StackId;
	};

	USHORT StackDepth;
	USHORT CallbackType;

} PF_HANDLE_RECORD, *PPF_HANDLE_RECORD;

typedef struct _PF_GDI_RECORD {

	LIST_ENTRY ListEntry;

	ULONG Type;
	HANDLE Value;
	ULONG Duration;

	union {
		ULONG StackHash;
		ULONG StackId;
	};

	USHORT StackDepth;
	USHORT CallbackType;

} PF_GDI_RECORD, *PPF_GDI_RECORD;

//
// I/O Profile
//

typedef enum _IO_CALLBACK_TYPE {
	_IoExitProcess,
	_IoCreateFile,
	_IoDuplicateHandle,
	_IoCloseHandle,
	_IoReadFile,
	_IoWriteFile,
	_IoReadFileEx,
	_IoWriteFileEx,
	_IoGetOverlappedResult,
	_IoGetQueuedCompletionStatus,
	_IoGetQueuedCompletionStatusEx,
	_IoPostQueuedCompletionStatus,
	_IoSocket,
	//_IoBind,
	_IoConnect,
	_IoAccept,
	_IoCtlSocket,
	_IoClosesocket,
	_IoWSASocket,
	_IoWSAConnect,
	_IoWSAAccept,
	_IoWSAIoctl,
	_IoWSAEventSelect,
	_IoWSAAsyncSelect,
	_IoWSAGetOverlappedResult,
	_IoRecv,
	_IoSend,
	_IoRecvfrom,
	_IoSendto,
	_IoWSARecv,
	_IoWSASend,
	_IoWSARecvFrom,
	_IoWSASendTo,
	_IoCreateThreadPoolIo,
	_IoCreateIoCompletionPort,
	_IoSetFileCompletionNotificationModes,

	//
	// N.B. The following routines are dynamically queried from
	// winsock, we need handle them specially when patching them
	//

	_IoAcceptEx,
	_IoConnectEx,
	_IoTransmitFile,
	_IoTransmitPackets,
	_IoWSARecvMsg,
	_IoWSASendMsg,
} IO_CALLBACK_TYPE;

typedef enum _CCR_CALLBACK_TYPE {
	_CcrExitProcess,
	_CcrEnterCriticalSection,
	_CcrTryEnterCriticalSection,
	_CcrLeaveCriticalSection, 
	_CcrAcquireSRWLockExclusive,
	_CcrAcquireSRWLockShared,
	_CcrTryAcquireSRWLockExclusive,
	_CcrTryAcquireSRWLockShared,
	_CcrReleaseSRWLockExclusive,
	_CcrReleaseSRWLockShared,
	_CcrSleepConditionVariableSRW,
	_CcrNtWaitForSingleObject,
	_CcrNtWaitForKeyedEvent,
	_CcrRtlAllocateHeap, 
	_CcrRtlFreeHeap, 
} CCR_CALLBACK_TYPE;

//
// Profile Record
//

typedef struct _BTR_PROFILE_RECORD {
	BTR_RECORD_HEADER Base;
	BTR_PROFILE_TYPE ProfileType;
	BTR_CALLBACK_TYPE CallbackType;
	union {
		PF_HEAP_RECORD Heap;
		PF_PAGE_RECORD Page;
		PF_HANDLE_RECORD Handle;
		PF_GDI_RECORD Gdi;
	};
} BTR_PROFILE_RECORD, *PBTR_PROFILE_RECORD;

//
// Profile Callback
//

typedef union _BTR_PROFILE_INFO {

	BTR_PROFILE_TYPE Type;
	ULONG Timestamp;
	ULONG Status;
	PVOID Context;

	struct {
		ULONG ThreadCount;
		ULONG SampleCount;
		ULONG PcCount;
		ULONG StackCount;
		ULONG ReportSize;
		ULONG SkipCount;
		LARGE_INTEGER TotalTime;
		LARGE_INTEGER KernelTime;
		LARGE_INTEGER UserTime;
	};

	struct {
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

	struct {
		ULONG NumberOfIoRead;
		ULONG NumberOfIoWrite;
		ULONG NumberOfIoCtl;
		ULONG64 SizeOfIoRead;
		ULONG64 SizeOfIoWrite;
		ULONG64 SizeOfIoCtl;
	};

} BTR_PROFILE_INFO, *PBTR_PROFILE_INFO;

typedef struct _BTR_PROFILE_OBJECT {

	BTR_PROFILE_ATTRIBUTE Attribute;

	//
	// Profile control information 
	//
	
	HANDLE SharedDataObject;
	HANDLE IndexFileObject;
	HANDLE DataFileObject;
	HANDLE StackFileObject;
	HANDLE ReportFileObject;

	//
	// IO record files
	//

	HANDLE IoObjectFile;
	HANDLE IoIrpFile;
	HANDLE IoNameFile;

	HANDLE StartEvent;
	HANDLE StopEvent;
	HANDLE UnloadEvent;
	HANDLE ControlEnd;

	HANDLE ExitProcessEvent;
	HANDLE ExitProcessAckEvent;

	//
	// Profile callback routines
	//

	BTR_START_ROUTINE  Start;
	BTR_STOP_ROUTINE   Stop;
	BTR_PAUSE_ROUTINE  Pause;
	BTR_RESUME_ROUTINE Resume;
	BTR_UNLOAD_ROUTINE Unload;
	BTR_THREAD_ATTACH_ROUTINE ThreadAttach;
	BTR_THREAD_DETACH_ROUTINE ThreadDetach;
	BTR_QUERY_HOTPATCH_ROUTINE QueryHotpatch;
	BTR_ISRUNTIMETHREAD_ROUTINE IsRuntimeThread;

    //
    // PauseEvent supports pause and resume
    //

    HANDLE PauseEvent;
	ULONG ExitStatus;

} BTR_PROFILE_OBJECT, *PBTR_PROFILE_OBJECT;

//
// Profile Report Structures
//

typedef enum _PF_STREAM_TYPE {
	STREAM_SYSTEM,
	STREAM_CPU_THREAD, 
	STREAM_CPU_ADDRESS,
	STREAM_MM_HEAP,
	STREAM_MM_PAGE,
	STREAM_MM_HANDLE,
	STREAM_MM_GDI,
	STREAM_IO_OBJECT,
	STREAM_IO_NAME,
	STREAM_IO_IRP,
	STREAM_IO_THREAD,
	STREAM_CCR,
	STREAM_STACK,
	STREAM_DLL,
	STREAM_CALLER,
	STREAM_LEAK_HEAP,
	STREAM_LEAK_PAGE,
	STREAM_LEAK_HANDLE,
	STREAM_LEAK_GDI,
	STREAM_PC,
	STREAM_FUNCTION,
	STREAM_SYMBOL,
	STREAM_LINE,
	STREAM_INDEX,
	STREAM_RECORD,
	STREAM_MARK,
	STREAM_LAST,
} PF_STREAM_TYPE, *PPF_STREAM_TYPE;

#pragma pack(push, 1)

typedef struct _PF_STREAM_INFO {
	ULONG64 Offset;
	ULONG64 Length;
} PF_STREAM_INFO, *PPF_STREAM_INFO;

#define DPF_SIGNATURE ((ULONG)'fpd')

typedef enum _BTR_CPU_TYPE {
	CPU_X86_32,
	CPU_X86_64,
	CPU_ARM_32,
	CPU_ARM_64,
} BTR_CPU_TYPE;

typedef enum _BTR_TARGET_TYPE {
	TARGET_WIN32,
	TARGET_WIN64,
	TARGET_WOW64,
} BTR_TARGET_TYPE;

typedef struct _PF_REPORT_HEAD {
	ULONG Signature;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG64 FileSize;
	ULONG IncludeCpu : 1;
	ULONG IncludeMm  : 1;
	ULONG IncludeIo  : 1;
	ULONG IncludeCcr : 1;
	ULONG Complete   : 1;
	ULONG Counters   : 1;
    ULONG Analyzed   : 1;
	ULONG CpuType    : 2;
	ULONG TargetType : 2;
	ULONG Reserved   : 22;
	ULONG CheckSum;
	PVOID Context;
	PF_STREAM_INFO Streams[STREAM_LAST];
} PF_REPORT_HEAD, *PPF_REPORT_HEAD;

typedef struct _PF_PERFORMANCE_INFO {
	FILETIME TimeStamp;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	SIZE_T VirtualBytes;
	SIZE_T PrivateBytes;
	ULONG KernelHandles;
	ULONG UserHandles;
	ULONG GdiHandles;
} PF_PERFORMANCE_INFO, *PPF_PERFORMANCE_INFO;

//
// CPU State
//

typedef enum _BTR_CPU_STATE_TYPE {
	CPU_STATE_RUN,
	CPU_STATE_WAIT,
	CPU_STATE_IO,
	CPU_STATE_SLEEP,
	CPU_STATE_MEMORY,
	CPU_STATE_PREEMPTED,
	CPU_STATE_UI,
	CPU_STATE_COUNT,
} BTR_CPU_STATE_TYPE;

//
// CPU state represent a sample thread's state,
// 1 byte size.
//

#pragma pack(push, 1)

typedef struct _BTR_CPU_STATE {
	UCHAR Type : 4;
	UCHAR Reserved : 4;
} BTR_CPU_STATE, *PBTR_CPU_STATE;

#pragma pack(pop)

typedef enum _BTR_CPU_ADDRESS_INDEX {
	
	//
	// KiFastSystemCallRet (only for 32 bits)
	//

	_KiFastSystemCallRet,

	//
	// Yield/Sleep
	//

	_NtYieldExecution,
	_NtDelayExecution,

	//
	// Synchronization
	//

	_NtWaitForSingleObject,
	_NtWaitForMultipleObjects,
	_NtSignalAndWaitForSingleObject,
	_NtWaitForKeyedEvent,
	_NtWaitForWorkViaWorkerFactory, 

	//
	// I/O
	//

	_NtCreateFile,
	_NtOpenFile,
	_NtReadFile,
	_NtWriteFile,
	_NtReadFileScatter,
	_NtWriteFileGather,
	_NtDeleteFile,
	_NtFsControlFile,
	_NtDeviceIoControlFile,
	_NtQueryInformationFile,
	_NtQueryDirectoryFile,
	_NtQueryAttributesFile,
	_NtQueryFullAttributesFile,
	_NtQueryEaFile,
	_NtSetInformationFile,
	_NtSetEaFile,

	BTR_CPU_ADDRESS_COUNT,

} BTR_CPU_ADDRESS_INDEX;

typedef struct _BTR_CPU_ADDRESS {
	BTR_CPU_ADDRESS_INDEX Index;
	ULONG64 Address;
	BTR_CPU_STATE_TYPE State;
	PCSTR Name;
} BTR_CPU_ADDRESS, *PBTR_CPU_ADDRESS;

//
// CPU PC Address Table
//

extern BTR_CPU_ADDRESS BtrCpuAddressTable[BTR_CPU_ADDRESS_COUNT];


typedef struct _PF_STREAM_CPU_ADDRESS {
	ULONG Count;
	BTR_CPU_ADDRESS Entry[ANYSIZE_ARRAY];
} PF_STREAM_CPU_ADDRESS, *PPF_STREAM_CPU_ADDRESS;

//
// System Stream
//

typedef struct _PF_STREAM_SYSTEM {

	WCHAR Computer[64];
	WCHAR Processor[64];
	WCHAR System[64];
	WCHAR Memory[64];

	ULONG ProcessId;
	ULONG SessionId;
	WCHAR ImagePath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];
	WCHAR UserName[MAX_PATH];
	
	//
	// QueryPerformanceCounter resolution
	//

	LARGE_INTEGER QpcFrequency;
	BTR_PROFILE_ATTRIBUTE Attr;
	PF_PERFORMANCE_INFO Info;

} PF_STREAM_SYSTEM, *PPF_STREAM_SYSTEM;

//
// Heap Stream
//

typedef struct _PF_STREAM_HEAP {

	HANDLE HeapHandle;
	ULONG Flags;
	WCHAR OwnerName[64];

	ULONG64 ReservedBytes;
	ULONG64 CommittedBytes;
	ULONG64 AllocatedBytes;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	ULONG CreateStackHash;
	ULONG CreateStackDepth;
	PVOID CreatePc;
	ULONG DestroyStackHash;
	ULONG DestroyStackDepth;
	PVOID DestroyPc;
	
	ULONG NumberOfRecords;
	PF_HEAP_RECORD Records[ANYSIZE_ARRAY];

} PF_STREAM_HEAP, *PPF_STREAM_HEAP;

typedef struct _PF_STREAM_HEAPLIST {

	ULONG64 ReservedBytes;
	ULONG64 CommittedBytes;

	ULONG64 NumberOfAllocs;
	ULONG64 NumberOfFrees;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;

	ULONG NumberOfActiveHeaps;
	ULONG NumberOfRetiredHeaps;

	ULONG NumberOfHeaps;
	PF_STREAM_INFO Heaps[ANYSIZE_ARRAY];

} PF_STREAM_HEAPLIST, *PPF_STREAM_HEAPLIST;

//
// Page Stream
//

typedef struct _PF_STREAM_PAGE {

	ULONG64 ReservedBytes;
	ULONG64 CommittedBytes;

	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	ULONG64 SizeOfAllocs;
	ULONG64 SizeOfFrees;

	ULONG NumberOfRecords;
	PF_PAGE_RECORD Records[ANYSIZE_ARRAY];

} PF_STREAM_PAGE, *PPF_STREAM_PAGE;

//
// Handle Stream
//

typedef struct _PF_STREAM_HANDLE {

	ULONG HandleCount;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	PF_TYPE_ENTRY Types[HANDLE_LAST];

	ULONG NumberOfRecords;
	PF_HANDLE_RECORD Records[ANYSIZE_ARRAY];

} PF_STREAM_HANDLE, *PPF_STREAM_HANDLE;

//
// GDI Stream
//

typedef struct _PF_STREAM_GDI {
	
	ULONG HandleCount;
	ULONG NumberOfAllocs;
	ULONG NumberOfFrees;
	PF_TYPE_ENTRY Types[GDI_OBJ_LAST]; 

	ULONG NumberOfRecords;
	PF_GDI_RECORD Records[ANYSIZE_ARRAY];

} PF_STREAM_GDI, *PPF_STREAM_GDI;

//
// Dll Stream
//

typedef BTR_DLL_FILE PF_STREAM_DLL, *PPF_STREAM_DLL;

//
// Core Dll Stream
//

typedef enum _BTR_CORE_DLL_TYPE {
	CORE_DLL_NTDLL,
	CORE_DLL_KERNEL32,
	CORE_DLL_USER32,
	CORE_DLL_GDI32,
	CORE_DLL_ADVAPI32,
	CORE_DLL_OLE32,
	CORE_DLL_OLE32AUT,
	CORE_DLL_MSVCRT,
	CORE_DLL_COUNT,
} BTR_CORE_DLL_TYPE;

//
// Heap Leak Stream, each entry points to a BTR_LEAK_FILE stream
//

typedef struct _PF_STREAM_HEAP_LEAK {
	ULONG NumberOfHeaps;
    ULONG Padding;
    ULONG64 Count;	
	ULONG64 LeakCount;
    ULONG64 Bytes;
	PF_STREAM_INFO Info[ANYSIZE_ARRAY];
} PF_STREAM_HEAP_LEAK, *PPF_STREAM_HEAP_LEAK;

typedef BTR_LEAK_FILE PF_STREAM_PAGE_LEAK, *PPF_STREAM_PAGE_LEAK;
typedef BTR_LEAK_FILE PF_STREAM_HANDLE_LEAK, *PPF_STREAM_HANDLE_LEAK;
typedef BTR_LEAK_FILE PF_STREAM_GDI_LEAK, *PPF_STREAM_GDI_LEAK;


//
// Define IO Stream
//

typedef struct _IO_COUNTER_METHOD {
	ULONG RequestCount;
	ULONG CompleteCount;
	ULONG FailureCount;
	ULONG SynchronousCount;
	ULONG AsynchronousCount;
	ULONG PendingCount; 
	ULONG64 RequestSize;
	ULONG64 CompleteSize;
	ULONG64 FailureSize;
	ULONG MinimumLatency;
	ULONG MaximumLatency;
	ULONG AverageLatency;
	ULONG MinimumSize;
	ULONG MaximumSize;
	ULONG AverageSize;
} IO_COUNTER_METHOD, *PIO_COUNTER_METHOD;

#define IO_INVALID_IRP_INDEX  ((ULONG)-1)

typedef struct _IO_OBJECT_ON_DISK {
	ULONG ObjectId;
	ULONG StackId; 
	ULONG Flags;
	ULONG NameOffset;
	FILETIME Start;
	FILETIME End;
	ULONG IrpCount;
	ULONG FirstIrp;
	union {
		PVOID Context;
		struct _IO_STACKTRACE *Trace;
		ULONG CurrentIrp;
	};
	IO_COUNTER_METHOD Counters[IO_OP_NUMBER];
} IO_OBJECT_ON_DISK, *PIO_OBJECT_ON_DISK;


#define IoIsFileObject(_O)\
	(_O->Flags & OF_FILE) 

#define IoIsSocketObject(_O)\
	(_O->Flags & (OF_SKIPV4|OF_SKIPV6))

#define IoIsSocketIpv4(_O)\
	(_O->Flags & OF_SKIPV4)

#define IoIsSocketIpv6(_O)\
	(_O->Flags & OF_SKIPV6)

#define IoIsSocketTcp(_O)\
	(_O->Flags & OF_SKTCP)

#define IoIsSocketUdp(_O)\
	(_O->Flags & OF_SKUDP)


typedef struct _IO_IRP_ON_DISK {
	ULONG RequestId;
	ULONG ObjectId;
	ULONG StackId;
	ULONG Operation        : 4;
	ULONG Asynchronous     : 1;
	ULONG IoCompletionPort : 1;
	ULONG UserApc          : 1;
	ULONG Aborted          : 1;
	ULONG Orphaned         : 1;
	ULONG Complete         : 1;
	ULONG RequestBytes;
	ULONG CompletionBytes;
	ULONG IoStatus;
	ULONG RequestThreadId;
	ULONG CompleteThreadId;
	ULONG NextIrp;
	ULONG ThreadedNextIrp;
	FILETIME Time;
	union {
		LARGE_INTEGER Duration;
		double LatencyMs;
	};
} IO_IRP_ON_DISK, *PIO_IRP_ON_DISK;

//
// STREAM_IO_OBJECT
// 

typedef struct _PF_STREAM_IO_OBJECT { 
	IO_OBJECT_ON_DISK Object[ANYSIZE_ARRAY];	
} PF_STREAM_IO_OBJECT, *PPF_STREAM_IO_OBJECT;

//
// STREAM_IO_IRP
//

typedef struct _PF_STREAM_IO_IRP {
	IO_IRP_ON_DISK Irp[ANYSIZE_ARRAY];
} PF_STREAM_IO_IRP, *PPF_STREAM_IO_IRP;


#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif
