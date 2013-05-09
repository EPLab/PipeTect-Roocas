//******************************************************************************
//	 SYS_UART.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 02/28/2009
//******************************************************************************
#include "define.h"

#define BPS_4800	4800
#define BPS_9600	9600	
#define BPS_19200	19200
#define BPS_38400	38400
#define BPS_57600	57600
#define BPS_115200	115200
#define BPS_230400	230400
#define BPS_460800  460800
#define BPS_921600  921600
#define BPS_1843200	1843200
#define BPS_3686400	3686400

#define MAX_SERIAL_BUF      128
#define MAX_SERIAL_PORT     4

#define UART_UCA0		0
#define UART_UCA1		1
#define UART_UCA2       2
#define UART_UCA3       3

#define ECO1_UART_MODE		UART_UCA0
#define ECO2_UART_MODE		UART_UCA1
#define XBEE_UART_MODE		UART_UCA2
#define RFM_UART_MODE       UART_UCA2
#define GPS_UART_MODE		UART_UCA3
#define XSTREAM_UART_MODE	UART_UCA3
#define WIFI_UART_MODE      UART_UCA2

void SYS_InitVarUART(void);
void SYS_InitUART(BYTE bConfig, DWORD dwClock, DWORD dwBps);
void SYS_WriteByteUART(BYTE bConfig, BYTE bChar);
void SYS_WriteStringUART(BYTE bConfig, BYTE *pStr);
void SYS_WriteFrameUART(BYTE bConfig, BYTE *pStr, int iLen);
BOOL SYS_ReadByteUART(BYTE bConfig, char *pChar);
unsigned char SYS_Is_WaitForUART2TXBufferReady(void);
BYTE SYS_GetUARTBUF_RdIdx(BYTE bConfig);

typedef struct _SERIAL_BUF {
	BYTE bWrIndex;
	BYTE bRdIndex;
	BYTE bOverflow;
	BYTE bRxCount;
	char cData[MAX_SERIAL_BUF];	
} SERIAL_BUF;


