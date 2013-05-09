//******************************************************************************
//	 SYS_ADC.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 08/21/2009
//******************************************************************************
#include "sys_adc.h"

extern short g_AdcData[];

void SYS_InitADC(BYTE Ch, BYTE Mode)
{
    BYTE i;
    
    P6SEL |= Ch;   // Enable A/D channel A0

    if (Mode == SYS_ADC_REF) 
    {
        // Turn on ADC12, Sampling time On Reference Generator and set to 2.5V
        ADC12CTL0 = ADC12ON + ADC12SHT02 + ADC12REFON + ADC12REF2_5V;
        ADC12CTL1 = ADC12SHP;               // Use sampling timer
        ADC12MCTL0 = ADC12SREF_1;           // Vr+=Vref+ and Vr-=AVss
        for ( i=0; i<0x30; i++);            // Delay for reference start-up
        ADC12CTL0 |= ADC12ENC;              // Enable conversions
    }
    else if (Mode == SYS_ADC_AREF)
    {    
        ADC12CTL0 = ADC12ON + ADC12SHT0_2;  // Turn on ADC12, set sampling time
        ADC12CTL1 = ADC12SHP;               // Use sampling timer
        ADC12MCTL0 = ADC12SREF_2;           // Vr+ = VeREF+ (ext) and Vr-=AVss
        ADC12CTL0 |= ADC12ENC;              // Enable conversions
    }
    else 
    {
        // Turn on ADC12, extend sampling time to avoid overflow of results
        ADC12CTL0 = ADC12ON + ADC12MSC + ADC12SHT0_8; 
        ADC12CTL1 = ADC12SHP + ADC12CONSEQ_3; // Use sampling timer, repeated sequence
        ADC12MCTL0 = ADC12INCH_0;           // ref+=AVcc, channel = A0
        ADC12MCTL1 = ADC12INCH_1;           // ref+=AVcc, channel = A1
        ADC12MCTL2 = ADC12INCH_2;           // ref+=AVcc, channel = A2
        ADC12MCTL3 = ADC12INCH_3 + ADC12EOS;  // ref+=AVcc, channel = A3, end seq.
        ADC12IE = 0x08;                     // Enable ADC12IFG.3
        ADC12CTL0 |= ADC12ENC;              // Enable conversions
        ADC12CTL0 |= ADC12SC;               // Start convn - software trigger
    }
}

void SYS_ReadADC(BYTE Ch, BYTE Mode, short *Data)
{
    BYTE i;
    BYTE *reg_addr;

    for (i=0; i<MAX_ADC_CH; i++) 
    {
        if (Ch == (BIT0 >> i)) break;
    }
    
    if (Mode == SYS_ADC_POLL)
    {
        ADC12CTL0 |= ADC12SC;                   // Start conversion-software trigger
        while (!(ADC12IFG & Ch));
        reg_addr = (BYTE *)(&ADC_REG_BASE) + i*2;
        *Data = *reg_addr;
    }
    else
    {
        *Data = g_AdcData[i];
    }
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
    switch(__even_in_range(ADC12IV,34))
    {
        case  0: break;                           // Vector  0:  No interrupt
        case  2: break;                           // Vector  2:  ADC overflow
        case  4: break;                           // Vector  4:  ADC timing overflow
        case  6: break;                           // Vector  6:  ADC12IFG0
        case  8: break;                           // Vector  8:  ADC12IFG1
        case 10: break;                           // Vector 10:  ADC12IFG2
        case 12:                                  // Vector 12:  ADC12IFG3
            g_AdcData[0] = ADC12MEM0;       // Move A0 results, IFG is cleared
            g_AdcData[1] = ADC12MEM1;       // Move A1 results, IFG is cleared
            g_AdcData[2] = ADC12MEM2;       // Move A2 results, IFG is cleared
            g_AdcData[3] = ADC12MEM3;       // Move A3 results, IFG is cleared
            break;
        case 14: break;                           // Vector 14:  ADC12IFG4
        case 16: break;                           // Vector 16:  ADC12IFG5
        case 18: break;                           // Vector 18:  ADC12IFG6
        case 20: break;                           // Vector 20:  ADC12IFG7
        case 22: break;                           // Vector 22:  ADC12IFG8
        case 24: break;                           // Vector 24:  ADC12IFG9
        case 26: break;                           // Vector 26:  ADC12IFG10
        case 28: break;                           // Vector 28:  ADC12IFG11
        case 30: break;                           // Vector 30:  ADC12IFG12
        case 32: break;                           // Vector 32:  ADC12IFG13
        case 34: break;                           // Vector 34:  ADC12IFG14
        default: break; 
    }  
}
