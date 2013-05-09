//******************************************************************************
//	 SYS_GBL.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 1/21/2009
//******************************************************************************
#include "sys_uart.h"
#include "sys_spi.h"
#include "sys_rtc.h"
#include "sys_timer.h"
#include "sys_adc.h"
#include "byte_queue.h"

short g_AdcData[MAX_ADC_CH];

SERIAL_BUF	g_SerialBuf[MAX_SERIAL_PORT];
//SERIAL_BUF  g_SerialTxBuf;

//SPI_BUF	g_SPIBuf;
RTC_TIME g_SYSTime;

volatile WORD g_MsTimer;

TIMER_REG g_TimerReg[MAX_TIMER_CONF] =
{	 
	 {&TA1CTL, &TA1CCTL0, &TA1R, &TA1CCR0},
	 {&TA1CTL, &TA1CCTL1, &TA1R, &TA1CCR1},
	 {&TA1CTL, &TA1CCTL2, &TA1R, &TA1CCR2},
	 {&TBCTL, &TBCCTL0, &TBR, &TBCCR0},
	 {&TBCTL, &TBCCTL1, &TBR, &TBCCR1},
	 {&TBCTL, &TBCCTL2, &TBR, &TBCCR2},
	 {&TBCTL, &TBCCTL3, &TBR, &TBCCR3},
	 {&TBCTL, &TBCCTL4, &TBR, &TBCCR4},
	 {&TBCTL, &TBCCTL5, &TBR, &TBCCR5},
	 {&TBCTL, &TBCCTL6, &TBR, &TBCCR6}
};
