#ifndef _MENU_H_
#define _MENU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

HMENU
MenuCreate(
	IN UINT ResourceId
	);

VOID
MenuDestroy(
	IN HMENU hMenu
	);


#ifdef __cplusplus
}
#endif

#endif