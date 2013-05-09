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
#include <stdlib.h>
#include <string.h>
#include "fat_defs.h"
#include "fat_access.h"
#include "fat_table.h"
#include "fat_write.h"
#include "fat_misc.h"
#include "fat_string.h"
#include "fat_filelib.h"


typedef enum{
	OPEN_DIRECTORY_STAGE_0 = 0,
	OPEN_DIRECTORY_STAGE_1,
	OPEN_DIRECTORY_STAGE_2
} open_dir_stage_t;

typedef struct _open_dir_status{
	int levels;
	int sublevel;
	char currentfolder[FATFS_MAX_LONG_FILENAME];
	FAT32_ShortEntry sfEntry;
	unsigned long startcluster;
} open_dir_status_t;

//-----------------------------------------------------------------------------
// Locals
//-----------------------------------------------------------------------------
static FL_FILE			Files[FATFS_MAX_OPEN_FILES];
static int				Filelib_Init = 0;
static int				Filelib_Valid = 0;
static struct fatfs		Fs;
static FL_FILE*			Filelib_open_files = NULL;
static FL_FILE*			Filelib_free_files = NULL;

static open_dir_stage_t current_open_dir_stage = OPEN_DIRECTORY_STAGE_0;
static open_dir_status_t current_oepn_dir_status;
//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

// Macro for checking if file lib is initialised
#define CHECK_FL_INIT()		{ if (Filelib_Init==0) fl_init(); }

#define FL_LOCK(a)			do { if ((a)->fl_lock) (a)->fl_lock(); } while (0)
#define FL_UNLOCK(a)		do { if ((a)->fl_unlock) (a)->fl_unlock(); } while (0)

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
//static void				_fl_init();

// YEB
static char tolower(char c)
{
    if (('A' <= c) && (c <= 'Z'))
    {
        c = 'a' + (c - 'A');
    }
    return c;
}

//-----------------------------------------------------------------------------
// _allocate_file: Find a slot in the open files buffer for a new file
//-----------------------------------------------------------------------------
static FL_FILE* _allocate_file(unsigned char isChunkRequired)
{
	// Allocate free file
	FL_FILE* mFile = Filelib_free_files;

	if (mFile)
	{
		Filelib_free_files = mFile->next;

		// Add to open list
		mFile->next = Filelib_open_files;
		// YEB edited
		mFile->isChunkRequired = isChunkRequired;
		mFile->LatestCluster = 0;
		Filelib_open_files = mFile;
	}

	return mFile;
}
//-----------------------------------------------------------------------------
// _check_file_open: Returns true if the file is already open
//-----------------------------------------------------------------------------
static int _check_file_open(FL_FILE* pFile)
{
	FL_FILE* openFile = Filelib_open_files;
	
	// Compare open files
	while (openFile)
	{
		// If not the current file 
		if (openFile != pFile)
		{
			// Compare path and name
			if ( (fatfs_compare_names(openFile->path,pFile->path)) && (fatfs_compare_names(openFile->filename,pFile->filename)) )
			{
				return 1;
			}
		}
		openFile = openFile->next;
	}
	return 0;
}
//-----------------------------------------------------------------------------
// _free_file: Free open file handle
//-----------------------------------------------------------------------------
static void _free_file(FL_FILE* pFile)
{
	FL_FILE* openFile = Filelib_open_files;
	FL_FILE* lastFile = NULL;
	
	// Remove from open list
	while (openFile)
	{
		// If the current file 
		if (openFile == pFile)
		{
			if (lastFile)
			{
				lastFile->next = openFile->next;
			}
			else
			{
 				Filelib_open_files = openFile->next;
			}
			break;
		}
		lastFile = openFile;
		openFile = openFile->next;
	}
	// Add to free list
	pFile->next = Filelib_free_files;
	Filelib_free_files = pFile;
}

//-----------------------------------------------------------------------------
//								Low Level
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// _open_directory: Cycle through path string to find the start cluster
// address of the highest subdir.
// YEB: Comp
//-----------------------------------------------------------------------------
static int _open_directory(char* __path, unsigned long* __pathCluster)
{
	static char* path;
	static unsigned long* pathCluster;
	int i;
	
	switch (current_open_dir_stage)
	{
	case OPEN_DIRECTORY_STAGE_0:
		path = __path;
		pathCluster = __pathCluster;
		// Set starting cluster to root cluster
		current_oepn_dir_status.startcluster = fatfs_get_root_cluster(&Fs);
		// Find number of levels
		current_oepn_dir_status.levels = fatfs_total_path_levels(path);
		// Cycle through each level and get the start sector
        current_oepn_dir_status.sublevel = 0;
		current_open_dir_stage = OPEN_DIRECTORY_STAGE_1;
	case OPEN_DIRECTORY_STAGE_1:
		if (current_oepn_dir_status.sublevel == current_oepn_dir_status.levels + 1)
		{
			*pathCluster = current_oepn_dir_status.startcluster;
			current_open_dir_stage = OPEN_DIRECTORY_STAGE_0;
			return 1;
		}
		if (fatfs_get_substring(path, current_oepn_dir_status.sublevel, current_oepn_dir_status.currentfolder, sizeof(current_oepn_dir_status.currentfolder)) == -1)
		{
			current_open_dir_stage = OPEN_DIRECTORY_STAGE_0;
			return 0;
		}
		current_open_dir_stage = OPEN_DIRECTORY_STAGE_2;
	case OPEN_DIRECTORY_STAGE_2:
		i = fatfs_get_file_entry(&Fs, current_oepn_dir_status.startcluster, current_oepn_dir_status.currentfolder,&current_oepn_dir_status.sfEntry);
		switch (i)
		{
		case 0:
			current_open_dir_stage = OPEN_DIRECTORY_STAGE_0;
			return 0;
		case -1:
			return -1;
		default:
			if (fatfs_entry_is_dir(&current_oepn_dir_status.sfEntry))
			{
				current_oepn_dir_status.startcluster = (((unsigned long)(current_oepn_dir_status.sfEntry.FstClusHI)) << 16) + current_oepn_dir_status.sfEntry.FstClusLO;
				current_open_dir_stage = OPEN_DIRECTORY_STAGE_1;
				++current_oepn_dir_status.sublevel;
				return -1;
			}
			else
			{
				current_open_dir_stage = OPEN_DIRECTORY_STAGE_0;
				return 0;
			}
		}
	}
	return 0;
}


typedef struct _create_dir_status{
	FL_FILE* file; 
	FAT32_ShortEntry sfEntry;
	char shortFilename[11];
	int tailNum;
	int sectorCounter;
} create_dir_status_t;

typedef enum{
	CREATE_DIR_STAGE_0 = 0,
	CREATE_DIR_STAGE_1,
	CREATE_DIR_STAGE_2,
	CREATE_DIR_STAGE_3,
	CREATE_DIR_STAGE_4,
	CREATE_DIR_STAGE_5,
	CREATE_DIR_STAGE_6,
	CREATE_DIR_STAGE_7,
	CREATE_DIR_STAGE_8,
	CREATE_DIR_STAGE_9,
	CREATE_DIR_STAGE_10
} create_dir_stage_t;

create_dir_status_t current_create_dir_status;
create_dir_stage_t current_create_dir_stage = CREATE_DIR_STAGE_0;

//-----------------------------------------------------------------------------
// _create_directory: Cycle through path string and create the end directory
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
static int _create_directory(char* __path, unsigned char isChunkRequired)
{
	//FL_FILE* file; 
	//FAT32_ShortEntry sfEntry;
	//char shortFilename[11];
	//int tailNum;
	//int i;
	static char* path;

	switch (current_create_dir_stage)
	{
	case CREATE_DIR_STAGE_0:
		path = __path;
		// Allocate a new file handle
		current_create_dir_status.file = _allocate_file(isChunkRequired);
		if (!current_create_dir_status.file)
		{
			return 0;
		}
		// Clear filename
		memset(current_create_dir_status.file->path, '\0', sizeof(current_create_dir_status.file->path));
		memset(current_create_dir_status.file->filename, '\0', sizeof(current_create_dir_status.file->filename));

		// Split full path into filename and directory path
		if (fatfs_split_path((char*)path, current_create_dir_status.file->path, sizeof(current_create_dir_status.file->path), current_create_dir_status.file->filename, sizeof(current_create_dir_status.file->filename)) == -1)
		{
			_free_file(current_create_dir_status.file);
			return 0;
		}

		// Check if file already open
		if (_check_file_open(current_create_dir_status.file))
		{
			_free_file(current_create_dir_status.file);
			return 0;
		}

		// If file is in the root dir
		if (current_create_dir_status.file->path[0] == 0)
		{
			current_create_dir_status.file->parentcluster = fatfs_get_root_cluster(&Fs);
			current_create_dir_stage = CREATE_DIR_STAGE_2;
		}
		else
		{
			current_create_dir_stage = CREATE_DIR_STAGE_1;
		}
	case CREATE_DIR_STAGE_1:
		if (current_create_dir_stage == CREATE_DIR_STAGE_1)
		{
			// Find parent directory start cluster
			switch (_open_directory(current_create_dir_status.file->path, &current_create_dir_status.file->parentcluster))
			{
			case 0:
				current_create_dir_stage = CREATE_DIR_STAGE_0;
				_free_file(current_create_dir_status.file);
				return 0;
			case -1:
				return -1;
			}
			current_create_dir_stage = CREATE_DIR_STAGE_2;
		}
	case CREATE_DIR_STAGE_2:
		// Check if same filename exists in directory
		switch (fatfs_get_file_entry(&Fs, current_create_dir_status.file->parentcluster, current_create_dir_status.file->filename, &current_create_dir_status.sfEntry))
		{
		case 1:
			current_create_dir_stage = CREATE_DIR_STAGE_0;
			_free_file(current_create_dir_status.file);
			return 0;
		case -1:
			return -1;
		}
		current_create_dir_status.file->startcluster = 0;
		current_create_dir_stage = CREATE_DIR_STAGE_3;
	case CREATE_DIR_STAGE_3:
		// Create the file space for the folder (at least one clusters worth!)
		switch (fatfs_allocate_free_space(&Fs, 1, &current_create_dir_status.file->startcluster, 1, isChunkRequired))
		{
		case 0:
			current_create_dir_stage = CREATE_DIR_STAGE_0;
			_free_file(current_create_dir_status.file);
			return 0;
		case -1:
			return -1;
		}
		// Erase new directory cluster
		memset(current_create_dir_status.file->file_data.sector, 0x00, FAT_SECTOR_SIZE);
		current_create_dir_stage = CREATE_DIR_STAGE_4;
	case CREATE_DIR_STAGE_4:
		if (current_create_dir_status.sectorCounter < Fs.sectors_per_cluster)
		{
			switch (fatfs_sector_writer(&Fs, current_create_dir_status.file->startcluster, current_create_dir_status.sectorCounter, current_create_dir_status.file->file_data.sector, 0, FALSE))
			{
			case 0:
				current_create_dir_stage = CREATE_DIR_STAGE_0;
				_free_file(current_create_dir_status.file);
				return 0;
			case -1:
				return -1;
			}
			++current_create_dir_status.sectorCounter;
			if (current_create_dir_status.sectorCounter < Fs.sectors_per_cluster)
			{
				return -1;
			}
		}
		// Generate a short filename & tail
		current_create_dir_status.tailNum = 0;
		current_create_dir_stage = CREATE_DIR_STAGE_5;
	case CREATE_DIR_STAGE_5:
		// Create a standard short filename (without tail)
		fatfs_lfn_create_sfn(current_create_dir_status.shortFilename, current_create_dir_status.file->filename);

        // If second hit or more, generate a ~n tail		
		if (current_create_dir_status.tailNum != 0)
		{
			fatfs_lfn_generate_tail((char*)current_create_dir_status.file->shortfilename, current_create_dir_status.shortFilename, current_create_dir_status.tailNum);
		}
		// Try with no tail if first entry
		else
		{
			memcpy(current_create_dir_status.file->shortfilename, current_create_dir_status.shortFilename, 11);
		}
		current_create_dir_stage = CREATE_DIR_STAGE_6;
	case CREATE_DIR_STAGE_6:
		// Check if entry exists already or not
		switch (fatfs_sfn_exists(&Fs, current_create_dir_status.file->parentcluster, (char*)current_create_dir_status.file->shortfilename))
		{
		case 0:
			current_create_dir_stage = CREATE_DIR_STAGE_7;
			break;
		case -1:
			return -1;
		default:
			++current_create_dir_status.tailNum;
			if (current_create_dir_status.tailNum < 9999)
			{
				current_create_dir_stage = CREATE_DIR_STAGE_5;
				return -1;
			}
		}
		// We reached the max number of duplicate short file names (unlikely!)
		if (current_create_dir_status.tailNum == 9999)
		{
			current_create_dir_stage = CREATE_DIR_STAGE_9;
		}
	case CREATE_DIR_STAGE_7:
		if (current_create_dir_stage == CREATE_DIR_STAGE_7)
		{
			// Add file to disk
			switch (fatfs_add_file_entry(&Fs, current_create_dir_status.file->parentcluster, (char*)current_create_dir_status.file->filename, (char*)current_create_dir_status.file->shortfilename, current_create_dir_status.file->startcluster, 0, 1))
			{
			case 0:
				current_create_dir_stage = CREATE_DIR_STAGE_9;
				break;
			case -1:
				return -1;
			}
			// General
			current_create_dir_status.file->filelength = 0;
			current_create_dir_status.file->bytenum = 0;
			current_create_dir_status.file->file_data.address = 0xFFFFFFFF;
			current_create_dir_status.file->file_data.dirty = 0;
			current_create_dir_status.file->filelength_changed = 0;
			current_create_dir_stage = CREATE_DIR_STAGE_8;
		}
	case CREATE_DIR_STAGE_8:
		if (current_create_dir_stage == CREATE_DIR_STAGE_8)
		{
			switch (fatfs_fat_purge(&Fs))
			{
			case -1:
				return -1;
			}
			current_create_dir_stage = CREATE_DIR_STAGE_0;
			_free_file(current_create_dir_status.file);
			return 1;
		}
	case CREATE_DIR_STAGE_9:
		// Delete allocated space
		switch (fatfs_free_cluster_chain(&Fs, current_create_dir_status.file->startcluster))
		{
		case -1:
			return -1;
		}
		current_create_dir_stage = CREATE_DIR_STAGE_0;
		_free_file(current_create_dir_status.file);
		return 0;	
	}
	current_create_dir_stage = CREATE_DIR_STAGE_0;
	_free_file(current_create_dir_status.file);
	return 0;
}

typedef struct _open_file_status{
	FL_FILE* mFile; 
	FAT32_ShortEntry sfEntry;
} open_file_status_t;

typedef enum{
	OPEN_FILE_STAGE_0 = 0,
	OPEN_FILE_STAGE_1,
	OPEN_FILE_STAGE_2,
	OPEN_FILE_STAGE_3,
	OPEN_FILE_STAGE_4
} open_file_stage_t;

open_file_status_t current_open_file_status;
open_file_stage_t current_open_file_stage = OPEN_FILE_STAGE_0;

//-----------------------------------------------------------------------------
// _open_file: Open a file for reading
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
static int _open_file(const char* __path, FL_FILE** ___openFile, unsigned char isChunkRequired)
{
	static FL_FILE** openFile;
	static const char* path;
	
	//FL_FILE* mFile; 
	//FAT32_ShortEntry sfEntry;
	switch (current_open_file_stage)
	{
	case OPEN_FILE_STAGE_0:
		openFile = ___openFile;
		path = __path;
		
		// Allocate a new file handle
		current_open_file_status.mFile = _allocate_file(isChunkRequired);
		if (!current_open_file_status.mFile)
		{
			return 0;
		}
		// Clear filename
		memset(current_open_file_status.mFile->path, '\0', sizeof(current_open_file_status.mFile->path));
		memset(current_open_file_status.mFile->filename, '\0', sizeof(current_open_file_status.mFile->filename));

		// Split full path into filename and directory path
		if (fatfs_split_path((char*)path, current_open_file_status.mFile->path, sizeof(current_open_file_status.mFile->path), current_open_file_status.mFile->filename, sizeof(current_open_file_status.mFile->filename)) == -1)
		{
			_free_file(current_open_file_status.mFile);
			current_open_file_stage = OPEN_FILE_STAGE_0;
			return 0;
		}

		// Check if file already open
		if (_check_file_open(current_open_file_status.mFile))
		{
			_free_file(current_open_file_status.mFile);
			current_open_file_stage = OPEN_FILE_STAGE_0;
			return 0;
		}

		// If file is in the root dir
		if (current_open_file_status.mFile->path[0]==0)
		{
			current_open_file_status.mFile->parentcluster = fatfs_get_root_cluster(&Fs);
			current_open_file_stage = OPEN_FILE_STAGE_2;
		}
		else
		{
			// Find parent directory start cluster
			current_open_file_stage = OPEN_FILE_STAGE_1;
		}
	case OPEN_FILE_STAGE_1:
		if (current_open_file_stage == OPEN_FILE_STAGE_1)
		{
			switch (_open_directory(current_open_file_status.mFile->path, &current_open_file_status.mFile->parentcluster))
			{
			case 0:
				_free_file(current_open_file_status.mFile);
				current_open_file_stage = OPEN_FILE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_open_file_stage = OPEN_FILE_STAGE_2;
		}
	case OPEN_FILE_STAGE_2:
		// Using dir cluster address search for filename
		switch (fatfs_get_file_entry(&Fs, current_open_file_status.mFile->parentcluster, current_open_file_status.mFile->filename,&current_open_file_status.sfEntry))
		{
		case 0:
			_free_file(current_open_file_status.mFile);
			current_open_file_stage = OPEN_FILE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		if (fatfs_entry_is_file(&current_open_file_status.sfEntry))
		{
			// Initialise file details
			memcpy(current_open_file_status.mFile->shortfilename, current_open_file_status.sfEntry.Name, 11);
			current_open_file_status.mFile->filelength = current_open_file_status.sfEntry.FileSize;
			current_open_file_status.mFile->bytenum = 0;
			current_open_file_status.mFile->startcluster = (((unsigned long)current_open_file_status.sfEntry.FstClusHI) << 16) + current_open_file_status.sfEntry.FstClusLO;
			current_open_file_status.mFile->file_data.address = 0xFFFFFFFF;
			current_open_file_status.mFile->file_data.dirty = 0;
			current_open_file_status.mFile->filelength_changed = 0;
			current_open_file_stage = OPEN_FILE_STAGE_3;		
		}
		else
		{
			_free_file(current_open_file_status.mFile);
			current_open_file_stage = OPEN_FILE_STAGE_0;
			return 0;
		}
	case OPEN_FILE_STAGE_3:
		switch (fatfs_fat_purge(&Fs))
		{
		case 0:
			_free_file(current_open_file_status.mFile);
			current_open_file_stage = OPEN_FILE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		*openFile = current_open_file_status.mFile;
		current_open_file_stage = OPEN_FILE_STAGE_0;
		return 1;
	}
	current_open_file_stage = OPEN_FILE_STAGE_0;
	return 0;
}


#ifdef FATFS_INC_WRITE_SUPPORT

typedef enum{
	CREATE_FILE_STAGE_0 = 0,
    CREATE_FILE_STAGE_0_1,
	CREATE_FILE_STAGE_1,
	CREATE_FILE_STAGE_2,
	CREATE_FILE_STAGE_3,
	CREATE_FILE_STAGE_4,
	CREATE_FILE_STAGE_5,
	CREATE_FILE_STAGE_6,
	CREATE_FILE_STAGE_7
} create_file_stage_t;

typedef struct _create_file_status{
	FL_FILE* mFile; 
	FAT32_ShortEntry sfEntry;
	char shortFilename[11];
	int tailNum;
} create_file_status_t;

create_file_stage_t current_create_file_stage = CREATE_FILE_STAGE_0;
create_file_status_t current_create_file_status;

//-----------------------------------------------------------------------------
// _create_file: Create a new file
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
static int _create_file(const char *__filename, FL_FILE** __newFile, unsigned char isChunkRequired)

{
	int i;
	static FL_FILE** newFile;
	static const char* filename;
	
	switch (current_create_file_stage)
	{
	case CREATE_FILE_STAGE_0:
		filename = __filename;
		newFile = __newFile;
		// Allocate a new file handle
		current_create_file_status.mFile = _allocate_file(isChunkRequired);
		if (!current_create_file_status.mFile)
		{
			return NULL;
		}
		// Clear filename
		memset(current_create_file_status.mFile->path, '\0', sizeof(current_create_file_status.mFile->path));
		memset(current_create_file_status.mFile->filename, '\0', sizeof(current_create_file_status.mFile->filename));
		// Split full path into filename and directory path
		if (fatfs_split_path((char*)filename, current_create_file_status.mFile->path, sizeof(current_create_file_status.mFile->path), current_create_file_status.mFile->filename, sizeof(current_create_file_status.mFile->filename)) == -1)
		{
			_free_file(current_create_file_status.mFile);
			return NULL;
		}
		// Check if file already open
		if (_check_file_open(current_create_file_status.mFile))
		{
			_free_file(current_create_file_status.mFile);
			return NULL;
		}
		// If file is in the root dir
		if (current_create_file_status.mFile->path[0] == 0)
		{
			current_create_file_status.mFile->parentcluster = fatfs_get_root_cluster(&Fs);
        }
		else
		{
			// Find parent directory start cluster
            current_create_file_stage = CREATE_FILE_STAGE_0_1;
        }
    case CREATE_FILE_STAGE_0_1:
        if (current_create_file_stage == CREATE_FILE_STAGE_0_1)
        {
            switch(_open_directory(current_create_file_status.mFile->path, &current_create_file_status.mFile->parentcluster))	
            {
            case 0:
                _free_file(current_create_file_status.mFile);
                //dir doesn't exist?
                //current_create_file_stage = CREATE_FILE_STAGE_0_2;
                current_create_file_stage = CREATE_FILE_STAGE_0;
                return 0;
            case -1:
                return -1;
            }
            // Check if same filename exists in directory
        }
        current_create_file_stage = CREATE_FILE_STAGE_1;
	case CREATE_FILE_STAGE_1:
		switch (fatfs_get_file_entry(&Fs, current_create_file_status.mFile->parentcluster, current_create_file_status.mFile->filename, &current_create_file_status.sfEntry))
		{
		case 0:
			break;
		case -1:
			return -1;
		default:
			current_create_file_stage = CREATE_FILE_STAGE_0;
			_free_file(current_create_file_status.mFile);
			return 0;
		}
		current_create_file_status.mFile->startcluster = 0;
		current_create_file_stage = CREATE_FILE_STAGE_2;
	case CREATE_FILE_STAGE_2:
		// Create the file space for the file (at least one clusters worth!)
		i = fatfs_allocate_free_space(&Fs, 1, &current_create_file_status.mFile->startcluster, 1, isChunkRequired);
		switch (i)
		{
		case 0:
			_free_file(current_create_file_status.mFile);
			current_create_file_stage = CREATE_FILE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		if (isChunkRequired)
		{
			current_create_file_status.mFile->LatestCluster = current_create_file_status.mFile->startcluster;
		}
		// Generate a short filename & tail
		current_create_file_status.tailNum = 0;
		current_create_file_stage = CREATE_FILE_STAGE_3;
	case CREATE_FILE_STAGE_3:
		// Create a standard short filename (without tail)
		// YEB -> why repeat?
		fatfs_lfn_create_sfn(current_create_file_status.shortFilename, current_create_file_status.mFile->filename);

        // If second hit or more, generate a ~n tail		
		if (current_create_file_status.tailNum != 0)
		{
			fatfs_lfn_generate_tail((char*)current_create_file_status.mFile->shortfilename, current_create_file_status.shortFilename, current_create_file_status.tailNum);
		}
		// Try with no tail if first entry
		else
		{
			memcpy(current_create_file_status.mFile->shortfilename, current_create_file_status.shortFilename, 11);
		}
		current_create_file_stage = CREATE_FILE_STAGE_4;
	case CREATE_FILE_STAGE_4:
		// Check if entry exists already or not
        // check from here 05272011
		switch (fatfs_sfn_exists(&Fs, current_create_file_status.mFile->parentcluster, (char*)current_create_file_status.mFile->shortfilename))
		{
		case 0:
			break;
		case -1:
			return -1;
		default:
			++current_create_file_status.tailNum;
			if (current_create_file_status.tailNum < 9999)
			{
				current_create_file_stage = CREATE_FILE_STAGE_5;
				return -1;
			}
		}
		if (current_create_file_status.tailNum == 9999)
		{
			current_create_file_stage = CREATE_FILE_STAGE_7;
		}
		else
		{
			current_create_file_stage = CREATE_FILE_STAGE_5;
		}
	case CREATE_FILE_STAGE_5:
		if (current_create_file_stage == CREATE_FILE_STAGE_5)
		{
			// Add file to disk
			switch (fatfs_add_file_entry(&Fs, current_create_file_status.mFile->parentcluster, (char*)current_create_file_status.mFile->filename, (char*)current_create_file_status.mFile->shortfilename, current_create_file_status.mFile->startcluster, 0, 0))
			{
			case 0:
				current_create_file_stage = CREATE_FILE_STAGE_7;
				break;
			case -1:
				return -1;
			default:
				// General
				current_create_file_status.mFile->filelength = 0;
				current_create_file_status.mFile->bytenum = 0;
				current_create_file_status.mFile->file_data.address = 0xFFFFFFFF;
				current_create_file_status.mFile->file_data.dirty = 0;
				current_create_file_status.mFile->filelength_changed = 0;
				current_create_file_stage = CREATE_FILE_STAGE_6;
			}
		}
	case CREATE_FILE_STAGE_6:
		if (current_create_file_stage == CREATE_FILE_STAGE_6)
		{
			switch(fatfs_fat_purge(&Fs))
			{
			case 0:
				_free_file(current_create_file_status.mFile);
				current_create_file_stage = CREATE_FILE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			*newFile = current_create_file_status.mFile;
			current_create_file_stage = CREATE_FILE_STAGE_0;
			return 1;
		}
	case CREATE_FILE_STAGE_7:
		// Delete allocated space
		switch (fatfs_free_cluster_chain(&Fs, current_create_file_status.mFile->startcluster))
		{
		case 0:
			break;
		case -1:
			return -1;
		}
		_free_file(current_create_file_status.mFile);
		current_create_file_stage = CREATE_FILE_STAGE_0;
		return 0;
	}
	current_create_file_stage = CREATE_FILE_STAGE_0;
	return 0;
}
#endif

//-----------------------------------------------------------------------------
//								External API
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// fl_init: Initialise library
//-----------------------------------------------------------------------------
void fl_init(void)
{	
	int i;

	// Add all file objects to free list
	for (i = 0; i < FATFS_MAX_OPEN_FILES; ++i)
	{
        //Files[i].id = i + 1;
		Files[i].next = Filelib_free_files;
		Filelib_free_files = &Files[i];
	}

	Filelib_Init = 1;
}
//-----------------------------------------------------------------------------
// fl_attach_locks: 
//-----------------------------------------------------------------------------
void fl_attach_locks(struct fatfs *fs, void (*lock)(void), void (*unlock)(void))
{
	fs->fl_lock = lock;
	fs->fl_unlock = unlock;
}
//-----------------------------------------------------------------------------
// fl_attach_media: 
//-----------------------------------------------------------------------------
int fl_attach_media(fn_diskio_read rd, fn_diskio_write wr)
{
	int res = FAT_INIT_ON_PROCESS;
	UINT32 free_chunk_start = 0;

	// If first call to library, initialise
	CHECK_FL_INIT();

	Fs.disk_io.read_sector = rd;
	Fs.disk_io.write_sector = wr;

	// Initialise FAT parameters
	while (res == FAT_INIT_ON_PROCESS)
	{
		res = fatfs_init(&Fs);
	}
	if (res != FAT_INIT_OK)
	{
		FAT_PRINTF(("FAT_FS: Error could not load FAT details (%d)!\r\n", res));
		return res;
	}
	
	//if ((res = fatfs_init(&Fs)) != FAT_INIT_OK)
	//{
	//	FAT_PRINTF(("FAT_FS: Error could not load FAT details (%d)!\r\n", res));
	//	return res;
	//}

	// find the fisrt empty pool
	while (fatfs_find_empty_cluster_chunk(&Fs, &free_chunk_start) == -1);
	if (Fs.empty_chunk_start_cluster_number >= 128)
	{
		Fs.empty_chunk_start_cluster_number -= 128;
	}
	Filelib_Valid = 1;
	return FAT_INIT_OK;
}
//-----------------------------------------------------------------------------
// fl_shutdown: Call before shutting down system
//-----------------------------------------------------------------------------
void fl_shutdown(void)
{
	// If first call to library, initialise
	CHECK_FL_INIT();

	FL_LOCK(&Fs);
	while (fatfs_fat_purge(&Fs) == -1);
	FL_UNLOCK(&Fs);
}

typedef struct _fl_fopen_status{
	FL_FILE* mFile; 
	unsigned char flags;
} fl_fopen_status_t;

typedef enum{
	FL_FOPEN_STAGE_0 = 0,
	FL_FOPEN_STAGE_1,
	FL_FOPEN_STAGE_2,
	FL_FOPEN_STAGE_3,
	FL_FOPEN_STAGE_4,
	FL_FOPEN_STAGE_5,
	FL_FOPEN_STAGE_6
} fl_fopen_stage_t;

fl_fopen_status_t current_fopen_status;
fl_fopen_stage_t current_fopen_stage = FL_FOPEN_STAGE_0;

//-----------------------------------------------------------------------------
// fopen: Open or Create a file for reading or writing
// YEB: returns 0 if failed
// YEB: Comp
//-----------------------------------------------------------------------------
int fl_fopen(const char* __path, const char* __mode, void** __openFile, unsigned char isChunkRequired)
{
	static const char* path;
	static const char* mode;
	static void** openFile;
	int i;
	static FL_FILE* tempFile[1]; 
	//unsigned char flags = 0;

	switch (current_fopen_stage)
	{
	case FL_FOPEN_STAGE_0:
		path = __path;
		mode = __mode;
		openFile = __openFile;
		current_fopen_status.flags = 0;
		// If first call to library, initialise
		CHECK_FL_INIT();
		if (!Filelib_Valid)
		{
			return NULL;
		}
		if (!path || !mode)
		{
			return NULL;
		}
	
		// Supported Modes:
		// "r" Open a file for reading. The file must exist. 
		// "w" Create an empty file for writing. If a file with the same name already exists its content is erased and the file is treated as a new empty file.  
		// "a" Append to a file. Writing operations append data at the end of the file. The file is created if it does not exist. 
		// "r+" Open a file for update both reading and writing. The file must exist. 
		// "w+" Create an empty file for both reading and writing. If a file with the same name already exists its content is erased and the file is treated as a new empty file. 
		// "a+" Open a file for reading and appending. All writing operations are performed at the end of the file, protecting the previous content to be overwritten. You can reposition (fseek, rewind) the internal pointer to anywhere in the file for reading, but writing operations will move it back to the end of file. The file is created if it does not exist. 
	
		for (i = 0; i < (int)strlen(mode); i++)
		{
			switch (tolower(mode[i]))
			{
			case 'r':
				current_fopen_status.flags |= FILE_READ;
				break;
			case 'w':
				current_fopen_status.flags |= FILE_WRITE;
				current_fopen_status.flags |= FILE_ERASE;
				current_fopen_status.flags |= FILE_CREATE;
				break;
			case 'a':
				current_fopen_status.flags |= FILE_WRITE;
				current_fopen_status.flags |= FILE_APPEND;
				//current_fopen_status.flags |= FILE_CREATE;
				break;
			case '+':
				if (current_fopen_status.flags & FILE_READ)
				{
					current_fopen_status.flags |= FILE_WRITE;
				}
				else if (current_fopen_status.flags & FILE_WRITE)
				{
					current_fopen_status.flags |= FILE_READ;
					current_fopen_status.flags |= FILE_ERASE;
					current_fopen_status.flags |= FILE_CREATE;
				}
				else if (current_fopen_status.flags & FILE_APPEND)
				{
					current_fopen_status.flags |= FILE_READ;
					current_fopen_status.flags |= FILE_WRITE;
					current_fopen_status.flags |= FILE_APPEND;
					current_fopen_status.flags |= FILE_CREATE;
				}
				break;
			case 'b':
				current_fopen_status.flags |= FILE_BINARY;
				break;
			}
		}
		
		current_fopen_status.mFile = NULL;

#ifndef FATFS_INC_WRITE_SUPPORT
		// No write support!
		current_fopen_status.flags &= ~(FILE_CREATE | FILE_WRITE | FILE_APPEND);
#endif

		FL_LOCK(&Fs);

		// Read
		if (current_fopen_status.flags & FILE_READ)
		{
			current_fopen_stage = FL_FOPEN_STAGE_1;
		}
		else
		{
			current_fopen_stage = FL_FOPEN_STAGE_2;
		}
	case FL_FOPEN_STAGE_1:
		if (current_fopen_stage == FL_FOPEN_STAGE_1)
		{
			// READ FILE
			switch(_open_file(path, tempFile, FALSE))
			{
			case 0:
				FL_UNLOCK(&Fs);
				current_fopen_stage = FL_FOPEN_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_fopen_status.mFile = tempFile[0];
			current_fopen_stage = FL_FOPEN_STAGE_2;
		}
	case FL_FOPEN_STAGE_2:
		// Create New
		if ((!current_fopen_status.mFile) && (current_fopen_status.flags & FILE_CREATE))
		{
			// YEB: attention
			current_fopen_stage = FL_FOPEN_STAGE_3;
		}
		else
		{
			current_fopen_stage = FL_FOPEN_STAGE_4;
		}
	case FL_FOPEN_STAGE_3:
		if (current_fopen_stage == FL_FOPEN_STAGE_3)
		{
			switch (_create_file(path, tempFile, isChunkRequired))
			{
			case 0:
				FL_UNLOCK(&Fs);
				current_fopen_stage = FL_FOPEN_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_fopen_status.mFile = tempFile[0];
			current_fopen_stage = FL_FOPEN_STAGE_4;
		}
	case FL_FOPEN_STAGE_4:
		// Write Existing (and not open due to read or create)
		current_fopen_stage = FL_FOPEN_STAGE_6;
		if (!(current_fopen_status.flags & FILE_READ))
		{
			if (!(current_fopen_status.flags & FILE_CREATE))
			{
				if (current_fopen_status.flags & (FILE_WRITE | FILE_APPEND))
				{
					current_fopen_stage = FL_FOPEN_STAGE_5;
				}
			}
		}
	case FL_FOPEN_STAGE_5:
		if (current_fopen_stage == FL_FOPEN_STAGE_5)
		{
			switch (_open_file(path, &current_fopen_status.mFile, isChunkRequired))
			{
			case 0:
				FL_UNLOCK(&Fs);
				current_fopen_stage = FL_FOPEN_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_fopen_stage = FL_FOPEN_STAGE_6;
		}
	case FL_FOPEN_STAGE_6:
		if (current_fopen_status.mFile)
		{
			current_fopen_status.mFile->flags = current_fopen_status.flags;
		}
		FL_UNLOCK(&Fs);
		*openFile = current_fopen_status.mFile;
		current_fopen_stage = FL_FOPEN_STAGE_0;
		return 1;
	}
	FL_UNLOCK(&Fs);
	current_fopen_stage = FL_FOPEN_STAGE_0;
	return 0;
}

typedef struct _fl_fflush_status{
	FL_FILE *mFile;
} fl_fflush_status_t;

typedef enum{
	FL_FFLUSH_STAGE_0 = 0,
	FL_FFLUSH_STAGE_1,
	FL_FFLUSH_STAGE_2,
} fl_fflush_stage_t;

fl_fflush_status_t current_fl_fflush_status;
fl_fflush_stage_t current_fl_fflush_stage = FL_FFLUSH_STAGE_0;

//-----------------------------------------------------------------------------
// fl_fflush: Flush un-written data to the file
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
int fl_fflush(void *f)
{
#ifdef FATFS_INC_WRITE_SUPPORT
	switch (current_fl_fflush_stage)
	{
	case FL_FFLUSH_STAGE_0:
		current_fl_fflush_status.mFile = (FL_FILE *)f;
		// If first call to library, initialise
		CHECK_FL_INIT();
		if (current_fl_fflush_status.mFile)
		{
			FL_LOCK(&Fs);
			// If some write data still in buffer
			if (current_fl_fflush_status.mFile->file_data.dirty)
			{
				current_fl_fflush_stage = FL_FFLUSH_STAGE_1;
			}
			else
			{
				FL_UNLOCK(&Fs);
				return 1;
			}
		}
		else
		{
			return 0;
		}
	case FL_FFLUSH_STAGE_1:
		// Write back current sector before loading next
		switch (fatfs_sector_writer(&Fs, current_fl_fflush_status.mFile->startcluster, current_fl_fflush_status.mFile->file_data.address, current_fl_fflush_status.mFile->file_data.sector, &current_fl_fflush_status.mFile->LatestCluster, current_fl_fflush_status.mFile->isChunkRequired))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_fl_fflush_stage = FL_FFLUSH_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		FL_UNLOCK(&Fs);
		current_fl_fflush_status.mFile->file_data.dirty = 0;
	}
	current_fl_fflush_stage = FL_FFLUSH_STAGE_0;
#endif
	return 1;
}

typedef struct _fl_fclose_status{
	FL_FILE *mFile;
} fl_fclose_status_t;

typedef enum{
	FL_FCLOSE_STAGE_0 = 0,
	FL_FCLOSE_STAGE_1,
	FL_FCLOSE_STAGE_2,
	FL_FCLOSE_STAGE_3,
	FL_FCLOSE_STAGE_4,
	FL_FCLOSE_STAGE_5
} fl_fclose_stage_t;

fl_fclose_status_t current_fl_fclose_status;
fl_fclose_stage_t current_fl_fclose_stage = FL_FCLOSE_STAGE_0;

//-----------------------------------------------------------------------------
// fl_fclose: Close an open file
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//----------------------------------------------------------------------------- 
int fl_fclose(void* f)
{
	switch (current_fl_fclose_stage)
	{
	case FL_FCLOSE_STAGE_0:
		current_fl_fclose_status.mFile = (FL_FILE *)f;
		// If first call to library, initialise
		CHECK_FL_INIT();
		if (current_fl_fclose_status.mFile)
		{
			FL_LOCK(&Fs);
			current_fl_fclose_stage = FL_FCLOSE_STAGE_1;
		}
		else
		{
			return 0;
		}
	case FL_FCLOSE_STAGE_1:
		// Flush un-written data to file
		switch (fl_fflush(f))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_fl_fclose_stage = FL_FCLOSE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		// File size changed?
		if (current_fl_fclose_status.mFile->filelength_changed)
		{
			current_fl_fclose_stage = FL_FCLOSE_STAGE_2;
		}
		else
		{
			current_fl_fclose_stage = FL_FCLOSE_STAGE_3;
		}
	case FL_FCLOSE_STAGE_2:
		if (current_fl_fclose_stage == FL_FCLOSE_STAGE_2)
		{
#ifdef FATFS_INC_WRITE_SUPPORT
			// Update filesize in directory
			switch (fatfs_update_file_length(&Fs, current_fl_fclose_status.mFile->parentcluster, (char*)current_fl_fclose_status.mFile->shortfilename, current_fl_fclose_status.mFile->filelength))
			{
			case 0:
				FL_UNLOCK(&Fs);
				current_fl_fclose_stage = FL_FCLOSE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
#endif
			current_fl_fclose_status.mFile->filelength_changed = 0;
			current_fl_fclose_stage = FL_FCLOSE_STAGE_3;
		}
	case FL_FCLOSE_STAGE_3:
		current_fl_fclose_status.mFile->bytenum = 0;
		current_fl_fclose_status.mFile->filelength = 0;
		current_fl_fclose_status.mFile->startcluster = 0;
		current_fl_fclose_status.mFile->file_data.address = 0xFFFFFFFF;
		current_fl_fclose_status.mFile->file_data.dirty = 0;
		current_fl_fclose_status.mFile->filelength_changed = 0;

		// Free file handle
		_free_file(current_fl_fclose_status.mFile);
		//if (current_fl_fclose_status.mFile->isChunkRequired)
		//{
		//	current_fl_fclose_stage = FL_FCLOSE_STAGE_4;
		//}
		//else
		{
			current_fl_fclose_stage = FL_FCLOSE_STAGE_5;
		}
	case FL_FCLOSE_STAGE_4:
		if (current_fl_fclose_stage == FL_FCLOSE_STAGE_4)
		{
			switch (fatfs_fat_set_cluster(&Fs, current_fl_fclose_status.mFile->LatestCluster, FAT32_LAST_CLUSTER))
			{
			case 0:
				current_fl_fclose_stage = FL_FCLOSE_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_fl_fclose_stage = FL_FCLOSE_STAGE_5;
		}
	case FL_FCLOSE_STAGE_5:
		switch (fatfs_fat_purge(&Fs))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_fl_fclose_stage = FL_FCLOSE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		FL_UNLOCK(&Fs);
		current_fl_fclose_stage = FL_FCLOSE_STAGE_0;
		return 1;
	}
	current_fl_fclose_stage = FL_FCLOSE_STAGE_0;
	return 0;
}
//-----------------------------------------------------------------------------
// fl_fgetc: Get a character in the stream
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
int fl_fgetc(void *f, unsigned char* c)
{
	return fl_fread(c, 1, 1, f);
}

typedef struct _fl_fread_status{
	unsigned long sector;
	unsigned long offset;
	int copyCount;
	int count;
	int bytesRead;
	FL_FILE *mFile;
} fl_fread_status_t;

typedef enum{
	FL_FREAD_STAGE_0 = 0,
	FL_FREAD_STAGE_1,
	FL_FREAD_STAGE_2,
	FL_FREAD_STAGE_3,
	FL_FREAD_STAGE_4
} fl_fread_stage_t;

fl_fread_status_t current_fread_status;
fl_fread_stage_t current_fread_stage = FL_FREAD_STAGE_0;

//-----------------------------------------------------------------------------
// fl_fread: Read a block of data from the file
// YEB: Returns 0 if failed
// YEB: Returns -1 if on process
// YEB: Returns else if succeeded
// YEB: Comp
//-----------------------------------------------------------------------------
int fl_fread(void* __buffer, int size, int length, void* f)
{
	static void* buffer;
	
	//unsigned long sector;
	//unsigned long offset;
	//int copyCount;
	//int count = size * length;
	//int bytesRead = 0;
	
	switch (current_fread_stage)
	{
	case FL_FREAD_STAGE_0:
		buffer = __buffer;
		current_fread_status.count = size * length;
		current_fread_status.bytesRead = 0;
		current_fread_status.mFile = (FL_FILE *)f;

		// If first call to library, initialise
		CHECK_FL_INIT();

		if (buffer==NULL || current_fread_status.mFile==NULL)
		{
			return -1;
		}
		// No read permissions
		if (!(current_fread_status.mFile->flags & FILE_READ))
		{
			return -1;
		}
		// Nothing to be done
		if (!current_fread_status.count)
		{
			return 0;
		}
		// Check if read starts past end of file
		if (current_fread_status.mFile->bytenum >= current_fread_status.mFile->filelength)
		{
			return -1;
		}
		// Limit to file size
		if ( (current_fread_status.mFile->bytenum + current_fread_status.count) > current_fread_status.mFile->filelength )
		{
			current_fread_status.count = current_fread_status.mFile->filelength - current_fread_status.mFile->bytenum;
		}
		// Calculate start sector
		current_fread_status.sector = current_fread_status.mFile->bytenum / FAT_SECTOR_SIZE;

		// Offset to start copying data from first sector
		current_fread_status.offset = current_fread_status.mFile->bytenum % FAT_SECTOR_SIZE;
		current_fread_stage = FL_FREAD_STAGE_1;
	case FL_FREAD_STAGE_1:
		if (current_fread_status.bytesRead < current_fread_status.count)
		{
			if (current_fread_status.mFile->file_data.address != current_fread_status.sector)
			{
				// Flush un-written data to file
				if (current_fread_status.mFile->file_data.dirty)
				{
					current_fread_stage = FL_FREAD_STAGE_2;
				}
				else
				{
					current_fread_stage = FL_FREAD_STAGE_3;
				}
			}
			else
			{
				current_fread_stage = FL_FREAD_STAGE_4;
			}
		}
		else
		{
			current_fread_stage = FL_FREAD_STAGE_0;
			return current_fread_status.bytesRead;
		}
	case FL_FREAD_STAGE_2:
		if (current_fread_stage == FL_FREAD_STAGE_2)
		{
			switch (fl_fflush(current_fread_status.mFile))
			{
			case 0:
				current_fread_stage = FL_FREAD_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_fread_stage = FL_FREAD_STAGE_3;
		}
	case FL_FREAD_STAGE_3:
		if (current_fread_stage == FL_FREAD_STAGE_3)
		{
			switch (fatfs_sector_reader(&Fs, current_fread_status.mFile->startcluster, current_fread_status.sector, current_fread_status.mFile->file_data.sector))
			{
			case 0:
				// Read failed - out of range (probably)
				current_fread_stage = FL_FREAD_STAGE_0;
				return current_fread_status.bytesRead;
			case -1:
				return -1;
			}
			current_fread_status.mFile->file_data.address = current_fread_status.sector;
			current_fread_status.mFile->file_data.dirty = 0;
			current_fread_stage = FL_FREAD_STAGE_4;
		}
	case FL_FREAD_STAGE_4:
		// We have upto one sector to copy
		current_fread_status.copyCount = FAT_SECTOR_SIZE - current_fread_status.offset;

		// Only require some of this sector?
		if (current_fread_status.copyCount > (current_fread_status.count - current_fread_status.bytesRead))
		{
			current_fread_status.copyCount = (current_fread_status.count - current_fread_status.bytesRead);
		}
		// Copy to application buffer
		memcpy((unsigned char*)((unsigned char*)buffer + current_fread_status.bytesRead), (unsigned char*)(current_fread_status.mFile->file_data.sector + current_fread_status.offset), current_fread_status.copyCount);
	
		// Increase total read count 
		current_fread_status.bytesRead += current_fread_status.copyCount;

		// Increment file pointer
		current_fread_status.mFile->bytenum += current_fread_status.copyCount;

		// Move onto next sector and reset copy offset
		++current_fread_status.sector;
		current_fread_status.offset = 0;
		if (current_fread_status.bytesRead < current_fread_status.count)
		{
			current_fread_stage = FL_FREAD_STAGE_1;
			return -1;
		}
	}
	current_fread_stage = FL_FREAD_STAGE_0;
	return current_fread_status.bytesRead;
}

//-----------------------------------------------------------------------------
// fl_fseek: Seek to a specific place in the file
//-----------------------------------------------------------------------------
int fl_fseek( void *f, long offset, int origin )
{
	FL_FILE *file = (FL_FILE *)f;
	int res = -1;

	// If first call to library, initialise
	CHECK_FL_INIT();

	if (!file)
		return -1;

	if (origin == SEEK_END && offset != 0)
		return -1;

	FL_LOCK(&Fs);

	// Invalidate file buffer
	file->file_data.address = 0xFFFFFFFF;
	file->file_data.dirty = 0;

	if (origin == SEEK_SET)
	{
		file->bytenum = (unsigned long)offset;

		if (file->bytenum > file->filelength)
			file->bytenum = file->filelength;

		res = 0;
	}
	else if (origin == SEEK_CUR)
	{
		// Positive shift
		if (offset >= 0)
		{
			file->bytenum += offset;

			if (file->bytenum > file->filelength)
				file->bytenum = file->filelength;
		}
		// Negative shift
		else
		{
			// Make shift positive
			offset = -offset;

			// Limit to negative shift to start of file
			if ((unsigned long)offset > file->bytenum)
				file->bytenum = 0;
			else
				file->bytenum-= offset;
		}

		res = 0;
	}
	else if (origin == SEEK_END)
	{
		file->bytenum = file->filelength;
		res = 0;
	}
	else
		res = -1;

	FL_UNLOCK(&Fs);

	return res;
}
//-----------------------------------------------------------------------------
// fl_fgetpos: Get the current file position
//-----------------------------------------------------------------------------
int fl_fgetpos(void *f , unsigned long * position)
{
	FL_FILE *file = (FL_FILE *)f;

	if (!file)
		return -1;

	FL_LOCK(&Fs);

	// Get position
	*position = file->bytenum;

	FL_UNLOCK(&Fs);

	return 0;
}
//-----------------------------------------------------------------------------
// fl_ftell: Get the current file position
//-----------------------------------------------------------------------------
long fl_ftell(void *f)
{
	unsigned long pos = 0;

	fl_fgetpos(f, &pos);

	return (long)pos;
}
//-----------------------------------------------------------------------------
// fl_feof: Is the file pointer at the end of the stream?
//-----------------------------------------------------------------------------
int fl_feof(void *f)
{
	FL_FILE *file = (FL_FILE *)f;
	int res;

	if (!file)
		return -1;

	FL_LOCK(&Fs);

	if (file->bytenum == file->filelength)
		res = EOF;
	else
		res = 0;

	FL_UNLOCK(&Fs);

	return res;
}
//-----------------------------------------------------------------------------
// fl_fputc: Write a character to the stream
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fl_fputc(int c, void* f)
{
	unsigned char data = (unsigned char)c;
	int res;

	res = fl_fwrite(&data, 1, 1, f);
	switch (res)
	{
	case -2:
		return -1;
	case -1:
		return 0;
	}
	if (res == 1)
	{
		return 1;
	}
	return 0;
}
#endif

#ifdef FATFS_INC_WRITE_SUPPORT
typedef struct _fl_fwrite_status{
	FL_FILE* mFile;
	unsigned long sector;
	unsigned long offset;	
	unsigned long length;
	unsigned char* buffer;
	int dirtySector;
	unsigned long bytesWritten;
	unsigned long copyCount;
} fl_fwrite_status_t;

typedef enum{
	FL_FWRITE_STAGE_0 = 0,
	FL_FWRITE_STAGE_1,
	FL_FWRITE_STAGE_2,
	FL_FWRITE_STAGE_3,
	FL_FWRITE_STAGE_4,
	//FL_FWRITE_STAGE_4_1,
	FL_FWRITE_STAGE_5,
	FL_FWRITE_STAGE_6,
	FL_FWRITE_STAGE_7
} fl_fwrite_stage_t;

fl_fwrite_status_t current_fwrite_status;
fl_fwrite_stage_t old_fwrite_stage;
fl_fwrite_stage_t current_fwrite_stage = FL_FWRITE_STAGE_0;

//-----------------------------------------------------------------------------
// fl_fwrite: Write a block of data to the stream
// YEB: returns -1 if failed
// YEB: returns -2 if on process
// YEB: returns else if success
// YEB: Complete
//-----------------------------------------------------------------------------
int fl_fwrite(const void* __data, int __size, int __count, void* f )
{
	static const void* data;
	static int size;
	static int count;
	static UINT32 tempCluster;
	
	switch (current_fwrite_stage)
	{
	case FL_FWRITE_STAGE_0:
		data = __data;
		size = __size;
		count = __count;
		current_fwrite_status.mFile = (FL_FILE *)f;
		current_fwrite_status.length = (size * count);
		current_fwrite_status.buffer = (unsigned char*)data;
		current_fwrite_status.dirtySector = 0;
		current_fwrite_status.bytesWritten = 0;
	
		// If first call to library, initialise
		CHECK_FL_INIT();

		if (!current_fwrite_status.mFile)
		{
			return -1;
		}

		FL_LOCK(&Fs);

		// No write permissions
		if (!(current_fwrite_status.mFile->flags & FILE_WRITE))
		{
			FL_UNLOCK(&Fs);
			return -1;
		}

		// Append writes to end of file
		if (current_fwrite_status.mFile->flags & FILE_APPEND)
		{
			current_fwrite_status.mFile->bytenum = current_fwrite_status.mFile->filelength;
		}
		// Else write to current position

		// Calculate start sector
		current_fwrite_status.sector = current_fwrite_status.mFile->bytenum / FAT_SECTOR_SIZE;

		// Offset to start copying data from first sector
		current_fwrite_status.offset = current_fwrite_status.mFile->bytenum % FAT_SECTOR_SIZE;
		if (current_fwrite_status.bytesWritten < current_fwrite_status.length)
		{
			current_fwrite_stage = FL_FWRITE_STAGE_1;
		}
		else
		{
			break;
		}
	case FL_FWRITE_STAGE_1:
		if (current_fwrite_stage == FL_FWRITE_STAGE_1)
		{
			// We have up to one sector to copy
			current_fwrite_status.copyCount = FAT_SECTOR_SIZE - current_fwrite_status.offset;

			// Only require some of this sector?
			if (current_fwrite_status.copyCount > (current_fwrite_status.length - current_fwrite_status.bytesWritten))
			{
				current_fwrite_status.copyCount = (current_fwrite_status.length - current_fwrite_status.bytesWritten);
			}
			
			// Do we need to read a new sector?
			if (current_fwrite_status.mFile->file_data.address != current_fwrite_status.sector)
			{
				// Flush un-written data to file
				if (current_fwrite_status.mFile->file_data.dirty)
				{
					current_fwrite_stage = FL_FWRITE_STAGE_2;
				}
				else
				{
					current_fwrite_stage = FL_FWRITE_STAGE_3;
				}
			}
			else
			{
				current_fwrite_stage = FL_FWRITE_STAGE_6;
			}
		}
	case FL_FWRITE_STAGE_2:
		if (current_fwrite_stage == FL_FWRITE_STAGE_2)
		{
			switch (fl_fflush(current_fwrite_status.mFile))
			{
			case 0:
				FL_UNLOCK(&Fs);
				old_fwrite_stage = current_fwrite_stage;
				current_fwrite_stage = FL_FWRITE_STAGE_0;
				return -1;
			case -1:
				return -2;
			}
			current_fwrite_stage = FL_FWRITE_STAGE_3;
		}
	case FL_FWRITE_STAGE_3:
		if (current_fwrite_stage == FL_FWRITE_STAGE_3)
		{
			// If we plan to overwrite the whole sector, we don't need to read it first!
			if (current_fwrite_status.copyCount != FAT_SECTOR_SIZE)
			{
				// Read the appropriate sector
				// NOTE: This does not have succeed; if last sector of file
				// reached, no valid data will be read in, but write will 
				// allocate some more space for new data.
				current_fwrite_stage = FL_FWRITE_STAGE_4;
			}
			else
			{
				current_fwrite_stage = FL_FWRITE_STAGE_5;
			}
		}
	case FL_FWRITE_STAGE_4:
		if (current_fwrite_stage == FL_FWRITE_STAGE_4)
		{
			// why do we need to read. we should put an option to prevent extra reading at this point
			// we can check if this is the last block we have
			if (current_fwrite_status.mFile->isChunkRequired)
			{
				tempCluster = current_fwrite_status.mFile->LatestCluster;
			}
			else
			{
				tempCluster = current_fwrite_status.mFile->startcluster;
			}
			switch (fatfs_sector_reader(&Fs, tempCluster, current_fwrite_status.sector, current_fwrite_status.mFile->file_data.sector))
			{
			case 0:
				// YEB: should fix it later
				//current_fwrite_stage = FL_FWRITE_STAGE_4_1;
				current_fwrite_stage = FL_FWRITE_STAGE_5;
				break;
			case -1:
				return -2;
			default:
				current_fwrite_stage = FL_FWRITE_STAGE_5;
			}
		}
	/*
	case FL_FWRITE_STAGE_4_1:
		if (current_fwrite_stage == FL_FWRITE_STAGE_4_1)
		{
			switch (fatfs_find_empty_cluster_chunk(&Fs, &Fs.empty_chunk_start_cluster_number))
			{
			case 0:
				current_fwrite_stage = FL_FWRITE_STAGE_0;
				return 0;
			case -1:
				return -2;
			}
			Fs.empty_chunk_used_counter = 0;
			current_fwrite_stage = FL_FWRITE_STAGE_5;
		}
	*/
	case FL_FWRITE_STAGE_5:
		if (current_fwrite_stage == FL_FWRITE_STAGE_5)
		{
			current_fwrite_status.mFile->file_data.address = current_fwrite_status.sector;
			current_fwrite_status.mFile->file_data.dirty = 0;
			current_fwrite_stage = FL_FWRITE_STAGE_6;
		}
	case FL_FWRITE_STAGE_6:
		// Copy from application buffer into sector buffer
		memcpy((unsigned char*)(current_fwrite_status.mFile->file_data.sector + current_fwrite_status.offset), (unsigned char*)(current_fwrite_status.buffer + current_fwrite_status.bytesWritten), current_fwrite_status.copyCount);
	
		// Mark buffer as dirty
		current_fwrite_status.mFile->file_data.dirty = 1;
		
		// Increase total read count 
		current_fwrite_status.bytesWritten += current_fwrite_status.copyCount;
	
		// Increment file pointer
		current_fwrite_status.mFile->bytenum += current_fwrite_status.copyCount;
	
		// Move onto next sector and reset copy offset
		++current_fwrite_status.sector;
		current_fwrite_status.offset = 0;
		if (current_fwrite_status.bytesWritten < current_fwrite_status.length)
		{
			current_fwrite_stage = FL_FWRITE_STAGE_1;
			return -2;
		}
	}
	
	// Write increased extent of the file?
	if (current_fwrite_status.mFile->bytenum > current_fwrite_status.mFile->filelength)
	{
		// Increase file size to new point
		current_fwrite_status.mFile->filelength = current_fwrite_status.mFile->bytenum;
		// We are changing the file length and this 
		// will need to be writen back at some point
		current_fwrite_status.mFile->filelength_changed = 1;
	}
	FL_UNLOCK(&Fs);
	current_fwrite_stage = FL_FWRITE_STAGE_0;
	return (size * count);
}
#endif


#ifdef FATFS_INC_WRITE_SUPPORT

#ifdef __CLUSTER_CHUNK_RESERVATION_ENABLED__
UINT32 search_free_cluster_chunk(void)
{
	UINT32 temp;
	while (fatfs_find_empty_cluster_chunk(&Fs, Fs.rootdir_first_cluster, &temp)  == -1);
	return temp;
}
#endif

//-----------------------------------------------------------------------------
// fl_fputs: Write a character string to the stream
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
//-----------------------------------------------------------------------------
int fl_fputs(const char * str, void *f)
{
	int len = (int)strlen(str);
	int res = fl_fwrite(str, 1, len, f);

	return res;
}
#endif


#ifdef FATFS_INC_WRITE_SUPPORT

typedef struct _fl_remove_status{
	FL_FILE* mFile;
} fl_remove_status_t;

typedef enum{
	FL_REMOVE_STAGE_0 = 0,
	FL_REMOVE_STAGE_1,
	FL_REMOVE_STAGE_2,
	FL_REMOVE_STAGE_3,
	FL_REMOVE_STAGE_4
} fl_remove_stage_t;

fl_remove_status_t current_remove_status;
fl_remove_stage_t current_remove_stage = FL_REMOVE_STAGE_0;

//-----------------------------------------------------------------------------
// fl_remove: Remove a file from the filesystem
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
int fl_remove(const char* __filename)
{
	//FL_FILE* mFile;
	//int res = -1;
	static const char* filename;
	static FL_FILE* tempFile[1];
	
	switch (current_remove_stage)
	{
	case FL_REMOVE_STAGE_0:
		filename = __filename;
		FL_LOCK(&Fs);
		current_remove_stage = FL_REMOVE_STAGE_1;
	case FL_REMOVE_STAGE_1:
		// Use read_file as this will check if the file is already open!
		switch(fl_fopen((char*)filename, "r", (void**)&tempFile[0], FALSE))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_remove_stage = FL_REMOVE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_remove_status.mFile = tempFile[0];
		if (current_remove_status.mFile)
		{
			current_remove_stage = FL_REMOVE_STAGE_2;
		}
		else
		{
			FL_UNLOCK(&Fs);
			current_remove_stage = FL_REMOVE_STAGE_0;
			return 0;
		}
	case FL_REMOVE_STAGE_2:
		// Delete allocated space
		switch (fatfs_free_cluster_chain(&Fs, current_remove_status.mFile->startcluster))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_remove_stage = FL_REMOVE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_remove_stage = FL_REMOVE_STAGE_3;
	case FL_REMOVE_STAGE_3:
		// Remove directory entries
		switch (fatfs_mark_file_deleted(&Fs, current_remove_status.mFile->parentcluster, (char*)current_remove_status.mFile->shortfilename))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_remove_stage = FL_REMOVE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_remove_stage = FL_REMOVE_STAGE_4;
	case FL_REMOVE_STAGE_4:
		// Close the file handle (this should not write anything to the file
		// as we have not changed the file since opening it!)
		switch (fl_fclose(current_remove_status.mFile))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_remove_stage = FL_REMOVE_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		FL_UNLOCK(&Fs);
		current_remove_stage = FL_REMOVE_STAGE_0;
		return 1;
	}
	FL_UNLOCK(&Fs);
	current_remove_stage = FL_REMOVE_STAGE_0;
	return 0;
}
#endif

typedef enum{
	FL_LISTDIR_STAGE_0 = 0,
	FL_LISTDIR_STAGE_1,
	FL_LISTDIR_STAGE_2,
	FL_LISTDIR_STAGE_3,
	FL_LISTDIR_STAGE_4
} fl_listdir_stage_t;

fl_listdir_stage_t current_listdir_stage = FL_LISTDIR_STAGE_0;

//-----------------------------------------------------------------------------
// fl_listdirectory: List a directory based on a path
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
//-----------------------------------------------------------------------------
int fl_listdirectory(const char* __path)
{
	static const char* path;
	static struct fs_dir_list_status dirstat;
	static int filenumber;
	static struct fs_dir_ent dirent;
	
	switch (current_listdir_stage)
	{
	case FL_LISTDIR_STAGE_0:
		path = __path;
		filenumber = 0;
		
		// If first call to library, initialise
		CHECK_FL_INIT();

		FL_LOCK(&Fs);

		FAT_PRINTF(("\r\nNo.             Filename\r\n"));
		current_listdir_stage = FL_LISTDIR_STAGE_1;
	case FL_LISTDIR_STAGE_1:
		switch (fl_list_opendir(path, &dirstat))
		{
		case 0:
			FL_UNLOCK(&Fs);
			current_listdir_stage = FL_LISTDIR_STAGE_0;
			return 0;
		case -1:
			return -1;
		}
		current_listdir_stage = FL_LISTDIR_STAGE_2;
	case FL_LISTDIR_STAGE_2:
		switch (fl_list_readdir(&dirstat, &dirent))
		{
		case 0:
			current_listdir_stage = FL_LISTDIR_STAGE_3;
			break;
		case -1:
			return -1;
		default:
			if (dirent.is_dir)
			{
				FAT_PRINTF(("%d - %s <DIR> (0x%08lx)\r\n",++filenumber, dirent.filename, dirent.cluster));
			}
			else
			{
				FAT_PRINTF(("%d - %s [%d bytes] (0x%08lx)\r\n",++filenumber, dirent.filename, dirent.size, dirent.cluster));
			}
			return -1; 
		}
	}
	FL_UNLOCK(&Fs);
	current_listdir_stage = FL_LISTDIR_STAGE_0;
	return 0;
}

typedef enum{
    FL_CREATE_DIR_STAGE_0 = 0,
    FL_CREATE_DIR_STAGE_1
} fl_create_dir_stage_t;

fl_create_dir_stage_t current_fl_dir_create_stage = FL_CREATE_DIR_STAGE_0;

//-----------------------------------------------------------------------------
// fl_createdirectory: Create a directory based on a path
//-----------------------------------------------------------------------------
int fl_createdirectory(const char *path)
{
	int res;

    switch (current_fl_dir_create_stage)
    {
    case FL_CREATE_DIR_STAGE_0:
	    // If first call to library, initialise
	    CHECK_FL_INIT();
	    FL_LOCK(&Fs);
        current_fl_dir_create_stage = FL_CREATE_DIR_STAGE_1;
    case FL_CREATE_DIR_STAGE_1:
        res =_create_directory((char*)path, FALSE);
	    if (res > -1)
        {
            FL_UNLOCK(&Fs);
            current_fl_dir_create_stage = FL_CREATE_DIR_STAGE_0;
        }
    }
	return res;
}

typedef enum{
	FL_LIST_OPENDIR_STAGE_0 = 0,
	FL_LIST_OPENDIR_STAGE_1,
	FL_LIST_OPENDIR_STAGE_2
} fl_list_opendir_stage_t;

fl_list_opendir_stage_t current_list_opendir_stage = FL_LIST_OPENDIR_STAGE_0;

//-----------------------------------------------------------------------------
// fl_list_opendir: Opens a directory for listing
// YEB: returns 0 if failed
// YEB: returns -1 if on process
// YEB: returns else if success
// YEB: Comp
//-----------------------------------------------------------------------------
int fl_list_opendir(const char* __path, struct fs_dir_list_status* __dirls)
{
	const char* path;
	struct fs_dir_list_status* dirls;
	int levels;
	UINT32 cluster;

	switch (current_list_opendir_stage)
	{
	case FL_LIST_OPENDIR_STAGE_0:
		path = __path;
		dirls = __dirls;
		cluster = FAT32_INVALID_CLUSTER;
		// If first call to library, initialise
		CHECK_FL_INIT();

		FL_LOCK(&Fs);

		levels = fatfs_total_path_levels((char*)path) + 1;

		// If path is in the root dir
		if (levels == 0)
		{
			cluster = fatfs_get_root_cluster(&Fs);
			current_list_opendir_stage = FL_LIST_OPENDIR_STAGE_2;
		}
		else
		{
			current_list_opendir_stage = FL_LIST_OPENDIR_STAGE_1;
		}
	case FL_LIST_OPENDIR_STAGE_1:
		if (current_list_opendir_stage == FL_LIST_OPENDIR_STAGE_1)
		{
			// Find parent directory start cluster
			switch (_open_directory((char*)path, &cluster))
			{
			case 0:
				FL_UNLOCK(&Fs);
				current_list_opendir_stage = FL_LIST_OPENDIR_STAGE_0;
				return 0;
			case -1:
				return -1;
			}
			current_list_opendir_stage = FL_LIST_OPENDIR_STAGE_2;
		}
	case FL_LIST_OPENDIR_STAGE_2:
		fatfs_list_directory_start(&Fs, dirls, cluster);
	}
	FL_UNLOCK(&Fs);
	return cluster != FAT32_INVALID_CLUSTER ? 1 : 0;
}

//-----------------------------------------------------------------------------
// fl_list_readdir: Get next item in directory
// YEB: returns 0 if failed
// YEB: returns -1 if on process
//-----------------------------------------------------------------------------
int fl_list_readdir(struct fs_dir_list_status *dirls, struct fs_dir_ent *entry)
{
	int res = 0;

	// If first call to library, initialise
	CHECK_FL_INIT();

	FL_LOCK(&Fs);

	res = fatfs_list_directory_next(&Fs, dirls, entry);

	FL_UNLOCK(&Fs);

	return res;
}
//-----------------------------------------------------------------------------
// fl_is_dir: Is this a directory?
//-----------------------------------------------------------------------------
int fl_is_dir(const char *path)
{
	unsigned long cluster = 0;

	if (_open_directory((char*)path, &cluster))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

