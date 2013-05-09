/**
 *	FATLib.c:		implementation of the FATLib class.
 *	class FATLib:	a portable FAT decoder class which is hardware independent.
 *			All hardware specific operations are abstracted with the
 *			class HALayer.  The FATLib class operates with only the buffer
 *			which it passes to the class HALayer
 *
 *	Author:	Ivan Sham
 *	Date: July 1, 2004
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
 *
 * Jul18/04     Alex Jiang        Made local variables and functions
 *                                static and moved to fatlib.c.
 *                                Prefixed public functions with
 *                                "fat_".
 *
 * Jan02/05     William Hue       Various bugfixes and clean-up for
 *                                Circuit Cellar article.
 **/

#ifdef __FILE_SAVE_ENABLED__

#include "FATLib.h"
#include "rtc.h"

//------------------
// private member variables:
//------------------

/**
 *	defines the structure of a opened file
 **/
typedef struct
{
	unsigned long currentCluster;
	unsigned int byteCount;	
	unsigned long firstCluster;    
    unsigned long fileSize;
    unsigned long dirLocation;    
    unsigned char sectorCount;    
	signed char fileHandle;
    unsigned char updateDir;
} file;


/**
 *	array of size BUFFER_SIZE of pointers to nodes of
 *	struct file opened for reading
 **/
static file openedRead[BUFFER_SIZE];

/**
 *	array of size BUFFER_SIZE of pointers to nodes of
 *	struct file opened for writing
 **/
static file openedWrite[BUFFER_SIZE];

/**
 *	sectors offset from absolute sector 0 in the MMC/SD.
 *  All operations with the MMC/SD card are offseted with
 *	this variable in a microcontroller application
 *	****this equals zero in Windows application
 **/
unsigned long sectorZero;

/**
 *	the sector in the MMC/SD card where data starts.
 *	This is right after the root directory sectors in FAT16
 **/
static unsigned long dataStarts;

/**
 *	the first sector where entries in the root directory is
 *	stored in FAT16.
 **/
static unsigned long rootDirectory;

/**
 *	the number of sectors per File Allocation Table (FAT)
 **/
static unsigned long sectorsPerFAT;

/**
 *	the number of sectors per cluster for the MMC/SD card
 **/
static unsigned int sectorsPerCluster;

/**
 *	the number of reserved sectors in the MMC/SD card
 **/
static unsigned int reservedSectors;

/**
 *	the number of File Allocation Table (FAT) on the MMC/SD.
 *	this is usually 2
 **/
static unsigned int numOfFATs;

/**
 *	the number of sectors in the section for root directory
 *	entries.
 **/
static unsigned int rootSectors;

/**
 *	the file system of which the MMC/SD card is formatted in
 *
 *	fileSys == 0  -> Unknown file system
 *	fileSys == 1  -> FAT16
 **/
static unsigned char fileSys;

/**
 *	number of files opened for reading
 **/
static unsigned char filesOpenedRead;

/**
 *	number of files opened for writing
 **/
static unsigned char filesOpenedWrite;

/**
 *	FAT sector number last searched for an empty cluster
 **/
static unsigned long int last_FAT_sector_searched_for_empty;

static unsigned char g_FATBuf[SECTOR_SIZE];


/* Local Prototypes */
static void FAT_LibInit();
static unsigned char FAT_IdentifyFileSystem(unsigned char *buf);
static signed char FAT_ReadFileSystemInfo(unsigned char *buf);
static unsigned long FAT_FindEmptyCluster(unsigned char *buf);
static unsigned long FAT_GetNextFAT(unsigned long cluster, unsigned char *buf);
static void FAT_UpdateFAT(unsigned long oldCluster, unsigned long newCluster, unsigned char *buf);
static signed char FAT_GetFileHandle();
static unsigned long FAT_GetFirstCluster(const char * pathname, unsigned long cluster, unsigned char *buf, boolean control, unsigned long * fileSize);
static unsigned long FAT_CreateNewEntry( const char * entryName, unsigned long cluster, unsigned char *buf, boolean control);
static signed char FAT_FindFileIndex(signed char handle);
static signed char FAT_OpenedCheck(unsigned long cluster);
static void FAT_UpdateDirectory(unsigned long dirCluster, unsigned long firstCluster, unsigned long filesize, unsigned char *buf);
static void FAT_UpdateAccessDate(unsigned long dirCluster, unsigned long firstCluster, unsigned char *buf);

void FAT_Test(void)
{
    signed int stringSize;
    signed char handle;
    unsigned char stringBuf[30];
    char i;
    RTC_TIME time1, time2;
    
   	handle = FAT_OpenWrite("hello.txt");
    
    if (handle >= 0)
    {
        RTC_GetTime(&time1); 
        for (i=0; i<100; i++)
        {
            P1OUT ^= BIT3;                  
            strcpy((char *)stringBuf, "[1:100] 12:22:33.4455 255 255 255\n");
            stringSize = strlen((char *)stringBuf);
            FAT_Write(handle, stringBuf, stringSize);
            FAT_Flush();    // Optional.
        }
        FAT_Close(handle);
    RTC_GetTime(&time2); 
    }
}

/**
 *	default constructor
 **/
static void FAT_LibInit()
{
	unsigned char i;

	sectorZero = 0;
	filesOpenedRead = 0;
	filesOpenedWrite = 0;
	fileSys = UNKNOWN;
	for(i = 0; i < BUFFER_SIZE;i++)
	{
		openedRead[i].fileHandle = -1;
		openedWrite[i].fileHandle = -1;
	}
    last_FAT_sector_searched_for_empty = 0;
}

/**
 *	initialize the system
 *
 *	@return	0			UNKNOWN file system
 *	@return	1			FAT16 file system
 *	@return 3			could not set block length
 *	@return	4			could not initialize memory card
 **/
unsigned char FAT_Initialize(void)
{
	FAT_LibInit();
	if(MMC_MemCardInit())
	{
		if(MMC_SetBLockLength())
		{
			sectorZero = MMC_GetPartitionOffset();
		}
		else
		{
			return 3;
		}
	}
	else
	{
		return 4;
	}
	FAT_IdentifyFileSystem(g_FATBuf);
	FAT_ReadFileSystemInfo(g_FATBuf);
	return fileSys;
}

/**
 *	identifies the file system used by the MMC/SD card
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *
 *	@return	0			for unknow file system
 *	@return	1			for FAT16
 **/
static unsigned char FAT_IdentifyFileSystem(unsigned char *buf)
{
	MMC_ReadSector(0, buf);
	fileSys = UNKNOWN;	
	if( (buf[54] == 'F') &&
		(buf[55] == 'A') &&
		(buf[56] == 'T') &&
		(buf[57] == '1') &&
		(buf[58] == '6'))
	{
		fileSys = FAT16;		
	}
	return fileSys;
}

/**
 *	initialize variables for file system
 *
 *	@param	buf		the buffer to be used to access the MMC/SD card
 *
 *	@return	0			variables sucessfully initialized
 *	@return	-1			unknown file system
 *	@return -2			error reading from MMC/SD card 
 **/
static signed char FAT_ReadFileSystemInfo(unsigned char *buf)
{	
	unsigned int maxRootEntry = 0;

	if(fileSys == UNKNOWN)
	{
		return -1;
	}
	if(MMC_ReadSector(0, buf) < SECTOR_SIZE)
	{
		return -2;
	}

	sectorsPerCluster = buf[13];
	reservedSectors = buf[14];
	reservedSectors |= (unsigned long int)buf[15] << 8;
	numOfFATs = buf[16];

	if(fileSys == FAT16)
	{
		maxRootEntry = buf[17];
		maxRootEntry |= (unsigned long int)buf[18] << 8;
		sectorsPerFAT = buf[22];
		sectorsPerFAT |= (unsigned long int)buf[23] << 8;
	} else
	{			
		sectorsPerFAT = buf[36];
		sectorsPerFAT |= (unsigned long int)buf[37] << 8;
		sectorsPerFAT |= (unsigned long int)buf[38] << 16;
		sectorsPerFAT |= (unsigned long int)buf[39] << 24;		
	}
	rootDirectory = reservedSectors + sectorsPerFAT * numOfFATs;
	rootSectors = ( maxRootEntry / SECTOR_SIZE ) * 32;
	dataStarts = rootDirectory + rootSectors;
	return 0;
}

/**
 *	finds an unoccupied cluster in the FAT and returns the location.
 *	The previously empty location in the FAT will be changed to a
 *	end of file marker
 *
 *	@pre	file system is properly initialized and fileSys != UNKNOWN
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *
 *	@return	0			no more clusters available (disk full)
 *	@return	...			location of empty cluster
 **/
static unsigned long FAT_FindEmptyCluster(unsigned char *buf)
{
	unsigned long sector, end_sector;
	unsigned long returnCluster;
	unsigned int i;
	unsigned char j;

    if (last_FAT_sector_searched_for_empty == 0)
    {
        sector = reservedSectors;
    }
    else
    {
        sector = last_FAT_sector_searched_for_empty;
    }
    end_sector = reservedSectors + sectorsPerFAT;

    if(fileSys == FAT16)
    {
        // 256 entries in a 512-byte FAT sector
        returnCluster = (sector - reservedSectors) * 256;
    }
    else
    {
        return 0;
    }

    while (TRUE)
    {
        while(sector < end_sector)
        {
            MMC_ReadSector(sector, buf);
            for(i = 0; i < SECTOR_SIZE; i += 2*fileSys)
            {
                if(buf[i] == 0x00 && buf[i+1] == 0)                   
                {
                    // This cluster is unused.
                    buf[i] = 0xFF;
                    buf[i + 1] = 0xFF;
                  	
                    for(j = 0; j < numOfFATs; j++)
                    {
                        MMC_WriteSector(sector + (j * sectorsPerFAT), buf);
                    }

                    // Remember where we found it:
                    last_FAT_sector_searched_for_empty = sector;

                    return returnCluster;
                }
                returnCluster++;
            }
            sector++;
        }

        // We reached the end of the FAT.  Should we wrap around and search from
        // the beginning?
        if (last_FAT_sector_searched_for_empty > reservedSectors)
        {
            // Yes.
            sector = reservedSectors;
            end_sector = last_FAT_sector_searched_for_empty;
            last_FAT_sector_searched_for_empty = 0;
            returnCluster = 0;
            // We will take the else block next time if we don't find anything.
        }
        else
        {
            return 0; 
        }
    }
}

/**
 *	gets the next cluster in the FAT chain given the current cluster
 *
 *	@param	cluster		the current cluster
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *
 *	@return	...			the location of the next cluster or end of file marker 
 **/
static unsigned long FAT_GetNextFAT(unsigned long cluster, unsigned char *buf)
{
	unsigned long sectorOffset;
	unsigned int byteOffset;
	
	cluster = 2 * cluster * fileSys;
	sectorOffset = (cluster >> 9);
	sectorOffset = sectorOffset + reservedSectors;
	byteOffset = cluster & 0x01FF;
	MMC_ReadSector(sectorOffset, buf);
	cluster = buf[byteOffset];
	cluster |= (unsigned long int)buf[byteOffset + 1] << 8;
	
	return cluster;
}

/**
 *	updates the FAT from in location of oldCluster and place the newCluster in 
 *	that location
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	oldCluster	the location of the cluster to be replaced 
 *	@param	newCluster	the new location
 **/
static void FAT_UpdateFAT(unsigned long oldCluster, unsigned long newCluster, unsigned char *buf)
{
	unsigned long sectorOffset;
	unsigned int byteOffset;
	unsigned char j;

	oldCluster = 2 * oldCluster * fileSys;
	sectorOffset = (oldCluster >> 9);
	sectorOffset = sectorOffset + reservedSectors;
	byteOffset = oldCluster & 0x01FF;
	MMC_ReadSector(sectorOffset, buf);
	buf[byteOffset] = newCluster & 0x000000FF;
	buf[byteOffset+1] = (newCluster & 0x0000FF00) >> 8;	
	
	for(j = 0; j < numOfFATs; j++)
	{
		MMC_WriteSector(sectorOffset + (j * sectorsPerFAT), buf);
	}
}

/**
 *	gets the next available file handle 
 *	Note:  all files have unique handle (no overlap between read and write)
 *
 *	@return	-10			no more file handles available
 *	@return ...			file handle
 **/
static signed char FAT_GetFileHandle()
{
	signed char i = 0;
	signed char newHandle = 0;
	while(newHandle < 2 * BUFFER_SIZE)
	{
		i = 0;
		while((i < BUFFER_SIZE) && (i >= 0))
		{
			if(openedRead[i].fileHandle != -1)
			{				
				if(openedRead[i].fileHandle==newHandle)
				{
					i = -128;
				}
			}
			if((openedWrite[i].fileHandle != -1)&&(i>=0))
			{
				if(openedWrite[i].fileHandle==newHandle)
				{
					i = -128;
				}
			}
			i++;
		}
		if(i == BUFFER_SIZE)
		{
			return newHandle;
		}
		newHandle++;
	}
	return -10;
}

/**
 *	closes the file indicated by the input
 *	
 *	@param	fileHandle	handle of file to be closed
 *	
 *	@return	0			file sucessfully closed
 *	@return	-1			invalid file handle
 *	@return	-2			invalid file system
 **/
signed char FAT_Close(signed char fileHandle)
{
	unsigned char i;

	if(MMC_DetectCard() == FALSE)
	{
		return -10;
	}
	if(fileSys == UNKNOWN)
	{
		return -2;
	}

    FAT_Flush();

	for(i = 0; i < filesOpenedRead; i++)
	{
		if(openedRead[i].fileHandle == fileHandle)
		{
			filesOpenedRead--;
			openedRead[i].fileHandle = -1;
			return 0;
		}
	}
	for(i = 0; i < filesOpenedWrite; i++)
	{
		if(openedWrite[i].fileHandle == fileHandle)
		{
			filesOpenedWrite--;
			openedWrite[i].fileHandle = -1;
			return 0;
		}
	}
	return -1;
}

/**
 *	determines the location of the cluster of the file or directory
 *	pointed to by the input pathname.  Only the first token in 
 *	the string pointed to by pathname is used (anything before '\').
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	pathname	pointer to the string of the pathname
 *	@param	cluster		location of the current directory where the
 *						file or directory being seek is located
 *	@param	control		determines whether a file or a directory is being seek
 *	@param	fileSize	stores the size of the file being seeked
 *
 *	@return	0			if the directory or file does not exist
 *	@return ...			the first cluster of the directory or file being seeked
 **/
static unsigned long FAT_GetFirstCluster(const char *pathname, unsigned long cluster, unsigned char *buf, boolean control, unsigned long * fileSize)
{
	int offset = 0;
	unsigned int sectorCount;
	unsigned long sector;		
	unsigned char i, j;
	unsigned char tokenLength = 0;		// path name contails tokes less than 255 characters long
	const char *tempPathname;
	boolean matchingNames;
	boolean targetFound;

	if((cluster == 1)&&(fileSys == FAT16))
	{
		sector = rootDirectory;
	}
	else
	{
        return 0;
	}

	tempPathname = pathname;

	while((*tempPathname != 0x5C)&&(*tempPathname))
	{
		tokenLength++;
		tempPathname++;
	}
	
	targetFound = FALSE;
	sectorCount = 0;
	i = 0;
	MMC_ReadSector(sector, buf);
	if(((tokenLength < 13)&&(control == FILE)))
	{	
		while((targetFound == FALSE)&&(sector > 0))
		{
			offset = i * 32;
			tempPathname = pathname;
			matchingNames = TRUE;

            if (control == FILE)
			{								
                if (buf[offset] == '\0')
                {
                    // End of entries.
                    break;
                }

                // Check attributes except for ARCHIVE or READONLY, and for deleted entries:
				if(((buf[11+offset] & 0xDE) == 0x00)&&((buf[offset] & 0xFF) != 0xE5))
				{
					for(j = 0; j < 8; j++)
					{					
                        if (*tempPathname != '.' && *tempPathname != '\0')
                        {
                            // Before end of filename, characters must match:
                            if (!MMC_CharEquals(buf[offset+j], *tempPathname))
                            {
                                matchingNames = FALSE;
                            }

							tempPathname++;
                        }
						else if ((buf[offset+j]) != ' ')
						{
                            // After end of filename, must have blanks.
							matchingNames = FALSE;
						}
					}

                    if (*tempPathname != '\0')
                    {
                        tempPathname++;
                    }

					for(j = 8; j < 11; j++)
					{					
                        if (*tempPathname != '\0')
                        {
                            // Before end of extension, characters must match:
                            if (!MMC_CharEquals(buf[offset+j], *tempPathname))
                            {
                                matchingNames = FALSE;
                            }

							tempPathname++;
                        }
						else if ((buf[j+offset]) != ' ')
						{
                            // After end of extension, must have blanks.
							matchingNames = FALSE;
						}
					}

					if (matchingNames)
					{
						targetFound = TRUE;
					}
				}

			}
			i++;
			if(i == 16)		// 16 = directory entries per sector (fixed number)
			{
				i = 0;
				sectorCount++;
				if((cluster == 1)&&(fileSys == FAT16))
				{
					if( sectorCount < rootSectors )
					{
						sector++;
					}
					else
					{
						sector = 0;
					}
				}
				else
				{
					if(sectorCount != sectorsPerCluster)
					{
						sector++;
					}
					else
					{
						sectorCount = 0;
						cluster = FAT_GetNextFAT(cluster, buf);
						if((cluster <= 0xFFF6)&&(cluster >= 2))
						{
							sector = (cluster - 2) * sectorsPerCluster + dataStarts;								
						}
						else
						{
							sector = 0;
						}
					}
				}
				MMC_ReadSector(sector, buf);
			}		
		}
	}

	if(targetFound)
	{
		cluster = buf[26+offset];
		cluster |= (unsigned long int)buf[27+offset] << 8;
		
		*fileSize = buf[28+offset];
		*fileSize += (unsigned long int)buf[29+offset] << 8;
		*fileSize += (unsigned long int)buf[30+offset] << 16;
		*fileSize += (unsigned long int)buf[31+offset] << 24;
	}
	else
	{
		cluster = 0;
	}
	return cluster;
}


/**
 *	creates a new entry (file or directory) at the location indicated by the
 *	input cluster.  The input control determines whether a file or a
 *	directory is created
 *	*****ONLY FILES AND DIRECTORY WITH SHORT FILE NAMES ARE SUPPORTED
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card	
 *	@param	entryName	pointer to the name of the new entry
 *	@param	cluster		location of the current directory where the new entry will be added
 *	@param	control		FILE or DIRECTORY
 *
 *	@return	0			if an error occurs while adding a new entry
 *	@return	...			the location of the first cluster of the new entry
 **/
static unsigned long FAT_CreateNewEntry(const char *entryName, unsigned long cluster, unsigned char *buf, boolean control)
{
	unsigned int offset;
	unsigned int sectorCount = 0;
	unsigned int tokenLength = 0;		
	unsigned long sector;
	unsigned long newCluster;
	unsigned char i;	
	const char *tempEntryName;	
	boolean done = FALSE;

    // Note:  findEmptyCluster() will mark the returned cluster in the FAT
    // as used.  If we fail for some other reason below, we should really
    // free the newCluster, but we don't right now.  Also, unless we are
    // creating a directory, we really shouldn't allocate a first cluster
    // until some data is written to the file.  Additionally, if we re-use a
    // deleted entry, we should re-use the cluster chain, adding clusters as
    // required, but we don't do that right now either.
	newCluster = FAT_FindEmptyCluster(buf);
	if(newCluster == 0)
	{
		return 0;	// no more empty cluster
	}

	if((cluster == 1) && (fileSys == FAT16))
	{
		sector = rootDirectory;
	}
	else
	{
		sector = (cluster - 2) * sectorsPerCluster + dataStarts;		
	}

	tempEntryName = entryName;

	while((*tempEntryName != '.')&&(*tempEntryName)&&(*tempEntryName != 0x5C))
	{
		tokenLength++;
		tempEntryName++;
	}

	while(!done)
	{				
		MMC_ReadSector(sector, buf);
		i = 0;
		while(i < 16)
		{
			offset = i * 32;			
			if(((buf[offset] & 0xFF) == 0x00) || ( (buf[offset] & 0xFF) == 0xE5) )
			{
				done = TRUE;
				i = 15;
			}
			i++;
		}
		if(!done)
		{
			sectorCount++;
			if((cluster == 1)&&(fileSys == FAT16))
			{
				if(sectorCount < rootSectors)
				{
					sector++;
				}
				else
				{
					return 0;	// no more root directory
				}
			}
			else
			{
                return 0;
			}				
		}
	}

	if(control == FILE)
	{
		buf[offset+11] = 0x20;
		if(tokenLength < 9)
		{			
			for(i = 0; i < 8; i++)
			{
				if(*entryName != '.')
				{
					if((*entryName >= 'a')&&(*entryName <= 'z'))
					{
						buf[offset+i] = (*entryName - 32);
					}
					else
					{
						buf[offset+i] = *entryName ;
					}
					entryName++;
				}
				else
				{
					buf[offset+i] = 0x20;
				}
			}
			entryName++;
			for(i = 8; i < 11; i++)
			{
				if(*entryName)
				{
					if((*entryName >= 'a')&&(*entryName <= 'z'))
					{
						buf[offset+i] = (*entryName - 32);
					}
					else
					{
						buf[offset+i] = *entryName ;
					}
					entryName++;
				}
				else
				{
					buf[offset+i] = 0x20;
				}
			}						
		}
		else
		{
			// file with long file name
			return 0;
		}
	}

	buf[offset+12] = 0x00;
	buf[offset+13] = 0x00;

    // Set the date and time to January 02, 2005 21:00:00 .
    // If a real time clock is available, we could use it.
	buf[offset+14] = (04 >> 1) & 0x1F;  // Seconds.
	buf[offset+14] |= 03 << 5;  // Part of minutes.
	buf[offset+22] = buf[offset+14];

	buf[offset+15] = (03 >> 3) & 0x07;  // More of minutes.
	buf[offset+15] |= 21 << 3;  // Hours.
	buf[offset+23] = buf[offset+15];			
	
	buf[offset+16] = 02 & 0x1F;     // Day.
	buf[offset+16] |= (01 << 5) & 0xE0;   // Part of month.
	buf[offset+18] = buf[offset+16];
	buf[offset+24] = buf[offset+16];
				
	buf[offset+17] = (01 >> 3) & 0x01;   // More of month.
	buf[offset+17] |= (((2005 - 1980) & 0xFF) << 1) & 0xFE;  // Year.
	buf[offset+19] = buf[offset+17];
	buf[offset+25] = buf[offset+17];

	buf[offset+26] = (newCluster & 0xFF);
	buf[offset+27] = (newCluster >> 8) & 0xFF;

	for(i = 28; i < 32; i++)
	{
		buf[offset+i] = 0x00;
	}
	MMC_WriteSector(sector, buf);

	return newCluster;
}


/**
 *	finds the index of the for the array of pointers which
 *	corresponds to the input file handle.
 *
 *	@param	handle		the handle of the file being seeked
 *
 *	@return	-1			invalid file handle
 *	@return	...			the index to the correct file pointer
 **/
static signed char FAT_FindFileIndex(signed char handle)
{
	signed char i, tempIndex;

	for(i = 0; i < BUFFER_SIZE; i++)
	{
        tempIndex = openedWrite[i].fileHandle;

		if(openedRead[i].fileHandle != -1)
		{
			if(openedRead[i].fileHandle == handle)
			{
					return i;
			}
		}
		if(tempIndex != -1)
		{
			if(tempIndex == handle)
			{
					return i;
			}
		}
	}
	return -1;
}

/**
 *	checks to see if the file indicated by the input cluster
 *	is already opened for either reading or writing
 *
 *	@pre	the input must be the location of the first cluster 
 *			of a file
 *	@param	cluster		first cluster of the file being checked
 *
 *	@return	-1			file is already opened for reading
 *	@return	-2			file is already opened for writing
 *	@return	0			file is currently not opened
 **/
static signed char FAT_OpenedCheck(unsigned long cluster)
{
	unsigned char i;
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		if(openedRead[i].fileHandle != -1)
		{
			if(openedRead[i].firstCluster == cluster)
			{
				return -1;
			}
		}
	}
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		if(openedWrite[i].fileHandle != -1)
		{
			if(openedWrite[i].firstCluster == cluster)
			{
				return -2;
			}
		}
	}
	return 0;
}

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
signed char FAT_OpenRead(char *pathname)
{
	char *tempPathname;
	unsigned long cluster;
	unsigned long tempCluster;
	unsigned long fileSize = 0;
	signed char i = 0;
	signed char index = -1;

	if(MMC_DetectCard() == FALSE)
	{
		return -20;
	}

	if(filesOpenedRead >= BUFFER_SIZE)
	{
		return -10;
	}
	else
	{
		while((i < BUFFER_SIZE) && (index < 0))
		{
			if(openedRead[i].fileHandle == -1)
			{
				index = i;
			}
			i++;
		}
	}
	cluster = fileSys;	
	if(*pathname == 0x5C)
	{
		pathname++;
	}
	tempPathname = pathname;

	while(*tempPathname)
	{
		if(*tempPathname == 0x5C)
			return -128;
		tempPathname++;
	}	

	tempPathname = pathname;
	if(cluster != 0)
	{
		tempCluster = cluster;
		cluster = FAT_GetFirstCluster(tempPathname, cluster, g_FATBuf, FILE, &fileSize);
	}
	else
	{
		return -1;
	}
	if(cluster > 0)
	{
		i = FAT_OpenedCheck(cluster);
		if( i < 0 )
		{
			if(i ==  -1)
			{
				return -4;
			}
			else
			{
				return -3;
			}
		}
		openedRead[index].dirLocation = tempCluster;
		openedRead[index].firstCluster = cluster;	
		openedRead[index].currentCluster = cluster;
		openedRead[index].fileSize = fileSize;
		openedRead[index].byteCount = 0;
		openedRead[index].sectorCount = 0;
		openedRead[index].fileHandle = FAT_GetFileHandle();
        openedRead[index].updateDir = FALSE;
		filesOpenedRead++;
		return openedRead[index].fileHandle;
	}
	else
	{
		return -2;
	}
}

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
signed char FAT_OpenWrite(char *pathname)
{
	char *tempPathname;
	unsigned long cluster;
	unsigned long tempCluster;
	unsigned long fileSize = 0;
	signed char i = 0;
	signed char index = -1;

	if(MMC_DetectCard() == FALSE)
	{
		return -20;
	}

	if(filesOpenedWrite >= BUFFER_SIZE)
	{
		return -10;
	}
	else
	{
		while((i < BUFFER_SIZE) && (index < 0))
		{
			if(openedWrite[i].fileHandle == -1)
			{
				index = i;
			}
			i++;
		}
	}
	cluster = fileSys;
	
	if(*pathname == 0x5C)
	{
		pathname++;
	}
	tempPathname = pathname;
	while(*tempPathname)
	{
		if(*tempPathname == 0x5C)
			return -128;
		tempPathname++;
	}	

	tempPathname = pathname;

	if(cluster != 0)
	{
		tempCluster = cluster;
		cluster = FAT_GetFirstCluster(tempPathname, cluster, g_FATBuf, FILE, &fileSize);
		if(cluster != 0)
		{
			return -2;
		}
		else
		{	
			cluster = FAT_CreateNewEntry(tempPathname, tempCluster, g_FATBuf, FILE);
			if(cluster != 0)
			{
				openedWrite[index].currentCluster = cluster;
				openedWrite[index].dirLocation = tempCluster;
				openedWrite[index].fileHandle = FAT_GetFileHandle();
				openedWrite[index].fileSize = 0;
				openedWrite[index].sectorCount = 0;
				openedWrite[index].firstCluster = cluster;
				openedWrite[index].byteCount = 0;
                openedWrite[index].updateDir = FALSE;
				filesOpenedWrite++;
				return openedWrite[index].fileHandle;
			}
			else
			{
				return -4;
			}
		}
	}
	else
	{
		return -1;
	}
}

/**
 *	reads the content of the file identified by the input handle.  It reads from
 *	where the last read operation on the same file ended.  If it's the first time
 *	the file is being read, it starts from the begining of the file.
 *
 *	@pre	nByte < SECTOR_SIZE
 *
 *	@param	buf			the buffer to be used to access the MMC/SD card
 *	@param	handle		handle of file to be closed
 *	@param	nByte		number of bytes to read
 *
 *	@return	-10			memory card error
 *	@return -1			invalid handle
 *	@return -32768		other error
 *	@return	...			number of bytes read
 **/
signed int FAT_Read(signed char handle, unsigned char *buf, unsigned int nByte)
{
	unsigned long sectorToRead;
	unsigned long sectorToRead2;
	signed char index;

	if(MMC_DetectCard() == FALSE)
	{
		return -10;
	}
	index = FAT_FindFileIndex(handle);
	if(index == -1)
	{
		return -1;	// invalid handle
	}

	if(fileSys == FAT16)
	{
		if((openedRead[index].currentCluster > 0xFFF6)||(openedRead[index].currentCluster < 2))
		{
			return -32768;		// already reached the end of file
		}
	}

	else
	{
		return -32768;		// invalid file system
	}
	sectorToRead = (openedRead[index].currentCluster - 2) * sectorsPerCluster + dataStarts;
	sectorToRead += openedRead[index].sectorCount;
	if(openedRead[index].fileSize < nByte)
	{
		nByte = openedRead[index].fileSize;
	}
	if((openedRead[index].byteCount + nByte) > SECTOR_SIZE)
	{
		if((openedRead[index].sectorCount + 1) == sectorsPerCluster)
		{
			// cross cluster read
			openedRead[index].currentCluster = FAT_GetNextFAT(openedRead[index].currentCluster, g_FATBuf);
			sectorToRead2 = (openedRead[index].currentCluster - 2) * sectorsPerCluster + dataStarts;
			openedRead[index].sectorCount = 0;			
		}
		else
		{
			// cross sector read
			sectorToRead2 = sectorToRead + 1;
			openedRead[index].sectorCount++;
		}
		MMC_ReadPartialMultiSector(sectorToRead,sectorToRead2, openedRead[index].byteCount, nByte, buf);
		openedRead[index].byteCount += nByte;
		openedRead[index].byteCount -= SECTOR_SIZE;
		if(nByte > openedRead[index].fileSize)
			openedRead[index].fileSize = 0;
		else
            openedRead[index].fileSize -= nByte;
		return nByte;
	}
	else
	{
		// single sector read
		MMC_ReadPartialSector(sectorToRead, openedRead[index].byteCount, nByte, buf);
		openedRead[index].byteCount += nByte;
		if(nByte > openedRead[index].fileSize)
			openedRead[index].fileSize = 0;
		else
            openedRead[index].fileSize -= nByte;
		return nByte;
	}
}


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
signed int FAT_Write(signed char handle, unsigned char *buf, unsigned int nByte)
{
	unsigned long sectorToWrite;
	unsigned long sectorToWrite2;
	signed char index;

	if(MMC_DetectCard() == FALSE)
	{
		return -10;
	}

	index = FAT_FindFileIndex(handle);
	if(index == -1)
	{
		return -1;	// invalid handle
	}
	sectorToWrite = (openedWrite[index].currentCluster - 2) * sectorsPerCluster + dataStarts;
	sectorToWrite += openedWrite[index].sectorCount;

	if((openedWrite[index].byteCount + nByte) > SECTOR_SIZE)
	{
		if((openedWrite[index].sectorCount + 1) == sectorsPerCluster)
		{
			// cross cluster write
			sectorToWrite2 = openedWrite[index].currentCluster;
			openedWrite[index].currentCluster = FAT_FindEmptyCluster(g_FATBuf);
			if(openedWrite[index].currentCluster > 0)
			{		
				FAT_UpdateFAT(sectorToWrite2, openedWrite[index].currentCluster, g_FATBuf);
				sectorToWrite2 = (openedWrite[index].currentCluster - 2) * sectorsPerCluster + dataStarts;
				openedWrite[index].sectorCount = 0;						
			}
			else
			{
				return -2;	// memory card full
			}
		}
		else
		{
			// cross sector write
			sectorToWrite2 = sectorToWrite + 1;
			openedWrite[index].sectorCount++;
		}
		MMC_WritePartialMultiSector(sectorToWrite, sectorToWrite2, openedWrite[index].byteCount, nByte, buf);
		openedWrite[index].byteCount += nByte;
		openedWrite[index].byteCount -= SECTOR_SIZE;
		openedWrite[index].fileSize += nByte;
        // Uncomment next line if you want FAT constantly updated, instead of using fat_flush():
		//updateDirectory(openedWrite[index].dirLocation, openedWrite[index].firstCluster, openedWrite[index].fileSize, tempBuf);
        openedWrite[index].updateDir = TRUE;
		return nByte;
	}
	else
	{
		// single sector write
		MMC_WritePartialSector(sectorToWrite, openedWrite[index].byteCount, nByte, buf);
		openedWrite[index].byteCount += nByte;
		openedWrite[index].fileSize += nByte;
        // Uncomment next line if you want FAT constantly updated, instead of using fat_flush():
		//updateDirectory(openedWrite[index].dirLocation, openedWrite[index].firstCluster, openedWrite[index].fileSize, tempBuf);
        openedWrite[index].updateDir = TRUE;
		return nByte;
	}
}


/**
 *	updates the file size in the directory table for all files with the update flag set
 **/
void FAT_Flush(void)
{
	int i;
    
	for(i = 0; i < filesOpenedWrite; i++)
	{
		if(openedWrite[i].updateDir == TRUE)
		{
			FAT_UpdateDirectory(openedWrite[i].dirLocation, openedWrite[i].firstCluster, openedWrite[i].fileSize, g_FATBuf);
			openedWrite[i].updateDir = FALSE;
		}
	}
}


/**
 *	updates the file size and date/time in the directory table of the file identified by the input first cluster.
 *
 *	@param	buf				the buffer to be used to access the MMC/SD card
 *	@param	dirCluster		location of the cluster where the directory table is store
 *	@param	firstCluster	the location of the first cluster of the file being updated
 **/
static void FAT_UpdateDirectory(unsigned long dirCluster, unsigned long firstCluster, unsigned long filesize, unsigned char *buf)
{
	unsigned long sector;
	unsigned char sectorCount = 0;
	unsigned char i;
	unsigned int offset;
	unsigned long cluster;
	boolean done = FALSE;

 	if((dirCluster == 1)&&(fileSys == FAT16))
	{
		sector = rootDirectory;
	}
	else
	{
		sector = (dirCluster - 2) * sectorsPerCluster + dataStarts;		
	}

	while((!done) && (sectorCount < sectorsPerCluster))
	{
		MMC_ReadSector(sector, buf);
		for(i = 0; i < 16; i++)
		{
			offset = i * 32;
			cluster = buf[26+offset];
			cluster |= (unsigned long int)buf[27+offset] << 8;
			
			if(cluster == firstCluster)
			{	
				done = TRUE;

				buf[28+offset] = (filesize) & 0x000000FF;
				buf[29+offset] = (filesize >> 8) & 0x000000FF;
				buf[30+offset] = (filesize >> 16) & 0x000000FF;
				buf[31+offset] = (filesize >> 24) & 0x000000FF;
	
				/** Set access date to March 04, 2005 23:15:18 . **/
                // If a real time clock is available, we could use it.
				buf[offset+22] = (18 >> 1) & 0x1F;  // Seconds.
				buf[offset+22] |= 15 << 5;  // Part of minutes.
				buf[offset+23] = (15 >> 3) & 0x07;  // More of inutes.
				buf[offset+23] |= 23 << 3;  // Hours.
				buf[offset+18] = 04 & 0x1F; // Day.
				buf[offset+18] |= (03 << 5) & 0xE0; // Part of month.
				buf[offset+24] = buf[offset+18];
				buf[offset+19] = (03 >> 3) & 0x01;    // More of month.
				buf[offset+19] |= (((2005 - 1980) & 0xFF) << 1) & 0xFE; // Year.
				buf[offset+25] = buf[offset+19];

				MMC_WriteSector(sector, buf);
			}
		}
		sector++;
		sectorCount++;
	}
}


// *** updateAccessDate() is not called right now.  It should probably be
// *** called when a file is opened for reading, and closed in read mode.
/**
 *	updates the access date of the file specified by the input parameters
 *
 *	@param	buf				the buffer to be used to access the MMC/SD card
 *	@param	dirCluster		location of the cluster where the directory table is store
 *	@param	firstCluster	the location of the first cluster of the file being updated
 **/
static void FAT_UpdateAccessDate(unsigned long dirCluster, unsigned long firstCluster, unsigned char *buf)
{
	unsigned long sector;
	unsigned char sectorCount = 0;
	unsigned char i;
	unsigned int offset;
	unsigned long cluster;
	boolean done = FALSE;

 	if((dirCluster == 1)&&(fileSys == FAT16))
	{
		sector = rootDirectory;
	}
	else
	{
		sector = (dirCluster - 2) * sectorsPerCluster + dataStarts;		
	}

	while((!done) && (sectorCount < sectorsPerCluster))
	{
		MMC_ReadSector(sector, buf);
		for(i = 0; i < 16; i++)
		{
			offset = i * 32;
			cluster = buf[26+offset];
			cluster |= (unsigned long int)buf[27+offset] << 8;
			
			if(cluster == firstCluster)
			{	
				done = TRUE;

				/** set access date to February 03, 2005. **/
                // If a real time clock is available, we could use it.
				buf[offset+18] = 03 & 0x1F; // Day.
				buf[offset+18] |= (02 << 5) & 0xE0; // Part of month.
				buf[offset+19] = (02 >> 3) & 0x01;   // More of month.
				buf[offset+19] |= (((2005 - 1980) & 0xFF) << 1) & 0xFE;
				MMC_WriteSector(sector, buf);
			}
		}
		sector++;
		sectorCount++;
	}
}

#endif //__FILE_SAVE_ENABLED__