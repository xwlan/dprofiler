//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PFCMD_H_
#define _PFCMD_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "apsprofile.h"

typedef enum _PF_COMMAND_TYPE {
	PF_START = 1,
	PF_PAUSE,
	PF_STOP,
} PF_COMMAND_TYPE;

typedef struct _PF_COMMAND {
	PF_COMMAND_TYPE Type;
} PF_COMMMAND, *PPF_COMMAND;

#ifdef __cplusplus 
}
#endif
#endif