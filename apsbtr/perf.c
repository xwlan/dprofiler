//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
//

#include "perf.h"
#include "ntapi.h"
#include "heap.h"
#include "hal.h"


DECLSPEC_CACHEALIGN
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PerfCpuTotal;

SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *PerfCpuInformation;

ULONG PerfCpuInformationLength;

PERF_INFORMATION PerfInformation;

ULONG64 PerfTotalTime;
FLOAT PerfKernelUsage;
FLOAT PerfUserUsage;

PERF_DELTA PerfCpuKernelDelta;
PERF_DELTA PerfCpuUserDelta;
PERF_DELTA PerfCpuOtherDelta;

PPERF_DELTA PerfCpusKernelDelta;
PPERF_DELTA PerfCpusUserDelta;
PPERF_DELTA PerfCpusOtherDelta;

FLOAT PerfCpuKernelUsage;
FLOAT PerfCpuUserUsage;
FLOAT PerfCpuOtherUsage;

PFLOAT PerfCpusKernelUsage;
PFLOAT PerfCpusUserUsage;
PFLOAT PerfCpusOtherUsage;

BOOLEAN PerfFirstRun;

//
// Track CPU time for host process
//

PERF_PROCESS PerfProcess;


ULONG
PerfInitialize(
    VOID
	)
{
	PerfCpuInformationLength = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * BtrNumberOfProcessors;
	PerfCpuInformation = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)BtrMalloc(PerfCpuInformationLength);

	PerfCpusKernelDelta = (PPERF_DELTA)BtrMalloc(sizeof(PERF_DELTA) * BtrNumberOfProcessors);
	PerfCpusUserDelta = (PPERF_DELTA)BtrMalloc(sizeof(PERF_DELTA) * BtrNumberOfProcessors);
	PerfCpusOtherDelta = (PPERF_DELTA)BtrMalloc(sizeof(PERF_DELTA) * BtrNumberOfProcessors);

	PerfCpusKernelUsage = (PFLOAT)BtrMalloc(sizeof(FLOAT) * BtrNumberOfProcessors);
	PerfCpusUserUsage = (PFLOAT)BtrMalloc(sizeof(FLOAT) * BtrNumberOfProcessors);
	PerfCpusOtherUsage = (PFLOAT)BtrMalloc(sizeof(FLOAT) * BtrNumberOfProcessors);

	PerfFirstRun = TRUE;
	return S_OK;
}

VOID
PerfUninitialize(
	VOID
	)
{
    return;	
}

BOOLEAN
PerfIsFirstRun(
	VOID
	)
{
	return PerfFirstRun;
}

ULONG64 FORCEINLINE 
PerfUpdateDelta(
	__in PPERF_DELTA Delta, 
	__in ULONG64 Current
	)
{
    Delta->Delta = Current - Delta->Value;
    Delta->Value = Current;
	return Delta->Delta;
}

ULONG
PerfUpdateCpuTime(
	__in PPERF_PROCESS Process
	)
{
	ULONG i;
    ULONG64 TotalTime;
	NTSTATUS Status;

    RtlZeroMemory(&PerfCpuTotal, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    Status = (*NtQuerySystemInformation)(SystemProcessorPerformanceInformation,
									     PerfCpuInformation,
									     PerfCpuInformationLength,
									     NULL);
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    for (i = 0; i < BtrNumberOfProcessors; i++) {

        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION Cpu = &PerfCpuInformation[i];

        Cpu->KernelTime.QuadPart -= Cpu->IdleTime.QuadPart + Cpu->DpcTime.QuadPart + Cpu->InterruptTime.QuadPart;

        PerfCpuTotal.DpcTime.QuadPart += Cpu->DpcTime.QuadPart;
        PerfCpuTotal.IdleTime.QuadPart += Cpu->IdleTime.QuadPart;
        PerfCpuTotal.InterruptCount += Cpu->InterruptCount;
        PerfCpuTotal.InterruptTime.QuadPart += Cpu->InterruptTime.QuadPart;
        PerfCpuTotal.KernelTime.QuadPart += Cpu->KernelTime.QuadPart;
        PerfCpuTotal.UserTime.QuadPart += Cpu->UserTime.QuadPart;

        PerfUpdateDelta(&PerfCpusKernelDelta[i], Cpu->KernelTime.QuadPart);
        PerfUpdateDelta(&PerfCpusUserDelta[i], Cpu->UserTime.QuadPart);
        PerfUpdateDelta(&PerfCpusOtherDelta[i],
			             Cpu->IdleTime.QuadPart + 
						 Cpu->DpcTime.QuadPart + 
						 Cpu->InterruptTime.QuadPart);

        TotalTime = PerfCpusKernelDelta[i].Delta + PerfCpusUserDelta[i].Delta + PerfCpusOtherDelta[i].Delta;

        if (TotalTime != 0) {
            PerfCpusKernelUsage[i] = (FLOAT)PerfCpusKernelDelta[i].Delta / TotalTime;
            PerfCpusUserUsage[i] = (FLOAT)PerfCpusUserDelta[i].Delta / TotalTime;
        }
        else {
            PerfCpusKernelUsage[i] = 0;
            PerfCpusUserUsage[i] = 0;
        }
	}

    PerfUpdateDelta(&PerfCpuKernelDelta, PerfCpuTotal.KernelTime.QuadPart);
    PerfUpdateDelta(&PerfCpuUserDelta, PerfCpuTotal.UserTime.QuadPart);
	PerfUpdateDelta(&PerfCpuOtherDelta, 
		             PerfCpuTotal.IdleTime.QuadPart + 
					 PerfCpuTotal.DpcTime.QuadPart  + 
					 PerfCpuTotal.InterruptTime.QuadPart);

    TotalTime = PerfCpuKernelDelta.Delta + PerfCpuUserDelta.Delta + PerfCpuOtherDelta.Delta;

    if (TotalTime) {
        PerfCpuKernelUsage = (FLOAT)PerfCpuKernelDelta.Delta / TotalTime;
        PerfCpuUserUsage = (FLOAT)PerfCpuUserDelta.Delta / TotalTime;
    }
    else {
        PerfCpuKernelUsage = 0;
        PerfCpuUserUsage = 0;
	}

    PerfTotalTime = TotalTime;
	
    //
    // Query host process's CPU time
    //

    PerfUpdateDelta(&PerfInformation.CpuKernelDelta, Process->KernelTime.QuadPart);
    PerfUpdateDelta(&PerfInformation.CpuUserDelta, Process->UserTime.QuadPart);

    if (PerfTotalTime != 0) {

        PerfInformation.CpuKernelUsage = (FLOAT)PerfInformation.CpuKernelDelta.Delta / PerfTotalTime;
        PerfInformation.CpuUserUsage = (FLOAT)PerfInformation.CpuUserDelta.Delta / PerfTotalTime;
        PerfInformation.CpuTotalUsage = (FLOAT)PerfInformation.CpuKernelUsage + 
                                               PerfInformation.CpuUserUsage;

    } else {

        PerfInformation.CpuKernelUsage = 0;
        PerfInformation.CpuUserUsage = 0;
        PerfInformation.CpuTotalUsage = 0;

    }

    if (PerfFirstRun) {
        PerfFirstRun = FALSE;
    }

	return S_OK;
}

VOID
PerfUpdateProcessTime(
    __in ULONG64 KernelTime, 
    __in ULONG64 UserTime 
    )
{
    PerfProcess.KernelTime.QuadPart = KernelTime;
    PerfProcess.UserTime.QuadPart = UserTime;

    PerfUpdateCpuTime(&PerfProcess);
}

FLOAT
PerfGetCpuUsage(
    VOID
    )
{
    return PerfInformation.CpuTotalUsage;
}