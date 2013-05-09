//******************************************************************************
//	 SYS.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 02/28/2009
//******************************************************************************
#include "sys.h"
#include "sys_spi.h"
#include "sys_uart.h"
#include "sys_timer.h"
#include "sys_rtc.h"
#include "dma_uart.h"

void SYS_Initialize(void)
{
	// Initialize Clock
//	SYS_InitClock(SYS_LOW_CLK);
	SYS_InitClock(SYS_HIGH_CLK);

    // Initialize internal timer	
    SYS_InitTimer();
	
    // Initialize internal RTC	
//  SYS_InitRTC();

 	// Initialize SPI Module
#ifdef CAN_ENABLE
	SYS_InitSPI(CAN_SPI_MODE);       // UCB0
#endif

#ifdef MMC_ENABLE
	SYS_InitSPI(MMC_SPI_MODE);       // UCB1
#endif

#ifdef FRAM_ENABLE
    SYS_InitSPI(FRAM_SPI_MODE);      // UCB1
#endif

#ifdef RTC_ENABLE
	SYS_InitSPI(RTC_SPI_MODE);       // UCB3
#endif

	// Initialize UART Module
#ifdef GPS_ENABLE
	SYS_InitUART(GPS_UART_MODE, SYS_CLK, BPS_4800);      // UCA3
#endif

#ifdef XSTREAM_ENABLE
	SYS_InitUART(XSTREAM_UART_MODE, SYS_CLK, BPS_9600);  // UCA3
#endif

#ifdef XBEE_ENABLE
	SYS_InitUART(XBEE_UART_MODE, SYS_CLK, BPS_115200);   // UCA2
#endif

#ifdef RFM_ENABLE
    SYS_InitUART(RFM_UART_MODE, SYS_CLK, BPS_460800);    // UCA2
	//SYS_InitUART(RFM_UART_MODE, SYS_CLK, BPS_921600);    // UCA2	
#endif

#ifdef WIFI_ENABLE
	SYS_InitUART(WIFI_UART_MODE, SYS_CLK, BPS_460800);   // UCA2
//    SYS_InitUART(WIFI_UART_MODE, SYS_CLK, BPS_921600);   // UCA2
#endif


	SYS_InitPort();

	__bis_SR_register(GIE);       // interrupts enabled
}

// Initialize system clock : low clock (SYS_CLK1), high clock (SYS_CLK2)
void SYS_InitClock(BYTE bMode)
{
	int i;
	
	if (bMode == SYS_LOW_CLK)
	{
	  	WDTCTL = (WDTPW + WDTHOLD);               // Stop watchdog timer
	
//	  	P11DIR |= BIT0;                           // P11.0 to output direction
//	  	P11SEL |= BIT0;                           // P11.0 to output ACLK
	  	P7SEL |= 0x03;                            // Select XT1
	
	  	UCSCTL6 &= (~XT1OFF);                     // XT1 On
	  	UCSCTL6 |= XCAP_3;                        // Internal load cap
	  	UCSCTL3 = 0;                              // FLL Reference Clock = XT1
	
	  	UCSCTL4 |= (SELA_0 + (SELS_4 + SELM_4));  // ACLK = LFTX1
	                                              // SMCLK = default DCO
	                                              // MCLK = default DCO
	  	// Loop until XT1 & DCO stabilizes
	  	while ( (SFRIFG1 &OFIFG))
        {
	    	UCSCTL7 &= (~(XT1LFOFFG + DCOFFG));   // Clear XT1,DCO fault flags
	    	SFRIFG1 &= (~OFIFG);                  // Clear fault flags
	  	}
	
	  	UCSCTL6 &= ~(XT1DRIVE_3);            // Xtal is now stable, reduce drive strength	
	}
	else
	{
	    WDTCTL = (WDTPW + WDTHOLD);          // Stop WDT
	
//        P11DIR = (BIT1 + BIT2);             // P11.1-2 to output direction
//        P11SEL |= (BIT1 + BIT2);            // P11.1-2 to output SMCLK,MCLK
            P5SEL |= 0x0C;                      // Port select XT2

            UCSCTL6 &= ~XT2OFF;                 // Enable XT2 even if not used
            UCSCTL3 |= SELREF_2;                // FLLref = REFO
                                            // Since LFXT1 is not used,
                                            // sourcing FLL with LFXT1 can cause
                                            // XT1OFFG flag to set
            UCSCTL4 |= SELA_2;                  // ACLK=REFO,SMCLK=DCO,MCLK=DCO

            // Loop until XT2 & DCO stabilize
            do
            {
                UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
                                                // Clear XT2,XT1,DCO fault flags
                SFRIFG1 &= ~OFIFG;              // Clear fault flags
                for(i=0; i<0xFFFF; i++);        // Delay for Osc to stabilize
            } while (SFRIFG1 & OFIFG);          // Test oscillator fault flag

            UCSCTL4 |= (SELS_5 | SELM_5);       // SMCLK=MCLK=XT2
#if SYS_CLK2 < 16000000
            UCSCTL6 &= 0x7FFF;
#endif
        }
}

void SYS_DelayMS(WORD ms)
{
    WORD i;

    for (i=0; i<ms; i++)
    {
        __delay_cycles((DWORD)(SYS_CLK/1000));
    }
}

// Initialize Port : MSP430F5438
void SYS_InitPort(void)
{
 	// Init Port for CAN
#ifdef CAN_ENABLE
	CAN_RESET_PxSEL &= ~CAN_RESET_BIT;
	CAN_RESET_PxDIR |= CAN_RESET_BIT;
	CAN_RESET_PxOUT |= CAN_RESET_BIT;

  	CAN_CS_PxSEL &= ~CAN_CS_BIT;
  	CAN_CS_PxDIR |= CAN_CS_BIT;
  	CAN_CS_PxOUT |= CAN_CS_BIT;

        CAN_INT_PxSEL &= ~CAN_INT_BIT;
        CAN_INT_PxDIR &= ~CAN_INT_BIT;

        //P2IES |= CAN_INT_BIT;
        //P2IE |= CAN_INT_BIT;
        //P2IFG &= ~CAN_INT_BIT;
#endif

 	// Init Port for MMC
#if defined(MMC_ENABLE) || defined(__DMA_MMC_ENABLED__)
  	MMC_CS_PxSEL &= ~MMC_CS_BIT;
  	MMC_CS_PxDIR |= MMC_CS_BIT;
  	MMC_CS_PxOUT |= MMC_CS_BIT;

 	MMC_CD_PxSEL &= ~MMC_CD_BIT;
 	MMC_CD_PxDIR &= ~MMC_CD_BIT;
#endif

 	// Init Port for FRAM
#ifdef FRAM_ENABLE
    FRAM1_CS_PxSEL &= ~FRAM1_CS_BIT;
    FRAM1_CS_PxDIR |= FRAM1_CS_BIT;
    FRAM1_CS_PxOUT |= FRAM1_CS_BIT;

    FRAM2_CS_PxSEL &= ~FRAM2_CS_BIT;
    FRAM2_CS_PxDIR |= FRAM2_CS_BIT;
    FRAM2_CS_PxOUT |= FRAM2_CS_BIT;

    #ifndef __ROOCAS2
        FRAM1_HOLD_PxSEL &= ~FRAM1_HOLD_BIT;
        FRAM1_HOLD_PxDIR |= FRAM1_HOLD_BIT;
        FRAM1_HOLD_PxOUT |= FRAM1_HOLD_BIT;

        FRAM2_HOLD_PxSEL &= ~FRAM2_HOLD_BIT;
        FRAM2_HOLD_PxDIR |= FRAM2_HOLD_BIT;
        FRAM2_HOLD_PxOUT |= FRAM2_HOLD_BIT;
    #else
        FRAM_HOLD_PxSEL &= ~FRAM_HOLD_BIT;
        FRAM_HOLD_PxDIR |= FRAM_HOLD_BIT;
        FRAM_HOLD_PxOUT |= FRAM_HOLD_BIT;
    #endif
#endif

 	// Init Port for GPS
#ifdef GPS_ENABLE
	GPS_ENABLE_PxSEL &= ~GPS_ENABLE_BIT;
	GPS_ENABLE_PxDIR |= GPS_ENABLE_BIT;
	GPS_ENABLE_PxOUT &= ~GPS_ENABLE_BIT;

	GPS_RESET_PxSEL &= ~GPS_RESET_BIT;
	GPS_RESET_PxDIR |= GPS_RESET_BIT;
	GPS_RESET_PxOUT |= GPS_RESET_BIT;

    GPS_PPS_PxSEL &= ~GPS_PPS_BIT;
    GPS_PPS_PxDIR &= ~GPS_PPS_BIT;
    GPS_PPS_PxREN |= GPS_PPS_BIT;
    GPS_PPS_PxOUT |= GPS_PPS_BIT;
    GPS_PPS_PxIE |= GPS_PPS_BIT;
    GPS_PPS_PxIES |= GPS_PPS_BIT;
    GPS_PPS_PxIFG &= ~GPS_PPS_BIT;

    GPS_BOOTSEL_PxSEL &= ~GPS_BOOTSEL_BIT;
    GPS_BOOTSEL_PxDIR |= GPS_BOOTSEL_BIT;
    GPS_BOOTSEL_PxOUT &= ~GPS_BOOTSEL_BIT;
#endif

 	// Init Port for RTC
#ifdef RTC_ENABLE
    RTC_CS_PxSEL &= ~RTC_CS_BIT;
    RTC_CS_PxDIR |= RTC_CS_BIT;
    RTC_CS_PxOUT |= RTC_CS_BIT;

    RTC_INT_PxSEL &= ~RTC_INT_BIT;
    RTC_INT_PxDIR &= ~RTC_INT_BIT;
    RTC_INT_PxREN |= RTC_INT_BIT;
    RTC_INT_PxOUT |= RTC_INT_BIT;
    RTC_INT_PxIE |= RTC_INT_BIT;
    RTC_INT_PxIES |= RTC_INT_BIT;
    RTC_INT_PxIFG &= ~RTC_INT_BIT;
#endif

 	// Init Port for RFM
#ifdef RFM_ENABLE
    #ifndef __ROOCAS2
        RFM_WAKEOUT_PxSEL &= ~RFM_WAKEOUT_BIT;
        RFM_WAKEOUT_PxDIR &= ~RFM_WAKEOUT_BIT;

        RFM_WAKEIN_PxSEL &= ~RFM_WAKEIN_BIT;
        RFM_WAKEIN_PxDIR |= RFM_WAKEIN_BIT;

        RFM_RESET_PxSEL &= ~RFM_RESET_BIT;
        RFM_RESET_PxDIR |= RFM_RESET_BIT;
    #else
        WIFI_RESET_PxSEL &= ~WIFI_RESET_BIT;
        WIFI_RESET_PxDIR |= WIFI_RESET_BIT;

        WIFI_U_RTS_PxSEL &= ~WIFI_U_RTS_BIT;
        WIFI_U_RTS_PxDIR &= ~WIFI_U_RTS_BIT;

        WIFI_U_CTS_PxSEL &= ~WIFI_U_CTS_BIT;
        WIFI_U_CTS_PxDIR |= WIFI_U_CTS_BIT;
    #endif
#endif

 	// Init Port for XBEE
#ifdef XBEE_ENABLE
	XBEE_RESET_PxSEL &= ~XBEE_RESET_BIT;
	XBEE_RESET_PxDIR |= XBEE_RESET_BIT;
	XBEE_RESET_PxOUT |= XBEE_RESET_BIT;

    XBEE_SLEEPRQ_PxSEL &= ~XBEE_SLEEPRQ_BIT;
    XBEE_SLEEPRQ_PxDIR |= XBEE_SLEEPRQ_BIT;
    XBEE_SLEEPRQ_PxOUT |= XBEE_SLEEPRQ_BIT;
#endif

 	// Init Port for XSTREAM
#ifdef XSTREAM_ENABLE
	XSTREAM_RESET_PxSEL &= ~XSTREAM_RESET_BIT;
	XSTREAM_RESET_PxDIR |= XSTREAM_RESET_BIT;
	XSTREAM_RESET_PxOUT |= XSTREAM_RESET_BIT;

	XSTREAM_ENABLE_PxSEL &= ~XSTREAM_ENABLE_BIT;
	XSTREAM_ENABLE_PxDIR |= XSTREAM_ENABLE_BIT;
	XSTREAM_ENABLE_PxOUT |= XSTREAM_ENABLE_BIT;
#endif

    // Init Port for RedPine WiFi
#ifdef WIFI_ENABLE
    WIFI_RESET_PxSEL &= ~WIFI_RESET_BIT;
    WIFI_RESET_PxDIR |= WIFI_RESET_BIT;
    WIFI_RESET_PxOUT |= WIFI_RESET_BIT;
#endif
    //ethernet and optical sensor power down
    P4DIR |= 0x0C;
    P4OUT &= 0xF3;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort1(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P1DIR |= num;
	else
		P1DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort1(BYTE num, BYTE state)
{
	if (state == SET)
		P1SEL |= num;
	else
		P1SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort1(BYTE num, BYTE state)
{
	if (state == SET)
		P1OUT |= num;
	else
		P1OUT &= ~num;
}

// Read Port : num = BIT0,
// Return value : SET or CLEAR
BYTE SYS_ReadPort1(BYTE num)
{
	if (P1IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort2(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P2DIR |= num;
	else
		P2DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort2(BYTE num, BYTE state)
{
	if (state == SET)
		P2SEL |= num;
	else
		P2SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort2(BYTE num, BYTE state)
{
	if (state == SET)
		P2OUT |= num;
	else
		P2OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort2(BYTE num)
{
	if (P2IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort3(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P3DIR |= num;
	else
		P3DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort3(BYTE num, BYTE state)
{
	if (state == SET)
		P3SEL |= num;
	else
		P3SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort3(BYTE num, BYTE state)
{
	if (state == SET)
		P3OUT |= num;
	else
		P3OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort3(BYTE num)
{
	if (P3IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort4(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P4DIR |= num;
	else
		P4DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort4(BYTE num, BYTE state)
{
	if (state == SET)
		P4SEL |= num;
	else
		P4SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort4(BYTE num, BYTE state)
{
	if (state == SET)
		P4OUT |= num;
	else
		P4OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort4(BYTE num)
{
	if (P4IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort5(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P5DIR |= num;
	else
		P5DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort5(BYTE num, BYTE state)
{
	if (state == SET)
		P5SEL |= num;
	else
		P5SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort5(BYTE num, BYTE state)
{
	if (state == SET)
		P5OUT |= num;
	else
		P5OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort5(BYTE num)
{
	if (P5IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort6(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P6DIR |= num;
	else
		P6DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort6(BYTE num, BYTE state)
{
	if (state == SET)
		P6SEL |= num;
	else
		P6SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort6(BYTE num, BYTE state)
{
	if (state == SET)
		P6OUT |= num;
	else
		P6OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort6(BYTE num)
{
	if (P6IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort7(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P7DIR |= num;
	else
		P7DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort7(BYTE num, BYTE state)
{
	if (state == SET)
		P7SEL |= num;
	else
		P7SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort7(BYTE num, BYTE state)
{
	if (state == SET)
		P7OUT |= num;
	else
		P7OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort7(BYTE num)
{
	if (P7IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort8(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P8DIR |= num;
	else
		P8DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort8(BYTE num, BYTE state)
{
	if (state == SET)
		P8SEL |= num;
	else
		P8SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort8(BYTE num, BYTE state)
{
	if (state == SET)
		P8OUT |= num;
	else
		P8OUT &= ~num;
}

// Read Port : num = BIT0
// Return value : SET or CLEAR
BYTE SYS_ReadPort8(BYTE num)
{
	if (P8IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort9(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P9DIR |= num;
	else
		P9DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort9(BYTE num, BYTE state)
{
	if (state == SET)
		P9SEL |= num;
	else
		P9SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort9(BYTE num, BYTE state)
{
	if (state == SET)
		P9OUT |= num;
	else
		P9OUT &= ~num;
}

// Read Port : num = BIT0,
// Return value : SET or CLEAR
BYTE SYS_ReadPort9(BYTE num)
{
	if (P9IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort10(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P10DIR |= num;
	else
		P10DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort10(BYTE num, BYTE state)
{
	if (state == SET)
		P10SEL |= num;
	else
		P10SEL &= ~num;
}

// Write Port : num = BIT0, state = SET
void SYS_WritePort10(BYTE num, BYTE state)
{
	if (state == SET)
		P10OUT |= num;
	else
		P10OUT &= ~num;
}

// Read Port : num = BIT0,
// Return value : SET or CLEAR
BYTE SYS_ReadPort10(BYTE num)
{
	if (P10IN & num)
		return SET;
	else
		return CLEAR;
}

// Set Port : num = BIT0, state = OUTPUT
void SYS_SetPort11(BYTE num, BYTE state)
{
	if (state == OUTPUT)
		P11DIR |= num;
	else
		P11DIR &= ~num;
}

// Select Port : num = BIT0, state = SET
void SYS_SelectPort11(BYTE num, BYTE state)
{
	if (state == SET)
		P11SEL |= num;
	else
		P11SEL &= ~num;
}


// Write Port : num = BIT0, state = SET
void SYS_WritePort11(BYTE num, BYTE state)
{
	if (state == SET)
		P11OUT |= num;
	else
		P11OUT &= ~num;
}

// Read Port : num = BIT0,
// Return value : SET or CLEAR
BYTE SYS_ReadPort11(BYTE num)
{
	if (P11IN & num)
		return SET;
	else
		return CLEAR;
}



