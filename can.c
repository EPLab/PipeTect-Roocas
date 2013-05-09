//******************************************************************************
//	 CAN.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 04/16/2009
//******************************************************************************
#include "can.h"
#include "can_reg.h"
#include "sys_spi.h"

void CAN_WakeUp(void)
{
    BYTE tmp;
    
    // when it wakes up, the mode is listen-only mode
    // gotta change the mode
    CAN_ChangeMode(CAN_NORMAL_MODE);
    CAN_ReadByte(CANSTAT, &tmp);
    while ((tmp & 0xE0) != OPMODE_NORMAL)
    {
        CAN_ChangeMode(CAN_NORMAL_MODE);
        CAN_ReadByte(CANSTAT, &tmp);
    }
    __no_operation();
}

void CAN_Sleep(void)
{
    BYTE tmp;
    
    CAN_ChangeMode(CAN_SLEEP_MODE);
    CAN_ReadByte(CANSTAT, &tmp);
    while ((tmp & 0xE0) != OPMODE_SLEEP)
    {
        CAN_ChangeMode(CAN_SLEEP_MODE);
        CAN_ReadByte(CANSTAT, &tmp);
    }
    __no_operation();
}

void CAN_Initialize(void)
{ 
    WORD flt[6]= {0, 0, 0, 0, 0, 0};
    BYTE tmp;
    
    P5SEL &= ~0x01;
    P5DIR |= 0x01;
    P5OUT &= ~0x01;
    
    CAN_Reset();

    CAN_ReadByte(CANSTAT, &tmp);
    CAN_ChangeMode(CAN_CONFIG_MODE);
    CAN_ReadByte(CANSTAT, &tmp);

    CAN_ModifyBit(CNF1, 0xFF, (SJW_1TQ + CAN_1MBPS));
    CAN_ModifyBit(CNF2, 0xFF, (BTLMODE_CNF3 + PHSEG1_3TQ + PRSEG_1TQ));
    CAN_ModifyBit(CNF3, 0xFF, PHSEG2_3TQ);

    CAN_ModifyBit(BFPCTRL, 0x3F, 0x3C);	//RX0BF, RX1BF = digital output
    CAN_ModifyBit(BFPCTRL, 0x30, 0x00); //LED ON
    CAN_ModifyBit(CANINTE, 0xFF, 0x41);	//configure interrupt masks receiving int, bus activity interrupt
    CAN_WriteByte(CANINTF, 0x00);
    CAN_ModifyBit(RXB0CTRL, 0x6F, 0x64);	//configure receive buffer0(roll over enabled)
    CAN_ModifyBit(RXB1CTRL, 0x6F, 0x60);	//configure receive buffer1
	
    //Set LOOPBACK mode
    CAN_ChangeMode(CAN_NORMAL_MODE);
    CAN_ReadByte(CANSTAT, &tmp); //dummy read to give 2515 time to switch to normal mode

    while((tmp & 0xE0) != OPMODE_NORMAL)
    {
	    CAN_ChangeMode(CAN_NORMAL_MODE);
        CAN_ReadByte(CANSTAT, &tmp);
    }

    CAN_SetRXB0Filters(0, &flt[0]);
    CAN_SetRXB1Filters(0, &flt[2]);
    
    CAN_INT_PxSEL &= ~CAN_INT_BIT;
    CAN_INT_PxDIR &= ~CAN_INT_BIT;   
}

void CAN_TrancevierOn(void)
{
    P5OUT &= ~0x01;
}

void CAN_TrancevierOff(void)
{
    P5OUT |= 0x01;
}

BOOL CAN_SendMsg(WORD Identifier, BYTE *pMsg, BYTE MsgSize )
{
	BYTE tmp;
	int i;
	
	for (i=0; i<CAN_TX_TIMEOUT; i++) 
    {
		// wait for TXB0 to get ready.  If not ready within XMIT_TIMEOUT ms,then return false
		CAN_ReadByte(TXB0CTRL, &tmp);
		if ((tmp & TXREQ) == CLEAR) break;
	}
	if (i == CAN_TX_TIMEOUT) return FALSE;

	CAN_WriteByte(TXB0SIDH, ((Identifier >> 3) & 0xFF));    //Set TXB0 SIDH
	CAN_WriteByte(TXB0SIDL, ((Identifier << 5) & 0xE0));    //Set TXB0 SIDL
	
	if (MsgSize > CAN_MAX_MSG)
		MsgSize = CAN_MAX_MSG;
	
	CAN_WriteByte(TXB0DLC, MsgSize);  //Set DLC
	
	for (i = 0; i < MsgSize; i++)
		CAN_WriteByte((TXB0D0 + i), pMsg[i]);
		
	CAN_WriteByte(TXB0CTRL, 0x08);	//Start Transmission.
		
	return TRUE;
}	

BOOL CAN_GetMsg(WORD *pIdentifier, BYTE *pMsg, BYTE *pMsgSize)
{
    int i;
    BYTE tmp;
    BYTE S1, S2;

    for (i=0; i<CAN_RX_TIMEOUT; i++) 
    {
    	CAN_ReadByte(CANINTF, &tmp);
	if ((tmp & 0x03) != CLEAR) break;
    }
    if (i == CAN_RX_TIMEOUT) return FALSE;
	
//   CAN_ReadStatus(&tmp);
//   if ((tmp & 0x40) == CLEAR) return FALSE;
    
    if (tmp & 0x01)
    {
        CAN_ReadByte(RXB0DLC, &tmp);
        *pMsgSize  = (tmp & 0x0F) ; //Data Length
    
        if (*pMsgSize > CAN_MAX_MSG)
            *pMsgSize = CAN_MAX_MSG;
            
        CAN_ReadByte(RXB0SIDH, &S1);
        CAN_ReadByte(RXB0SIDL, &S2);
                    
        *pIdentifier = ((S1 << 3) | (S2 >> 5)); //format the 11 bit identifier
            
        for (i = 0; i < *pMsgSize; i++)
            CAN_ReadByte(RXB0D0 + i, &pMsg[i]);
                    
        CAN_ModifyBit(CANINTF, 0x01, 0x00);
    }
    else if (tmp & 0x02)
    {
        CAN_ReadByte(RXB1DLC, &tmp);
        *pMsgSize  = (tmp & 0x0F) ; //Data Length
    
        if (*pMsgSize > CAN_MAX_MSG)
            *pMsgSize = CAN_MAX_MSG;
            
        CAN_ReadByte(RXB1SIDH, &S1);
        CAN_ReadByte(RXB1SIDL, &S2);
                    
        *pIdentifier = ((S1 << 3) | (S2 >> 5)); //format the 11 bit identifier
            
        for (i = 0; i < *pMsgSize; i++)
            CAN_ReadByte(RXB1D0 + i, &pMsg[i]);
                    
        CAN_ModifyBit(CANINTF, 0x02, 0x00);
    }
    return TRUE;
}

void CAN_Reset(void)
{
	int i;
	
    CAN_RESET_PxOUT &= (~CAN_RESET_BIT);
	for (i = 0; i <1000; i++);
	CAN_RESET_PxOUT |= CAN_RESET_BIT;
	
	CAN_EnableCS();
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_RESET);
	CAN_DisableCS();
}

void CAN_SetRXB0Filters(WORD Mask0, WORD *pFlt0_1)
{
	CAN_WriteByte(RXM0SIDH, (Mask0 >> 3));
	CAN_WriteByte(RXM0SIDL, (Mask0 << 5));

	// Set two filters associated with RXB0	
	CAN_WriteByte(RXF0SIDH, (*pFlt0_1 >> 3));
	CAN_WriteByte(RXF0SIDL, (*pFlt0_1 << 5));
	
	pFlt0_1++;
	CAN_WriteByte(RXF1SIDH, (*pFlt0_1 >> 3));
	CAN_WriteByte(RXF1SIDL, (*pFlt0_1 << 5));
}

void CAN_SetRXB1Filters(WORD Mask1, WORD *pFlt2_5)
{
	CAN_WriteByte(RXM1SIDH, (Mask1 >> 3));
	CAN_WriteByte(RXM1SIDL, (Mask1 << 5));

	// Set Four filters associated with RXB1	
	CAN_WriteByte(RXF2SIDH, (*pFlt2_5 >> 3));
	CAN_WriteByte(RXF2SIDL, (*pFlt2_5 << 5));
	
	pFlt2_5++;
	CAN_WriteByte(RXF3SIDH, (*pFlt2_5 >> 3));
	CAN_WriteByte(RXF3SIDL, (*pFlt2_5 << 5));
	
	pFlt2_5++;
	CAN_WriteByte(RXF4SIDH, (*pFlt2_5 >> 3));
	CAN_WriteByte(RXF4SIDL, (*pFlt2_5 << 5));

	pFlt2_5++;
	CAN_WriteByte(RXF5SIDH, (*pFlt2_5 >> 3));
	CAN_WriteByte(RXF5SIDL, (*pFlt2_5 << 5));
}

BOOL CANTxReady(void)
{
	BYTE tmp;
	
	CAN_ReadStatus(&tmp);
	if ((tmp & 0x04) == 0) return TRUE;
	else return FALSE;
}	

BOOL CANRxReady(void)
{
	BYTE tmp;

	CAN_ReadStatus(&tmp);
	if ((tmp & 0x03) != 0) return TRUE;
	else return FALSE;
}	

void CAN_ChangeMode(BYTE Mode)
{
	CAN_ModifyBit(CANCTRL, 0xE0, Mode);
}

void CAN_ModifyBit(BYTE Address, BYTE Mask, BYTE Value)
{
	CAN_EnableCS();
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_BIT_MODIFY);
	SYS_WriteByteSPI(CAN_SPI_MODE, Address);
	SYS_WriteByteSPI(CAN_SPI_MODE, Mask);
	SYS_WriteByteSPI(CAN_SPI_MODE, Value);
	CAN_DisableCS();	
}

void CAN_ReadStatus(BYTE *pValue)
{
	CAN_EnableCS();
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_RD_STATUS);
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_DUMMY_CHAR);
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_DUMMY_CHAR);
	SYS_ReadByteSPI(CAN_SPI_MODE, pValue);
	CAN_DisableCS();
}

void CAN_WriteByte(BYTE Addr, BYTE Value)
{
	CAN_EnableCS();
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_WRITE);
	SYS_WriteByteSPI(CAN_SPI_MODE, Addr);
	SYS_WriteByteSPI(CAN_SPI_MODE, Value);
	CAN_DisableCS();
}

void CAN_ReadByte(BYTE Addr, BYTE *pValue)
{
	CAN_EnableCS();
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_READ);
	SYS_WriteByteSPI(CAN_SPI_MODE, Addr);
	SYS_WriteByteSPI(CAN_SPI_MODE, CAN_DUMMY_CHAR);
	SYS_ReadByteSPI(CAN_SPI_MODE, pValue);
	CAN_DisableCS();
}

void CAN_Retransmit(BYTE Address)
{
	CAN_EnableCS();
	SYS_WriteByteSPI(CAN_SPI_MODE, (0x80 | Address));
	CAN_DisableCS();
}

void CAN_EnableCS(void)
{
  	CAN_CS_PxOUT &= (~CAN_CS_BIT);
}

void CAN_DisableCS(void)
{
  	CAN_CS_PxOUT |= CAN_CS_BIT;
}

BOOL CAN_SendExtMsg(BYTE StdId, BYTE Cmd, WORD ExtId, BYTE *pMsg, BYTE MsgSize)
{
    BYTE tmp;
    int i;
	
    for (i=0; i<CAN_TX_TIMEOUT; i++) 
    {
	// wait for TXB0 to get ready.  If not ready within XMIT_TIMEOUT ms,then return false
	CAN_ReadByte(TXB0CTRL, &tmp);
	if ((tmp & TXREQ) == CLEAR) break;
    }
    if (i == CAN_TX_TIMEOUT) return FALSE;

    CAN_WriteByte(TXB0SIDH, StdId);    //Set TXB0 SIDH
    tmp = ((Cmd & 0x1C) << 3) + EXIDE_SET + (Cmd & 0x03);
    CAN_WriteByte(TXB0SIDL, tmp);    //Set TXB0 SIDL

    tmp = ((WORD)(ExtId >> 8)) & 0xFF;
    CAN_WriteByte(TXB0EID8, tmp);    //Set TXB0 EID8
    CAN_WriteByte(TXB0EID0, (ExtId & 0xFF));    //Set TXB0 EID0
	
    if (MsgSize > CAN_MAX_MSG)
	MsgSize = CAN_MAX_MSG;
	
    CAN_WriteByte(TXB0DLC, MsgSize);  //Set DLC
	
    for (i = 0; i < MsgSize; i++)
	CAN_WriteByte((TXB0D0 + i), pMsg[i]);
		
    CAN_WriteByte(TXB0CTRL, 0x08);	//Start Transmission.
		
    return TRUE;
}	

BOOL CAN_GetExtMsg(BYTE *pStdId, BYTE *pCmd, WORD *pExtId, BYTE *pMsg, BYTE *pMsgSize)
{
    int i;
    BYTE S, E1, E2, tmp;

    //for (i=0; i<CAN_RX_TIMEOUT; i++) 
    {
    	CAN_ReadByte(CANINTF, &tmp);
	//if ((tmp & 0x03) != CLEAR) break;
    }
    //if (i == CAN_RX_TIMEOUT) return FALSE;

//	CAN_ReadStatus(&tmp);
//	if ((tmp & 0x40) == CLEAR) return FALSE;
    
    if (tmp & 0x01)
    {
        CAN_EnableCS();
        SYS_WriteByteSPI(CAN_SPI_MODE, 0x90);
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, pStdId);
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, &S);
        *pCmd = ((S >> 3) | (S & 0x03));
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, &E1);
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, &E2);
        *pExtId = ((E1 << 8) | E2); //format the 18 bit identifier
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, pMsgSize);
        
        if (*pMsgSize > CAN_MAX_MSG)
            *pMsgSize = CAN_MAX_MSG;
        
        for (i = 0; i < *pMsgSize; i++)
        {
            SYS_WriteByteSPI(CAN_SPI_MODE, 0);
            SYS_ReadByteSPI(CAN_SPI_MODE, &pMsg[i]);
        }
        CAN_DisableCS();
    }
    if (tmp & 0x02)
    {
        CAN_EnableCS();
        SYS_WriteByteSPI(CAN_SPI_MODE, 0x94);
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, pStdId);
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, &S);
        *pCmd = ((S >> 3) | (S & 0x03));
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, &E1);
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, &E2);
        *pExtId = ((E1 << 8) | E2); //format the 18 bit identifier
        SYS_WriteByteSPI(CAN_SPI_MODE, 0);
        SYS_ReadByteSPI(CAN_SPI_MODE, pMsgSize);
        
        if (*pMsgSize > CAN_MAX_MSG)
            *pMsgSize = CAN_MAX_MSG;
        
        for (i = 0; i < *pMsgSize; i++)
        {
            SYS_WriteByteSPI(CAN_SPI_MODE, 0);
            SYS_ReadByteSPI(CAN_SPI_MODE, &pMsg[i]);
        }
        CAN_DisableCS();
    }
    return TRUE;
}
