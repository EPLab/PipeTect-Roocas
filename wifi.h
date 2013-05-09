#ifndef __WIFI_H__
#define __WIFI_H__

#include "define.h"
#include "redpine.h"
#include "uif.h"

// WiFi Setting


#define PREF_ENABLE     0
#define SSID            "WSN-Default"
#define SEC_ENABLE      1
#define SEC_PASS        "WSN-PASSWORD"
#define AUTHMODE        0
#define DHCP_ENABLE     0
#define IP              "192.168.1.101"
#define BMASK           "255.255.255.0"
#define GATEWAY         "192.168.1.1"

#define BASE_IP         "192.168.1.2"
#define BASE_DATA_SPORT "8000"
#define BASE_DATA_DPORT "8255"
#define BASE_CMD_PORT   "2000"
#define BASE_HB_SPORT   "1234"
#define BASE_HB_DPORT   "55555"

// WiFi states
#define INIT            0
#define WIFICFG         1
#define NETWORK         2
#define DATACOMM        3

#define INIT_ABRD       0
#define INIT_FWUP       1
#define INIT_WAIT       2
#define NW_IPCONF       0xF0
#define NW_LISTEN       0xF1
#define NW_CONN         0xF2
#define NW_SEND         0xF3
#define NW_PARAMS       0xF4

// state machine flags
#define RECV_INIT       0
#define RECV_HEADER     1
#define RECV_DATA       2
#define RECV_EOL        3
#define RECV_EOLR		3
#define RECV_EOLN		4
#define RECV_READY      5

#define CMD_OK          1
#define CMD_ERR         2
#define PKT_IN          3

// pkt recv flags
#define pkt_socket      0
#define pkt_size        1
#define pkt_sip         2
#define pkt_sport       3
#define pkt_payload     4

// Redpine module state
#define RP_IDLE			0
#define RP_CMD			1
#define RP_SEND			2
#define RP_RECV			3

// Global Variables
extern BYTE RoocasID;

// WiFi structs
typedef struct _NETWORK_CONFIG{
	BYTE channel;
	BYTE networkType;
	BYTE secMode;
	BYTE dataRate;
	BYTE powerLevel;
	BYTE PSK[32];
    BYTE ssid[32];
	BYTE reserved[3];
	BYTE DHCP;
	BYTE IPAddr[4];
	BYTE netmask[4];
	BYTE defaultGW[4];
	
	BYTE mac[6];
} t_network_config;

typedef struct _SOCKET_CONFIG{
  	BYTE s_id;
	BYTE s_type;
	WORD sport;
	WORD dport;
	BYTE dip[4];
} t_socket_config;

typedef struct _ROUTE_INFO{
	BYTE dip[4];
	BYTE vip[4];
	WORD vport;
	BYTE hop;
	BYTE activate;
} t_route_info;

#define MAX_WIFI_DATA_LENGTH    128
#define MAX_WIFI_CMD_NUM        2
typedef struct _WIFI_STRUCT{
    BYTE state;
    BYTE subState;
    BYTE cmdMode;
    BYTE recvType;
    BYTE recvState;
    DWORD *recvHeader;
    BYTE recvHeaderCnt;
	BYTE cmd_sent;
    BYTE cmd_cnt;
    BYTE cmd_buf[MAX_WIFI_CMD_NUM][3];    // 0:type 1:length 2:idx to buff
	
    BYTE bWrIndex;
	BYTE bRdIndex;
	BYTE bOverflow;
	BYTE bRxCount;
    char cData[MAX_WIFI_DATA_LENGTH];
} WIFI_STRUCT;

#define MAX_SOCKET_TX_BUF_LEN   10
#define MAX_SOCKET_RX_BUF_LEN   512

typedef struct _SOCKET_ENTITY{
    BYTE socket_id;
    BYTE socket_type;       // udp or tcp
    BYTE listening;         // 1:server interface
    BYTE dest_ip[4][3];
    BYTE dest_port[5];
    BYTE src_port[5];

    BYTE sndState;      // 0: empty, 1: buffering
    BYTE txWrIndex;
    BYTE txRdIndex;
    BYTE txbuf[MAX_SOCKET_TX_BUF_LEN];

    BYTE recvState;
    WORD recvCnt;
    WORD recvLen;
    int rxWrIndex;
    int rxRdIndex;
    int rxCount;
	BYTE pktCount;
    BYTE rxbuf[MAX_SOCKET_RX_BUF_LEN];
} SOCKET_ENTITY;

// WiFi API functions
void WIFI_Test(unsigned char tid, void* msg);
void WIFI_Initialize(void);
void WIFI_Reset(void);
void WIFI_ABRD(void);
BYTE WIFI_START(void);
BYTE WIFI_FIND(void);
void WIFI_ASSOC(void);
void WIFI_Start_HB(void);
void WIFI_Stop_HB(void);
void WIFI_RECONF(void);
void WIFI_PROTO(void);
BYTE WIFI_CHK(void);
void WIFI_Sleep(void);
void WIFI_Wakeup(void);
BYTE WIFI_CheckStatus(void);
void WIFI_DefaultIBSS(void);
BYTE WIFI_ReadByte(char *pChar);
void WIFI_WriteFrame(char *pData, int iLen);
void WIFI_SoftReset(void);

void WIFI_SendHearBeatMsg(unsigned char tid, void* msg);
void WIFI_CheckNetwork(unsigned char tid, void* msg);
void WIFI_Wait4AvailableNetwork(unsigned char tid, void* msg);
void WIFI_ProcessUART(unsigned char tid, void* msg);
void WIFI_RelayToServer(unsigned char tid, void* msg);

void WIFI_RelayCMD(UIF_STRUCT *cmdBuf);
//void WIFI_SendTimeToChild(BYTE *timebuf);

// WiFi internal functions
void RP_ISR_WB(void);
//BYTE RP_WriteFrame(BYTE *pStr, int iLen);
BYTE RP_Wait_Recv_Ready(void);
int RP_WaitModuleResponse(char *resp);
BYTE RP_Wait_Recv_Ready_Long(void);
int RP_WaitModuleResponse_Long(char *resp);
//int RP_ByteStuffing(char *pData, int iLen, char *rData);


// Utility functions
int uint2str(int num, char *str);

// RedPine UART commands
//#define RP_EOL          "\r\n"
//#define RP_SEND_PREFIX	"at+rsi_"

#endif