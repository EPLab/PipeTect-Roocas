//******************************************************************************
//	 MEM.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 12/15/2009
//******************************************************************************
#include "mem.h"
#include "sys_spi.h"

void MEM_Test(void)
{
    DWORD i=0;
    BYTE data1, data2;

    for (i=0; i<=MEM1_ADDR_END; i++)
    {
         MEM_WriteByte(i, (BYTE)i);
         MEM_ReadByte(i, &data1);
                  
         MEM_WriteByte(i+MEM2_ADDR_START, (BYTE)i);
         MEM_ReadByte(i+MEM2_ADDR_START, &data2);
         if (data1 != data2) 
             break;
    }
}

void MEM_Initialize(void)
{

}

void MEM_WriteFrame(DWORD addr, BYTE *data, BYTE num)
{
    BYTE i;
    DWORD address;
    
    MEM_WriteEnable(addr);

    MEM_EnableCS(addr);

    if (addr <= MEM1_ADDR_END)
      address = addr;
    else
      address = addr - MEM2_ADDR_START;

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_WRITE);
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 16));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 8));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)address);
    
    for (i=0; i<num; i++) 
    { 
        SYS_WriteByteSPI(FRAM_SPI_MODE, *(data++));
    }

    MEM_DisableCS(addr);
}

void MEM_ReadFrame(DWORD addr, BYTE *data, BYTE num)
{
    int i;
    DWORD address;
    
    MEM_EnableCS(addr);

    if (addr <= MEM1_ADDR_END)
      address = addr;
    else
      address = addr - MEM2_ADDR_START;

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_FSTRD);
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 16));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 8));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)address);
    
    for (i=0; i<num; i++) 
    { 
        SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_DUMMY_DATA);
        SYS_ReadByteSPI(FRAM_SPI_MODE, data++);
    }

    MEM_DisableCS(addr);
}

void MEM_WriteByte(DWORD addr, BYTE data)
{
    DWORD address;
    
    MEM_WriteEnable(addr);

    MEM_EnableCS(addr);

    if (addr <= MEM1_ADDR_END)
      address = addr;
    else
      address = addr - MEM2_ADDR_START;
    
    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_WRITE);
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 16));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 8));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)addr);
    SYS_WriteByteSPI(FRAM_SPI_MODE, data);

    MEM_DisableCS(addr);
}

void MEM_ReadByte(DWORD addr, BYTE *data)
{
    DWORD address;

    MEM_EnableCS(addr);

    if (addr <= MEM1_ADDR_END)
      address = addr;
    else
      address = addr - MEM2_ADDR_START;

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_READ);
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 16));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)(address >> 8));
    SYS_WriteByteSPI(FRAM_SPI_MODE, (BYTE)address);
    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_DUMMY_DATA);
    SYS_ReadByteSPI(FRAM_SPI_MODE, data);

    MEM_DisableCS(addr);
}

void MEM_WriteCommand(DWORD addr, BYTE command)
{
    MEM_WriteEnable(addr);

    MEM_EnableCS(addr);

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_WRSR);
    SYS_WriteByteSPI(FRAM_SPI_MODE, command);

    MEM_DisableCS(addr);
}

void MEM_ReadStatus(DWORD addr, BYTE *status)
{
    MEM_EnableCS(addr);

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_RDSR);
    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_DUMMY_DATA);
    SYS_ReadByteSPI(FRAM_SPI_MODE, status);

    MEM_DisableCS(addr);
}

void MEM_WriteEnable(DWORD addr)
{
    MEM_EnableCS(addr);

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_WREN);

    MEM_DisableCS(addr);
}

void MEM_WriteDisable(DWORD addr)
{
    MEM_EnableCS(addr);

    SYS_WriteByteSPI(FRAM_SPI_MODE, MEM_OPCODE_WRDI);

    MEM_DisableCS(addr);
}

void MEM_EnableCS(DWORD addr)
{
    if (addr <= MEM1_ADDR_END)
        FRAM1_CS_PxOUT &= ~FRAM1_CS_BIT;
    else
        FRAM2_CS_PxOUT &= ~FRAM2_CS_BIT;
}

void MEM_DisableCS(DWORD addr)
{
    if (addr <= MEM1_ADDR_END)
        FRAM1_CS_PxOUT |= FRAM1_CS_BIT;
    else
        FRAM2_CS_PxOUT |= FRAM2_CS_BIT;
}




