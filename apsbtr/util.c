//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "apsbtr.h"
#include "heap.h"
#include "hal.h"
#include "util.h"
#include <dbghelp.h>
#include <stdlib.h>

ULONG_PTR
BtrUlongPtrRoundDown(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	)
{
	return Value & ~(Align - 1);
}

ULONG
BtrUlongRoundDown(
	__in ULONG Value,
	__in ULONG Align
	)
{
	return Value & ~(Align - 1);
}

ULONG64
BtrUlong64RoundDown(
	__in ULONG64 Value,
	__in ULONG64 Align
	)
{
	return Value & ~(Align - 1);
}

ULONG_PTR
BtrUlongPtrRoundUp(
	__in ULONG_PTR Value,
	__in ULONG_PTR Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG
BtrUlongRoundUp(
	__in ULONG Value,
	__in ULONG Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG64
BtrUlong64RoundUp(
	__in ULONG64 Value,
	__in ULONG64 Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

BOOLEAN
BtrIsExecutableAddress(
	__in PVOID Address
	)
{
	SIZE_T Status;
	ULONG Mask;
	MEMORY_BASIC_INFORMATION Mbi = {0};

	Status = VirtualQuery(Address, &Mbi, sizeof(Mbi));
	if (!Status) {
		return FALSE;
	}

	Mask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

	//
	// The specific address must be committed, and execute page.
	//

	if (!FlagOn(Mbi.State, MEM_COMMIT) || !FlagOn(Mbi.Protect, Mask)) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
BtrIsValidAddress(
	__in PVOID Address
	)
{
	CHAR Byte;
	BOOLEAN Status = TRUE;

	__try {
		Byte = *(PCHAR)Address;	
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = FALSE;
	}

	return Status;
}

ULONG
BtrGetByteOffset(
	__in PVOID Address
	)
{
	return (ULONG)((ULONG_PTR)Address & (BtrPageSize - 1));
}

SIZE_T 
BtrUnicodeToAnsi(
	__in PWSTR Source, 
	__out PCHAR *Destine
	)
{
	int Length;
	int RequiredLength;
	PCHAR Buffer;

	Length = (int)wcslen(Source) + 1;
	RequiredLength = WideCharToMultiByte(CP_UTF8, 0, Source, Length, 0, 0, 0, 0);

	Buffer = (PCHAR)BtrMalloc(RequiredLength);
	Buffer[0] = 0;

	WideCharToMultiByte(CP_UTF8, 0, Source, Length, Buffer, RequiredLength, 0, 0);
	*Destine = Buffer;

	return Length;
}

VOID 
BtrAnsiToUnicode(
	__in PCHAR Source,
	__out PWCHAR *Destine
	)
{
	int Length;
	PWCHAR Buffer;

	Length = (int)strlen(Source) + 1;
	Buffer = (PWCHAR)BtrMalloc(Length * sizeof(WCHAR));
	Buffer[0] = L'0';

	MultiByteToWideChar(CP_UTF8, 0, Source, Length, Buffer, Length);
	*Destine = Buffer;
}

VOID 
DebugTrace(
	__in PSTR Format,
	...
	)
{

#ifdef _DEBUG

	va_list arg;
	char format[512];
	char buffer[512];
	
	va_start(arg, Format);

	__try {
		StringCchVPrintfA(format, 512, Format, arg);
		StringCchPrintfA(buffer, 512, "[apsbtr]: %s\n", format);
		OutputDebugStringA(buffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
	
	va_end(arg);

#endif

}

VOID 
DebugTrace2(
	__in PSTR Format,
	...
	)
{
	va_list arg;
	char format[512];
	char buffer[512];
	
	va_start(arg, Format);

	__try {
		StringCchVPrintfA(format, 512, Format, arg);
		StringCchPrintfA(buffer, 512, "[apsbtr]: %s\n", format);
		OutputDebugStringA(buffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

	}
	
	va_end(arg);
}

ULONG 
BtrCompareMemoryPointer(
	__in PVOID Source,
	__in PVOID Destine,
	__in ULONG NumberOfUlongPtr
	)
{
	ULONG Number;

	for(Number = 0; Number < NumberOfUlongPtr; Number += 1) {
		if (*(PULONG_PTR)Source != *(PULONG_PTR)Destine) {
			return Number;
		}
	}

	return Number;
}

ULONG
BtrComputeAddressHash(
	__in PVOID Address,
	__in ULONG Limit
	)
{
	ULONG Hash;

	Hash = (ULONG)((ULONG_PTR)Address ^ (ULONG_PTR)GOLDEN_RATIO);
	Hash = (Hash >> 24) ^ (Hash >> 16) ^ (Hash >> 8) ^ (Hash);

	return Hash % Limit;
}

int
BtrPrintfW(
	__out wchar_t *buffer,
	__in size_t count,
	__in const wchar_t *format,
	... 
	)
{
	int len;
	va_list arg;
	
	va_start(arg, format);

	//
	// N.B. This routine ensure that the result string is
	// always null terminated whether or not it's truncated.
	//

	len = _vsnwprintf_s(buffer, count, count, format, arg);

	if (len > 0  &&  (size_t)len < count - 1) {
		return len;
	}

	if ((size_t)len == count - 1) {
		buffer[count - 1] = 0;
		return len;
	}

	if (len < 0) {
		return 0;
	}

	return 0;
}

int
BtrPrintfA(
	__out char *buffer,
	__in size_t count,
	__in const char *format,
	... 
	)
{
	int len;
	va_list arg;
	
	va_start(arg, format);

	//
	// N.B. This routine ensure that the result string is
	// always null terminated whether or not it's truncated.
	//

	len = _vsnprintf_s(buffer, count, count - 1, format, arg);

	if (len > 0  &&  (size_t)len < count - 1) {
		return len;
	}

	if ((size_t)len == count - 1) {
		buffer[count - 1] = 0;
		return len;
	}

	if (len < 0) {
		return 0;
	}

	return 0;
}

ULONG
BtrGetImageInformation(
	__in PVOID ImageBase,
	__out PULONG Timestamp,
	__out PULONG CheckSum
	)
{
	ULONG Status;
	PIMAGE_NT_HEADERS Headers;
	
	__try {

		Headers = ImageNtHeader(ImageBase);
		*Timestamp = Headers->FileHeader.TimeDateStamp;	
		*CheckSum = Headers->OptionalHeader.CheckSum;
		Status = S_OK;

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		*Timestamp = 0;
		*CheckSum = 0;
		Status = S_FALSE;

	}

	return Status;
}


#define BTR_MINIDUMP_FULL  MiniDumpWithFullMemory        | \
                           MiniDumpWithFullMemoryInfo    | \
						   MiniDumpWithHandleData        | \
						   MiniDumpWithThreadInfo        | \
						   MiniDumpWithUnloadedModules   |\
						   MiniDumpWithFullAuxiliaryState

#define BTR_MINIDUMP_DLL  (MiniDumpNormal | MiniDumpWithUnloadedModules)


ULONG
BtrWriteMinidump(
	__in PWSTR Path,
	__in BOOLEAN Full,
	__out PHANDLE DumpHandle 
	)
{
	HANDLE ProcessHandle;
	HANDLE FileHandle;
	ULONG ProcessId;
	MINIDUMP_TYPE Type;
	ULONG Status;

	*DumpHandle = NULL;

	ProcessId = GetCurrentProcessId();
	ProcessHandle = GetCurrentProcess();

	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
		                    FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	Type = Full ? (BTR_MINIDUMP_FULL) : BTR_MINIDUMP_DLL;
	Status = MiniDumpWriteDump(ProcessHandle, ProcessId, FileHandle, 
		                       Type, NULL, NULL, NULL);

	if (!Status) {
		Status = GetLastError();
		CloseHandle(FileHandle);
	} else {
		*DumpHandle = FileHandle;
		Status = S_OK;
	}

	return Status;
}

BOOLEAN
BtrIsExecutingRange(
	__in PLIST_ENTRY ListHead,
	__in PVOID Address,
	__in ULONG Length
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;

	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)

		if ((ULONG_PTR)Context->Registers.Eip >= (ULONG_PTR) Address && 
			(ULONG_PTR)Context->Registers.Eip < (ULONG_PTR) Address + (ULONG_PTR)Length) {
			return TRUE;
		}

#elif defined (_M_X64)

		if ((ULONG_PTR)Context->Registers.Rip >= (ULONG_PTR) Address && 
			(ULONG_PTR)Context->Registers.Rip < (ULONG_PTR) Address + (ULONG_PTR)Length) {
			return TRUE;
		}

#endif
		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

VOID
BtrZeroSmallMemory(
	__in PUCHAR Ptr,
	__in ULONG Size
	)
{
	ULONG i;

	for(i = 0; i < Size; i++) {
		Ptr[i] = 0;
	}
}

VOID
BtrCopySmallMemory(
	__in PUCHAR Destine,
	__in PUCHAR Source,
	__in ULONG Size
	)
{
	ULONG i;

	for(i = 0; i < Size; i++) {
		Destine[i] = Source[i];
	}
}

BOOLEAN
BtrValidateHandle(
	__in HANDLE Handle
	)
{
	ULONG Flag;

	return (BOOLEAN)GetHandleInformation(Handle, &Flag);
}

ULONG
BtrCreateRandomNumber(
    __in ULONG Minimum,
    __in ULONG Maximum
    )
{
    ULONG Result;
    
    Result = (ULONG)(((double)rand()/(double)RAND_MAX) * Maximum + Minimum);
    return Result;
}