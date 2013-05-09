//******************************************************************************
//	 ACC_GBL.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 08/06/2009
//******************************************************************************
#include "acc.h"
#include "uif.h"
#include "rtc.h"
#include "sys_adc.h"

ACC_SENSOR g_AccSensor[MAX_ACC_NODE+1];
//signed char g_FATHandle[MAX_ACC_NODE];
UIF_STRUCT *g_CommBuf;  
BYTE g_CanTxMsg[8];
BYTE g_CanRxMsg[15];
BYTE g_Str[50];    
volatile WORD g_TimeFrameCount;
RTC_TIME g_RTCTime;
RTC_TIME g_StartTime;
BYTE g_FATDateTime;
