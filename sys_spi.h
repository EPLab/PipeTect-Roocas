//******************************************************************************
//	 SYS_SPI.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 02/28/2009
//******************************************************************************
#include "define.h"

#define MAX_SPI_BUF 100
#define MAX_SPI_PORT    4

#define SPI_UCB0		0
#define SPI_UCB1		1
#define SPI_UCB2		2
#define SPI_UCB3		3

#define CAN_SPI_MODE	SPI_UCB0
#define ECO1_SPI_MODE	SPI_UCB0	
#define MMC_SPI_MODE	SPI_UCB1
#define FRAM_SPI_MODE	SPI_UCB1
#define ECO2_SPI_MODE	SPI_UCB2
#define RTC_SPI_MODE    SPI_UCB3 

#define SPI_DUMMY_DATA	0x00

void SYS_InitSPI(BYTE bConfig);
void SYS_WriteByteSPI(BYTE bConfig, BYTE bChar);
BOOL SYS_ReadByteSPI(BYTE bConfig, BYTE *pChar);
BYTE SYS_ReadFrameSPI(BYTE bConfig, BYTE bDummy, BYTE *pBuffer, WORD wSize);
BYTE SYS_WriteFrameSPI(BYTE bConfig, BYTE *pBuffer, WORD wSize);
//void SYS_InitVarSPI(void);

typedef struct _SPI_BUF
{
	BYTE bWrIndex;
	BYTE bRdIndex;
	BYTE bOverflow;
	BYTE bRxCount;
	char cData[MAX_SPI_BUF];	
} SPI_BUF;
