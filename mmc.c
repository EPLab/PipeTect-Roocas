/**
 *	HALayer.c:	implementation of the HALayer.
 *	class HALayer:	defines the hardware specific portions
 *					of the MMC/SD card FAT library.  This file
 *					needs to be adapted for the specific microcontroller
 *					of choice
 *
 *	Author:	Ivan Sham
 *	Date: June 26, 2004
 *	Version: 2.0
 *	Note: Developed for William Hue and Pete Rizun
 *
 *****************************************************************
 *  Change Log
 *----------------------------------------------------------------
 *  Date    |   Author          | Reason for change
 *----------------------------------------------------------------
 * Aug31/04     William Hue       Removed convertCharToLong().
 *                                Changed char types to
 *                                unsigned char.
 *
 * Jul18/04     Alex Jiang        Added void parameter to
 *                                functions with no parameters.
 *                                Made delay a static function.
 *
 * Jan03/05     William Hue       Clean-up for
 *                                Circuit Cellar article.
 **/

#include "mmc.h"
#include "sys_spi.h"

extern unsigned long sectorZero;
unsigned char readBuf[SECTOR_SIZE];

/**
 *	case insensitive compare of the 2 inputs
 *	returns true if and only if c1 = c2 or if
 *	c1 and c2 are the same character with different
 *	capitialization
 *
 *	@param	c1			input 1
 *	@param	c2			input 2
 **/
boolean MMC_CharEquals(const char c1, const char c2)
{
	if((((c1 >= 'A')&&(c1 <= 'Z'))||((c1 >= 'a')&&(c1 <= 'z')))&&
		(((c2 >= 'A')&&(c2 <= 'Z'))||((c2 >= 'a')&&(c2 <= 'z'))))
	{
		if((c1 | 0x20) == (c2 | 0x20))
		{
			return TRUE;
		}
	}
	else
	{
		if(c1 == c2)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 *	checks if there is a memory card in the slot
 *	pulls 
 *
 *	@return	FALSE		card not detected
 *	@return TRUE		car detected
 **/
boolean MMC_DetectCard(void)
{
/*
    MMC_EnableCS();
    MMC_Delay(18);
    
    if (!(MMC_CD_PxIN & MMC_CD_BIT))
    {  
        MMC_DisableCS();
        return TRUE;
    }
    else 
    {
        MMC_DisableCS();
		return FALSE;
	}
*/
    return TRUE;
}

/**
 *	sends the command to the the memory card with the 4 arguments and the CRC
 *
 *	@param command		the command
 *	@param argX			the arguments
 *	@param CRC			cyclic redundancy check
 **/
void MMC_SendCommand(unsigned char command, unsigned char argA,
                 unsigned char argB, unsigned char argC,
                 unsigned char argD, unsigned char CRC)
{	
	SYS_WriteByteSPI(MMC_SPI_MODE, command);
	SYS_WriteByteSPI(MMC_SPI_MODE, argA);
	SYS_WriteByteSPI(MMC_SPI_MODE, argB);
	SYS_WriteByteSPI(MMC_SPI_MODE, argC);
	SYS_WriteByteSPI(MMC_SPI_MODE, argD);
	SYS_WriteByteSPI(MMC_SPI_MODE, CRC);
}

/**
 *	waits for the memory card to response
 *
 *	@param expRes		expected response
 *
 *	@return TRUE		card responded within time limit
 *	@return FALSE		card did not response
 **/
boolean MMC_CardResponse(unsigned char expRes)
{
	unsigned char actRes;
	unsigned int count = 256;
	actRes = 0xFF;
	while ((actRes != expRes) && (count > 0))
	{
    	SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &actRes);		
		count--;
	}
	if (count == 0)
		return FALSE; 
	return TRUE;
}

/**
 *	initializes the memory card by brign it out of idle state and 
 *	sets it up to use SPI mode
 *
 *	@return TRUE		card successfully initialized
 *	@return FALSE		card not initialized
 **/
boolean MMC_MemCardInit(void)
{
	unsigned int count;
    
    MMC_DisableCS();
	for (count = 0;count < 10; count++)
    {
		SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
    }

    MMC_EnableCS();

	MMC_SendCommand(CMD0,0,0,0,0,0x95);	// initializes the MMC

	if(MMC_CardResponse(0x01))
	{				
		count = 255;
		do
		{
			MMC_SendCommand(CMD1,0,0,0,0,0xFF);	// brings card out of idle state
			count--;
		} while ((!MMC_CardResponse(0x00)) && (count > 0));

        MMC_DisableCS();

		SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);	

		if(count == 0)
			return FALSE;
	}
	else
	{
        MMC_DisableCS();
		return FALSE;		
	}
	return TRUE;
}

/**
 *	Initialize the block length in the memory card to be 512 bytes
 *	
 *	return TRUE			block length sucessfully set
 *	return FALSE		block length not sucessfully set
 **/
boolean MMC_SetBLockLength(void)
{
    MMC_EnableCS();

	MMC_SendCommand(CMD16,0,0,2,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
        MMC_DisableCS();
		return TRUE;
	}
	SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);

    MMC_DisableCS();

	return FALSE;
}

/**
 *	locates the offset of the partition table from the absolute zero sector
 *
 *	return ...			the offset of the partition table
 **/
unsigned long MMC_GetPartitionOffset(void)
{
	unsigned int count = 0;
	unsigned long offset = 0;
	unsigned char charBuf;

    MMC_EnableCS();

	MMC_SendCommand(CMD17,0,0,0,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		if (MMC_CardResponse(0xFE))
		{
            for (count = 0; count < 454; count++) 
            {
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			    SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
            }
            
            SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
			offset = charBuf;

            SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
			offset |= (unsigned long int)charBuf << 8;

            SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
			offset |= (unsigned long int)charBuf << 16;

            SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
			offset += (unsigned long int)charBuf << 24;
			for (count = 458; count < SECTOR_SIZE; count++)
            {  
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			    SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
            }
            SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		    SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);

            SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		    SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);
		}
	}
    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
    SYS_ReadByteSPI(MMC_SPI_MODE, &charBuf);

	return offset;
}

/**
 *	checks to see if previous write operation is sucessful
 *	
 *	@return TRUE		if data is successfully written to disk
 *	@return FALSE		if data is not successfully written to disk
 **/
boolean MMC_CheckWriteState(void)
{
	unsigned int count = 0;
	BYTE inRes;
	while (count <= 64)
	{
        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
        SYS_ReadByteSPI(MMC_SPI_MODE, &inRes);
		if ((inRes & 0x1F) == 0x05)
			break;
		else if ((inRes & 0x1F) == 0x0B)
			return FALSE;
		else if ((inRes & 0x1F) == 0x0D)
			return FALSE;
		count++;
	}
	count = 0;

    while (count < 64) 
    {
        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
        SYS_ReadByteSPI(MMC_SPI_MODE, &inRes);
        if (inRes != 0x00) break;
    }
    
	if(count < 64)
		return TRUE;
	else
		return FALSE;
}

/**
 *	read content of the sector indicated by the input and place it in the buffer
 *
 *	@param	sector		sector to be read
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes read
 **/
unsigned int MMC_ReadSector(unsigned long sector, unsigned char *buf)
{
    unsigned int count = 0;
    unsigned char tmp;
    
	sector += sectorZero;
	sector *= 2;	

        MMC_EnableCS();
	MMC_SendCommand(CMD17,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		if (MMC_CardResponse(0xFE))
		{
		    for (count = 0; count < SECTOR_SIZE; count++)
		    {
                        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_ReadByteSPI(MMC_SPI_MODE, &buf[count]);
		    }
                    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		    SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

                    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
        	    SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);
		}
	}
        MMC_DisableCS();

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

	return count;
}

/**
 *	read partial content of the sector indicated by the input and place it in the buffer
 *
 *	@param	sector		sector to be read
 *	@paran	*buf		pointer to the buffer
 *	@param	bytesOffset	starting point in the sector for reading
 *	@param	bytesToRead	number of bytes to read
 *
 *	@return	...			number of bytes read
 **/
unsigned int MMC_ReadPartialSector(unsigned long sector,
                               unsigned int byteOffset,
                               unsigned int bytesToRead,
                               unsigned char *buf)
{
    int count = 0;
	int countRead = 0;
	int leftover = SECTOR_SIZE - byteOffset - bytesToRead;
    unsigned char tmp;

	sector += sectorZero;
	sector *= 2;	

    MMC_EnableCS();

	MMC_SendCommand(CMD17,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		if (MMC_CardResponse(0xFE))
		{
			for (count = 0; count < byteOffset; count++)
            {  
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
       			SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);
            }

			for (countRead = 0; countRead < bytesToRead; countRead++)
			{
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
    			SYS_ReadByteSPI(MMC_SPI_MODE, &buf[countRead]);
			}
			for (count = 0; count < leftover; count++)
            {  
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
       			SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);
            }
			SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		}
	}
    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

	return countRead;
}

/**
 *	read partial content at the end of the sector1 indicated by the input and
 *	the begging of sector 2 and place it in the buffer
 *
 *	@param	sector1		first sector to be read
 *	@param	sector2		second sector to be read
 *	@paran	*buf		pointer to the buffer
 *	@param	bytesOffset	starting point in the sector for reading
 *	@param	bytesToRead	number of bytes to read
 *
 *	@return	...			number of bytes read
 **/
unsigned int MMC_ReadPartialMultiSector(unsigned long sector1,
                                    unsigned long sector2,
                                    unsigned int byteOffset,
                                    unsigned int bytesToRead,
                                    unsigned char *buf)
{
	int count = 0;
	int countRead = 0;
    unsigned char tmp;

	sector1 += sectorZero;
	sector1 *= 2;	
	sector2 += sectorZero;
	sector2 *= 2;	

    MMC_EnableCS();

	MMC_SendCommand(CMD17,(sector1>>16)&0xFF,(sector1>>8)&0xFF,sector1&0xFF,0,0xFF);

	if(MMC_CardResponse(0x00))
	{
		if(MMC_CardResponse(0xFE))
		{
			for (count = 0; count < byteOffset; count++)
            {  
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
       			SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);
            }
			for (count = byteOffset; count < SECTOR_SIZE; count++, countRead++)
			{
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
       			SYS_ReadByteSPI(MMC_SPI_MODE, &buf[countRead]);
			}

			SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, 0xFF);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

    MMC_EnableCS();

	MMC_SendCommand(CMD17,(sector2>>16)&0xFF,(sector2>>8)&0xFF,sector2&0xFF,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		if (MMC_CardResponse(0xFE))
		{			
			for (count = 0; countRead < bytesToRead; count++, countRead++)
			{
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
       			SYS_ReadByteSPI(MMC_SPI_MODE, &buf[countRead]);
			}
			for (count = count; count < SECTOR_SIZE; count++)
            {
                SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
       			SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);
            }
			SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
			SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}

    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

    return countRead;
}

/**
 *	writes content of the buffer to the sector indicated by the input
 *
 *	@param	sector		sector to be written
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes written
 **/
unsigned int MMC_WriteSector(unsigned long sector, unsigned char *buf)
{
    unsigned int count = 0;
    unsigned char tmp;

	sector += sectorZero;
	sector *= 2;	

    MMC_EnableCS();

	MMC_SendCommand(CMD24,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		SYS_WriteByteSPI(MMC_SPI_MODE, 0xFF);
		SYS_WriteByteSPI(MMC_SPI_MODE, 0xFE);

		for (count = 0; count < SECTOR_SIZE; count++)
		{
			SYS_WriteByteSPI(MMC_SPI_MODE, buf[count]);
		}

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

		MMC_CheckWriteState();
	}

    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
    SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

	return count;
}

/**
 *	write the content of the buffer indicated by the input to the MMC/SD card location
 *	indicated by the input sector.
 *
 *	@param	sector		sector to be written to
 *	@paran	*buf		pointer to the buffer
 *	@param	bytesOffset	starting point in the sector for writing
 *	@param	bytesToRead	number of bytes to write
 *
 *	@return	...			number of bytes written
 **/
unsigned int MMC_WritePartialSector(unsigned long sector,
                                unsigned int byteOffset,
                                unsigned int bytesToWrite,
                                unsigned char *buf)
{
    int count = 0;
	int countWrote = 0;	
    unsigned char tmp;
    
	MMC_ReadSector(sector,readBuf);

	sector += sectorZero;
	sector *= 2;	

    MMC_EnableCS();

	MMC_SendCommand(CMD24,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);

	if(MMC_CardResponse(0x00))
	{
		SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_WriteByteSPI(MMC_SPI_MODE, 0xFE);

		for (count = 0; count < byteOffset; count++)		
			SYS_WriteByteSPI(MMC_SPI_MODE, readBuf[count]);

		for (countWrote = 0; countWrote < bytesToWrite; countWrote++, count++)
		{
			SYS_WriteByteSPI(MMC_SPI_MODE, buf[countWrote]);
		}

		for (count = count; count < SECTOR_SIZE; count++)
			SYS_WriteByteSPI(MMC_SPI_MODE, readBuf[count]);

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

		MMC_CheckWriteState();
	}
    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

	return countWrote;
}

/**
 *	write the content of the buffer to the end of the sector1
 *	and the begging of sector 2
 *
 *	@param	sector1			first sector to be written to
 *	@param	sector2			second sector to be written to
 *	@paran	*buf			pointer to the buffer
 *	@param	bytesOffset		starting point in the sector for reading
 *	@param	bytesToWrite	number of bytes to write
 *
 *	@return	...				number of bytes written
 **/
unsigned int MMC_WritePartialMultiSector(unsigned long sector1,
                                     unsigned long sector2,
                                     unsigned int byteOffset,
                                     unsigned int bytesToWrite,
                                     unsigned char *buf)
{
    unsigned int count = 0;
	unsigned int countWrote = 0;	
	unsigned int writeSector2 = bytesToWrite - SECTOR_SIZE + byteOffset;
    unsigned char tmp;
    
	MMC_ReadSector(sector1,readBuf);

	sector1 += sectorZero;
	sector1 *= 2;	

    MMC_EnableCS();
	MMC_SendCommand(CMD24,(sector1>>16)&0xFF,(sector1>>8)&0xFF,sector1&0xFF,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_WriteByteSPI(MMC_SPI_MODE, 0xFE);

		for (count = 0; count < byteOffset; count++)		
			SYS_WriteByteSPI(MMC_SPI_MODE, readBuf[count]);

		for (count = byteOffset; count < SECTOR_SIZE; countWrote++, count++)
		{
			SYS_WriteByteSPI(MMC_SPI_MODE, buf[countWrote]);
		}
        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

		MMC_CheckWriteState();
	}
    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

	MMC_ReadSector(sector2,readBuf);
	sector2 += sectorZero;
	sector2 *= 2;

    MMC_EnableCS();

	MMC_SendCommand(CMD24,(sector2>>16)&0xFF,(sector2>>8)&0xFF,sector2&0xFF,0,0xFF);

	if (MMC_CardResponse(0x00))
	{
		SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_WriteByteSPI(MMC_SPI_MODE, 0xFE);

		for (count = 0; count < writeSector2; countWrote++, count++)
		{
			SYS_WriteByteSPI(MMC_SPI_MODE, buf[countWrote]);
		}

		for (count = writeSector2; count < SECTOR_SIZE; count++)
			SYS_WriteByteSPI(MMC_SPI_MODE, readBuf[count]);		

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

        SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
		SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

		MMC_CheckWriteState();
	}
    MMC_DisableCS();

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
    SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

    SYS_WriteByteSPI(MMC_SPI_MODE, MMC_DUMMY_DATA);
	SYS_ReadByteSPI(MMC_SPI_MODE, &tmp);

	return countWrote;
}

void MMC_EnableCS(void)
{
	MMC_CS_PxOUT  &= ~MMC_CS_BIT;
}

void MMC_DisableCS(void)
{
	MMC_CS_PxOUT  |= MMC_CS_BIT;
}
