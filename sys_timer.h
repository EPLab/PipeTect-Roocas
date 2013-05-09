//******************************************************************************
//	 SYS_RTC.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 10/06/2009
//******************************************************************************
#include "define.h"

void SYS_TestTimer(void);
void SYS_InitTimer(void);
void SYS_SetTimer(BYTE bConfig, WORD wMsec);
void SYS_ClearTimer(BYTE bConfig);
void SYS_EnableTimer(BYTE bConfig);
void SYS_DisableTimer(BYTE bConfig);

typedef struct _TIMER_REG 
{
	volatile WORD *CTL;
	volatile WORD *CCTL;
	volatile WORD *CNT; 
	volatile WORD *CCR; 
} TIMER_REG;

typedef enum _TIMER_CONF 
{
	TA1_CONF0 = 0,
	TA1_CONF1,
	TA1_CONF2,
	TB_CONF0,
	TB_CONF1,
	TB_CONF2,
	TB_CONF3,
	TB_CONF4,
	TB_CONF5,
	TB_CONF6,
	MAX_TIMER_CONF
} TIMER_CONF;




