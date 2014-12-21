//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "aps.h"
#include "apsdefs.h"
#include "apspri.h"
#include "apsctl.h"
#include "apsbtr.h"
#include "apsfile.h"
#include "ntapi.h"
#include "apspdb.h"
#include "apsrpt.h"

ULONG
ApsCheckMmAttribute(
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attr
	)
{
	return APS_STATUS_OK;
}

ULONG
ApsCreateMmProfile(
	__in ULONG ProcessId,
	__in PWSTR ImagePath,
	__in BTR_PROFILE_MODE Mode,
	__in PBTR_PROFILE_ATTRIBUTE Attribute,
	__in PWSTR ReportPath,
	__out PAPS_PROFILE_OBJECT *Object
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	PAPS_PROFILE_OBJECT Profile;

	*Object = NULL;

	Attribute->Type = PROFILE_MM_TYPE;
	Attribute->Mode = Mode;

	//
	// Validate profiling attributes
	//

	Status = ApsCheckMmAttribute(Mode, Attribute);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Open target process object
	//

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}

	//
	// Create profile object
	//

	Status = ApsCreateProfile(&Profile, Mode, PROFILE_MM_TYPE, 
		                      ProcessId, ProcessHandle, ReportPath);
	if (Status != APS_STATUS_OK) {
		return Status;
	}

	//
	// Copy attribute to local profile object
	//

	ASSERT(Profile != NULL);
	memcpy(&Profile->Attribute, Attribute, sizeof(BTR_PROFILE_ATTRIBUTE));

	*Object = Profile;
	return Status;
}