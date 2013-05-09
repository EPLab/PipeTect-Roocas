/**
 *	FATLib.h:	interface for the FATLib.
 *	class FATLib:	a portable FAT decoder class which is hardware independent.
 *			All hardware specific operations are abstracted with the
 *			class HALayer.  The FATLib class operates with only the buffer
 *			which it passes to the class HALayer
 *
 *	Author:	Ivan Sham
 *	Date: JUly 1, 2004
 *	Version: 2.0
 *	Note: Developed for William Hue and Pete Rizun
 *
 *****************************************************************
 *  Change Log
 *----------------------------------------------------------------
 *  Date    |   Author          | Reason for change
 *----------------------------------------------------------------
 * Aug31/04     William Hue       Changed char types to
 *                                unsigned char.
 *                                Put more parentheses in abs()
 *                                macro.
 *
 * Jul18/04     Alex Jiang        Ported to FAB belt clip. Made
 *                                local variables and functions
 *                                static and moved to fatlib.c.
 *                                Prefixed public funtios with
 *                                "fat_".
 *
 * Jan02/05     William Hue       Various bugfixes and clean-up for
 *                                Circuit Cellar article.
 **/


#ifdef __FILE_SAVE_ENABLED__

#ifndef _FATLIB_
#define _FATLIB_

#include "mmc.h"

#define UNKNOWN 0
#define FAT16 1

#define DIRECTORY TRUE
#define FILE FALSE

#define READ TRUE
#define WRITE FALSE

#define BUFFER_SIZE 4

#define abs(x)  (((x) > 0) ? (x) : (-(x)))


void FAT_Test(void);
//------------------
// member functions:
//------------------

/**
 *	initialize the system
 *
 *	@return	0			UNKNOWN file system
 *	@return	1			FAT16 file system
 *	@return 3			could not set block length
 *	@return	4			could not initialize memory card
 **/
unsigned char FAT_Initialize(void);

/**
 *	closes the file indicated by the input
 *
 *	@param	fileHandle	handle of file to be closed
 *
 *	@return	0			file sucessfully closed
 *	@return	-1			invalid file handle
 *	@return	-2			invalid file system
 **/
signed char FAT_Close(signed char fileHandle);

/**
 *	opens the file indicated by the input path name.  If the pathname
 *	points to a valid file, the file is added to the list of currently
 *	opened files for reading and the unique file handle is returned.
 *
 *	@param	pathname	a pointer to the location of the file to be opened
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *
 *	@return	-1			invalid pathname
 *	@return	-2			file does not exist
 *	@return	-3			file already opened for writing
 *	@return -4			file already opened for reading
 *	@return -10			no handles available
 *	@return	-20			memory card error
 *	@return	-128		other error
 *	@return	...			file handle of sucessfully opened file
 **/
signed char FAT_OpenRead(char * pathname);

/**
 *	opens the file indicated by the input path name.  If the pathname
 *	points to a valid path, the file is created and added to the list of
 *	currently opened files for writing and the unique file handle is returned.
 *
 *	@param	pathname	a pointer to the location of the file to be opened
 *
 *	@return	-1			invalid pathname
 *	@return	-2			file already exist
 *	@return	-3			file already opened for writing
 *	@return	-4			no directory entries left
 *	@return -10			no handles available
 *	@return	-20			memory card error
 *	@return	-128		other error
 *	@return	(non-negative)	file handle of sucessfully opened file
 **/
signed char FAT_OpenWrite(char * pathname);

/**
 *	reads the content of the file identified by the input handle.  It reads from
 *	where the last read operation on the same file ended.  If it's the first time
 *	the file is being read, it starts from the beginning of the file.
 *
 *	@pre	nByte < SECTOR_SIZE
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	handle		handle of file to be read
 *	@param	nByte		number of bytes to read
 *
 *	@return	-10			memory card error
 *	@return -1			invalid handle
 *	@return -32768		other error
 *	@return	...			number of bytes read
 **/
signed int FAT_Read(signed char handle, unsigned char *buf, unsigned int nByte);

/**
 *	writes the content in the buffer to the file identified by the input handle.  It writes
 *	to where the last write operation on the same file ended.  If it's the first time
 *	the file is being written to, it starts from the beginning of the file.
 *
 *	@pre	nByte < SECTOR_SIZE
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	handle		handle of file to be written to
 *	@param	nByte		number of bytes to write
 *
 *	@return	-10			memory card error
 *	@return -1			invalid handle
 *	@return	-2			memory card is full
 *	@return -32768		other error
 *	@return	...			number of bytes written
 **/
signed int FAT_Write(signed char handle, unsigned char *buf, unsigned int nByte);

/**
 *	updates the file size in the directory table for all files with the update flag set
 **/
void FAT_Flush(void);


#endif	// _FATLIB_

#endif //__FILE_SAVE_ENABLED__