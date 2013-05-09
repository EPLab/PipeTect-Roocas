//******************************************************************************
//	 MAIN.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 02/28/2009
//******************************************************************************
#include "sys.h"
#include "sys_timer.h"
#include "rtc.h"
#include "gps.h"
#include "wwvb.h"
#include "acc.h"
#include "cam.h"
#include "xbee.h"
#include "xstream.h"
#include "rfm.h"
//#include "fatlib.h"
//#include "mem.h"
#include "switcher.h"
#include "wifi.h"

#ifdef PROBE_ENABLED
#include "probe.h"
#endif

////////////////////////////////////////////////////////////////
// SD and FRAM
////////////////////////////////////////////////////////////////
//#include "fram.h"
//#include "fat_filelib.h"
#ifdef __LOCAL_STORE_ENABLED__
	#include "local_store.h"
#endif

#define OPERATION_MODE 1


int __low_level_init(void)
{
    /* Insert your low-level initializations here */
    WDTCTL = WDTPW + WDTHOLD; // Stop Watchdog timer

    /*==================================*/
    /* Choose if segment initialization */
    /* should be done or not. */
    /* Return: 0 to omit seg_init */
    /* 1 to run seg_init */
    /*==================================*/

    return (1);
}



void main(void)
{
    // Initialize System parameter
    SYS_Initialize();

#ifdef PROBE_ENABLED
	probe_setup();
#endif
	
    initSwitcher(1);

	// Initialize peripherals
#ifdef RTC_ENABLE
    RTC_Initialize();
#endif

#ifdef GPS_ENABLE
    GPS_Initialize();	
#endif

#ifdef WWVB_ENABLE
    WWVB_Initialize();
#endif

#ifdef XSTREAM_ENABLE
    XSTREAM_Initialize();
#endif

#ifdef XBEE_ENABLE
    XBEE_Initialize();
#endif

#ifdef RFM_ENABLE
    RFM_Initialize();
#endif

#ifdef MMC_ENABLE
    temp = FAT_Initialize();
    while (temp != 1)
    {
        temp = FAT_Initialize();
    }
#endif

#ifdef FRAM_ENABLE
    //MEM_Initialize();
	//FRAM_Initialize();
#endif
	
#ifdef __LOCAL_STORE_ENABLED__
	LOS_init();
#endif

#ifdef WIFI_ENABLE
    WIFI_Initialize();
#endif

#ifdef CAN_ENABLE
    ACC_Initialize();
    //SYS_InitClock(SYS_LOW_CLK);
    //__bis_SR_register(LPM3_bits);             // Enter LPM3
  __no_operation();                         // For debugger
#endif
	
#ifdef OPERATION_MODE
//    RFM_Test();
//    ACC_Process();
//    GPS_Process();
//    GPS_Test();
    Switcher();
#else
    // Test peripherals
//    SYS_TestTimer();
//    RTC_Test();
//    GPS_Test();
//    WWVB_Test();
//    ACC_Test();
//    CAM_Test();
//    XSTREAM_Test();
//    XBEE_Test();
//    RFM_Test();
//    FAT_Test();
//    MEM_Test();
#endif
}
