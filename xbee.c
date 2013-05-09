//******************************************************************************
//	 XBEE.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 02/28/2009
//******************************************************************************
#include "xbee.h"
#include "sys.h"
#include "sys_uart.h"

void XBEE_Test(void)
{
	char str[20] = "ABCDEFGH01234567\r\n";
	int i;
		
	for (;;) {
		XBEE_WriteFrame(str, 18);
		for (i=0; i<1000; i++);
	}
}

void XBEE_Initialize(void)
{
	XBEE_Enable();
    
	XBEE_Reset();
}

void XBEE_Enable(void)
{
//	XBEE_ENABLE_PxOUT &= (~XBEE_ENABLE_BIT);
}

void XBEE_Disable(void)
{
//	XBEE_ENABLE_PxOUT |= XBEE_ENABLE_BIT;
}

void XBEE_Reset(void)
{
	int i;

	XBEE_RESET_PxOUT &= (~XBEE_RESET_BIT);
	for (i=0; i<1000; i++);
	XBEE_RESET_PxOUT |= XBEE_RESET_BIT;
}

void XBEE_Wakeup(void)
{
//    XBEE_SLEEPRQ_PxOUT |= XBEE_SLEEPRQ_BIT;
}

void XBEE_Sleep(void)
{
//    XBEE_SLEEPRQ_PxOUT &= ~XBEE_SLEEPRQ_BIT;
}

BYTE XBEE_ReadByte(char *pData)
{
    BYTE val;
    
    val = SYS_ReadByteUART(XBEE_UART_MODE, pData);
    return val;
}

void XBEE_WriteFrame(char *pData, int iLen)
{
	SYS_WriteFrameUART(XBEE_UART_MODE, (BYTE *)pData, iLen);
}


