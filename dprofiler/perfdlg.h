//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PERFDLG_H_
#define _PERFDLG_H_

#include "dprobe.h"
#include "graphctrl.h"
#include "perf.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_DTL_OBJECT;

typedef enum _PERF_GRAPH_TYPE {
	PerfGraphCpu,
	PerfGraphMemory,
	PerfGraphIo,
	PERF_GRAPH_NUMBER,
} PERF_GRAPH_TYPE;

typedef struct _PERF_TASK_CONTEXT {
	LIST_ENTRY ListEntry;
	PERF_INFORMATION Information;
	GRAPH_HISTORY CpuHistory;
	GRAPH_HISTORY MemoryHistory;
	GRAPH_HISTORY IoHistory;
	GRAPH_HISTORY BtrHistory;
} PERF_TASK_CONTEXT, *PPERF_TASK_CONTEXT;

typedef struct _PERF_DIALOG_CONTEXT {
	HWND hWnd;
	PGRAPH_CONTROL Graph[PERF_GRAPH_NUMBER];	
	LIST_ENTRY TaskContextList;
	PPERF_TASK_CONTEXT Current;
} PERF_DIALOG_CONTEXT, *PPERF_DIALOG_CONTEXT;

INT_PTR
PerfDialog(
	IN HWND hWndParent
	);

INT_PTR CALLBACK
PerfProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
PerfOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);
 
LRESULT
PerfOnSelChanged(
	IN HWND hWnd,
	IN HWND hWndList,
	IN LONG Current
	);

LRESULT
PerfOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
PerfCleanUp(
	IN PDIALOG_OBJECT Object	
	);

ULONG
PerfInsertTaskList(
	IN PDIALOG_OBJECT Object,
	IN HWND hWnd
	);

ULONG
PerfInsertCounterList(
	IN PDIALOG_OBJECT Object,
	IN HWND hWnd
	);

PPERF_TASK_CONTEXT
PerfCreateTaskContext(
	IN PPERF_INFORMATION Information
	);

VOID
PerfUpdateCountersReport(
	IN HWND hWndCounters,
	IN PPERF_TASK_CONTEXT TaskContext 
	);

LRESULT
PerfHighlightCounters(
	IN HWND hWnd,
	IN NMHDR *lpnmhdr
	);

ULONG CALLBACK
PerfGraphConsumerCallback(
	IN PPERF_INFORMATION Information,
	IN PVOID Context
	);

VOID
PerfUpdateGraphHistory(
	IN PPERF_TASK_CONTEXT Context
	);

ULONG CALLBACK
PerfGraphHistoryCallback(
	IN PGRAPH_CONTROL Object,
	OUT PGRAPH_HISTORY *Primary,
	OUT PGRAPH_HISTORY *Secondary,
	IN PVOID Context
	);

VOID
PerfRemoveTerminatedTasks(
	IN HWND hWnd
	);

VOID
PerfUpdateRangeTitle(
	IN HWND hWnd
	);

VOID
PerfComputeAverage(
	IN PPERF_TASK_CONTEXT Context
	);

#ifdef __cplusplus
}
#endif

#endif