//******************************************************************************
//	 WWVB.H
//
//	 Programmed by HONG ROK KIM, CEST
//	 04/16/2009
//******************************************************************************
#include "define.h"

#define DOOMSDAY	0
#define ZERO	    0
#define ONE		    1

void WWVB_Initialize(void);
void WWVB_Process(void);
void WWVB_DispTime(void);
int WWVB_SampleInput(void);
int WWVB_FindStart(void);
int WWVB_ClassifyPulse(void);

