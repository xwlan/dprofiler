//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2014
//

#include "aps.h"
#include "apslog.h"
#include "apsdefs.h"
#include <stdlib.h>

DECLSPEC_CACHEALIGN
APS_LOG ApsLogObject;

CHAR ApsLogName[] = "dprofile.log";
CHAR ApsLogHead[] = "# Time  PID  Level  Text";
ULONG ApsLogHeadSize = sizeof(ApsLogHead);

CHAR *LogLevelString[] = {
	"Success",
	"Warning",
	"Failure",
}; 

VOID
ApsWriteLogEntry(
	__in ULONG ProcessId,
	__in APS_LOG_LEVEL Level,
	__in PSTR Format,
	...
	)
{
	ULONG IsLogging;
	va_list Argument;
	PAPS_LOG_ENTRY Entry;
	CHAR Buffer[APS_LOG_TEXT_LIMIT];

	//
	// Check whether logging's enabled
	//

	IsLogging = InterlockedCompareExchange(&ApsLogObject.Flag, 0, 0);
	if (!IsLogging) {
		return;
	}

	va_start(Argument, Format);
	StringCchVPrintfA(Buffer, MAX_PATH, Format, Argument);
	va_end(Argument);

	Entry = (PAPS_LOG_ENTRY)ApsMalloc(sizeof(APS_LOG_ENTRY));
	Entry->ProcessId = ProcessId;
	Entry->Level = Level;

	GetLocalTime(&Entry->TimeStamp);
	StringCchCopyA(Entry->Text, APS_LOG_TEXT_LIMIT, Buffer);

	//
	// Queue record entry to logging queue, note that this routine can be
	// called concurrently, so logging object's lock must be acquired to
	// protect the log record queue
	//

	EnterCriticalSection(&ApsLogObject.Lock);
	InsertTailList(&ApsLogObject.ListHead, &Entry->ListEntry);
	ApsLogObject.ListDepth += 1;
	LeaveCriticalSection(&ApsLogObject.Lock);

	return;
}

ULONG
ApsCurrentLogListDepth(
	VOID
	)
{
	ULONG Depth;

	Depth = InterlockedCompareExchange(&ApsLogObject.ListDepth, 0, 0);
	return Depth;
}

ULONG CALLBACK
ApsLogProcedure(
	__in PVOID Context
	)
{	
	ULONG Status;
	ULONG TimerPeriod;
	HANDLE TimerHandle;
	LARGE_INTEGER DueTime;
	HANDLE Handles[2];

	UNREFERENCED_PARAMETER(Context);
	
	Status = APS_STATUS_OK;

	//
	// Create timer expire every 1 second
	//

	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	TimerPeriod = 1000;

	DueTime.QuadPart = -10000 * TimerPeriod;
	SetWaitableTimer(TimerHandle, &DueTime, TimerPeriod, 
		             NULL, NULL, FALSE);	

	Handles[0] = TimerHandle;
	Handles[1] = ApsLogObject.StopObject;

	while (TRUE) {

		Status = WaitForMultipleObjects(2, Handles, FALSE, INFINITE);
		if (Status == WAIT_OBJECT_0) {
			ApsFlushLogQueue();
		}
		else if (Status == WAIT_OBJECT_0 + 1) {
			Status = S_OK;
			break;
		}
		else {
			Status = APS_STATUS_ERROR;
			break;
		}
	}

	return Status;
}

ULONG
ApsFlushLogQueue(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PAPS_LOG_ENTRY LogEntry;
	ULONG IsLogging;
	ULONG Length;
	ULONG Complete;
	ULONG Count = 0;
	CHAR Buffer[APS_PAGESIZE];

	IsLogging = InterlockedCompareExchange(&ApsLogObject.Flag, 0, 0);
	if (!IsLogging) {
		return 0;
	}

	EnterCriticalSection(&ApsLogObject.Lock);

	if (IsListEmpty(&ApsLogObject.ListHead)) {
		LeaveCriticalSection(&ApsLogObject.Lock);
		return 0;
	}

	//
	// Hand over queued log entries to QueueListHead
	//

	while (IsListEmpty(&ApsLogObject.ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ApsLogObject.ListHead);
		LogEntry = CONTAINING_RECORD(ListEntry, APS_LOG_ENTRY, ListEntry);

		StringCchPrintfA(Buffer, APS_PAGESIZE, "\r\n%02d:%02d:%02d:%03d %04x %s %s", 
			    LogEntry->TimeStamp.wHour, LogEntry->TimeStamp.wMinute, 
				LogEntry->TimeStamp.wSecond, LogEntry->TimeStamp.wMilliseconds, 
				LogEntry->ProcessId, LogLevelString[LogEntry->Level], LogEntry->Text); 

		Length = (ULONG)strlen(Buffer);
		WriteFile(ApsLogObject.FileObject, Buffer, Length, &Complete, NULL);
		ApsFree(LogEntry);

		Count += 1;
	}

	ApsLogObject.ListDepth = 0;
	LeaveCriticalSection(&ApsLogObject.Lock);
	return Count;
}

ULONG
ApsInitializeLog(
	__in ULONG Flag	
	)
{
	SYSTEMTIME Time;
	HANDLE Handle;
	ULONG Complete;
	LARGE_INTEGER Size;

	RtlZeroMemory(&ApsLogObject, sizeof(ApsLogObject));

	//
	// If logging is not enabled, do nothing
	//

	if (!FlagOn(Flag, APS_FLAG_LOGGING)) {
		return APS_STATUS_OK;
	}

	ApsLogObject.StopObject = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!ApsLogObject.StopObject) {
		return APS_STATUS_ERROR;
	}

	InitializeCriticalSection(&ApsLogObject.Lock);
	InitializeListHead(&ApsLogObject.ListHead);
	ApsLogObject.ListDepth = 0;

	//
	// Create log file based on process information and time
	//

	GetLocalTime(&Time);
	StringCchCopyA(ApsLogObject.Path, MAX_PATH, ApsLogName);

	Handle = CreateFileA(ApsLogObject.Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
		                 NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE) {
		DeleteCriticalSection(&ApsLogObject.Lock);
		CloseHandle(ApsLogObject.StopObject);
		return APS_STATUS_CREATEFILE;
	}

	GetFileSizeEx(Handle, &Size);

	//
	// If it's a new file, add a format description 
	//

	if (Size.QuadPart == 0) {
		WriteFile(Handle, ApsLogHead, ApsLogHeadSize, &Complete, NULL);
	}
	else {

		//
		// Adjust file pointer to end of file
		//

		Size.QuadPart = 0;
		SetFilePointerEx(Handle, Size, NULL, FILE_END);
	}

	ApsLogObject.FileObject = Handle;
	ApsLogObject.Flag = 1;

	//
	// Create log worker thread
	//

	ApsLogObject.ThreadObject = CreateThread(NULL, 0, ApsLogProcedure, NULL, 0, NULL);
	return APS_STATUS_OK;
}

BOOLEAN
ApsStopLog(
	VOID
	)
{
	SetEvent(ApsLogObject.StopObject);
	WaitForSingleObject(ApsLogObject.ThreadObject, INFINITE);
	return TRUE;
}

ULONG
ApsUninitializeLog(
	VOID
	)
{
	if (!ApsLogObject.Flag) {
		return APS_STATUS_OK;
	}

	InterlockedExchange(&ApsLogObject.Flag, 0);

	ApsStopLog();

	CloseHandle(ApsLogObject.StopObject);
	ApsLogObject.StopObject = NULL;
	
	CloseHandle(ApsLogObject.ThreadObject);
	ApsLogObject.ThreadObject = NULL;

	CloseHandle(ApsLogObject.FileObject);
	ApsLogObject.FileObject = NULL;

	DeleteCriticalSection(&ApsLogObject.Lock);
	return APS_STATUS_OK;
}

VOID
ApsGetLogFilePath(
	__out PCHAR Buffer,
	__in ULONG Length
	)
{
	StringCchCopyA(Buffer, Length, ApsLogObject.Path);
}

BOOLEAN
ApsIsLoggingEnabled(
	VOID
	)
{
	return (BOOLEAN)FlagOn(ApsLogObject.Flag, APS_FLAG_LOGGING);
}