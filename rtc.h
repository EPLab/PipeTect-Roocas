//******************************************************************************
//	 RTC.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 10/06/2009
//******************************************************************************
#ifndef __RTC_H
#define __RTC_H

#include "define.h"

#ifndef __ROOCAS2
    #define RTC_ADDR_MSECOND    0x00
    #define RTC_ADDR_SECOND     0x01
    #define RTC_ADDR_MINUTE     0x02
    #define RTC_ADDR_HOUR       0x03
    #define RTC_ADDR_DAY        0x04
    #define RTC_ADDR_DATE       0x05
    #define RTC_ADDR_MONTH      0x06
    #define RTC_ADDR_YEAR       0x07
    #define RTC_ALARM_MSECOND   0x08
    #define RTC_ALARM_SECOND    0x09
    #define RTC_ALARM_MINUTE    0x0A
    #define RTC_ALARM_HOUR      0x0B
    #define RTC_ALARM_DAY       0x0C
    #define RTC_ALARM_DATE      0x0C
    #define RTC_ADDR_CONTROL    0x0D
    #define RTC_ADDR_STATUS     0x0E
    #define RTC_ADDR_CHARGER    0x0F

    #define RTC_CMD_ENABLE      0x00
    #define RTC_CMD_DISABLE     BIT7
    #define RTC_CMD_BBSQI       BIT5
    #define RTC_CMD_RS0         0x00
    #define RTC_CMD_RS1         BIT3
    #define RTC_CMD_RS2         BIT4
    #define RTC_CMD_RS3         (BIT3 + BIT4)
    #define RTC_CMD_INTCN       BIT2
    #define RTC_CMD_AIE         BIT0
#else
    //#define RTC_ADDR_MSECOND    0x00
    #define RTC_ADDR_SECOND     0x00
    #define RTC_ADDR_MINUTE     0x01
    #define RTC_ADDR_HOUR       0x02
    #define RTC_ADDR_DAY        0x03
    #define RTC_ADDR_DATE       0x04
    #define RTC_ADDR_MONTH      0x05
    #define RTC_ADDR_YEAR       0x06
    //#define RTC_ALARM_MSECOND   0x08
    #define RTC_ALARM_SECOND   0x07
    #define RTC_ALARM_MINUTE   0x08
    #define RTC_ALARM_HOUR     0x09
    #define RTC_ALARM_DAY      0x0A
    #define RTC_ALARM_DATE     0x0A
    #define RTC_ALARM2_MINUTE   0x0B
    #define RTC_ALARM2_HOUR     0x0C
    #define RTC_ALARM2_DAY      0x0D
    #define RTC_ALARM2_DATE     0x0D
    #define RTC_ADDR_CONTROL    0x0E
    #define RTC_ADDR_STATUS     0x0F
    #define RTC_ADDR_CAO        0x10
    #define RTC_TEMP_MSB        0x11
    #define RTC_TEMP_LSB        0x12
    #define RTC_TEMP_DISABLE    0x13

    #define RTC_CMD_ENABLE  0x00
    #define RTC_CMD_EOSC    0x80
    #define RTC_CMD_BBSQW   0x40
    #define RTC_CMD_CONV    0x20
    #define RTC_CMD_RS2     0x10
    #define RTC_CMD_RS1     0x08
    #define RTC_CMD_RS      0x18
    #define RTC_CMD_INTCN   0x04
    #define RTC_CMD_A2IE    0x02
    #define RTC_CMD_A1IE    0x01
    
    #define RTC_ST_OSF      0x80
    #define RTC_ST_BB32     0x40
    #define RTC_ST_CRATE1   0x20
    #define RTC_ST_CRATE0   0x10
    #define RTC_ST_CRATE    0x30
    #define RTC_ST_EN32     0x08
    #define RTC_ST_BSY      0x04
    #define RTC_ST_A2F      0x02
    #define RTC_ST_A1F      0x01
#endif

#define RTC_WRITE_REG       0x80
#define RTC_DUMMY_DATA      0x00
#define RTC_MASK_HOUR       BIT6

typedef struct _RTC_TIME 
{ 
    BYTE Year;
    BYTE Month;
    BYTE Date;
    BYTE Day;
    BYTE Hour;
    BYTE Minute;
    BYTE Second; 
    WORD MSecond;
} RTC_TIME;

void RTC_Test(void);
void RTC_Initialize(void);
void RTC_SetTime(RTC_TIME *pTime);
void RTC_GetTime(RTC_TIME *pTime);
void RTC_SaveTime(RTC_TIME *pTime);
void RTC_CalcTOffset(WORD *pOffset);
void RTC_DelayMS(DWORD msec);

void RTC_SetMSecAlarm(BYTE msec);
void RTC_SetSecAlarm(BYTE second);
void RTC_SetMinAlarm(BYTE minute);
void RTC_SetHourAlarm(BYTE hour);
void RTC_SetDayAlarm(BYTE day);
void RTC_SetDateAlarm(BYTE date);

void RTC_WriteByte(BYTE addr, BYTE data);
void RTC_ReadByte(BYTE addr, BYTE *data);
void RTC_WriteCommand(BYTE command);
void RTC_EnableCS(void);
void RTC_DisableCS(void);

extern void RTC_ReserveTimeToSet(RTC_TIME* pTime);
extern long RTC_GetLongTimeStamp(RTC_TIME *pTime);
extern void RTC_SetEvery1SEC_WB(void (*fp)(void));
extern void RTC_ReadTime(RTC_TIME *pTime);

#endif
