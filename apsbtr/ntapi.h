//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _NTAPI_H_
#define _NTAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef LONG NTSTATUS, *PNTSTATUS;

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

#define STATUS_SUCCESS                ((NTSTATUS)0L)
#define STATUS_BUFFER_TOO_SMALL       ((NTSTATUS)0xC0000023L)
#define STATUS_INFO_LENGTH_MISMATCH   ((NTSTATUS)0xC0000004L)

typedef LONG KPRIORITY;

typedef struct _ANSI_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG  Length;
    HANDLE  RootDirectory;
    PUNICODE_STRING  ObjectName;
    ULONG  Attributes;
    PVOID  SecurityDescriptor;
    PVOID  SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _TEB {
    NT_TIB Tib;
    PVOID EnvironmentPointer;
    CLIENT_ID Cid;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    struct _PEB *ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    struct _W32THREAD* Win32ThreadInfo;
    ULONG User32Reserved[0x1A];
    ULONG UserReserved[5];
    PVOID WOW32Reserved;
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    PVOID SystemReserved1[0x36];
    LONG ExceptionCode;

    //
	// N.B. Windows XP embed a _ACTIVATION_CONTEXT_STACK structure,
	// not a pointer. like the followings:
	//
	// +0x1a8 ActivationContextStack : _ACTIVATION_CONTEXT_STACK
    //  +0x000 Flags            : Uint4B
    //  +0x004 NextCookieSequenceNumber : Uint4B
    //  +0x008 ActiveFrame      : Ptr32 Void
    //  +0x00c FrameListCache   : _LIST_ENTRY
    // +0x1bc SpareBytes1      : [24] UChar
	//

    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;

} TEB, *PTEB;

typedef struct _INITIAL_TEB {
	PVOID  StackBase;
	PVOID  StackLimit;
	PVOID  StackCommit;
	PVOID  StackCommitMax;
	PVOID  StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _PEB_LDR_DATA {
   ULONG Length;
   BOOLEAN Initialized;
   PVOID SsHandle;
   LIST_ENTRY InLoadOrderModuleList;
   LIST_ENTRY InMemoryOrderModuleList;
   LIST_ENTRY InInitializationOrderModuleList;
   PVOID EntryInProgress;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        PVOID SectionPointer;
    };
    ULONG CheckSum;
    union {
        ULONG TimeDateStamp;
        PVOID LoadedImports;
    };
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _CURDIR {
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;
    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[32];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        struct
        {
            UCHAR ImageUsesLargePages:1;
            UCHAR IsProtectedProcess:1;
            UCHAR IsLegacyProcess:1;
            UCHAR IsImageDynamicallyRelocated:1;
            UCHAR SkipPatchingUser32Forwarders:1;
            UCHAR SpareBits:3;
        };
        UCHAR BitField;
    };
#else
    BOOLEAN SpareBool;
#endif
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PVOID Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
} PEB, *PPEB;

typedef enum _NT_THREAD_INFORMATION_CLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,
    MaxThreadInfoClass
} NT_THREAD_INFORMATION_CLASS;

typedef enum _NT_PROCESS_INFORMATION_CLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    ProcessIoPriority,
    ProcessExecuteFlags,
    ProcessTlsInformation,
    ProcessCookie,
    ProcessImageInformation,
    ProcessCycleTime,
    ProcessPagePriority,
    ProcessInstrumentationCallback,
    MaxProcessInfoClass
} NT_PROCESS_INFORMATION_CLASS;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
} SYSTEM_INFORMATION_CLASS;

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    KPRIORITY Priority;
    LONG BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;
 
typedef struct _THREAD_CYCLE_TIME_INFORMATION {
    ULARGE_INTEGER AccumulatedCycles;
    ULARGE_INTEGER CurrentCycleCount;
} THREAD_CYCLE_TIME_INFORMATION, *PTHREAD_CYCLE_TIME_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
    GateWait,
    MaximumThreadState
} KTHREAD_STATE, *PKTHREAD_STATE;

typedef enum _KWAIT_REASON {
    Executive = 0,
    FreePage = 1,
    PageIn = 2,
    PoolAllocation = 3,
    DelayExecution = 4,
    Suspended = 5,
    UserRequest = 6,
    WrExecutive = 7,
    WrFreePage = 8,
    WrPageIn = 9,
    WrPoolAllocation = 10,
    WrDelayExecution = 11,
    WrSuspended = 12,
    WrUserRequest = 13,
    WrEventPair = 14,
    WrQueue = 15,
    WrLpcReceive = 16,
    WrLpcReply = 17,
    WrVirtualMemory = 18,
    WrPageOut = 19,
    WrRendezvous = 20,
    Spare2 = 21,
    Spare3 = 22,
    Spare4 = 23,
    Spare5 = 24,
    WrCalloutStack = 25,
    WrKernel = 26,
    WrResource = 27,
    WrPushLock = 28,
    WrMutex = 29,
    WrQuantumEnd = 30,
    WrDispatchInt = 31,
    WrPreempted = 32,
    WrYieldExecution = 33,
    WrFastMutex = 34,
    WrGuardedMutex = 35,
    WrRundown = 36,
    MaximumWaitReason = 37
} KWAIT_REASON, *PKWAIT_REASON;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER Spare[3];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR PageDirectoryBase;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[ANYSIZE_ARRAY];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectTypesInformation,
    ObjectHandleFlagInformation,
    ObjectSessionInformation,
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_BASIC_INFORMATION {
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG Reserved[3];
    ULONG NameInfoSize;
    ULONG TypeInfoSize;
    ULONG SecurityDescriptorSize;
    LARGE_INTEGER CreationTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_NAME_INFORMATION {
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyCachedInformation,
	KeyNameInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyVirtualizationInformation
} KEY_INFORMATION_CLASS;

typedef struct _KEY_NAME_INFORMATION {
    ULONG NameLength;
    WCHAR Name[ANYSIZE_ARRAY];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

#define RTL_DEBUG_QUERY_MODULES            0x01
#define RTL_DEBUG_QUERY_BACKTRACES         0x02
#define RTL_DEBUG_QUERY_HEAPS              0x04
#define RTL_DEBUG_QUERY_HEAP_TAGS          0x08
#define RTL_DEBUG_QUERY_HEAP_BLOCKS        0x10
#define RTL_DEBUG_QUERY_LOCKS              0x20

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    ULONG Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    CHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[ANYSIZE_ARRAY];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX {
    ULONG NextOffset;
    RTL_PROCESS_MODULE_INFORMATION BaseInfo;
    ULONG ImageCheckSum;
    ULONG TimeDateStamp;
    PVOID DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX, *PRTL_PROCESS_MODULE_INFORMATION_EX;

typedef struct _RTL_HEAP_TAG_INFO {
   ULONG NumberOfAllocations;
   ULONG NumberOfFrees;
   ULONG BytesAllocated;
} RTL_HEAP_TAG_INFO, *PRTL_HEAP_TAG_INFO;

typedef struct _RTL_HEAP_USAGE_ENTRY {
    struct _RTL_HEAP_USAGE_ENTRY *Next;
} RTL_HEAP_USAGE_ENTRY, *PRTL_HEAP_USAGE_ENTRY;

typedef struct _RTL_HEAP_USAGE {
    ULONG Length;
    ULONG BytesAllocated;
    ULONG BytesCommitted;
    ULONG BytesReserved;
    ULONG BytesReservedMaximum;
    PRTL_HEAP_USAGE_ENTRY Entries;
    PRTL_HEAP_USAGE_ENTRY AddedEntries;
    PRTL_HEAP_USAGE_ENTRY RemovedEntries;
    UCHAR Reserved[32];
} RTL_HEAP_USAGE, *PRTL_HEAP_USAGE;

typedef struct _RTL_HEAP_INFORMATION {
    PVOID BaseAddress;
    ULONG Flags;
    USHORT EntryOverhead;
    USHORT CreatorBackTraceIndex;
    ULONG BytesAllocated;
    ULONG BytesCommitted;
    ULONG NumberOfTags;
    ULONG NumberOfEntries;
    ULONG NumberOfPseudoTags;
    ULONG PseudoTagGranularity;
    ULONG Reserved[4];
    PVOID Tags;
    PVOID Entries;
} RTL_HEAP_INFORMATION, *PRTL_HEAP_INFORMATION;

typedef struct _RTL_PROCESS_HEAPS {
    ULONG NumberOfHeaps;
    RTL_HEAP_INFORMATION Heaps[ANYSIZE_ARRAY];
} RTL_PROCESS_HEAPS, *PRTL_PROCESS_HEAPS;

typedef struct _RTL_PROCESS_LOCK_INFORMATION {
    PVOID Address;
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG OwnerThreadId;
    ULONG ActiveCount;
    ULONG ContentionCount;
    ULONG EntryCount;
    ULONG RecursionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
} RTL_PROCESS_LOCK_INFORMATION, *PRTL_PROCESS_LOCK_INFORMATION;

typedef struct _RTL_PROCESS_LOCKS {
    ULONG NumberOfLocks;
    RTL_PROCESS_LOCK_INFORMATION Locks[ANYSIZE_ARRAY];
} RTL_PROCESS_LOCKS, *PRTL_PROCESS_LOCKS;

typedef struct _RTL_PROCESS_BACKTRACE_INFORMATION {
    PVOID SymbolicBackTrace;
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[16];
} RTL_PROCESS_BACKTRACE_INFORMATION, *PRTL_PROCESS_BACKTRACE_INFORMATION;

typedef struct _RTL_PROCESS_BACKTRACES {
    ULONG CommittedMemory;
    ULONG ReservedMemory;
    ULONG NumberOfBackTraceLookups;
    ULONG NumberOfBackTraces;
    RTL_PROCESS_BACKTRACE_INFORMATION BackTraces[1];
} RTL_PROCESS_BACKTRACES, *PRTL_PROCESS_BACKTRACES;

typedef struct _RTL_PROCESS_VERIFIER_OPTIONS {
    ULONG SizeStruct;
    ULONG Option;
    UCHAR OptionData[ANYSIZE_ARRAY];
} RTL_PROCESS_VERIFIER_OPTIONS, *PRTL_PROCESS_VERIFIER_OPTIONS;

typedef struct _RTL_DEBUG_INFORMATION {
    HANDLE SectionHandleClient;
    PVOID ViewBaseClient;
    PVOID ViewBaseTarget;
    ULONG ViewBaseDelta;
    HANDLE EventPairClient;
    PVOID EventPairTarget;
    HANDLE TargetProcessId;
    HANDLE TargetThreadHandle;
    ULONG Flags;
    ULONG OffsetFree;
    ULONG CommitSize;
    ULONG ViewSize;
    union {
        PRTL_PROCESS_MODULES Modules;
        PRTL_PROCESS_MODULE_INFORMATION_EX ModulesEx;
    };
    PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_PROCESS_LOCKS Locks;
    HANDLE SpecificHeap;
    HANDLE TargetProcessHandle;
    RTL_PROCESS_VERIFIER_OPTIONS VerifierOptions;
    HANDLE ProcessHeap;
    HANDLE CriticalSectionHandle;
    HANDLE CriticalSectionOwnerThread;
    PVOID Reserved[4];
} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

//
// Process
//

typedef NTSTATUS 
(NTAPI *NTCREATEUSERPROCESS)(
    __out PHANDLE ProcessHandle,
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK ProcessDesiredAccess,
    __in ACCESS_MASK ThreadDesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ProcessObjectAttributes,
    __in_opt POBJECT_ATTRIBUTES ThreadObjectAttributes,
    __in ULONG ProcessFlags, 
    __in ULONG ThreadFlags,  
    __in_opt PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    __inout PVOID CreateInfo,
    __in_opt PVOID AttributeList
    );

typedef NTSTATUS
(NTAPI *NTCREATEPROCESSEX)(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ParentProcess,
    __in ULONG Flags,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort,
    __in ULONG JobMemberLevel
    );

typedef NTSTATUS 
(NTAPI *NTCREATEPROCESS)(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ParentProcess,
    __in BOOLEAN InheritObjectTable,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort
    );

typedef NTSTATUS 
(NTAPI *NTTERMINATEPROCESS)(
    __in_opt HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

typedef
NTSTATUS
(NTAPI *NTSUSPENDPROCESS)(
    __in HANDLE ProcessHandle
	);

typedef
NTSTATUS
(NTAPI *NTRESUMEPROCESS)(
    __in HANDLE ProcessHandle
	);

//
// Thread 
//

typedef NTSTATUS
(NTAPI *NTCREATETHREAD)(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ProcessHandle,
    __out PCLIENT_ID ClientId,
    __in PCONTEXT ThreadContext,
    __in PINITIAL_TEB InitialTeb,
    __in BOOLEAN CreateSuspended
	);

typedef NTSTATUS 
(NTAPI *NTCREATETHREADEX)(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ProcessHandle,
    __in PVOID StartRoutine,
    __in_opt PVOID Argument,
    __in ULONG CreateFlags, 
    __in_opt ULONG_PTR ZeroBits,
    __in_opt SIZE_T StackSize,
    __in_opt SIZE_T MaximumStackSize,
    __in_opt PVOID AttributeList
    );

typedef NTSTATUS 
(NTAPI *NTTERMINATETHREAD)(
    __in_opt HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

typedef NTSTATUS 
(NTAPI *RTLCREATEUSERTHREAD)(
    __in HANDLE Process,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in BOOLEAN CreateSuspended,
    __in ULONG ZeroBits,
    __in SIZE_T MaximumStackSize,
    __in SIZE_T CommittedStackSize,
    __in PTHREAD_START_ROUTINE StartAddress,
    __in PVOID Context,
    __out PHANDLE Thread,
    __out PCLIENT_ID ClientId
    );

typedef NTSTATUS 
(NTAPI *RTLEXITUSERTHREAD)(
    __in ULONG Status
	);

//
// Loader
//

typedef NTSTATUS 
(NTAPI *LDRLOADDLL)(
    __in_opt PWSTR DllPath,
    __in_opt PULONG DllCharacteristics,
    __in PUNICODE_STRING DllName,
    __out PVOID *DllHandle
    );

typedef NTSTATUS 
(NTAPI *LDRGETPROCEDUREADDRESS)(
    __in PVOID BaseAddress,
    __in PANSI_STRING Name,
    __in ULONG Ordinal,
    __out PVOID *ProcedureAddress
	);

typedef NTSTATUS 
(NTAPI *LDRLOCKLOADERLOCK)(
    __in ULONG Flags,
    __out PULONG Disposition ,
    __out PULONG Cookie 
	);

typedef NTSTATUS 
(NTAPI *LDRUNLOCKLOADERLOCK)(
    __in ULONG Flags,
    __in ULONG Cookie 
	);

//
// Section 
//

typedef NTSTATUS
(NTAPI *NTCREATESECTION)(
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PLARGE_INTEGER MaximumSize,
    __in ULONG SectionPageProtection,
    __in ULONG AllocationAttributes,
    __in_opt HANDLE FileHandle
    );

typedef NTSTATUS 
(NTAPI *NTMAPVIEWOFSECTION)(
    __in HANDLE SectionHandle,
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __in ULONG_PTR ZeroBits,
    __in SIZE_T CommitSize,
    __inout_opt PLARGE_INTEGER SectionOffset,
    __inout PSIZE_T ViewSize,
    __in SECTION_INHERIT InheritDisposition,
    __in ULONG AllocationType,
    __in ULONG Win32Protect
    );

typedef NTSTATUS
(NTAPI *NTUNMAPVIEWOFSECTION)(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress
    );

//
// Registry
//

typedef 
NTSTATUS 
(NTAPI *NTQUERYKEY)(
    __in HANDLE  KeyHandle,
    __in KEY_INFORMATION_CLASS  KeyInformationClass,
    __out PVOID  KeyInformation,
    __in ULONG  Length,
    __out PULONG  ResultLength
    );

//
// Query
//

typedef
NTSTATUS 
(NTAPI *NTQUERYSYSTEMINFORMATION)(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out PULONG ReturnLength
	);

typedef 
NTSTATUS
(NTAPI *NTQUERYINFORMATIONTHREAD)(
    __in HANDLE ThreadHandle,
    __in NT_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out PULONG ReturnLength
    );

typedef
NTSTATUS
(NTAPI *NTQUERYINFORMATIONPROCESS)(
    __in HANDLE ProcessHandle,
    __in NT_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out PULONG ReturnLength 
	);

typedef NTSTATUS 
(NTAPI *NTQUERYOBJECT)(
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __out_bcount_opt(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __out_opt PULONG ReturnLength
    );

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID    Pointer;
  };
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _FILE_INFORMATION_CLASS { 
  FileDirectoryInformation                 = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileObjectIdInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileQuotaInformation,
  FileReparsePointInformation,
  FileNetworkOpenInformation,
  FileAttributeTagInformation,
  FileTrackingInformation,
  FileIdBothDirectoryInformation,
  FileIdFullDirectoryInformation,
  FileValidDataLengthInformation,
  FileShortNameInformation,
  FileIoCompletionNotificationInformation,
  FileIoStatusBlockRangeInformation,
  FileIoPriorityHintInformation,
  FileSfioReserveInformation,
  FileSfioVolumeInformation,
  FileHardLinkInformation,
  FileProcessIdsUsingFileInformation,
  FileNormalizedNameInformation,
  FileNetworkPhysicalNameInformation,
  FileIdGlobalTxDirectoryInformation,
  FileIsRemoteDeviceInformation,
  FileAttributeCacheInformation,
  FileNumaNodeInformation,
  FileStandardLinkInformation,
  FileRemoteProtocolInformation,
  FileReplaceCompletionInformation,
  FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef NTSTATUS 
(NTAPI *NTQUERYINFORMATIONFILE)(
  _In_   HANDLE FileHandle,
  _Out_  PIO_STATUS_BLOCK IoStatusBlock,
  _Out_  PVOID FileInformation,
  _In_   ULONG Length,
  _In_   FILE_INFORMATION_CLASS FileInformationClass
);

//
// Debug
//

typedef NTSTATUS 
(NTAPI *RTLQUERYPROCESSDEBUGINFORMATION)(
    __in ULONG ProcessId,
    __in ULONG InformationClass,
	__in PRTL_DEBUG_INFORMATION Buffer 
    );

typedef PRTL_DEBUG_INFORMATION
(NTAPI *RTLCREATEQUERYDEBUGBUFFER)(
    __in ULONG Size,
    __in BOOLEAN EventPair
    );

typedef NTSTATUS
(NTAPI *RTLDESTROYQUERYDEBUGBUFFER)(
    __in PRTL_DEBUG_INFORMATION Buffer
    );

typedef USHORT 
(NTAPI *RTLCAPTURESTACKTRACE)(
	__in ULONG FramesToSkip,
	__in ULONG FramesToCapture,
	__out PVOID* BackTrace,
	__out PULONG BackTraceHash
	);

typedef BOOLEAN 
(NTAPI *RTLDISPATCHEXCEPTION)(
    __in PEXCEPTION_RECORD ExceptionRecord,
    __in PCONTEXT ContextRecord
    );

//
// String
//

typedef VOID 
(NTAPI *RTLINITUNICODESTRING)(
    __out PUNICODE_STRING  DestinationString,
    __in PCWSTR  SourceString
    );

typedef VOID 
(NTAPI *RTLFREEUNICODESTRING)(
    __in PUNICODE_STRING  UnicodeString
    );

typedef VOID 
(NTAPI *RTLINITANSISTRING)(
    __out PANSI_STRING  DestinationString,
    __in PSTR SourceString
    );

typedef VOID  
(NTAPI *RTLFREEANSISTRING)(
    __in PANSI_STRING AnsiString
    );

typedef NTSTATUS 
(NTAPI *RTLUNICODESTRINGTOANSISTRING)(
    __out PANSI_STRING  DestinationString,
    __in PUNICODE_STRING  SourceString,
    __in BOOLEAN  AllocateDestinationString
    );

typedef NTSTATUS
(NTAPI *RTLANSISTRINGTOUNICODESTRING)(
    __out PUNICODE_STRING  DestinationString,
    __in PANSI_STRING  SourceString,
    __in BOOLEAN  AllocateDestinationString
    );

//
// Miscellaneous
//

typedef ULONG 
(NTAPI *RTLGETCURRENTPROCESSORNUMBER)(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif