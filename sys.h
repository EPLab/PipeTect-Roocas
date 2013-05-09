//******************************************************************************
//	 SYS.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 02/28/2009
//******************************************************************************
#include "define.h"
#include "intrinsics.h"

#define SYS_CLK1	32768
//#define SYS_CLK2	16000000
#define SYS_CLK2        14745600
#define SYS_CLK		SYS_CLK2 

#define SYS_LOW_CLK	0
#define SYS_HIGH_CLK	1

void SYS_Initialize(void);
void SYS_InitClock(BYTE bMode);
void SYS_DelayMS(WORD ms);

void SYS_InitPort(void);
void SYS_SetPort1(BYTE num, BYTE state);
void SYS_SetPort2(BYTE num, BYTE state);
void SYS_SetPort3(BYTE num, BYTE state);
void SYS_SetPort4(BYTE num, BYTE state);
void SYS_SetPort5(BYTE num, BYTE state);
void SYS_SetPort6(BYTE num, BYTE state);
void SYS_SetPort7(BYTE num, BYTE state);
void SYS_SetPort8(BYTE num, BYTE state);
void SYS_SetPort9(BYTE num, BYTE state);
void SYS_SetPort10(BYTE num, BYTE state);
void SYS_SetPort11(BYTE num, BYTE state);

void SYS_SelectPort1(BYTE num, BYTE state);
void SYS_SelectPort2(BYTE num, BYTE state);
void SYS_SelectPort3(BYTE num, BYTE state);
void SYS_SelectPort4(BYTE num, BYTE state);
void SYS_SelectPort5(BYTE num, BYTE state);
void SYS_SelectPort6(BYTE num, BYTE state);
void SYS_SelectPort7(BYTE num, BYTE state);
void SYS_SelectPort8(BYTE num, BYTE state);
void SYS_SelectPort9(BYTE num, BYTE state);
void SYS_SelectPort10(BYTE num, BYTE state);
void SYS_SelectPort11(BYTE num, BYTE state);

void SYS_WritePort1(BYTE num, BYTE state);
void SYS_WritePort2(BYTE num, BYTE state);
void SYS_WritePort3(BYTE num, BYTE state);
void SYS_WritePort4(BYTE num, BYTE state);
void SYS_WritePort5(BYTE num, BYTE state);
void SYS_WritePort6(BYTE num, BYTE state);
void SYS_WritePort7(BYTE num, BYTE state);
void SYS_WritePort8(BYTE num, BYTE state);
void SYS_WritePort9(BYTE num, BYTE state);
void SYS_WritePort10(BYTE num, BYTE state);
void SYS_WritePort11(BYTE num, BYTE state);

BYTE SYS_ReadPort1(BYTE num);
BYTE SYS_ReadPort2(BYTE num);
BYTE SYS_ReadPort3(BYTE num);
BYTE SYS_ReadPort4(BYTE num);
BYTE SYS_ReadPort5(BYTE num);
BYTE SYS_ReadPort6(BYTE num);
BYTE SYS_ReadPort7(BYTE num);
BYTE SYS_ReadPort8(BYTE num);
BYTE SYS_ReadPort9(BYTE num);
BYTE SYS_ReadPort10(BYTE num);
BYTE SYS_ReadPort11(BYTE num);
