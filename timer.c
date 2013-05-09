//******************************************************************************
//	 TIMER.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 08/06/2009
//******************************************************************************
#include "define.h"

void TIMER_Initialize(void)
{
    P1DIR |= 0x01;                            // P1.0 output
    TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
    TA1CCR0 = 50000;
    TA1CTL = TASSEL_2 + MC_2 + TACLR;         // SMCLK, contmode, clear TAR
}

// Timer A0 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
    P1OUT ^= 0x01;                            // Toggle P1.0
    TA1CCR0 += 50000;                         // Add Offset to CCR0
}
