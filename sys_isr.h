#ifndef __SYS_ISR_H
#define __SYS_ISR_H

#include "byte_queue.h"

extern void ISR_SetRTC_WB(void (*fp)(void));
extern void ISR_SetTimer_A1_WB(void (*fp)(void));
extern void ISR_SetUTXQueue(ByteQueue_t* q);
extern void UTX_SetOnTransmissionFlag(void);
extern void UTX_ClearOnTransmissionFlag(void);
extern unsigned char UTX_GetOnTransmissionFlag(void);
extern void ISR_SetUART_A0_WB(void (*fp)(void));
extern void ISR_SetUART_A2_WB(void (*fp)(void));
extern void ISR_SetUART_A3_WB(void (*fp)(void));
extern void ISR_Set1Sec_Alarm_WB(void (*fp)(void));

#endif
