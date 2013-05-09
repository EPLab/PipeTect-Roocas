//******************************************************************************
//	 MEM.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 12/15/2009
//******************************************************************************
#include "define.h"

#define MEM_OPCODE_WRSR     0x01
#define MEM_OPCODE_WRITE    0x02
#define MEM_OPCODE_READ     0x03
#define MEM_OPCODE_WRDI     0x04
#define MEM_OPCODE_RDSR     0x05
#define MEM_OPCODE_WREN     0x06
#define MEM_OPCODE_FSTRD    0x0B
#define MEM_OPCODE_RDID     0x9F
#define MEM_OPCODE_SLEEP    0xB9
#define MEM_OPCODE_SNR      0xC3

#define MEM_DUMMY_DATA      0x00
#define MEM1_ADDR_START     0x00000
#define MEM1_ADDR_END       0x1FFFF
#define MEM2_ADDR_START     0x20000
#define MEM2_ADDR_END       0x3FFFF

void MEM_Test(void);
void MEM_Initialize(void);
void MEM_FastWriteBytes(DWORD addr, BYTE *data, BYTE num);
void MEM_FastReadBytes(DWORD addr, BYTE *data, BYTE num);
void MEM_WriteByte(DWORD addr, BYTE data);
void MEM_ReadByte(DWORD addr, BYTE *data);
void MEM_WriteCommand(DWORD addr, BYTE command);
void MEM_ReadStatus(DWORD addr, BYTE *status);
void MEM_WriteEnable(DWORD addr);
void MEM_WriteDisable(DWORD addr);
void MEM_EnableCS(DWORD addr);
void MEM_DisableCS(DWORD addr);

