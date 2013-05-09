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

#ifdef FRAM_METADATA_CACHE_ENABLED
#include "fram_wr.h"
#endif

#ifndef FAT_BUFFERED_SECTORS
	#define FAT_BUFFERED_SECTORS 1
#endif

#if FAT_BUFFERED_SECTORS < 1
	#error "FAT_BUFFERED_SECTORS must be at least 1"
#endif

//-----------------------------------------------------------------------------
//							FAT Sector Buffer
//-----------------------------------------------------------------------------
#define FAT32_GET_32BIT_WORD(pbuf, location)	( GET_32BIT_WORD(pbuf->sector, location) )
#define FAT32_SET_32BIT_WORD(pbuf, location, value)	{ SET_32BIT_WORD(pbuf->sector, location, value); pbuf->dirty = 1; }
#define FAT16_GET_16BIT_WORD(pbuf, location)	( GET_16BIT_WORD(pbuf->sector, location) )
#define FAT16_SET_16BIT_WORD(pbuf, location, value)	{ SET_16BIT_WORD(pbuf->sector, location, value); pbuf->dirty = 1; }


typedef enum{
	FATFS_PURGE_STAGE_0 = 0,
	FATFS_PURGE_STAGE_1,
	FATFS_PURGE_STAGE_2
} fatfs_purge_stage_t;

typedef enum{
	FATFS_READ_SECTOR_STAGE_0 = 0,
	FATFS_READ_SECTOR_STAGE_1,
	FATFS_READ_SECTOR_STAGE_2,
} fatfs_read_sector_stage_t;

fatfs_purge_stage_t current_purge_stage = FATFS_PURGE_STAGE_0;
fatfs_read_sector_stage_t current_read_sector_stage = FATFS_READ_SECTOR_STAGE_0;
int FATFS_write_count = 0;
int FATFS_read_count = 0;


//-----------------------------------------------------------------------------
// fatfs_fat_init:
//-----------------------------------------------------------------------------
void fatfs_fat_init(struct fatfs *fs)
{
	int i;

	// FAT buffer chain head
	fs->fat_buffer_head = NULL;

	for (i=0;i<FAT_BUFFERED_SECTORS;i++)
	{
		// Initialise buffers to invalid
		fs->fat_buffers[i].address = FAT32_INVALID_CLUSTER;
		fs->fat_buffers[i].dirty = 0;
		memset(fs->fat_buffers[i].sector, 0x00, sizeof(fs->fat_buffers[i].sector));

		// Add to head of queue
		fs->fat_buffers[i].next = fs->fat_buffer_head;
		fs->fat_buffer_head = &fs->fat_buffers[i];
	}
}
//-----------------------------------------------------------------------------
// fatfs_fat_read_sector: Read a FAT sector
// YEB: changed the structure
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns positive integer
// YEB: Comp
//-----------------------------------------------------------------------------
static int fatfs_fat_read_sector(struct fatfs* __fs, UINT32 __sector, struct sector_buffer** __dest)
{
	static struct fatfs* fs;
	static UINT32 sector;
	static struct sector_buffer** dest;
	static struct sector_buffer* current_pcur = NULL;
	
	int i;
	struct sector_buffer* last;
	
	switch (current_read_sector_stage)
	{
	case FATFS_READ_SECTOR_STAGE_0:
		fs = __fs;
		sector = __sector;
		dest = __dest;
		
		last = NULL;
		current_pcur = fs->fat_buffer_head;
		// Itterate through sector buffer list
		while (current_pcur)
		{
			// Sector already in sector list
			if (current_pcur->address == sector)
			{
				break;
			}
			// End of list?
			if (current_pcur->next == NULL)
			{
				// Remove buffer from list
				if (last)
				{
					last->next = NULL;
				}
				// We the first and last buffer in the chain?
				else
				{
					fs->fat_buffer_head = NULL;
				}
			}
			last = current_pcur;
			current_pcur = current_pcur->next;
		}
		
		// We found the sector already in FAT buffer chain
		if (current_pcur)
		{
			// YEB: LRU
			//if (last)
			//{
			//	last->next = current_pcur->next;
			//	current_pcur->next = fs->fat_buffer_head;
			//	fs->fat_buffer_head = current_pcur->next;
			//}
			*dest = current_pcur;
			current_pcur = NULL;
			return 1;
		}
		current_pcur = last;
		// Add to start of sector buffer list (now newest sector)
		current_pcur->next = fs->fat_buffer_head;
		fs->fat_buffer_head = current_pcur;
		
		// Writeback sector if changed
		if (!current_pcur->dirty)
		{
			current_read_sector_stage = FATFS_READ_SECTOR_STAGE_2;
		}
		else
		{
			current_read_sector_stage = FATFS_READ_SECTOR_STAGE_1;
		}
	case FATFS_READ_SECTOR_STAGE_1:
		if (current_read_sector_stage == FATFS_READ_SECTOR_STAGE_1)
		{
#ifdef FRAM_METADATA_CACHE_ENABLED
            switch (FRAM_WR_write(current_pcur->address, 0, current_pcur->sector, FAT_SECTOR_SIZE, 0))
#else
			switch (fs->disk_io.write_sector(current_pcur->address, current_pcur->sector))
#endif
            {
			case 0:
				current_read_sector_stage = FATFS_READ_SECTOR_STAGE_0;
				*dest = NULL;
				return 0;
			case -1:
				return -1;
			}
			++FATFS_write_count;
			//while (fs->disk_io.read_sector(current_pcur->address, current_pcur->sector) == -1);
			current_pcur->dirty = 0;
			current_read_sector_stage = FATFS_READ_SECTOR_STAGE_2;
		}
	case FATFS_READ_SECTOR_STAGE_2:
		current_pcur->address = sector;
#ifdef FRAM_METADATA_CACHE_ENABLED
        i = FRAM_WR_read(current_pcur->address, 0, current_pcur->sector, FAT_SECTOR_SIZE, 0);
#else
		i = fs->disk_io.read_sector(current_pcur->address, current_pcur->sector);
#endif
		switch (i)
		{
		case 0:
			current_pcur->address = FAT32_INVALID_CLUSTER;
			*dest = NULL;
			current_read_sector_stage = FATFS_READ_SECTOR_STAGE_0;
			break;
		case -1:
			break;
		default:
			++FATFS_read_count;
			*dest = current_pcur;
			current_pcur = NULL;
			current_read_sector_stage = FATFS_READ_SECTOR_STAGE_0;
		}
		return i;
	}
	current_read_sector_stage = FATFS_READ_SECTOR_STAGE_0;
	return 0;
}
//-----------------------------------------------------------------------------
// fatfs_fat_purge: Purge 'dirty' FAT sectors to disk
// YEB: returns 0 if failed 
// YEB: returns 1 if succeeded
// YEB: returns -1 if on process
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_fat_purge(struct fatfs* __fs)
{
	static struct fatfs* fs;
	int i;
	static struct sector_buffer* current_pcur = NULL;;
	
	switch (current_purge_stage)
	{
	case FATFS_PURGE_STAGE_0:
		fs = __fs;
		current_pcur = fs->fat_buffer_head;
		if (!current_pcur)
		{
			return 1;
		}
		current_purge_stage = FATFS_PURGE_STAGE_1;
	case FATFS_PURGE_STAGE_1:
		if (current_pcur->dirty)
		{
			i = fs->disk_io.write_sector(current_pcur->address, current_pcur->sector);
			switch (i)
			{
			case 0:
				current_purge_stage = FATFS_PURGE_STAGE_0;
				current_pcur = NULL;
				return 0;
			case -1:
				return -1;
			}
			++FATFS_write_count;
			//while (fs->disk_io.read_sector(current_pcur->address, current_pcur->sector) == -1);
			current_pcur->dirty = 0;
		}
		current_pcur = current_pcur->next;
		if (!current_pcur)
		{
			current_purge_stage = FATFS_PURGE_STAGE_0;
			current_pcur = NULL;
			return 1;
		}
		return -1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
//						General FAT Table Operations
//-----------------------------------------------------------------------------

typedef struct _fatfs_next_cluster{
	UINT32 fat_sector_offset;
	UINT32 position;
	UINT32 nextcluster;
	struct sector_buffer *pbuf;
} fatfs_next_cluster_status_t;

typedef enum{
	FATFS_NEXT_CLUSTER_STAGE_0 = 0,
	FATFS_NEXT_CLUSTER_STAGE_1,
	FATFS_NEXT_CLUSTER_STAGE_2
} fatfs_next_cluster_stage_t;

fatfs_next_cluster_status_t current_next_cluster_status;
fatfs_next_cluster_stage_t current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_find_next_cluster: Return cluster number of next cluster in chain by 
// reading FAT table and traversing it. Return 0xffffffff for end of chain.
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_find_next_cluster(struct fatfs *__fs, UINT32 __current_cluster, UINT32* __next_cluster)
{
	//UINT32 fat_sector_offset, position;
	//UINT32 nextcluster;
	//struct sector_buffer *pbuf;
	static struct fatfs* st_fs;
	static UINT32 st_current_cluster;
	static UINT32* st_next_cluster;
	static struct sector_buffer* tempBuf[1];
	
	switch (current_next_cluster_stage)
	{
	case FATFS_NEXT_CLUSTER_STAGE_0:
		st_fs = __fs;
		st_current_cluster = __current_cluster;
		st_next_cluster = __next_cluster;
		
		// Why is '..' labelled with cluster 0 when it should be 2 ??
		if (st_current_cluster == 0) 
		{
			st_current_cluster = 2;
		}
		// Find which sector of FAT table to read
		if (st_fs->fat_type == FAT_TYPE_16)
		{
			current_next_cluster_status.fat_sector_offset = st_current_cluster / 256;
		}
		else
		{
			current_next_cluster_status.fat_sector_offset = st_current_cluster / 128;
		}
		// Read FAT sector into buffer
		// pbuf = fatfs_fat_read_sector(fs, fs->fat_begin_lba+fat_sector_offset);
		// YEB modified change it to if
		current_next_cluster_status.pbuf = NULL;
		current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_1;
	case FATFS_NEXT_CLUSTER_STAGE_1:
		switch (fatfs_fat_read_sector(st_fs, st_fs->fat_begin_lba + current_next_cluster_status.fat_sector_offset, tempBuf))
		{
		case 0:
			current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;
			*st_next_cluster = FAT32_LAST_CLUSTER;
			return 0;
		case -1:
			return -1;
		}
		current_next_cluster_status.pbuf = tempBuf[0];
		if (!current_next_cluster_status.pbuf)
		{
			current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;
			*st_next_cluster = FAT32_LAST_CLUSTER;
			return 1;
		}
		if (st_fs->fat_type == FAT_TYPE_16)
		{
			// Find 32 bit entry of current sector relating to cluster number 
			current_next_cluster_status.position = (st_current_cluster - (current_next_cluster_status.fat_sector_offset * 256)) * 2; 

			// Read Next Clusters value from Sector Buffer
			current_next_cluster_status.nextcluster = FAT16_GET_16BIT_WORD(current_next_cluster_status.pbuf, (UINT16)current_next_cluster_status.position);	 

			// If end of chain found
			if ((current_next_cluster_status.nextcluster >= 0xFFF8) && (current_next_cluster_status.nextcluster <= 0xFFFF))
			{
				current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;
				*st_next_cluster = FAT32_LAST_CLUSTER;
				return 1;
			}
		}				
		else
		{
			// Find 32 bit entry of current sector relating to cluster number 
			current_next_cluster_status.position = (st_current_cluster - (current_next_cluster_status.fat_sector_offset * 128)) * 4; 

			// Read Next Clusters value from Sector Buffer
			current_next_cluster_status.nextcluster = FAT32_GET_32BIT_WORD(current_next_cluster_status.pbuf, (UINT16)current_next_cluster_status.position);	 

			// Mask out MS 4 bits (its 28bit addressing)
			current_next_cluster_status.nextcluster = current_next_cluster_status.nextcluster & 0x0FFFFFFF;		

			// If end of chain found
			if ((current_next_cluster_status.nextcluster >= 0x0FFFFFF8) && (current_next_cluster_status.nextcluster <= 0x0FFFFFFF) )
			{
				current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;
				*st_next_cluster = FAT32_LAST_CLUSTER;
				return 1;
			}
		}
		current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;
		*st_next_cluster = current_next_cluster_status.nextcluster;
		return 1;
	}
	current_next_cluster_stage = FATFS_NEXT_CLUSTER_STAGE_0;
	*st_next_cluster = FAT32_LAST_CLUSTER;
	return 0;		 
}

typedef enum{
	FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0 = 0,
	FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_1,
	FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_2
} fatfs_set_fs_info_next_free_cluster_stage_t;

fatfs_set_fs_info_next_free_cluster_stage_t current_sfinfc_stage = FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_set_fs_info_next_free_cluster: Write the next free cluster to the FSINFO table
// YEB: Working
// Comp
//-----------------------------------------------------------------------------
int fatfs_set_fs_info_next_free_cluster(struct fatfs* __fs, UINT32 __newValue)
{
	static struct fatfs* fs;
	static UINT32 newValue;
	static struct sector_buffer* pbuf;
	static struct sector_buffer* tempBuf[1];
	
	switch (current_sfinfc_stage)
	{
	case FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0:
		fs = __fs;
		newValue = __newValue;
		
		// Load sector to change it
		if (fs->fat_type == FAT_TYPE_16)
		{	
			break;
		}
		else
		{
			current_sfinfc_stage = FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_1;
		}
	case FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_1:
		switch (fatfs_fat_read_sector(fs, fs->lba_begin + fs->fs_info_sector, tempBuf))
		{
		case 0:
			current_sfinfc_stage = FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		pbuf = tempBuf[0];
		if (!pbuf)
		{
			current_sfinfc_stage = FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0;
			return 0;
		}
		// Change 
		FAT32_SET_32BIT_WORD(pbuf, 492, newValue);
		fs->next_free_cluster = newValue;
		current_sfinfc_stage = FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0;
		return 1;
	}
	current_sfinfc_stage = FATFS_SET_FS_INFO_NEXT_FREE_CLUSTER_STAGE_0;
	return 0;
}

typedef struct _fatfs_find_blank_cluster_status{
	UINT32 fat_sector_offset, position;
	UINT32 nextcluster;
	UINT32 current_cluster;
	struct sector_buffer *pbuf;
} fatfs_find_blank_cluster_status_t;

typedef enum{
	FATFS_FIND_BLANK_CLUSTER_STAGE_0 = 0,
	FATFS_FIND_BLANK_CLUSTER_STAGE_1,
	FATFS_FIND_BLANK_CLUSTER_STAGE_2,
	FATFS_FIND_BLANK_CLUSTER_STAGE_3
} fatfs_find_blank_cluster_stage_t;

fatfs_find_blank_cluster_status_t current_find_blank_cluster_status;
fatfs_find_blank_cluster_stage_t current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_find_blank_cluster: Find a free cluster entry by reading the FAT
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// Comp
//-----------------------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_find_blank_cluster(struct fatfs* __fs, UINT32 __start_cluster, UINT32* __free_cluster)
{
	//UINT32 fat_sector_offset, position;
	//UINT32 nextcluster;
	//UINT32 current_cluster = start_cluster;
	//struct sector_buffer *pbuf;
	static struct fatfs* fs;
	static UINT32 start_cluster;
	static UINT32 *free_cluster;
	static struct sector_buffer* tempBuf[1];

	switch (current_find_blank_cluster_stage)
	{
	case FATFS_FIND_BLANK_CLUSTER_STAGE_0:
		fs = __fs;
		start_cluster = __start_cluster;
		free_cluster = __free_cluster;
		
		current_find_blank_cluster_status.current_cluster = start_cluster;
		
		current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_1;
	case FATFS_FIND_BLANK_CLUSTER_STAGE_1:
		// Find which sector of FAT table to read
		if (fs->fat_type == FAT_TYPE_16)
		{
			current_find_blank_cluster_status.fat_sector_offset = current_find_blank_cluster_status.current_cluster / 256;
		}
		else
		{
			current_find_blank_cluster_status.fat_sector_offset = current_find_blank_cluster_status.current_cluster / 128;
		}
		if (current_find_blank_cluster_status.fat_sector_offset < fs->fat_sectors)
		{
			current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_2;
		}
		else
		{
			current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_0;
			return 0;
		}
	case FATFS_FIND_BLANK_CLUSTER_STAGE_2:
		// Read FAT sector into buffer
		switch (fatfs_fat_read_sector(fs, fs->fat_begin_lba + current_find_blank_cluster_status.fat_sector_offset, tempBuf))
		{
		case 0:
			current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_find_blank_cluster_status.pbuf = tempBuf[0];
		if (!current_find_blank_cluster_status.pbuf)
		{
			current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_0;
			return 0;
		}
		if (fs->fat_type == FAT_TYPE_16)
		{
			// Find 32 bit entry of current sector relating to cluster number 
			current_find_blank_cluster_status.position = (current_find_blank_cluster_status.current_cluster - (current_find_blank_cluster_status.fat_sector_offset * 256)) * 2; 
			// Read Next Clusters value from Sector Buffer
			current_find_blank_cluster_status.nextcluster = FAT16_GET_16BIT_WORD(current_find_blank_cluster_status.pbuf, (UINT16)current_find_blank_cluster_status.position);	 
		}
		else
		{
			// Find 32 bit entry of current sector relating to cluster number 
			current_find_blank_cluster_status.position = (current_find_blank_cluster_status.current_cluster - (current_find_blank_cluster_status.fat_sector_offset * 128)) * 4; 
			// Read Next Clusters value from Sector Buffer
			current_find_blank_cluster_status.nextcluster = FAT32_GET_32BIT_WORD(current_find_blank_cluster_status.pbuf, (UINT16)current_find_blank_cluster_status.position);	 
			// Mask out MS 4 bits (its 28bit addressing)
			current_find_blank_cluster_status.nextcluster = current_find_blank_cluster_status.nextcluster & 0x0FFFFFFF;		
		}
		if (current_find_blank_cluster_status.nextcluster != 0)
		{
			++current_find_blank_cluster_status.current_cluster;
			current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_1;
			return -1;
		}
		// Found blank entry
		*free_cluster = current_find_blank_cluster_status.current_cluster;
		current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_0;
		return 1;
	}
	current_find_blank_cluster_stage = FATFS_FIND_BLANK_CLUSTER_STAGE_0;
	return 0;
}

typedef enum{
	FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0 = 0,
	FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0_1,
	FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0_2,
	FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_1,
	FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_2,
	FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_3
} fatfs_find_empty_cluster_chunk_stage_t;

fatfs_find_empty_cluster_chunk_stage_t current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
UINT32 current_free_cluster_chunk_sector = 0;

////////////////////////////////////////////////////////////////////////////////
// YEB
////////////////////////////////////////////////////////////////////////////////
int fatfs_find_empty_cluster_chunk(struct fatfs* __fs, /*UINT32 __start_cluster,*/ UINT32* __free_cluster)
{
	static struct fatfs* fs;
	static UINT32 fat_sector_offset;
	static UINT32 current_cluster;
	static struct sector_buffer* tempBuf[1];
	static UINT32* free_cluster;
	int i;
	
	switch (current_fecc_stage)
	{
	case FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0:
		fs = __fs;
		free_cluster = __free_cluster;
		current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0_1;
	case FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0_1:
		if (fs->next_free_cluster != FAT32_LAST_CLUSTER)
		{
			switch (fatfs_set_fs_info_next_free_cluster(fs, FAT32_LAST_CLUSTER))
			{
			case 0:
				current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0_2;
		}
	case FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0_2:
		// accessed for the first time
		if (fs->empty_chunk_start_cluster_number == 0)
		{
			fs->empty_chunk_start_cluster_number = fs->rootdir_first_cluster;
		}
		else
		{
			fs->empty_chunk_start_cluster_number += 128;
		}
		current_cluster = fs->empty_chunk_start_cluster_number;	//__start_cluster;
		
		// Find which sector of FAT table to read
		if (fs->fat_type == FAT_TYPE_16)
		{
			// implement later
			current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
			return 0;
		}
		else
		{
			fat_sector_offset = current_cluster / 128;
			current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_1;
		}
	case FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_1:
		// Read FAT sector into buffer
		switch (fatfs_fat_read_sector(fs, fs->fat_begin_lba + fat_sector_offset, tempBuf))
		{
		case 0:
			current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		if (!tempBuf[0])
		{
			current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
			return 0;
		}
		if (fs->fat_type == FAT_TYPE_16)
		{
			// Find 32 bit entry of current sector relating to cluster number 
			//current_find_blank_cluster_status.position = (current_find_blank_cluster_status.current_cluster - (current_find_blank_cluster_status.fat_sector_offset * 256)) * 2; 
			// Read Next Clusters value from Sector Buffer
			//current_find_blank_cluster_status.nextcluster = FAT16_GET_16BIT_WORD(current_find_blank_cluster_status.pbuf, (UINT16)current_find_blank_cluster_status.position);	 
			current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
			return 0;
		}
		else
		{
			for (i = 0; i < FAT_SECTOR_SIZE; ++i)
			{
				if (tempBuf[0]->sector[i])
				{
					break;
				}
			}
			if (i < FAT_SECTOR_SIZE)
			{
				++fat_sector_offset;
				return -1;
			}
		}
		// Found blank entry
		current_free_cluster_chunk_sector = fat_sector_offset;
		*free_cluster = fat_sector_offset * 128;
		fs->empty_chunk_start_cluster_number = *free_cluster;
		current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
		return 1;
	}
	current_fecc_stage = FATFS_FIND_EMPTY_CLUSTER_CHUNK_STAGE_0;
	return 0;
}
#endif

//-----------------------------------------------------------------------------
// fatfs_fat_set_cluster: Set a cluster link in the chain. NOTE: Immediate
// write (slow).
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
//-----------------------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
typedef struct _fatfs_fat_set_cluster_status{
	struct sector_buffer *pbuf;
	UINT32 fat_sector_offset;
	UINT32 position;
} fatfs_set_cluster_status_t;

typedef enum{
	FATFS_SET_CLUSTER_STAGE_0 = 0,
	FATFS_SET_CLUSTER_STAGE_1,
	FATFS_SET_CLUSTER_STAGE_2,
} fatfs_set_cluster_stage_t;

fatfs_set_cluster_status_t current_set_cluster_status;
fatfs_set_cluster_stage_t current_set_cluster_stage = FATFS_SET_CLUSTER_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_fat_set_cluster
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_fat_set_cluster(struct fatfs* __fs, UINT32 __cluster, UINT32 __next_cluster
#ifdef __CLUSTER_CHUNK_RESERVATION_ENABLED__
						  , BOOL isChunkRequired)
#else
						 )
#endif

{
	//struct sector_buffer *pbuf;
	//UINT32 fat_sector_offset, position;
	static struct fatfs* fs;
	static UINT32 cluster;
	static UINT32 next_cluster;
	static struct sector_buffer* tempBuf[1];
	
	switch (current_set_cluster_stage)
	{
	case FATFS_SET_CLUSTER_STAGE_0:
		fs = __fs;
		cluster = __cluster;
		next_cluster = __next_cluster;
		
		// Find which sector of FAT table to read
		if (fs->fat_type == FAT_TYPE_16)
		{
			current_set_cluster_status.fat_sector_offset = cluster / 256;
		}
		else
		{
			current_set_cluster_status.fat_sector_offset = cluster / 128;
		}
		
		// Read FAT sector into buffer
		current_set_cluster_status.pbuf = NULL;
		current_set_cluster_stage = FATFS_SET_CLUSTER_STAGE_1;
	case FATFS_SET_CLUSTER_STAGE_1:
		switch (fatfs_fat_read_sector(fs, fs->fat_begin_lba + current_set_cluster_status.fat_sector_offset, tempBuf))
		{
		case 0:
			current_set_cluster_stage = FATFS_SET_CLUSTER_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_set_cluster_status.pbuf = tempBuf[0];
		if (!current_set_cluster_status.pbuf)
		{
			current_set_cluster_stage = FATFS_SET_CLUSTER_STAGE_0;
			return 0;
		}
		if (fs->fat_type == FAT_TYPE_16)
		{
			// Find 16 bit entry of current sector relating to cluster number 
			current_set_cluster_status.position = (cluster - (current_set_cluster_status.fat_sector_offset * 256)) * 2; 

			// Write Next Clusters value to Sector Buffer
			FAT16_SET_16BIT_WORD(current_set_cluster_status.pbuf, (UINT16)current_set_cluster_status.position, ((UINT16)next_cluster));	 
		}
		else
		{
			// Find 32 bit entry of current sector relating to cluster number 
			current_set_cluster_status.position = (cluster - (current_set_cluster_status.fat_sector_offset * 128)) * 4; 

			// Write Next Clusters value to Sector Buffer
			FAT32_SET_32BIT_WORD(current_set_cluster_status.pbuf, (UINT16)current_set_cluster_status.position, next_cluster);	 
		}
		current_set_cluster_stage = FATFS_SET_CLUSTER_STAGE_0;
		return 1;
	}
	current_set_cluster_stage = FATFS_SET_CLUSTER_STAGE_0;
	return 0;
} 
#endif

#ifdef FATFS_INC_WRITE_SUPPORT
typedef struct _fatfs_free_cluster_chain_status{
	UINT32 last_cluster;
	UINT32 next_cluster;
} fatfs_free_cluster_chain_status_t;

typedef enum{
	FATFS_FREE_CLUSTER_CHAIN_STAGE_0 = 0,
	FATFS_FREE_CLUSTER_CHAIN_STAGE_1,
	FATFS_FREE_CLUSTER_CHAIN_STAGE_2,
	FATFS_FREE_CLUSTER_CHAIN_STAGE_3,
	FATFS_FREE_CLUSTER_CHAIN_STAGE_4
} fatfs_free_cluster_chain_stage_t;

fatfs_free_cluster_chain_status_t current_free_cluster_chain_status;
fatfs_free_cluster_chain_stage_t current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_0;

//-----------------------------------------------------------------------------
// fatfs_free_cluster_chain: Follow a chain marking each element as free
// YEB: Comp
//-----------------------------------------------------------------------------
int fatfs_free_cluster_chain(struct fatfs *__fs, UINT32 __start_cluster)
{
	static struct fatfs* fs;
	static UINT32 start_cluster;
	UINT32 tempCluster;
	
	switch (current_free_cluster_chain_stage)
	{
	case FATFS_FREE_CLUSTER_CHAIN_STAGE_0:
		fs = __fs;
		start_cluster = __start_cluster;
		current_free_cluster_chain_status.next_cluster = start_cluster;
		current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_0;
	case FATFS_FREE_CLUSTER_CHAIN_STAGE_1:
		if ((current_free_cluster_chain_status.next_cluster != FAT32_LAST_CLUSTER) && (current_free_cluster_chain_status.next_cluster != 0x00000000))
		{
			current_free_cluster_chain_status.last_cluster = current_free_cluster_chain_status.next_cluster;
			current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_2;
		}
		else
		{
			current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_4;
		}
	case FATFS_FREE_CLUSTER_CHAIN_STAGE_2:
		if (current_free_cluster_chain_stage == FATFS_FREE_CLUSTER_CHAIN_STAGE_2)
		{
			// Find next link
			switch (fatfs_find_next_cluster(fs, current_free_cluster_chain_status.next_cluster, &tempCluster))
			{
			case 0:
				current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_free_cluster_chain_status.next_cluster = tempCluster;
			current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_3;
		}
	case FATFS_FREE_CLUSTER_CHAIN_STAGE_3:
		if (current_free_cluster_chain_stage == FATFS_FREE_CLUSTER_CHAIN_STAGE_3)
		{
			// Clear last link
			switch (fatfs_fat_set_cluster(fs, current_free_cluster_chain_status.last_cluster, 0x00000000))
			{
			case 0:
				current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_4;
		}
	case FATFS_FREE_CLUSTER_CHAIN_STAGE_4:
		current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_0;
		return 1;
	}
	current_free_cluster_chain_stage = FATFS_FREE_CLUSTER_CHAIN_STAGE_0;
	return 0;
} 
#endif
//-----------------------------------------------------------------------------
// fatfs_fat_add_cluster_to_chain: Follow a chain marking and then add a new entry
// to the current tail.
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
typedef struct _fat_add_cluster_chain_status{	
	UINT32 last_cluster;
	UINT32 next_cluster;
} fat_add_cluster_chain_status_t;

typedef enum{
	FAT_ADD_CLUSTER_CHAIN_STAGE_0 = 0,
	FAT_ADD_CLUSTER_CHAIN_STAGE_1,
	FAT_ADD_CLUSTER_CHAIN_STAGE_2,
	FAT_ADD_CLUSTER_CHAIN_STAGE_3,
	FAT_ADD_CLUSTER_CHAIN_STAGE_4
} fat_add_cluster_chain_stage_t;

fat_add_cluster_chain_status_t current_add_cluster_chain_status;
fat_add_cluster_chain_stage_t current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;

int fatfs_fat_add_cluster_to_chain(struct fatfs* __fs, UINT32 __start_cluster, UINT32 __newEntry)
{
	//UINT32 last_cluster = FAT32_LAST_CLUSTER;
	//UINT32 next_cluster = start_cluster;
	UINT32 tempCluster;
	static struct fatfs* fs;
	static UINT32 start_cluster;
	static UINT32 newEntry;

	switch (current_add_cluster_chain_stage)
	{
	case FAT_ADD_CLUSTER_CHAIN_STAGE_0:
		fs = __fs;
		start_cluster = __start_cluster;
		newEntry = __newEntry;
		current_add_cluster_chain_status.last_cluster = FAT32_LAST_CLUSTER;
		current_add_cluster_chain_status.next_cluster = start_cluster;
		if (start_cluster == FAT32_LAST_CLUSTER)
		{
			return 0;
		}
		current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_1;
	case FAT_ADD_CLUSTER_CHAIN_STAGE_1:
		// Loop until end of chain
		if (current_add_cluster_chain_status.next_cluster != FAT32_LAST_CLUSTER)
		{
			current_add_cluster_chain_status.last_cluster = current_add_cluster_chain_status.next_cluster;
			current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_2;
		}
		else
		{
			current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_3;
		}
	case FAT_ADD_CLUSTER_CHAIN_STAGE_2:
		if (current_add_cluster_chain_stage == FAT_ADD_CLUSTER_CHAIN_STAGE_2)
		{
			// Find next link
			switch (fatfs_find_next_cluster(fs, current_add_cluster_chain_status.next_cluster, &tempCluster))
			{
			case 0:
				current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_add_cluster_chain_status.next_cluster = tempCluster;
			if (!current_add_cluster_chain_status.next_cluster)
			{
				current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;
				return 0;
			}
			if (current_add_cluster_chain_status.next_cluster != FAT32_LAST_CLUSTER)
			{
				current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_1;
				return -1;
			}
			current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_3;
		}
	case FAT_ADD_CLUSTER_CHAIN_STAGE_3:
		// Add link in for new cluster
		switch (fatfs_fat_set_cluster(fs, current_add_cluster_chain_status.last_cluster, newEntry))
		{
		case 0:
			current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_4;
	case FAT_ADD_CLUSTER_CHAIN_STAGE_4:
		// Mark new cluster as end of chain
		switch(fatfs_fat_set_cluster(fs, newEntry, FAT32_LAST_CLUSTER))
		{
		case 0:
			current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;
		return 1;
	}
	current_add_cluster_chain_stage = FAT_ADD_CLUSTER_CHAIN_STAGE_0;
	return 0;
} 
#endif

