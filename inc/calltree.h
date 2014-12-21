//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#ifndef _CALL_TREE_H_
#define _CALL_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "apsbtr.h"
#include "apsprofile.h"
#include "apsport.h"
#include "apsqueue.h"
#include "aps.h"
#include "apscpu.h"

//
// Call Node structure
//

typedef struct _CALL_NODE {

    LIST_ENTRY ListEntry;

    PVOID Address;
    ULONG FunctionId;
    ULONG DllId;
	ULONG LineId;
    BOOLEAN Root;
    BOOLEAN Spare;

    union {

        struct {
            ULONG Inclusive;
            ULONG Exclusive;
        } Cpu;

        struct {
            ULONG Count;
            ULONG64 InclusiveBytes;
            ULONG64 ExclusiveBytes;
        } Mm;

    };

    LONG ChildCount;

    union {
        LONG Offset;
        PVOID Context;
    };

    LIST_ENTRY ChildListHead;

} CALL_NODE, *PCALL_NODE;

//
// Call Tree structure
//

typedef struct _CALL_TREE {

    BTR_PROFILE_TYPE Type;

    BOOLEAN Stream;
    BOOLEAN Parallel;
    USHORT MaximumDepth;

    ULONG Count;
    ULONG Inclusive;
    ULONG Exclusive;
	ULONG Number;

    ULONG64 InclusiveBytes;
    ULONG64 ExclusiveBytes;

    PCALL_NODE RootNode;
    LIST_ENTRY ListEntry;

} CALL_TREE, *PCALL_TREE;


//
// Call Graph structure
//

typedef struct _CALL_GRAPH {

    BTR_PROFILE_TYPE Type;
    USHORT MaximumDepth;
    ULONG Count;
	ULONG Skipped;
    ULONG Inclusive;
    ULONG Exclusive;
    
    ULONG64 InclusiveBytes;
    ULONG64 ExclusiveBytes;

    ULONG NumberOfTrees;
    LIST_ENTRY TreeListHead;

} CALL_GRAPH, *PCALL_GRAPH;


//
// Routines
//

PCALL_NODE
ApsAllocateCallNode(
    VOID
    );

VOID
ApsFreeCallNode(
    __in PCALL_NODE Node
    );

PCALL_NODE
ApsPcToCallNode(
    __in BTR_PROFILE_TYPE Type,
    __in PBTR_PC_ENTRY PcEntry,
    __in LONG Depth,
    __in LONG Count,
	__in ULONG64 Size,
    __in PBTR_FUNCTION_ENTRY FuncTable,
    __in PBTR_TEXT_TABLE LineTable
    );

VOID
ApsInitCallGraph(
    __in BTR_PROFILE_TYPE Type,
    __out PCALL_GRAPH *CallGraph
    );

PCALL_TREE
ApsCreateCallTree(
    __in BTR_PROFILE_TYPE Type,
    __in BOOLEAN Parallel
    );

PCALL_TREE
ApsAllocateCallTree(
    __in BTR_PROFILE_TYPE Type,
    __in PCALL_GRAPH Graph,
    __in PCALL_NODE Node,
	__in ULONG Number
    );

//
// Call Tree Merging Algorithm
//

VOID
ApsCreateCallGraph(
    __out PCALL_GRAPH *CallGraph,
    __in BTR_PROFILE_TYPE Type,
    __in PBTR_STACK_RECORD Stack,
    __in LONG Count,
    __in PBTR_PC_TABLE PcTable,
    __in PBTR_FUNCTION_ENTRY FuncTable,
    __in PBTR_TEXT_TABLE TextTable
    );

PCALL_TREE
ApsFindCallTreeByRoot(
    __in PCALL_GRAPH Graph,
    __in PCALL_NODE Root,
    __in BOOLEAN Insert
    );

PCALL_NODE
ApsInsertCallNode(
    __in BTR_PROFILE_TYPE Type,
    __in PCALL_TREE Tree,
    __in PCALL_NODE Parent,
    __in PCALL_NODE Node,
    __in ULONG Depth
    );

PCALL_TREE
ApsSortCallGraph(
    __in PCALL_GRAPH Graph
    );

VOID
ApsSortCallTreeDescendant(
    __in BTR_PROFILE_TYPE Type,
    __in PCALL_NODE Node
    );

VOID
ApsDestroyCallGraph(
    __in PCALL_GRAPH Graph
    );

VOID
ApsDestroyCallTree(
    __in PCALL_TREE Tree
    );

VOID
ApsDestroyCallNode(
    __in PCALL_NODE Node
    );

VOID
ApsCreateCallGraphCpuPerThread(
    __out PCALL_GRAPH *CallGraph,
    __in PCPU_THREAD Thread,
    __in PBTR_STACK_RECORD Stack,
    __in PBTR_PC_TABLE PcTable,
    __in PBTR_FUNCTION_ENTRY FuncTable,
    __in PBTR_TEXT_TABLE TextTable
    );

#ifdef __cplusplus
}
#endif

#endif