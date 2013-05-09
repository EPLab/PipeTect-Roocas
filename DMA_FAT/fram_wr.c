#include "fram_wr.h"
#include "fram.h"
#include "byte_queue.h"
#include "switcher.h"
#include "string.h"
#include "dma_mmc.h"

FRAM_WR_control_data_control_center_t FRAM_WR_data_hashtable;
FRAM_WR_stage_t FRAM_WR_current_stage = FRAM_WR_STAGE_NONE;

unsigned char FRAM_WR_done;
unsigned short FRAM_WR_write_num = 0;
unsigned short FRAM_WR_write_count = 0;
unsigned int FRAM_WR_bytes_to_write;
unsigned long FRAM_WR_current_bl_addr;
unsigned long FRAM_WR_current_hash_val;
unsigned char* FRAM_WR_current_hold_data = 0;
//unsigned char incoming_buffer[FRAM_WR_BUF_SIZE];

unsigned char FRAM_WR_in_buffer[FRAM_WR_CACHE_BL_SIZE];
unsigned long FRAM_WR_in_buf_addr;
unsigned char FRAM_WR_out_buffer[FRAM_WR_CACHE_BL_SIZE];
unsigned long FRAM_WR_out_buf_addr;

unsigned char FRAM_WR_done = 0;

unsigned char FRAM_WR_tid = 0;

#ifdef FRAM_DEBUG_ENABLED
int FRAM_WR_read_try_count = 0;
int FRAM_WR_read_miss_count = 0;
int FRAM_WR_read_destage_count = 0;
int FRAM_WR_write_try_count = 0;
int FRAM_WR_write_miss_count = 0;
int FRAM_WR_write_destage_count = 0;
#endif


//static unsigned long FRAM_WR_get_ctrl_item_addr(unsigned char index)
//{
//	return (FRAM_WR_CONTROL_BL_STR + (((unsigned long)index) << 4));
//}
//static unsigned long FRAM_WR_get_ref_bits_addr(void)
//{
//	return (FRAM_WR_CONTROL_BL_STR + sizeof(FRAM_WR_control_bl_item_t) * FRAM_WR_DATA_BL_MAX);
//}

//static unsigned long FRAM_WR_get_last_accessed_block_item_addr(void)
//{
//	return (FRAM_WR_CONTROL_BL_STR + sizeof(FRAM_WR_control_bl_item_t) * FRAM_WR_DATA_BL_MAX) + 2;
//}

static void FRAM_WR_CB(void)
{
	if (FRAM_get_state() == 0)
	{
		FRAM_WR_done = 1;
	}
}

typedef enum{
    FRAM_WR_READ_STAGE_0 = 0,
    FRAM_WR_READ_HIT_STAGE_1,
    FRAM_WR_READ_HIT_STAGE_2,
    FRAM_WR_READ_DESTAGE_STAGE_1,
    FRAM_WR_READ_DESTAGE_STAGE_2,
    FRAM_WR_READ_DESTAGE_STAGE_3,
    FRAM_WR_READ_MISS_STAGE_1,
    FRAM_WR_READ_MISS_STAGE_2,
    FRAM_WR_READ_MISS_STAGE_3,
    FRAM_WR_READ_MISS_STAGE_4,
    FRAM_WR_READ_MISS_STAGE_5,
} FRAM_WR_READ_stage_t;

FRAM_WR_READ_stage_t FRAM_WR_READ_current_stage = FRAM_WR_READ_STAGE_0;

#pragma inline
static unsigned long FRAM_WR_calCacheAddr(unsigned long addr, unsigned char item_num, unsigned char offset)
{
    return ((((addr % FRAM_WR_CACHE_GR_NUM) << FRAM_WR_ASSOCIATIVITY_BIT_NUM) + item_num) << FRAM_WR_CACHE_BL_BITS) + offset;
}

int FRAM_WR_update_control_block(unsigned char gr_num)
{
    FRAM_WR_control_bl_assoc_t* tempGR = &FRAM_WR_data_hashtable.gr[gr_num];
    return FRAM_Write(FRAM_WR_CONTROL_BL_STR + sizeof(FRAM_WR_control_bl_assoc_t) * gr_num,
                      (unsigned char*)tempGR, 6, &FRAM_WR_CB);
}

int FRAM_WR_update_item(unsigned char gr_num)
{
    FRAM_WR_control_bl_assoc_t* tempGR = &FRAM_WR_data_hashtable.gr[gr_num];
    return FRAM_Write(FRAM_WR_CONTROL_BL_STR + sizeof(FRAM_WR_control_bl_assoc_t) * gr_num + sizeof(FRAM_WR_control_bl_item_t) * tempGR->newest_item + 6,
                      (unsigned char*)(&tempGR->item[tempGR->newest_item]),
			  		  sizeof(FRAM_WR_control_bl_item_t), &FRAM_WR_CB);
}
    

int FRAM_WR_read(unsigned long __addr, int __offset, unsigned char* __data, int __length, unsigned char __backGroundFRAM)
{
    static unsigned long addr;
    static int offset;
    static unsigned char* data;
    static int length;
    static unsigned char backGroundFRAM;
    static FRAM_WR_control_bl_assoc_t* gr;
    
	unsigned char i;
	
    switch (FRAM_WR_READ_current_stage)
    {
    case FRAM_WR_READ_STAGE_0:
        addr = __addr;
        offset = __offset;
        data = __data;
        length = __length;
        backGroundFRAM = __backGroundFRAM;
        
        // search the table for the block resides in it
        i = 0;
        gr = &FRAM_WR_data_hashtable.gr[addr % FRAM_WR_CACHE_GR_NUM];
        while (i < FRAM_WR_ASSOCIATIVITY)
        {
            if (gr->item[gr->newest_item].sector_addr == addr)
            {
                gr->ref_bits |= (1 << gr->newest_item);
                FRAM_WR_done = 0;
                FRAM_WR_READ_current_stage = FRAM_WR_READ_HIT_STAGE_1;
                break;
            }
            gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
            ++i;
        }
        // couldn't find the sector in the cache -> cache miss
        // at this point all the ref_bits must have been cleared
        // decide what to replace
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_STAGE_0)
        {
            i = 0;
            while (i < (FRAM_WR_ASSOCIATIVITY << 1))
            {
                if ((gr->dirty_bits & (1 << gr->newest_item)) == 0)
                {
                    if ((gr->ref_bits & (1 << gr->newest_item)) == 0)
                    {
                        FRAM_WR_READ_current_stage = FRAM_WR_READ_MISS_STAGE_1;
                        break;
                    }
                }
                gr->ref_bits &= ~(1 << gr->newest_item);
                gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
                ++i;
            }
        }
        // everything is dirty now
        // the next of newest will be destaged 
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_STAGE_0)
        {
            gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
            FRAM_WR_READ_current_stage = FRAM_WR_READ_DESTAGE_STAGE_1;
            FRAM_WR_done = 0;
        }
    case FRAM_WR_READ_HIT_STAGE_1:
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_HIT_STAGE_1)
        {
            if (FRAM_Read(FRAM_WR_calCacheAddr(addr, gr->newest_item, offset), data, length, &FRAM_WR_CB))
            {
                FRAM_WR_READ_current_stage = FRAM_WR_READ_HIT_STAGE_2;
            }
            else
            {
                return -1;
            }
        }
    case FRAM_WR_READ_HIT_STAGE_2:
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_HIT_STAGE_2)
        {
            if (FRAM_WR_done == 0)
            {
                return -1;
            }
            //gr->ref_bits |= (1 << gr->newest_item);
            //gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
            FRAM_WR_READ_current_stage = FRAM_WR_READ_STAGE_0;
            return 1;
        }
    case FRAM_WR_READ_DESTAGE_STAGE_1:
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_DESTAGE_STAGE_1)
        {
            if (FRAM_Read(FRAM_WR_calCacheAddr(addr, gr->newest_item, 0), FRAM_WR_out_buffer, FRAM_WR_CACHE_BL_SIZE, &FRAM_WR_CB))
            {
                FRAM_WR_READ_current_stage = FRAM_WR_READ_DESTAGE_STAGE_2;
            }
            else
            {
                return -1;
            }
        }
    case FRAM_WR_READ_DESTAGE_STAGE_2:
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_DESTAGE_STAGE_2)
        {
            if (FRAM_WR_done == 0)
            {
                return -1;
            }
            FRAM_WR_READ_current_stage = FRAM_WR_READ_DESTAGE_STAGE_3;
        }
    case FRAM_WR_READ_DESTAGE_STAGE_3:
        if (FRAM_WR_READ_current_stage == FRAM_WR_READ_DESTAGE_STAGE_3)
        {
            switch (DMA_MMC_WriteSector(gr->item[gr->newest_item].sector_addr, FRAM_WR_out_buffer))
            {
            case 0:
                FRAM_WR_READ_current_stage = FRAM_WR_READ_STAGE_0;
                return 0;
            case -1:
                return -1;
            }
            gr->dirty_bits &= ~(1 << gr->newest_item);
            FRAM_WR_READ_current_stage = FRAM_WR_READ_MISS_STAGE_1;
        }
    case FRAM_WR_READ_MISS_STAGE_1:
        switch (DMA_MMC_ReadSector(addr, FRAM_WR_in_buffer, 0))
        {
        case 0:
            FRAM_WR_READ_current_stage = FRAM_WR_READ_STAGE_0;
            return 0;
        case -1:
            return -1;
        }
        gr->ref_bits |= (1 << gr->newest_item);
        memcpy(data, FRAM_WR_in_buffer + offset, length);
        FRAM_WR_done = 0;
        FRAM_WR_READ_current_stage = FRAM_WR_READ_MISS_STAGE_2;
    case FRAM_WR_READ_MISS_STAGE_2:
        if (FRAM_Write(FRAM_WR_calCacheAddr(addr, gr->newest_item, 0), FRAM_WR_in_buffer, FRAM_WR_CACHE_BL_SIZE, &FRAM_WR_CB))
        {
            FRAM_WR_READ_current_stage = FRAM_WR_READ_MISS_STAGE_3;
        }
        else
        {
            return -1;
        }
    case FRAM_WR_READ_MISS_STAGE_3:
        if (FRAM_WR_done)
        {
            gr->ref_bits |= (1 << gr->newest_item);
            //FRAM_WR_done = 0;
            //FRAM_Read(FRAM_WR_calCacheAddr(addr, gr->newest_item, 0), FRAM_WR_out_buffer, FRAM_WR_CACHE_BL_SIZE, &FRAM_WR_CB);
            //while (FRAM_WR_done == 0);
            //FRAM_WR_done = 0;
            //FRAM_Read(FRAM_WR_calCacheAddr(addr, gr->newest_item, offset), data, length, &FRAM_WR_CB);
            //while (FRAM_WR_done == 0);
            //gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
            gr->item[gr->newest_item].sector_addr = addr;
            FRAM_WR_done = 0;
            if (FRAM_WR_update_item(addr % FRAM_WR_CACHE_GR_NUM))
            {
                FRAM_WR_READ_current_stage = FRAM_WR_READ_MISS_STAGE_4;
            }
            //FRAM_WR_READ_current_stage = FRAM_WR_READ_HIT_STAGE_1;
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    case FRAM_WR_READ_MISS_STAGE_4:
        if (FRAM_WR_done)
        {
            FRAM_WR_done = 0;
            if (FRAM_WR_update_control_block(addr % FRAM_WR_CACHE_GR_NUM))
            {
                FRAM_WR_READ_current_stage = FRAM_WR_READ_MISS_STAGE_5;
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
    case FRAM_WR_READ_MISS_STAGE_5:
        if (FRAM_WR_done)
        {
            FRAM_WR_READ_current_stage = FRAM_WR_READ_STAGE_0;
            return 1;
        }
        return -1;
    }
    return 0;
}

typedef enum{
    FRAM_WR_WRITE_STAGE_0 = 0,
    FRAM_WR_WRITE_HIT_STAGE_1,
    FRAM_WR_WRITE_HIT_STAGE_2,
    FRAM_WR_WRITE_DESTAGE_STAGE_1,
    FRAM_WR_WRITE_DESTAGE_STAGE_2,
    FRAM_WR_WRITE_DESTAGE_STAGE_3,
    FRAM_WR_WRITE_MISS_STAGE_1,
    FRAM_WR_WRITE_MISS_STAGE_2,
    FRAM_WR_WRITE_MISS_STAGE_3
} FRAM_WR_WRITE_stage_t;

FRAM_WR_WRITE_stage_t FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_STAGE_0;

int FRAM_WR_write(unsigned long __addr, int __offset, unsigned char* __data, int __length, unsigned char __backGroundFRAM)
{
    static unsigned long addr;
    static int offset;
    static unsigned char* data;
    static int length;
    static unsigned char backGroundFRAM;
    static FRAM_WR_control_bl_assoc_t* gr;
    
	unsigned char i;
	
    switch (FRAM_WR_WRITE_current_stage)
    {
    case FRAM_WR_WRITE_STAGE_0:
        addr = __addr;
        offset = __offset;
        data = __data;
        length = __length;
        backGroundFRAM = __backGroundFRAM;
        
        // search the table for the block resides in it
        i = 0;
        gr = &FRAM_WR_data_hashtable.gr[addr % FRAM_WR_CACHE_GR_NUM];
        while (i < FRAM_WR_ASSOCIATIVITY)
        {
            gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
            if (gr->item[gr->newest_item].sector_addr == addr)
            {
                gr->ref_bits |= (1 << gr->newest_item);
#ifdef FRAM_STATIC_META_DATA_ENABLED
                FRAM_WR_write_ref_bits(addr % FRAM_WR_CACHE_GR_NUM);
#endif
                FRAM_WR_done = 0;
                FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_HIT_STAGE_1;
                break;
            }
            ++i;
        }
        // couldn't find the sector in the cache -> cache miss
        // at this point all the ref_bits must have been cleared
        // decide what to replace
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_STAGE_0)
        {
            i = 0;
            while (i < (FRAM_WR_ASSOCIATIVITY << 1))
            {
                gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
                if (gr->dirty_bits & (1 << gr->newest_item) == 0)
                {
                    if (gr->ref_bits & (1 << gr->newest_item) == 0)
                    {
                        FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_MISS_STAGE_1;
                        break;
                    }
                }
                gr->ref_bits &= ~(1 << gr->newest_item);
                ++i;
            }
        }
        // everything is dirty now
        // the next of newest will be destaged 
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_STAGE_0)
        {
            gr->newest_item = (gr->newest_item + 1) % FRAM_WR_ASSOCIATIVITY;
            FRAM_WR_done = 0;
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_DESTAGE_STAGE_1;
        }
    case FRAM_WR_WRITE_HIT_STAGE_1:
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_HIT_STAGE_1)
        {
            if (FRAM_Write(FRAM_WR_calCacheAddr(addr, gr->newest_item, offset), data, length, &FRAM_WR_CB))
            {
                FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_HIT_STAGE_2;
            }
            else
            {
                return -1;
            }
        }
    case FRAM_WR_WRITE_HIT_STAGE_2:
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_HIT_STAGE_2)
        {
            if (FRAM_WR_done == 0)
            {
                return -1;
            }
            gr->dirty_bits |= (1 << gr->newest_item);
            gr->ref_bits |= (1 << gr->newest_item);
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_STAGE_0;
            return 1;
        }
    case FRAM_WR_WRITE_DESTAGE_STAGE_1:
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_DESTAGE_STAGE_1)
        {
            if (FRAM_Read(FRAM_WR_calCacheAddr(addr, gr->newest_item, 0), FRAM_WR_out_buffer, FRAM_WR_CACHE_BL_SIZE, &FRAM_WR_CB))
            {
                FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_DESTAGE_STAGE_2;
            }
            else
            {
                return -1;
            }
        }
    case FRAM_WR_WRITE_DESTAGE_STAGE_2:
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_DESTAGE_STAGE_2)
        {
            if (FRAM_WR_done == 0)
            {
                return -1;
            }
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_DESTAGE_STAGE_3;
        }
    case FRAM_WR_WRITE_DESTAGE_STAGE_3:
        if (FRAM_WR_WRITE_current_stage == FRAM_WR_WRITE_DESTAGE_STAGE_3)
        {
            switch (DMA_MMC_WriteSector(gr->item[gr->newest_item].sector_addr, FRAM_WR_out_buffer))
            {
            case 0:
                FRAM_WR_READ_current_stage = FRAM_WR_READ_STAGE_0;
                return 0;
            case -1:
                return -1;
            }
            gr->dirty_bits &= ~(1 << gr->newest_item);
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_MISS_STAGE_1;
        }
    case FRAM_WR_WRITE_MISS_STAGE_1:
        switch (DMA_MMC_ReadSector(addr, FRAM_WR_in_buffer, 0))
        {
        case 0:
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_STAGE_0;
            return 0;
        case -1:
            return -1;
        }
        //gr->ref_bits |= (1 << gr->newest_item);
        memcpy(FRAM_WR_in_buffer, data, length);
        FRAM_WR_done = 0;
        FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_MISS_STAGE_2;
    case FRAM_WR_WRITE_MISS_STAGE_2:
        if (FRAM_Write(FRAM_WR_calCacheAddr(addr, gr->newest_item, 0), FRAM_WR_in_buffer, FRAM_WR_CACHE_BL_SIZE, &FRAM_WR_CB))
        {
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_MISS_STAGE_3;
        }
        else
        {
            return -1;
        }
    case FRAM_WR_WRITE_MISS_STAGE_3:
        if (FRAM_WR_done)
        {
            gr->dirty_bits &= ~(1 << gr->newest_item);
            gr->ref_bits |= (1 << gr->newest_item);
            FRAM_WR_WRITE_current_stage = FRAM_WR_WRITE_HIT_STAGE_1;
        }
        return -1;
    }
    return 0;
}

int FRAM_WR_load_control_block(void)
{
    FRAM_WR_done = 0;
	return FRAM_Read(FRAM_WR_CONTROL_BL_STR,
					 (unsigned char*)(&FRAM_WR_data_hashtable),
			  		 sizeof(FRAM_WR_data_hashtable),
			  		 &FRAM_WR_CB);
}

#pragma inline
unsigned char FRAM_WR_get_CONTROL_BL_GR_offset(unsigned char gr_num)
{
    unsigned long res;
    unsigned char i;
    
    res = 0;
    for (i = 0; i < gr_num; ++i)
    {
        res += sizeof(FRAM_WR_control_bl_assoc_t);
    }
    return res;
}

inline unsigned char FRAM_WR_get_CONTROL_BL_ITEM_offset(unsigned char gr_num, unsigned char item_num)
{
    unsigned long res;
    unsigned char i;
    
    res = 0;
    for (i = 0; i < item_num; ++i)
    {
        res += sizeof(FRAM_WR_control_bl_item_t);
    }
    res += FRAM_WR_get_CONTROL_BL_GR_offset(gr_num) + 5;
    return res;
}

int FRAM_WR_write_ref_bits(unsigned char gr_num)
{
    unsigned long temp;
    
    temp = FRAM_WR_get_CONTROL_BL_GR_offset(gr_num);
    // use the synchronous write for short use
    MEM_WriteByte(FRAM_WR_CONTROL_BL_STR + temp,
                  (unsigned char*)(&FRAM_WR_data_hashtable.gr[gr_num].ref_bits),
                  2);
    return 1;
}

int FRAM_WR_write_dirty_bits(unsigned char gr_num)
{
    unsigned long temp;
    
    temp = FRAM_WR_get_CONTROL_BL_GR_offset(gr_num);
    // use the synchronous write for short use
    MEM_WriteByte(FRAM_WR_CONTROL_BL_STR + temp + 2,
                  (unsigned char*)(&FRAM_WR_data_hashtable.gr[gr_num].dirty_bits),
                  2);
    return 1;
}

int FRAM_WR_write_newest_item(unsigned char gr_num)
{
    unsigned long temp;
    
    temp = FRAM_WR_get_CONTROL_BL_GR_offset(gr_num);
    // use the synchronous write for short use
    MEM_WriteByte(FRAM_WR_CONTROL_BL_STR + temp + 4,
                  (unsigned char*)(&FRAM_WR_data_hashtable.gr[gr_num].newest_item),
                  1);
    return 1;
}

int FRAM_WR_write_sector_addr(unsigned char gr_num, unsigned char item_num)
{
    unsigned long temp_item;
    
    temp_item = FRAM_WR_get_CONTROL_BL_ITEM_offset(gr_num, item_num);
    MEM_WriteByte(FRAM_WR_CONTROL_BL_STR + temp_item + 1,
                  (unsigned char*)(&FRAM_WR_data_hashtable.gr[gr_num].item[item_num].sector_addr),
                  4);
    return 1;
}

int FRAM_WR_write_byte_count(unsigned char gr_num, unsigned char item_num)
{
    unsigned long temp_item;
    
    temp_item = FRAM_WR_get_CONTROL_BL_ITEM_offset(gr_num, item_num);
    MEM_WriteByte(FRAM_WR_CONTROL_BL_STR + temp_item + 5,
                  (unsigned char*)(&FRAM_WR_data_hashtable.gr[gr_num].item[item_num].byte_count),
                  2);
    return 1;
}

int FRAM_WR_write_file_index(unsigned char gr_num, unsigned char item_num)
{
    unsigned long temp_item;
    
    temp_item = FRAM_WR_get_CONTROL_BL_ITEM_offset(gr_num, item_num);
    MEM_WriteByte(FRAM_WR_CONTROL_BL_STR + temp_item,
                  (unsigned char*)(&FRAM_WR_data_hashtable.gr[gr_num].item[item_num].file_index),
                  4);
    return 1;
}

void FRAM_WR_init(void)
{
    FRAM_Initialize(1);
	//probe_high();
	FRAM_WR_load_control_block();
	while (FRAM_WR_done == 0);	//wait for dma's job finished
	//probe_low();
	//FRAM_WR_tid = registerTask(&FRAM_WR_proc, 0, 0, 0xFF);
}
