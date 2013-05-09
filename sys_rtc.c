//******************************************************************************
//	 SYS_RTC.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 10/06/2009
//******************************************************************************
#include "SYS_RTC.h"

extern RTC_TIME g_SYSTime;

void SYS_TestRTC(void)
{
    SYS_InitRTC();
    
    while (1) 
    {
        __no_operation();
    }
}

void SYS_InitRTC(void)
{
    // RTC enable, BCD mode, alarm every Minute, enable RTC interrupt
    RTCCTL01 = (RTCTEVIE + RTCMODE);     

    g_SYSTime.Year = 10;
    g_SYSTime.Month = 1;
    g_SYSTime.Day = 1;
    g_SYSTime.Date = 1;
    g_SYSTime.Hour = 12;
    g_SYSTime.Minute = 0;
    g_SYSTime.Second = 0;
  
    SYS_SetTimeRTC(&g_SYSTime);
}


void SYS_SetTimeRTC(RTC_TIME *pTime)
{
    SYS_SetYearRTC(pTime->Year);                
    SYS_SetMonRTC(pTime->Month);                
    SYS_SetDayRTC(pTime->Day);                  
    SYS_SetDateRTC(pTime->Date);                  
    SYS_SetHourRTC(pTime->Hour);
    SYS_SetMinRTC(pTime->Minute);
    SYS_SetSecRTC(pTime->Second);

}

void SYS_GetTimeRTC(RTC_TIME *pTime)
{
    pTime->Year = SYS_GetYearRTC();
    pTime->Month = SYS_GetMonRTC();  
    pTime->Date = SYS_GetDateRTC(); 
    pTime->Day = SYS_GetDayRTC();  
    pTime->Hour = SYS_GetHourRTC(); 
    pTime->Minute = SYS_GetMinRTC(); 
    pTime->Second = SYS_GetSecRTC(); 
}

BOOL SYS_WaitRTC(void)
{
//    if (RTCCTL01 & RTCRDY) 
        return TRUE;
//    return FALSE;
}


