//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _SMP_H_
#define _SMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef struct _PER_CPU_BUFFER {
    volatile PUCHAR Head;
    volatile PUCHAR Tail;
    PUCHAR Buffer;
    SIZE_T Length;
} PER_CPU_BUFFER, *PPER_CPU_BUFFER;

#ifdef __cplusplus
}
#endif

#endif