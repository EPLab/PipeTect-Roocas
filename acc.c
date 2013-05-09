//******************************************************************************
//	 ACC.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 08/06/2009
//******************************************************************************
#include "acc.h"
#include "cam.h"
#include "can.h"
#include "can_reg.h"
#include "rtc.h"
#include "uif.h"
#include "rfm.h"
//#include "fatlib.h"
#include "assert.h"
#include "can_recv_queue.h"
#include "sys.h"
#include "switcher.h"
#include "local_store.h"
#include "convert.h"
#include "tuid.h"
#include "wifi.h"

#ifdef PROBE_ENABLED
#include "probe.h"
#endif

#define TEMP_MAX_FILES 4

extern ACC_SENSOR g_AccSensor[];
extern UIF_STRUCT *g_CommBuf;
extern BYTE g_CanTxMsg[];
extern BYTE g_CanRxMsg[];
extern BYTE g_Str[];
extern volatile WORD g_TimeFrameCount;
extern RTC_TIME g_StartTime, g_RTCTime;
//extern RTC_TIME g_FATDateTime;

//BYTE heartbeat_tid;
BYTE acc_tid = 0;

// counting missing ack
BYTE acc_started = 0;
BYTE ack_cnt = 0;

extern BYTE myRoocasID;
extern BYTE relayOnly;
extern BYTE send_ha;
BYTE can_len_on = 1;

// state variable
BYTE setTime = 0;
BYTE globalStart = 0;
BYTE globalStop = 0;
UIF_STRUCT fakeCmd = {0};

// test
int get_start = 0;
int get_time = 0;
int countDown = 10;
int send_time = 0;
int send_start = 0;

#ifdef __FILE_SAVE_ENABLED__
typedef struct bufFile
{
    BYTE data[2][512];
    WORD smallIndex;
    BYTE bigIndex;
} bufFile_t;

static bufFile_t mbf[TEMP_MAX_FILES];

static signed char g_FATHandle[TEMP_MAX_FILES];
static short g_SampleCount[TEMP_MAX_FILES] = {0};
#endif

//static unsigned short acc_count[8];
//static unsigned char acc_storage_cnt = 0;
//static unsigned short acc_storage[4][10];
static unsigned char sendTime = 0;
#define MISS_ACK_MAX 15

void ACC_Test(void)
{
    int i;

    ACC_SendTimeData(CAN_ID_ACC1);

    while (1)
    {
        ACC_RequestSensor(CAN_ID_ACC1);
        for (i = 0; i < 1000; i++);
    }
}

void ACC_SendHB(unsigned char tid, void* msg)
{
    ACC_SendTimeData(0);
}

static void ACC_SendTimeDataWithoutID(void)
{
    unsigned char i;

    sendTime = 1;
/*
    if (acc_storage_cnt < 10)
    {
        acc_storage[0][acc_storage_cnt] = acc_count[1];
        acc_storage[1][acc_storage_cnt] = acc_count[2];
        acc_storage[2][acc_storage_cnt] = acc_count[3];
        acc_storage[3][acc_storage_cnt++] = acc_count[4];
    }
    else
    {
		acc_storage_cnt  = 0;
        __no_operation();
    }
    for (i = 0; i < 8; ++i)
    {
        acc_count[i] = 0;
    }
*/
    // not sure how long the following takes
    RTC_ReadTime(&g_RTCTime);
}

void ACC_Initialize(void)
{
	//UTX_init();
    CAN_Initialize();
    acc_tid = registerTask(ACC_Process, 0, 0, 0xFF);
    RTC_SetEvery1SEC_WB(&ACC_SendTimeDataWithoutID);
    ACC_StopAll();
    SYS_DelayMS(2000);
    //ACC_StartAll();
    //heartbeat_tid = registerTask(ACC_SendHB, 0, 1000, 0xFF);
    //send_exit();
	//ACC_StartAll();
    // put mcp2515, 231 asleep
    //CAN_Sleep();
    //CAN_TrancevierOff();
}

void ACC_Process(unsigned char tid, void* msg)
{
	//static unsigned char p;
    BYTE node;
    BYTE i;
//    DATA16 msec;
//    BYTE tbuf[8];

#ifdef PROBE_ENABLED
	static unsigned p;
#endif

	//unsigned char data[] = "abcdefghijklmnopqrstuvwxyz\n";
    //automatic start
    //ACC_StartAll();
    //UTX_enqueue(data, sizeof(data));
	
	/*
    char data[] = "1234567890ABCDEFGHIJKLMNOPQ\r\n";
	char data2[32] = {0};
	
	MEM_WriteByte(131072L, data, 30);
	MEM_ReadByte(131072L, data2, 30);
	for (;;)
	{
		for (i = 0; i < sizeof(data2); ++i)
		{
			data2[i] = 0xFF;
		}
		FRAM_Write(131072 - 16, data, sizeof(data), &probe_high);
		while (FRAM_get_state() != 0);
		FRAM_Read(131072, data2, 16, &probe_low);
		while (FRAM_get_state() != 0);
		FRAM_Read(131072 - 16, data2, sizeof(data), &probe_low);
		while (FRAM_get_state() != 0);
		__no_operation();
	}
	*/
	
    if (sendTime & acc_started)
//	if (sendTime)
    {
        sendTime = 0;
        ACC_SendTimeData(0);
		if (can_len_on){
			CAN_ModifyBit(BFPCTRL, 0x30, 0x30);
			can_len_on = 0;
		} else{
			CAN_ModifyBit(BFPCTRL, 0x30, 0x00);
			can_len_on = 1;
		}	
		//if (globalStart){
		//	fakeCmd.headerEnum = 255;
		//	WIFI_RelayCMD(&fakeCmd);
	  	//}
		
		if (countDown){
		  countDown--;
		}else{	
		  countDown = 10;
		  get_start = 0;
		  get_time = 0;
		  send_time = 0;
		  send_start = 0;
		}
    }
	
	if (sendTime){
	  sendTime = 0;
	  //fakeCmd.addr = myRoocasID << 4;
	  //if (globalStart && !relayOnly){
		//fakeCmd.headerEnum = 255;
//		WIFI_RelayCMD(&fakeCmd);
	  //}
	  //if (globalStop && !relayOnly){
		//fakeCmd.headerEnum = 2;
		//WIFI_RelayCMD(&fakeCmd);
	  //}
		if (countDown){
		  countDown--;
		}else{
		  countDown = 10;
		  get_start = 0;
		  get_time = 0;
		  send_time = 0;
		  send_start = 0;
		}
    }

	
	if (!acc_started && !can_len_on){
		CAN_ModifyBit(BFPCTRL, 0x30, 0x00);
		can_len_on = 1;
	}

    //ACC_SendTimeData(node);
#ifdef PROBE_ENABLED
	p ^= 1;
	probe2_change(p);
#endif
    g_CommBuf = UIF_TrackCommState();
	//LOS_write(data, sizeof(data));
    if (g_CommBuf)
    {
        node = g_CommBuf->addr;
		if (relayOnly){
		  node = 0xF0;
		}
        switch (g_CommBuf->headerEnum)
        {
            case UIF_START_ACC :
//			  	if (node > (myRoocasID << 4)){
//					break;
//			  	}
			  	get_start++;
				globalStart = 1;
				globalStop = 0;
				ack_cnt = 0;
				WIFI_RelayCMD(g_CommBuf);
			  	if (relayOnly){
					break;
			  	}
				ACC_StartAll();
				//if (!acc_started && setTime){
				//	ACC_StartAll();
				//	ACC_SendTimeData(0);
				//}
                //for (i = 0; i < 8; ++i)
                //{
                //    acc_count[i] = 0;
                //}
				//WIFI_Stop_HB();
                break;
            case UIF_STOP_ACC :
				globalStart = 0;
				globalStop = 1;
				ack_cnt = 0;
				WIFI_RelayCMD(g_CommBuf);
			  	if (relayOnly){
					break;
			  	}
				ACC_StopAll();
				//WIFI_Start_HB();
                break;
            case UIF_TIME_ACC :
			  	get_time++;
			  	ack_cnt = 0;
			  	if (node == 0xF0){
					WIFI_RelayCMD(g_CommBuf);
					ACC_SetTime(node, g_CommBuf->data);
			  	} else if (((myRoocasID << 4) & (node & 0xF0))){
					ACC_SetTime(node, g_CommBuf->data);
				} else{
					// NOT FOR THIS ROOCAS
					WIFI_RelayCMD(g_CommBuf);
				}
                //acc_started = 1;
                break;
            case UIF_RETRY_ACC :
                break;
            case UIF_TAKE_CAM :
            case UIF_START_CAM :
                CAM_Start(node);
                break;
            case UIF_SEND_CAM :
                CAM_SendData(node);
                break;
            case UIF_RETRY_CAM :
                break;
            case UIF_CMD :
                send_exit();
                break;
            case UIF_HB_ACK :
                recv_ack(g_CommBuf->addr);
                break;
			case UIF_HB :
			  	send_ha = 1;
//				fakeCmd.headerEnum = 200; // send ACK back to client
//				WIFI_RelayCMD(&fakeCmd);
			  	break;
            default : break;
        } // end of switch
    } // end of if

    // Read Acc. Data from active nodes
    ACC_ReadSensor();

#ifdef PROBE_ENABLED
	p ^= 1;
	probe2_change(p);
#endif
/**************************************************
       for (i=1; i<=MAX_ACC_NODE; i++)
       {
            if (g_AccSensor[i].State == TRUE)
                ACC_RequestData(i);
       }
****************************************************/
}

void ACC_StartAll(void)
{
#ifdef __FILE_SAVE_ENABLED__
    unsigned char i;
    RTC_TIME time;
    BYTE name[12];

    RTC_GetTime(&time);
    for (i = 0; i < TEMP_MAX_FILES; ++i)
    {
        sprintf(name, "_%01d%02d%02d%2d.x%2d", i + 1, time.Date, time.Hour, time.Minute, time.Second);
        g_FATHandle[i] = FAT_OpenWrite(name);
        FAT_Write(g_FATHandle[i], name, 12);
        name[0] = 0x0D;
        name[1] = 0x0A;
        FAT_Write(g_FATHandle[i], name, 2);
        g_SampleCount[i] = 0;
        mbf[i].bigIndex = 0;
        mbf[i].smallIndex = 0;
    }
#endif

    for (;;)
    {
        if (CAN_SendExtMsg(CAN_ID_ALL, CAN_START_ACC_DATA, 0, g_CanTxMsg, 1) == TRUE) break;

    }

    if (acc_started){
      return;
    }else{
      acc_started = 1;
    }
}

void ACC_StopAll(void)
{
#ifdef  __FILE_SAVE_ENABLED__
    unsigned char i;
#endif

    for (;;)
    {
        if (CAN_SendExtMsg(CAN_ID_ALL, CAN_STOP_ACC_DATA, 0, g_CanTxMsg, 1) == TRUE) break;
    }
#ifdef  __FILE_SAVE_ENABLED__
    for (i = 0; i < TEMP_MAX_FILES; ++i)
    {
        FAT_Close(g_FATHandle[i]);
    }
#endif
    acc_started = 0;
}

void ACC_StartSensor(BYTE NodeId)
{
    for (;;)
    {
        if (CAN_SendExtMsg(NodeId, CAN_START_ACC_DATA, 0, g_CanTxMsg, 1) == TRUE) break;
    }
}

void ACC_StopSensor(BYTE NodeId)
{
    for (;;)
    {
        if (CAN_SendExtMsg(NodeId, CAN_STOP_ACC_DATA, 0, g_CanTxMsg, 1) == TRUE) break;
    }
}

BYTE ACC_ReadSensor(void)
{
    //BYTE cmd;
    BYTE node;
    //WORD ext_id;
    //BYTE std_id;
    //BYTE len;
    can_recv_queue_item_t* temp;

    if ((CAN_INT_PxIN & CAN_INT_BIT) == 0)
    {
        temp = can_recv_enqueue();
        //takes less than 50us
        if (temp)
        {
            CAN_GetExtMsg(&temp->cmd, &temp->cmd, &temp->ext_id,
                          temp->data, &temp->len);
        }
        //P2IFG &= 0xBF;
    //}

    //{
        //if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
        temp = can_recv_dequeue();

        if (temp)
        {
            node = (BYTE)(temp->ext_id);
            g_AccSensor[node].RxDone = TRUE;
            ACC_SaveData(node, temp->data);
			//probe2_change(1);
            ACC_SendData(node, temp->data[6]);
			//probe2_change(0);
 //           ++acc_count[node];
            //if ((++g_TimeFrameCount) == MAX_TIMEFRAME_COUNT)
            //{
            //    ACC_SendTimeData(node);
            //    g_TimeFrameCount = 0;
            //}
            return TRUE;
        }
    }
    return FALSE;
}

BYTE ACC_RequestSensor(BYTE NodeId)
{
	BYTE cmd, node;
	WORD ext_id;
    BYTE std_id;
	BYTE len;
	
    g_AccSensor[NodeId].RxDone = FALSE;
    CAN_SendExtMsg(NodeId, CAN_REQ_ACC_DATA, 0, g_CanTxMsg, 1);

    if (CAN_GetExtMsg(&std_id, &cmd, &ext_id, g_CanRxMsg, &len) == TRUE)
    {
        node = (BYTE)ext_id;
        g_AccSensor[node].RxDone = TRUE;

        ACC_SaveData(node, g_CanRxMsg);
        ACC_SendData(node, g_CanRxMsg[6]);
        if ((g_TimeFrameCount++) == (MAX_TIMEFRAME_COUNT-1))
        {
            ACC_SendTimeData(node);
            g_TimeFrameCount = 0;
        }
    	return TRUE;
    }
   	return FALSE;
}

void ACC_SaveData(BYTE NodeId, BYTE *pBuf)
{
#ifdef __LOCAL_STORE_ENABLED__
    RTC_TIME time;

#ifndef __LOCAL_STORE_ACCEL_MODE__
    int size = 1;
	char* str;
#endif

    //BYTE name[10];
    //bufFile_t* t_mbf = &mbf[NodeId - 1];
#endif

    ACC_SENSOR* sensor;

    //Assert(NodeId > 0);
    if (NodeId == 0) return;
    if (NodeId > 8) return;
    sensor = &(g_AccSensor[NodeId]);
    sensor->Acc[ACC_X_AXIS].Value[0] = pBuf[0];
    sensor->Acc[ACC_X_AXIS].Value[1] = pBuf[1];
    sensor->Acc[ACC_Y_AXIS].Value[0] = pBuf[2];
    sensor->Acc[ACC_Y_AXIS].Value[1] = pBuf[3];
    sensor->Acc[ACC_Z_AXIS].Value[0] = pBuf[4];
    sensor->Acc[ACC_Z_AXIS].Value[1] = pBuf[5];

    // not the transmission time
    RTC_CalcTOffset(&(sensor->TimeOffset.Data));

#ifdef __LOCAL_STORE_ENABLED__
	RTC_GetTime(&time);
	RTC_GetLongTimeStamp(&time);
	//sprintf((char*)g_Str, "[%d] %d:%d:%d.%d %d %d %d\n", sensor->CurSeq, \
            time.Hour, time.Minute, time.Second, time.MSecond, \
            sensor->Acc[ACC_X_AXIS].Data, sensor->Acc[ACC_Y_AXIS].Data, sensor->Acc[ACC_Z_AXIS].Data);
#ifndef __LOCAL_STORE_ACCEL_MODE__
	str = (char*)g_Str;
	str[0] = '[';
	size = 1;
	//size += getUNumString(sensor->CurSeq, &str[1]);
    size += getUNumString(pBuf[6], &str[1]);
	str[size++] = ']';
	str[size++] = ' ';
	size += getNumString(time.Hour, &str[size]);
	str[size++] = ':';
	size += getNumString(time.Minute, &str[size]);
	str[size++] = ':';
	size += getNumString(time.Second, &str[size]);
	str[size++] = '.';
	size += getNumString(time.MSecond, &str[size]);
	str[size++] = ' ';
	size += getNumString(sensor->Acc[ACC_X_AXIS].Data, &str[size]);
	str[size++] = ' ';
	size += getNumString(sensor->Acc[ACC_Y_AXIS].Data, &str[size]);
	str[size++] = ' ';
	size += getNumString(sensor->Acc[ACC_Z_AXIS].Data, &str[size]);
	str[size++] = '\n';
	str[size] = 0;
	LOS_write(g_Str, size);
#else
    LOS_accel_write(&time, sensor);
#endif
#endif
	
#ifdef  __FILE_SAVE_ENABLED__
    RTC_GetTime(&time);

    sprintf(g_Str, "[%d] %d:%d:%d.%d %d %d %d\n", sensor->CurSeq, \
            time.Hour, time.Minute, time.Second, time.MSecond, \
            sensor->Acc[ACC_X_AXIS].Data, sensor->Acc[ACC_Y_AXIS].Data, sensor->Acc[ACC_Z_AXIS].Data);
    size = strlen((char *)g_Str);

    ++g_SampleCount[NodeId];

    //FAT_Write(g_FATHandle[NodeId - 1], g_Str, size);
    memcpy(&(t_mbf->data[t_mbf->bigIndex][t_mbf->smallIndex]), g_Str, size);
    t_mbf->smallIndex += size;

    if (t_mbf->smallIndex > (512 - 50))
    {
        FAT_Write(g_FATHandle[NodeId - 1], t_mbf->data[t_mbf->bigIndex], t_mbf->smallIndex);
        if (t_mbf->bigIndex == 0)
        {
            t_mbf->bigIndex = 1;
        }
        else
        {
            t_mbf->bigIndex = 0;
        }
        t_mbf->smallIndex = 0;
    }

    if ((g_SampleCount[NodeId] >= MAX_SAMPLE4FILE) && (t_mbf->smallIndex == 0))
    {
        g_SampleCount[NodeId] = 0;
        FAT_Close(g_FATHandle[NodeId - 1]);
        //RTC_GetTime(&g_FATDateTime);
        //sprintf(name, "%02d%02d.txt", time.Month, time.Date);
        sprintf(name, "_%01d%02d%2d%2d.x%2d", NodeId, time.Date, time.Hour, time.Minute, time.Second);
        g_FATHandle[NodeId - 1] = FAT_OpenWrite(name);
    }
    //FAT_Flush();    // Optional.
#endif
}

void ACC_SendData(BYTE NodeId, BYTE RemoteSeq)
{
    ACC_SENSOR *sensor;
    BYTE buf[10];
    signed short temp;

    sensor = &(g_AccSensor[NodeId]);

    buf[0] = (myRoocasID << 4) | NodeId;
    //buf[1] = (BYTE)(sensor->CurSeq);
    buf[1] = RemoteSeq;
	//sensor->CurSeq = (sensor->CurSeq + 1) % 1000;
    buf[2] = sensor->Acc[ACC_X_AXIS].Value[1];
    buf[3] = sensor->Acc[ACC_X_AXIS].Value[0];
    temp = buf[2];
    temp <<= 8;
    temp |= buf[3];
    if ((temp > 20000) || (temp < -20000))
    {
        buf[0] = NodeId;
    }
    buf[4] = sensor->Acc[ACC_Y_AXIS].Value[1];
    buf[5] = sensor->Acc[ACC_Y_AXIS].Value[0];
    buf[6] = sensor->Acc[ACC_Z_AXIS].Value[1];
    buf[7] = sensor->Acc[ACC_Z_AXIS].Value[0];
    buf[8] = sensor->TimeOffset.Value[0];
    buf[9] = sensor->TimeOffset.Value[1];

    UIF_WriteCommand(UIF_SEND_ACC, buf);
    //SYS_DelayMS(100);
}

void ACC_SendTimeData(BYTE NodeId)
{
    DATA16 msec;
    BYTE buf[10];
	
    RTC_GetTime((RTC_TIME*)(&g_RTCTime));
    buf[0] = (myRoocasID << 4) | NodeId;
    buf[1] = g_RTCTime.Month;
    buf[2] = g_RTCTime.Date;
    buf[3] = g_RTCTime.Year;
    buf[4] = g_RTCTime.Hour;
    buf[5] = g_RTCTime.Minute;
    buf[6] = g_RTCTime.Second;
    msec.Data = (WORD)(g_RTCTime.MSecond);
    buf[7] = msec.Value[0];
    buf[8] = msec.Value[1];

    UIF_WriteCommand(UIF_TIME_ACC, buf);
}

void ACC_SetTime(BYTE NodeId, BYTE *buf)
{
    DATA16 msec;
	
    g_StartTime.Month = buf[0];
    g_StartTime.Date = buf[1];
    g_StartTime.Year = buf[2];
    g_StartTime.Hour = buf[3];
    g_StartTime.Minute = buf[4];
    g_StartTime.Second = buf[5];
    msec.Value[0] = buf[6];
    msec.Value[1] = buf[7];
    g_StartTime.MSecond = msec.Data;

    RTC_SetTime((RTC_TIME*)(&g_StartTime));
    //RTC_AdjustClock(&g_StartTime);

    //calculate the difference in millisecond

    //RTC_ReserveTimeToSet((RTC_TIME*)(&g_StartTime));
	
	setTime = 1;
}

void send_exit(void)
{
    char buf[] = {'e', 'x', 'i', 't', 0x0D};

    UIF_WriteByteArray(buf, 5);
	//UTX_enqueue(buf, 5);
}

void recv_ack(BYTE chanllenge)
{
  if (chanllenge == 0x99){
    ack_cnt = 0;
  }else{
    miss_ack();
  }
}

void miss_ack(void){
  ack_cnt++;
  if (ack_cnt > MISS_ACK_MAX){
    if (acc_started){
      ACC_StopAll();
    }
    ack_cnt = 0;
    CAN_ModifyBit(BFPCTRL, 0x30, 0x30);
    PMMCTL0 = PMMPW + PMMSWBOR;
  }
}