#ifdef __LOCAL_STORE_ENABLED__

#include "switcher.h"
#include "local_store.h"
#include "dma_mmc.h"
#include "fat_filelib.h"

#ifdef PROBE_ENABLED
#include "probe.h"
#endif

#ifdef __LOCAL_STORE_ACCEL_MODE__
#include "convert.h"

#define MAX_LS_ACCEL_ITEMS 140

typedef struct __LS_accel_item{
    RTC_TIME time;
    ACC_SENSOR sensor_data;
} LS_accel_item_t;

LS_accel_item_t LS_accel_buffer[MAX_LS_ACCEL_ITEMS];
unsigned char LS_accel_queue_in_count = 0;
unsigned char LS_accel_queue_out_count = 0;
unsigned char LS_num_accel_queue = 0;
char LS_accel_string[50];
unsigned char LS_accel_string_size = 0;
SD_status_t mSD_status = SD_OK;

#else
#include "byte_queue.h"
#endif

#include "stdio.h"
#include "string.h"

#ifdef FRAM_ENABLED 
	#include "fram_wr.h"
    #pragma message("FRAM Cache enabled")
#endif


typedef enum{
	LOS_NONE = 0,
	LOS_CREATE_START,
	LOS_CREATE_DONE,
	LOS_OPEN_START,
	LOS_OPEN_DONE,
#ifndef __LOCAL_STORE_ACCEL_MODE__
	LOS_WRITE_HEAD,
	LOS_WRITE_ALL,
#else
    LOS_WRITE,
#endif
	LOS_CLOSE_START,
} LOS_state_t;

unsigned char LOS_tid = 0;

#ifndef __LOCAL_STORE_ACCEL_MODE__
// buffer to buy time to create or open a file
unsigned char LOS_write_buffer[LOS_WRITE_BUFFER_SIZE];
ByteQueue_t LOS_queue;
#endif

FL_FILE* mFile[MAX_NUM_FILES] = {0};
LOS_state_t st = LOS_NONE;
int LOS_open_file_index = 0;

static void LOS_proc(unsigned char tid, void* msg);
static int LS_get_accel_string(void);


int media_init()
{
	int i;
	i = MMC_MemCardInit();
	if (i == 0)
	{
		return 1;
	}
	return 0;
}

int media_read(unsigned long sector, unsigned char *buffer)
{
	// ...
	return DMA_MMC_ReadSector(sector, buffer, 0);
}

int media_write(unsigned long sector, unsigned char *buffer)
{
	// ...
	return DMA_MMC_WriteSector(sector, buffer);
}

void LOS_init(void)
{	
	if (media_init() == 0)
	{
        mSD_status = SD_ERR_INIT;
		printf("ERROR: Media init failed\n");
		return;
	}
	
	// Attach media access functions to library
    if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
    {
        mSD_status = SD_ERR_ATTACH;
		printf("ERROR: Media attach failed\n");
		return; 
    }

#ifndef __LOCAL_STORE_ACCEL_MODE__
	BQ_init(&LOS_queue, LOS_write_buffer, sizeof(LOS_write_buffer));
#endif
    
	LOS_tid = registerTask(LOS_proc, 0, 0, 0xFF);

#ifdef FRAM_ENABLED	
    FRAM_WR_init();
#endif
	
	// Delete File
	int i = 0;
    while ((i = fl_remove("/file.txt")) == -1);
	if (!i)
	{
		//printf("ERROR: Delete file failed\n");
    }
}

void LOS_proc(unsigned char tid, void* msg)
{
    //static int file_num = 0;
	//static int size;
	static LOS_state_t last_st;
	int i;
    
#ifndef __LOCAL_STORE_ACCEL_MODE__
	if (LOS_queue.numBytes > 0)
#else
    if (LS_num_accel_queue > 0)
#endif
	{
		switch (st)
		{
		case LOS_NONE:
			//probe_high();
			last_st = st;
			st = LOS_CREATE_START;
		case LOS_CREATE_START:
			i = fl_fopen("/file.txt", "w", (void**)&mFile[LOS_open_file_index], TRUE);
			switch (i)
			{
			case 0:
				st = LOS_OPEN_START;
				break;
			case -1:
				return;
			default:
				if (mFile[LOS_open_file_index])
				{
					last_st = st;
					st = LOS_CREATE_DONE;
				}
				else
				{
					st = LOS_OPEN_START;
				}
			}
		case LOS_OPEN_START:
			if (st == LOS_OPEN_START)
			{
#ifdef PROBE_ENABLED
				probe_low();
#endif
				i = fl_fopen("/file.txt", "w", (void**)mFile[LOS_open_file_index], TRUE);
				switch (i)
				{
				case 0:
					st = LOS_NONE;
					return;
				case -1:
					return;
				default:
					if (mFile[LOS_open_file_index])
					{
						last_st = st;
						st = LOS_OPEN_DONE;
					}
					else
					{
						st = LOS_NONE;
						return;
					}
				}
			}
		case LOS_CREATE_DONE:
		case LOS_OPEN_DONE:
			//probe_low();
			last_st = st;
#ifndef __LOCAL_STORE_ACCEL_MODE__
			if (LOS_queue.outCount == LOS_queue.inCount)
			{
				st = LOS_WRITE_HEAD;
			}
			else if (LOS_queue.outCount > LOS_queue.inCount)
			{
				size = LOS_queue.maxBytes - LOS_queue.outCount;
				st = LOS_WRITE_HEAD;
			}
			else
			{
				size = LOS_queue.numBytes;
				st = LOS_WRITE_ALL;
			}
		case LOS_WRITE_HEAD:
			if (st == LOS_WRITE_HEAD)
			{
				i = fl_fwrite(&LOS_queue.buffer[LOS_queue.outCount], 1, size, mFile[LOS_open_file_index]);
				switch (i)
				{
				case -1:
					printf("ERROR: Write file failed\n");
					st = last_st;
					return;
				case -2:
					return;
				}
				st = last_st;
				if (i == size)
				{
					LOS_queue.numBytes -= size;
					LOS_queue.outCount = 0;
				}
				else
				{
					printf("ERROR: Write file failed\n");
				}
				return;
			}
		case LOS_WRITE_ALL:
			if (st == LOS_WRITE_ALL)
			{
				i = fl_fwrite(&LOS_queue.buffer[LOS_queue.outCount], 1, size, mFile[LOS_open_file_index]);
				switch (i)
				{
				case -1:
					printf("ERROR: Write file failed\n");
					st = last_st;
					return;
				case -2:
					return;
				default:
					if (i == size)
					{
						LOS_queue.numBytes -= size;
						LOS_queue.outCount += size;
#ifdef PROBE_ENABLED
						probe2_change(0);
#endif
						if (mFile[LOS_open_file_index]->filelength > MAX_FILE_SIZE)
						{
							st = LOS_CLOSE_START;
						}
						else
						{
							st = last_st;
							return;
						}
					}
					else
					{
						printf("ERROR: Write file failed\n");
						st = last_st;
						return;
					}
				}
			}
#else 
            if (LS_get_accel_string())
            {
                st = LOS_WRITE;
            }
            else
            {
                st = last_st;
                return;
            }
        case LOS_WRITE:
            if (st == LOS_WRITE)
            {
                i = fl_fwrite(LS_accel_string, 1, LS_accel_string_size, mFile[LOS_open_file_index]);
                switch (i)
                {
                case -1:
                    printf("ERROR: Write file failed\n");
                    st = last_st;
                    return;
                case -2:
                    return;
                }
                st = last_st;
                if (i == LS_accel_string_size)
                {
                    LS_accel_queue_out_count = (LS_accel_queue_out_count + 1) % MAX_LS_ACCEL_ITEMS;
                    --LS_num_accel_queue;
#ifdef PROBE_ENABLED
                    probe2_change(0);
#endif
					if (mFile[LOS_open_file_index]->filelength > MAX_FILE_SIZE)
					{
						st = LOS_CLOSE_START;
					}
					else
					{
						st = last_st;
						return;
					}
                }
                else
                {
                    st = last_st;
                    return;
                }
            }
#endif
		case LOS_CLOSE_START:
			i = fl_fclose(mFile[LOS_open_file_index]);
			switch (i)
			{
			case 0:
				printf("ERROR: Close file failed\n");
				return;
			case -1:
				i = 0;
				return;
			}
			st = LOS_NONE;
		}
	}
}

#ifndef __LOCAL_STORE_ACCEL_MODE__
int LOS_write(unsigned char* data, int length)
{
	/*
	int i;
	if ((LOS_queue.numBytes + length) <= LOS_queue.maxBytes)
	{
		for (i = 0; i < length; ++i)
		{
			BQ_enqueue(&LOS_queue, data[i]);
		}
		return 1;
	}
	return 0;
	*/
	return BQ_enqueue_msg(&LOS_queue, data, length);
}

int LOS_read(unsigned char* data, int length)
{
	return 0;
}

#else
int LOS_accel_write(RTC_TIME* time, ACC_SENSOR* s_data)
{
    if (LS_num_accel_queue < MAX_LS_ACCEL_ITEMS)
    {
        memcpy(&LS_accel_buffer[LS_accel_queue_in_count].time, time, sizeof(RTC_TIME));
        memcpy(&LS_accel_buffer[LS_accel_queue_in_count].sensor_data, s_data, sizeof(ACC_SENSOR));
        LS_accel_queue_in_count = (LS_accel_queue_in_count + 1) % MAX_LS_ACCEL_ITEMS;
        ++LS_num_accel_queue;
        return 1;
    }
    return 0;
}

int LS_get_accel_string(void)
{
    RTC_TIME* time;
    ACC_SENSOR* sensor;
    
    time = &LS_accel_buffer[LS_accel_queue_out_count].time;
    sensor = &LS_accel_buffer[LS_accel_queue_out_count].sensor_data;
    
    LS_accel_string[0] = '[';
	LS_accel_string_size = 1;
	LS_accel_string_size += getUNumString(sensor->CurSeq, &LS_accel_string[1]);
	LS_accel_string[LS_accel_string_size++] = ']';
	LS_accel_string[LS_accel_string_size++] = ' ';
	LS_accel_string_size += getNumString(time->Hour, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = ':';
	LS_accel_string_size += getNumString(time->Minute, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = ':';
	LS_accel_string_size += getNumString(time->Second, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = '.';
	LS_accel_string_size += getNumString(time->MSecond, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = ' ';
	LS_accel_string_size += getNumString(sensor->Acc[ACC_X_AXIS].Data, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = ' ';
	LS_accel_string_size += getNumString(sensor->Acc[ACC_Y_AXIS].Data, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = ' ';
	LS_accel_string_size += getNumString(sensor->Acc[ACC_Z_AXIS].Data, &LS_accel_string[LS_accel_string_size]);
	LS_accel_string[LS_accel_string_size++] = '\n';
	LS_accel_string[LS_accel_string_size] = 0;
	
    return 1;
}
#endif

#endif
