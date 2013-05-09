//******************************************************************************
//	 XSTREAM.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 04/10/2009
//******************************************************************************
#include "define.h"

void XSTREAM_Test(void);
void XSTREAM_Process(void);
void XSTREAM_Initialize(void);
void XSTREAM_Reset(void);
void XSTREAM_Enable(void);
void XSTREAM_Disable(void);
void XSTREAM_Wakeup(void);
void XSTREAM_Sleep(void);
BYTE XSTREAM_ReadByte(char *pData);
void XSTREAM_WriteFrame(char *pData, int iLen);

typedef struct _XSTREAM_{
	int nState;						// Current state protocol parser is in
	BYTE bChecksum;					// Calculated sentence checksum
	BYTE bReceivedChecksum;			// Received sentence checksum (if exists)
	WORD wIndex;					// Index used for command and data
	BYTE pCommand[MAX_CMD_LEN];		// XSTREAM command
	BYTE pData[MAX_DATA_LEN];		// XSTREAM data	
	DWORD dwCommandCount;			// number of commands received (processed or not processed)
} XSTREAM;
