//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _STATUS_H_
#define _STATUS_H_

//
// from <winerror.h>
//

//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//

//
// Customer Bit
//

#define BTR_C_BIT				0x20000000

//
// Error Bit
//

#define BTR_E_BIT               0xE0000000

//
// Information Bit
//

#define BTR_I_BIT               BTR_C_BIT 

#define BTR_FACILITY			0x00000000
#define BTR_I_BASE			    BTR_I_BIT | BTR_FACILITY   // 0x20000000
#define BTR_E_BASE			    BTR_E_BIT | BTR_FACILITY   // 0xE0000000

#define BTS_FACILITY            0x00010000
#define BTS_I_BASE              BTR_I_BIT | BTS_FACILITY   // 0x20010000
#define BTS_E_BASE              BTR_E_BIT | BTS_FACILITY   // 0xE0010000

//
// Error Status
//

#define BTR_E_UNEXPECTED				0xE0000001
#define BTR_E_INIT_FAILED		        0xE0000002
#define	BTR_E_INVALID_PARAMETER			0xE0000003
#define BTR_E_IO_ERROR                  0xE0000004
#define BTR_E_BAD_MESSAGE               0xE0000005
#define BTR_E_EXCEPTION                 0xE0000006
#define BTR_E_BUFFER_LIMITED            0xE0000007
#define BTR_E_UNEXPECTED_MESSAGE        0xE0000008
#define BTR_E_OUTOFMEMORY               0xE0000009
#define BTR_E_PORT_BROKEN               0xE000000A
#define BTR_E_INVALID_ARGUMENT          0xE000000B
#define BTR_E_PROBE_COLLISION           0xE000000C
#define BTR_E_CANNOT_PROBE              0xE000000D
#define BTR_E_OUTOF_PROBE               0xE000000E
#define BTR_E_ACCESSDENIED              0xE000000F
#define BTR_E_FAILED_LOAD_FILTER        0xE0000010
#define BTR_E_FAILED_GET_FILTER_API     0xE0000011
#define BTR_E_FILTER_REGISTRATION       0xE0000012
#define BTR_E_LOADLIBRARY               0xE0000013
#define BTR_E_GETPROCADDRESS            0xE0000014
#define BTR_E_UNKNOWN_MESSAGE           0xE0000015
#define BTR_E_DISASTER                  0xE0000016
#define BTR_E_STACKCORRUPTION           0xE0000017
#define BTR_E_BAD_PROBE_FLAG            0xE0000019
#define BTR_E_BAD_ADDRESS               0xE0000020
#define BTR_E_NO_FILTER                 0xE0000021
#define BTR_E_NO_FILTER_CONTROL         0xE0000022
#define BTR_E_CREATEFILEMAPPING         0xE0000023
#define BTR_E_MAPVIEWOFFILE             0xE0000024
#define BTR_E_DISK_FULL                 0xE0000025
#define BTR_E_GETFILESIZE               0xE0000026
#define BTR_E_STOP                      0xE0000027
#define BTR_E_FILESIZELIMIT             0xE0000028
#define BTR_E_WRITER_FULL               0xE0000029
#define BTR_E_INDEX_FULL                0xE0000030
#define BTR_E_CREATEFILE                0xE0000031
#define BTR_E_GETMODULEHANDLE           0xE0000032
#define BTR_E_NO_API                    0xE0000033

//
// Fusion Error Code
//

#define BTR_E_FUSION_NOPADDING          0xE0000100
#define BTR_E_FUSION_NOADDRESS          0xE0000101

//
// Normal Status 
//

#define BTR_S_MORE_DATA                 0x20000001
#define BTR_S_UNLOAD                    0x20000002
#define BTR_S_EXITPROCESS               0x20000003
#define BTR_S_USERSTOP                  0x20000004

#endif