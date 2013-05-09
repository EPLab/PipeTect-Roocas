#include "redpine.h"
#include "sys_uart.h"

void RP_test(void){
	RP_WriteCmd(WIFI_BAND, NULL, 0);
}

void RP_WriteCmd(rp_cmd_t cmd, char* body, int iLen){
	BYTE hasBody = 0;
    SYS_WriteFrameUART(WIFI_UART_MODE, RP_SEND_PREFIX, 7);

    switch(cmd){
        case WIFI_BAND:
            SYS_WriteFrameUART(WIFI_UART_MODE, "band=0", 6);
            break;
        case WIFI_INIT:
            SYS_WriteFrameUART(WIFI_UART_MODE, "init", 4);
            break;
        case WIFI_NUMSCAN:
			hasBody = 1;
            SYS_WriteFrameUART(WIFI_UART_MODE, "numscan=", 8);
            break;
//        case WIFI_PASSSCAN:
//			hasBody = 1;
//            SYS_WriteFrameUART(WIFI_UART_MODE, "passscan=", 9);
//            break;
        case WIFI_SCAN:
			hasBody = 1;
            SYS_WriteFrameUART(WIFI_UART_MODE, "scan=", 5);
            break;
        case WIFI_BSSID:
            SYS_WriteFrameUART(WIFI_UART_MODE, "bssid=", 6);
            break;
        case WIFI_NWTYPE:
            SYS_WriteFrameUART(WIFI_UART_MODE, "nwtype=", 7);
            break;
        case WIFI_NETWORK:
            SYS_WriteFrameUART(WIFI_UART_MODE, "network=", 8);
            break;
        case WIFI_PSK:
            SYS_WriteFrameUART(WIFI_UART_MODE, "psk=", 4);
            break;
        case WIFI_AUTHMODE:
            SYS_WriteFrameUART(WIFI_UART_MODE, "authmode=", 8);
            break;
        case WIFI_JOIN:
            SYS_WriteFrameUART(WIFI_UART_MODE, "join=", 5);
            break;
        case WIFI_DISASSOC:
            SYS_WriteFrameUART(WIFI_UART_MODE, "disassoc", 8);
            break;
        case WIFI_PWMODE:
            SYS_WriteFrameUART(WIFI_UART_MODE, "pwmode=", 7);
            break;
//        case WIFI_SLEEPTIMER:
//            SYS_WriteFrameUART(WIFI_UART_MODE, "sleeptimer=", 11);
//            break;
        case NW_IPCONF:
            SYS_WriteFrameUART(WIFI_UART_MODE, "ipconf=", 7);
            break;
        case NW_LTCP:
            SYS_WriteFrameUART(WIFI_UART_MODE, "ltcp=", 5);
            break;
        case NW_TCP:
            SYS_WriteFrameUART(WIFI_UART_MODE, "tcp=", 4);
            break;
        case NW_CTCP:
            SYS_WriteFrameUART(WIFI_UART_MODE, "ctcp=", 5);
            break;
        case NW_LUDP:
            SYS_WriteFrameUART(WIFI_UART_MODE, "ludp=", 5);
            break;
        case NW_UDP:
            SYS_WriteFrameUART(WIFI_UART_MODE, "udp=", 4);
            break;
//        case NW_MULTICAST:
//            SYS_WriteFrameUART(WIFI_UART_MODE, "multicast=", 10);
//            break;
        case NW_CLS:
            SYS_WriteFrameUART(WIFI_UART_MODE, "cls=", 4);
            break;
        case NW_SND:
            SYS_WriteFrameUART(WIFI_UART_MODE, "snd=", 4);
            break;
//        case NW_DNSSERVER:
//            SYS_WriteFrameUART(WIFI_UART_MODE, "dnsserver=", 10);
//            break;
//        case NW_DNSGET:
//            SYS_WriteFrameUART(WIFI_UART_MODE, "dnsget=", 7);
//            break;
        case MISC_FWVERSION:
            SYS_WriteFrameUART(WIFI_UART_MODE, "fwversion?", 10);
            break;
        case MISC_NWPARAMS:
            SYS_WriteFrameUART(WIFI_UART_MODE, "nwparams?", 9);
            break;
        case MISC_RESET:
            SYS_WriteFrameUART(WIFI_UART_MODE, "reset", 5);
            break;
//        case MISC_BAUDRATE  :
//            SYS_WriteFrameUART(WIFI_UART_MODE, "baudrate=", 9);
//            break;
        case MISC_MAC:
            SYS_WriteFrameUART(WIFI_UART_MODE, "mac?", 4);
            break;
        case MISC_RSSI:
            SYS_WriteFrameUART(WIFI_UART_MODE, "rssi?", 5);
            break;
        case MISC_CFGSAVE:
            SYS_WriteFrameUART(WIFI_UART_MODE, "cfgsave", 7);
            break;
        case MISC_CFGGET:
            SYS_WriteFrameUART(WIFI_UART_MODE, "cfgget?", 7);
            break;
        default:
            break;
    }
	
	if (hasBody){
		SYS_WriteFrameUART(WIFI_UART_MODE, body, iLen);
	}
	
    SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
}

BYTE RP_WriteFrame(BYTE *pStr, int iLen){
    SYS_WriteFrameUART(WIFI_UART_MODE, pStr, iLen);
    SYS_WriteFrameUART(WIFI_UART_MODE, RP_EOL , 2);
    return 1;
}

int RP_ByteStuffing(char *pData, int iLen, char *rData){
    int i;
    int rLen = iLen;
    int rIdx = 0;

    for (i = 0; i < iLen; i++){
        if (pData[i] == 0x0D && pData[i + 1] == 0x0A){
            rData[rIdx++] = 0xDB;
            rData[rIdx++] = 0xDC;
            i++;
        } else if (pData[i] == 0xDB){
            rData[rIdx++] = 0xDB;
            rData[rIdx++] = 0xDD;
            rLen += 1;
        } else{
            rData[rIdx++] = pData[i];
        }
    }

    return rLen;
}