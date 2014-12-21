//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012 
//

#include "status.h"
#include "apsbtr.h"
#include "apsprofile.h"
#include "mmstat.h"
#include "msputility.h"

DPF_HEAPLIST_TABLE DpfHeapListTable;
DPF_HANDLE_TABLE   DpfHandleTable;
DPF_PAGE_TABLE     DpfPageTable;
DPF_GDI_TABLE      DpfGdiTable;
DPF_DLL_TABLE      DpfDllTable;

#define HEAPLIST_BUCKET(_V) \
	((ULONG_PTR)_V % DPF_HEAPLIST_BUCKET)

#define HEAP_BUCKET(_V) \
	((ULONG_PTR)_V % DPF_HEAP_BUCKET)

#define HANDLE_BUCKET(_V) \
	((ULONG_PTR)_V % DPF_HANDLE_BUCKET)

#define PAGE_BUCKET(_V) \
	((ULONG_PTR)_V % DPF_PAGE_BUCKET)

#define GDI_BUCKET(_V) \
	((ULONG_PTR)_V % DPF_GDI_BUCKET)


ULONG
DpfHandleDuplicate(
	IN HANDLE_CALLBACK_TYPE Callback,
	IN HANDLE Value,
	IN HANDLE_TYPE Type
	);

ULONG
DpfHandleReAlloc(
	IN PDPF_HEAP_TABLE Table,
	IN HEAP_CALLBACK_TYPE Callback,
	IN HANDLE HeapHandle,
	IN PVOID Address
	);

//
// N.B. The best is to add a field in record
// to indicate callback's action is alloc or free,
// current implementation is a bit slow since, this
// will greatly increase CPU branch mispredication.
//

BOOLEAN FORCEINLINE
DpfIsHandleClose(
	IN ULONG Ordinal 
	)
{
	if (Ordinal == _CloseHandle ||
		Ordinal == _RegCloseKey ||
		Ordinal == _closesocket ||
		Ordinal == _FindClose) {
		return TRUE;
	}

	return FALSE;
}

BOOLEAN FORCEINLINE
DpfIsGdiClose(
	IN ULONG Ordinal
	)
{
	if (Ordinal == _DeleteObject      ||
		Ordinal == _DeleteDC          ||
		Ordinal == _DeleteEnhMetaFile ||
		Ordinal == _CloseEnhMetaFile) {

		return TRUE;
	}

	return FALSE;
}

BOOLEAN FORCEINLINE
DpfIsHeapFree(
	IN ULONG Ordinal
	)
{
	if (Ordinal >= _RtlFreeHeap  &&  Ordinal <= _SysFreeString) {    
		return TRUE;
	}

	return FALSE;
}

BOOLEAN FORCEINLINE
DpfIsPageFree(
	IN ULONG Ordinal
	)
{
	if (Ordinal == _VirtualFreeEx) {
		return TRUE;
	}

	return FALSE;
}

BOOLEAN FORCEINLINE
DpfIsHeapDestroy(
	IN ULONG Ordinal
	)
{
	if (Ordinal != _HeapDestroy && Ordinal != _RtlDestroyHeap) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN FORCEINLINE
DpfIsHeapCreate(
	IN ULONG Ordinal
	)
{
	if (Ordinal != _HeapCreate && Ordinal != _RtlCreateHeap) {
		return FALSE;
	}

	return TRUE;
}

ULONG
DpfInitMmTables(
	VOID
	)
{
	ULONG Number;

	for(Number = 0; Number < DPF_HEAPLIST_BUCKET; Number += 1) {
		InitializeListHead(&DpfHeapListTable.ActiveListHead[Number]);
	}

	InitializeListHead(&DpfHeapListTable.RetireListHead);

	for(Number = 0; Number < DPF_HANDLE_BUCKET; Number += 1) {
		InitializeListHead(&DpfHandleTable.ListHead[Number]);
	}
	
	for(Number = 0; Number < DPF_PAGE_BUCKET; Number += 1) {
		InitializeListHead(&DpfPageTable.ListHead[Number]);
	}

	for(Number = 0; Number < DPF_GDI_BUCKET; Number += 1) {
		InitializeListHead(&DpfGdiTable.ListHead[Number]);
	}

	return PF_STATUS_OK;
}

PDPF_HEAP_TABLE
DpfGetHeapTable(
	IN HANDLE HeapHandle
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PDPF_HEAP_TABLE Table;
	ULONG Number;

	Bucket = HEAPLIST_BUCKET(HeapHandle);
	ListHead = &DpfHeapListTable.ActiveListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Table = CONTAINING_RECORD(ListEntry, DPF_HEAP_TABLE, ListEntry);	
		if (Table->HeapHandle == HeapHandle) {
			return Table;
		}
		ListEntry = ListEntry->Flink;
	}

	//
	// Allocate new heap table object
	//

	Table = (PDPF_HEAP_TABLE)MspMalloc(sizeof(DPF_HEAP_TABLE));
	RtlZeroMemory(Table, sizeof(DPF_HEAP_TABLE));
	Table->HeapHandle = HeapHandle;

	for(Number = 0; Number < DPF_HEAP_BUCKET; Number += 1) {
		InitializeListHead(&Table->ListHead[Number]);
	}

	//
	// Insert heap list table
	//

	InsertHeadList(ListHead, &Table->ListEntry);
	return Table;
}

VOID
DpfHandleHeapDestroy(
	IN PDPF_HEAP_TABLE Table
	)
{
	ULONG Number;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PHEAP_RECORD Record;

	for(Number = 0; Number < DPF_HEAP_BUCKET; Number += 1) {
		
		//
		// Free all inserted heap records
		//

		ListHead = &Table->ListHead[Number];
		while (IsListEmpty(ListHead) != TRUE) {
			ListEntry = RemoveHeadList(ListHead);
			Record = CONTAINING_RECORD(ListEntry, HEAP_RECORD, ListEntry);
			MspFree(Record);
		}
	}

	RemoveEntryList(&Table->ListEntry);
	InsertHeadList(&DpfHeapListTable.RetireListHead, &Table->ListEntry);
}

ULONG
DpfInsertHeapRecord(
	IN PBTR_PROFILER_RECORD Record
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PHEAP_RECORD HeapRecord;
	PDPF_HEAP_TABLE Table;
	BOOLEAN IsFree;
	BOOLEAN IsDestroy;
	BOOLEAN IsCreate;
	PDPF_DLL_ENTRY Dll;

	//
	// Get heap table by record's heap handle, note that
	// for BSTR heap, its handle is -2, even if it's 
	// allocated from process heap, we don't rely on this,
	// we treat BSTR heap as an independant heap.
	//

	Table = DpfGetHeapTable(Record->Mm.Heap.HeapHandle);
	ASSERT(Table != NULL);

	//
	// Hash heap address to get its bucket in heap table
	//

	Bucket = HEAP_BUCKET(Record->Mm.Heap.Address);
	ListHead = &Table->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	//
	// It's heap create, track its caller
	//

	IsCreate = DpfIsHeapCreate(Record->Mm.Ordinal);
	if (IsCreate) {
		Table->CallerCreate = Record->Base.Caller;
		return PF_STATUS_OK;
	}

	//
	// It's heap destroy, if yes, clean all entries in heap table,
	// but keep its statistics data.
	//

	IsDestroy = DpfIsHeapDestroy(Record->Mm.Ordinal);
	if (IsDestroy) {
		Table->CallerDestroy = Record->Base.Caller;
		DpfHandleHeapDestroy(Table);
		return PF_STATUS_OK;
	}

	IsFree = DpfIsHeapFree(Record->Mm.Ordinal);
	Dll = DpfLookupDll(Record->Base.Caller);

	if (IsFree) {

		while (ListEntry != ListHead) {

			HeapRecord = CONTAINING_RECORD(ListEntry, HEAP_RECORD, ListEntry);
			if (HeapRecord->Address == Record->Mm.Heap.Address) {

				Table->NumberOfFrees += 1;
				Table->SizeOfFrees += HeapRecord->Size;

				//
				// N.B. Because the address is tracked, so Dll must be valid,
				// if it's null, bugcheck it
				//

				if (Dll != NULL) {
					DpfUpdateDllHeapFree(Dll, HeapRecord->HeapHandle, 
						                 HeapRecord->Address);
				}

				RemoveEntryList(&HeapRecord->ListEntry);
				MspFree(HeapRecord);
				break;
			}	

			ListEntry = ListEntry->Flink;
		}
		
		return PF_STATUS_OK;

	}

	//
	// Insert new value
	//

	HeapRecord = (PHEAP_RECORD)MspMalloc(sizeof(HEAP_RECORD));
	HeapRecord->HeapHandle = Record->Mm.Heap.HeapHandle;
	HeapRecord->Address = Record->Mm.Heap.Address;
	HeapRecord->Size = Record->Mm.Heap.Size;
	InsertHeadList(ListHead, &HeapRecord->ListEntry);

	Table->NumberOfAllocs += 1;
	Table->SizeOfAllocs += Record->Mm.Heap.Size;

	DpfHeapListTable.NumberOfAllocs += 1;
	DpfHeapListTable.SizeOfAllocs += Record->Mm.Heap.Size;

	if (Dll != NULL) {
		DpfUpdateDllHeapAlloc(Dll, HeapRecord);
	}

	//
	// Handle case of various ReAlloc
	//

	if (Record->Mm.Heap.OldAddr != NULL) {
		DpfHandleReAlloc(Table, Record->Mm.Ordinal, 
			             Record->Mm.Heap.HeapHandle,
						 Record->Mm.Heap.OldAddr);
		if (Dll != NULL) {
			DpfUpdateDllHeapFree(Dll, Record->Mm.Heap.HeapHandle,
							     Record->Mm.Heap.OldAddr);
		}
	}

	return PF_STATUS_OK;
}

ULONG
DpfHandleReAlloc(
	IN PDPF_HEAP_TABLE Table,
	IN HEAP_CALLBACK_TYPE Callback,
	IN HANDLE HeapHandle,
	IN PVOID Address
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PHEAP_RECORD HeapRecord;

	ASSERT(Table != NULL);

	Bucket = HEAP_BUCKET(Address);
	ListHead = &Table->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		HeapRecord = CONTAINING_RECORD(ListEntry, HEAP_RECORD, ListEntry);
		if (HeapRecord->Address == Address) {
			
			Table->NumberOfFrees += 1;
			Table->SizeOfFrees += HeapRecord->Size;

			DpfHeapListTable.NumberOfFrees += 1;
			DpfHeapListTable.SizeOfFrees += HeapRecord->Size;

			RemoveEntryList(&HeapRecord->ListEntry);
			MspFree(HeapRecord);

			break;
		}	

		ListEntry = ListEntry->Flink;
	}

	return PF_STATUS_OK;
}

ULONG
DpfInsertHandleRecord(
	IN PBTR_PROFILER_RECORD Record
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PHANDLE_RECORD HandleRecord;
	PDPF_HANDLE_TYPE_ENTRY TypeEntry;
	BOOLEAN IsClose;

	ASSERT(Record->Type == PROFILE_MM_TYPE);
	ASSERT(Record->Mm.Type == HandleCallbackType);

	Bucket = HANDLE_BUCKET(Record->Mm.Handle.Value);
	ListHead = &DpfHandleTable.ListHead[Bucket];
	ListEntry = ListHead->Flink;

	IsClose = DpfIsHandleClose(Record->Mm.Ordinal);

	while (ListEntry != ListHead) {

		HandleRecord = CONTAINING_RECORD(ListEntry, HANDLE_RECORD, ListEntry);

		//
		// If it's hit
		//

		if (HandleRecord->Value == Record->Mm.Handle.Value) {
			
			//
			// If it's close method
			//

			if (IsClose) {

				if (Record->Mm.Handle.Type >= HANDLE_PROCESS && Record->Mm.Handle.Type < HANDLE_LAST) {

					//
					// N.B. For CloseHandle(), we don't know handle type, 
					// so its type is -1, we may consider query handle type
					// when capture CloseHandle() record to improve type statistics.
					//

					TypeEntry = &DpfHandleTable.TypeEntry[Record->Mm.Handle.Type];
					TypeEntry->NumberOfFrees += 1;	
				}

				RemoveEntryList(&HandleRecord->ListEntry);
				MspFree(HandleRecord);

				DpfHandleTable.NumberOfFrees += 1;

			} else {

				//
				// if it's hit and it's not close, it means the same value is allocated,
				// this should not happen, we just ignore this case, the code path should
				// never reach here.
				//
			}

			break;
		}	

		ListEntry = ListEntry->Flink;
	}

	if (IsClose) {

		//
		// N.B. This is an close method, but the handle is not captured by dpf,
		// so we just ignore it.
		//

		return PF_STATUS_OK;
	}

	//
	// Insert new value
	//

	HandleRecord = (PHANDLE_RECORD)MspMalloc(sizeof(HANDLE_RECORD));
	HandleRecord->Value = Record->Mm.Handle.Value;
	HandleRecord->Type = Record->Mm.Handle.Type;

	//
	// We insert the new entry into list head, for well behaviored application,
	// (close after use), the handle will be immediately closed after usage,
	// so we expect a new close record will arrive soon, this can help to increase
	// first hit ratio.
	//

	InsertHeadList(ListHead, &HandleRecord->ListEntry);
	DpfHandleTable.NumberOfAllocs += 1;

	TypeEntry = &DpfHandleTable.TypeEntry[Record->Mm.Handle.Type];
	TypeEntry->NumberOfAllocs += 1;	

	//
	// 1, CreateProcessXxx return both process/thread handles
	// 2, DuplicateHandle can indirectly call CloseHandle if DUPLICATE_CLOSE_SOURCE is on
	//

	if (Record->Mm.Handle.Duplicate != NULL) {
		DpfHandleDuplicate(Record->Mm.Ordinal, 
			               Record->Mm.Handle.Duplicate, 
						   Record->Mm.Handle.Type);
	}

	return PF_STATUS_OK;
}

ULONG
DpfHandleDuplicate(
	IN HANDLE_CALLBACK_TYPE Callback,
	IN HANDLE Value,
	IN HANDLE_TYPE Type
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PHANDLE_RECORD HandleRecord;
	PDPF_HANDLE_TYPE_ENTRY TypeEntry;

	Bucket = HANDLE_BUCKET(Value);
	ListHead = &DpfHandleTable.ListHead[Bucket];
	ListEntry = ListHead->Flink;

	//
	// N.B. This indicate DuplicateHandle(DUPLICATE_CLOSE_SOURCE)
	//

	if (Callback == _DuplicateHandle) {

		ASSERT(Value != NULL);

		while (ListEntry != ListHead) {

			HandleRecord = CONTAINING_RECORD(ListEntry, HANDLE_RECORD, ListEntry);

			//
			// If it's hit
			//

			if (HandleRecord->Value == Value) {

				if (Type >= HANDLE_PROCESS && Type < HANDLE_LAST) {
					TypeEntry = &DpfHandleTable.TypeEntry[Type];
					TypeEntry->NumberOfFrees += 1;	
				}

				RemoveEntryList(&HandleRecord->ListEntry);
				MspFree(HandleRecord);

				DpfHandleTable.NumberOfFrees += 1;
				break;

			}

			ListEntry = ListEntry->Flink;
		}
	}

	//
	// CreateProcess variants store thread handle in Duplicate field,
	// we need explicitly insert thread handle into handle table
	//

	else if (Callback >= _CreateProcessA && Callback <= _CreateProcessWithLogonW) {

		HandleRecord = (PHANDLE_RECORD)MspMalloc(sizeof(HANDLE_RECORD));
		HandleRecord->Value = Value;
		HandleRecord->Type = HANDLE_THREAD;

		InsertHeadList(ListHead, &HandleRecord->ListEntry);
		DpfHandleTable.NumberOfAllocs += 1;

		TypeEntry = &DpfHandleTable.TypeEntry[HANDLE_THREAD];
		TypeEntry->NumberOfAllocs += 1;	

	}

	//
	// N.B. This is unexpected, you need check btr's callback implementation,
	// ensure any callback use Duplicate field is aligned with this routine.
	//

	else {
	
	}

	return PF_STATUS_OK;
}
	
ULONG
DpfInsertPageRecord(
	IN PBTR_PROFILER_RECORD Record
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PPAGE_RECORD PageRecord;
	BOOLEAN IsFree;

	ASSERT(Record->Type == PROFILE_MM_TYPE);
	ASSERT(Record->Mm.Type == PageCallbackType);

	Bucket = PAGE_BUCKET(Record->Mm.Page.Address);
	ListHead = &DpfPageTable.ListHead[Bucket];
	ListEntry = ListHead->Flink;
	IsFree = DpfIsPageFree(Record->Mm.Ordinal);

	while (ListEntry != ListHead) {

		PageRecord = CONTAINING_RECORD(ListEntry, PAGE_RECORD, ListEntry);

		//
		// If it's hit, for paged memory, it's possible free operation is partially applied,
		// e.g. allocate a big chunk, and free a small part, currently we blindly match
		// the base address, this handle only most simple case, we need improve the algorithm
		//

		if ((ULONG_PTR)Record->Mm.Page.Address == (ULONG_PTR)PageRecord->Address) {
			
			//
			// If it's free method
			//

			if (IsFree) {

				DpfPageTable.NumberOfFrees += 1;
				DpfPageTable.SizeOfFrees += PageRecord->Size;
				
				RemoveEntryList(&PageRecord->ListEntry);
				MspFree(PageRecord);


			} else {

				//
				// if it's hit and it's not close, it means the same value is allocated,
				// this should not happen, we just ignore this case, the code path should
				// never reach here.
				//
			}

			break;
		}	

		ListEntry = ListEntry->Flink;
	}
	
	if (IsFree) {

		//
		// N.B. This is an close method, but the handle is not captured by dpf,
		// so we just ignore it, or for VirtualFree case, user can free a middle
		// address in the whole allocation range, we just ignore it, because we
		// only care about the base address of address range, if it's never freed,
		// then it's a leak, but we lost its size, we can however, compare 2 snapshot
		// when profiling stops, this can give size difference by VirtualQuery.
		//

		return PF_STATUS_OK;
	}

	//
	// Insert new allocation 
	//

	PageRecord = (PPAGE_RECORD)MspMalloc(sizeof(PAGE_RECORD));
	PageRecord->Address = Record->Mm.Page.Address;
	PageRecord->Size = Record->Mm.Page.Size;

	//
	// We insert the new entry into list head, for well behaviored application,
	// (close after use), the handle will be immediately closed after usage,
	// so we expect a new close record will arrive soon, this can help to increase
	// first hit ratio.
	//

	InsertHeadList(ListHead, &PageRecord->ListEntry);
	DpfPageTable.NumberOfAllocs += 1;
	DpfPageTable.SizeOfAllocs += PageRecord->Size;

	return PF_STATUS_OK;
}

ULONG
DpfInsertGdiRecord(
	IN PBTR_PROFILER_RECORD Record
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PGDI_RECORD GdiRecord;
	PDPF_HANDLE_TYPE_ENTRY TypeEntry;
	BOOLEAN IsClose;

	ASSERT(Record->Type == PROFILE_MM_TYPE);
	ASSERT(Record->Mm.Type == GdiCallbackType);

	Bucket = GDI_BUCKET(Record->Mm.Handle.Value);
	ListHead = &DpfGdiTable.ListHead[Bucket];
	ListEntry = ListHead->Flink;
	
	IsClose = DpfIsGdiClose(Record->Mm.Ordinal);

	if (IsClose) {

		while (ListEntry != ListHead) {

			GdiRecord = CONTAINING_RECORD(ListEntry, GDI_RECORD, ListEntry);

			if (GdiRecord->Value == Record->Mm.Gdi.Value) {

				if (Record->Mm.Gdi.Type >= OBJ_PEN && Record->Mm.Gdi.Type < GDI_LAST) {

					//
					// N.B. For DeleteObject(), we don't know handle type, 
					// so its type is -1, we may consider query handle type
					// useing GetObjectType() to determine object type
					//

					TypeEntry = &DpfGdiTable.TypeEntry[Record->Mm.Gdi.Type];
					TypeEntry->NumberOfFrees += 1;	
				}

				RemoveEntryList(&GdiRecord->ListEntry);
				MspFree(GdiRecord);

				DpfGdiTable.NumberOfFrees += 1;
				break;
			}	

			ListEntry = ListEntry->Flink;
		}

		return PF_STATUS_OK;

	}

	GdiRecord = (PGDI_RECORD)MspMalloc(sizeof(GDI_RECORD));
	GdiRecord->Value = Record->Mm.Gdi.Value;
	GdiRecord->Type = Record->Mm.Gdi.Type;

	InsertHeadList(ListHead, &GdiRecord->ListEntry);
	DpfGdiTable.NumberOfAllocs += 1;

	if (Record->Mm.Gdi.Type >= OBJ_PEN && Record->Mm.Gdi.Type < GDI_LAST) {
		TypeEntry = &DpfGdiTable.TypeEntry[Record->Mm.Gdi.Type];
		TypeEntry->NumberOfAllocs += 1;	
	}

	return PF_STATUS_OK;
}

ULONG
DpfBuildNameTable(
	IN HANDLE MappedObject,
	IN PVOID MappedVa,
	IN ULONG Size,
	IN ULONG NumberOfFixed,
	IN ULONG NumberOfChained
	)
{
	return 0;
}

ULONG
DpfComputeAddressHash(
	IN PVOID Address,
	IN ULONG Limit
	)
{
	ULONG Hash;
	ULONG_PTR Value;

	//
	// Mask off its lower 16 bits, because dll are always mapped
	// with 64KB aligned
	//

	Value = (ULONG_PTR)Address >> 16;

	Hash = (ULONG)((ULONG_PTR)Value ^ (ULONG_PTR)GOLDEN_RATIO);
	Hash = (Hash >> 24) ^ (Hash >> 16) ^ (Hash >> 8) ^ (Hash);

	return Hash % Limit;
}

ULONG
DpfInitDllTable(
	IN ULONG ProcessId
	)
{
	LIST_ENTRY ListHead;
	PDPF_DLL_ENTRY Entry;
	PLIST_ENTRY ListEntry;
	PBSP_MODULE Module;
	ULONG Number;
	ULONG Bucket;
	ULONG Status;
	HANDLE Handle;

	Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!Handle) {
		Status = GetLastError();
		return Status;
	}

	InitializeListHead(&ListHead);
	Status = BspQueryModule(ProcessId, FALSE, &ListHead);
	if (Status != ERROR_SUCCESS) {
		CloseHandle(Handle);
		return Status;
	}

	DpfDllTable.ProcessId = ProcessId;
	DpfDllTable.ProcessHandle = Handle;
	
	for(Number = 0; Number < DPF_DLL_BUCKET; Number += 1) {
		InitializeListHead(&DpfDllTable.ListHead[Number]);
	}

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);
		
		//
		// Get base, size, path and linker time of dll entry
		//

		Entry = (PDPF_DLL_ENTRY)MspMalloc(sizeof(DPF_DLL_ENTRY));
		ZeroMemory(Entry, sizeof(DPF_DLL_ENTRY));

		Entry->BaseVa = Module->BaseVa;
		Entry->Size = Module->Size;

		//
		// Convert string to be lower case
		//

		wcslwr(Module->FullPath);
		wcscpy_s(Entry->Path, MAX_PATH, Module->FullPath);

		BspGetModuleTimeStamp(Module, &Entry->LinkerTime);

		Entry->Flag = DPF_DLL_LOADED;
		QueryPerformanceCounter(&Entry->LoadTime);

		//
		// Hash the dll base address and insert into its slot
		//

		Bucket = DpfComputeAddressHash(Entry->BaseVa, DPF_DLL_BUCKET);
		InsertTailList(&DpfDllTable.ListHead[Bucket], &Entry->ListEntry);

		BspFreeModule(Module);
	}

	return PF_STATUS_OK;
}

PDPF_DLL_ENTRY
DpfLookupDll(
	IN PVOID Pc
	)
{
	PDPF_DLL_ENTRY Entry;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	ULONG Bucket;
	PVOID BaseVa;

	BaseVa = (PVOID)SymGetModuleBase64(DpfDllTable.ProcessHandle, (DWORD64)Pc);
	if (!BaseVa) {
		return NULL;
	}

	Bucket = DpfComputeAddressHash(BaseVa, DPF_DLL_BUCKET);

	ListHead = &DpfDllTable.ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, DPF_DLL_ENTRY, ListEntry);
		if (Entry->BaseVa == BaseVa) {
			return Entry;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PDPF_DLL_ENTRY
DpfInsertDll(
	IN PVOID Pc 
	)
{
	PDPF_DLL_ENTRY Entry;
	ULONG Bucket;
	ULONG Status;
	MODULEINFO Info;
	PVOID BaseVa;
	
	//
	// N.B. We expect that the specified Pc is not available in 
	// Pc database, the profiler work loop should always first 
	// lookup pc database to check whether it's tracked one.
	//

	BaseVa = (PVOID)SymGetModuleBase64(DpfDllTable.ProcessHandle, (DWORD64)Pc);
	if (!BaseVa) {
		return NULL;
	}

	Entry = DpfLookupDll(BaseVa);
	if (Entry != NULL) {
		return Entry;
	}

	Status = GetModuleInformation(DpfDllTable.ProcessHandle, BaseVa, 
		                          &Info, sizeof(MODULEINFO));
	if (!Status) {
		return NULL;
	}

	Entry = (PDPF_DLL_ENTRY)MspMalloc(sizeof(DPF_DLL_ENTRY));
	ZeroMemory(Entry, sizeof(DPF_DLL_ENTRY));
	Entry->BaseVa = Info.lpBaseOfDll;
	Entry->Size = Info.SizeOfImage;
	QueryPerformanceCounter(&Entry->LoadTime);

	GetModuleFileNameEx(DpfDllTable.ProcessHandle, Info.lpBaseOfDll, 
		                Entry->Path, MAX_PATH);

	Bucket = DpfComputeAddressHash(Info.lpBaseOfDll, DPF_DLL_BUCKET);
	InsertTailList(&DpfDllTable.ListHead[Bucket], &Entry->ListEntry);

	return Entry;
}
	
ULONG
DpfUpdateDllHeapAlloc(
	IN PDPF_DLL_ENTRY Dll,
	IN PHEAP_RECORD Record
	)
{

	return 0;
}

ULONG
DpfUpdateDllHeapFree(
	IN PDPF_DLL_ENTRY Dll,
	IN HANDLE HeapHandle,
	IN PVOID Address
	)
{
	return 0;
}

ULONG
DpfUpdatePcHeapAlloc(
	IN PDPF_PC_ENTRY Dll,
	IN PHEAP_RECORD Record
	)
{
	return 0;
}

ULONG
DpfUpdatePcHeapFree(
	IN PDPF_PC_ENTRY Dll,
	IN PHEAP_RECORD Record
	)
{
	return 0;
}

ULONG CALLBACK
DpfCountProcedure(
	IN PVOID Context
	)
{
	return 0;
}