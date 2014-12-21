//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BTRSDK_H_
#define _BTRSDK_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// N.B. Define _BTR_SDK_ to use this file
//

#if defined (_BTR_SDK_)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <emmintrin.h>

typedef enum _BTR_FEATURE_FLAG {
	FeatureLibrary      = 0x00000001,
	FeatureLocal        = 0x00000002,
	FeatureRemote       = 0x00000004,
	FeatureLogging      = 0x00000100,
	FeatureStdCache     = 0x00001000,
	FeatureTlsCache     = 0x00002000,       
	FeatureSharedCache  = 0x00004000,
} BTR_FEATURE_FLAG, *PBTR_FEATURE_FLAG;

typedef enum _BTR_RETURN_TYPE {
	ReturnVoid,
	ReturnInt8,
	ReturnInt16,
	ReturnInt32,
	ReturnInt64,
	ReturnUInt8,
	ReturnUInt16,
	ReturnUInt32,
	ReturnUInt64,
	ReturnPtr,
	ReturnIntPtr,   // ptr sized int
	ReturnUIntPtr,  // ptr sized uint
	ReturnFloat,
	ReturnDouble,
	ReturnNumber,
} BTR_RETURN_TYPE, PBTR_RETURN_TYPE;

typedef enum _BTR_RECORD_TYPE {
	RECORD_BUILDIN,
	RECORD_FAST,
	RECORD_FILTER,
	RECORD_CALLBACK,
} BTR_RECORD_TYPE, *PBTR_RECORD_TYPE;

typedef enum _BTR_PROBE_TYPE {
	PROBE_BUILDIN,
	PROBE_FAST,
	PROBE_FILTER,
	PROBE_CALLBACK,
} BTR_PROBE_TYPE, *PBTR_PROBE_TYPE;

typedef struct _BTR_RECORD_HEADER {

	ULONG TotalLength;

	ULONG RecordFlag : 4;
	ULONG RecordType : 4;
	ULONG ReturnType : 4;
	ULONG Reserved   : 4;
	ULONG StackDepth : 6;
	ULONG CallDepth  : 10;

	PVOID Address;
	PVOID Caller;
	ULONG Sequence;
	ULONG ObjectId;
	ULONG StackHash;
	ULONG ProcessId;
	ULONG ThreadId;
	ULONG LastError;
	ULONG Duration;
	FILETIME FileTime;

	union {
		ULONG64 Return;
		double  FpuReturn;
	};

} BTR_RECORD_HEADER, *PBTR_RECORD_HEADER;

typedef struct _BTR_FILTER_RECORD {
	BTR_RECORD_HEADER Base;
	GUID FilterGuid;
	ULONG ProbeOrdinal;
	CHAR Data[ANYSIZE_ARRAY];
} BTR_FILTER_RECORD, *PBTR_FILTER_RECORD;

//
// _R, Filter record base address
// _T, Type to be converted to
//

#define FILTER_RECORD_POINTER(_R, _T)   ((_T *)(&_R->Data[0]))

typedef struct _BTR_BITMAP {
    ULONG SizeOfBitMap;
    PULONG Buffer; 
} BTR_BITMAP, *PBTR_BITMAP;

typedef struct _BTR_FILTER_PROBE {
	ULONG Ordinal;
	ULONG Flags;
	BTR_RETURN_TYPE ReturnType;
	PVOID Address;
	PVOID FilterCallback;
	PVOID DecodeCallback;
	WCHAR DllName[64];	
	WCHAR ApiName[64];
} BTR_FILTER_PROBE, *PBTR_FILTER_PROBE;

typedef ULONG 
(*BTR_UNREGISTER_CALLBACK)(
	VOID
	);

//
// N.B. BTR_FILTER_CONTROL's size limit is 8KB
//

#define BTR_MAX_FILTER_CONTROL_SIZE  (1024 * 8)

typedef struct _BTR_FILTER_CONTROL {
	ULONG Length;
	CHAR Control[ANYSIZE_ARRAY];
} BTR_FILTER_CONTROL, *PBTR_FILTER_CONTROL;

typedef ULONG
(*BTR_CONTROL_CALLBACK)(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

typedef ULONG
(*BTR_OPTION_CALLBACK)(
	IN HWND hWndParent,
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG ActualLength
	);

typedef struct _BTR_FILTER {

	WCHAR FilterName[MAX_PATH];
	WCHAR Author[MAX_PATH];
	WCHAR Description[MAX_PATH];
	WCHAR MajorVersion;
	WCHAR MinorVersion;
	HICON FilterBanner;

	ULONG FilterTag;
	GUID FilterGuid;
	PBTR_FILTER_PROBE Probes;
	ULONG ProbesCount;
	
	BTR_UNREGISTER_CALLBACK UnregisterCallback;
	BTR_CONTROL_CALLBACK ControlCallback;
	BTR_OPTION_CALLBACK  OptionCallback;

	//
	// N.B. Don't touch the following fields.
	//

	LIST_ENTRY ListEntry;
	BTR_BITMAP BitMap;
	WCHAR FilterFullPath[MAX_PATH];
	PVOID BaseOfDll;
	ULONG SizeOfImage;

} BTR_FILTER, *PBTR_FILTER;

//
// BTR_PROBE_CONTEXT is used by filter to get the destine address to call,
// and used to capture call result just in time.
//

typedef struct _BTR_PROBE_CONTEXT {
	PVOID Destine;
	PVOID Frame;
} BTR_PROBE_CONTEXT, *PBTR_PROBE_CONTEXT;

typedef ULONG
(*BTR_DECODE_CALLBACK)(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG BufferLength,
	OUT PWCHAR Buffer
	);

typedef PBTR_FILTER
(*BTR_FLT_GETOBJECT)(
	VOID
	);

PBTR_FILTER 
BtrFltGetObject(
	VOID
	);

PVOID 
BtrFltMalloc(
	IN ULONG Length
	);

VOID 
BtrFltFree(
	IN PVOID Address
	);

//
// Free the Destine via BtrFltFree
//

SIZE_T 
BtrFltConvertUnicodeToAnsi(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	);

//
// Free the Destine via BtrFltFree
//

VOID 
BtrFltConvertAnsiToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	);

VOID
BtrFltGetContext(
	IN PBTR_PROBE_CONTEXT Context 
	);

VOID
BtrFltSetContext(
	IN PBTR_PROBE_CONTEXT Context 
	);

VOID WINAPI
BtrFltGetLastError(
	IN PBTR_PROBE_CONTEXT Context 
	);

VOID 
BtrFltQueueRecord(
	IN PVOID Record
	);

PBTR_FILTER_RECORD 
BtrFltAllocateRecord(
	IN ULONG DataLength, 
	IN GUID FilterGuid, 
	IN ULONG Ordinal
	);

VOID 
BtrFltFreeRecord(
	PBTR_FILTER_RECORD Record
	);

ULONG 
BtrFltDeQueueRecord(
	IN PVOID Buffer,
	IN ULONG Length,
	OUT PULONG RecordLength,
	OUT PULONG RecordCount 
	);

VOID 
BtrFltSetExemption(
	VOID
	);

VOID 
BtrFltClearExemption(
	VOID
	);

VOID WINAPI
BtrFltEnterExemptionRegion(
	VOID
	);

VOID WINAPI
BtrFltLeaveExemptionRegion(
	VOID
	);

ULONG WINAPI
BtrFltStartRuntime(
	IN PULONG Features 
	);

ULONG WINAPI
BtrFltStopRuntime(
	IN PVOID Reserved
	);

VOID
BtrFormatError(
	IN ULONG ErrorCode,
	IN BOOLEAN SystemError,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength, 
	OUT PULONG ActualLength
	);

#endif

#ifdef __cplusplus
}
#endif

#endif