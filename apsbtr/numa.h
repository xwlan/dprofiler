//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _NUMA_H_
#define _NUMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef struct _PER_NODE_BUFFER {
    volatile PUCHAR Head;
    volatile PUCHAR Tail;
    PUCHAR Buffer;
    SIZE_T Length;
} PER_NODE_BUFFER, *PPER_NODE_BUFFER;


#ifdef __cplusplus
}
#endif

#endif