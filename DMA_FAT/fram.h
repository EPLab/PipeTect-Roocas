//******************************************************************************
//	 FRAM.H
//
//	 Programmed by EUNBAE YOON, UCT
//	 02/28/2011
//******************************************************************************

#ifndef __FRAM_H
#define __FRAM_H

#define FRAM_OPCODE_WRSR     0x01
#define FRAM_OPCODE_WRITE    0x02
#define FRAM_OPCODE_READ     0x03
#define FRAM_OPCODE_WRDI     0x04
#define FRAM_OPCODE_RDSR     0x05
#define FRAM_OPCODE_WREN     0x06
#define FRAM_OPCODE_FSTRD    0x0B
#define FRAM_OPCODE_RDID     0x9F
#define FRAM_OPCODE_SLEEP    0xB9
#define FRAM_OPCODE_SNR      0xC3

#define FRAM_TOTAL_SIZE      0x80000                            //128KB x 2
#define FRAM_DUMMY_DATA      0x00
#define FRAM1_ADDR_START     0x00000
#define FRAM1_ADDR_END       (FRAM_TOTAL_SIZE >> 1) - 1         //0x1FFFF
#define FRAM2_ADDR_START     (FRAM_TOTAL_SIZE >> 1)             //0x20000
#define FRAM2_ADDR_END       FRAM_TOTAL_SIZE - 1                //0x3FFFF
#define FRAM_INDIVIDUAL_SIZE (FRAM_TOTAL_SIZE >> 1)

#define FRAM_DATA_RESIDUAL		0x04
#define FRAM_DATA_FRAGMENTED	0x08
#define FRAM_DATA_TO_LOAD		0x10
#define FRAM_ADDR_TO_LOAD		0x20
#define FRAM2_SEL_BIT			0x40
#define FRAM_WRITE_BIT			0x80

extern void FRAM_Initialize(unsigned char op);
extern void FRAM_EnableCS(unsigned long addr);
extern void FRAM_DisableCS(unsigned long addr);
extern unsigned int FRAM_Write(unsigned long addr, unsigned char* data, unsigned int length, void (*WB_func)(void));
extern unsigned int FRAM_Read(unsigned long addr, unsigned char* data, unsigned int length, void (*RB_func)(void));
extern unsigned int FRAM_Register_WB_proc(void (*func)(void));
extern unsigned int FRAM_Register_RB_proc(void (*func)(void));
extern unsigned char FRAM_get_state(void);
extern unsigned char FRAM_get_status(unsigned long addr);

extern void MEM_WriteByte(unsigned long addr, unsigned char* data, int length);
extern void MEM_ReadByte(unsigned long addr, unsigned char* data, int length);

#endif
