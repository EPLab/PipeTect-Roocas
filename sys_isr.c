//******************************************************************************
//	 SYS_ISR.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 1/21/2009
//******************************************************************************
#include "sys_timer.h"
#include "sys_rtc.h"
#include "sys_spi.h"
#include "sys_uart.h"

#include "can_recv_queue.h"
#include "can.h"

#include "byte_queue.h"


void (*Timer_A1_WB)(void) = 0;
void (*UART_A0_WB)(void) = 0;
void (*UART_A2_WB)(void) = 0;
void (*UART_A3_WB)(void) = 0;
void (*SYS_RTCISR_WB)(void) = 0;
void (*Alarm1Sec_WB)(void) = 0;

ByteQueue_t* isrUTXQueue;
unsigned char utxOnTransmission = 0;

extern SERIAL_BUF   g_SerialBuf[];
extern SERIAL_BUF   g_SerialTxBuf;
extern RTC_TIME     g_SYSTime;
extern volatile WORD         g_MsTimer;

void ISR_SetRTC_WB(void (*fp)(void))
{
    SYS_RTCISR_WB = fp;
}

void ISR_SetTimer_A1_WB(void (*fp)(void))
{
    Timer_A1_WB = fp;
}

void ISR_Set1Sec_Alarm_WB(void (*fp)(void))
{
    Alarm1Sec_WB = fp;
}

void ISR_SetUTXQueue(ByteQueue_t* q)
{
	isrUTXQueue = q;
}

void ISR_SetUART_A0_WB(void (*fp)(void)){
    UART_A0_WB = fp;
}

void ISR_SetUART_A2_WB(void (*fp)(void)){
    UART_A2_WB = fp;
}

void ISR_SetUART_A3_WB(void (*fp)(void)){
    UART_A3_WB = fp;
}

void UTX_SetOnTransmissionFlag(void)
{
	utxOnTransmission = 1;
}

void UTX_ClearOnTransmissionFlag(void)
{
	utxOnTransmission = 0;
}

unsigned char UTX_GetOnTransmissionFlag(void)
{
	return utxOnTransmission;
}

// USART0 interrupt service routine
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    register SERIAL_BUF	*serial_buf;

    serial_buf = (SERIAL_BUF *)&(g_SerialBuf[UART_UCA0]);

  	switch(__even_in_range(UCA0IV,4))
  	{
  		case 0:	break;                             // Vector 0 - no interrupt
  		case 2:                                    // Vector 2 - RXIFG
			// Put a byte into serial buffer
 			serial_buf->cData[serial_buf->bWrIndex] = UCA0RXBUF;

			// Reset output index of serial buffer	
			if ((++(serial_buf->bWrIndex)) == MAX_SERIAL_BUF)
				serial_buf->bWrIndex = 0;

			if ((++(serial_buf->bRxCount)) == MAX_SERIAL_BUF)
            {
				serial_buf->bRxCount = MAX_SERIAL_BUF;
				serial_buf->bOverflow = SET;
			}
				
    		break;
  		case 4:	break;                             // Vector 4 - TXIFG
  		default: break;
  	}
}

// USART1 interrupt service routine
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    register SERIAL_BUF	*serial_buf;

    serial_buf = (SERIAL_BUF *)&(g_SerialBuf[UART_UCA1]);

  	switch(__even_in_range(UCA1IV,4))
  	{
  		case 0:	break;                             // Vector 0 - no interrupt
  		case 2:                                    // Vector 2 - RXIFG
			// Put a byte into serial buffer
 			serial_buf->cData[serial_buf->bWrIndex] = UCA1RXBUF;

			// Reset output index of serial buffer	
			if ((++(serial_buf->bWrIndex)) == MAX_SERIAL_BUF)
				serial_buf->bWrIndex = 0;

			if ((++(serial_buf->bRxCount)) == MAX_SERIAL_BUF)
            {
				serial_buf->bRxCount = MAX_SERIAL_BUF;
				serial_buf->bOverflow = SET;
			}
				
    		break;
  		case 4:	break;                             // Vector 4 - TXIFG
  		default: break;
  	}
}

// USART2 interrupt service routine
#pragma vector=USCI_A2_VECTOR
__interrupt void USCI_A2_ISR(void)
{
    register SERIAL_BUF	*serial_buf;
	unsigned char temp;

    serial_buf = (SERIAL_BUF *)&(g_SerialBuf[UART_UCA2]);

    switch(__even_in_range(UCA2IV,4))
    {
  	case 0:	break;                             // Vector 0 - no interrupt
  	case 2:                                    // Vector 2 - RXIFG
        // Put a byte into serial buffer
 		serial_buf->cData[serial_buf->bWrIndex] = UCA2RXBUF;
    	// Reset output index of serial buffer	
		if ((++(serial_buf->bWrIndex)) == MAX_SERIAL_BUF)
		{
        	serial_buf->bWrIndex = 0;
        }

		if ((++(serial_buf->bRxCount)) == MAX_SERIAL_BUF)
        {
		    serial_buf->bRxCount = MAX_SERIAL_BUF;
		    serial_buf->bOverflow = SET;
		}		
        if (UART_A2_WB){
            UART_A2_WB();
        }
    	break;
  	case 4:
		if (BQ_dequeue(isrUTXQueue, &temp) == 0)
		{
			utxOnTransmission = 0;
		}
		else
		{
			UCA2TXBUF = temp;
		}
/**********************************************************
            serial_buf = (SERIAL_BUF *)&g_SerialTxBuf;

	        // Check if there is a new data
	        if (serial_buf->bRxCount == 0) break;
			
        	// Read a byte from serial buffer
  	        UCA2TXBUF = serial_buf->cData[serial_buf->bRdIndex];
			
	        // Reset output index of serial buffer	
	        if ((++(serial_buf->bRdIndex)) == MAX_SERIAL_BUF)
		        serial_buf->bRdIndex = 0;

           	--(serial_buf->bRxCount);	
**********************************************************/
		break;                             // Vector 4 - TXIFG
  	default: break;
    }
}

// USART3 interrupt service routine
#pragma vector=USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
    register SERIAL_BUF	*serial_buf;

    serial_buf = (SERIAL_BUF *)&(g_SerialBuf[UART_UCA3]);

  	switch(__even_in_range(UCA3IV,4))
  	{
  		case 0:	break;                             // Vector 0 - no interrupt
  		case 2:                                    // Vector 2 - RXIFG
			// Put a byte into serial buffer
 			serial_buf->cData[serial_buf->bWrIndex] = UCA3RXBUF;

			// Reset output index of serial buffer	
			if ((++(serial_buf->bWrIndex)) == MAX_SERIAL_BUF)
				serial_buf->bWrIndex = 0;

			if ((++(serial_buf->bRxCount)) == MAX_SERIAL_BUF)
            {
				serial_buf->bRxCount = MAX_SERIAL_BUF;
				serial_buf->bOverflow = SET;
			}
				
    		break;
  		case 4:	break;                             // Vector 4 - TXIFG
  		default: break;
  	}
}

/**************************
#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
  	switch(__even_in_range(UCB0IV,4))
  	{
    	case 0: break;
    	case 2:
      		break;
    	case 4: break;
    	default: break;
  	}
}
************************/

// RTC interrupt service routine
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    char str[5];

    if (RTCCTL01 & RTCTEVIFG)
    {
        if (SYS_RTCISR_WB)
        {
            SYS_RTCISR_WB();
        }
        SYS_GetTimeRTC(&g_SYSTime);
        sprintf(str, "%d\r\n", g_SYSTime.Second);
        RTCCTL01 &= ~RTCTEVIFG;
    }
}

// Timer A0 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
    g_MsTimer++;
    //if (g_MsTimer == 1000)
    //{
    //    g_MsTimer = 0;
    //}
//    P1OUT ^= BIT3;

    if (Timer_A1_WB)
    {
        Timer_A1_WB();
    }
}

// Timer A1 Interrupt Vector (TAIV) handler
#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER1_A1_ISR(void)
{
    switch(__even_in_range(TA1IV, 14))
    {
        case  0: break;                          // No interrupt
        case  2: break;                          // CCR1 not used
        case  4: break;                          // CCR2 not used
        case  6: break;                          // reserved
        case  8: break;                          // reserved
        case 10: break;                          // reserved
        case 12: break;                          // reserved
        case 14:                                 // overflow
             break;
        default: break;
    }
}

// Timer_B0 Interrupt Vector (TBIV) handler
#pragma vector=TIMERB0_VECTOR
__interrupt void TIMERB0_ISR(void)
{
}

// Timer_B7 Interrupt Vector (TBIV) handler
#pragma vector=TIMERB1_VECTOR
__interrupt void TIMERB1_ISR(void)
{
    /* Any access, read or write, of the TBIV register automatically resets the
    highest "pending" interrupt flag. */
    switch( __even_in_range(TBIV,14) )
    {
        case  0: break;                          // No interrupt
        case  2: break;                          // CCR1 not used
        case  4: break;                          // CCR2 not used
        case  6: break;                          // CCR3 not used
        case  8: break;                          // CCR4 not used
        case 10: break;                          // CCR5 not used
        case 12: break;                          // CCR6 not used
        case 14: ;                  // overflow
            break;
        default: break;
    }
}

// Port 1 interrupt service routine
// 1 sec alarm from external RTC
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    if (RTC_INT_PxIFG & RTC_INT_BIT)
    {
        g_MsTimer = 0;
        if (Alarm1Sec_WB)
        {
            Alarm1Sec_WB();
        }
        RTC_INT_PxIFG &= ~RTC_INT_BIT;
    }
    else if (P1IFG & 0x01)
    {
        GPS_PPS_PxIFG &= ~GPS_PPS_BIT;
    }
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    if (P2IFG & 0x40)
    {
        P2IFG &= 0xBF;
    }
}
