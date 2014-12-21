//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "bsp.h"
#include <stdio.h>
#include <psapi.h>
#include <strsafe.h>
#include <time.h>
#include "ntapi.h"
#include "dprofiler.h"
#include "dpf.h"
#include <dbghelp.h>
#include "wizard.h"

VOID
DpfBuildConfigFromWizard(
	IN struct _WIZARD_CONTEXT *Context,
	IN PDPF_PROFILE_CONFIG Config
	)
{
	Config->Type = Context->Type;
	Config->Attach = Context->Attach;
	Config->Process = Context->Process;

	if (Config->Type == PROFILE_CPU_TYPE) {
		Config->Cpu.SamplingPeriod = Context->Cpu.SamplingPeriod;
		Config->Cpu.StackDepth = Context->Cpu.StackDepth;
		Config->Cpu.IncludeWait = Context->Cpu.IncludeWait;
	}

	if (Config->Type == PROFILE_MM_TYPE) {
		Config->Mm.EnableHeap = Context->Mm.EnableHeap;
		Config->Mm.EnablePage = Context->Mm.EnablePage;
		Config->Mm.EnableHandle = Context->Mm.EnableHandle;
		Config->Mm.EnableGdi = Context->Mm.EnableGdi;
	}

	if (Config->Type == PROFILE_IO_TYPE) {
		Config->Io.EnableNet = Context->Io.EnableNet;
		Config->Io.EnableDisk = Context->Io.EnableDisk;
		Config->Io.EnablePipe = Context->Io.EnablePipe;
		Config->Io.EnableDevice = Context->Io.EnableDevice;
	}

	if (!Config->Attach) {
		StringCchCopy(Config->ImagePath, MAX_PATH, Context->ImagePath);
		StringCchCopy(Config->Argument, MAX_PATH, Context->Argument);
		StringCchCopy(Config->WorkPath, MAX_PATH, Context->WorkPath);
	}
}

ULONG
DpfInitDbghelp(
	IN HANDLE Handle	
	)
{
	SymInitialize(Handle, NULL, TRUE);
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | 
		          SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_LOAD_LINES | 
				  SYMOPT_OMAP_FIND_NEAREST);
	return PF_STATUS_OK;
}

VOID
DpfUninitDbghelp(
	IN HANDLE Handle	
	)
{
	SymCleanup(Handle);
}