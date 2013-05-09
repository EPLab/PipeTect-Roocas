//******************************************************************************
//	 RTC.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 12/16/2009
//******************************************************************************
#include "rtc.h"
#include "sys_spi.h"
#include "sys_timer.h"
#include "xbee.h"
#include "sys_isr.h"

extern RTC_TIME g_RTCTime;
extern RTC_TIME g_StartTime;
extern volatile WORD g_MsTimer;

RTC_TIME CheckOutTime;
RTC_TIME* pReservedTime = 0;

void (*Every1SEC_WB)(void);


//static void RTC_AdjustClock(RTC_TIME *pTime);


void RTC_SetEvery1SEC_WB(void (*fp)(void))
{
    Every1SEC_WB = fp;
}

void RTC_ReserveTimeToSet(RTC_TIME* pTime)
{
    pReservedTime = pTime;
}

static void RTC_SetTimeWithReservedVal(void)
{
    if (pReservedTime)
    {
        RTC_SetTime(pReservedTime);
        pReservedTime = 0;
    }
}

void RTC_Test(void)
{
    WORD i;
    BYTE str[10];
    
    while (1) 
    {
        RTC_GetTime(&g_RTCTime);
        sprintf((char*)str, "%d + %d\r\n", g_RTCTime.Second, g_MsTimer);   
        XBEE_WriteFrame((char*)str, 10);
        for (i=0; i<1000; i++);
    }
}

static void RTC_SyncTimeWithRTC(void)
{
    BYTE temp;
    
    SYS_ClearTimer(TA1_CONF0);
    //RTC_ReadTime(&g_RTCTime);
    if (Every1SEC_WB)
    {
        Every1SEC_WB();
    }
    //RTC_ReadByte(0x0F, &temp);
    //if (temp & 0x01)
    {
        RTC_WriteByte(0x0F, temp & 0xFE);
    }
}

static void RTC_Start(void)
{
    unsigned char temp;
    RTC_ReadByte(0x0E, &temp);
}

static void RTC_Stop(void)
{
    RTC_WriteByte(0x0E, 0x84);
}

void RTC_Initialize(void)
{
    //RTC_TIME tempTime;
    BYTE temp[4];
    
    RTC_CS_PxSEL &= ~RTC_CS_BIT;
    RTC_CS_PxDIR |= RTC_CS_BIT;
    RTC_CS_PxOUT |= RTC_CS_BIT;

	__bic_SR_register(GIE);       // interrupts disabled
    
    RTC_INT_PxSEL &= ~RTC_INT_BIT;
    RTC_INT_PxDIR &= ~RTC_INT_BIT;
    RTC_INT_PxREN |= RTC_INT_BIT;                
    RTC_INT_PxOUT |= RTC_INT_BIT;                
    RTC_INT_PxIE |= RTC_INT_BIT;                 
    RTC_INT_PxIES |= RTC_INT_BIT;                
    RTC_INT_PxIFG &= ~RTC_INT_BIT;               
    
    g_RTCTime.Year = 10;
    g_RTCTime.Month = 5;
    g_RTCTime.Date = 5;
    g_RTCTime.Day = 5;
    g_RTCTime.Hour = 12;
    g_RTCTime.Minute = 8;
    g_RTCTime.Second = 8;
    g_RTCTime.MSecond = 0;
    
    RTC_Stop();
    
    RTC_SetTime(&g_RTCTime);
    //RTC_GetTime(&CheckOutTime);
    //ISR_SetRTC_WB(&RTC_SetTimeWithReservedVal);
    
    ISR_Set1Sec_Alarm_WB(&RTC_SyncTimeWithRTC);
    //RTC_SetSecAlarm(1);

    RTC_WriteByte(0x07, 0x80);
    RTC_WriteByte(0x08, 0x80);
    RTC_WriteByte(0x09, 0x80);
    RTC_WriteByte(0x0A, 0x80);
#ifndef __ROOCAS
    RTC_WriteCommand(RTC_CMD_ENABLE | RTC_CMD_INTCN | RTC_CMD_A1IE);
#else
    RTC_WriteCommand(RTC_CMD_ENABLE | RTC_CMD_INTCN | RTC_CMD_A1IE);
#endif
    
    __bis_SR_register(GIE);       // interrupts enabled
    SYS_EnableTimer(TA1_CONF0);
}

void RTC_SetTime(RTC_TIME *pTime)
{
    BYTE time;

    time = ((BYTE)(pTime->Year / 10) << 4) + (pTime->Year % 10);
    RTC_WriteByte(RTC_ADDR_YEAR, time);
    time = ((BYTE)(pTime->Month / 10) << 4) + (pTime->Month % 10);
    RTC_WriteByte(RTC_ADDR_MONTH, time);
    time = ((BYTE)(pTime->Date / 10) << 4) + (pTime->Date % 10);
    RTC_WriteByte(RTC_ADDR_DATE, time); 
    RTC_WriteByte(RTC_ADDR_DAY, pTime->Day);                  
    time = ((BYTE)(pTime->Hour / 10) << 4) + (pTime->Hour % 10);
    RTC_WriteByte(RTC_ADDR_HOUR, time);
    time = ((BYTE)(pTime->Minute / 10) << 4) + (pTime->Minute % 10);
    RTC_WriteByte(RTC_ADDR_MINUTE, time);
    time = ((BYTE)(pTime->Second / 10) << 4) + (pTime->Second % 10);
    RTC_WriteByte(RTC_ADDR_SECOND, time);
    //time = ((BYTE)(pTime->MSecond / 10) << 4) + (BYTE)(pTime->MSecond / 100);
    //RTC_WriteByte(RTC_ADDR_MSECOND, time);
}

void RTC_ReadTime(RTC_TIME *pTime)
{
    BYTE time;
    
    // get the millisecond asap for better precision
    pTime->MSecond = g_MsTimer;
    RTC_ReadByte(RTC_ADDR_SECOND, &time);
    pTime->Second = ((time >> 4) << 3) + ((time >> 4) << 1) + (time & 0x0F);
    RTC_ReadByte(RTC_ADDR_MINUTE, &time);
    pTime->Minute = ((time >> 4) << 3) + ((time >> 4) << 1) + (time & 0x0F);
    RTC_ReadByte(RTC_ADDR_HOUR, &time);
    time &= ~RTC_MASK_HOUR;
    pTime->Hour = ((time >> 4) << 3) + ((time >> 4) << 1) + (time & 0x0F);
    RTC_ReadByte(RTC_ADDR_DATE, &time); 
    pTime->Date = ((time >> 4) << 3) + ((time >> 4) << 1) + (time & 0x0F);
    //RTC_ReadByte(RTC_ADDR_DAY, &(pTime->Day));
    RTC_ReadByte(RTC_ADDR_MONTH, &time); 
    pTime->Month = ((time >> 4) << 3) + ((time >> 4) << 1) + (time & 0x0F);
    RTC_ReadByte(RTC_ADDR_YEAR, &time);
    pTime->Year = ((time >> 4) << 3) + ((time >> 4) << 1) + (time & 0x0F);
    //RTC_ReadByte(RTC_ADDR_MSECOND, &time);
    //pTime->MSecond = ((time >> 4) * 100 + (time & 0x0F)) * 10;
}

void RTC_GetTime(RTC_TIME *pTime)
{
    pTime = &g_RTCTime;
}

void RTC_CalcTOffset(WORD *pOffset)
{
    /*
    DWORD msec1, msec2;
    
    msec1 = (g_StartTime.MSecond + g_StartTime.Second * 1000);

    RTC_GetTime(&g_RTCTime);
    
    msec2 = (g_RTCTime.MSecond + g_RTCTime.Second * 1000);

    if (msec2 >= msec1) 
    {
        *pOffset = (WORD)(msec2 - msec1);        
    }
    else    // overflow timer : 1 min
    {
        *pOffset = (WORD)((msec2 + 60000) - msec1);
    }
    */
    *pOffset = g_MsTimer;   //g_MsTimer - g_RTCTime.MSecond;
}

void RTC_DelayMS(DWORD msec)
{
    RTC_TIME time1, time2;
    DWORD msec1, msec2;
    DWORD offset;

    RTC_GetTime(&time1);
    msec1 = (time1.MSecond + time1.Second * 1000);
    while (1) 
    {  
        RTC_GetTime(&time2);
        msec2 = (time2.MSecond + time2.Second * 1000);

        if (msec1 <= msec2) 
        {
            offset = (WORD)(msec2 - msec1);        
        }
        else    
        {
            offset = (WORD)((msec2 + 60000) - msec1);
        }
        
        if (offset >= msec) break;
    }
}

#ifndef __ROOCAS2
void RTC_SetMSecAlarm(BYTE msec)
{
    BYTE time;

    time = ((BYTE)(msec / 100) << 4) + (BYTE)(msec / 10);
    RTC_WriteByte(RTC_ALARM_MSECOND, time);
}
#endif

void RTC_SetSecAlarm(BYTE second)
{
    BYTE time;
    
    time = ((BYTE)(second / 10) << 4) + (second % 10);
    RTC_WriteByte(RTC_ALARM_SECOND, time);
}

void RTC_SetMinAlarm(BYTE minute)
{
    BYTE time;
    
    time = ((BYTE)(minute / 10) << 4) + (minute % 10);
    RTC_WriteByte(RTC_ALARM_MINUTE, time);
}

void RTC_SetHourAlarm(BYTE hour)
{
    BYTE time;
    
    time = ((BYTE)(hour / 10) << 4) + (hour % 10);
    RTC_WriteByte(RTC_ALARM_HOUR, time);
}

void RTC_SetDayAlarm(BYTE day)
{
    RTC_WriteByte(RTC_ALARM_DAY, day);
}

void RTC_SetDateAlarm(BYTE date)
{
    BYTE time;
    
    time = ((BYTE)(date / 10) << 4) + ((date % 10) + RTC_MASK_HOUR);
    RTC_WriteByte(RTC_ALARM_DATE, time);
}

void RTC_WriteByte(BYTE addr, BYTE data)
{
    BYTE dummy;  
  
    RTC_EnableCS();
    
    SYS_WriteByteSPI(RTC_SPI_MODE, (RTC_WRITE_REG + addr));
    SYS_WriteByteSPI(RTC_SPI_MODE, data);
    SYS_ReadByteSPI(RTC_SPI_MODE, &dummy);
    
    RTC_DisableCS();
}

void RTC_ReadByte(BYTE addr, BYTE *data)
{
    RTC_EnableCS();
    
    SYS_WriteByteSPI(RTC_SPI_MODE, addr);
    SYS_WriteByteSPI(RTC_SPI_MODE, RTC_DUMMY_DATA);
    SYS_ReadByteSPI(RTC_SPI_MODE, data);
    
    RTC_DisableCS();
}
                  
void RTC_WriteCommand(BYTE command)
{
    BYTE dummy;  
  
    RTC_EnableCS();
    
    SYS_WriteByteSPI(RTC_SPI_MODE, (RTC_WRITE_REG + RTC_ADDR_CONTROL));
    SYS_WriteByteSPI(RTC_SPI_MODE, command);
    SYS_ReadByteSPI(RTC_SPI_MODE, &dummy);
    
    RTC_DisableCS();
}

void RTC_EnableCS(void)
{
    RTC_CS_PxOUT &= ~RTC_CS_BIT;
}

void RTC_DisableCS(void)
{
    RTC_CS_PxOUT |= RTC_CS_BIT;
}

// The origin of timestamp is year 2010
long RTC_GetLongTimeStamp(RTC_TIME *pTime)
{
	long ret;
	int i;
	
	ret = 0;
	for (i = 2010; i < pTime->Year; ++i)
	{
		if ((((i % 4) == 0) && ((i % 100)!= 0)) || ((i % 400) == 0))
		{
			ret += 366;
		}
		else
		{
			ret += 365;
		}
	}
	switch (pTime->Month)
	{
	case 12:
		ret += 31;
	case 11:
		ret += 30;
	case 10:
		ret += 31;
	case 9:
		ret += 30;
	case 8:
		ret += 31;
	case 7:
		ret += 31;
	case 6:
		ret += 30;
	case 5:
		ret += 31;
	case 4:
		ret += 30;
	case 3:
		ret += 31;
	case 2:
		if ((((pTime->Year % 4) == 0) && ((pTime->Year % 100)!= 0)) || ((pTime->Year % 400) == 0))
		{
			ret += 29;
		}
		else
		{
			ret += 28;
		}
	case 1:
		ret += 31;
		break;
	default:
		return -1L;
	}
	ret += (long)pTime->Date;
	ret *= 24;
	ret += (long)pTime->Hour;
	ret *= 60;
	ret += (long)pTime->Minute;
	ret *= 60;
	ret += (long)pTime->Second;
	ret *= 1000;
	ret += (long)pTime->MSecond;
	return ret;
}

/*
void RTC_AdjustClock(RTC_TIME *pTime)
{
    long oldTimeStamp;
    long tempTimeStamp1;
    long tempTimeStamp2;
    
    RTC_GetTime(&g_RTCTime);
    tempTimeStamp1 = RTC_GetLongTimeStamp(&g_RTCTime);
    tempTimeStamp2 = RTC_GetLongTimeStamp(pTime);
    oldTimeStamp = RTC_GetLongTimeStamp(&CheckOutTime);
    
    tempTimeStamp1 -= tempTimeStamp2;
    oldTimeStamp = tempTimeStamp2 - oldTimeStamp;
    
    tempTimeStamp1 *= 86400000 / oldTimeStamp;  //convert into time over a day
    if ((tempTimeStamp1 > 1100) || (tempTimeStamp1 < -1110))
    {
        RTC_SetTime(pTime);                    //the differnce is too severe, just set time with new value
        // need to clear the millisecond
    }
    // get the proper PPM value and set the AGING register
    
}    
*/
