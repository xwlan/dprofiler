//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

//
// N.B. This file is shared by both kernel and user mode projects.
//

#ifndef _KBTR_H_
#define _KBTR_H_

//
// Device I/O Control Types
//

#define KBTR_IOCTL_TYPE 40000

//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//

#define IOCTL_KBTR_METHOD_BUFFERED \
    CTL_CODE(KBTR_IOCTL_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_KBTR_METHOD_IN_DIRECT \
    CTL_CODE(KBTR_IOCTL_TYPE, 0x901, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_KBTR_METHOD_OUT_DIRECT \
    CTL_CODE(KBTR_IOCTL_TYPE, 0x902, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_KBTR_METHOD_NEITHER \
    CTL_CODE(KBTR_IOCTL_TYPE, 0x903, METHOD_NEITHER , FILE_ANY_ACCESS)

//
// N.B. Wake up system worker thread
//

#define IOCTL_KBTR_METHOD_WAKEUP_THREAD \
    CTL_CODE(KBTR_IOCTL_TYPE, 0x904, METHOD_NEITHER , FILE_ANY_ACCESS)


#endif  // KBTR_H_