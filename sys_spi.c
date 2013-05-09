//******************************************************************************
//	 SYS_SPI.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 02/28/2009
//******************************************************************************
#include "sys_spi.h"

extern SPI_BUF	g_SPIBuf;

void SYS_InitSPI(BYTE bConfig)
{
	//SYS_InitVarSPI();

	switch (bConfig)
	{
		case SPI_UCB0 :
			P3SEL |= (BIT1 + (BIT2 + BIT3));
			P3DIR |= (BIT1 + BIT3);
			P3DIR &= (~BIT2);

		  	UCB0CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCB0CTL0 |= ((UCMST + UCSYNC) + (UCCKPH + UCMSB));  
                                                      // 3-pin, 8-bit SPI master
  		                                              // Clock polarity high, MSB
		  	UCB0CTL1 |= UCSSEL_2;                     // SMCLK
		  	UCB0BR0 = 0x02;                           // /2
		  	UCB0BR1 = 0;                              //
  			UCB0CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
			break;
			
		case SPI_UCB1 :
			P3SEL |= BIT7;
			P5SEL |= (BIT4 + BIT5);
            
		  	UCB1CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCB1CTL0 |= ((UCMST + UCSYNC) + (UCCKPL + UCMSB));  
                                                      // 3-pin, 8-bit SPI master
  		                                              // Clock polarity low, phase low, MSB
		  	UCB1CTL1 |= UCSSEL_2;             	      // SMCLK
		  	UCB1BR0 = 0x04;                           // /4
		  	UCB1BR1 = 0;                              //
  			UCB1CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
			break;

		case SPI_UCB2 :
			P9SEL |= (BIT1 + (BIT2 + BIT3));

		  	UCB2CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCB2CTL0 |= ((UCMST + UCSYNC) + (UCCKPH + UCMSB));  
                                                      // 3-pin, 8-bit SPI master
  		                                              // Clock polarity high, MSB
		  	UCB2CTL1 |= UCSSEL_2;                     // SMCLK
		  	UCB2BR0 = 0x04;                           // /4
		  	UCB2BR1 = 0;                              //
  			UCB2CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
			break;
			
		case SPI_UCB3 :
			P10SEL |= (BIT1 + BIT2 + BIT3);

		  	UCB3CTL1 |= UCSWRST;                      // **Put state machine in reset**
		  	UCB3CTL0 |= ((UCMST + UCSYNC) + (UCCKPL + UCMSB));  
                                                      // 3-pin, 8-bit SPI master
  		                                              // Clock polarity high, MSB
		  	UCB3CTL1 |= UCSSEL_2;             	      // SMCLK
		  	UCB3BR0 = 0x04;                           // /4
		  	UCB3BR1 = 0;                              //
  			UCB3CTL1 &= (~UCSWRST);                   // **Initialize USCI state machine**
			break;

        default :
            break;
	}
}

// Print a character using SPI0
void SYS_WriteByteSPI(BYTE bConfig, BYTE bChar)
{
	switch (bConfig)
	{
		case SPI_UCB0 :	
           	UCB0TXBUF = bChar;
		    while (!(UCB0IFG & UCTXIFG));
			break;
			
		case SPI_UCB1 :	
			UCB1TXBUF = bChar;
		    while (!(UCB1IFG & UCTXIFG));
			break;

        case SPI_UCB2 :	
           	UCB2TXBUF = bChar;
		    while (!(UCB2IFG & UCTXIFG));
			break;
			
		case SPI_UCB3 :	
			UCB3TXBUF = bChar;
		    while (!(UCB3IFG & UCTXIFG));
			break;

        default :
            break;
	}
}

// Read a character from SPI buffer
BOOL SYS_ReadByteSPI(BYTE bConfig, BYTE *pChar)
{
	switch (bConfig)
	{
		case SPI_UCB0 :	
		    while (!(UCB0IFG & UCRXIFG));
		    *pChar = UCB0RXBUF;
			break;

		case SPI_UCB1 :	
		    while (!(UCB1IFG & UCRXIFG));
		    *pChar = UCB1RXBUF;
			break;

		case SPI_UCB2 :	
		    while (!(UCB2IFG & UCRXIFG));
		    *pChar = UCB2RXBUF;
			break;

		case SPI_UCB3 :	
		    while (!(UCB3IFG & UCRXIFG));
		    *pChar = UCB3RXBUF;
			break;

        default :
            break;
	}
	return TRUE;
}

//Read a frame of bytes via SPI
BYTE SYS_ReadFrameSPI(BYTE bConfig, BYTE bDummy, BYTE *pBuffer, WORD wSize)
{
  	DWORD i;

  	// clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
  	for (i = 0; i < wSize; i++) 
    {
    	SYS_WriteByteSPI(bConfig, bDummy);     // dummy write
    	SYS_ReadByteSPI(bConfig, &pBuffer[i]);
  	}
  	
  	return TRUE;
}

//Send a frame of bytes via SPI
BYTE SYS_WriteFrameSPI(BYTE bConfig, BYTE *pBuffer, WORD wSize)
{
  	DWORD i;

  	// clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
  	for (i = 0; i < wSize; i++) 
    {
    	SYS_WriteByteSPI(bConfig, pBuffer[i]);     // dummy write
    	SYS_ReadByteSPI(bConfig, &pBuffer[i]);
  	}
  	
  	return TRUE;
}

/*
void SYS_InitVarSPI(void)
{
	int i;
	
	// Initialize SPI buffer
	g_SPIBuf.bRdIndex = 0;
	g_SPIBuf.bWrIndex = 0;
	g_SPIBuf.bRxCount = 0;
	g_SPIBuf.bOverflow = 0;
	for (i=0; i<MAX_SPI_BUF; i++)
    {
		g_SPIBuf.cData[i] = 0;
    }
}
*/
