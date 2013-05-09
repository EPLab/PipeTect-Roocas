/**
 *	HALayer.h:		implementation of the HALayer.
 *	class HALayer:	defines the hardware specific portions
 *					of the MMC/SD card FAT library.
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

#ifndef _HA_LAYER_
#define _HA_LAYER_

#include "define.h"
#include "typedefs.h"
#include "FATLib.h"

#define MMC_DUMMY_DATA      0xFF

/* Memory card sector size */
#define SECTOR_SIZE 512

/**
 * MMC/SD card SPI mode commands
 **/
#define CMD0 0x40	// software reset
#define CMD1 0x41	// brings card out of idle state
#define CMD2 0x42	// not used in SPI mode
#define CMD3 0x43	// not used in SPI mode
#define CMD4 0x44	// not used in SPI mode
#define CMD5 0x45	// Reserved
#define CMD6 0x46	// Reserved
#define CMD7 0x47	// not used in SPI mode
#define CMD8 0x48	// Reserved
#define CMD9 0x49	// ask card to send card speficic data (CSD)
#define CMD10 0x4A	// ask card to send card identification (CID)
#define CMD11 0x4B	// not used in SPI mode
#define CMD12 0x4C	// stop transmission on multiple block read
#define CMD13 0x4D	// ask the card to send it's status register
#define CMD14 0x4E	// Reserved
#define CMD15 0x4F	// not used in SPI mode
#define CMD16 0x50	// sets the block length used by the memory card
#define CMD17 0x51	// read single block
#define CMD18 0x52	// read multiple block
#define CMD19 0x53	// Reserved
#define CMD20 0x54	// not used in SPI mode
#define CMD21 0x55	// Reserved
#define CMD22 0x56	// Reserved
#define CMD23 0x57	// Reserved
#define CMD24 0x58	// writes a single block
#define CMD25 0x59	// writes multiple blocks
#define CMD26 0x5A	// not used in SPI mode
#define CMD27 0x5B	// change the bits in CSD
#define CMD28 0x5C	// sets the write protection bit
#define CMD29 0x5D	// clears the write protection bit
#define CMD30 0x5E	// checks the write protection bit
#define CMD31 0x5F	// Reserved
#define CMD32 0x60	// Sets the address of the first sector of the erase group
#define CMD33 0x61	// Sets the address of the last sector of the erase group
#define CMD34 0x62	// removes a sector from the selected group
#define CMD35 0x63	// Sets the address of the first group
#define CMD36 0x64	// Sets the address of the last erase group
#define CMD37 0x65	// removes a group from the selected section
#define CMD38 0x66	// erase all selected groups
#define CMD39 0x67	// not used in SPI mode
#define CMD40 0x68	// not used in SPI mode
#define CMD41 0x69	// Reserved
#define CMD42 0x6A	// locks a block
// CMD43 ... CMD57 are Reserved
#define CMD58 0x7A	// reads the OCR register
#define CMD59 0x7B	// turns CRC off
// CMD60 ... CMD63 are not used in SPI mode


/**
 *	case insensitive compare of the 2 inputs
 *	returns true if and only if c1 = c2 or if
 *	c1 and c2 are the same character with different
 *	capitialization
 *
 *	@param	c1			input 1
 *	@param	c2			input 2
 **/
boolean MMC_CharEquals(const char c1, const char c2);

/**
 *	Hardware Abstraction Layer initialization
 **/

/**
 *	checks if there is a memory card in the slot
 *
 *	@return	FALSE		card not detected
 *	@return TRUE		car detected
 **/
boolean MMC_DetectCard(void);

/**
 *	sends the command to the the memory card with the 4 arguments and the CRC
 *
 *	@param command		the command
 *	@param argX			the arguments
 *	@param CRC			cyclic redundancy check
 **/
void MMC_SendCommand(unsigned char command, unsigned char argA, unsigned char argB,
                 unsigned char argC, unsigned char argD, unsigned char CRC);

/**
 *	waits for the memory card to response
 *
 *	@param expRes		expected response
 *
 *	@return TRUE		card responded within time limit
 *	@return FALSE		card did not response
 **/
boolean MMC_CardResponse(unsigned char expRes);

/**
 *	initializes the memory card by brign it out of idle state and 
 *	sets it up to use SPI mode
 *
 *	@return TRUE		card successfully initialized
 *	@return FALSE		card not initialized
 **/
boolean MMC_MemCardInit(void);

/**
 *	Initialize the block lenght in the memory card to be 512 bytes
 *	
 *	return TRUE			block length sucessfully set
 *	return FALSE		block length not sucessfully set
 **/
boolean MMC_SetBLockLength(void);

/**
 *	locates the offset of the partition table from the absolute zero sector
 *
 *	return ...			the offset of the partition table
 **/
unsigned long MMC_GetPartitionOffset(void);

/**
 *	read content of the sector indicated by the input and place it in the buffer
 *
 *	@param	sector		sector to be read
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes read
 **/
unsigned int MMC_ReadSector(unsigned long sector, unsigned char *buf);

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
unsigned int MMC_ReadPartialSector(unsigned long sector, unsigned int byteOffset, unsigned int bytesToRead, unsigned char *buf);

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
                                    unsigned char *buf);

/**
 *	writes content of the buffer to the sector indicated by the input
 *
 *	@param	sector		sector to be written
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes written
 **/
unsigned int MMC_WriteSector(unsigned long sector, unsigned char *buf);

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
                                unsigned char *buf);

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
                                     unsigned char *buf);

void MMC_EnableCS(void);
void MMC_DisableCS(void);

#endif
