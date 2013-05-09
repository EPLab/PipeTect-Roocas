//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//					        FAT16/32 File IO Library
//								    V2.6
// 	  							 Rob Riglar
//						    Copyright 2003 - 2010
//
//   					  Email: rob@robriglar.com
//
//								License: GPL
//   If you would like a version with a more permissive license for use in
//   closed source commercial applications please contact me for details.
//-----------------------------------------------------------------------------
//
// This file is part of FAT File IO Library.
//
// FAT File IO Library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// FAT File IO Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FAT File IO Library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "fat_defs.h"
#include "fat_access.h"
#include "fat_table.h"
#include "fat_write.h"
#include "fat_string.h"
#include "fat_misc.h"

#ifdef FATFS_INC_WRITE_SUPPORT

typedef enum{
	FATFS_ADD_FREE_SPACE_STAGE_0 = 0,
	FATFS_ADD_FREE_SPACE_STAGE_1_0,
	FATFS_ADD_FREE_SPACE_STAGE_1_1,
	FATFS_ADD_FREE_SPACE_STAGE_2,
	FATFS_ADD_FREE_SPACE_STAGE_3,
	FATFS_ADD_FREE_SPACE_STAGE_4
} fatfs_add_free_space_stage_t;

//typedef struct _fatfs_add_free_space_status{
//	UINT32 nextcluster;
//} fatfs_add_free_space_status_t;

//fatfs_add_free_space_status_t current_add_free_space_status;
fatfs_add_free_space_stage_t current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_add_free_space: Allocate another cluster of free space to the end
// of a files cluster chain.
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if succeeded
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_add_free_space(struct fatfs* __fs, UINT32* __startCluster, unsigned char __isChunkRequired)
{
	//UINT32 nextcluster;
	static struct fatfs* fs;
	//static UINT32* startCluster;
	static unsigned char isChunkRequired;
	static int clusterCount;
	static UINT32* currentCluster;
	static UINT32 nextCluster;
	static UINT32 lastCluster;
	
	switch (current_add_free_space_stage)
	{
	case FATFS_ADD_FREE_SPACE_STAGE_0:
		fs = __fs;
		//startCluster = __startCluster;
		isChunkRequired = __isChunkRequired;
		clusterCount = 0;
		currentCluster = __startCluster;
		// Set the next free cluster hint to unknown
		if (fs->next_free_cluster != FAT32_LAST_CLUSTER)
		{
			switch (fatfs_set_fs_info_next_free_cluster(fs, FAT32_LAST_CLUSTER))
			{
			case 0:
				return 0;
			case -1:
				return -1;
			}
		}
		if (isChunkRequired)
		{
			clusterCount = 128;
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_1_1;
		}
		else
		{
			clusterCount = 1;
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_1_0;
		}
	case FATFS_ADD_FREE_SPACE_STAGE_1_0:
		if (current_add_free_space_stage == FATFS_ADD_FREE_SPACE_STAGE_1_0)
		{
			// Start looking for free clusters from the beginning
			switch (fatfs_find_blank_cluster(fs, fs->rootdir_first_cluster, &nextCluster))
			{
			case 0:
				current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			lastCluster = nextCluster;
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_2;
		}
	case FATFS_ADD_FREE_SPACE_STAGE_1_1:
		if (current_add_free_space_stage == FATFS_ADD_FREE_SPACE_STAGE_1_1)
		{
			switch (fatfs_find_empty_cluster_chunk(fs, &nextCluster))
			{
			case 0:
				current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			lastCluster = nextCluster;
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_2;
		}
	case FATFS_ADD_FREE_SPACE_STAGE_2:
		if (clusterCount == 0)
		{
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
			return 0;	// the request asked for none
		}
		current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_3;
	case FATFS_ADD_FREE_SPACE_STAGE_3:
		switch (fatfs_fat_set_cluster(fs, *currentCluster, nextCluster))
		{
		case 0:
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		if (isChunkRequired)
		{
			--clusterCount;
			if (clusterCount > 0)
			{
				*currentCluster = nextCluster;
				++nextCluster;
				return -1;
			}
		}
		current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_4;
	case FATFS_ADD_FREE_SPACE_STAGE_4:
		// Point this to end of file
		switch (fatfs_fat_set_cluster(fs, nextCluster, FAT32_LAST_CLUSTER))
		{
		case 0:
			current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// Adjust argument reference
		*currentCluster = lastCluster;
		current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
		return 1;
	}
	current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
	return 0;
}

typedef struct _fatfs_allocate_space_status{
	UINT32 clusterSize;
	UINT32 clusterCount;
	UINT32 nextcluster;
} fatfs_allocate_space_status_t;

typedef enum{
	FATFS_ALLOCATE_SPACE_STAGE_0 = 0,
	FATFS_ALLOCATE_SPACE_STAGE_1,
	FATFS_ALLOCATE_SPACE_STAGE_2,
	FATFS_ALLOCATE_SPACE_STAGE_3,
	FATFS_ALLOCATE_SPACE_STAGE_4,
	FATFS_ALLOCATE_SPACE_STAGE_5,
	FATFS_ALLOCATE_SPACE_STAGE_6,
	FATFS_ALLOCATE_SPACE_STAGE_7,
	FATFS_ALLOCATE_SPACE_STAGE_8,
	FATFS_ALLOCATE_SPACE_STAGE_9,
	FATFS_ALLOCATE_SPACE_STAGE_10
} fatfs_allocate_space_stage_t;

fatfs_allocate_space_status_t current_allocate_space_status;
fatfs_allocate_space_stage_t current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
//-----------------------------------------------------------------------------
// fatfs_allocate_free_space: Add an ammount of free space to a file either from
// 'startCluster' if newFile = false, or allocating a new start to the chain if
// newFile = true.
// YEB: return 0 if failed
// YEB: return -1 if on proccess
// YEB: return else if succeeded
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_allocate_free_space(struct fatfs* __fs, int __newFile, UINT32* __startCluster, UINT32 __size, unsigned char __isChunkRequired)
{
	static struct fatfs* fs;
	static int newFile;
	static UINT32* startCluster;
	static UINT32 size;
	static unsigned char isChunkRequired;
	UINT32 i;
	//UINT32 clusterSize;
	//UINT32 clusterCount;
	//UINT32 nextcluster;

	switch (current_allocate_space_stage)
	{
	case FATFS_ALLOCATE_SPACE_STAGE_0:
		fs = __fs;
		newFile = __newFile;
		startCluster = __startCluster;
		size = __size;
		isChunkRequired = __isChunkRequired;
		
		if (!isChunkRequired)
		{
			if (size==0)
			{
				return 0;
			}
		}
		// Set the next free cluster hint to unknown
		if (fs->next_free_cluster != FAT32_LAST_CLUSTER)
		{
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_1;
		}
		else
		{
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_2;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_1:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_1)
		{
			// YEB check
			switch (fatfs_set_fs_info_next_free_cluster(fs, FAT32_LAST_CLUSTER))
			{
			case 0:
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_2;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_2:
		if (isChunkRequired)
		{
			current_allocate_space_status.clusterCount = 128;
		}
		else
		{
			// Work out size and clusters
			current_allocate_space_status.clusterSize = fs->sectors_per_cluster * FAT_SECTOR_SIZE;
			current_allocate_space_status.clusterCount = (size / current_allocate_space_status.clusterSize);
		
			// If any left over
			if (size - (current_allocate_space_status.clusterSize * current_allocate_space_status.clusterCount))
			{
				++current_allocate_space_status.clusterCount;
			}
		}
		// Allocated first link in the chain if a new file
		if (newFile)
		{
			if (isChunkRequired)
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_7;
			}
			else
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_3;
			}
		}
		else
		{
			current_allocate_space_status.nextcluster = *startCluster;
			if (isChunkRequired)
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_8;
			}
			else
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_4;
			}
		}
	case FATFS_ALLOCATE_SPACE_STAGE_3:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_3)
		{
			switch (fatfs_find_blank_cluster(fs, fs->rootdir_first_cluster, &current_allocate_space_status.nextcluster))
			{
			case 0:
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			if (current_allocate_space_status.clusterCount == 1)
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_6;
			}
			else
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_4;
			}
		}
	case FATFS_ALLOCATE_SPACE_STAGE_4:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_4)
		{
			if (current_allocate_space_status.clusterCount == 0)
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 1;
			}
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_5;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_5:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_5)
		{
			switch (fatfs_add_free_space(fs, &current_allocate_space_status.nextcluster, isChunkRequired))
			{
			case 0:
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			--current_allocate_space_status.clusterCount;
			if (current_allocate_space_status.clusterCount)
			{
				return -1;
			}
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
			return 1;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_6:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_6)
		{
			switch (fatfs_fat_set_cluster(fs, current_allocate_space_status.nextcluster, FAT32_LAST_CLUSTER))
			{
			case 0:
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			*startCluster = current_allocate_space_status.nextcluster;
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
			return 1;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_7:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_7)
		{
			switch (fatfs_find_empty_cluster_chunk(fs, &current_allocate_space_status.nextcluster))
			{
			case 0:
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_8;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_8:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_8)
		{
			if (current_allocate_space_status.clusterCount == 0)
			{
				current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
				return 1;
			}
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_9;
		}
	case FATFS_ALLOCATE_SPACE_STAGE_9:
		if (current_allocate_space_stage == FATFS_ALLOCATE_SPACE_STAGE_9)
		{
			// Point last to this
			i = fs->empty_chunk_start_cluster_number + 128 - current_allocate_space_status.clusterCount;
			if (((i + 1) % 128) == 0)
			{
				current_allocate_space_status.nextcluster = FAT32_LAST_CLUSTER;
			}
			else
			{
				current_allocate_space_status.nextcluster = i + 1;
			}
			switch (fatfs_fat_set_cluster(fs, i, current_allocate_space_status.nextcluster))
			{
			case 0:
				current_add_free_space_stage = FATFS_ADD_FREE_SPACE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			--current_allocate_space_status.clusterCount;
			if (current_allocate_space_status.clusterCount)
			{
				return -1;
			}
			current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
			*startCluster = fs->empty_chunk_start_cluster_number;
			return 1;
		}
	}
	current_allocate_space_stage = FATFS_ALLOCATE_SPACE_STAGE_0;
	return 0;
}

typedef enum{
	FATFS_FREE_DIR_OFFSET_STAGE_0 = 0,
	FATFS_FREE_DIR_OFFSET_STAGE_1,
	FATFS_FREE_DIR_OFFSET_STAGE_2,
	FATFS_FREE_DIR_OFFSET_STAGE_3,
	FATFS_FREE_DIR_OFFSET_STAGE_4,
	FATFS_FREE_DIR_OFFSET_STAGE_5,
	FATFS_FREE_DIR_OFFSET_STAGE_6,
	FATFS_FREE_DIR_OFFSET_STAGE_7,
	FATFS_FREE_DIR_OFFSET_STAGE_8
} fatfs_free_dir_offset_stage_t;

fatfs_free_dir_offset_stage_t current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_find_free_dir_offset: Find a free space in the directory for a new entry 
// which takes up 'entryCount' blocks (or allocate some more)
// YEB: Returns 0 if failed
// YEB: Returns -1 if on process
// YEB: Returns else if succeeded
// YEB: Comp
//-----------------------------------------------------------------------------
static int fatfs_find_free_dir_offset(struct fatfs* __fs, UINT32 __dirCluster, int __entryCount, UINT32* __pSector, unsigned char* __pOffset)
{
	static struct fatfs* fs;
	static UINT32 dirCluster;
	static int entryCount;
	static UINT32* pSector;
	static unsigned char* pOffset;

	static unsigned char item;
	static UINT16 recordoffset;
	static int currentCount;
	static unsigned char i;
	static int x;
	static UINT32 newCluster;
	static int firstFound;
	
	switch (current_free_dir_offset_stage)
	{
	case FATFS_FREE_DIR_OFFSET_STAGE_0:
		fs = __fs;
		dirCluster = __dirCluster;
		entryCount = __entryCount;
		pSector = __pSector;
		pOffset = __pOffset;
		if (entryCount == 0)
		{
			return 0;
		}
		item = 0;
		recordoffset = 0;
		currentCount = entryCount;
		x = 0;
		firstFound = FALSE;
		current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_1;
	case FATFS_FREE_DIR_OFFSET_STAGE_1:
		switch (fatfs_sector_reader(fs, dirCluster, x, FALSE))
		{
		case 0:
			current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_2;
			break;
		case -1:
			return -1;
		}
		if (current_free_dir_offset_stage == FATFS_FREE_DIR_OFFSET_STAGE_1)
		{
			// Analyse Sector
			for (item = 0; item <= 15; ++item)
			{
				// Create the multiplier for sector access
				recordoffset = (32 * item);
				// If looking for the last used directory entry
				if (firstFound == FALSE)
				{
					if (fs->currentsector.sector[recordoffset]==0x00)
					{
						firstFound = TRUE;
						// Store start
						*pSector = x;
						*pOffset = item;
						currentCount--;
					}
				}
				// Check that there are enough free entries left
				else
				{
					// If everthing fits
					if (currentCount == 0)
					{
						current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;
						return 1;
					}
					else
					{
						--currentCount;
					}
				}
			}
			++x;
			return -1;
		}
	case FATFS_FREE_DIR_OFFSET_STAGE_2:
		if (current_free_dir_offset_stage == FATFS_FREE_DIR_OFFSET_STAGE_2)
		{
			// Get a new cluster for directory
			switch (fatfs_find_blank_cluster(fs, fs->rootdir_first_cluster, &newCluster))
			{
			case 0:
				current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_3;
		}
	case FATFS_FREE_DIR_OFFSET_STAGE_3:
		if (current_free_dir_offset_stage == FATFS_FREE_DIR_OFFSET_STAGE_3)
		{
			// Add cluster to end of directory tree
			switch (fatfs_fat_add_cluster_to_chain(fs, dirCluster, newCluster))
			{
			case 0:
				current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			// Erase new directory cluster
			memset(fs->currentsector.sector, 0x00, FAT_SECTOR_SIZE);
			i = 0;
			if (i < fs->sectors_per_cluster)
			{
				current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_5;
			}
			else
			{
				current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_4;
			}
		}
	case FATFS_FREE_DIR_OFFSET_STAGE_4:
		if (current_free_dir_offset_stage == FATFS_FREE_DIR_OFFSET_STAGE_4)
		{
			switch (fatfs_sector_writer(fs, newCluster, i, FALSE, 0, FALSE))
			{
			case 0:
				current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			if (i < fs->sectors_per_cluster)
			{
				++i;
				return -1;
			}
			
			// If non of the name fitted on previous sectors
			if (firstFound == FALSE) 
			{
				// Store start
				*pSector = x - 1;
				*pOffset = 0;
				firstFound = TRUE;
			}
			current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;
			return 1;
		}
	}
	current_free_dir_offset_stage = FATFS_FREE_DIR_OFFSET_STAGE_0;
	return 0;
}


typedef enum{
	FATFS_ADD_FILE_STAGE_0 = 0,
	FATFS_ADD_FILE_STAGE_1,
	FATFS_ADD_FILE_STAGE_2,
	FATFS_ADD_FILE_STAGE_3,
	FATFS_ADD_FILE_STAGE_4
} fatfs_add_file_stage_t;

typedef struct _fatfs_add_file_entry{
	unsigned char item;
	UINT16 recordoffset;
	unsigned char i;
	UINT32 x;
	int entryCount;
	FAT32_ShortEntry shortEntry;
	int dirtySector;

	UINT32 dirSector;
	unsigned char dirOffset;
	int foundEnd;
	
	unsigned char checksum;
} fatfs_add_file_entry_t;

fatfs_add_file_entry_t current_add_file_entry;
fatfs_add_file_stage_t current_add_file_stage = FATFS_ADD_FILE_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_add_file_entry: Add a directory entry to a location found by FindFreeOffset
// YEB: Returns 0 if failed
// YEB: Returns -1 if on process
// YEB: Returns else if succeeded
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_add_file_entry(struct fatfs* __fs, UINT32 __dirCluster, char* __filename, char* __shortfilename, UINT32 __startCluster, UINT32 __size, int __dir)
{
	static struct fatfs* fs;
	static UINT32 dirCluster;
	static char* filename;
	static char* shortfilename;
	static UINT32 startCluster;
	static UINT32 size;
	static int dir;
	static unsigned char *pSname;
	
	//unsigned char item;
	//UINT16 recordoffset;
	unsigned char i;
	//UINT32 x;
	//int entryCount;
	//FAT32_ShortEntry shortEntry;
	//int dirtySector;

	//UINT32 dirSector;
	//unsigned char dirOffset;
	//int foundEnd;
	int res;
	
	
	switch (current_add_file_stage)
	{
	case FATFS_ADD_FILE_STAGE_0:
		fs = __fs;
		dirCluster = __dirCluster;
		filename = __filename;
		shortfilename = __shortfilename;
		startCluster = __startCluster;
		size = __size;
		dir = __dir;
		
		current_add_file_entry.entryCount = fatfs_lfn_entries_required(filename);
		if (current_add_file_entry.entryCount==0)
		{
			current_add_file_stage = FATFS_ADD_FILE_STAGE_0;
			return 0;
		}
	
		current_add_file_entry.item = 0;
		current_add_file_entry.recordoffset = 0;
		i = 0;
		current_add_file_entry.x = 0;
		current_add_file_entry.dirtySector = FALSE;
		current_add_file_entry.dirSector = 0;
		current_add_file_entry.dirOffset = 0;
		current_add_file_entry.foundEnd = FALSE;
		current_add_file_stage = FATFS_ADD_FILE_STAGE_1;
	case FATFS_ADD_FILE_STAGE_1:
		// Find space in the directory for this filename (or allocate some more)
		switch (fatfs_find_free_dir_offset(fs, dirCluster, current_add_file_entry.entryCount, &current_add_file_entry.dirSector, &current_add_file_entry.dirOffset))
		{
		case 0:
			current_add_file_stage = FATFS_ADD_FILE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// Generate checksum of short filename
		pSname = (unsigned char*)shortfilename;
		current_add_file_entry.checksum = 0;
		for (i = 11; i != 0; i--)
		{
			current_add_file_entry.checksum = ((current_add_file_entry.checksum & 1) ? 0x80 : 0) + (current_add_file_entry.checksum >> 1) + *pSname++;
		}
		current_add_file_stage = FATFS_ADD_FILE_STAGE_2;
	case FATFS_ADD_FILE_STAGE_2:
		// Read sector
		switch (fatfs_sector_reader(fs, dirCluster, current_add_file_entry.x, FALSE))
		{
		case 0:
			current_add_file_stage = FATFS_ADD_FILE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// Analyse Sector
		for (current_add_file_entry.item = 0; current_add_file_entry.item <= 15; current_add_file_entry.item++)
		{
			// Create the multiplier for sector access
			current_add_file_entry.recordoffset = (32 * current_add_file_entry.item);
			// If the start position for the entry has been found
			if (current_add_file_entry.foundEnd==FALSE)
			{
				if ((current_add_file_entry.dirSector == (current_add_file_entry.x)) && (current_add_file_entry.dirOffset == current_add_file_entry.item))
				{
					current_add_file_entry.foundEnd = TRUE;
				}
			}
			// Start adding filename
			if (current_add_file_entry.foundEnd)
			{				
				if (current_add_file_entry.entryCount == 0)
				{
					// Short filename
					fatfs_sfn_create_entry(shortfilename, size, startCluster, &current_add_file_entry.shortEntry, dir);
					memcpy(&fs->currentsector.sector[current_add_file_entry.recordoffset], &current_add_file_entry.shortEntry, sizeof(current_add_file_entry.shortEntry));
					current_add_file_stage = FATFS_ADD_FILE_STAGE_4;
					break;
				}
				else
				{
					current_add_file_entry.entryCount--;
					// Copy entry to directory buffer
					fatfs_filename_to_lfn(filename, &fs->currentsector.sector[current_add_file_entry.recordoffset], current_add_file_entry.entryCount, current_add_file_entry.checksum); 
					current_add_file_entry.dirtySector = TRUE;
				}
			}
		} // End of for
		if (current_add_file_stage == FATFS_ADD_FILE_STAGE_2)
		{
			// Write back to disk before loading another sector
			if (current_add_file_entry.dirtySector)
			{
				current_add_file_stage = FATFS_ADD_FILE_STAGE_3;				
			}
			else
			{
				++current_add_file_entry.x;
				return -1;
			}
		}
	case FATFS_ADD_FILE_STAGE_3:
		if (current_add_file_stage == FATFS_ADD_FILE_STAGE_3)
		{
			// Writeback
			switch (fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector))
			{
			case 0:
				current_add_file_stage = FATFS_ADD_FILE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_add_file_entry.dirtySector = FALSE;
			++current_add_file_entry.x;
			current_add_file_stage = FATFS_ADD_FILE_STAGE_2;
			return -1;
		}
	case FATFS_ADD_FILE_STAGE_4:
		res = fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
		switch (res)
		{
		case 0:
			current_add_file_stage = FATFS_ADD_FILE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_add_file_stage = FATFS_ADD_FILE_STAGE_0;
		return res;
	}
	return 0;
}
#endif
