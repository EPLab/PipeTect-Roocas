//******************************************************************************
//	 WWVB.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 10/16/2009
//******************************************************************************
#include "define.h"
#include "wwvb.h"
#include "sys.h"
#include "xstream.h"

int WWVB_Minutes;
int WWVB_Hours;
int WWVB_Seconds;

void WWVB_DispTime()
{
    char str[30];
  
    sprintf(str, "WWVB(GMT) : %d(h) %d(m)\r\n" , WWVB_Hours, WWVB_Minutes); 
    XSTREAM_WriteFrame(str, 25);
}

int WWVB_SampleInput()
{
// Sample the code input pin every 5 mS, for 5 samples then
// return voted average to cancel noise.
    int avg = 0;
    int i;

	for (i=0; i<=4; i++)
	{
		if (WWVB_TDATA_PxIN & WWVB_TDATA_BIT)
			avg++;
			
        SYS_DelayMS(5);
	}
	
	// Return average of digital input
	if (avg >= 3)
		return(ONE);  // The code was a one
	else
		return(ZERO); // The code was a zero
}

int WWVB_FindStart()
{
// Look for a start pulse

	// --- Loop till negative edge ---
	while (WWVB_SampleInput() == 0); // Wait till input = 1
	while (WWVB_SampleInput() == 1);  // Wait till input = 0	

	// Now I have found the falling edge, See if it was a start pulse
    SYS_DelayMS(650);
	
	// Check if input is still low, indicating a start pulse
	if (WWVB_SampleInput() == 0)	
		return(TRUE); // Input is still low, then it is a start
	else
		return(FALSE); // Input was not low long enough	
}

int WWVB_ClassifyPulse()
{
// Classify whether pulse is a one or zero

	// --- Loop till negative edge ---
	while(WWVB_SampleInput() == 0); // Wait till input = 1
	while(WWVB_SampleInput() == 1);  // Wait till input = 0	

	// Now I have found the falling edge, See what it is
    SYS_DelayMS(350);
	
	// Check if input is still low, indicating a one
	if (WWVB_SampleInput() == 0)	
		return(ONE); // Input is still low, it is a one
	else
		return(ZERO); // Input was not low long enough	
}
	
void WWVB_Initialize(void)
{
    WWVB_ENABLE_PxDIR |= WWVB_ENABLE_BIT;
    WWVB_ENABLE_PxOUT &= (~WWVB_ENABLE_BIT);

    WWVB_HOLD_PxDIR |= WWVB_HOLD_BIT;
    WWVB_HOLD_PxOUT |= WWVB_HOLD_BIT;

    WWVB_TDATA_PxDIR &= (~WWVB_TDATA_BIT);
}

void WWVB_Process()
{
    // Initialize WWVB port
    WWVB_Initialize();

	//----- Start of main loop -----			
	while (1)
	{
		// Find the double pulse start sequence 
		if (WWVB_FindStart())
		{
			if (WWVB_FindStart())
			{
				// If I got here then we found the start sequence.
				
				// Decode minutes and hours information for display

				// Minutes first! 
				WWVB_Minutes = 0;
	
				if (WWVB_ClassifyPulse()) WWVB_Minutes = 40;
				if (WWVB_ClassifyPulse()) WWVB_Minutes += 20;
				if (WWVB_ClassifyPulse()) WWVB_Minutes += 10;
	
				// Next comes an uncoded bit
				WWVB_ClassifyPulse();	

				if (WWVB_ClassifyPulse()) WWVB_Minutes += 8;
				if (WWVB_ClassifyPulse()) WWVB_Minutes += 4;
				if (WWVB_ClassifyPulse()) WWVB_Minutes += 2;
				if (WWVB_ClassifyPulse()) WWVB_Minutes += 1;
		
				// Next comes a sync pulse, then two uncoded
				WWVB_ClassifyPulse();
				WWVB_ClassifyPulse();
				WWVB_ClassifyPulse();

				// Now hours 
				WWVB_Hours = 0;
	
				if (WWVB_ClassifyPulse()) WWVB_Hours = 20;
				if (WWVB_ClassifyPulse()) WWVB_Hours += 10;

				// Next comes an uncoded bit
				WWVB_ClassifyPulse();
				if (WWVB_ClassifyPulse()) WWVB_Hours += 8;
				if (WWVB_ClassifyPulse()) WWVB_Hours += 4;
				if (WWVB_ClassifyPulse()) WWVB_Hours += 2;
				if (WWVB_ClassifyPulse()) WWVB_Hours += 1;
				
				// Update display with info
				WWVB_DispTime();
			
			} // End of 2nd 'if start pulse'
			
		} // End of 1st 'if start pulse'

	} // end of main while loop (forever loop)
}

