//******************************************************************************
//	 ACC.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 04/16/2009
//******************************************************************************
#ifndef __ACC_H
#define __ACC_H

#include "define.h"

#define MAX_ACC_NODE	    8
#define MAX_ACC_AXIS	    3
#define MAX_ACC_COUNT       16
#define MAX_ACC_TIMEOUT	    10
#define MAX_TIMEFRAME_COUNT (MAX_ACC_NODE*100)

#define ACC_X_AXIS	    0
#define ACC_Y_AXIS	    1
#define ACC_Z_AXIS	    2

#define MAX_SAMPLE4FILE     25000

#define CAN_REQ_ACC_DATA	0

void ACC_Test(void);
void ACC_Initialize(void);
void ACC_Process(unsigned char tid, void* msg);

BYTE ACC_RequestSensor(BYTE NodeId);
BYTE ACC_ReadSensor(void);
void ACC_StartSensor(BYTE NodeId);
void ACC_StopSensor(BYTE NodeId);
void ACC_StartAll(void);
extern void ACC_StopAll(void);
void ACC_SaveData(BYTE NodeId, BYTE *pBuf);
void ACC_SendData(BYTE NodeId, BYTE RemoteSeq);
void ACC_SendTimeData(BYTE NodeId);
void ACC_SetTime(BYTE NodeId, BYTE *buf);
extern void send_exit(void);
void recv_ack(BYTE chanllenge);
extern void miss_ack();

typedef union _DATA16 
{
    BYTE Value[2];
    WORD Data;	
} DATA16;

typedef struct _ACC_SENSOR 
{
    BYTE State;					
    WORD CurSeq;   
    WORD SeqNo;
    BYTE RxDone;
    DATA16 Acc[MAX_ACC_AXIS];	
    DATA16 TimeOffset;
} ACC_SENSOR;

#endif
