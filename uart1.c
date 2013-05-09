//******************************************************************************
//	 UART.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 02/28/2009
//******************************************************************************
#include "uart.h"

typedef struct _SERIAL_BUF {
	BYTE bWrIndex;
	BYTE bRdIndex;
	BYTE bOverflow;
	BYTE bRxCount;
	char cData[MAX_SERIAL_BUF];	
} SERIAL_BUF;
SERIAL_BUF	g_SerialBuf0, g_SerialBuf1;

void UART_InitVariables(void)
{
	int i;
	
	// Initialize serial buffer
	g_SerialBuf0.bRdIndex = 0;
	g_SerialBuf0.bWrIndex = 0;
	g_SerialBuf0.bRxCount = 0;
	g_SerialBuf0.bOverflow = 0;
	for (i=0; i<MAX_SERIAL_BUF; i++)
		g_SerialBuf0.cData[i] = 0;

	// Initialize serial buffer
	g_SerialBuf1.bRdIndex = 0;
	g_SerialBuf1.bWrIndex = 0;
	g_SerialBuf1.bRxCount = 0;
	g_SerialBuf1.bOverflow = 0;
	for (i=0; i<MAX_SERIAL_BUF; i++)
		g_SerialBuf1.cData[i] = 0;
}

// Initialize UART : MSP430F5x38 USCI_A0
void UART_Initialize(BYTE bConfig, DWORD dwClock, DWORD dwBps)
{
	WORD n;
	
	UART_InitVariables();

	switch (bConfig) 
	{
		case UART_UCA0 : 
			P3SEL |= (BIT4 + BIT5);			
		
		  	UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
		
			n = (WORD)(dwClock / dwBps);  
			UCA0BR0 = (BYTE)( n & 0xFF);
		  	UCA0BR1 = (BYTE)((n >> 8) & 0xFF); 
		
		  	UCA0MCTL |= (UCBRS_0 + UCBRF_0);          // Modulation UCBRSx=0, UCBRFx=0
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

		  	UCA1MCTL |= (UCBRS_7 + UCBRF_0);          // Modulation UCBRSx=7, UCBRFx=0
		  	UCA1CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
		  	UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt
			break;
	}
}

// Print a character using UART0 
void UART_WriteByte(BYTE bConfig, BYTE bChar)
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
	}			
}

// Print a string using UART0 
void UART_WriteString(BYTE bConfig, BYTE *pStr)
{
	BYTE ch;
	
    while((ch = *(pStr++)) != NULL) {
    	UART_WriteByte(bConfig, ch);
   	}
}

void UART_WriteStringN(BYTE bConfig, BYTE *pStr, int iLen)
{
	int i;
	
	for (i = 0; i < iLen; i++) {
    	UART_WriteByte(bConfig, *(pStr++));
   	}
}

// Read a character from serial buffer 
BOOL UART_ReadByte(BYTE bConfig, char *pChar)
{
	switch (bConfig) 
	{
		case UART_UCA0 :
			// Check if there is a new data 
			if (g_SerialBuf0.bRxCount == 0) return FALSE;
			
			// Read a byte from serial buffer
		  	*pChar = g_SerialBuf0.cData[g_SerialBuf0.bRdIndex++];
			
			// Reset output index of serial buffer	
			if (g_SerialBuf0.bRdIndex == MAX_SERIAL_BUF) 
				g_SerialBuf0.bRdIndex = 0;
		
			// Disable global interrupts
//			__bic_SR_register(GIE);       // interrupts enabled
		
			g_SerialBuf0.bRxCount--;	
		
			// Enable global interrupts
//			__bis_SR_register(GIE);       // interrupts enabled
			break;
				
		case UART_UCA1 :
			// Check if there is a new data 
			if (g_SerialBuf1.bRxCount == g_SerialBuf1.bWrIndex) return FALSE;
			
			// Read a byte from serial buffer
		  	*pChar = g_SerialBuf1.cData[g_SerialBuf1.bRdIndex++];
UART_WriteByte(XSTREAM_UART_MODE, *pChar);

			// Reset output index of serial buffer	
			if (g_SerialBuf1.bRdIndex == MAX_SERIAL_BUF) 
				g_SerialBuf1.bRdIndex = 0;
		
			// Disable global interrupts
//			__bic_SR_register(GIE);       // interrupts enabled
		
//			g_SerialBuf1.bRxCount--;	
            
			// Enable global interrupts
//			__bis_SR_register(GIE);       // interrupts enabled
			break;
	}
	return TRUE;
}

// USART0 interrupt service routine
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  	switch(__even_in_range(UCA0IV,4))
  	{
  		case 0:	break;                             // Vector 0 - no interrupt
  		case 2:                                    // Vector 2 - RXIFG
			// Put a byte into serial buffer  
 			g_SerialBuf0.cData[g_SerialBuf0.bWrIndex++] = UCA0RXBUF;        

			// Reset output index of serial buffer	
			if (g_SerialBuf0.bWrIndex == MAX_SERIAL_BUF) 
				g_SerialBuf0.bWrIndex = 0;

			if ((g_SerialBuf0.bRxCount++) == MAX_SERIAL_BUF) {
				g_SerialBuf0.bRxCount = MAX_SERIAL_BUF;
				g_SerialBuf0.bOverflow = SET;
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
	register char data;
	
  	switch(__even_in_range(UCA1IV,4))
  	{
  		case 0:	break;                             // Vector 0 - no interrupt
  		case 2:                                    // Vector 2 - RXIFG
			
			// Put a byte into serial buffer  
 			g_SerialBuf1.cData[g_SerialBuf1.bWrIndex++] = UCA1RXBUF;        

			// Reset output index of serial buffer	
			if (g_SerialBuf1.bWrIndex == MAX_SERIAL_BUF) 
				g_SerialBuf1.bWrIndex = 0;

//			if ((g_SerialBuf1.bRxCount++) == MAX_SERIAL_BUF) {
//				g_SerialBuf1.bRxCount = MAX_SERIAL_BUF;
//				g_SerialBuf1.bOverflow = SET;
//				
//			}
				
    		break;
  		case 4:	break;                             // Vector 4 - TXIFG
  		default: break;
  	}
}

