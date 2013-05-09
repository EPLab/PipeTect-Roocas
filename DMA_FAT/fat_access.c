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

#ifdef FRAM_METADATA_CACHE_ENABLED
#include "fram_wr.h"
#define FILE_ENTRY_SIZE 32
#endif


typedef enum{
	FATFS_INIT_STAGE_0 = 0,
	FATFS_INIT_STAGE_1,
	FATFS_INIT_STAGE_2
} fatfs_init_stage_t;

typedef enum{
	FATFS_GET_FILE_ENTRY_STAGE_0 = 0,
	FATFS_GET_FILE_ENTRY_STAGE_1,
    FATFS_GET_FILE_ENTRY_STAGE_1_1,
    FATFS_GET_FILE_ENTRY_STAGE_1_2,
	FATFS_GET_FILE_ENTRY_STAGE_2
} fatfs_get_file_entry_stage_t;

typedef enum{
	FATFS_UPDATE_FILE_LENGTH_STAGE_0 = 0,
	FATFS_UPDATE_FILE_LENGTH_STAGE_1,
	FATFS_UPDATE_FILE_LENGTH_STAGE_2
} fatfs_update_file_length_stage_t;

typedef enum{
	FATFS_LIST_NEXT_DIR_STAGE_0 = 0,
	FATFS_LIST_NEXT_DIR_STAGE_1,
	FATFS_LIST_NEXT_DIR_STAGE_2
} fatfs_list_next_dir_stage_t;

typedef enum{
	FATFS_MARK_FILE_DELETED_STAGE_0 = 0,
	FATFS_MARK_FILE_DELETED_STAGE_1,
	FATFS_MARK_FILE_DELETED_STAGE_2
} fatfs_mark_file_deleted_stage_t;

typedef struct _fatfs_file_entry{
	unsigned char item;
	UINT16 recordoffset;
	int x;
    int index;
	char *LongFilename;
	char ShortFilename[13];
	unsigned char LFNIndex;
	struct lfn_cache lfn;
	int dotRequired;
	FAT32_ShortEntry *directoryEntry;
} fatfs_file_entry_t;

typedef struct _fatfs_update_file_length_entry{
	unsigned char item;
	UINT16 recordoffset;
	int x;
	FAT32_ShortEntry *directoryEntry;
} fatfs_update_file_length_entry_t;

typedef struct _fatfs_mark_file_deleted_entry{
	unsigned char item;
	UINT16 recordoffset;
	int x;
	FAT32_ShortEntry *directoryEntry;
} fatfs_mark_file_deleted_entry_t;

static fatfs_init_stage_t current_init_stage = FATFS_INIT_STAGE_0;
static fatfs_get_file_entry_stage_t current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
static fatfs_update_file_length_stage_t current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_0;
static fatfs_list_next_dir_stage_t current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
static fatfs_mark_file_deleted_stage_t current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;

static fatfs_file_entry_t working_file_entry;
static fatfs_file_entry_t working_dir_entry;
static fatfs_update_file_length_entry_t current_file_length_entry;
static fatfs_mark_file_deleted_entry_t current_mark_file_deleted;


//-----------------------------------------------------------------------------
// fatfs_init: Load FAT Parameters
// YEB: Returns 0 if failed
// YEB: Returns -1 if on process
// YEB: Returns else if succeeded
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_init(struct fatfs* __fs)
{
	static struct fatfs* fs;
	static unsigned char   num_of_fats;
	static UINT16 reserved_sectors;
	static UINT32 FATSz;
	static UINT32 root_dir_sectors;
	static UINT32 total_sectors;
	static UINT32 data_sectors;
	static UINT32 count_of_clusters;
	//UINT32 partition_size;
	static unsigned char valid_partition;
	int i;

	switch (current_init_stage)
	{
	case FATFS_INIT_STAGE_0:
		fs = __fs;
		//partition_size = 0;
		valid_partition = 0;

		fs->currentsector.address = FAT32_INVALID_CLUSTER;
		fs->currentsector.dirty = 0;

		fs->next_free_cluster = 0; // Invalid

		fatfs_fat_init(fs);

		// Make sure we have read and write functions
		if (!fs->disk_io.read_sector || !fs->disk_io.write_sector)
		{
			return FAT_INIT_MEDIA_ACCESS_ERROR;
		}
		// MBR: Sector 0 on the disk
		// NOTE: Some removeable media does not have this.

		// Load MBR (LBA 0) into the 512 byte buffer
		//if (!fs->disk_io.read_sector(0, fs->currentsector.sector))
		//	return FAT_INIT_MEDIA_ACCESS_ERROR;
		current_init_stage = FATFS_INIT_STAGE_1;
	case FATFS_INIT_STAGE_1:
		////////////////////////////////////////////////////////////////////////////
		// YEB START
		////////////////////////////////////////////////////////////////////////////
		i = fs->disk_io.read_sector(0, fs->currentsector.sector);
		switch (i)
		{
			case 0:
				current_init_stage = FATFS_INIT_STAGE_0;
				return FAT_INIT_MEDIA_ACCESS_ERROR;
			case -1:
				return FAT_INIT_ON_PROCESS;
		}
		////////////////////////////////////////////////////////////////////////////
		// YEB END
		////////////////////////////////////////////////////////////////////////////
		// Make Sure 0x55 and 0xAA are at end of sector
		// (this should be the case regardless of the MBR or boot sector)
		if (fs->currentsector.sector[SIGNATURE_POSITION] != 0x55 || fs->currentsector.sector[SIGNATURE_POSITION+1] != 0xAA)
		{
			current_init_stage = FATFS_INIT_STAGE_0;
			return FAT_INIT_INVALID_SIGNATURE;
		}
		// Now check again using the access function to prove endian conversion function
		if (GET_16BIT_WORD(fs->currentsector.sector, SIGNATURE_POSITION) != SIGNATURE_VALUE) 
		{
			current_init_stage = FATFS_INIT_STAGE_0;
			return FAT_INIT_ENDIAN_ERROR;
		}
		// Check the partition type code
		switch(fs->currentsector.sector[PARTITION1_TYPECODE_LOCATION])
		{
			case 0x0B: 
			case 0x06: 
			case 0x0C: 
			case 0x0E: 
			case 0x0F: 
			case 0x05: 
				valid_partition = 1;
			break;
			case 0x00:
				valid_partition = 0;
				break;
			default:
				if (fs->currentsector.sector[PARTITION1_TYPECODE_LOCATION] <= 0x06)
				{
					valid_partition = 1;
				}
			break;
		}

		if (valid_partition)
		{
			// Read LBA Begin for the file system
			fs->lba_begin = GET_32BIT_WORD(fs->currentsector.sector, PARTITION1_LBA_BEGIN_LOCATION);
			//partition_size = GET_32BIT_WORD(fs->currentsector.sector, PARTITION1_SIZE_LOCATION);
		}
		// Else possibly MBR less disk
		else
		{
			fs->lba_begin = 0;
		}
		
		// Load Volume 1 table into sector buffer
		// (We may already have this in the buffer if MBR less drive!)
		//if (!fs->disk_io.read_sector(fs->lba_begin, fs->currentsector.sector))
		//	return FAT_INIT_MEDIA_ACCESS_ERROR;
		current_init_stage = FATFS_INIT_STAGE_2;
	case FATFS_INIT_STAGE_2:
		////////////////////////////////////////////////////////////////////////////
		// YEB START
		////////////////////////////////////////////////////////////////////////////
		i = fs->disk_io.read_sector(fs->lba_begin, fs->currentsector.sector);
		switch (i)
		{
			case 0:
				current_init_stage = FATFS_INIT_STAGE_0;
				return FAT_INIT_MEDIA_ACCESS_ERROR;
			case -1:
				return FAT_INIT_ON_PROCESS;
		}
		////////////////////////////////////////////////////////////////////////////
		// YEB END
		////////////////////////////////////////////////////////////////////////////
		
		// Make sure there are 512 bytes per cluster
		if (GET_16BIT_WORD(fs->currentsector.sector, 0x0B) != FAT_SECTOR_SIZE) 
		{
			return FAT_INIT_INVALID_SECTOR_SIZE;
		}
		// Load Parameters of FAT partition	 
		fs->sectors_per_cluster = fs->currentsector.sector[BPB_SECPERCLUS];
		reserved_sectors = GET_16BIT_WORD(fs->currentsector.sector, BPB_RSVDSECCNT);
		num_of_fats = fs->currentsector.sector[BPB_NUMFATS];
		fs->root_entry_count = GET_16BIT_WORD(fs->currentsector.sector, BPB_ROOTENTCNT);

		if(GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16) != 0)
		{
			fs->fat_sectors = GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16);
		}
		else
		{
			fs->fat_sectors = GET_32BIT_WORD(fs->currentsector.sector, BPB_FAT32_FATSZ32);
		}
		// For FAT32 (which this may be)
		fs->rootdir_first_cluster = GET_32BIT_WORD(fs->currentsector.sector, BPB_FAT32_ROOTCLUS);
		fs->fs_info_sector = GET_16BIT_WORD(fs->currentsector.sector, BPB_FAT32_FSINFO);

		// For FAT16 (which this may be), rootdir_first_cluster is actuall rootdir_first_sector
		fs->rootdir_first_sector = reserved_sectors + (num_of_fats * fs->fat_sectors);
		fs->rootdir_sectors = ((fs->root_entry_count * 32) + (FAT_SECTOR_SIZE - 1)) / FAT_SECTOR_SIZE;

		// First FAT LBA address
		fs->fat_begin_lba = fs->lba_begin + reserved_sectors;

		// The address of the first data cluster on this volume
		fs->cluster_begin_lba = fs->fat_begin_lba + (num_of_fats * fs->fat_sectors);

		if (GET_16BIT_WORD(fs->currentsector.sector, 0x1FE) != 0xAA55) // This signature should be AA55
		{
			return FAT_INIT_INVALID_SIGNATURE;
		}
		// Calculate the root dir sectors
		root_dir_sectors = ((GET_16BIT_WORD(fs->currentsector.sector, BPB_ROOTENTCNT) * 32) + (GET_16BIT_WORD(fs->currentsector.sector, BPB_BYTSPERSEC) - 1)) / GET_16BIT_WORD(fs->currentsector.sector, BPB_BYTSPERSEC);
		
		if(GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16) != 0)
		{
			FATSz = GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16);
		}
		else
		{
			FATSz = GET_32BIT_WORD(fs->currentsector.sector, BPB_FAT32_FATSZ32);  
		}
		if(GET_16BIT_WORD(fs->currentsector.sector, BPB_TOTSEC16) != 0)
		{
			total_sectors = GET_16BIT_WORD(fs->currentsector.sector, BPB_TOTSEC16);
		}
		else
		{
			total_sectors = GET_32BIT_WORD(fs->currentsector.sector, BPB_TOTSEC32);
		}
		data_sectors = total_sectors - (GET_16BIT_WORD(fs->currentsector.sector, BPB_RSVDSECCNT) + (fs->currentsector.sector[BPB_NUMFATS] * FATSz) + root_dir_sectors);

		// Find out which version of FAT this is...
		if (fs->sectors_per_cluster != 0)
		{
			count_of_clusters = data_sectors / fs->sectors_per_cluster;

			if(count_of_clusters < 4085)
			{
				// Volume is FAT12 
				current_init_stage = FATFS_INIT_STAGE_0;
				return FAT_INIT_WRONG_FILESYS_TYPE;
			}
			else if(count_of_clusters < 65525) 
			{
				// Clear this FAT32 specific param
				fs->rootdir_first_cluster = 0;
				current_init_stage = FATFS_INIT_STAGE_0;
				// Volume is FAT16
				fs->fat_type = FAT_TYPE_16;
				return FAT_INIT_OK;
			}
			else
			{
				// Volume is FAT32
				fs->fat_type = FAT_TYPE_32;
				current_init_stage = FATFS_INIT_STAGE_0;
				return FAT_INIT_OK;
			}
		}
		else
		{
			current_init_stage = FATFS_INIT_STAGE_0;
			return FAT_INIT_WRONG_FILESYS_TYPE;
		}
	}
	current_init_stage = FATFS_INIT_STAGE_0;
	return FAT_INIT_MEDIA_ACCESS_ERROR; 
}
//-----------------------------------------------------------------------------
// fatfs_lba_of_cluster: This function converts a cluster number into a sector / 
// LBA number.
//-----------------------------------------------------------------------------
UINT32 fatfs_lba_of_cluster(struct fatfs *fs, UINT32 Cluster_Number)
{
	if (fs->fat_type == FAT_TYPE_16)
	{
		return (fs->cluster_begin_lba + (fs->root_entry_count * 32 / FAT_SECTOR_SIZE) + ((Cluster_Number-2) * fs->sectors_per_cluster));
	}
	else
	{
		return ((fs->cluster_begin_lba + ((Cluster_Number-2)*fs->sectors_per_cluster)));
	}
}

typedef struct _fatfs_sector_reader_status{
	UINT32 SectortoRead;
	UINT32 ClustertoRead;
	UINT32 ClusterChain;
	UINT32 currentSectorCount;
	UINT32 lba;
} fatfs_sector_reader_status_t;

typedef enum{
	FATFS_SECTOR_READER_STAGE_0 = 0,
	FATFS_SECTOR_READER_STAGE_1,
	FATFS_SECTOR_READER_STAGE_2,
	FATFS_SECTOR_READER_STAGE_3,
	FATFS_SECTOR_READER_STAGE_4,
	FATFS_SECTOR_READER_STAGE_5
} fatfs_sector_reader_stage_t;

fatfs_sector_reader_status_t current_sector_reader_status;
fatfs_sector_reader_stage_t current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_sector_reader: From the provided startcluster and sector offset
// Returns True if success, returns False if not (including if read out of range)
// YEB: On Process: -1
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_sector_reader(struct fatfs *__fs, UINT32 __Startcluster, UINT32 __offset, unsigned char* __target)
{
	//UINT32 SectortoRead = 0;
	//UINT32 ClustertoRead = 0;
	//UINT32 ClusterChain = 0;
	static UINT32 i;
	//UINT32 lba;
	UINT32 tempClusterChain;
	static struct fatfs* st_fs;
	static UINT32 st_Startcluster;
	static UINT32 st_offset;
	static unsigned char* st_target;

	switch (current_sector_reader_stage)
	{
	case FATFS_SECTOR_READER_STAGE_0:
		st_fs = __fs;
		st_Startcluster = __Startcluster;
		st_offset = __offset;
		st_target = __target;
		
		current_sector_reader_status.SectortoRead = 0;
		current_sector_reader_status.ClustertoRead = 0;
		current_sector_reader_status.ClusterChain = 0;
		// FAT16 Root directory
		if ((st_fs->fat_type == FAT_TYPE_16) && (st_Startcluster == 0))
		{
			if (st_offset < st_fs->rootdir_sectors)
			{
				current_sector_reader_status.lba = st_fs->lba_begin + st_fs->rootdir_first_sector + st_offset;
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_3;
			}
			else
			{
				return 0;
			}
		}
		// FAT16/32 Other
		else
		{
			// Set start of cluster chain to initial value
			current_sector_reader_status.ClusterChain = st_Startcluster;

			// Find parameters
			current_sector_reader_status.ClustertoRead = st_offset / st_fs->sectors_per_cluster;	  
			current_sector_reader_status.SectortoRead = st_offset - (current_sector_reader_status.ClustertoRead * st_fs->sectors_per_cluster);
			
			//current_sector_reader_status.currentSectorCount = 0;
			i = 0;
			//if (current_sector_reader_status.currentSectorCount < current_sector_reader_status.ClustertoRead)
			//{
			//	current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_1;
			//}
			//else
			//{
			//	current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_2;
			//}
			if (i < current_sector_reader_status.ClustertoRead)
			{
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_1;
			}
			else
			{
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_2;
			}
		}
	case FATFS_SECTOR_READER_STAGE_1:
		if (current_sector_reader_stage == FATFS_SECTOR_READER_STAGE_1)
		// Follow chain to find cluster to read
		{
			// YEB: if we use the cluster chunk reservation scheme then no need to do the followin
			switch (fatfs_find_next_cluster(st_fs, current_sector_reader_status.ClusterChain, &tempClusterChain))
			{
			case 0:
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_sector_reader_status.ClusterChain = tempClusterChain;
			if (current_sector_reader_status.ClusterChain == FAT32_LAST_CLUSTER)
			{
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_2;
				// this means the cluster chunk reservoir is now empty
				// we need to fill up the reservoir with new chunk of clusters
			}
			else
			{
				++i;
				//if (current_sector_reader_status.currentSectorCount < current_sector_reader_status.ClustertoRead)
				if (i < current_sector_reader_status.ClustertoRead)
				{
					//++current_sector_reader_status.currentSectorCount;
					return -1;
				}
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_2;
			}
		}
	case FATFS_SECTOR_READER_STAGE_2:
		if (current_sector_reader_stage == FATFS_SECTOR_READER_STAGE_2)
		{
			// If end of cluster chain then return false
			if (current_sector_reader_status.ClusterChain == FAT32_LAST_CLUSTER)
			{
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
				return 0;
			}
			// Calculate sector address
			current_sector_reader_status.lba = fatfs_lba_of_cluster(st_fs, current_sector_reader_status.ClusterChain) + current_sector_reader_status.SectortoRead;
			current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_3;
		}
	case FATFS_SECTOR_READER_STAGE_3:
		// User provided target array
		if (st_target)
		{
			current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_4;
		}
		// Else read sector if not already loaded
		else if (current_sector_reader_status.lba != st_fs->currentsector.address)
		{
			st_fs->currentsector.address = current_sector_reader_status.lba;
			current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_5;
		}
		else
		{
			current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
			return 1;
		}
	case FATFS_SECTOR_READER_STAGE_4:
		if (current_sector_reader_stage == FATFS_SECTOR_READER_STAGE_4)
		{
			switch (st_fs->disk_io.read_sector(current_sector_reader_status.lba, st_target))
			{
			case 0:
				current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
			return 1;
		}
	case FATFS_SECTOR_READER_STAGE_5:
		// now let's just read one sector (512bytes) -> later byte address should be used
		// need to compare the performance bewteeen byte address and sector access
		// look up the FRAM hash table first
		
		switch (st_fs->disk_io.read_sector(st_fs->currentsector.address, st_fs->currentsector.sector))
		{
		case 0:
			current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
        //printf("%u, %u\n", st_fs->currentsector.address, st_fs->currentsector.sector);
		// meta data should be cached at this point
		// this should happen if current block according to the cache policies
		return 1;
	}
	current_sector_reader_stage = FATFS_SECTOR_READER_STAGE_0;
	return 0;
}

//-----------------------------------------------------------------------------
// fatfs_sector_writer: Write to the provided startcluster and sector offset
// Returns True if success, returns False if not
// YEB: Returns -1 if on process
// YEB: Comp
//-----------------------------------------------------------------------------

#ifdef FATFS_INC_WRITE_SUPPORT
typedef struct _fatfs_sector_writer_status{
	UINT32 SectortoWrite;
	UINT32 ClustertoWrite;
	UINT32 ClusterChain;
	UINT32 LastClusterChain;
	UINT32 currentClusterCount;
	UINT32 lba;
} fatfs_sector_writer_status_t;

typedef enum{
	FATFS_SECTOR_WRITER_STAGE_0 = 0,
	FATFS_SECTOR_WRITER_STAGE_1,
	FATFS_SECTOR_WRITER_STAGE_2,
	FATFS_SECTOR_WRITER_STAGE_3,
	FATFS_SECTOR_WRITER_STAGE_4,
	FATFS_SECTOR_WRITER_STAGE_5,
	FATFS_SECTOR_WRITER_STAGE_6,
	FATFS_SECTOR_WRITER_STAGE_7
} fatfs_sector_writer_stage_t;

fatfs_sector_writer_status_t current_sector_writer_status;
fatfs_sector_writer_stage_t current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;

int fatfs_sector_writer(struct fatfs *__fs, UINT32 __Startcluster, UINT32 __offset, unsigned char *__target, UINT32* __LatestCluster, unsigned char isChunkRequired)
{
 	//UINT32 SectortoWrite = 0;
	//UINT32 ClustertoWrite = 0;
	//UINT32 ClusterChain = 0;
	//UINT32 LastClusterChain = FAT32_INVALID_CLUSTER;
	//UINT32 i;
	//UINT32 lba;
	static struct fatfs* st_fs;
	static UINT32 st_Startcluster;
	static UINT32 st_offset;
	static unsigned char* st_target;
	static UINT32* LatestCluster;	
	UINT32 tempClusterChain;
	int i;
	
	switch (current_sector_writer_stage)
	{
	case FATFS_SECTOR_WRITER_STAGE_0:
		st_fs = __fs;
		st_Startcluster = __Startcluster;
		st_offset = __offset;
		st_target = __target;
		LatestCluster = __LatestCluster;
		
		current_sector_writer_status.SectortoWrite = 0;
		current_sector_writer_status.ClustertoWrite = 0;
		current_sector_writer_status.ClusterChain = 0;
		current_sector_writer_status.LastClusterChain = FAT32_INVALID_CLUSTER;
		
		// FAT16 Root directory
		if (st_fs->fat_type == FAT_TYPE_16 && st_Startcluster == 0)
		{
			// In FAT16 we cannot extend the root dir!
			if (st_offset < st_fs->rootdir_sectors)
			{
				current_sector_writer_status.lba = st_fs->lba_begin + st_fs->rootdir_first_sector + st_offset;
			}
			else
			{
				return 0;
			}
			
			// User target buffer passed in
			if (st_target)
			{
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_6;
				// Write to disk
				//return fs->disk_io.write_sector(lba, target);
			}
			else
			{
				// Calculate write address
				st_fs->currentsector.address = current_sector_writer_status.lba;
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_7;
				// Write to disk
				//return fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
			}
		}
		// FAT16/32 Other
		else
		{
			// Set start of cluster chain to initial value
			if (isChunkRequired)
			{
				if (*LatestCluster)
				{
					current_sector_writer_status.ClusterChain = *LatestCluster;
				}
				else
				{
					current_sector_writer_status.ClusterChain = st_Startcluster;
				}
			}
			else
			{
				current_sector_writer_status.ClusterChain = st_Startcluster;
			}
	
			// Find parameters
			current_sector_writer_status.ClustertoWrite = st_offset / st_fs->sectors_per_cluster;	  
			current_sector_writer_status.SectortoWrite = st_offset - (current_sector_writer_status.ClustertoWrite * st_fs->sectors_per_cluster);
	
			// Follow chain to find cluster to read
			if (isChunkRequired)
			{
				current_sector_writer_status.currentClusterCount = *LatestCluster - st_Startcluster;
			}
			else
			{
				current_sector_writer_status.currentClusterCount = 0;
			}
			if (current_sector_writer_status.currentClusterCount < current_sector_writer_status.ClustertoWrite)
			{
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_1;
			}
			else
			{
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_3;
			}
		}
	case FATFS_SECTOR_WRITER_STAGE_1:
		if (current_sector_writer_stage == FATFS_SECTOR_WRITER_STAGE_1)
		{
			// Find next link in the chain
			current_sector_writer_status.LastClusterChain = current_sector_writer_status.ClusterChain;
			//if ((current_sector_writer_status.ClusterChain % 128) == 126)
			//{
			//	current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_2;
			//}
			current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_2;
		}
	case FATFS_SECTOR_WRITER_STAGE_2:
		if (current_sector_writer_stage == FATFS_SECTOR_WRITER_STAGE_2)
		{
			switch (fatfs_find_next_cluster(st_fs, current_sector_writer_status.ClusterChain, &tempClusterChain))
			{
			case 0:
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_sector_writer_status.ClusterChain = tempClusterChain;
			// Dont keep following a dead end
			if (current_sector_writer_status.ClusterChain == FAT32_LAST_CLUSTER)
			{
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_3;
			}
			else
			{
				++current_sector_writer_status.currentClusterCount;
				if (current_sector_writer_status.currentClusterCount < current_sector_writer_status.ClustertoWrite)
				{
					current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_1;
					return -1;
				}
				else
				{
					current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_3;
					*LatestCluster = current_sector_writer_status.ClusterChain;
					//if (current_sector_writer_status.ClustertoWrite == 127)
					//{
					//	if (current_sector_writer_status.SectortoWrite == 0)
					//	{
					//		current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_3;
					//	}
					//}
				}
			}
		}
	case FATFS_SECTOR_WRITER_STAGE_3:
		if (current_sector_writer_stage == FATFS_SECTOR_WRITER_STAGE_3)
		{
			// If end of cluster chain 
			if (current_sector_writer_status.ClusterChain == FAT32_LAST_CLUSTER) 
			{
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_4;
			}
			else
			{
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_5;
			}
		}
	case FATFS_SECTOR_WRITER_STAGE_4:
		if (current_sector_writer_stage == FATFS_SECTOR_WRITER_STAGE_4)
		{
			// Add another cluster to the last good cluster chain
			switch (fatfs_add_free_space(st_fs, &current_sector_writer_status.LastClusterChain, isChunkRequired))
			{
			case 0:
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_sector_writer_status.ClusterChain = current_sector_writer_status.LastClusterChain;
			*LatestCluster = current_sector_writer_status.ClusterChain;
			current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_5;
		}
	case FATFS_SECTOR_WRITER_STAGE_5:
		// User target buffer passed in
		if (st_target)
		{
			// Calculate write address
			current_sector_writer_status.lba = fatfs_lba_of_cluster(st_fs, current_sector_writer_status.ClusterChain) + current_sector_writer_status.SectortoWrite;
			
			current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_6;
		}
		else
		{
			// Calculate write address
			st_fs->currentsector.address = fatfs_lba_of_cluster(st_fs, current_sector_writer_status.ClusterChain) + current_sector_writer_status.SectortoWrite;
			current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_7;
		}
	case FATFS_SECTOR_WRITER_STAGE_6:
		if (current_sector_writer_stage == FATFS_SECTOR_WRITER_STAGE_6)
		{
			// Write to disk
			i = st_fs->disk_io.write_sector(current_sector_writer_status.lba, st_target);
			switch (i)
			{
			case 0:
				current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
			return i;
		}
	case FATFS_SECTOR_WRITER_STAGE_7:
		// Write to disk
		i = st_fs->disk_io.write_sector(st_fs->currentsector.address, st_fs->currentsector.sector);
		switch (i)
		{
		case 0:
			current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
		return i;
	}
	current_sector_writer_stage = FATFS_SECTOR_WRITER_STAGE_0;
	return 0;
}
#endif
//-----------------------------------------------------------------------------
// fatfs_show_details: Show the details about the filesystem
//-----------------------------------------------------------------------------
void fatfs_show_details(struct fatfs *fs)
{
	FAT_PRINTF(("\r\nCurrent Disc FAT details\r\n------------------------\r\nRoot Dir First Cluster = "));   
	FAT_PRINTF(("0x%x", fs->rootdir_first_cluster));
	FAT_PRINTF(("\r\nFAT Begin LBA = "));
	FAT_PRINTF(("0x%x", fs->fat_begin_lba));
	FAT_PRINTF(("\r\nCluster Begin LBA = "));
	FAT_PRINTF(("0x%x", fs->cluster_begin_lba));
	FAT_PRINTF(("\r\nSectors Per Cluster = "));
	FAT_PRINTF(("%d", fs->sectors_per_cluster));
	FAT_PRINTF(("\r\n\r\nFormula for conversion from Cluster num to LBA is;"));
	FAT_PRINTF(("\r\nLBA = (cluster_begin_lba + ((Cluster_Number-2)*sectors_per_cluster)))\r\n"));
}
//-----------------------------------------------------------------------------
// fatfs_get_root_cluster: Get the root dir cluster
//-----------------------------------------------------------------------------
UINT32 fatfs_get_root_cluster(struct fatfs *fs)
{
	// NOTE: On FAT16 this will be 0 which has a special meaning...
	return fs->rootdir_first_cluster;
}
//-------------------------------------------------------------
// fatfs_get_file_entry: Find the file entry for a filename
// YEB: DMA compatible
// YEB: Returns 0 if failed
// YEB: Returns -1 if on process
// YEB: Returns else if succeeded
// YEB: Comp
//-------------------------------------------------------------
int fatfs_get_file_entry(struct fatfs *__fs, UINT32 __Cluster, char* __nametofind, FAT32_ShortEntry* __sfEntry)
{
	static UINT32 Cluster;
	static char *nametofind;
	static FAT32_ShortEntry* sfEntry;
#ifdef FRAM_METADATA_CACHE_ENABLED
    static unsigned char strFRAMCache[FILE_ENTRY_SIZE];
    static UINT32 oldC;
    UINT32 temp;
#endif
    static struct fatfs *fs;   
	int readSectorResult;
	unsigned char i;
	//fatfs_lfn_cache_init(&lfn, TRUE);
	// Main cluster following loop
	//while (TRUE)
	
	switch (current_get_file_entry_stage)
	{
	case FATFS_GET_FILE_ENTRY_STAGE_0:
		fs = __fs;
		Cluster = __Cluster;
		nametofind = __nametofind;
		sfEntry = __sfEntry;
        
#ifdef FRAM_METADATA_CACHE_ENABLED
        oldC = Cluster;
#endif
        
		working_file_entry.item=0;
		working_file_entry.recordoffset = 0;
		working_file_entry.x=0;
		working_file_entry.dotRequired = 0;
		fatfs_lfn_cache_init(&working_file_entry.lfn, TRUE);
		current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_1;
	case FATFS_GET_FILE_ENTRY_STAGE_1:
		// Read sector
		// if (fatfs_sector_reader(fs, Cluster, x++, FALSE)) // If sector read was successfull
#ifndef FRAM_METADATA_CACHE_ENABLED    
		readSectorResult = fatfs_sector_reader(fs, Cluster, working_file_entry.x, FALSE);
		switch (readSectorResult)
		{
		case 0:
			current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
        // Analyse Sector
		for (working_file_entry.item = 0; working_file_entry.item < 16; working_file_entry.item++)
		{
			// Create the multiplier for sector access
			working_file_entry.recordoffset = (32 * working_file_entry.item);
			// Overlay directory entry over buffer
			working_file_entry.directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector + working_file_entry.recordoffset);
			// Long File Name Text Found
			if (fatfs_entry_lfn_text(working_file_entry.directoryEntry)) 
			{
				fatfs_lfn_cache_entry(&working_file_entry.lfn, fs->currentsector.sector + working_file_entry.recordoffset);
			}
			// If Invalid record found delete any long file name information collated
			else if (fatfs_entry_lfn_invalid(working_file_entry.directoryEntry) ) 
			{
				fatfs_lfn_cache_init(&working_file_entry.lfn, FALSE);
			}
			// Normal SFN Entry and Long text exists 
			else if (fatfs_entry_lfn_exists(&working_file_entry.lfn, working_file_entry.directoryEntry) ) 
			{
				working_file_entry.LongFilename = fatfs_lfn_cache_get(&working_file_entry.lfn);
				// Compare names to see if they match
				if (fatfs_compare_names(working_file_entry.LongFilename, nametofind)) 
				{
					memcpy(sfEntry, working_file_entry.directoryEntry, sizeof(FAT32_ShortEntry));
					current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
					return 1;
				}
				fatfs_lfn_cache_init(&working_file_entry.lfn, FALSE);
			}
			// Normal Entry, only 8.3 Text		 
			else if (fatfs_entry_sfn_only(working_file_entry.directoryEntry) )
			{
				memset(working_file_entry.ShortFilename, 0, sizeof(working_file_entry.ShortFilename));
				// Copy name to string
				for (i = 0; i < 8; i++) 
				{
					working_file_entry.ShortFilename[i] = working_file_entry.directoryEntry->Name[i];
				}
				
				// Extension
				working_file_entry.dotRequired = 0;
				for (i = 8; i < 11; i++) 
				{
					working_file_entry.ShortFilename[i+1] = working_file_entry.directoryEntry->Name[i];
					if (working_file_entry.directoryEntry->Name[i] != ' ')
					{	
						working_file_entry.dotRequired = 1;
					}
				}
				// Dot only required if extension present
				if (working_file_entry.dotRequired)
				{
					// If not . or .. entry
					if (working_file_entry.ShortFilename[0] != '.')
					{
						working_file_entry.ShortFilename[8] = '.';
					}
					else
					{
						working_file_entry.ShortFilename[8] = ' ';
					}
				}
				else
				{
					working_file_entry.ShortFilename[8] = ' ';
				}
				// Compare names to see if they match
				if (fatfs_compare_names(working_file_entry.ShortFilename, nametofind)) 
				{
					memcpy(sfEntry, working_file_entry.directoryEntry, sizeof(FAT32_ShortEntry));
					current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
					return 1;
				}
				fatfs_lfn_cache_init(&working_file_entry.lfn, FALSE);
			}
		}
		++working_file_entry.x;
		return -1;
#else
        /*
        // FAT table check
        if (Cluster == 0)
        {
            Cluster = 2;
        }
        if (fs->fat_type == FAT_TYPE_16)
        {
            working_file_entry.recordoffset = Cluster / 256;
        }
        else
        {
            working_file_entry.recordoffset = Cluster / 128;
        }
        current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_1_1;
    case FATFS_GET_FILE_ENTRY_STAGE_1_1:
        if (current_get_file_entry_stage == FATFS_GET_FILE_ENTRY_STAGE_1_1)
        {
            readSectorResult = FRAM_WR_read(fs->fat_begin_lba + working_file_entry.recordoffset, 0, strFRAMCache, 4, 0);
            switch (readSectorResult)
            {
            case 0:
                current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
                return 0;
            case -1:
                return -1;
            }
            current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_1_2;
        }
    case FATFS_GET_FILE_ENTRY_STAGE_1_2:
        */
        readSectorResult = FRAM_WR_read(fatfs_lba_of_cluster(fs, Cluster) + working_file_entry.x, working_file_entry.recordoffset, strFRAMCache, FILE_ENTRY_SIZE, 0);
        switch (readSectorResult)
        {
        case 0:
            current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
        working_file_entry.directoryEntry = (FAT32_ShortEntry*)strFRAMCache;
        // Long File Name Text Found
	    if (fatfs_entry_lfn_text(working_file_entry.directoryEntry))
        {
            fatfs_lfn_cache_entry(&working_file_entry.lfn, strFRAMCache);
        }
        // If Invalid record found delete any long file name information collated
        else if (fatfs_entry_lfn_invalid(working_file_entry.directoryEntry) ) 
		{
			fatfs_lfn_cache_init(&working_file_entry.lfn, FALSE);
        }
        // Normal SFN Entry and Long text exists 
		else if (fatfs_entry_lfn_exists(&working_file_entry.lfn, working_file_entry.directoryEntry) ) 
		{
            working_file_entry.LongFilename = fatfs_lfn_cache_get(&working_file_entry.lfn);
			// Compare names to see if they match
			if (fatfs_compare_names(working_file_entry.LongFilename, nametofind)) 
			{
                memcpy(sfEntry, working_file_entry.directoryEntry, sizeof(FAT32_ShortEntry));
				current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
				return 1;
			}
            fatfs_lfn_cache_init(&working_file_entry.lfn, FALSE);
        }
        // Normal Entry, only 8.3 Text		 
		else if (fatfs_entry_sfn_only(working_file_entry.directoryEntry) )
		{
			memset(working_file_entry.ShortFilename, 0, sizeof(working_file_entry.ShortFilename));
            // Copy name to string
			for (i = 0; i < 8; i++) 
			{
				working_file_entry.ShortFilename[i] = working_file_entry.directoryEntry->Name[i];
			}
				
			// Extension
			working_file_entry.dotRequired = 0;
			for (i = 8; i < 11; i++) 
			{
				working_file_entry.ShortFilename[i + 1] = working_file_entry.directoryEntry->Name[i];
				if (working_file_entry.directoryEntry->Name[i] != ' ')
				{	
					working_file_entry.dotRequired = 1;
				}
			}
			// Dot only required if extension present
			if (working_file_entry.dotRequired)
			{
				// If not . or .. entry
				if (working_file_entry.ShortFilename[0]!='.')
				{
					working_file_entry.ShortFilename[8] = '.';
				}
				else
				{
					working_file_entry.ShortFilename[8] = ' ';
				}
			}
			else
			{
				working_file_entry.ShortFilename[8] = ' ';
			}
			// Compare names to see if they match
			if (fatfs_compare_names(working_file_entry.ShortFilename, nametofind)) 
			{
				memcpy(sfEntry, working_file_entry.directoryEntry, sizeof(FAT32_ShortEntry));
				current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
				return 1;
			}
			fatfs_lfn_cache_init(&working_file_entry.lfn, FALSE);
        }
        working_file_entry.recordoffset = (working_file_entry.recordoffset + 32) % 512;
        if (working_file_entry.recordoffset == 0)
        {
            ++working_file_entry.x;
            if ((working_file_entry.x % fs->sectors_per_cluster) == 0)
            {
                ++Cluster;
                if (Cluster > oldC) //check to see if new cluster required
                {
                    current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_1_1;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    case FATFS_GET_FILE_ENTRY_STAGE_1_1:
        // only consider FAT32 at this time
        readSectorResult = FRAM_WR_read(fs->fat_begin_lba + (oldC >> 7), (oldC - ((oldC >> 7) << 7)) << 2, strFRAMCache, 4, 0);
        switch (readSectorResult)
        {
        case 0:
            current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
            return 0;
        case -1:
            return -1;
        }
        oldC = Cluster;
        temp = GET_32BIT_WORD(strFRAMCache, 0) & 0x0FFFFFFF;
        if ((temp >= 0x0FFFFFF8) && (temp <= 0x0FFFFFFF))
		{
            //end of cluster chain found
            current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
            return 0;
		}
        return -1;
#endif
	}
	current_get_file_entry_stage = FATFS_GET_FILE_ENTRY_STAGE_0;
	return 0;
}

typedef enum{
	FATFS_CHECK_SNF_STAGE_0 = 0,
	FATFS_CHECK_SNF_STAGE_1,
	FATFS_CHECK_SNF_STAGE_2
} fatfs_check_snf_stage_t;

typedef struct _fatfs_snf_entry{
	unsigned char item;
	UINT16 recordoffset;
	int x;
	FAT32_ShortEntry *directoryEntry;
} fatfs_snf_entry_t;

static fatfs_check_snf_stage_t current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
static fatfs_snf_entry_t current_snf_entry;

//-------------------------------------------------------------
// fatfs_sfn_exists: Check if a short filename exists.
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
// YEB: DMA compatible
// YEB: Comp
//-------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_sfn_exists(struct fatfs* __fs, UINT32 __Cluster, char* __shortname)
{
	static UINT32 Cluster;
	static char* shortname;
	
#ifdef FRAM_METADATA_CACHE_ENABLED
    static unsigned char strFRAMCache[FILE_ENTRY_SIZE];
    int i;
    UINT32 tempCluster;
#endif
    static struct fatfs* fs;
    
	//unsigned char item=0;
	//UINT16 recordoffset = 0;
	//int x=0;
	//FAT32_ShortEntry *directoryEntry;

	// Main cluster following loop
	//while (TRUE)
	switch (current_snf_stage)
	{
	case FATFS_CHECK_SNF_STAGE_0:
		fs = __fs;
		Cluster = __Cluster;
		shortname = __shortname;
		
		current_snf_entry.item = 0;
		current_snf_entry.recordoffset = 0;
		current_snf_entry.x = 0;
		current_snf_stage = FATFS_CHECK_SNF_STAGE_1;
	case FATFS_CHECK_SNF_STAGE_1:
		// Read sector
		//if (fatfs_sector_reader(fs, Cluster, current_snf_entry.x++, FALSE)) // If sector read was successfull
#ifndef FRAM_METADATA_CACHE_ENABLED 
		switch (fatfs_sector_reader(fs, Cluster, current_snf_entry.x, FALSE))
		{
		case 0:
			current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// Analyse Sector
		for (current_snf_entry.item = 0; current_snf_entry.item < 16; current_snf_entry.item++)
		{
			// Create the multiplier for sector access
			current_snf_entry.recordoffset = current_snf_entry.item << 5;
			// Overlay directory entry over buffer
			current_snf_entry.directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector + current_snf_entry.recordoffset);
			// Long File Name Text Found
			if (fatfs_entry_lfn_text(current_snf_entry.directoryEntry) ) 
			{
			}
			// If Invalid record found delete any long file name information collated
			else if (fatfs_entry_lfn_invalid(current_snf_entry.directoryEntry) ) 
			{
			}
			// Normal Entry, only 8.3 Text		 
			else if (fatfs_entry_sfn_only(current_snf_entry.directoryEntry))
			{
				if (strncmp((const char*)(current_snf_entry.directoryEntry->Name), shortname, 11)==0)
				{
					current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
					return 1;
				}
			}
		}
		++current_snf_entry.x;
        return -1;
#else
        i = FRAM_WR_read(fatfs_lba_of_cluster(fs, Cluster) + current_snf_entry.x, current_snf_entry.recordoffset, strFRAMCache, FILE_ENTRY_SIZE, 0);
        switch (i)
        {
        case 0:
            current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
        // Overlay directory entry over buffer
		current_snf_entry.directoryEntry = (FAT32_ShortEntry*)(strFRAMCache);
        // Long File Name Text Found
		if (fatfs_entry_lfn_text(current_snf_entry.directoryEntry) ) 
		{
		}
        // If Invalid record found delete any long file name information collated
		else if (fatfs_entry_lfn_invalid(current_snf_entry.directoryEntry) ) 
		{
		}
		// Normal Entry, only 8.3 Text		 
		else if (fatfs_entry_sfn_only(current_snf_entry.directoryEntry))
		{
			if (strncmp((const char*)(current_snf_entry.directoryEntry->Name), shortname, 11)==0)
			{
				current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
				return 1;
			}
		}
        current_snf_entry.recordoffset = (current_snf_entry.recordoffset + 32) % 512;
        if (current_snf_entry.recordoffset == 0)
        {
		    ++current_snf_entry.x;
            if (current_snf_entry.x == 16)
            {
                current_snf_stage = FATFS_CHECK_SNF_STAGE_2;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    case FATFS_CHECK_SNF_STAGE_2:
        switch (fatfs_find_next_cluster(fs, Cluster, &tempCluster))
        {
        case 0:
            current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
            return 0;
        case -1:
            return -1;
        }
        if (tempCluster == FAT32_LAST_CLUSTER)
        {
            current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
            return 0;
        }
        current_snf_stage = FATFS_CHECK_SNF_STAGE_1;
        return -1;
#endif
	}
	current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
	return 0;
}
#endif
//-------------------------------------------------------------
// fatfs_update_file_length: Find a SFN entry and update it 
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
// YEB: DMA compatible
// YEB: Returns 0 if failed
// YEB: Returns -1 if on process
// YEB: Returns else if succeeded
// YEB: Comp
//-------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_update_file_length(struct fatfs* __fs, UINT32 __Cluster, char* __shortname, UINT32 __fileLength)
{
	static UINT32 Cluster;
	static char* shortname;
	static UINT32 fileLength;
	
	int i;
    
#ifdef FRAM_METADATA_CACHE_ENABLED
    static unsigned char strFRAMCache[FILE_ENTRY_SIZE];
#endif
    static struct fatfs* fs;

    
	//unsigned char item=0;
	//UINT16 recordoffset = 0;
	//int x=0;
	//FAT32_ShortEntry *directoryEntry;

	// Main cluster following loop
	//while (TRUE)
	switch (current_update_file_length_stage)
	{
	case FATFS_UPDATE_FILE_LENGTH_STAGE_0:
		fs = __fs;
		Cluster = __Cluster;
		shortname = __shortname;
		fileLength = __fileLength;
		
		current_file_length_entry.item = 0;
		current_file_length_entry.recordoffset = 0;
		current_file_length_entry.x = 0;
		current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_1;
	case FATFS_UPDATE_FILE_LENGTH_STAGE_1:
		// Read sector
		//if (fatfs_sector_reader(fs, Cluster, x++, FALSE)) // If sector read was successfull
#ifndef FRAM_METADATA_CACHE_ENABLED        
		switch (fatfs_sector_reader(fs, Cluster, current_file_length_entry.x, FALSE))
		{
		case 0:
			current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// Analyse Sector
		for (current_file_length_entry.item = 0; current_file_length_entry.item < 16; current_file_length_entry.item++)
		{
			// Create the multiplier for sector access
			current_file_length_entry.recordoffset = (32 * current_file_length_entry.item);

			// Overlay directory entry over buffer
			current_file_length_entry.directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector + current_file_length_entry.recordoffset);

			// Long File Name Text Found
			if (fatfs_entry_lfn_text(current_file_length_entry.directoryEntry) ) 
			{
			}
			// If Invalid record found delete any long file name information collated
			else if (fatfs_entry_lfn_invalid(current_file_length_entry.directoryEntry) ) 
			{
			}
			// Normal Entry, only 8.3 Text		 
			else if (fatfs_entry_sfn_only(current_file_length_entry.directoryEntry) )
			{
				if (strncmp((const char*)(current_file_length_entry.directoryEntry->Name), shortname, 11)==0)
				{
					current_file_length_entry.directoryEntry->FileSize = fileLength;
					// TODO: Update last write time

					// Update sfn entry
					memcpy((unsigned char*)(fs->currentsector.sector + current_file_length_entry.recordoffset), (unsigned char*)(current_file_length_entry.directoryEntry), sizeof(FAT32_ShortEntry));					
					current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_2;
                    break;
				}
			}
		}
		if (current_update_file_length_stage == FATFS_UPDATE_FILE_LENGTH_STAGE_1)
		{
			++current_file_length_entry.x;
			return -1;
		}
    case FATFS_UPDATE_FILE_LENGTH_STAGE_2:
		i = fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
		if (i > -1)
		{
			current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_0; 
		}
		return i;
#else
        i = FRAM_WR_read(fatfs_lba_of_cluster(fs, Cluster) + current_file_length_entry.x, current_file_length_entry.recordoffset, strFRAMCache, FILE_ENTRY_SIZE, 0);
        switch (i)
        {
        case 0:
            current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
        // Overlay directory entry over buffer
		current_file_length_entry.directoryEntry = (FAT32_ShortEntry*)strFRAMCache;
		// Long File Name Text Found
		if (fatfs_entry_lfn_text(current_file_length_entry.directoryEntry) ) 
		{
		}
		// If Invalid record found delete any long file name information collated
		else if (fatfs_entry_lfn_invalid(current_file_length_entry.directoryEntry) ) 
		{
		}
		// Normal Entry, only 8.3 Text		 
		else if (fatfs_entry_sfn_only(current_file_length_entry.directoryEntry) )
		{
			if (strncmp((const char*)(current_file_length_entry.directoryEntry->Name), shortname, 11)==0)
			{
				current_file_length_entry.directoryEntry->FileSize = fileLength;
				// TODO: Update last write time
				// Update sfn entry
				//memcpy((unsigned char*)(fs->currentsector.sector + current_file_length_entry.recordoffset), (unsigned char*)(current_file_length_entry.directoryEntry), sizeof(FAT32_ShortEntry));					
				memcpy((unsigned char*)strFRAMCache, (unsigned char*)(current_file_length_entry.directoryEntry), sizeof(FAT32_ShortEntry));
                current_snf_stage = FATFS_CHECK_SNF_STAGE_2;
                //current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_2;
			}
		}
        if (current_snf_stage == FATFS_CHECK_SNF_STAGE_1)
        {
            // Create the multiplier for sector access
		    current_file_length_entry.recordoffset = (current_file_length_entry.recordoffset + 32) % 512;
            if (current_file_length_entry.recordoffset == 0)
            {
                ++current_file_length_entry.x;
            }
            return -1;
        }
    case FATFS_UPDATE_FILE_LENGTH_STAGE_2:
        i = FRAM_WR_write((Cluster << 4) + current_file_length_entry.x, current_file_length_entry.recordoffset, strFRAMCache, sizeof(FAT32_ShortEntry), 0);
        switch (i)
        {
        case 0:
            current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
#endif
	} // End of switch
	current_update_file_length_stage = FATFS_UPDATE_FILE_LENGTH_STAGE_0;
	return 0;
}
#endif
//-------------------------------------------------------------
// fatfs_mark_file_deleted: Find a SFN entry and mark if as deleted 
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
// YEB: Return 0 if failed
// YEB: Return -1 if on process
// YEB: Return else if succeed
// YEB: Comp
//-------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_mark_file_deleted(struct fatfs* __fs, UINT32 __Cluster, char* __shortname)
{
	static UINT32 Cluster;
	static char* shortname;
	//unsigned char item=0;
	//UINT16 recordoffset = 0;
	//int x=0;
	//FAT32_ShortEntry *directoryEntry;
	int i;
    
#ifdef FRAM_METADATA_CACHE_ENABLED
    static unsigned char strFRAMCache[FILE_ENTRY_SIZE];
#endif
    static struct fatfs* fs;

	switch (current_mark_file_deleted_stage)
	{
	case FATFS_MARK_FILE_DELETED_STAGE_0:
		fs = __fs;
		Cluster = __Cluster;
		shortname = __shortname;
		
		current_mark_file_deleted.item = 0;
		current_mark_file_deleted.recordoffset = 0;
		current_mark_file_deleted.x = 0;
		current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_1;
	case FATFS_MARK_FILE_DELETED_STAGE_1:
		// Read sector
#ifndef FRAM_METADATA_CACHE_ENABLED
		switch (fatfs_sector_reader(fs, Cluster, current_mark_file_deleted.x, FALSE)) // If sector read was successfull
		{
		case 0:
			current_mark_file_deleted.x = 0;
			current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// Analyse Sector
		for (current_mark_file_deleted.item = 0; current_mark_file_deleted.item < 16; current_mark_file_deleted.item++)
		{
			// Create the multiplier for sector access
			current_mark_file_deleted.recordoffset = (32 * current_mark_file_deleted.item);
			// Overlay directory entry over buffer
			current_mark_file_deleted.directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector + current_mark_file_deleted.recordoffset);
			// Long File Name Text Found
			if (fatfs_entry_lfn_text(current_mark_file_deleted.directoryEntry) ) 
			{
			}
			// If Invalid record found delete any long file name information collated
			else if (fatfs_entry_lfn_invalid(current_mark_file_deleted.directoryEntry)) 
			{
			}
			// Normal Entry, only 8.3 Text		 
			else if (fatfs_entry_sfn_only(current_mark_file_deleted.directoryEntry) )
			{
				if (strncmp((const char *)current_mark_file_deleted.directoryEntry->Name, shortname, 11)==0)
				{
					// Mark as deleted
					current_mark_file_deleted.directoryEntry->Name[0] = 0xE5; 
					// Update sfn entry
					memcpy((unsigned char*)(fs->currentsector.sector + current_mark_file_deleted.recordoffset), (unsigned char*)current_mark_file_deleted.directoryEntry, sizeof(FAT32_ShortEntry));					
					// Write sector back
					current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_2;
				}
			}
		}
		if (current_mark_file_deleted_stage == FATFS_MARK_FILE_DELETED_STAGE_1)
		{
			current_mark_file_deleted.x++;
			return -1;
		}
	case FATFS_MARK_FILE_DELETED_STAGE_2:
		i = fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
		if (i > -1)
		{
			current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;
            return 0;
		}
        
        current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;
		return i;
#else
        i = FRAM_WR_read(fatfs_lba_of_cluster(fs, Cluster) + current_mark_file_deleted.x, current_mark_file_deleted.recordoffset, strFRAMCache, sizeof(FAT32_ShortEntry), 0);
        switch (i)
        {
        case 0:
			current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
        // Overlay directory entry over buffer
		current_mark_file_deleted.directoryEntry = (FAT32_ShortEntry*)(strFRAMCache);
		// Long File Name Text Found
		if (fatfs_entry_lfn_text(current_mark_file_deleted.directoryEntry) ) 
		{
		}
		// If Invalid record found delete any long file name information collated
		else if (fatfs_entry_lfn_invalid(current_mark_file_deleted.directoryEntry)) 
		{
		}
		// Normal Entry, only 8.3 Text		 
		else if (fatfs_entry_sfn_only(current_mark_file_deleted.directoryEntry) )
		{
			if (strncmp((const char *)current_mark_file_deleted.directoryEntry->Name, shortname, 11)==0)
			{
				// Mark as deleted
				current_mark_file_deleted.directoryEntry->Name[0] = 0xE5; 
				// Update sfn entry
				memcpy((unsigned char*)strFRAMCache, (unsigned char*)current_mark_file_deleted.directoryEntry, sizeof(FAT32_ShortEntry));					
				// Write sector back
				current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_2;
            }			
		}
		if (current_mark_file_deleted_stage == FATFS_MARK_FILE_DELETED_STAGE_1)
		{
            // Create the multiplier for sector access
			current_mark_file_deleted.recordoffset = (current_mark_file_deleted.recordoffset + 32) % 512;
            if (current_mark_file_deleted.recordoffset == 0)
            {
			    ++current_mark_file_deleted.x;
            }
			return -1;
		}
	case FATFS_MARK_FILE_DELETED_STAGE_2:
		i = FRAM_WR_write((Cluster << 4) + current_mark_file_deleted.x, current_mark_file_deleted.recordoffset, strFRAMCache, sizeof(FAT32_ShortEntry), 0);
        switch (i)
        {
        case 0:
			current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
		return i;
#endif
	}
	current_mark_file_deleted_stage = FATFS_MARK_FILE_DELETED_STAGE_0;
	return 0;
}
#endif
//-----------------------------------------------------------------------------
// fatfs_list_directory_start: Initialise a directory listing procedure
//-----------------------------------------------------------------------------
void fatfs_list_directory_start(struct fatfs *fs, struct fs_dir_list_status *dirls, UINT32 StartCluster)
{
	dirls->cluster = StartCluster;
	dirls->sector = 0;
	dirls->offset = 0;
}
//-----------------------------------------------------------------------------
// fatfs_list_directory_next: Get the next entry in the directory.
// Returns: 1 = found, 0 = end of listing
// YEB: Returns -1 if on process
//-----------------------------------------------------------------------------
int fatfs_list_directory_next(struct fatfs* __fs, struct fs_dir_list_status* __dirls, struct fs_dir_ent* __entry)
{
	static struct fs_dir_list_status* dirls;
	static struct fs_dir_ent* entry;
	//unsigned char i,item;
	//UINT16 recordoffset;
	//unsigned char LFNIndex=0;
	//UINT32 x=0;
	//FAT32_ShortEntry *directoryEntry;
	//char *LongFilename;
	//char ShortFilename[13];
	//struct lfn_cache lfn;
	//int dotRequired = 0;
	//int result = 0;
	unsigned char i;
    
#ifdef FRAM_METADATA_CACHE_ENABLED
    static unsigned char strFRAMCache[FILE_ENTRY_SIZE];
#endif
    static struct fatfs* fs;
	
	// Initialise LFN cache first
	//fatfs_lfn_cache_init(&working_dir_entry.lfn, FALSE);

	//while (TRUE)
	switch (current_list_next_dir_stage)
	{
	case FATFS_LIST_NEXT_DIR_STAGE_0:
		dirls = __dirls;
		entry = __entry;
		
		//result = 0;
		// Initialise LFN cache first
		fatfs_lfn_cache_init(&working_dir_entry.lfn, FALSE);
#ifdef FRAM_METADATA_CACHE_ENABLED
        working_dir_entry.item = dirls->offset;
#endif
        fs = __fs;
	case FATFS_LIST_NEXT_DIR_STAGE_1:
#ifndef FRAM_METADATA_CACHE_ENABLED 
		i = fatfs_sector_reader(fs, dirls->cluster, dirls->sector, FALSE);
		// If data read OK
		switch (i)
		{
		case 0:
			current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
		working_dir_entry.LFNIndex=0;

		// Maximum of 16 directory entries
		for (working_dir_entry.item = dirls->offset; working_dir_entry.item < 16; working_dir_entry.item++)
		{
			// Increase directory offset 
			working_dir_entry.recordoffset = (32 * working_dir_entry.item);
			// Overlay directory entry over buffer
			working_dir_entry.directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector + working_dir_entry.recordoffset);
		 	// Long File Name Text Found
			if ( fatfs_entry_lfn_text(working_dir_entry.directoryEntry) )   
			{
				fatfs_lfn_cache_entry(&working_dir_entry.lfn, fs->currentsector.sector + working_dir_entry.recordoffset);
			} 
			// If Invalid record found delete any long file name information collated
			else if ( fatfs_entry_lfn_invalid(working_dir_entry.directoryEntry) ) 	
			{
				fatfs_lfn_cache_init(&working_dir_entry.lfn, FALSE);
			}
			// Normal SFN Entry and Long text exists 
			else if (fatfs_entry_lfn_exists(&working_dir_entry.lfn, working_dir_entry.directoryEntry) ) 
			{
				// Get text
				working_dir_entry.LongFilename = fatfs_lfn_cache_get(&working_dir_entry.lfn);
				strncpy(entry->filename, working_dir_entry.LongFilename, FATFS_MAX_LONG_FILENAME - 1);
		 		if (fatfs_entry_is_dir(working_dir_entry.directoryEntry))
				{
					entry->is_dir = 1; 
				}
				else
				{
					entry->is_dir = 0;
				}
				entry->size = working_dir_entry.directoryEntry->FileSize;
				entry->cluster = (((UINT32)(working_dir_entry.directoryEntry->FstClusHI)) << 16) | working_dir_entry.directoryEntry->FstClusLO;

				// Next starting position
				dirls->offset = working_dir_entry.item + 1;
				current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
		 		return 1;
			}			
			// Normal Entry, only 8.3 Text		 
			else if ( fatfs_entry_sfn_only(working_dir_entry.directoryEntry) )
			{
       			fatfs_lfn_cache_init(&working_dir_entry.lfn, FALSE);
				
				memset(working_dir_entry.ShortFilename, 0, sizeof(working_dir_entry.ShortFilename));
				// Copy name to string
				for (i = 0; i < 8; ++i)
				{
					working_dir_entry.ShortFilename[i] = working_dir_entry.directoryEntry->Name[i];
				}
				// Extension
				working_dir_entry.dotRequired = 0;
				for (i = 8; i < 11; ++i)
				{
					working_dir_entry.ShortFilename[i+1] = working_dir_entry.directoryEntry->Name[i];
					if (working_dir_entry.directoryEntry->Name[i] != ' ')
					{
						working_dir_entry.dotRequired = 1;
					}
				}
				// Dot only required if extension present
				if (working_dir_entry.dotRequired)
				{
					// If not . or .. entry
					if (working_dir_entry.ShortFilename[0]!='.')
					{
						working_dir_entry.ShortFilename[8] = '.';
					}
					else
					{
						working_dir_entry.ShortFilename[8] = ' ';
					}
				}
				else
				{
					working_dir_entry.ShortFilename[8] = ' ';
		  		}
				strncpy(entry->filename, working_dir_entry.ShortFilename, FATFS_MAX_LONG_FILENAME - 1);
		 		if (fatfs_entry_is_dir(working_dir_entry.directoryEntry)) 
				{
					entry->is_dir = 1; 
				}
				else
				{
					entry->is_dir = 0;
				}
				entry->size = working_dir_entry.directoryEntry->FileSize;
				entry->cluster = (((UINT32)(working_dir_entry.directoryEntry->FstClusHI)) << 16) | working_dir_entry.directoryEntry->FstClusLO;

				// Next starting position
				dirls->offset = working_dir_entry.item + 1;
				current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
				return 1;
			}
		}// end of for

		// If reached end of the dir move onto next sector
		dirls->sector++;
		dirls->offset = 0;
#else
        i = FRAM_WR_read(fatfs_lba_of_cluster(fs, dirls->cluster) + dirls->sector, current_snf_entry.recordoffset, strFRAMCache, FILE_ENTRY_SIZE, 0);
        switch (i)
        {
        case 0:
            current_snf_stage = FATFS_CHECK_SNF_STAGE_0;
			return 0;
        case -1:
            return -1;
        }
        current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
		working_dir_entry.LFNIndex=0;
        // Overlay directory entry over buffer
		working_dir_entry.directoryEntry = (FAT32_ShortEntry*)(strFRAMCache);
        // Long File Name Text Found
		if ( fatfs_entry_lfn_text(working_dir_entry.directoryEntry) )   
		{
			fatfs_lfn_cache_entry(&working_dir_entry.lfn, strFRAMCache);
		} 
		// If Invalid record found delete any long file name information collated
		else if ( fatfs_entry_lfn_invalid(working_dir_entry.directoryEntry) ) 	
		{
			fatfs_lfn_cache_init(&working_dir_entry.lfn, FALSE);
		}
		// Normal SFN Entry and Long text exists 
		else if (fatfs_entry_lfn_exists(&working_dir_entry.lfn, working_dir_entry.directoryEntry) ) 
		{
			// Get text
			working_dir_entry.LongFilename = fatfs_lfn_cache_get(&working_dir_entry.lfn);
			strncpy(entry->filename, working_dir_entry.LongFilename, FATFS_MAX_LONG_FILENAME - 1);
			if (fatfs_entry_is_dir(working_dir_entry.directoryEntry))
			{
				entry->is_dir = 1; 
			}
			else
			{
				entry->is_dir = 0;
			}
			entry->size = working_dir_entry.directoryEntry->FileSize;
			entry->cluster = (((UINT32)(working_dir_entry.directoryEntry->FstClusHI)) << 16) | working_dir_entry.directoryEntry->FstClusLO;

			// Next starting position
			dirls->offset = ++working_dir_entry.item;
			current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
		 	return 1;
		}			
		// Normal Entry, only 8.3 Text		 
		else if ( fatfs_entry_sfn_only(working_dir_entry.directoryEntry) )
		{
       		fatfs_lfn_cache_init(&working_dir_entry.lfn, FALSE);
				
			memset(working_dir_entry.ShortFilename, 0, sizeof(working_dir_entry.ShortFilename));
			// Copy name to string
			for (i = 0; i < 8; ++i)
			{
				working_dir_entry.ShortFilename[i] = working_dir_entry.directoryEntry->Name[i];
			}
			// Extension
			working_dir_entry.dotRequired = 0;
			for (i = 8; i < 11; ++i)
			{
				working_dir_entry.ShortFilename[i+1] = working_dir_entry.directoryEntry->Name[i];
				if (working_dir_entry.directoryEntry->Name[i] != ' ')
				{
					working_dir_entry.dotRequired = 1;
				}
			}
			// Dot only required if extension present
			if (working_dir_entry.dotRequired)
			{
				// If not . or .. entry
				if (working_dir_entry.ShortFilename[0]!='.')
				{
					working_dir_entry.ShortFilename[8] = '.';
				}
				else
				{
					working_dir_entry.ShortFilename[8] = ' ';
				}
			}
			else
			{
				working_dir_entry.ShortFilename[8] = ' ';
		  	}
			strncpy(entry->filename, working_dir_entry.ShortFilename, FATFS_MAX_LONG_FILENAME - 1);
		 	if (fatfs_entry_is_dir(working_dir_entry.directoryEntry)) 
			{
				entry->is_dir = 1; 
			}
			else
			{
				entry->is_dir = 0;
			}
			entry->size = working_dir_entry.directoryEntry->FileSize;
			entry->cluster = (((UINT32)(working_dir_entry.directoryEntry->FstClusHI)) << 16) | working_dir_entry.directoryEntry->FstClusLO;
			// Next starting position
			++dirls->offset;        // = working_dir_entry.item + 1;
			current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
			return 1;
		}
        dirls->offset = (dirls->offset + 1) % 16;
        if (dirls->offset == 0)
        {
            // If reached end of the dir move onto next sector
		    dirls->sector++;
		}
#endif
		return -1;
	}
	current_list_next_dir_stage = FATFS_LIST_NEXT_DIR_STAGE_0;
	return 0;
}
