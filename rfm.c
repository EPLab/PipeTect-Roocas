//******************************************************************************
//	 RFM.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 12/10/2009
//******************************************************************************
#include "rfm.h"
#include "sys.h"
#include "sys_uart.h"

//static int counterr = 0;


void RFM_Test(void)
{
    char str[20] = "0123456789ABCDEF\r\n";
		
    for (;;) 
    {
	RFM_WriteFrame(str, 18);
    }
}

void RFM_Initialize(void)
{
    //CTS
    SYS_SetPort2(0x01, OUTPUT);
    SYS_SelectPort2(0x01, CLEAR);
    SYS_WritePort2(0x01, CLEAR);
    
    //RTS
    SYS_SetPort2(0x02, INPUT);
    SYS_SelectPort2(0x02, CLEAR);
    SYS_WritePort2(0x02, CLEAR);
    
    RFM_Reset();

    RFM_Wakeup();
    
    //while (SYS_ReadPort2(0x02));
    __no_operation();
}

void RFM_Reset(void)
{
    int i;
	
#ifndef __ROOCAS2
    RFM_RESET_PxOUT &= ~RFM_RESET_BIT;
    for (i=0; i<1000; i++);
    RFM_RESET_PxOUT |= RFM_RESET_BIT;
#else
    WIFI_RESET_PxOUT &= ~WIFI_RESET_BIT;
    for (i=0; i<1000; i++);
    WIFI_RESET_PxOUT |= WIFI_RESET_BIT;
#endif
}

void RFM_Wakeup(void)
{
#ifndef __ROOCAS2
    RFM_WAKEIN_PxOUT &= ~RFM_WAKEIN_BIT;
#endif
}

void RFM_Sleep(void)
{
#ifndef __ROOCAS2
    RFM_WAKEIN_PxOUT |= RFM_WAKEIN_BIT;
#endif
}

BYTE RFM_CheckStatus(void)
{
#ifndef __ROOCAS2
    if (RFM_WAKEOUT_PxIN & RFM_WAKEOUT_BIT)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    return TRUE;
#endif
}

BYTE RFM_ReadByte(char *pData)
{
    BYTE val;
    
    val = SYS_ReadByteUART(RFM_UART_MODE, pData);
    return val;
}

void RFM_WriteFrame(char *pData, int iLen)
{
/*
    for (;;) 
    {  
        if (RFM_CheckStatus() == FALSE) break;
    }
*/  
    //SYS_WritePort2(0x01, CLEAR);
    //SYS_WriteFrameUART(RFM_UART_MODE, (BYTE *)pData, iLen);
    
#ifndef __ROOCAS2
    int i;
    
    for (i = 0; i < iLen; ++i)
    {
        while (SYS_ReadPort2(0x02))
        {
            __no_operation();
        }
        SYS_WriteByteUART(RFM_UART_MODE, pData[i]);
        while (SYS_ReadPort2(0x02))
        {
            __no_operation();
        }
    }
#endif
    //counterr = 0;
    while (SYS_Is_WaitForUART2TXBufferReady() == 0);
    //SYS_WritePort2(0x01, SET);
    // YEB
    // DMA_UART_Enqueue(RFM_UART_MODE, (BYTE *)pData, iLen);
}


