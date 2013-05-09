//******************************************************************************
//	 UIF.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 08/10/2009
//******************************************************************************
#include "uif.h"
#include "sys.h"
#include "sys_uart.h"
#include "xstream.h"
#include "xbee.h"
#include "rfm.h"
#include "wifi.h"

////////////////////////////////////////////////////////////////////////////////
//  YEB (Start)
////////////////////////////////////////////////////////////////////////////////
volatile UIF_STATE mUIFCommState;
UIF_STRUCT mUIFComm = {0};
BYTE header[UIF_HEADER_LENGTH];
volatile BYTE countByteDataRecorded;
volatile unsigned short temp_chksum;
//void (*UIF_TODO[MAX_UIF_COMMAND])(void* any) = NULL;

static BYTE UIF_InterpreteHeader(BYTE* header);

////////////////////////////////////////////////////////////////////////////////
//  YEB (End)
////////////////////////////////////////////////////////////////////////////////


BYTE UIF_COMMAND_INFO[MAX_UIF_COMMAND+1][3] =
{
	// command[1], command[0], length
	{'A', 'S', 1},		// Start acceleration data
	{'A', 'D', 10},		// Send acceleration data
	{'A', 'F', 1},		// Stop transmission of acceleration data
	{'A', 'T', 9},		// Send time data
	{'A', 'R', 2},		// Retry transmission of acceleration data
	{'C', 'T', 1},		// Take camera image
	{'C', 'S', 3},		// Start camera image
	{'C', 'D', 11},		// Send image data
	{'C', 'R', 3},		// Retransmit image data
        {'C', 'M', 0},
        {'H', 'A', 1},             // Ack of Heartbeat message
	 {'H', 'B', 1}
};


////////////////////////////////////////////////////////////////////////////////
//  YEB (Start)
////////////////////////////////////////////////////////////////////////////////

UIF_STRUCT *UIF_TrackCommState(void)
{
    char data;

#ifdef RFM_ENABLE
    if (RFM_ReadByte(&data) == FALSE) return FALSE;
#endif

#ifdef XBEE_ENABLE
    if (XBEE_ReadByte(&data) == FALSE) return FALSE;
#endif

#ifdef XSTREAM_ENABLE
    if (XSTREAM_ReadByte(&data) == FALSE) return FALSE;
#endif

#ifdef WIFI_ENABLE
    if (WIFI_ReadByte(&data) == FALSE) return FALSE;
#endif

    switch (mUIFCommState)
    {
        case NONE:
        case REC_COMPLETED:
            if (data == UIF_COMMAND_HEADER)
            {
                mUIFCommState = DELIMITER_RECEIVED;
                mUIFComm.chkSum = UIF_COMMAND_HEADER;
            }
            else if (data == 'C')
            {
                mUIFCommState = POSSIBLE_CONF_MODE;
            }
            break;
        case DELIMITER_RECEIVED:
            header[0] = data;
            mUIFCommState = FIRST_MNE_RECEIVED;
            mUIFComm.chkSum += data;
            break;
        case FIRST_MNE_RECEIVED:
            header[1] = data;
            mUIFCommState = SECOND_MNE_RECEIVED;
            mUIFComm.chkSum += data;
            break;
        case SECOND_MNE_RECEIVED:
            mUIFComm.addr = data;
            mUIFComm.dataLength = UIF_InterpreteHeader(header);
            mUIFCommState = ADDR_RECEIVED;
            mUIFComm.chkSum += data;
            countByteDataRecorded = 0;
            break;
        case ADDR_RECEIVED:
            if (mUIFComm.dataLength > (countByteDataRecorded + 1))
            {
                mUIFComm.data[countByteDataRecorded++] = data;
                mUIFComm.chkSum += data;
                break;
            }
            temp_chksum = data;
            temp_chksum <<= 8;
            mUIFCommState = CHECKSUM_RECEIVED;
            break;
        case CHECKSUM_RECEIVED:
            temp_chksum |= data;
            if (mUIFComm.chkSum == temp_chksum)
            {
                mUIFCommState = REC_COMPLETED;
                return &mUIFComm;
            }
            else
            {
                __no_operation();
            }
        case POSSIBLE_CONF_MODE:
            if ((data == 'M') || (data == 'm'))
            {
                mUIFCommState = ALMOST_CONF_MODE;
            }
            else
            {
                if (data == UIF_COMMAND_HEADER)
                {
                    mUIFCommState = DELIMITER_RECEIVED;
                    mUIFComm.chkSum = UIF_COMMAND_HEADER;
                }
                else
                {
                    mUIFCommState = NONE;
                }
            }
            break;
        case ALMOST_CONF_MODE:
            if ((data == 'D') || (data == 'd'))
            {
                mUIFCommState = CONF_MODE_2MORE_2GO;
            }
            else
            {
                if (data == UIF_COMMAND_HEADER)
                {
                    mUIFCommState = DELIMITER_RECEIVED;
                    mUIFComm.chkSum = UIF_COMMAND_HEADER;
                }
                else
                {
                    mUIFCommState = NONE;
                }
            }
            break;
        case CONF_MODE_2MORE_2GO:
            if (data == 0x0D)
            {
                mUIFCommState = CONF_MODE_1MORE_2GO;
            }
            else
            {
                if (data == UIF_COMMAND_HEADER)
                {
                    mUIFCommState = DELIMITER_RECEIVED;
                    mUIFComm.chkSum = UIF_COMMAND_HEADER;
                }
                else
                {
                    mUIFCommState = NONE;
                }
            }
            break;
        case CONF_MODE_1MORE_2GO:
            if (data == 0x0A)
            {
                mUIFCommState = REC_COMPLETED;
                mUIFComm.headerEnum = 9;
                return &mUIFComm;
            }
            else
            {
                if (data == UIF_COMMAND_HEADER)
                {
                    mUIFCommState = DELIMITER_RECEIVED;
                    mUIFComm.chkSum = UIF_COMMAND_HEADER;
                }
                else
                {
                    mUIFCommState = NONE;
                }
            }
            break;
        default:
            mUIFCommState = NONE;
    }
    return NULL;
}

BYTE UIF_InterpreteHeader(BYTE* str)
{
    BYTE i;

    for (i = 0; i < MAX_UIF_COMMAND; ++i)
    {
        if (memcmp(UIF_COMMAND_INFO[i], str, UIF_HEADER_LENGTH) == 0)
        {
            mUIFComm.headerEnum = i;
            return UIF_COMMAND_INFO[i][UIF_HEADER_LENGTH];
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  YEB (End)
////////////////////////////////////////////////////////////////////////////////

BYTE UIF_ReadCommand(BYTE *pCommand, char *pBuf)
{
	char data[3];
	BYTE i, j;

#ifdef RFM_ENABLE
    if (RFM_ReadByte(&data[0]) == FALSE) return FALSE;
#endif

#ifdef XBEE_ENABLE
    if (XBEE_ReadByte(&data[0]) == FALSE) return FALSE;
#endif

#ifdef XSTREAM_ENABLE
    if (XSTREAM_ReadByte(&data[0]) == FALSE) return FALSE;
#endif
	
	if (data[0] == UIF_COMMAND_HEADER)
	{
#ifdef RFM_ENABLE
        RFM_ReadByte(&data[1]);
        RFM_ReadByte(&data[2]);
#endif

#ifdef XBEE_ENABLE
        XBEE_ReadByte(&data[1]);
        XBEE_ReadByte(&data[2]);
#endif

#ifdef XSTREAM_ENABLE
        XSTREAM_ReadByte(&data[1]);
        XSTREAM_ReadByte(&data[2]);
#endif

#ifdef WIFI_ENABLE
        WIFI_ReadByte(&data[1]);
        WIFI_ReadByte(&data[2]);
#endif
		for (i=0; i<MAX_UIF_COMMAND; i++)
		{
			if ((data[1] == UIF_COMMAND_INFO[i][0]) && (data[2] == UIF_COMMAND_INFO[i][1]))
			{
				*pCommand = i;
				for (j=0; j<UIF_COMMAND_INFO[i][2]; j++)
				{
#ifdef RFM_ENABLE
                    RFM_ReadByte(&pBuf[i]);
#endif

#ifdef XBEE_ENABLE
                    XBEE_ReadByte(&pBuf[i]);
#endif

#ifdef XSTREAM_ENABLE
                    XSTREAM_ReadByte(&pBuf[i]);
#endif

#ifdef WIFI_ENABLE
                    WIFI_ReadByte(&pBuf[i]);
#endif
				} 	
                return TRUE;
			}
		}
	}
	return FALSE;
}

void UIF_WriteCommand(BYTE bCommand, BYTE *pBuf)
{
    BYTE i, len;
    unsigned short chksum;
    char buf[30];

    buf[0] = UIF_COMMAND_HEADER;
    buf[1] = UIF_COMMAND_INFO[bCommand][0];
    buf[2] = UIF_COMMAND_INFO[bCommand][1];
    buf[3] = UIF_COMMAND_INFO[bCommand][2];

    len = buf[3];
    for (i = 0; i < len; i++)
    {
        buf[i+4] = pBuf[i];
    }
    len += 4;
    //buf[len] = 0;
    chksum = 0;
    for (i=0; i<len; i++)
    {
        //buf[len] += buf[i];
        chksum += buf[i];
    }
    buf[i] = (char)(chksum >> 8);
    buf[i + 1] = (char)chksum;
    /*
    if (len < 16)
    {
        for (i = len + 1; i < 16; ++i)
        {
            buf[i] = 0;
        }
        len = i - 1;
    }
    */
#ifdef RFM_ENABLE
    RFM_WriteFrame(buf, len + 2);
	//UTX_enqueue(buf, len + 2);
#endif

#ifdef XBEE_ENABLE
    XBEE_WriteFrame(buf, len + 2);
#endif

#ifdef XSTREAM_ENABLE
    XSTREAM_WriteFrame(buf, len + 2);
#endif

#ifdef WIFI_ENABLE
    WIFI_WriteFrame(buf, len + 2);
#endif
}

void UIF_WriteByteArray(char* buf, BYTE len)
{
    #ifdef RFM_ENABLE
    RFM_WriteFrame(buf, len);
#endif

#ifdef XBEE_ENABLE
    XBEE_WriteFrame(buf, len);
#endif

#ifdef XSTREAM_ENABLE
    XSTREAM_WriteFrame(buf, len);
#endif

#ifdef WIFI_ENABLE
    WIFI_WriteFrame(buf, len);
#endif
}

BYTE UIF_CheckSum(BYTE bCommand, BYTE *pBuf)
{
	BYTE i;
	BYTE chk_sum;
	
	chk_sum = UIF_COMMAND_HEADER;
	chk_sum += UIF_COMMAND_INFO[bCommand][0];
	chk_sum += UIF_COMMAND_INFO[bCommand][1];

	for (i=0; i<UIF_COMMAND_INFO[bCommand][2]; i++)
		chk_sum += pBuf[i];

	if (chk_sum == pBuf[i]) return TRUE;
	else return FALSE;
}
