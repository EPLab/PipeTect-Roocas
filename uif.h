//******************************************************************************
//	 UIF.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 08/10/2009
//******************************************************************************
#ifndef __UIF_H
#define __UIF_H

#include "define.h"

////////////////////////////////////////////////////////////////////////////////
//  YEB (Start)
////////////////////////////////////////////////////////////////////////////////

#define UIF_MAX_COMM_DATA_LENGTH 10

typedef struct _UIF_STRUCT 
{
    BYTE headerEnum;
    BYTE addr;
    BYTE data[UIF_MAX_COMM_DATA_LENGTH];
    BYTE dataLength;
    unsigned short chkSum;
} UIF_STRUCT;

////////////////////////////////////////////////////////////////////////////////
//  YEB (End)
////////////////////////////////////////////////////////////////////////////////

typedef enum _UIF_COMMAND
{
	UIF_START_ACC = 0,
	UIF_SEND_ACC,
	UIF_STOP_ACC,
    UIF_TIME_ACC,
    UIF_RETRY_ACC,
	UIF_TAKE_CAM,
	UIF_START_CAM,
	UIF_SEND_CAM,
	UIF_RETRY_CAM,
	UIF_CMD,
    UIF_HB_ACK,
	UIF_HB,
        
	MAX_UIF_COMMAND
} UIF_COMMAND;

typedef enum _UIF_STATE 
{
    NONE = 0,
    DELIMITER_RECEIVED,
    FIRST_MNE_RECEIVED,
    SECOND_MNE_RECEIVED,
    ADDR_RECEIVED,
    CHECKSUM_RECEIVED,
    REC_COMPLETED,
    
    POSSIBLE_CONF_MODE,
    ALMOST_CONF_MODE,
    CONF_MODE_2MORE_2GO,
    CONF_MODE_1MORE_2GO
        
} UIF_STATE;

#define UIF_HEADER_LENGTH 2
#define UIF_COMMAND_HEADER '{'
#define UIF_COMMAND_FOOTER '}'     

extern UIF_STRUCT *UIF_TrackCommState(void);
BYTE UIF_InterpreteHeader(BYTE* header);
BYTE UIF_ReadCommand(BYTE *pCommand, char *pBuf);
void UIF_WriteCommand(BYTE bCommand, BYTE *pBuf);
extern void UIF_WriteByteArray(char* buf, BYTE len);
BYTE UIF_CheckSum(BYTE bCommand, BYTE *pBuf);

#endif
