//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _STREAM_H_
#define _STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "mmprof.h"

ULONG
BtrWriteHeapListStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteHeapStream(
	IN PMM_HEAP_TABLE Table,
	IN HANDLE FileHandle,
	OUT PULONG Length
	);

ULONG
BtrWritePageStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteHandleStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteGdiStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteDllStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteDllStreamEx(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteMinidumpStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteStackEntryStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteStackRecordStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteStackFileStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteCpuThreadStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteCpuAddressStream(
	IN PPF_REPORT_HEAD Head,
	IN HANDLE FileHandle,
	IN LARGE_INTEGER Start,
	OUT PLARGE_INTEGER End
	);

ULONG
BtrWriteProfileStream(
	__in HANDLE Handle
	);

VOID
BtrComputeStreamCheckSum(
	IN PPF_REPORT_HEAD Head
	);

ULONG
BtrCopyCodeViewRecord(
	__in PVOID Base,
	__out PCV_INFO_PDB70 CvRecord,
	__out PULONG NameLength
	);

ULONG
BtrCreateCvRecord(
	__in ULONG_PTR Base,
	__in ULONG Size,
	__in ULONG Timestamp,
	__out PBTR_CV_RECORD CvRecord
	);

ULONG
BtrWriteIoStream(
	_In_ PPF_REPORT_HEAD Head,
	_In_ HANDLE FileHandle,
	_In_ LARGE_INTEGER Start,
	_In_ PLARGE_INTEGER End
	);

ULONG
BtrWriteCcrStream(
	_In_ PPF_REPORT_HEAD Head,
	_In_ HANDLE FileHandle,
	_In_ LARGE_INTEGER Start,
	_In_ PLARGE_INTEGER End
	);

#ifdef __cplusplus
}
#endif
#endif
