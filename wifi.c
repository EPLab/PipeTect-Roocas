#include "wifi.h"
#include "sys.h"
#include "sys_uart.h"
#include "sys_isr.h"

#include "switcher.h"

#include "acc.h"

#include "uif.h"

#include "msp430x54x.h"

#include "rtc.h"

extern void ACC_StopAll(void);
extern void miss_ack(void);

// temp ISR routine
void RP_ISR_WB_INIT(void);
void RP_ISR_WB_NOR(void);

BYTE RP_RelayReadByte(char *pChar);
void RP_WriteIP(char *ip);

BYTE self_en_dhcp = 0;
BYTE self_bmask[4] = {255, 255, 255, 0};
BYTE self_en_ibss = 0;
BYTE self_SSID[32];
BYTE self_SSID_len;
//#define self_SSID "UCInet Mobile Access"
//#define self_SSID_LEN 20
#define self_SSID "WSN-Default"
#define self_SSID_LEN 11
//#define self_SEC "WSN-PASSWORD"
//#define self_SEC_LEN 12
#define self_SEC "password"
#define self_SEC_LEN 8
#define self_IBSS_SSID "Duramote_IBSS"
#define self_IBSS_SSID_LEN 13
#define self_IBSS_SEC "hawmyungbridge"
#define self_IBSS_SEC_LEN 14

BYTE relayOnly = 0;
BYTE self_ip[4] = {192, 168, 11, 216};
BYTE self_gw[4] = {192, 168, 11, 1};
BYTE parent_hop[4] = {192, 168, 11, 200};
BYTE hasChild = 0;
BYTE child_hop[4] = {192, 168, 2, 116};

BYTE myRoocasID;
BYTE myRoocasIDBuf[2];
BYTE myRoocasIDLen;

int heartBeatInterval = 3000;
//BYTE *hb_msg_prefix = "{HB";
//BYTE hb_msg_prefix_len = 3;

WIFI_STRUCT mWIFICommState = {0};
SOCKET_ENTITY* sptr;
//SOCKET_ENTITY socket = {0};
SOCKET_ENTITY cmdChannel = {0};
//SOCKET_ENTITY dataChannel = {0};
SOCKET_ENTITY relayChannel = {0};

BYTE wifi_hb_tid;
BYTE wifi_chk_tid;
BYTE wifi_wait_tid;
BYTE wifi_snd_tid;

BYTE err_cnt;

DWORD header_prefix[4][2];
const BYTE *header_ok = "OK";
const BYTE *header_err = "ERROR";
const BYTE *header_recv = "AT+RSI_READ";
const BYTE *header_cls = "AT+RSI_CLOSE";

const BYTE tx_max_cnt = 30;
BYTE rp_txbuf[30][20];
BYTE tx_pkt_hdr = 0;
BYTE rp_snd_idx = 0;
BYTE rp_snd_flag = 0;
BYTE rp_tx_cnt = 0;
BYTE tx_cnt = 0;
int tx_tLen = 0;

#define PKT_MAX_LEN 320
char pkt_max_len_buf[4];
BYTE pkt_max_len_buf_len;
int pkt_len = 0;
BYTE tx_state = 0;
BYTE tx_buf_wr = 0;
BYTE tx_buf_rd = 0;
BYTE tx_buf_len = 0;

BYTE snd_HB = 0;
BYTE snd_idle = 0;

BYTE wifi_default_setup = 0;

BYTE rp_busy = 0;

int temp;
BYTE rp_send_state = 0;
BYTE relay_cmd = 0;
UIF_STRUCT unsend_cmd;

BYTE hb_fail_count = 0;
BYTE send_ha = 0;

extern RTC_TIME g_RTCTime;

extern int send_time;
extern int send_start;


void WIFI_Test(unsigned char tid, void* msg){
    RP_WriteFrame("at+rsi_snd=1,10,172.30.1.1,55555,abcdefghij", 45);
    RP_WriteFrame("at+rsi_snd=1,10,172.30.1.1,8255,abcdefghij", 44);
}

void WIFI_Initialize(void){
  	BYTE i, reset;

        // set for Infrastructure mode
        myRoocasID = 0x00;
        myRoocasIDLen = uint2str(myRoocasID, myRoocasIDBuf);
	
	WDTCTL = WDTPW + WDTSSEL__ACLK +WDTIS__512K;
	
	// enable ETH button
	SYS_SetPort8(8, INPUT);
	P8OUT |= 0x08;
	P8REN |= 0x08;
	reset = 1;
	SYS_DelayMS(10);
	for (i = 0; i < 10; i++){
	  SYS_DelayMS(10);
	  reset *= SYS_ReadPort8(8);
	}
	if (reset){
	  // button not pressed
	  wifi_default_setup = 0;
	} else{
	  // button pressed
	  wifi_default_setup = 1;
	}
    // build header prefix array
    header_prefix[0][0] = 2;
    header_prefix[0][1] = (DWORD)&header_ok;
    header_prefix[1][0] = 5;
    header_prefix[1][1] = (DWORD)&header_err;
    header_prefix[2][0] = 11;
    header_prefix[2][1] = (DWORD)&header_recv;
    header_prefix[3][0] = 12;
    header_prefix[3][1] = (DWORD)&header_cls;
	
	// init. fix variables
	 pkt_max_len_buf_len = uint2str(PKT_MAX_LEN, &(pkt_max_len_buf[0]));

    wifi_hb_tid = 255;
    wifi_chk_tid = 255;
    wifi_wait_tid = 255;
    wifi_snd_tid = 255;

    // register call back function
    ISR_SetUART_A2_WB(&RP_ISR_WB);

	WIFI_Reset();
	WIFI_ABRD();
	
	do{	
		//RP_test();
//		ISR_SetUART_A2_WB(&RP_ISR_WB);
//		SYS_DelayMS(200);
		reset = WIFI_START();
		if (!reset){
		  SYS_DelayMS(2000);
		  PMMCTL0 = PMMPW + PMMSWBOR;
		}
	} while(!reset);

//	wifi_default_setup = 1;
    // register wifi waiting task
	if (!wifi_default_setup){
                WDTCTL = WDTPW + WDTSSEL__ACLK +WDTIS__512K + WDTCNTCL;
                wifi_wait_tid = registerTask(WIFI_Wait4AvailableNetwork, 0, 15000, 0xFF);
		WIFI_Wait4AvailableNetwork(0, NULL);
	} else{
		WIFI_DefaultIBSS();
		WIFI_PROTO();
		registerTask(WIFI_RelayToServer, 0, 0, 0xFF);
	}
	
//	_NOP();
	WDTCTL = WDTPW + WDTSSEL__ACLK +WDTIS__512K + WDTCNTCL;
}

BYTE RP_IsModuleBusy(){
  return rp_busy;
}

int RP_SetModuleBusy(){
  if (RP_IsModuleBusy()){
	return 0;
  } else{
	rp_busy = 1;
	return 1;
  }
}

int RP_ClearModuleBusy(){
  if (!RP_IsModuleBusy()){
	return 0;
  } else{
	rp_busy = 0;
	return 1;
  }
}

void WIFI_RelayToServer(unsigned char tid, void* msg){
	int i;
	BYTE buf[16];
	char bData;
	BYTE status;
	
	if (relayChannel.rxCount < 15){
	  _NOP();
		return;
	}
	
	if (!RP_RelayReadByte(&bData)){
	  return;
	}

	if (bData == '{'){
	  buf[0] = '{';
	  if (RP_RelayReadByte(&bData)){
		if (bData != 'A'){
		  return;
		} else{
		  buf[1] = 'A';
		  if (RP_RelayReadByte(&bData)){
			if (bData == 'D'){
			  buf[2] = 'D';
			  for (i = 0; i < 13; i++){
				RP_RelayReadByte(&(buf[i + 3]));
			  }
			  WIFI_WriteFrame(&(buf[0]), 16);
			} else if (bData == 'T'){
			  buf[2] = 'T';
			  for (i = 0; i < 12; i++){
				RP_RelayReadByte(&(buf[i + 3]));
			  }
			  WIFI_WriteFrame(&(buf[0]), 15);
			} else{
			  return;
			}
		  } else{
			return;
		  }
		}
	  } else{
		return;
	  }
	} else{
	  return;
	}
	
	_NOP();
}

void WIFI_RelayCMD(UIF_STRUCT *cmdBuf){
  int i;
  int status;
  char resp;
  unsigned char buf[6];
  unsigned short chkSum;
  unsigned short tchkSum;
  unsigned char chkSumBuf[2];
  unsigned char tbuf[14];
  DATA16 msec;

  if (!hasChild){
	return;
  }

  if (temp || rp_send_state){
	unsend_cmd.headerEnum = cmdBuf->headerEnum;
	relay_cmd = 1;
	return;
  }

  buf[0] = '{';
  buf[1] = 'A';

  tbuf[0] = '{';
  tbuf[1] = 'A';
  tbuf[2] = 'T';
  tbuf[3] = 0xF0;
	// time packet
	tbuf[4] = g_RTCTime.Month;
	tbuf[5] = g_RTCTime.Date;
	tbuf[6] = g_RTCTime.Year;
	tbuf[7] = g_RTCTime.Hour;
	tbuf[8] = g_RTCTime.Minute;
	tbuf[9] = g_RTCTime.Second;
	msec.Data = (WORD)(g_RTCTime.MSecond);
	tbuf[10] = msec.Value[0];
	tbuf[11] = msec.Value[1];
	tchkSum = '{';
	for (i = 1; i < 12; i++){
	  tchkSum += tbuf[i];
	}
	tbuf[12] = tchkSum >> 8;
	tbuf[13] = tchkSum;

/*	
  chkSum = 0;
  for (i = 0; i < 4; i++){
	chkSum += buf[i];
  }
  chkSumBuf[0] = chkSum >> 8;
  chkSumBuf[1] = chkSum;
*/


  switch(cmdBuf->headerEnum){
  case 0:	// start cmd
	buf[2] = 'S';
	buf[3] = myRoocasID << 4;
	chkSum = '{';
	for (i = 1; i < 4; i++){
	  chkSum += buf[i];
	}
	buf[4] = chkSum >> 8;
	buf[5] = chkSum;

//	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,6,", 15);
	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,20,", 16);
	RP_WriteIP(&(child_hop[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(tbuf[0]), 14);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 6);
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
	status = RP_WaitModuleResponse(&resp);
	send_time++;
	send_start++;
	break;
  case 1:	// acc data
	break;
  case 2:	// stop cmd
	buf[2] = 'F';
	buf[3] = myRoocasID << 4;
	chkSum = '{';
	for (i = 1; i < 4; i++){
	  chkSum += buf[i];
	}
	buf[4] = chkSum >> 8;
	buf[5] = chkSum;
	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,6,", 15);
	RP_WriteIP(&(child_hop[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 6);	
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
	status = RP_WaitModuleResponse(&resp);
	break;
  case 3:	// time data
	if (relay_cmd == 1){
		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,14,", 16);
		RP_WriteIP(&(child_hop[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
		SYS_WriteFrameUART(WIFI_UART_MODE, &(tbuf[0]), 14);
	} else{
		buf[2] = 'T';
		buf[3] = cmdBuf->addr;
		chkSumBuf[0] = cmdBuf->chkSum >> 8;
		chkSumBuf[1] = cmdBuf->chkSum;

		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,14,", 16);
		RP_WriteIP(&(child_hop[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
		SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 4);
		SYS_WriteFrameUART(WIFI_UART_MODE, cmdBuf->data, 8);
		SYS_WriteFrameUART(WIFI_UART_MODE, chkSumBuf , 2);
	}
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
	status = RP_WaitModuleResponse(&resp);
	send_time++	;
	break;
  case 200: // HeartBeat ACK Message
	buf[1] = 'H';
	buf[2] = 'A';
	buf[3] = 0x99;
	chkSum = '{';
	for (i = 1; i < 4; i++){
	  chkSum += buf[i];
	}
	buf[4] = chkSum >> 8;
	buf[5] = chkSum;
	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,6,", 15);
	RP_WriteIP(&(child_hop[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 6);	
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
	status = RP_WaitModuleResponse(&resp);
	break;
  case 255:	// start cmd // send with time
	buf[2] = 'S';
	buf[3] = myRoocasID << 4;
	chkSum = '{';
	for (i = 1; i < 4; i++){
	  chkSum += buf[i];
	}
	buf[4] = chkSum >> 8;
	buf[5] = chkSum;
	
	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,20,", 16);
//	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,6,", 15);
	RP_WriteIP(&(child_hop[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(tbuf[0]), 14);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 6);
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
	status = RP_WaitModuleResponse(&resp);
	send_time++;;
	send_start++;
	break;
  default:
	break;
  }

  if (relay_cmd){
	relay_cmd = 0;
  }
}

void WIFI_DefaultIBSS(void){
	int status;
	char resp;

	do{
		mWIFICommState.cmdMode = 0;
		RP_WriteFrame("at+rsi_network=IBSS,0,11", 24);
		status = RP_WaitModuleResponse(&resp);

		mWIFICommState.cmdMode = 0;
		RP_WriteFrame("at+rsi_numscan=1", 16);
		status = RP_WaitModuleResponse(&resp);

		mWIFICommState.cmdMode = 0;
//		RP_WriteFrame("at+rsi_scan=0,Duramote_IBSS", 27);
		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_scan=0," , 14);
		SYS_WriteFrameUART(WIFI_UART_MODE, self_IBSS_SSID , self_IBSS_SSID_LEN);
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
		status = RP_WaitModuleResponse_Long(&resp);
	} while(status != CMD_OK);
	if (!mWIFICommState.cData[0]){
		RP_WriteFrame("at+rsi_network=IBSS,1,11", 24);
		status = RP_WaitModuleResponse(&resp);	
	}
		
	do{
		mWIFICommState.cmdMode = 0;
//		RP_WriteFrame("at+rsi_join=Duramote_IBSS,0,0", 29);
		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_join=" , 12);
		SYS_WriteFrameUART(WIFI_UART_MODE, self_IBSS_SSID , self_IBSS_SSID_LEN);
		SYS_WriteFrameUART(WIFI_UART_MODE, ",0,0" , 4);
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);		
		status = RP_WaitModuleResponse(&resp);
		
		mWIFICommState.cmdMode = 0;
//		RP_WriteFrame("at+rsi_ipconf=0,192.168.1.111,255.255.255.0,192.168.1.101", 55);
		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_ipconf=0," , 16);
		RP_WriteIP(&(self_ip[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, "," , 1);
		RP_WriteIP(&(self_bmask[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, "," , 1);
		RP_WriteIP(&(self_gw[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
		status = RP_WaitModuleResponse(&resp);
		myRoocasID = self_ip[3] - 110;
		myRoocasIDLen = uint2str(myRoocasID, myRoocasIDBuf);
	}while(status != CMD_OK);
}

void WIFI_SendHearBeatMsg(unsigned char tid, void* msg){
    int status;
    BYTE errcode = 0;
    char resp;
	char buf[6];
	unsigned short chkSum;
//	int len;
	int i;
	
	if (temp == 1 || rp_send_state == 1){
		snd_HB = 1;
		if (++hb_fail_count > 5 && (temp == 1 || rp_send_state == 1)){
		  temp = 0;
		  rp_send_state = 0;
		  hb_fail_count = 0;
		}
		return;
	}
	
	if (pkt_len > 0){
		tx_buf_len = 0;
		tx_buf_wr = 0;
		tx_buf_rd = 0;
		snd_HB = 1;
		mWIFICommState.cmdMode = 0;
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
		if (snd_idle++ > 3){
			pkt_len = 0;
			
		}
		return;
	}
	
//	fakeCmd.headerEnum = 100;
//	WIFI_RelayCMD(&fakeCmd);

//	RP_WriteFrame("at+rsi_snd=1,10,192.168.1.101,55555,abcdefghij", 48);
	buf[0] = '{';
	buf[1] = 'H';
	buf[2] = 'B';
	buf[3] = myRoocasID + '0';
	chkSum = '{';
	for (i = 1; i < 4; i++){
	  chkSum += buf[i];
	}
	buf[4] = chkSum >> 8;
	buf[5] = chkSum;
	

	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=1,6," , 15);
	RP_WriteIP(&(parent_hop[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 6);
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
    status = RP_WaitModuleResponse(&resp);
	if (send_ha){
		buf[2] = 'A';
		buf[3] = 0x99;
		chkSum = '{';
		for (i = 1; i < 4; i++){
	  		chkSum += buf[i];
		}
		buf[4] = chkSum >> 8;
		buf[5] = chkSum;
	

		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=4,6," , 15);
		RP_WriteIP(&(child_hop[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
		SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]), 6);
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
    	status = RP_WaitModuleResponse(&resp);
		send_ha = 0;
	}
//    mWIFICommState.cmdMode = 0;
//	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=1," , 13);
//	len = uint2str(hb_msg_prefix_len + myRoocasIDLen, &(buf[0]));
//	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]) , len);
//	SYS_WriteFrameUART(WIFI_UART_MODE, "," , 1);
//	RP_WriteIP(&(parent_hop[0]));
//	SYS_WriteFrameUART(WIFI_UART_MODE, ",55555," , 7);
//	SYS_WriteFrameUART(WIFI_UART_MODE, hb_msg_prefix , hb_msg_prefix_len);
//	SYS_WriteFrameUART(WIFI_UART_MODE, myRoocasIDBuf , myRoocasIDLen);
//	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
//    status = RP_WaitModuleResponse(&resp);

/*	
    if (!status){
      errcode = mWIFICommState.cData[0];
	  if (errcode == 0xEC){
		_NOP();
	  } else if (errcode == 0xF3){
		_NOP();
	  } else{
		_NOP();
	  }
      if (err_cnt > 10){
        // dis-associate
        WIFI_RECONF();
        err_cnt = 0;
      } else if (err_cnt > 5){
        //ACC_StopAll();
      } else{
      }
      err_cnt++;
	} else{
      miss_ack();
    }
*/
        if (status != CMD_OK){
          errcode = mWIFICommState.cData[0];
          if (errcode == 0xFE){
            ACC_StopAll();
//            WIFI_RECONF();
          }
        }
	
	miss_ack();
}

void WIFI_Start_HB(void){
	while (wifi_hb_tid == 255){
	    wifi_hb_tid = registerTask(WIFI_SendHearBeatMsg, 0, heartBeatInterval, 0xFF);
	    WIFI_SendHearBeatMsg(wifi_hb_tid, NULL);
	}
}

void WIFI_Stop_HB(void){
  if (wifi_hb_tid != 255){
    unregisterTask(wifi_hb_tid);
    wifi_hb_tid = 255;
  }
}

void WIFI_RECONF(void){
  BYTE status;
  char *resp;

  mWIFICommState.cmdMode = 0;
  RP_WriteFrame("at+rsi_disassoc", 15);
  status = RP_WaitModuleResponse(resp);

  if (wifi_wait_tid == 255){
    wifi_wait_tid = registerTask(WIFI_Wait4AvailableNetwork, 0, 15000, 0xFF);
  }
  if (wifi_hb_tid != 255){
    unregisterTask(wifi_hb_tid);
    wifi_hb_tid = 255;
  }
}

void WIFI_Wait4AvailableNetwork(unsigned char tid, void* msg){
    // wait until target WiFi network appears
  if (WIFI_FIND()){
    WIFI_ASSOC();
    WIFI_PROTO();
  }else{

  }
}

void WIFI_CheckNetwork(unsigned char tid, void* msg){
  BYTE reset = 0;
  reset = WIFI_CHK();

  if (reset){
    WIFI_START();
    WIFI_ASSOC();
    WIFI_PROTO();
  }
}

void WIFI_Send(unsigned char tid, void* msg){
}


void WIFI_Reset(void){
  SYS_DelayMS(50);
  WIFI_RESET_PxOUT &= ~WIFI_RESET_BIT;
  SYS_DelayMS(200);
  WIFI_RESET_PxOUT |= WIFI_RESET_BIT;
  SYS_DelayMS(100);
  WIFI_RESET_PxOUT &= ~WIFI_RESET_BIT;
  SYS_DelayMS(50);
  WIFI_RESET_PxOUT |= WIFI_RESET_BIT;
}

void WIFI_SoftReset(void){
	int status;
	char resp;

	mWIFICommState.cmdMode = 0;
	RP_WriteFrame("at+rsi_reset", 12);
	status = RP_WaitModuleResponse(&resp);
}

void WIFI_ABRD(void){
  BYTE count = 0;
  do{
    if (mWIFICommState.state == INIT && mWIFICommState.subState == INIT_ABRD){
      SYS_WriteByteUART(WIFI_UART_MODE, 0x1C);
      SYS_DelayMS(200);
	  if (++count == 3){
		//Reboot
		PMMCTL0 |= PMMSWBOR;
	  }
    } else{
	  SYS_DelayMS(2000);
	  if (++count == 10){
		//Reboot
		PMMCTL0 |= PMMSWBOR;
	  }
	}
  } while(mWIFICommState.state == INIT);
}

BYTE WIFI_START(void){
  int status;
  char resp;

  do{
    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_band=0", 13);
    status = RP_WaitModuleResponse(&resp);
//	status = 1;
    if (status != CMD_OK){
//      SYS_DelayMS(10);
//      continue;
	  return 0;
    }

    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_init", 11);
    status = RP_WaitModuleResponse(&resp);
	if (status != CMD_OK){
	  return 0;
	}
  }while(status != CMD_OK);
  return 1;
}

BYTE WIFI_FIND(void){
  int status;
  char resp;

  mWIFICommState.cmdMode = 0;
  RP_WriteFrame("at+rsi_numscan=1", 16);
  status = RP_WaitModuleResponse(&resp);

  mWIFICommState.cmdMode = 0;
//  RP_WriteFrame("at+rsi_scan=0,WSN-KR", 20);
  SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_scan=0,", 14);
  SYS_WriteFrameUART(WIFI_UART_MODE, self_SSID, self_SSID_LEN);
  SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
  status = RP_WaitModuleResponse_Long(&resp);

  if (status != CMD_OK){
    if (mWIFICommState.cData[0] == 0xF1){
      WIFI_RECONF();
    }
  }

  return status;
}

void WIFI_ASSOC(void){
  int status;
  char resp;

  do{
    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_network=INFRASTRUCTURE", 29);
    status = RP_WaitModuleResponse(&resp);

    mWIFICommState.cmdMode = 0;
//    RP_WriteFrame("at+rsi_psk=WSN-PASSWORD", 23);
    SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_psk=", 11);
    SYS_WriteFrameUART(WIFI_UART_MODE, self_SEC, self_SEC_LEN);
    SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
    status = RP_WaitModuleResponse(&resp);

    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_authmode=0", 17);
    status = RP_WaitModuleResponse(&resp);

    mWIFICommState.cmdMode = 0;
//    RP_WriteFrame("at+rsi_join=WSN-KR,0,2", 22);
    SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_join=", 12);
    SYS_WriteFrameUART(WIFI_UART_MODE, self_SSID, self_SSID_LEN);
    SYS_WriteFrameUART(WIFI_UART_MODE, ",0,2", 4);
    SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
    status = RP_WaitModuleResponse(&resp);

    SYS_DelayMS(3000);

    mWIFICommState.cmdMode = 0;
    //RP_WriteFrame("at+rsi_ipconf=1,0,0,0", 21);
    if (self_en_dhcp){
      RP_WriteFrame("at+rsi_ipconf=1,0,0,0", 21);
    } else{
      RP_WriteFrame("at+rsi_ipconf=0,192.168.11.216,255.255.255.0,192.168.11.1", 57);
    }
    status = RP_WaitModuleResponse_Long(&resp);

//    status = 1;

//  add for the power saving
//    mWIFICommState.cmdMode = 0;
//    RP_WriteFrame("at+rsi_pwmode=2", 15);
//    status = RP_WaitModuleResponse(resp);

    if (status == CMD_OK){
      mWIFICommState.state = NETWORK;
    }else{
      WIFI_RECONF();
      WIFI_FIND();
    }
  } while(mWIFICommState.state == WIFICFG);
}

void WIFI_PROTO(void){
  int status;
  char resp;

  do{
    // SOCKET NUM 1, set up command listen channel
    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_ludp=2000", 16);
    status = RP_WaitModuleResponse(&resp);
	
    // SOCKET NUM 2, set up connectin socket to parent (for UDP, one host just one socket)
    mWIFICommState.cmdMode = 0;
	SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_udp=" , 11);
	RP_WriteIP(&(parent_hop[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, ",33333,1234" , 11);
	SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
	status = RP_WaitModuleResponse(&resp);

    // SOCKET NUM 3, set up data relay channel
    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_ludp=8255", 16);
    status = RP_WaitModuleResponse(&resp);
	
    // SOCKET NUM 4, set up cmd relay channel
    mWIFICommState.cmdMode = 0;
    RP_WriteFrame("at+rsi_ludp=55555", 17);
    status = RP_WaitModuleResponse(&resp);

	// SOCKET NUM 5, set up connectin socket to child (for UDP, one host just one socket)
	if (hasChild){
		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_udp=" , 11);
		RP_WriteIP(&(child_hop[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, ",44444,1237" , 11);
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
    	status = RP_WaitModuleResponse(&resp);
	}
		
    //unreigtser wifi waiting task
    unregisterTask(wifi_wait_tid);
    //wifi_wait_tid = 255;
    //register the heart beat message task
    if (wifi_hb_tid == 255){
      wifi_hb_tid = registerTask(WIFI_SendHearBeatMsg, 0, heartBeatInterval, 0xFF);
      WIFI_SendHearBeatMsg(wifi_hb_tid, NULL);
    }
    //register the network liveness check task
    //wifi_chk_tid = registerTask(WIFI_CheckNetwork, 0, 3000, 0xFF);

    mWIFICommState.state = DATACOMM;
  } while(mWIFICommState.state == NETWORK);
}

BYTE WIFI_CHK(void){
  int status;
  char *resp;
  BYTE reset = 0;

  mWIFICommState.cmdMode = 0;
  RP_WriteFrame("at+rsi_nwparams?", 16);
  status = RP_WaitModuleResponse(resp);
  if (status){
    if (mWIFICommState.cData[0] != 'W'){
      // Not Associate
      reset = 1;
    }
    if (mWIFICommState.cData[85] != 2){
      // Socket Problem
      reset = 1;
    }
  }else{
    reset = 1;
  }
  if (reset){
    do{
      mWIFICommState.cmdMode = 0;
      RP_WriteFrame("at+rsi_reset", 12);
      status = RP_WaitModuleResponse(resp);
    } while(status != CMD_OK);
  }

  return reset;
}

void WIFI_Sleep(void){
}

void WIFI_Wakeup(void){
}

BYTE RP_RelayReadByte(char *pChar){
	if (relayChannel.rxCount == 0){
		return FALSE;
	}

	*pChar = relayChannel.rxbuf[relayChannel.rxRdIndex];

	if (++relayChannel.rxRdIndex >= MAX_SOCKET_RX_BUF_LEN){
		relayChannel.rxRdIndex = 0;
	}

	relayChannel.rxCount--;

	return TRUE;
}


BYTE WIFI_ReadByte(char *pChar){
  if (cmdChannel.rxCount == 0){
	//    if (socket.rxCount == 0){
	  return FALSE;
  }

//    *pChar = socket.rxbuf[socket.rxRdIndex];
  *pChar = cmdChannel.rxbuf[cmdChannel.rxRdIndex];

    if (++cmdChannel.rxRdIndex >= MAX_SOCKET_RX_BUF_LEN){
//    if (++socket.rxRdIndex >= MAX_SOCKET_RX_BUF_LEN){
//        socket.rxRdIndex = 0;
	  cmdChannel.rxRdIndex = 0;
    }

//    socket.rxCount--;
	cmdChannel.rxCount--;

    return TRUE;
}

void WIFI_WriteFrame(char *pData, int iLen){
    char buf[5];
    int i;
    int len;
	BYTE record_len = 0;
	BYTE padding_len = 0;
	int status;
	char resp;

	if (tx_buf_len < tx_max_cnt){
		rp_txbuf[tx_buf_wr][0] = (BYTE)RP_ByteStuffing(pData, iLen, &rp_txbuf[tx_buf_wr][1]);
		tx_buf_wr++;
		tx_buf_len++;
		if (tx_buf_wr == tx_max_cnt){
			tx_buf_wr = 0;
		}
	}else{
	  i = 0;
	}
	
	switch(tx_state){
	case 0:
	  	rp_send_state = 1;
		mWIFICommState.cmdMode = 0;
		SYS_WriteFrameUART(WIFI_UART_MODE, "at+rsi_snd=1,", 13);
		SYS_WriteFrameUART(WIFI_UART_MODE, &( pkt_max_len_buf[0]),  pkt_max_len_buf_len);
		tx_state++;
		break;
	case 1:
		SYS_WriteFrameUART(WIFI_UART_MODE, ",", 1);
		RP_WriteIP(&(parent_hop[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, ",8255,", 6);
		tx_state++;
		break;
	case 2:
		if (tx_buf_len){
			record_len = rp_txbuf[tx_buf_rd][0];
			if (pkt_len + record_len > PKT_MAX_LEN){
				// do padding
			  	padding_len = PKT_MAX_LEN - pkt_len;
				SYS_WriteByteUART(WIFI_UART_MODE, '{');
				for (i = 1; i < padding_len; i++){
					SYS_WriteByteUART(WIFI_UART_MODE, 0);
				}
				
				tx_state++;
			}else if(pkt_len + record_len == PKT_MAX_LEN){
				pkt_len += record_len;
				
				for (i = 0; i < record_len; i++){
					SYS_WriteByteUART(WIFI_UART_MODE, rp_txbuf[tx_buf_rd][i + 1]);
				}
				
				tx_buf_rd++;
				tx_buf_len--;
				if (tx_buf_rd == tx_max_cnt){
					tx_buf_rd = 0;
				}
				tx_state++;
			} else{
				pkt_len += record_len;
				
				for (i = 0; i < record_len; i++){
					SYS_WriteByteUART(WIFI_UART_MODE, rp_txbuf[tx_buf_rd][i + 1]);
				}
				
				tx_buf_rd++;
				tx_buf_len--;
				if (tx_buf_rd == tx_max_cnt){
					tx_buf_rd = 0;
				}
				
				record_len = rp_txbuf[tx_buf_rd][0];
				if (tx_buf_len && pkt_len + record_len < PKT_MAX_LEN){
					pkt_len += record_len;
				
					for (i = 0; i < record_len; i++){
						SYS_WriteByteUART(WIFI_UART_MODE, rp_txbuf[tx_buf_rd][i + 1]);
					}
				
					tx_buf_rd++;
					tx_buf_len--;
					if (tx_buf_rd == tx_max_cnt){
						tx_buf_rd = 0;
					}
				}
			}
		}else{
		  i = 1;
		}
	  	break;
	default:
		break;
	}
	
	if (tx_state == 3){
		mWIFICommState.cmdMode = 0;
		if (temp){
		  _NOP();
		  return;
		}
		SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
		status = RP_WaitModuleResponse(&resp);
		if (status != 1){
		  _NOP();
		}
		rp_send_state = 0;
		pkt_len = 0;
//		tx_state = 0;
		if (snd_HB){
		  tx_state++;
		} else if (relay_cmd){
		  tx_state = 5;
		} else{
		  tx_state = 0;
		}
	} else if (tx_state == 4){
	  WIFI_SendHearBeatMsg(wifi_hb_tid, NULL);
	  snd_HB = 0;
	  tx_state = 0;
	} else if (tx_state == 5){
	  WIFI_RelayCMD(&unsend_cmd);
	  relay_cmd = 0;
	  tx_state = 0;
	} else{
	}
}

void RP_ISR_WB_INIT(void){
	char bData;
	BYTE status = 0;

	status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
	switch(mWIFICommState.subState){
	case INIT_ABRD:
		if (bData == 0x55){
			mWIFICommState.subState = INIT_FWUP;
			SYS_WriteByteUART(WIFI_UART_MODE, 0x55);
		}
		break;
	case INIT_FWUP:
		if (bData == 0x29){
			mWIFICommState.subState = INIT_WAIT;
			SYS_WriteByteUART(WIFI_UART_MODE, 'n');
		}
		break;
	case INIT_WAIT:
		if (bData == 'e'){
			mWIFICommState.state = WIFICFG;
			mWIFICommState.subState = WIFI_BAND;
			mWIFICommState.recvState = RECV_INIT;
		}
		break;
	}
}

void RP_ISR_WB_NOR(void){
	BYTE status = 0;
	char bData;
	
	status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
	switch(mWIFICommState.recvState){
	case RECV_INIT:
		if(bData == 'O'){
			mWIFICommState.recvType = CMD_OK;
			mWIFICommState.recvState = RECV_EOLR;
		} else if (bData == 'E'){
			mWIFICommState.recvType = CMD_ERR;
			mWIFICommState.recvState = RECV_EOLR;
		} else if (bData == 'A'){
			mWIFICommState.recvType = PKT_IN;
			mWIFICommState.recvState = RECV_EOLR;
		} else{
			// unkown start
			mWIFICommState.recvState = RECV_INIT;
		}
		break;
	case RECV_EOLR:
		mWIFICommState.bRxCount++;
//		socket.rxbuf[socket.rxWrIndex++] = bData;
//		if (socket.rxWrIndex > MAX_SOCKET_RX_BUF_LEN){
//			socket.rxWrIndex = 0;
//		}

		if (bData == '\r'){
			mWIFICommState.recvState = RECV_EOLN;
		}
		break;
	case RECV_EOLN:
		if (bData == '\n'){
			mWIFICommState.cmd_cnt++;
			mWIFICommState.bRxCount -= 1;
			mWIFICommState.recvState = RECV_INIT;
		}
		break;
	default:
		// unkown state
		mWIFICommState.recvState = RECV_INIT;
		break;
	}
}

void WIFI_ProcessUART(unsigned char tid, void* msg){
  if (mWIFICommState.cmd_cnt){
	
	mWIFICommState.cmd_cnt--;
  }
}

void RP_ISR_WB(void){
    BYTE status = 0;
    char bData;

    if (mWIFICommState.state == INIT){
        switch(mWIFICommState.subState){
            case INIT_ABRD:
                status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
                if (bData == 0x55){
                    mWIFICommState.subState = INIT_FWUP;
                    SYS_WriteByteUART(WIFI_UART_MODE, 0x55);
                }
                break;
            case INIT_FWUP:
                status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
                if (bData == 0x29){
                    mWIFICommState.subState = INIT_WAIT;
                    SYS_WriteByteUART(WIFI_UART_MODE, 'n');
                }
                break;
            case INIT_WAIT:
                status = SYS_ReadByteUART(WIFI_UART_MODE, &mWIFICommState.cData[mWIFICommState.bWrIndex++]);
                if (mWIFICommState.cData[mWIFICommState.bWrIndex - 3] == 'e'){
                    mWIFICommState.state = WIFICFG;
                    mWIFICommState.subState = WIFI_BAND;
                    mWIFICommState.bWrIndex = 0;
                    mWIFICommState.recvState = RECV_INIT;
                }
                break;
            default:
                break;
        }
    } else{ //recv till get EOL
        switch(mWIFICommState.recvState){
            case RECV_INIT:
                status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
//                mWIFICommState.cData[mWIFICommState.bWrIndex++] = bData;
                mWIFICommState.recvHeaderCnt = 1;
                if(bData == 'O'){
                    mWIFICommState.recvState = RECV_HEADER;
                    mWIFICommState.recvHeader = &header_prefix[0][0];
                    mWIFICommState.recvType = CMD_OK;
                } else if (bData == 'E'){
                    mWIFICommState.recvState = RECV_HEADER;
                    mWIFICommState.recvHeader = &header_prefix[1][0];
                    mWIFICommState.recvType = CMD_ERR;
                } else if (bData == 'A'){
                    mWIFICommState.recvState = RECV_HEADER;
                    mWIFICommState.recvHeader = &header_prefix[2][0];
                    mWIFICommState.recvType = PKT_IN;
					temp = 1;
                } else{
                    // Unknown header
                    mWIFICommState.bWrIndex = 0;
//                    mWIFICommState.recvType = 0;
                }
                break;
            case RECV_HEADER:
                status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
//                mWIFICommState.cData[mWIFICommState.bWrIndex] = bData;

                if (bData == ((char *)*((DWORD *)mWIFICommState.recvHeader[1]))[mWIFICommState.recvHeaderCnt]){
                    mWIFICommState.recvHeaderCnt++;
                } else{
                    mWIFICommState.recvState = RECV_INIT;
                }

                if (mWIFICommState.recvHeaderCnt == mWIFICommState.recvHeader[0]){
                    mWIFICommState.recvState = RECV_DATA;
					if (mWIFICommState.recvType == PKT_IN){
						sptr = NULL;
					}
                    mWIFICommState.bWrIndex = 0;
                }
                break;
            case RECV_DATA:
                if (mWIFICommState.recvType == PKT_IN){
				  if (sptr == NULL){
					status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
					if (bData == 1){
					  sptr = &cmdChannel;
					} else if (bData == 3){
					  sptr = &relayChannel;
//					  sptr->rxWrIndex = 0;
					} else {
					  sptr = &cmdChannel;
					}
					sptr->recvCnt = 2;
					sptr->recvState = pkt_size;
				  } else{
					switch(sptr->recvState){
//					switch(socket.recvState){
//                        case pkt_socket:
//                            status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
//                            socket.socket_id = bData;
//                            socket.recvCnt = 2;
//                            socket.recvState = pkt_size;
//                            break;
                        case pkt_size:
                            status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
							if (--sptr->recvCnt){
//                            if (--socket.recvCnt){
//                                socket.recvLen = bData;
								sptr->recvLen = bData;
                            } else{
//                                socket.recvLen |= (DWORD)(bData << 8);
//                                socket.recvState = pkt_sip;
//                                socket.recvCnt = 4;
								sptr->recvLen |= (WORD)(bData << 8);
								sptr->recvState = pkt_sip;
								sptr->recvCnt = 4;
                            }
                            break;
                        case pkt_sip:
                            status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
							if (--sptr->recvCnt){
//                            if (--socket.recvCnt){
                            } else{
//                                socket.recvState = pkt_sport;
//                                socket.recvCnt = 2;
							  sptr->recvState = pkt_sport;
							  sptr->recvCnt = 2;
                            }
                            break;
                        case pkt_sport:
                            status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
							if (--sptr->recvCnt){
//                            if (--socket.recvCnt){
                            } else{
//                                socket.recvState = pkt_payload;
//                                socket.recvCnt = socket.recvLen;
							  sptr->recvState = pkt_payload;
							  sptr->recvCnt = sptr->recvLen;
                            }
                            break;
                        case pkt_payload:
                            status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
							if (sptr->recvCnt == 0){
 //                           if (socket.recvCnt == 0 && bData == 0x0D){
							  if (bData == 0x0D){
                                mWIFICommState.recvState = RECV_EOL;
//                                socket.recvState = pkt_socket;
								sptr->recvState = pkt_socket;
							  } else{
								mWIFICommState.recvState = RECV_EOL;
								sptr->recvState = pkt_socket;
							  }
                            } else{
								if (sptr->recvCnt--){
//                                if (socket.recvCnt--){
//                                    socket.rxbuf[socket.rxWrIndex++] = bData;
//                                    socket.rxCount++;
//                                    if (socket.rxWrIndex > MAX_SOCKET_RX_BUF_LEN){
//                                        socket.rxWrIndex = 0;
//                                    }
                                    sptr->rxbuf[sptr->rxWrIndex++] = bData;
									sptr->rxCount++;
                                    if (sptr->rxWrIndex > MAX_SOCKET_RX_BUF_LEN){
                                        sptr->rxWrIndex = 0;
                                    }
                                }
                            }
                            break;
                        default:
//                            socket.recvState = pkt_socket;
							sptr->recvState = pkt_socket;
                            break;
                    }
				  }
//                } else if (mWIFICommState.recvType == CMD_ERR){
//                    status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
//                    mWIFICommState.cData[mWIFICommState.bWrIndex] = bData;
//                    if (bData == 0x0D){
//                        mWIFICommState.recvState = RECV_EOL;
//                    } else{
//                        mWIFICommState.bWrIndex++;
//                    }
                } else{
                    status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
                    if (bData == 0x0D){
                        mWIFICommState.recvState = RECV_EOL;
                    } else{
                    	mWIFICommState.cData[mWIFICommState.bWrIndex++] = bData;
                    }
                }
                break;
            case RECV_EOL:
                status = SYS_ReadByteUART(WIFI_UART_MODE, &bData);
                if (bData == 0x0A){
                    mWIFICommState.recvState = RECV_INIT;
                    mWIFICommState.cmdMode = RECV_READY;
					mWIFICommState.cmd_cnt++;
					if (mWIFICommState.recvType == PKT_IN){
					  sptr->pktCount++;
					  temp = 0;
					} else{
					  if (temp){
					  	temp = 0;
					  }
					}
                } else{
//				  if (temp == 1 && mWIFICommState.recvType == PKT_IN){
//					_NOP();
					temp = 0;
//				  }
					mWIFICommState.recvState = RECV_INIT;
                }
//				RP_ClearModuleBusy();
                break;
            default:
//				RP_ClearModuleBusy();
                break;
        }
    }
}

BYTE RP_Wait_Recv_Ready(){
  int i;

  for (i = 0; i < 1000; i++){
//    if (mWIFICommState.cmdMode == RECV_READY){
	if (mWIFICommState.cmd_cnt){
	  mWIFICommState.cmd_cnt -= 1;
      return 1;
    }else{
	  SYS_DelayMS(1);
    }
  }
  return 0;
}

int RP_WaitModuleResponse(char *resp){
    BYTE wait = 0;
	BYTE status;
    wait = RP_Wait_Recv_Ready();
    if (!wait){
      return -1;
    }
	status = mWIFICommState.recvType;
	mWIFICommState.recvType = 0;
    if (status == CMD_OK){
        return 1;
    } else if (status == CMD_ERR) {
        return 0;
    } else{
        return -1;
    }
}

BYTE RP_Wait_Recv_Ready_Long(){
  int i;

  for (i = 0; i < 1000; i++){
//    if (mWIFICommState.cmdMode == RECV_READY){
	if (mWIFICommState.cmd_cnt){
	  mWIFICommState.cmd_cnt -= 1;
      return 1;
    }else{
	  SYS_DelayMS(10);
    }
  }
  return 0;
}

int RP_WaitModuleResponse_Long(char *resp){
    BYTE wait = 0;
	BYTE status;
    wait = RP_Wait_Recv_Ready_Long();
    if (!wait){
      return -1;
    }
	status = mWIFICommState.recvType;
	mWIFICommState.recvType = 0;
    if (status == CMD_OK){
        return 1;
    } else if (status == CMD_ERR) {
        return 0;
    } else{
        return -1;
    }
}

// Utility functions
int uint2str(int num, char *str){
    static char *cnum = "0123456789";
    int len = 0;
    int div = 1;
    int val;

    // get length of the num
    while((num / div) > 0){
        len++;
        div *= 10;
    }
    val = len;

    // convert int to string
    while(len){
        str[--len] = cnum[num % 10];
        num /= 10;
    }

    return val;
}

void RP_WriteIP(char *ip){
	int len, i;
	BYTE buf[3];
	
	len = uint2str(*(ip++), &(buf[0]));
	SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]) , len);
	for (i = 1; i < 4; i++){
		len = uint2str(*(ip++), &(buf[0]));
		SYS_WriteFrameUART(WIFI_UART_MODE, "." , 1);
		SYS_WriteFrameUART(WIFI_UART_MODE, &(buf[0]) , len);
	}
}