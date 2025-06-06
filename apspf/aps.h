//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _APS_H_
#define _APS_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#ifndef _WIN32_WINNT          
#define _WIN32_WINNT 0x0600 
#endif

#define WIN32_LEAN_AND_MEAN

#if defined(_IPCTL_)
#include <WinSock2.h>
#include <MSWSock.h>
#include <Ws2tcpip.h>
#include <in6addr.h>
#endif

#include "apsbtr.h"
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <assert.h>
#include "ntapi.h"
#include "list.h"
#include <stdlib.h>


//
// Global Macros
//

#define APS_PAGESIZE 4096


typedef struct _APS_MODULE {
	LIST_ENTRY ListEntry;
	PVOID BaseVa;
	ULONG Size;
	PWSTR Name;
	PWSTR FullPath;
	PWSTR Version;
	PWSTR TimeStamp;
	PWSTR Company;
	PWSTR Description;
	PVOID Context;
} APS_MODULE, *PAPS_MODULE;

typedef struct _APS_THREAD {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	ULONG Priority;
	PVOID StartAddress;
} APS_THREAD, *PAPS_THREAD;

typedef struct _APS_PROCESS {

	LIST_ENTRY ListEntry;
	PVOID PebAddress;
	ULONG ProcessId;
	ULONG ParentId;
	ULONG SessionId;
	PWSTR Name;
	PWSTR FullPath;
	PWSTR CommandLine;
	
	ULONG ThreadCount;
	PSYSTEM_THREAD_INFORMATION Thread;

	ULONG Priority;
	FILETIME CreateTime; 
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	ULONG KernelHandleCount;
	ULONG GdiHandleCount;
	ULONG UserHandleCount;
	ULONG_PTR VirtualBytes;
	ULONG_PTR PrivateBytes;
	ULONG_PTR WorkingSetBytes;
	LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;

} APS_PROCESS, *PAPS_PROCESS;

typedef struct _APS_EXPORT_DESCRIPTOR {
	LIST_ENTRY Entry;
	PSTR Name;
	PSTR Forward;
	ULONG Ordinal;
	ULONG Rva;
	ULONG_PTR Va;
} APS_EXPORT_DESCRIPTOR, *PAPS_EXPORT_DESCRIPTOR;

//
// Standard Callback Routine
//

typedef ULONG 
(*APS_CALLBACK_ROUTINE)(
    __in PVOID Context
    );

typedef struct _APS_CALLBACK {
    LIST_ENTRY ListEntry;
    APS_CALLBACK_ROUTINE Callback;
    PVOID Context;
    ULONG Status;
} APS_CALLBACK, *PAPS_CALLBACK;

typedef struct _APS_SECURITY_ATTRIBUTE {
	SECURITY_ATTRIBUTES Attribute;
	PSECURITY_DESCRIPTOR Descriptor;
	PACL Acl;
	PSID Sid;
} APS_SECURITY_ATTRIBUTE, *PAPS_SECURITY_ATTRIBUTE;

typedef struct _STRING_ENTRY {
	LIST_ENTRY ListEntry;
	PCHAR Buffer;
	USHORT Bytes;
	USHORT IsAnsi;
} STRING_ENTRY, *PSTRING_ENTRY;


//
// Initialize
//

ULONG
ApsInitialize(
	__in ULONG Flag	
	);

ULONG
ApsUninitialize(
	VOID
	);

//
// Heap
//

PVOID
ApsMalloc(
	__in SIZE_T Length
	);

VOID
ApsFree(
	__in PVOID Address
	);

PVOID
ApsAlignedMalloc(
	__in ULONG ByteCount,
	__in ULONG Alignment
	);

VOID
ApsAlignedFree(
	__in PVOID Address
	);

//
// Process
//

BOOLEAN
ApsIsWindowsXPAbove(
	VOID
	);

BOOLEAN
ApsIsVistaAbove(
	VOID
	);

BOOLEAN 
ApsIsWow64Process(
	__in HANDLE ProcessHandle	
	);

BOOLEAN
ApsIs64BitsProcess(
	VOID
	);

BOOLEAN
ApsIsLegitimateProcess(
	__in ULONG ProcessId
	);

ULONG
ApsQueryProcessFullPath(
	__in ULONG ProcessId,
	__in PWSTR Buffer,
	__in ULONG Length,
	__out PULONG Result
	);

HANDLE
ApsDuplicateHandle(
	__in HANDLE DestineProcess,
	__in HANDLE SourceHandle
	);

ULONG 
ApsAdjustPrivilege(
    __in PWSTR PrivilegeName, 
    __in BOOLEAN Enable
    );

ULONG
ApsAllocateQueryBuffer(
	__in SYSTEM_INFORMATION_CLASS InformationClass,
	__out PVOID *Buffer,
	__out PULONG BufferLength,
	__out PULONG ActualLength
	);

VOID
ApsReleaseQueryBuffer(
	__in PVOID StartVa
	);

ULONG
ApsQueryProcessList(
	__out PLIST_ENTRY ListHead
	);

ULONG
ApsQueryThreadList(
	__in ULONG ProcessId,
	__out PAPS_PROCESS *Process
	);

PAPS_PROCESS
ApsQueryProcessById(
	__in PLIST_ENTRY ListHead,
	__in ULONG ProcessId
	);

ULONG
ApsQueryProcessInformation(
	__in ULONG ProcessId,
	__out PSYSTEM_PROCESS_INFORMATION *Information
	);

ULONG
ApsQueryProcessByName(
	__in PWSTR Name,
	__out PLIST_ENTRY ListHead
	);

VOID
ApsFreeProcess(
	__in PAPS_PROCESS Process
	);

VOID
ApsFreeProcessList(
	__in PLIST_ENTRY ListHead
	);

ULONG
ApsSuspendProcess(
	__in HANDLE ProcessHandle
	);

ULONG
ApsResumeProcess(
	__in HANDLE ProcessHandle
	);

ULONG
ApsCreateRemoteThread(
	__in HANDLE ProcessHandle,
	__in PVOID StartAddress,
	__in PVOID Context,
	__out PHANDLE ThreadHandle
	);

PVOID
ApsQueryTebAddress(
	__in HANDLE ThreadHandle
	);

PVOID
ApsQueryPebAddress(
	__in HANDLE ProcessHandle 
	);

ULONG
ApsGetFullPathName(
	__in PWSTR BaseName,
	__in PWCHAR Buffer,
	__in USHORT Length,
	__out PUSHORT ActualLength
	);

ULONG
ApsGetProcessPath(
	__in PWCHAR Buffer,
	__in USHORT Length,
	__out PUSHORT ActualLength
	);

ULONG
ApsGetProcessPathA(
	__in PCHAR Buffer,
	__in USHORT Length,
	__out PUSHORT ActualLength
);

BOOLEAN
ApsGetSystemRoutine(
	VOID
	);

ULONG
ApsQuerySystemVersion(
	__in PWCHAR Buffer,
	__in ULONG Length
	);

ULONG
ApsQueryCpuModel(
	__in PWCHAR Buffer,
	__in ULONG Length
	);

ULONG
ApsGetComputerName(
	__in PWCHAR Buffer,
	__in ULONG Length
	);

ULONG64
ApsGetPhysicalMemorySize(
	VOID
	);

//
// Module
//

ULONG
ApsQueryModule(
	__in ULONG ProcessId,
	__in BOOLEAN Version,
	__out PLIST_ENTRY ListHead
	);

PAPS_MODULE
ApsGetModuleByName(
	__in PLIST_ENTRY ListHead,
	__in PWSTR ModuleName
	);

PAPS_MODULE
ApsGetModuleByAddress(
	__in PLIST_ENTRY ListHead,
	__in PVOID StartVa
	);

ULONG
ApsGetModuleVersion(
	__in PAPS_MODULE Module
	);

VOID
ApsFreeModuleList(
	__in PLIST_ENTRY ListHead
	);

VOID
ApsFreeModule(
	__in PAPS_MODULE Module 
	);

ULONG
ApsGetModuleTimeStamp(
	__in PAPS_MODULE Module,
	__out PULONG64 TimeStamp
	);

BOOLEAN
ApsGetModuleInformation(
	__in ULONG ProcessId,
	__in PWSTR DllName,
	__in BOOLEAN FullPath,
	__out HMODULE *DllHandle,
	__out PULONG_PTR Address,
	__out SIZE_T *Size
	);

PVOID
ApsGetRemoteApiAddress(
	__in ULONG ProcessId,
	__in PWSTR DllName,
	__in PSTR ApiName
	);

PVOID
ApsMapImageFile(
	__in PWSTR ModuleName,
	__out PHANDLE FileHandle,
	__out PHANDLE ViewHandle
	);

VOID
ApsUnmapImageFile(
	__in PVOID  MappedBase,
	__in HANDLE FileHandle,
	__in HANDLE ViewHandle
	);

ULONG
ApsRvaToOffset(
	__in ULONG Rva,
	__in PIMAGE_SECTION_HEADER Headers,
	__in ULONG NumberOfSections
	);

ULONG
ApsEnumerateDllExport(
	__in PWSTR ModuleName,
	__in PAPS_MODULE Module, 
	__out PULONG Count,
	__out PAPS_EXPORT_DESCRIPTOR *Descriptor
	);

PVOID
ApsGetDllForwardAddress(
	__in ULONG ProcessId,
	__in PSTR DllForward
	);

VOID
ApsGetForwardDllName(
	__in PSTR Forward,
	__out PCHAR DllName,
	__in ULONG Length
	);

VOID
ApsGetForwardApiName(
	__in PSTR Forward,
	__out PCHAR ApiName,
	__in ULONG Length
	);

ULONG
ApsEnumerateDllExportByName(
	__in ULONG ProcessId,
	__in PWSTR ModuleName,
	__in PWSTR ApiName,
	__out PULONG Count,
	__out PLIST_ENTRY ApiList
	);

VOID
ApsFreeExportDescriptor(
	__in PAPS_EXPORT_DESCRIPTOR Descriptor,
	__in ULONG Count
	);

//
// Loader 
//

ULONG
ApsLoadRuntime(
	__in ULONG ProcessId,
	__in HANDLE ProcessHandle,
	__in PWSTR Path,
	__in PSTR StartApiName,
	__in PVOID Argument
	);

BOOLEAN
ApsIsRuntimeLoaded(
	__in ULONG ProcessId,
	__in PWSTR Path 
	);

ULONG
ApsGetRuntimeStatus(
	__in PWSTR Path,
	__out PULONG Status
	);

//
// Critical Section 
//

typedef CRITICAL_SECTION APS_CRITICAL_SECTION;
typedef APS_CRITICAL_SECTION *PAPS_CRITICAL_SECTION;

VOID
ApsInitCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	);

VOID
ApsInitCriticalSectionEx(
	__in PAPS_CRITICAL_SECTION Lock,
	__in ULONG SpinCount
	);

VOID
ApsEnterCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	);

BOOLEAN
ApsTryEnterCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	);

VOID
ApsLeaveCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	);

VOID
ApsDeleteCriticalSection(
	__in PAPS_CRITICAL_SECTION Lock
	);

VOID
ApsInitSpinLock(
	__in PBTR_SPINLOCK Lock,
	__in ULONG SpinCount
	);

VOID
ApsAcquireSpinLock(
	__in PBTR_SPINLOCK Lock
	);

VOID
ApsReleaseSpinLock(
	__in PBTR_SPINLOCK Lock
	);

//
// String 
//

SIZE_T 
ApsConvertUnicodeToAnsi(
	__in PWSTR Source, 
	__out PSTR *Destine
	);

VOID 
ApsConvertAnsiToUnicode(
	__in PSTR Source,
	__out PWSTR *Destine
	);

ULONG
ApsCreateUuidString(
	__in PWCHAR Buffer,
	__in ULONG Length
	);

VOID
ApsConvertGuidToString(
	__in GUID *Guid,
	__out PCHAR Buffer,
	__in ULONG Length
	);

ULONG
ApsFormatAddress(
    __in PVOID Buffer,
    __in ULONG Length,
    __in PVOID Address,
    __in BOOLEAN Unicode
    );

//
// Alignment
//

ULONG_PTR
ApsUlongPtrRoundDown(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	);

ULONG_PTR
ApsUlongPtrRoundUp(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	);

ULONG
ApsUlongRoundDown(
	__in ULONG Value,
	__in ULONG Align
	);

ULONG
ApsUlongRoundUp(
	__in ULONG Value,
	__in ULONG Align
	);

ULONG64
ApsUlong64RoundDown(
	__in ULONG64 Value,
	__in ULONG64 Align
	);

ULONG64
ApsUlong64RoundUp(
	__in ULONG64 Value,
	__in ULONG64 Align
	);

BOOLEAN
ApsIsUlongAligned(
	__in ULONG Value,
	__in ULONG Unit
	);

BOOLEAN
ApsIsUlong64Aligned(
	__in ULONG64 Value,
	__in ULONG64 Unit
	);

//
// Time
//

double
ApsComputeMilliseconds(
	__in ULONG Duration
	);

double
ApsNanoUnitToMilliseconds(
    __in ULONG NanoUnit
    );

//
// Utility
//

BOOLEAN
ApsIsValidPath(
	__in PWSTR Path
	);

BOOLEAN
ApsIsValidFolder(
	__in PWSTR Path
	);

ULONG
ApsSplitUnicodeString(
	__in PWSTR MultiSz,
	__in WCHAR Delimiter,
	__out PLIST_ENTRY ListHead
	);

ULONG
ApsSplitAnsiString(
	__in PSTR MultiSz,
	__in CHAR Delimiter,
	__out PLIST_ENTRY ListHead
	);

ULONG
ApsCreateRandomNumber(
    __in ULONG Minimum,
    __in ULONG Maximum
    );

LONG
ApsComputeClosestLong(
    __in FLOAT Value
    );

double
ApsComputeMicroseconds(
	_In_ LONG64 Ticks,
	_In_ LONG64 QpcFreqency
	);

VOID
ApsNormalizeDuration(
	__in ULONG Duration,
	__out PULONG Millisecond,
	__out PULONG Second,
	__out PULONG Minute
	);

VOID
ApsFormatDuration(
	__in ULONG Millisecond,
	__in ULONG Second,
	__in ULONG Minute,
	__in PWCHAR Buffer,
	__in ULONG Length
	);

#ifdef _IPCTL_

PWSTR 
ApsIpv4AddressToString(
	_In_  const IN_ADDR *Addr,
	_Out_       PWSTR   S
	);

PWSTR 
ApsIpv6AddressToString(
	_In_  const IN6_ADDR *Addr,
	_Out_       PWSTR   S
	);

PWSTR 
ApsSockaddrToString(
	_In_  const SOCKADDR_STORAGE *Address,
	_Out_       PWSTR   S
	);
PWSTR 
ApsSockaddrToPort(
	_In_  const SOCKADDR_STORAGE *Address,
	_Out_       PWSTR   S,
	_In_ ULONG Length
	);

#endif

VOID
ApsFailFast(
	VOID
	);

VOID __cdecl
ApsDebugTrace(
	__in PSTR Format,
	__in ...
	);

VOID __cdecl
ApsTrace(
	__in PSTR Format,
	__in ...
	);

//
// Report Deduction
//

typedef enum _DEDUCTION_KEY {
	DEDUCTION_INCLUSIVE_PERCENT,
} DEDUCTION_KEY;

BOOLEAN
ApsIsDeductionEnabled(
	VOID
	);

VOID
ApsEnableDeduction(
	__in BOOLEAN Enable
	);

ULONG
ApsGetDeductionThreshold(
	__in DEDUCTION_KEY Key
	);

VOID
ApsSetDeductionThreshold(
	__in DEDUCTION_KEY Key,
	__in ULONG Value
	);

struct _CALL_GRAPH;
struct _CALL_NODE;
struct _CALL_TREE;

BOOLEAN
ApsIsCallNodeAboveThreshold(
	__in struct _CALL_GRAPH *Graph,
	__in struct _CALL_NODE *Node
	);

BOOLEAN
ApsIsCallTreeAboveThreshold(
	__in struct _CALL_GRAPH *Graph,
	__in struct _CALL_TREE *Node
	);

#ifdef _APS_IMPORT

__declspec(dllimport)
extern BOOLEAN ApsIs64Bits;

__declspec(dllimport)
extern BOOLEAN ApsIsWow64;

__declspec(dllimport)
extern ULONG ApsPageSize;

#else

extern BOOLEAN ApsIs64Bits;
extern BOOLEAN ApsIsWow64;
extern ULONG ApsPageSize;
extern CHAR ApsCurrentPathA[];
extern WCHAR ApsCurrentPath[];

#endif

//
// User defined warning
//

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOW__ __FILE__ "("__STR1__(__LINE__)") : warning: "

//
// #pragma message(__LOW__"text")
//

#ifdef __cplusplus
}
#endif
#endif
