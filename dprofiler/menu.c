#include "sdk.h"
#include "resource.h"

HMENU
MenuCreate(
	IN UINT ResourceId
	)
{
	HMENU hMenu;
	hMenu = LoadMenu(SdkInstance, MAKEINTRESOURCE(ResourceId));
    ASSERT(hMenu);
    return hMenu;
}

VOID
MenuDestroy(
	IN HMENU hMenu
	)
{
	ASSERT(hMenu != NULL);
	DestroyMenu(hMenu);
}
