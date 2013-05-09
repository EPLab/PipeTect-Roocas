//******************************************************************************
//	 CAN.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 04/16/2009
//******************************************************************************
#include "define.h"


#define CAN_ID_MASK		0x03FF
#define CAN_ID_NO_MASK	0

#define CAN_ID_ALL		0xFF
#define CAN_ID_MAIN		0x00
#define CAN_ID_ACC1		0x01
#define CAN_ID_ACC2		0x02
#define CAN_ID_ACC3		0x03

#define CAN_DUMMY_CHAR	0x00

#define CAN_TX_TIMEOUT	1000
#define CAN_RX_TIMEOUT	10
#define CAN_MAX_MSG		8

#define CAN_TX_DATA_LEN	8		
#define CAN_RX_DATA_LEN	8		

////////////////////////////////////////////////////////////////////////////////
//  YEB (Start)
////////////////////////////////////////////////////////////////////////////////

//#define MAX_ACC_NODE	4
//#define MAX_ACC_AXIS	3

////////////////////////////////////////////////////////////////////////////////
//  YEB (End)
////////////////////////////////////////////////////////////////////////////////
#define CAN_REQ_ACC_DATA	0
#define CAN_START_ACC_DATA	1
#define CAN_STOP_ACC_DATA	2
#define CAN_TAKE_CAM_PIC	9
#define CAN_REQ_CAM_SIZE	10
#define CAN_START_CAM_DATA	11			
#define CAN_END_CAM_DATA	12
#define CAN_REQ_CAM_DATA	13

void CAN_Test(void);
void CAN_Initialize(void);
void CAN_Reset(void);
void CAN_SetRXB0Filters(WORD Mask0, WORD *pFlt0_1);
void CAN_SetRXB1Filters(WORD Mask1, WORD *pFlt2_5);
void CAN_Enable(int BusSpeed);
BOOL CAN_TxReady(void);
BOOL CAN_RxReady(void);
BOOL CAN_SendMsg(WORD Identifier, BYTE *pMsg, BYTE MsgSize);
BOOL CAN_GetMsg(WORD *pIdentifier, BYTE *pMsg, BYTE *pMsgSize);
BOOL CAN_SendExtMsg(BYTE StdId, BYTE Cmd, WORD ExtId, BYTE *pMsg, BYTE MsgSize);
BOOL CAN_GetExtMsg(BYTE *pStdId, BYTE *pCmd, WORD *pExtId, BYTE *pMsg, BYTE *pMsgSize);
void CAN_ReadStatus(BYTE *pValue);
void CAN_ReadByte(BYTE Addr, BYTE *pValue);
void CAN_WriteByte(BYTE Addr, BYTE Value);
BYTE CAN_TransferSPI(BYTE Data);
void CAN_ChangeMode(BYTE Mode);
void CAN_ModifyBit(BYTE Address, BYTE Mask, BYTE Value);
void CAN_Retransmit(BYTE Address);
void CAN_EnableCS(void);
void CAN_DisableCS(void);
extern void CAN_TrancevierOn(void);
extern void CAN_TrancevierOff(void);
extern void CAN_Sleep(void);
extern void CAN_WakeUp(void);
