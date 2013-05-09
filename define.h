//******************************************************************************
//	 DEFINE.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 02/28/2009
//******************************************************************************
#include "msp430x54x.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define CAN_ENABLE      1
//#define GPS_ENABLE    1
#define RTC_ENABLE      1
//#define WWVB_ENABLE   1
//#define MMC_ENABLE      1
//#define FRAM_ENABLE     1
//#define XBEE_ENABLE     1
//#define XSTREAM_ENABLE  1
//#define RFM_ENABLE      1
#define WIFI_ENABLE     1

#define BYTE unsigned char
#define BOOL unsigned char
#define WORD unsigned short
#define DWORD unsigned long

#define BIT_0   0x01
#define BIT_1   0x02
#define BIT_2   0x04
#define BIT_3   0x08
#define BIT_4   0x10
#define BIT_5   0x20
#define BIT_6   0x40
#define BIT_7   0x80

#define BIT_8	0x0100
#define BIT_9	0x0200
#define BIT_10	0x0400
#define BIT_11	0x0800
#define BIT_12	0x1000
#define BIT_13	0x2000
#define BIT_14	0x4000
#define BIT_15	0x8000

#define INPUT	0
#define OUTPUT	1

#define LOW 	0
#define HIGH   	1

#define SET     1
#define CLEAR   0

#define TRUE	1
#define FALSE	0

#define ENABLE	1
#define DISABLE	0

#define MAX_CMD_LEN	8		
#define MAX_DATA_LEN	256	

// CAN port definitions
#define CAN_SIMO_PxSEL         	P3SEL
#define CAN_SIMO_PxDIR         	P3DIR
#define CAN_SIMO_PxOUT         	P3OUT
#define CAN_SIMO_BIT           	BIT1

#define CAN_SOMI_PxSEL         	P3SEL
#define CAN_SOMI_PxDIR         	P3DIR
#define CAN_SOMI_PxIN          	P3IN
#define CAN_SOMI_BIT           	BIT2

#define CAN_SCK_PxSEL         	P3SEL
#define CAN_SCK_PxDIR         	P3DIR
#define CAN_SCK_PxOUT         	P3OUT
#define CAN_SCK_BIT           	BIT3

#ifndef __ROOCAS2
    #define CAN_CS_PxSEL         	P3SEL
    #define CAN_CS_PxDIR         	P3DIR
    #define CAN_CS_PxOUT         	P3OUT
    #define CAN_CS_BIT           	BIT0

    #define CAN_RESET_PxSEL         P2SEL
    #define CAN_RESET_PxDIR         P2DIR
    #define CAN_RESET_PxOUT         P2OUT
    #define CAN_RESET_BIT           BIT7

    #define CAN_INT_PxSEL         	P2SEL
    #define CAN_INT_PxDIR         	P2DIR
    #define CAN_INT_PxIN         	P2IN
    #define CAN_INT_BIT           	BIT6
#else
    #define CAN_CS_PxSEL         	P3SEL
    #define CAN_CS_PxDIR         	P3DIR
    #define CAN_CS_PxOUT         	P3OUT
    #define CAN_CS_BIT           	BIT0

    #define CAN_RESET_PxSEL         P4SEL
    #define CAN_RESET_PxDIR         P4DIR
    #define CAN_RESET_PxOUT         P4OUT
    #define CAN_RESET_BIT           BIT6

    #define CAN_INT_PxSEL         	P1SEL
    #define CAN_INT_PxDIR         	P1DIR
    #define CAN_INT_PxIN         	P1IN
    #define CAN_INT_BIT           	BIT7
#endif

// MMC port definitions
#define MMC_SIMO_PxSEL         	P3SEL
#define MMC_SIMO_PxDIR         	P3DIR
#define MMC_SIMO_PxOUT         	P3OUT
#define MMC_SIMO_BIT           	BIT7

#define MMC_SOMI_PxSEL         	P5SEL
#define MMC_SOMI_PxDIR         	P5DIR
#define MMC_SOMI_PxIN          	P5IN
#define MMC_SOMI_BIT           	BIT4

#define MMC_SCK_PxSEL         	P5SEL
#define MMC_SCK_PxDIR         	P5DIR
#define MMC_SCK_PxOUT         	P5OUT
#define MMC_SCK_BIT           	BIT5

#ifndef __ROOCAS2
    #define MMC_CS_PxSEL         	P4SEL
    #define MMC_CS_PxDIR         	P4DIR
    #define MMC_CS_PxOUT         	P4OUT
    #define MMC_CS_BIT           	BIT0

    #define MMC_CD_BIT           	BIT7
#else
    #define MMC_CS_PxSEL         	P3SEL
    #define MMC_CS_PxDIR         	P3DIR
    #define MMC_CS_PxOUT         	P3OUT
    #define MMC_CS_BIT           	BIT6

    #define MMC_CD_BIT           	BIT7
#endif

#define MMC_CD_PxSEL         	P4SEL
#define MMC_CD_PxDIR         	P4DIR
#define MMC_CD_PxIN          	P4IN


// GPS port definitions
#define GPS_RX_PxSEL         	P10SEL
#define GPS_RX_PxDIR         	P10DIR
#define GPS_RX_PxOUT         	P10OUT
#define GPS_RX_BIT           	BIT4

#define GPS_TX_PxSEL         	P10SEL
#define GPS_TX_PxDIR         	P10DIR
#define GPS_TX_PxIN          	P10IN
#define GPS_TX_BIT           	BIT5

#define GPS_ENABLE_PxSEL       	P10SEL
#define GPS_ENABLE_PxDIR        P10DIR
#define GPS_ENABLE_PxOUT        P10OUT
#define GPS_ENABLE_BIT          BIT7

#define GPS_RESET_PxSEL         P10SEL
#define GPS_RESET_PxDIR         P10DIR
#define GPS_RESET_PxOUT         P10OUT
#define GPS_RESET_BIT           BIT6

#define GPS_PPS_PxSEL         	P1SEL
#define GPS_PPS_PxDIR         	P1DIR
#define GPS_PPS_PxOUT           P1OUT
#define GPS_PPS_PxIN            P1IN
#define GPS_PPS_PxREN           P1REN
#define GPS_PPS_PxIE            P1IE
#define GPS_PPS_PxIES           P1IES
#define GPS_PPS_PxIFG           P1IFG
#define GPS_PPS_BIT           	BIT0

#define GPS_BOOTSEL_PxSEL     	P11SEL
#define GPS_BOOTSEL_PxDIR      	P11DIR
#define GPS_BOOTSEL_PxOUT       P11OUT
#define GPS_BOOTSEL_BIT        	BIT0

// XBEE port definitions
#define XBEE_RX_PxSEL         	P9SEL
#define XBEE_RX_PxDIR         	P9DIR
#define XBEE_RX_PxOUT         	P9OUT
#define XBEE_RX_BIT           	BIT4

#define XBEE_TX_PxSEL         	P9SEL
#define XBEE_TX_PxDIR         	P9DIR
#define XBEE_TX_PxIN          	P9IN
#define XBEE_TX_BIT           	BIT5

#define XBEE_RESET_PxSEL        P8SEL
#define XBEE_RESET_PxDIR        P8DIR
#define XBEE_RESET_PxOUT        P8OUT
#define XBEE_RESET_BIT          BIT4

#define XBEE_SLEEPRQ_PxSEL      P9SEL
#define XBEE_SLEEPRQ_PxDIR      P9DIR
#define XBEE_SLEEPRQ_PxOUT      P9OUT
#define XBEE_SLEEPRQ_BIT        BIT6

// XSTREAM port definitions
#define XSTREAM_RX_PxSEL       	P10SEL
#define XSTREAM_RX_PxDIR        P10DIR
#define XSTREAM_RX_PxOUT        P10OUT
#define XSTREAM_RX_BIT          BIT4

#define XSTREAM_TX_PxSEL        P10SEL
#define XSTREAM_TX_PxDIR        P10DIR
#define XSTREAM_TX_PxIN         P10IN
#define XSTREAM_TX_BIT          BIT5

#define XSTREAM_RESET_PxSEL     P10SEL
#define XSTREAM_RESET_PxDIR     P10DIR
#define XSTREAM_RESET_PxOUT     P10OUT
#define XSTREAM_RESET_BIT       BIT6

#define XSTREAM_ENABLE_PxSEL    P11SEL
#define XSTREAM_ENABLE_PxDIR    P11DIR
#define XSTREAM_ENABLE_PxOUT    P11OUT
#define XSTREAM_ENABLE_BIT      BIT2

// FRAM port definitions
#define FRAM_SIMO_PxSEL         P3SEL
#define FRAM_SIMO_PxDIR         P3DIR
#define FRAM_SIMO_PxOUT         P3OUT
#define FRAM_SIMO_BIT           BIT7

#define FRAM_SOMI_PxSEL         P5SEL
#define FRAM_SOMI_PxDIR         P5DIR
#define FRAM_SOMI_PxIN          P5IN
#define FRAM_SOMI_BIT           BIT4

#define FRAM_SCK_PxSEL          P5SEL
#define FRAM_SCK_PxDIR          P5DIR
#define FRAM_SCK_PxOUT          P5OUT
#define FRAM_SCK_BIT            BIT5

#ifndef __ROOCAS2
    #define FRAM1_CS_PxSEL         	P4SEL
    #define FRAM1_CS_PxDIR         	P4DIR
    #define FRAM1_CS_PxOUT         	P4OUT
    #define FRAM1_CS_BIT           	BIT1

    #define FRAM1_HOLD_PxSEL        P4SEL
    #define FRAM1_HOLD_PxDIR        P4DIR
    #define FRAM1_HOLD_PxOUT        P4OUT
    #define FRAM1_HOLD_BIT          BIT4

    #define FRAM2_CS_PxSEL         	P4SEL
    #define FRAM2_CS_PxDIR         	P4DIR
    #define FRAM2_CS_PxOUT         	P4OUT
    #define FRAM2_CS_BIT           	BIT3

    #define FRAM2_HOLD_PxSEL        P4SEL
    #define FRAM2_HOLD_PxDIR        P4DIR
    #define FRAM2_HOLD_PxOUT        P4OUT
    #define FRAM2_HOLD_BIT          BIT2
#else
    #define FRAM1_CS_PxSEL         	P4SEL
    #define FRAM1_CS_PxDIR         	P4DIR
    #define FRAM1_CS_PxOUT         	P4OUT
    #define FRAM1_CS_BIT           	BIT0

    #define FRAM2_CS_PxSEL         	P4SEL
    #define FRAM2_CS_PxDIR         	P4DIR
    #define FRAM2_CS_PxOUT         	P4OUT
    #define FRAM2_CS_BIT           	BIT1

    #define FRAM_HOLD_PxSEL        P4SEL
    #define FRAM_HOLD_PxDIR        P4DIR
    #define FRAM_HOLD_PxOUT        P4OUT
    #define FRAM_HOLD_BIT          BIT4
#endif


// RTC port definitions
#define RTC_SIMO_PxSEL         	P10SEL
#define RTC_SIMO_PxDIR         	P10DIR
#define RTC_SIMO_PxOUT         	P10OUT
#define RTC_SIMO_BIT           	BIT1

#define RTC_SOMI_PxSEL         	P10SEL
#define RTC_SOMI_PxDIR         	P10DIR
#define RTC_SOMI_PxIN          	P10IN
#define RTC_SOMI_BIT           	BIT2

#define RTC_SCK_PxSEL         	P10SEL
#define RTC_SCK_PxDIR         	P10DIR
#define RTC_SCK_PxOUT         	P10OUT
#define RTC_SCK_BIT           	BIT3

#define RTC_CS_PxSEL         	P10SEL
#define RTC_CS_PxDIR         	P10DIR
#define RTC_CS_PxOUT         	P10OUT
#define RTC_CS_BIT           	BIT0

#define RTC_INT_PxSEL         	P1SEL
#define RTC_INT_PxDIR         	P1DIR
#define RTC_INT_PxOUT           P1OUT
#define RTC_INT_PxIN            P1IN
#define RTC_INT_PxREN           P1REN
#define RTC_INT_PxIE            P1IE
#define RTC_INT_PxIES           P1IES
#define RTC_INT_PxIFG           P1IFG
#define RTC_INT_BIT           	BIT3

// WWVB port definitions
#define WWVB_ENABLE_PxSEL       P1SEL
#define WWVB_ENABLE_PxDIR       P1DIR
#define WWVB_ENABLE_PxOUT       P1OUT
#define WWVB_ENABLE_BIT         BIT5

#define WWVB_HOLD_PxSEL         P1SEL
#define WWVB_HOLD_PxDIR         P1DIR
#define WWVB_HOLD_PxOUT         P1OUT
#define WWVB_HOLD_BIT           BIT2

#define WWVB_TDATA_PxSEL        P1SEL
#define WWVB_TDATA_PxDIR        P1DIR
#define WWVB_TDATA_PxIN         P1IN
#define WWVB_TDATA_BIT          BIT4

// RFM port definitions
#define RFM_RX_PxSEL         	P9SEL
#define RFM_RX_PxDIR         	P9DIR
#define RFM_RX_PxOUT         	P9OUT
#define RFM_RX_BIT           	BIT4

#define RFM_TX_PxSEL         	P9SEL
#define RFM_TX_PxDIR         	P9DIR
#define RFM_TX_PxIN          	P9IN
#define RFM_TX_BIT           	BIT5

#ifdef __ROOCAS2
    #define WIFI_U_RX_PxSEL     RFM_RX_PxSEL
    #define WIFI_U_RX_PxDIR     RFM_RX_PxDIR
    #define WIFI_U_RX_PxOUT     RFM_RX_PxOUT
    #define WIFI_U_RX_BIT       RFM_RX_BIT

    #define WIFI_U_TX_PxSEL     RFM_TX_PxSEL
    #define WIFI_U_TX_PxDIR     RFM_TX_PxDIR
    #define WIFI_U_TX_PxOUT     RFM_TX_PxIN
    #define WIFI_U_TX_BIT       RFM_TX_BIT
#endif

#ifndef __ROOCAS2
    #define RFM_ENABLE_PxSEL       	P9SEL
    #define RFM_ENABLE_PxDIR        P9DIR
    #define RFM_ENABLE_PxOUT        P9OUT
    #define RFM_ENABLE_BIT          BIT6

    #define RFM_RESET_PxSEL       	P8SEL
    #define RFM_RESET_PxDIR         P8DIR
    #define RFM_RESET_PxOUT         P8OUT
    #define RFM_RESET_BIT           BIT4

    #define RFM_WAKEIN_PxSEL       	P8SEL
    #define RFM_WAKEIN_PxDIR        P8DIR
    #define RFM_WAKEIN_PxOUT        P8OUT
    #define RFM_WAKEIN_BIT          BIT3

    #define RFM_WAKEOUT_PxSEL      	P8SEL
    #define RFM_WAKEOUT_PxDIR       P8DIR
    #define RFM_WAKEOUT_PxIN        P8IN
    #define RFM_WAKEOUT_BIT         BIT2
#else
    #define WIFI_RESET_PxSEL       	P8SEL
    #define WIFI_RESET_PxDIR        P8DIR
    #define WIFI_RESET_PxOUT        P8OUT
    #define WIFI_RESET_BIT          BIT4

    #define WIFI_U_RTS_PxSEL        P1SEL
    #define WIFI_U_RTS_PxDIR        P1DIR
    #define WIFI_U_RTS_PxIN         P1IN
    #define WIFI_U_RTS_BIT          BIT5

    #define WIFI_U_CTS_PxSEL        P1SEL
    #define WIFI_U_CTS_PxDIR        P1DIR
    #define WIFI_U_CTS_PxIN         P1OUT
    #define WIFI_U_CTS_BIT          BIT6
#endif
