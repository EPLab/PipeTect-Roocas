//******************************************************************************
//	 CAM.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 08/06/2009
//******************************************************************************
#include "cam.h"
#include "acc.h"
#include "can.h"
#include "uif.h"

extern ACC_SENSOR g_AccSensor[];
extern BYTE g_CanTxMsg[];
extern BYTE g_CanRxMsg[];
extern BYTE g_Str[];    

void CAM_Test(void)
{
    BYTE node_id = CAN_ID_ACC1;
    WORD ext_id;
	BYTE std_id;
	BYTE cmd;
	BYTE len;
	WORD seq_no;
	int i;

	g_AccSensor[node_id].RxDone = CLEAR;
	g_CanTxMsg[0] = CAN_TAKE_CAM_PIC;
	CAN_SendExtMsg(node_id, CAN_TAKE_CAM_PIC, 0, g_CanTxMsg, 1);
 	while (1)
 	{
	 	if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
	 	{
 			if ((cmd == CAN_TAKE_CAM_PIC) && (std_id == CAN_ID_MAIN))
 			{
 				g_AccSensor[node_id].SeqNo = ((g_CanRxMsg[1] << 8) | g_CanRxMsg[0]);
 				break;
 			}
	 	}
 	}

 	seq_no = 0;
 	while (g_AccSensor[node_id].RxDone == CLEAR) {
		ext_id = seq_no;
		g_CanTxMsg[0] = CAN_START_CAM_DATA;
		CAN_SendExtMsg(node_id, CAN_START_CAM_DATA, ext_id, g_CanTxMsg, 1);
		for (i=0; i<100; i++);
 		while (1)
 		{
	 		if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
	 		{
		 		if ((cmd == CAN_START_CAM_DATA) && (std_id == CAN_ID_MAIN))
	 			{
					if (seq_no == ext_id)
					{
						g_AccSensor[node_id].CurSeq = (seq_no++);
//						XSTREAM_SendData(g_CanRxMsg, len);
//						XBEE_SendData((char *)g_CanRxMsg, len);
					}
					break;
	 			}
	 		}
 		}
 					
		if (seq_no == g_AccSensor[node_id].SeqNo)
 		{
			g_AccSensor[node_id].RxDone = SET;
			g_CanTxMsg[0] = CAN_END_CAM_DATA;
			CAN_SendExtMsg(node_id, CAN_END_CAM_DATA, 0, g_CanTxMsg, 1);
		}
 	}	
}

void CAM_Initialize(void)
{
    CAN_Initialize();  
}

BYTE CAM_Start(BYTE NodeId)
{
	BYTE std_id;
	WORD ext_id;
	BYTE cmd;
	BYTE msg[8];
	BYTE len;
	BYTE i;
    BYTE buf[8];

	g_AccSensor[NodeId].RxDone = CLEAR;
	g_CanTxMsg[0] = CAN_TAKE_CAM_PIC;
	CAN_SendExtMsg(CAN_ID_ACC1, CAN_TAKE_CAM_PIC, 0, g_CanTxMsg, 1);
	for (i=0; i<MAX_CAM_TIMEOUT; i++)
 	{
	 	if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
	 	{
 			if ((cmd == CAN_TAKE_CAM_PIC) && (std_id == NodeId))
 			{
 				g_AccSensor[NodeId].SeqNo = ((msg[1] << 8) | g_CanTxMsg[0]);
                buf[0] = NodeId;
                buf[1] = msg[1];
                buf[2] = msg[0];
                UIF_WriteCommand(UIF_START_CAM, buf);
 				return TRUE;
 			}
	 	}
 	}
	return FALSE;
}

BYTE CAM_SendData(BYTE NodeId)
{
	BYTE std_id;
	WORD ext_id;
	BYTE cmd;
	BYTE len;
	WORD seq_no = 0;
	BYTE i, j;

	ext_id = seq_no;
	g_CanTxMsg[0] = CAN_START_CAM_DATA;
 	while (g_AccSensor[NodeId].RxDone == CLEAR) 
    {
		CAN_SendExtMsg(CAN_ID_ACC1, CAN_START_CAM_DATA, ext_id, g_CanTxMsg, 1);
		for (i=0; i<MAX_CAM_TIMEOUT; i++)
 		{
	 		if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
	 		{
		 		if ((cmd == CAN_START_CAM_DATA) && (std_id == NodeId))
	 			{
					if (seq_no == ext_id)
					{
						g_AccSensor[NodeId].CurSeq = (seq_no++);
                        g_Str[0] = NodeId;
                        g_Str[1] = (ext_id >> 8);
                        g_Str[2] = (ext_id & 0xFF);
                        for (j=0; j<len; j++)
						{
                            g_Str[j+3] = g_CanRxMsg[j];
						}
                        //UIF_WriteCommand(UIF_SEND_CAM, g_Str);
						break;
					}
	 			}
	 		}
 		}
		if (i == MAX_ACC_TIMEOUT) return FALSE;
 					
		if (seq_no == g_AccSensor[NodeId].SeqNo)
 		{
			g_AccSensor[NodeId].RxDone = SET;
			g_CanTxMsg[0] = CAN_END_CAM_DATA;
			CAN_SendExtMsg(CAN_ID_ACC1, CAN_END_CAM_DATA, 0, g_CanTxMsg, 1);
		}
 	}
 	return TRUE;	
}

BYTE CAM_RetryDataCAM(BYTE NodeId)
{
	WORD ext_id;
	BYTE std_id;
	BYTE cmd;
	BYTE len;
	WORD seq_no = 0;
	BYTE i, j;

	ext_id = seq_no;
	g_CanTxMsg[0] = CAN_START_CAM_DATA;
	CAN_SendExtMsg(CAN_ID_ACC1, CAN_START_CAM_DATA, ext_id, g_CanTxMsg, 1);
	for (i=0; i<MAX_CAM_TIMEOUT; i++)
	{
 		if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
 		{
	 		if ((cmd == CAN_REQ_CAM_DATA) && (std_id == NodeId))
 			{
                g_Str[0] = NodeId;
                g_Str[1] = (ext_id >> 8);
                g_Str[2] = (ext_id & 0xFF);
                for (j=0; j<len; j++) 
                    g_Str[j+3] = g_CanRxMsg[j];
                UIF_WriteCommand(UIF_RETRY_CAM, g_Str);
				return TRUE;
			}
		}
	}
 	return FALSE;	
}


