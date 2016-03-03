//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#ifndef _APS_QUEUE_H_
#define _APS_QUEUE_H_

#include "apsdefs.h"
#include "apsprofile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG 
(CALLBACK *APS_QUEUE_CALLBACK)(
	__in PVOID Packet,
	__in PVOID Context
	);

//
// Packet Type
//

typedef enum _APS_PACKET_TYPE {
	APS_CTL_START,
	APS_CTL_STOP,
	APS_CTL_PAUSE,
	APS_CTL_RESUME,
	APS_CTL_MARK,
} APS_PACKET_TYPE, *PAPS_PACKET_TYPE;

//
// Packet Flag
//

#define PACKET_FLAG_FREE  0x00000001


typedef struct _APS_QUEUE_PACKET {

	LIST_ENTRY ListEntry;

	APS_PACKET_TYPE Type;
	ULONG Flag;
	ULONG Status;

	APS_QUEUE_CALLBACK Callback;
	PVOID Context;
	HANDLE CompleteEvent;

	union {

		struct {
			HANDLE SharedData;
			HANDLE IndexFile;
			HANDLE DataFile;
			HANDLE StackFile;
			HANDLE ReportFile;
			HANDLE UnloadEvent;
			HANDLE ExitProcessEvent;
			HANDLE ExitProcessAckEvent;
			HANDLE ControlEnd;
			HANDLE IoObjectFile;
			HANDLE IoIrpFile;
			HANDLE IoNameFile;
			BTR_PROFILE_ATTRIBUTE Attribute;
		} Start;
		
		struct {
			ULONG Spare;
		} Stop;

		struct {
			ULONG Duration;
		} Pause;

		struct {
			ULONG Spare;
		} Resume;

		struct {
			ULONG Spare;
		} Mark;
	};

} APS_QUEUE_PACKET, *PAPS_QUEUE_PACKET;

typedef struct _APS_QUEUE {
	LIST_ENTRY ListEntry;
	HANDLE ObjectHandle;
} APS_QUEUE, *PAPS_QUEUE;


ULONG
ApsCreateQueue(
	__out PAPS_QUEUE *Object
	);

ULONG
ApsCloseQueue(
	__in PAPS_QUEUE Object
	);

ULONG
ApsQueuePacket(
	__in PAPS_QUEUE Object,
	__in PAPS_QUEUE_PACKET Packet
	);

ULONG
ApsDeQueuePacket(
	__in PAPS_QUEUE Object,
	__out PAPS_QUEUE_PACKET *Packet
	);

ULONG
ApsDeQueuePacketEx(
	__in PAPS_QUEUE Object,
	__in ULONG Milliseconds,
	__out PAPS_QUEUE_PACKET *Packet
	);

ULONG
ApsDeQueuePacketList(
	__in PAPS_QUEUE Object,
	__out PLIST_ENTRY ListHead
	);


#ifdef __cplusplus
}
#endif

#endif
