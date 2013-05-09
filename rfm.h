//******************************************************************************
//	 RFM.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 12/10/2009
//******************************************************************************
#include "define.h"

void RFM_Test(void);
void RFM_Initialize(void);
void RFM_Reset(void);
void RFM_Sleep(void);
void RFM_Wakeup(void);
BYTE RFM_CheckStatus(void);
BYTE RFM_ReadByte(char *pData);
void RFM_WriteFrame(char *pData, int iLen);

typedef struct _RFM_{
	int nState;						// Current state protocol parser is in
	BYTE bChecksum;					// Calculated sentence checksum
	BYTE bReceivedChecksum;			// Received sentence checksum (if exists)
	WORD wIndex;					// Index used for command and data
	BYTE pCommand[MAX_CMD_LEN];		// XSTREAM command
	BYTE pData[MAX_DATA_LEN];		// XSTREAM data	
	DWORD dwCommandCount;			// number of commands received (processed or not processed)
} RFM;
