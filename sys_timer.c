//******************************************************************************
//	 SYS_TIMER.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 1/21/2009
//******************************************************************************
#include "sys_timer.h"
#include "sys.h"

extern TIMER_REG g_TimerReg[];

/*
void SYS_TestTimer(void)
{
    WORD count;
  
    SYS_SetTimer(TA1_CONF0, 1);
    
    while(1)
    {
        count = *g_TimerReg[TA1_CONF0].CNT; 
    }
}
*/

void SYS_InitTimer(void)
{
    P1SEL &= ~BIT3;
    P1OUT |= BIT3;
    //P1REN |= BIT3;
    P1DIR &= ~BIT3;
    
    SYS_SetTimer(TA1_CONF0, 1);
}

void SYS_SetTimer(BYTE bConfig, WORD wMsec)
{
    *g_TimerReg[bConfig].CCR = (WORD)((wMsec * SYS_CLK / 1000) >> 3); 

    // SMCLK/8, up mode, clear TAR
    if (bConfig <= TA1_CONF2)
    {
		*g_TimerReg[bConfig].CTL = ((TASSEL_2 + MC_1) + (TACLR + ID__8));
    }
    else
    {
		*g_TimerReg[bConfig].CTL = ((TBSSEL_2 + MC_1) + (TBCLR + ID__8));
    }
    *g_TimerReg[bConfig].CCTL &= ~CCIE;
}

void SYS_ClearTimer(BYTE bConfig)
{
    *g_TimerReg[bConfig].CNT = 0; 
}

void SYS_EnableTimer(BYTE bConfig)
{
    *g_TimerReg[bConfig].CCTL |= CCIE;
}

void SYS_DisableTimer(BYTE bConfig)
{
    *g_TimerReg[bConfig].CCTL &= ~CCIE;
}
