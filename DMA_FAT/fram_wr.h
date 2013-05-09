#ifndef __FRAM_WR_H
#define __FRAM_WR_H

#define FRAM_WR_ASSOCIATIVITY_BIT_NUM 2
#define FRAM_WR_ASSOCIATIVITY (1 << FRAM_WR_ASSOCIATIVITY_BIT_NUM)

#define FRAM_WR_CACHE_BL_MAX 256
#define FRAM_WR_CACHE_BL_BITS 9
#define FRAM_WR_CACHE_BL_SIZE (1 << FRAM_WR_CACHE_BL_BITS)
#define FRAM_WR_CACHE_GR_NUM (FRAM_WR_CACHE_BL_MAX / FRAM_WR_ASSOCIATIVITY)
#define FRAM_WR_META_BL_MAX 15
#define FRAM_WR_DATA_BL_STR 0L
#define FRAM_WR_META_BL_STR 131072L
#define FRAM_WR_CONTROL_BL_STR 253952L //8KB for control block
#define FRAM_WR_DATA_MOD_FACTOR 16

#define FRAM_WR_BUF_TX_THRESHOLD FRAM_WR_DATA_SECTOR_SIZE
#define FRAM_WR_BUF_SIZE FRAM_WR_DATA_SECTOR_SIZE * 2

/////////////////////////////////////////////////////////////////////
// The hash table should be maintained in the FRAM
// Ths hash table structure supposed have the following elements
// The hash table has 16 blocks of 16 sector for each
// 
// 1. key value pair: key (0 ~ 15), the actual block number
//		current number of bytes written
// 2. block hold? or sector hold?
// 		= if there is any block (group of sectors) handling
//		- at first go with block method first (dirty bit should be used per block or we can use byte counters as dirty bits
// 3. 4bytes (Block address) + 2bytes (Byte Counter) + 2bytes (reserved) 
// 4. Need a buffer
// 		- size - I don't want to transfer to empty the buffer until it gets enough amount filled in.
//		- need a threshold value - later we can change the value for evaluation
// 5. Rather than using the hash, it looks better to do the sequential search for
//	  to manage LRU
//
// 04272011 - change plan
// Metadata 
// unit change: from block to sector (8KB -> 512bytes)


typedef struct __FRAM_WR_data_bl{
	unsigned char* data;
	unsigned char offset;	//0 ~ 15
} FRAM_WR_data_bl_t;

typedef struct __FRAM_WR_control_bl_item{
	unsigned long sector_addr;
	//unsigned char sector_count;
	unsigned short byte_count;
    unsigned char file_index;
	unsigned char flag;	//flag[0] -> dirty bit 
} FRAM_WR_control_bl_item_t;

typedef struct __FRAM_WR_control_bl_assoc{
	unsigned short dirty_bits;
    unsigned short ref_bits;
	unsigned char newest_item;
    unsigned char flag;
    FRAM_WR_control_bl_item_t item[FRAM_WR_ASSOCIATIVITY];
} FRAM_WR_control_bl_assoc_t;

typedef struct __FRAM_WR_control_data_control_center{
	FRAM_WR_control_bl_assoc_t gr[FRAM_WR_CACHE_GR_NUM];
	unsigned int checksum;
} FRAM_WR_control_data_control_center_t;

typedef enum{
	FRAM_WR_STAGE_NONE = 0,
	FRAM_WR_STATGE_DIRTY_BLOCK_WB,
	FRAM_WR_STAGE_UPDATE_HASH_INFO,
} FRAM_WR_stage_t;


extern void FRAM_WR_init(void);
//extern int FRAM_WR_add_data(unsigned long bl_addr, unsigned char* data, int data_size);
extern int FRAM_WR_read(unsigned long __addr, int __offset, unsigned char* __data, int __length, unsigned char __backGroundFRAM);
extern int FRAM_WR_write(unsigned long __addr, int __offset, unsigned char* __data, int __length, unsigned char __backGroundFRAM);
//extern int FRAM_WR_remove_block(unsigned long bl_addr, unsigned char offset, unsigned char* data);
extern unsigned char* FRAM_WR_get_block(unsigned long bl_addr, unsigned char offset);

#endif
