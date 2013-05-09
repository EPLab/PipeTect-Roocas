//******************************************************************************
//	 FRAM.C
//
//	 Programmed by HONG ROK KIM, CEST
//	 02/28/2011
//******************************************************************************

#ifdef FRAM_ENABLED
#include "fram.h"
#include "msp430x54x.h"
#include "dma_spi.h"

// FRAM port definitions 
#define FRAM1_CS_PxSEL         	P4SEL      
#define FRAM1_CS_PxDIR         	P4DIR      
#define FRAM1_CS_PxOUT         	P4OUT      
#define FRAM1_CS_BIT           	BIT0

#define FRAM1_HOLD_PxSEL        P4SEL      
#define FRAM1_HOLD_PxDIR        P4DIR      
#define FRAM1_HOLD_PxOUT        P4OUT       
#define FRAM1_HOLD_BIT          BIT4

#define FRAM2_CS_PxSEL         	P4SEL      
#define FRAM2_CS_PxDIR         	P4DIR      
#define FRAM2_CS_PxOUT         	P4OUT      
#define FRAM2_CS_BIT           	BIT1

#define FRAM2_HOLD_PxSEL        P4SEL      
#define FRAM2_HOLD_PxDIR        P4DIR      
#define FRAM2_HOLD_PxOUT        P4OUT       
#define FRAM2_HOLD_BIT          BIT2

#define FRAM_INIT_SIGNATURE_ADDR_1 0x0003FFFE
#define FRAM_INIT_SIGNATURE_ADDR_2 0x0003FFFF

static unsigned char FRAM_current_state = 0;
static unsigned char FRAM_current_address[5];
static unsigned long FRAM_current_addressL;
static unsigned long FRAM_old_addressL;
static unsigned char* FRAM_current_data = 0;
static unsigned int FRAM_current_data_length = 0;
static unsigned char FRAM_DMA_wait = 0;

static void (*FRAM_WB_func)(void) = 0;
static void (*FRAM_RB_func)(void) = 0;


#pragma inline 
void FRAM_WriteEnable(unsigned long addr)
{
    FRAM_EnableCS(addr);
    N_DMA_WriteByteSPI(0x06);
	FRAM_DisableCS(addr);
}

void FRAM_Initialize(unsigned char op)
{
	unsigned char temp[2] = {0};
    //unsigned char temp2[2] = {0};
	//unsigned char data[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    unsigned char data[16] = {0};
	unsigned long i;
	
	FRAM1_CS_PxSEL &= ~FRAM1_CS_BIT;
	FRAM1_CS_PxDIR |= FRAM1_CS_BIT;
	FRAM1_CS_PxOUT |= FRAM1_CS_BIT;
	FRAM2_CS_PxSEL &= ~FRAM2_CS_BIT;
	FRAM2_CS_PxDIR |= FRAM2_CS_BIT;
	FRAM2_CS_PxOUT |= FRAM2_CS_BIT;
	
	FRAM_current_state = 0;
    
#ifndef DMA_SPI_INITIALIZED
#define DMA_SPI_INITIALIZED
    init_DMA_SPI(MMC_DUMMY_DATA);
#endif
    //temp[0] = FRAM_get_status(FRAM1_ADDR_START);
    //temp[1] = FRAM_get_status(FRAM2_ADDR_START);
	
    if (op == 0)
    {
    	MEM_ReadByte(FRAM_INIT_SIGNATURE_ADDR_1, temp, 2);
    }
    if ((temp[0] != 0xAA) || (temp[1] != 0x55))
    {
        i = 0;
        while (i < FRAM_TOTAL_SIZE)
        {
            MEM_WriteByte(i, data, sizeof(data));
            //MEM_ReadByte(i, data2, sizeof(data2));
            if (i > 0x20000)
            {
                __no_operation();
            }
            i += 16;
        }
        temp[0] = 0xAA;
        temp[1] = 0x55;
        MEM_WriteByte(FRAM_INIT_SIGNATURE_ADDR_1, temp, 2);
    }
}

void FRAM_EnableCS(unsigned long addr)
{
    if (addr <= FRAM1_ADDR_END)
	{
        FRAM1_CS_PxOUT &= ~FRAM1_CS_BIT;
	}
    else
	{
        FRAM2_CS_PxOUT &= ~FRAM2_CS_BIT;
	}
}

void FRAM_DisableCS(unsigned long addr)
{
    if (addr <= FRAM1_ADDR_END)
	{
        FRAM1_CS_PxOUT |= FRAM1_CS_BIT;
	}
    else
	{
        FRAM2_CS_PxOUT |= FRAM2_CS_BIT;
	}
}

void MEM_WriteByte(unsigned long addr, unsigned char* data, int length)
{
    unsigned long address;
    //unsigned char temp = 0;
    int i;
	
    FRAM_WriteEnable(addr);

    //temp = FRAM_get_status(addr);
    
    FRAM_EnableCS(addr);

    if (addr <= FRAM1_ADDR_END)
	{
    	address = addr;
	}
    else
	{
    	address = addr - FRAM2_ADDR_START;
    }
    N_DMA_WriteByteSPI(0x02);
    N_DMA_WriteByteSPI((unsigned char)(address >> 16));
    N_DMA_WriteByteSPI((unsigned char)(address >> 8));
    N_DMA_WriteByteSPI((unsigned char)addr);
	for (i = 0; i < length; ++i)
	{
    	N_DMA_WriteByteSPI(data[i]);
	}
	
    FRAM_DisableCS(addr);
}

void MEM_ReadByte(unsigned long addr, unsigned char* data, int length)
{
    unsigned long address;
	int i;
	
    FRAM_EnableCS(addr);

    if (addr <= FRAM1_ADDR_END)
      address = addr;
    else
      address = addr - FRAM2_ADDR_START;

    N_DMA_WriteByteSPI(0x03);
    N_DMA_WriteByteSPI((unsigned char)(address >> 16));
    N_DMA_WriteByteSPI((unsigned char)(address >> 8));
    N_DMA_WriteByteSPI((unsigned char) address);
	for (i = 0; i < length; ++i)
	{
    	N_DMA_WriteByteSPI(0x00);
    	N_DMA_ReadByteSPI(&data[i]);
	}
	
    FRAM_DisableCS(addr);
}

static void FRAM_DMA_done(void)
{
    FRAM_DMA_wait = 0;
}

unsigned char FRAM_get_status(unsigned long addr)
{
	unsigned char res = 0x05;
	FRAM_EnableCS(addr);
    FRAM_DMA_wait = 1;
    DMA_SPI_Write(&res, 1, &FRAM_DMA_done);
    while (FRAM_DMA_wait);
    FRAM_DMA_wait = 1;
    DMA_SPI_Read(&res, 1, &FRAM_DMA_done);
	//N_DMA_WriteByteSPI(0x05);
	//N_DMA_WriteByteSPI(0x00);
    //N_DMA_WriteByteSPI(0x00);
    while (FRAM_DMA_wait);
    FRAM_DMA_wait = 1;
    DMA_SPI_Read(&res, 1, &FRAM_DMA_done);
	//N_DMA_WriteByteSPI(0x05);
	//N_DMA_WriteByteSPI(0x00);
    //N_DMA_WriteByteSPI(0x00);
    while (FRAM_DMA_wait);
	FRAM_DisableCS(addr);
	return res;
}

void FRAM_DMA_SPI_CB(void)
{
	unsigned long tempLength;
	unsigned char* tempData;
	
	// back up the address for next operation
	FRAM_old_addressL = FRAM_current_addressL;
	
	if (FRAM_current_state & FRAM_ADDR_TO_LOAD)
	{
		FRAM_current_state &= ~FRAM_ADDR_TO_LOAD;
		// check to see if it is a data transfer
		if (FRAM_current_state & FRAM_DATA_TO_LOAD)
		{
			tempData = FRAM_current_data;
			// to read/write the data spead from FRAM1 to FRAM2
			if (FRAM_current_state & FRAM_DATA_FRAGMENTED)
			{
				// clear flag
				FRAM_current_state &= ~FRAM_DATA_FRAGMENTED;
				// one more operation for resisual data
				FRAM_current_state |= FRAM_DATA_RESIDUAL;
				// length of data retrieved or stored in/from FRAM1
				tempLength = FRAM2_ADDR_START - FRAM_current_addressL;
				// new data length for residual data
				FRAM_current_data_length -= tempLength;
				// new address for next opeation should be the start address of FRAM2
				FRAM_current_addressL = FRAM2_ADDR_START;
				// new data src/dst for the residual data
				FRAM_current_data = &FRAM_current_data[tempLength];
			}
			// data purely resides in one FRAM
			else
			{
				tempLength = FRAM_current_data_length;
			}
			if (FRAM_current_state & FRAM_WRITE_BIT)
			{
				DMA_SPI_Write(tempData, tempLength, &FRAM_DMA_SPI_CB);
			}
			else
			{
				DMA_SPI_Read(tempData, tempLength, &FRAM_DMA_SPI_CB);
			}
			return;
		}
		// Neither write or read, possibly register check or special commands
		FRAM_DisableCS(FRAM_current_addressL);
		if (FRAM_current_state & FRAM_WRITE_BIT)
		{
			FRAM_current_state = 0;
			//FRAM_current_state &= ~FRAM_WRITE_BIT;
			if (FRAM_WB_func)
			{
				FRAM_WB_func();
				FRAM_WB_func = 0;
			}
		}
		else
		{
			FRAM_current_state = 0;
			if (FRAM_RB_func)
			{
				FRAM_RB_func();
				FRAM_RB_func = 0;
			}
		}
		return;
	}
	if (FRAM_current_state & FRAM_DATA_TO_LOAD)
	{
		if (FRAM_current_state & FRAM_DATA_RESIDUAL)
		{
			// close the transaction of old operation (FRAM1)
			FRAM_DisableCS(FRAM_old_addressL);
			// clear flag
			FRAM_current_state &= ~FRAM_DATA_RESIDUAL;
			// new address for residual data has to be set up
			FRAM_current_state |= FRAM_ADDR_TO_LOAD;
			FRAM_old_addressL = FRAM_current_addressL;
			FRAM_current_addressL -= FRAM_INDIVIDUAL_SIZE;
			FRAM_current_address[1] = (unsigned char)(FRAM_current_addressL >> 16);
			FRAM_current_address[2] = (unsigned char)(FRAM_current_addressL >> 8);
			FRAM_current_address[3] = (unsigned char)FRAM_current_addressL;
			// start of the transaction of residual data
			// opcode and address first
			if (FRAM_current_state & FRAM_WRITE_BIT)
			{
				FRAM_WriteEnable(FRAM_old_addressL);
				FRAM_EnableCS(FRAM_old_addressL);
				FRAM_current_address[0] = FRAM_OPCODE_WRITE;
				DMA_SPI_Write(FRAM_current_address, 4, &FRAM_DMA_SPI_CB);
			}
			else
			{
				FRAM_EnableCS(FRAM_old_addressL);
				FRAM_current_address[0] = FRAM_OPCODE_READ;
				FRAM_current_address[4] = 0;
				DMA_SPI_Write(FRAM_current_address, 5, &FRAM_DMA_SPI_CB);
			}
			return;
		}
		// data transfer ends here
		FRAM_DisableCS(FRAM_old_addressL);
		if (FRAM_current_state & FRAM_WRITE_BIT)
		{
			//FRAM_current_state &= ~FRAM_WRITE_BIT;
			FRAM_current_state = 0;
			if (FRAM_WB_func)
			{
				FRAM_WB_func();
				FRAM_WB_func = 0;
			}
		}
		else
		{
			FRAM_current_state = 0;
			if (FRAM_RB_func)
			{
				FRAM_RB_func();
				FRAM_RB_func = 0;
			}
		}
	}
}

unsigned int FRAM_Write(unsigned long addr, unsigned char* data, unsigned int length, void (*WB_func)(void))
{
	if ((FRAM_current_state == 0) && (DMA_SPI_IsBeingUsed() == 0))
	{
		FRAM_current_data = data;
		FRAM_current_data_length = length;
		FRAM_WB_func = WB_func;
		FRAM_current_addressL = addr;
		// if the address is in the range of FRAM1 < 0x1FFFF
		if (addr < FRAM2_ADDR_START)
		{
			if ((addr + (unsigned long)length) > FRAM1_ADDR_END)
			{
				FRAM_current_state = FRAM_ADDR_TO_LOAD | FRAM_DATA_TO_LOAD | FRAM_DATA_FRAGMENTED;
			}
			else
			{
				FRAM_current_state = FRAM_ADDR_TO_LOAD | FRAM_DATA_TO_LOAD;
			}
		}
		else
		{
			if ((addr + (unsigned long)length) > FRAM2_ADDR_END)
			{
				return 0;	// can't be stored in the FRAM's
			}
			FRAM_current_state = FRAM2_SEL_BIT | FRAM_ADDR_TO_LOAD | FRAM_DATA_TO_LOAD;
			addr -= FRAM2_ADDR_START;
		}
		FRAM_current_address[0] = FRAM_OPCODE_WRITE;
		FRAM_current_address[1] = (unsigned char)(addr >> 16);
		FRAM_current_address[2] = (unsigned char)(addr >> 8);
		FRAM_current_address[3] = (unsigned char)addr;
		FRAM_current_state |= FRAM_WRITE_BIT;
		FRAM_WriteEnable(FRAM_current_addressL);
		FRAM_EnableCS(FRAM_current_addressL);
		return DMA_SPI_Write(FRAM_current_address, 4, &FRAM_DMA_SPI_CB);
	}
	return 0;
}

unsigned int FRAM_Read(unsigned long addr, unsigned char* data, unsigned int length, void (*RB_func)(void))
{
	if ((FRAM_current_state == 0) && (DMA_SPI_IsBeingUsed() == 0))
	{
		FRAM_current_data = data;
		FRAM_current_data_length = length;
		FRAM_RB_func = RB_func;
		FRAM_current_addressL = addr;
		// if the address is in the range of FRAM1 < 0x1FFFF
		if (addr < FRAM2_ADDR_START)
		{
			if ((addr + (unsigned long)length) > FRAM2_ADDR_START)
			{
				FRAM_current_state = FRAM_ADDR_TO_LOAD | FRAM_DATA_TO_LOAD | FRAM_DATA_FRAGMENTED;
			}
			else
			{
				FRAM_current_state = FRAM_ADDR_TO_LOAD | FRAM_DATA_TO_LOAD;
			}
		}
		else
		{
			if ((addr + (unsigned long)length) > FRAM2_ADDR_END)
			{
				return 0;	// can't be stored in the FRAM's
			}
			FRAM_current_state = FRAM2_SEL_BIT | FRAM_ADDR_TO_LOAD | FRAM_DATA_TO_LOAD;
			addr -= FRAM2_ADDR_START;
		}
		FRAM_current_address[0] = FRAM_OPCODE_READ;
		FRAM_current_address[1] = (unsigned char)(addr >> 16);
		FRAM_current_address[2] = (unsigned char)(addr >> 8);
		FRAM_current_address[3] = (unsigned char)addr;
		FRAM_current_address[4] = 0;
		FRAM_EnableCS(FRAM_current_addressL);
		return DMA_SPI_Write(FRAM_current_address, 5, &FRAM_DMA_SPI_CB);
	}
	return 0;
}

unsigned char FRAM_get_state(void)
{
	return FRAM_current_state;
}

#endif
