//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _PDB_H_
#define _PDB_H_

#include "apsbtr.h"
#include "apsprofile.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "aps.h"
#include <rpc.h>
#include "list.h"
#include "bitmap.h"

//
// PDB Types
//

typedef enum _PDB_TYPE {
	PDB_MSFT, 
	PDB_MAP,   
	PDB_DWARF,
} PDB_TYPE;

//
// PDB Entry 
//

typedef struct _PDB_ENTRY {
	PVOID Address;
	ULONG NameId;
	ULONG Type    : 4;
	ULONG Private : 1;
	ULONG Offset  : 27;
	ULONG NameOffset;
	ULONG LineId;
	ULONG SourceId;
} PDB_ENTRY, *PPDB_ENTRY;

//
// PDB Module
//

typedef struct _PDB_MODULE {

	union {
		BTR_DLL_ENTRY Msft;
		BTR_DLL_ENTRY Map;
		BTR_DLL_ENTRY Dwarf;
	} u;

	PDB_TYPE Type;
	BOOLEAN Private;

	ULONG Count;
	PDB_ENTRY Entry[ANYSIZE_ARRAY];

} PDB_MODULE, *PPDB_MODULE;

//
// PDB Line Entry
//

typedef struct _PDB_LINE_ENTRY {
	ULONG LineId;
} PDB_LINE_ENTRY, *PPDB_LINE_ENTRY;

//
// PDB Source Entry
//

typedef struct _PDB_SOURCE_ENTRY {
	CHAR Name[MAX_PATH];
} PDB_SOURCE_ENTRY, *PPDB_SOURCE_ENTRY;

//
// PDB Stream Entry 
//

typedef struct _PDB_STREAM_ENTRY {
	ULONG Start;
	ULONG Size;
} PDB_STREAM_ENTRY, *PPDB_STREAM_ENTRY;

//
// PDB Stream File
//

#define PDB_FILE_HAS_SOURCE   0x00000001
#define PDB_FILE_COMPRESSED   0x00000002

typedef struct _PDB_STREAM_FILE {

	ULONG Size;
	ULONG Flag;
	
	//
	// Count = DllCount + Name + Line + Source
	//

	ULONG Count;  
	PDB_STREAM_ENTRY Entry[ANYSIZE_ARRAY];

} PDB_STREAM_FILE, *PPDB_STREAM_FILE;

//
// PDB Control Object
//

typedef struct _PDB_CONTROL {
	HANDLE MetaHandle;
	HANDLE NameHandle;
	PVOID MetaVa;
	PVOID NameVa;
	PWSTR MetaFile[MAX_PATH];
	PWSTR NameFile[MAX_PATH];
} PDB_CONTROL, *PPDB_CONTROL;

//
// SymEnumSourceFiles can be used to determine whether a .pdb file is private or public
//

ULONG
ApsCreatePdbStreamFile(
	__in PWSTR Path,
	__in PPF_REPORT_HEAD Head,
	__out PPDB_STREAM_FILE *File
	);

#ifdef __cplusplus
}
#endif
#endif