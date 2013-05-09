//******************************************************************************
//	 XBEE.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 02/28/2009
//******************************************************************************
#include "define.h"

void XBEE_Test(void);
void XBEE_Initialize(void);
void XBEE_Enable(void);
void XBEE_Disable(void);
void XBEE_Reset(void);
void XBEE_Wakeup(void);
void XBEE_Sleep(void);
BYTE XBEE_ReadByte(char *pData);
void XBEE_WriteFrame(char *pData, int iLen);

typedef struct _XBEE_{
	int nState;						// Current state protocol parser is in
	BYTE bChecksum;					// Calculated sentence checksum
	BYTE bReceivedChecksum;			// Received sentence checksum (if exists)
	WORD wIndex;					// Index used for command and data
	BYTE pCommand[MAX_CMD_LEN];		// XSTREAM command
	BYTE pData[MAX_DATA_LEN];		// XSTREAM data	
	DWORD dwCommandCount;			// number of commands received (processed or not processed)
} XBEE;

