//******************************************************************************
//	 SYS_RTC.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 10/06/2009
//******************************************************************************
#include "define.h"
#include "rtc.h"

void SYS_TestRTC(void);
void SYS_InitRTC(void);
void SYS_SetTimeRTC(RTC_TIME *pTime);
void SYS_GetTimeRTC(RTC_TIME *pTime);
BOOL SYS_WaitRTC(void);

int SYS_SetYearRTC(int year); 	
int SYS_SetMonRTC(int month);
int SYS_SetDateRTC(int day);
int SYS_SetDayRTC(int dow);
int SYS_SetHourRTC(int hour);
int SYS_SetMinRTC(int min);
int SYS_SetSecRTC(int sec);

int SYS_GetT0RTC(void); 	
int SYS_GetT1RTC(void); 	
int SYS_GetYearRTC(void); 	
int SYS_GetMonRTC(void);
int SYS_GetDateRTC(void);
int SYS_GetDayRTC(void);
int SYS_GetHourRTC(void);
int SYS_GetMinRTC(void);
int SYS_GetSecRTC(void);




