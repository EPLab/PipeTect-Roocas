//******************************************************************************
//	 SYS_UART.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 02/28/2009
//******************************************************************************
#include "sys_uart.h"

extern SERIAL_BUF  g_SerialBuf[];
extern SERIAL_BUF  g_SerialTxBuf;

void SYS_InitVarUART(void)
{
	int i, j;
	
	// Initialize serial buffer
    for (i=0; i<MAX_SERIAL_PORT; i++) 
    {
	    g_SerialBuf[i].bRdIndex = 0;
    	g_SerialBuf[i].bWrIndex = 0;
    	g_SerialBuf[i].bRxCount = 0;
    	g_SerialBuf[i].bOverflow = 0;
	    for (j=0; j<MAX_SERIAL_BUF; j++)
		    g_SerialBuf[i].cData[j] = 0;
    }
}

// Initialize UART : MSP430F5x38 USCI_A0
void SYS_InitUART(BYTE bConfig, DWORD dwClock, DWORD dwBps)
{
	WORD n;
	
	SYS_InitVarUART();

	switch (bConfig) 
	{
		case UART_UCA0 : 
			P3SEL |= (BIT4 + BIT5);			
		
		  	UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
		
			n = (WORD)(dwClock / dwBps);  
			UCA0BR0 = (BYTE)( n & 0xFF);
		  	UCA0BR1 = (BYTE)((n >> 8) & 0xFF); 
		
		  	//UCA0MCTL |= (UCBRS_0 + UCBRF_0);          // Modulation UCBRSx=0, UCBRFx=0
		  	UCA0CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
		  	UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
			break;

		case UART_UCA1 : 
			P5SEL |= (BIT6 + BIT7);			

		  	UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCA1CTL1 |= UCSSEL_2;                     // SMCLK
		
			n = (WORD)(dwClock / dwBps);  
			UCA1BR0 = (BYTE)( n & 0xFF);
		  	UCA1BR1 = (BYTE)((n >> 8) & 0xFF); 

		  	UCA1MCTL |= UCBRF_0;          // Modulation UCBRSx=1, UCBRFx=0
		  	UCA1CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
		  	UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt
			break;

        case UART_UCA2 : 	
		  	UCA2CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	
            P9SEL |= (BIT4 + BIT5);
                        
            UCA2CTL1 |= UCSSEL_2;                     // SMCLK
		
			n = (WORD)(dwClock / dwBps);  
			UCA2BR0 = (BYTE)( n & 0xFF);
		  	UCA2BR1 = (BYTE)((n >> 8) & 0xFF); 

		    //UCA2MCTL |= (UCBRS_1 + UCBRF_0);          // Modulation UCBRSx=1, UCBRFx=0
		  	UCA2MCTL = 0;
            UCA2CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
		  	UCA2IE |= UCRXIE;                         // Enable USCI_A2 RX interrupt
			//UCA2IE |= UCTXIE;						// ENABLE TX interrupt
			//UCA2IFG &= ~UCTXIFG;
            break;

		case UART_UCA3 : 
			P10SEL |= (BIT4 + BIT5);			

		  	UCA3CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCA3CTL1 |= UCSSEL_2;                     // SMCLK
		
			n = (WORD)(dwClock / dwBps);  
			UCA3BR0 = (BYTE)( n & 0xFF);
		  	UCA3BR1 = (BYTE)((n >> 8) & 0xFF); 

		  	//UCA3MCTL |= (UCBRS_0 + UCBRF_0);          // Modulation UCBRSx=1, UCBRFx=0
		  	UCA3CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
		  	UCA3IE |= UCRXIE;                         // Enable USCI_A3 RX interrupt
			break;
        default :
            break;
	}
}

// Print a character using UART0 
/********************************************
void SYS_WriteByteUART(BYTE bConfig, BYTE bChar)
{
    SERIAL_BUF	*serial_buf;
    
    serial_buf = (SERIAL_BUF *)&g_SerialTxBuf;

    // Put a byte into serial buffer  
 	serial_buf->cData[serial_buf->bWrIndex] = bChar;        

	// Reset output index of serial buffer	
	if ((++(serial_buf->bWrIndex)) == MAX_SERIAL_BUF) 
		serial_buf->bWrIndex = 0;

	if ((++(serial_buf->bRxCount)) == MAX_SERIAL_BUF) 
    {
		serial_buf->bRxCount = MAX_SERIAL_BUF;
		serial_buf->bOverflow = SET;
	}
}
********************************************/
void SYS_WriteByteUART(BYTE bConfig, BYTE bChar)
{
	switch (bConfig) 
	{
	case UART_UCA0 : 
  			while (!(UCA0IFG & UCTXIFG));
			UCA0TXBUF = bChar;
			break;

        case UART_UCA1 : 
  			while (!(UCA1IFG & UCTXIFG));
			UCA1TXBUF = bChar;
			break;

        case UART_UCA2 : 
  			while (!(UCA2IFG & UCTXIFG));
			UCA2TXBUF = bChar;
			break;

        case UART_UCA3 : 
  			while (!(UCA3IFG & UCTXIFG));
			UCA3TXBUF = bChar;
			break;
        default :
            break;
	}			
}
/********************************************/
void SYS_WriteStringUART(BYTE bConfig, BYTE *pStr)
{
    BYTE ch;
	
    while((ch = *(pStr++)) != NULL) 
    {
    	SYS_WriteByteUART(bConfig, ch);
    }
}

void SYS_WriteFrameUART(BYTE bConfig, BYTE *pStr, int iLen)
{
    int i;
	
    for (i = 0; i < iLen; i++) 
    {
        SYS_WriteByteUART(bConfig, *(pStr++));
    }
}

unsigned char SYS_Is_WaitForUART2TXBufferReady(void)
{
    if (!(UCA2IFG & UCTXIFG))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// Read a character from serial buffer 
BOOL SYS_ReadByteUART(BYTE bConfig, char *pChar)
{
    SERIAL_BUF	*serial_buf;
    
    serial_buf = (SERIAL_BUF *)&(g_SerialBuf[bConfig]);
  
	// Check if there is a new data 
	if (serial_buf->bRxCount == 0) return FALSE;
			
	// Read a byte from serial buffer
  	*pChar = serial_buf->cData[serial_buf->bRdIndex];
			
	// Reset output index of serial buffer	
	if ((++(serial_buf->bRdIndex)) == MAX_SERIAL_BUF) 
		serial_buf->bRdIndex = 0;
		
	// Disable global interrupts
	__bic_SR_register(GIE);       // interrupts disabled
		
	--(serial_buf->bRxCount);	
		
	// Enable global interrupts
	__bis_SR_register(GIE);       // interrupts enabled

	return TRUE;
}

BYTE SYS_GetUARTBUF_RdIdx(BYTE bConfig){
    SERIAL_BUF	*serial_buf;
    
    serial_buf = (SERIAL_BUF *)&(g_SerialBuf[bConfig]);
	
	return serial_buf->bRdIndex;
}
