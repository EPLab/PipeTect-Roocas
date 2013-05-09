#ifndef _REDPINE_H

#include "define.h"

#define RP_EOL          "\r\n"
#define RP_SEND_PREFIX	"at+rsi_"

typedef struct RP_INFO{
	char mac[6];
	char fwver[4];
} rp_info_t;

typedef enum RP_CMD{
    WIFI_BAND = 0x00,
    WIFI_INIT,
    WIFI_NUMSCAN,
    WIFI_PASSSCAN,
    WIFI_SCAN,
    WIFI_BSSID,
    WIFI_NWTYPE,
    WIFI_NETWORK,
    WIFI_PSK,
    WIFI_AUTHMODE,
    WIFI_JOIN,
    WIFI_DISASSOC,
    WIFI_PWMODE,
    WIFI_SLEEPTIMER,
    NW_IPCONF,
    NW_LTCP,
    NW_TCP,
    NW_CTCP,
    NW_LUDP,
    NW_UDP,
    NW_MULTICAST,
    NW_CLS,
    NW_SND,
    NW_DNSSERVER,
    NW_DNSGET,
    MISC_FWVERSION,
    MISC_NWPARAMS,
    MISC_RESET,
    MISC_BAUDRATE,
    MISC_MAC,
    MISC_RSSI,
    MISC_CFGSAVE,
    MISC_CFGGET
} rp_cmd_t;

typedef enum RP_CMD_RESP_CODE{
	CMD_OK,
	CMD_ERROR,
	SOCKET_CLOSE
} rp_cmd_resp_code_t;

typedef struct RP_CMD_BAND_RESP{
	rp_cmd_resp_code_t rCode;
	BYTE errCode;
} rp_cmd_band_resp_t;

typedef struct RP_CMD_INIT_RESP{
	rp_cmd_resp_code_t rCode;
	BYTE errCode;
} rp_cmd_init_resp_t;

typedef struct RP_CMD_NUMSCAN_RESP{
	rp_cmd_resp_code_t rCode;
	union{
		BYTE retvalue;
		BYTE errCode;
	};
} rp_cmd_numscan_resp_t;

typedef struct RP_CMD_SCAN_RESP_CONTENT{
	char uSSID[32];
	BYTE uMode;
	BYTE uRSSI;
} rp_cmd_scan_resp_content_t;

typedef struct RP_CMD_SCAN_RESP{
	rp_cmd_resp_code_t rCode;
	union{
		rp_cmd_scan_resp_content_t ap[12];
		BYTE errCode;
	};
} rp_cmd_scan_resp_t; //max len: 2 + 34 x 12 = 410


typedef struct{
	union{
		BYTE buffer[512];
		rp_cmd_band_resp_t		band_resp;
		rp_cmd_init_resp_t		init_resp;
		rp_cmd_numscan_resp_t	numscan_resp;
		rp_cmd_scan_resp_t		scan_resp;
	};
} rp_cmd_resp_t;


typedef enum RP_MODULE_STATE{
	INIT,
	CONFIG
	
} t_rp_module_state;

typedef enum RP_RECV_STATE{
	RECV_INIT,
	RECV_HEADER,
	RECV_DATA,
	RECV_EOLR,
	RECV_EOLN
} t_rp_recv_state;

typedef enum RP_RECV_PKT_STATE{
	PKT_SNUM,
	PKT_SIZE,
	PKT_SIP,
	PKT_SPORT,
	PKT_STREAM
} t_rp_recv_pkt_state;

// functions
void RP_test(void);
void RP_ISR(void);
void RP_WriteCmd(rp_cmd_t cmd, char* body, int iLen);
BYTE RP_WriteFrame(BYTE *pStr, int iLen);
int RP_ByteStuffing(char* pData, int iLen, char* rData);

#endif