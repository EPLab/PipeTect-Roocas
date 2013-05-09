#ifdef PROBE_ENABLED

#include "probe.h"
#include "msp430x54x.h"

void probe_setup(void)
{
	P2DIR |= 0x30;
}

void probe1_change(unsigned char hl)
{
	if (hl)
	{
		P2OUT |= 0x10;
	}
	else
	{
		P2OUT &= 0xEF;
	}
}

void probe2_change(unsigned char hl)
{
	if (hl)
	{
		P2OUT |= 0x20;
	}
	else
	{
		P2OUT &= 0xDF;
	}
}

void probe_high(void)
{
	//fram_done = 1;
	probe1_change(1);
}

void probe_low(void)
{
	//fram_done = 1;
	probe1_change(0);
}

#endif
