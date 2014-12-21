//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "apsdefs.h"
#include "apspri.h"
#include "apsctl.h"
#include "apsbtr.h"
#include "aps.h"
#include "apsfile.h"
#include "aps.h"
#include "ntapi.h"
#include "apspdb.h"
#include "apsrpt.h"
#include "apscpu.h"
#include "cpuprof.h"
#include "calltree.h"


PCALL_TREE
ApsCreateCallTree(
    __in BTR_PROFILE_TYPE Type,
    __in BOOLEAN Parallel 
    )
{
    PCALL_TREE Tree;

    Tree = (PCALL_TREE)ApsMalloc(sizeof(CALL_TREE));

    Tree->Type = Type;
    Tree->Parallel = Parallel;
    Tree->Stream = FALSE;
    Tree->MaximumDepth = 0;
    Tree->Count = 0; 
    Tree->Inclusive = 0;
    Tree->Exclusive = 0;
    Tree->InclusiveBytes = 0;
    Tree->ExclusiveBytes = 0;

    return Tree;
}

PCALL_NODE
ApsLookupCallNodeByPc(
    __in PCALL_NODE Node,
    __in PVOID Pc
    )
{
    return NULL;
}

VOID
ApsInitCallGraph(
    __in BTR_PROFILE_TYPE Type,
    __out PCALL_GRAPH *CallGraph
    )
{
    PCALL_GRAPH Graph;

    Graph = (PCALL_GRAPH)ApsMalloc(sizeof(CALL_GRAPH));
    RtlZeroMemory(Graph, sizeof(*Graph));

    Graph->Type = Type;
    Graph->MaximumDepth = 0;
    InitializeListHead(&Graph->TreeListHead);

    *CallGraph = Graph;
}

PCALL_NODE
ApsAllocateCallNode(
    VOID
    )
{
    PCALL_NODE Node;

    Node = (PCALL_NODE)ApsMalloc(sizeof(CALL_NODE));
    InitializeListHead(&Node->ChildListHead);

    return Node;
}

VOID
ApsFreeCallNode(
    __in PCALL_NODE Node
    )
{
    ASSERT(Node != NULL);
    ApsFree(Node);
}

PCALL_NODE
ApsPcToCallNode(
    __in BTR_PROFILE_TYPE Type,
    __in PBTR_PC_ENTRY PcEntry,
    __in LONG Depth,
    __in LONG Count,
	__in ULONG64 Size,
    __in PBTR_FUNCTION_ENTRY FuncTable,
    __in PBTR_TEXT_TABLE TextTable
    )
{
    PCALL_NODE Node;
    PBTR_FUNCTION_ENTRY FuncEntry;
	PBTR_TEXT_ENTRY TextEntry;
	
    Node = ApsAllocateCallNode();
    Node->FunctionId = PcEntry->FunctionId;
    Node->DllId = PcEntry->DllId;

    if (PcEntry->FunctionId != -1) {
        FuncEntry = (PBTR_FUNCTION_ENTRY)&FuncTable[PcEntry->FunctionId];
		Node->Address = (PVOID)FuncEntry->Address;
    } else {
        Node->Address = PcEntry->Address;
    }
    
	//
	// Query line id by text table
	//

	TextEntry = ApsLookupSymbol(TextTable, (ULONG64)Node->Address);
	if (TextEntry != NULL && TextEntry->LineId != -1) {
		Node->LineId = TextEntry->LineId;
	} else {
		Node->LineId = -1;
	}

    if (Depth != 0) {

        if (Type == PROFILE_CPU_TYPE) {
            Node->Cpu.Inclusive = Count;
            Node->Cpu.Exclusive = 0;
        }
        if (Type == PROFILE_MM_TYPE) {
            //Node->Mm.InclusiveBytes = PcEntry->SizeOfAllocs;
            Node->Mm.InclusiveBytes = Size;
            Node->Mm.ExclusiveBytes = 0;
        }

    } else {

        if (Type == PROFILE_CPU_TYPE) {
            Node->Cpu.Inclusive = Count;
            Node->Cpu.Exclusive = Count;
        }
        
        if (Type == PROFILE_MM_TYPE) {
            //Node->Mm.InclusiveBytes = PcEntry->SizeOfAllocs;
            //Node->Mm.ExclusiveBytes = PcEntry->SizeOfAllocs;
            Node->Mm.InclusiveBytes = Size;
            Node->Mm.ExclusiveBytes = Size;
            Node->Mm.Count = PcEntry->Count;
        }
    }

    return Node;
}

PCALL_TREE
ApsAllocateCallTree(
    __in BTR_PROFILE_TYPE Type,
    __in PCALL_GRAPH Graph,
    __in PCALL_NODE Node,
	__in ULONG Number
    )
{
    PCALL_TREE Tree;

    Tree = (PCALL_TREE)ApsMalloc(sizeof(CALL_TREE));
    Tree->Type = Type;
    Tree->Stream = FALSE;
    Tree->Parallel = FALSE;
    Tree->Count = 1;
    Tree->Inclusive = Node->Cpu.Inclusive;
    Tree->Exclusive = 0;
    Tree->InclusiveBytes = Node->Mm.InclusiveBytes;
    Tree->ExclusiveBytes = 0;
    Tree->RootNode = Node;

    //
    // Insert the tree into call graph
    //

    InsertHeadList(&Graph->TreeListHead, &Tree->ListEntry);
    Graph->NumberOfTrees += 1;

    return Tree;
}

BOOLEAN 
ApsFilterRecordForCallTree(
    __in PBTR_STACK_RECORD Record
    )
{
    //
    // Require the stack depth is at least 3 because ntdll!RtlUserThreadStart and
    // kernel32!BaseThreadInitThunk occupy the stack trace bottom for a complete
    // and accurate stack trace, so a useful stack trace has at least depth 3.
    //

    return (Record->Depth >= 3) ? TRUE : FALSE;
}

VOID
ApsCreateCallGraphCpuPerThread(
    __out PCALL_GRAPH *CallGraph,
    __in PCPU_THREAD Thread,
    __in PBTR_STACK_RECORD Stack,
    __in PBTR_PC_TABLE PcTable,
    __in PBTR_FUNCTION_ENTRY FuncTable,
    __in PBTR_TEXT_TABLE TextTable
    )
{
    PCALL_GRAPH Graph;
    ULONG Number;
    SHORT Depth;
    PBTR_PC_ENTRY PcEntry;
    PCALL_NODE Node;
    PCALL_NODE Parent;
    PCALL_TREE Tree;
    ULONG Skipped = 0;
    PBTR_STACK_RECORD Record;
	ULONG Ordinal = 0;
    PCPU_STACK_TABLE StackTable;
    PCPU_STACK_ENTRY StackEntry;

    //
    // Allocate call graph object
    //

    ApsInitCallGraph(PROFILE_CPU_TYPE, &Graph);

    StackTable = Thread->StackTable;
    ASSERT(StackTable != NULL);
    Record = Stack;

    for(Number = 0; Number < StackTable->Count; Number += 1) {

        StackEntry = &StackTable->Table[Number];
        Record = &Stack[StackEntry->StackId];

        //
        // Ensure the stack trace is a useful sample
        //

        if (!ApsFilterRecordForCallTree(Record)) {
            Skipped += StackEntry->Count;
            continue;
        }

        //
        // Create root node of the stack trace and find a matching tree in graph
        //

        ASSERT(Record->Committed == 1);
        ASSERT(Record->Depth <= MAX_STACK_DEPTH);

        PcEntry = ApsLookupPcEntry(Record->Frame[Record->Depth - 1], PcTable);
        ASSERT(PcEntry != NULL);

        Node = ApsPcToCallNode(PROFILE_CPU_TYPE, PcEntry, Record->Depth - 1, 
                               StackEntry->Count, 0, FuncTable, TextTable);
        Tree = ApsFindCallTreeByRoot(Graph, Node, FALSE);

        if (Tree) {

            //
            // Increase inclusive for root node
            //

            Tree->RootNode->Cpu.Inclusive += Node->Cpu.Inclusive;
            ApsFreeCallNode(Node);
        }
        else {

            //
            // Allocate a new call tree and insert the node as root node
            //

            Tree = ApsAllocateCallTree(PROFILE_CPU_TYPE, Graph, Node, Ordinal);
			Ordinal += 1;

			Tree->Number = Ordinal;
        }

        //
        // Track the maximum stack depth
        //

        Tree->MaximumDepth = (USHORT)max(Record->Depth, Tree->MaximumDepth);
        Graph->MaximumDepth = (USHORT)max(Tree->MaximumDepth, Graph->MaximumDepth);

        Node = Tree->RootNode;

        //
        // Translate the left stack frames into call nodes and insert into call tree 
        //

        for(Depth = (SHORT)Record->Depth - 2; Depth >= 0; Depth -= 1) {
            
            Parent = Node;

            PcEntry = ApsLookupPcEntry(Record->Frame[Depth], PcTable);
            ASSERT(PcEntry != NULL);

            Node = ApsPcToCallNode(PROFILE_CPU_TYPE, PcEntry, Depth, StackEntry->Count, 
				                   0, FuncTable, TextTable);
            Node = ApsInsertCallNode(PROFILE_CPU_TYPE, Tree, Parent, Node, Depth);
        }

        //
        // Update total counters
        //

        Graph->Count += StackEntry->Count;
        Graph->Inclusive += StackEntry->Count;
        Graph->Exclusive += StackEntry->Count;
    }

	Graph->Skipped = Skipped;

	//
	// Sort the call trees
	//

    ApsSortCallGraph(Graph);
    *CallGraph = Graph;
}

VOID
ApsCreateCallGraph(
    __out PCALL_GRAPH *CallGraph,
    __in BTR_PROFILE_TYPE Type,
    __in PBTR_STACK_RECORD Stack,
    __in LONG Count,
    __in PBTR_PC_TABLE PcTable,
    __in PBTR_FUNCTION_ENTRY FuncTable,
    __in PBTR_TEXT_TABLE TextTable
    )
{
    PCALL_GRAPH Graph;
    LONG Number;
    SHORT Depth;
    PBTR_PC_ENTRY PcEntry;
    PCALL_NODE Node;
    PCALL_NODE Parent;
    PCALL_TREE Tree;
    ULONG Skipped = 0;
    PBTR_STACK_RECORD Record;
	ULONG64 Size;
	ULONG Ordinal = 0;

    //
    // Allocate call graph object
    //

    ApsInitCallGraph(Type, &Graph);
    Record = Stack;

    for(Number = 0; Number < Count; Number += 1) {

        Record = &Stack[Number];

		if (Type == PROFILE_MM_TYPE && Record->SizeOfAllocs == 0) {

			//
			// If profile is MM type and there's no allocation size,
			// e.g. HANDLE/GDI etc, just skip the record.
			//

			continue;
		}
        
        //
        // Ensure the stack trace is a useful sample
        //

        if (!ApsFilterRecordForCallTree(Record)) {
			Skipped += Record->Count;
            continue;
        }

        //
        // Create root node of the stack trace and find a matching tree in graph
        //

        ASSERT(Record->Committed == 1);
        ASSERT(Record->Depth <= MAX_STACK_DEPTH);

		if (Type == PROFILE_MM_TYPE) {
			Size = Record->SizeOfAllocs;
		} 
		else if (Type == PROFILE_IO_TYPE) {
			Size = 0;
		}
		else {
			Size = 0;
		}

        PcEntry = ApsLookupPcEntry(Record->Frame[Record->Depth - 1], PcTable);
        ASSERT(PcEntry != NULL);

        Node = ApsPcToCallNode(Type, PcEntry, Record->Depth - 1, 
			                   Record->Count, Size, FuncTable, TextTable);
        Tree = ApsFindCallTreeByRoot(Graph, Node, FALSE);

        if (Tree) {

            //
            // Increase inclusive for root node
            //

            if (Type == PROFILE_CPU_TYPE) { 
                Tree->RootNode->Cpu.Inclusive += Node->Cpu.Inclusive;
            }
            if (Type == PROFILE_MM_TYPE) {
                Tree->RootNode->Mm.InclusiveBytes += Node->Mm.InclusiveBytes;
            }

            ApsFreeCallNode(Node);
        }
        else {

            //
            // Allocate a new call tree and insert the node as root node
            //

            Tree = ApsAllocateCallTree(Type, Graph, Node, Ordinal);
			Ordinal += 1;

			Tree->Number = Ordinal;
        }

        //
        // Track the maximum stack depth
        //

        Tree->MaximumDepth = (USHORT)max(Record->Depth, Tree->MaximumDepth);
        Graph->MaximumDepth = (USHORT)max(Tree->MaximumDepth, Graph->MaximumDepth);

        Node = Tree->RootNode;

        //
        // Translate the left stack frames into call nodes and insert into call tree 
        //

        for(Depth = (SHORT)Record->Depth - 2; Depth >= 0; Depth -= 1) {
            
            Parent = Node;

            PcEntry = ApsLookupPcEntry(Record->Frame[Depth], PcTable);
            ASSERT(PcEntry != NULL);

            Node = ApsPcToCallNode(Type, PcEntry, Depth, Record->Count, 
				                   Size, FuncTable, TextTable);
            Node = ApsInsertCallNode(Type, Tree, Parent, Node, Depth);
        }

        //
        // Update total counters
        //

        Graph->Count += Record->Count;
        Graph->Inclusive += Record->Count;
        Graph->Exclusive += Record->Count;
        Graph->InclusiveBytes += Size;
        Graph->ExclusiveBytes += Size;
    }

	Graph->Skipped = Skipped;

	//
	// Sort the call trees
	//

    ApsSortCallGraph(Graph);
    *CallGraph = Graph;
}

PCALL_TREE
ApsSortCallGraph(
    __in PCALL_GRAPH Graph
    )
{
    PCALL_TREE Tree;
    PCALL_TREE Tree2;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListEntry2;
    PLIST_ENTRY ListHead2;
    LIST_ENTRY TreeList;
    PLIST_ENTRY Blink;

    InitializeListHead(&TreeList);

    //
    // Switch the tree list into TreeList
    //

    TreeList.Flink = Graph->TreeListHead.Flink;
    TreeList.Blink = Graph->TreeListHead.Blink;
    Graph->TreeListHead.Flink->Blink = &TreeList;
    Graph->TreeListHead.Blink->Flink = &TreeList;

    InitializeListHead(&Graph->TreeListHead);

    //
    // Sort the tree list by insertion 
    //

    while (IsListEmpty(&TreeList) != TRUE) {
    
        ListEntry = RemoveHeadList(&TreeList);
        Tree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);
    
        //
        // Recursively sort the call tree nodes
        //

        ApsSortCallTreeDescendant(Graph->Type, Tree->RootNode);

        ListHead2 = &Graph->TreeListHead;
        ListEntry2 = ListHead2->Flink;

        while (ListEntry2 != ListHead2) {
            Tree2 = CONTAINING_RECORD(ListEntry2, CALL_TREE, ListEntry); 
            if (Graph->Type == PROFILE_CPU_TYPE) {
				if (Tree->RootNode->Cpu.Inclusive > Tree2->RootNode->Cpu.Inclusive) {
                    break;
                }
            }
            if (Graph->Type == PROFILE_MM_TYPE) {
				if (Tree->RootNode->Mm.InclusiveBytes > Tree2->RootNode->Mm.InclusiveBytes) {
                    break;
                }
            }
            ListEntry2 = ListEntry2->Flink;
        }

        //
        // Adjust backward link 
        //

        Blink = ListEntry2->Blink;
        ListEntry2->Blink = &Tree->ListEntry;
        Tree->ListEntry.Blink = Blink;

        //
        // Adjust forward link 
        //

        Blink->Flink = &Tree->ListEntry;
        Tree->ListEntry.Flink = ListEntry2;
    }

    //
    // Return the first tree after sort
    //

    ListEntry = Graph->TreeListHead.Flink;
    Tree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);
    return Tree;
}

VOID
ApsSortCallTreeDescendant(
    __in BTR_PROFILE_TYPE Type,
    __in PCALL_NODE Node
    )
{
    PCALL_NODE Node1;
    PCALL_NODE Node2;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListEntry2;
    LIST_ENTRY ChildList;
    PLIST_ENTRY Blink;
    PLIST_ENTRY ListHead;

    ListHead = &Node->ChildListHead;
    if (IsListEmpty(ListHead)) {
        return;
    }

    //
    // Switch the tree list into TreeList
    //

    ChildList.Flink = ListHead->Flink;
    ListHead->Flink->Blink = &ChildList;

    ChildList.Blink = ListHead->Blink;
    ListHead->Blink->Flink = &ChildList;

    InitializeListHead(ListHead);

    //
    // Sort the tree list by insertion 
    //

    while (IsListEmpty(&ChildList) != TRUE) {
    
        ListEntry = RemoveHeadList(&ChildList);
        Node1 = CONTAINING_RECORD(ListEntry, CALL_NODE, ListEntry);
    
        ApsSortCallTreeDescendant(Type, Node1);

        //
        // Walk the node's ChildListHead to do insertion sort
        //

        ListEntry2 = ListHead->Flink;

        while (ListEntry2 != ListHead) {
            Node2 = CONTAINING_RECORD(ListEntry2, CALL_NODE, ListEntry); 
            if (Type == PROFILE_CPU_TYPE) {
                if (Node1->Cpu.Inclusive > Node2->Cpu.Inclusive) {
                    break;
                }
            }
            if (Type == PROFILE_MM_TYPE) {
                if (Node1->Mm.InclusiveBytes > Node2->Mm.InclusiveBytes) {
                    break;
                }
            }
            ListEntry2 = ListEntry2->Flink;
        }

        //
        // Adjust backward link 
        //

        Blink = ListEntry2->Blink;
        ListEntry2->Blink = &Node1->ListEntry;
        Node1->ListEntry.Blink = Blink;

        //
        // Adjust forward link 
        //

        Blink->Flink = &Node1->ListEntry;
        Node1->ListEntry.Flink = ListEntry2;
    }
}

BOOLEAN
ApsIsSameCallNode(
    __in PCALL_NODE Node1,
    __in PCALL_NODE Node2
    )
{
    if ((Node1->FunctionId == Node2->FunctionId) && 
        (Node1->Address == Node2->Address)) {
        return TRUE;
    }

    return FALSE;
}

PCALL_TREE
ApsFindCallTreeByRoot(
    __in PCALL_GRAPH Graph,
    __in PCALL_NODE Root,
    __in BOOLEAN Insert
    )
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PCALL_TREE Tree;

    ListHead = &Graph->TreeListHead;
    ListEntry = ListHead->Flink;

    //
    // Scan the graph's tree list to find a matching call tree
    //

    while (ListEntry != ListHead) {
        Tree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);
        if (Tree->RootNode != NULL) {
            if (Tree->RootNode->Address == Root->Address) {
                return Tree;
            }
        }
        ListEntry = ListEntry->Flink;
    }

    return NULL;
}

PCALL_NODE
ApsInsertCallNode(
    __in BTR_PROFILE_TYPE Type,
    __in PCALL_TREE Tree,
    __in PCALL_NODE Parent,
    __in PCALL_NODE Node,
    __in ULONG Depth
    )
{
    PCALL_NODE Child;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;

    ListHead = &Parent->ChildListHead;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead) {

        Child = CONTAINING_RECORD(ListEntry, CALL_NODE, ListEntry);
        if (Child->Address == Node->Address) {
        
            //
            // N.B. Even if the node match the found child node, we need ensure
            // it's appropriate to merge them, if neither is leaf node, or both are
            // leaf node, if one is leaf node, one is non-leaf node, the nodes can
            // not be merged since they're in fact two different code path
            //

            if (Depth == 0 && IsListEmpty(&Child->ChildListHead)) { 
                if (Type == PROFILE_CPU_TYPE) {
                    Child->Cpu.Exclusive += Node->Cpu.Exclusive;
                }
                if (Type == PROFILE_MM_TYPE) {
                    Child->Mm.ExclusiveBytes += Node->Mm.ExclusiveBytes;
                }
                
                ApsFreeCallNode(Node);
                return Child;
            }

            if (Depth > 0 && !IsListEmpty(&Child->ChildListHead)) {
                if (Type == PROFILE_CPU_TYPE) {
                    Child->Cpu.Inclusive += Node->Cpu.Inclusive;
                }
                if (Type == PROFILE_MM_TYPE) {
                    Child->Mm.InclusiveBytes += Node->Mm.InclusiveBytes;
                }
                
                ApsFreeCallNode(Node);
                return Child;
            }

            //
            // N.B. Try to merge nodes in a relaxed condition, only require that
            // the compared two nodes have same address
            //

            if (Depth != 0) {
                if (Type == PROFILE_CPU_TYPE) {
                    Child->Cpu.Inclusive += Node->Cpu.Inclusive;
                }
                if (Type == PROFILE_MM_TYPE) {
                    Child->Mm.InclusiveBytes += Node->Mm.InclusiveBytes;
                }
                ApsFreeCallNode(Node);
                return Child;
            }

            if (Depth == 0) {
                if (Type == PROFILE_CPU_TYPE) {
                    Child->Cpu.Inclusive += Node->Cpu.Inclusive;
                    Child->Cpu.Exclusive += Node->Cpu.Exclusive;
                }
                if (Type == PROFILE_MM_TYPE) {
                    Child->Mm.InclusiveBytes += Node->Mm.InclusiveBytes;
                    Child->Mm.ExclusiveBytes += Node->Mm.ExclusiveBytes;
                }
                ApsFreeCallNode(Node);
                return Child;
            }

        }

        ListEntry = ListEntry->Flink;
    }

    //
    // The nodes can not be merged, insert the new child
    //

    InsertHeadList(ListHead, &Node->ListEntry); 
    Tree->Count += 1;

    return Node;
}

VOID
ApsDestroyCallGraph(
    __in PCALL_GRAPH Graph
    )
{
    PLIST_ENTRY ListEntry;
    PCALL_TREE Tree;

    while(IsListEmpty(&Graph->TreeListHead) != TRUE) {
        ListEntry = RemoveHeadList(&Graph->TreeListHead);
        Tree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);
        ApsDestroyCallTree(Tree);  
    }

    ApsFree(Graph);
}

VOID
ApsDestroyCallTree(
    __in PCALL_TREE Tree
    )
{ 
    ApsDestroyCallNode(Tree->RootNode);
    ApsFree(Tree);
}

VOID
ApsDestroyCallNode(
    __in PCALL_NODE Node
    )
{
    PLIST_ENTRY ListEntry;
    PCALL_NODE Child;

    if (!Node) {
        return;
    }

    while(IsListEmpty(&Node->ChildListHead) != TRUE) {
        ListEntry = RemoveHeadList(&Node->ChildListHead);
        Child = CONTAINING_RECORD(ListEntry, CALL_NODE, ListEntry);
        ApsDestroyCallNode(Child);
    }

    ApsFree(Node);
}