//******************************************************************************
//	 XSTREAM.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 04/10/2009
//******************************************************************************
#include "xstream.h"
#include "sys.h"
#include "sys_uart.h"

void XSTREAM_Test(void)
{
	char str[20] = "ABCDEFGH012345\r\n";
	int i;
		
	for (;;) 
    {
		XSTREAM_WriteFrame(str, 16);
		for (i=0; i<1000; i++);
	}
}

void XSTREAM_Initialize(void)
{
	XSTREAM_Reset();

	XSTREAM_Enable();
}

void XSTREAM_Enable(void)
{
    XSTREAM_ENABLE_PxOUT |= XSTREAM_ENABLE_BIT; 
}

void XSTREAM_Disable(void)
{
    XSTREAM_ENABLE_PxOUT &= ~XSTREAM_ENABLE_BIT; 
}

void XSTREAM_Reset(void)
{
	int i;
	
	XSTREAM_RESET_PxOUT &= ~XSTREAM_RESET_BIT;
	for (i=0; i<1000; i++);
	XSTREAM_RESET_PxOUT |= XSTREAM_RESET_BIT;
}

void XSTREAM_Wakeup(void)
{
//    XSTREAM_SLEEP_PxOUT |= XSTREAM_SLEEP_BIT;
}

void XSTREAM_Sleep(void)
{
//    XSTREAM_SLEEP_PxOUT &= ~XSTREAM_SLEEP_BIT;
}

BYTE XSTREAM_ReadByte(char *pData)
{
    BYTE val;
    
    val = SYS_ReadByteUART(XSTREAM_UART_MODE, pData);
    return val;
}

void XSTREAM_WriteFrame(char *pData, int iLen)
{
	SYS_WriteFrameUART(XSTREAM_UART_MODE, (BYTE *)pData, iLen);
}

