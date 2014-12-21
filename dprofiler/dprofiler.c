// dprofiler.cpp : Defines the entry point for the application.
//

#include "main.h"
#include "dprofiler.h"
#include "apsctl.h"
#include "aps.h"

BOOLEAN
ParseCommandLine(
	__out PWCHAR Buffer
	)
{	
	return TRUE;
}

int WINAPI 
wWinMain(
	__in HINSTANCE Instance,
	__in HINSTANCE Previous,
	__in PWSTR CommandLine,
	__in int nCmdShow
	)
{
	ULONG Status;
	WCHAR Buffer[MAX_PATH];

	if (ParseCommandLine(Buffer) != TRUE) {
		return FALSE;
	}

    //
    // Initialize SDK
    //

	Status = SdkInitialize(Instance);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Initialize APS library
	//

	Status = ApsInitialize(0);
    if (Status != S_OK) {
        return Status;
    }

    //
    // Initialize MDB
    //

    Status = MdbInitialize(0);
    if (Status != S_OK) {
        return Status;
    }

    //
    // Initialize UI
    //

	Status = MainInitialize();
	if (!Status) {
		return Status;
	}

	MainMessagePump();
	return 0;
}
