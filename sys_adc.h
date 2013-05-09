//******************************************************************************
//	 SYS_ADC.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 08/20/2009
//******************************************************************************
#include "define.h"

#define MAX_ADC_CH      4

#define ADC_REG_BASE    ADC12MEM0

#define SYS_ADC_0	BIT0
#define SYS_ADC_1	BIT1
#define SYS_ADC_2	BIT2
#define SYS_ADC_3	BIT3
#define SYS_ADC_ALL  (BIT0 + BIT1 + BIT2 + BIT3)

#define SYS_ADC_REF	    0
#define SYS_ADC_AREF	1
#define SYS_ADC_REFALL	2	

#define SYS_ADC_POLL   0
#define SYS_ADC_INT    1

void SYS_InitADC(BYTE Ch, BYTE Mode);
void SYS_ReadADC(BYTE Ch, BYTE Mode, short *Data);


