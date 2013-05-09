//******************************************************************************
//	 ACC.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 04/16/2009
//******************************************************************************
#include "define.h"

#define MAX_CAM_TIMEOUT	    10

#define CAN_TAKE_CAM_PIC	9
#define CAN_REQ_CAM_SIZE	10
#define CAN_START_CAM_DATA	11			
#define CAN_END_CAM_DATA	12
#define CAN_REQ_CAM_DATA	13

void CAM_Test(void);
void CAM_Initialize(void);
BYTE CAM_Start(BYTE NodeId);
BYTE CAM_SendData(BYTE NodeId);


