#ifndef __DMA_MMC_H
#define __DMA_MMC_H

/* Memory card sector size */
#define SECTOR_SIZE 512
#define MAX_NUM_PARTITIONS 4

#define R1_ILL_COMMAND 2

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
#define CMD55 0x77
// CMD43 ... CMD57 are Reserved
#define CMD58 0x7A	// reads the OCR register
#define CMD59 0x7B	// turns CRC off
// CMD60 ... CMD63 are not used in SPI mode


// SPI Response Flags
#define IN_IDLE_STATE	   	(unsigned char)(0x01)
#define ERASE_RESET	   		(unsigned char)(0x02)
#define ILLEGAL_COMMAND	   	(unsigned char)(0x04)
#define COM_CRC_ERROR	   	(unsigned char)(0x08)
#define ERASE_ERROR	   		(unsigned char)(0x10)
#define ADRESS_ERROR	   	(unsigned char)(0x20)
#define PARAMETER_ERROR	   	(unsigned char)(0x40)


#define CARD_RESPONSE_MAX_COUNT 0x7FFF
#define DMA_MMC_MAX_WRITESTATE_COUNT 0xFFFF

//#include "fat32_pbr.h"
#include "dma_mmc_transaction.h"

typedef enum{
    UNSUPPORTED_CARD = 0,
    SD,
    SDHC,
    SDXC
} DMA_MMC_CardType;

typedef struct MMC_CardSpecInfo{
    unsigned char   resesrved_1:6,
                    CSD_STRUCTURE:2;
    unsigned char   TAAC;
    unsigned char   NSAC;
    unsigned char   TRAN_SPEED;
    unsigned short  READ_BL_LEN:4,
                    CCC:12;
    unsigned long   C_SIZE:22,
                    reserved_2:6,
                    DSR_IMP:1,
                    READ_BLK_MISALIGN:1,
                    WRITE_BLK_MISALIGN:1,
                    READ_BL_PARTIAL:1;
    unsigned short  WP_GRP_SIZE:7,
                    SECT_SIZE:7,
                    ERASE_BLK_EN:1,
                    reserved_3:1;
    unsigned short  reserved_4:5,
                    WRITE_BL_PARTIAL:1,
                    WRITE_BL_LEN:4,
                    R2W_FACTOR:3,
                    reserved_5:2,
                    WP_GRP_ENABLE:1;
    unsigned char   reserved_6:2,
                    FILE_FORMAT:2,
                    TMP_WRITE_PROTECT:1,
                    PERM_WRITE_PROTECT:1,
                    COPY:1,
                    FILE_FORMAT_GRP:1;
    unsigned char   ALWAYS_1:1,
                    CRC:7;
} MMC_CardSpecInfo_t;

typedef struct MMC_MasterBootRecord{
    unsigned char current_state_of_partition; //(00:inactive, 80:active)
    unsigned char B_Head;
    unsigned short B_Sector;
    unsigned char PartitionType;
    unsigned char E_Head;
    unsigned short E_Sector;
    unsigned long offset;
    unsigned long numSectorsInPartition;
} MMC_MasterBootRecord_t;


extern unsigned char MMC_CardResponse(unsigned char expRes);
//extern unsigned char DMA_MMC_init(void);
extern int DMA_MMC_IsIdle(void);
extern DMA_MMC_CardType MMC_MemCardType(void);
extern int MMC_MemCardInit(void);
extern unsigned char MMC_SetBLockLength(void);
extern MMC_CardSpecInfo_t* MMC_GetCardSpecInfo(void);
extern unsigned int MMC_GetMasterBootRecord(void);
extern MMC_MasterBootRecord_t* MMC_GetPartitionInfo(void);
extern unsigned long MMC_GetPartitionOffset(void);
extern void MMC_EnableCS(void);
extern void MMC_DisableCS(void);
extern void MMC_SendCommand(unsigned char command,
                            unsigned char argA,
                            unsigned char argB,
                            unsigned char argC,
                            unsigned char argD,
                            unsigned char CRC);
extern unsigned int MMC_ReadSector(unsigned long sector, unsigned char *buf);
extern unsigned int MMC_WriteSector(unsigned long sector, unsigned char* buf);

extern int DMA_MMC_ReadSector(unsigned long sector, unsigned char* buf, unsigned char override);
extern unsigned int DMA_MMC_ReadPartialSector(unsigned long sector,
                                              unsigned int byteOffset,
                                              unsigned int bytesToRead,
                                              unsigned char* buf);
extern unsigned int DMA_MMC_ReadPartialMultiSector(unsigned long sector1,
                                                   unsigned long sector2,
                                                   unsigned int byteOffset,
                                                   unsigned int bytesToRead,
                                                   unsigned char* buf);
extern int DMA_MMC_WriteSector(unsigned long sector, unsigned char* buf);
extern unsigned int DMA_MMC_WritePartialSector(unsigned long sector,
                                               unsigned int byteOffset,
                                               unsigned int bytesToWrite,
                                               unsigned char* buf);
extern unsigned int DMA_MMC_WritePartialMultiSector(unsigned long sector1,
                                                    unsigned long sector2,
                                                    unsigned int byteOffset,
                                                    unsigned int bytesToWrite,
                                                    unsigned char *buf);
extern unsigned int DMA_MMC_RequestTransaction(DMA_MMC_COMM_t transactionType,
                                               unsigned long sector1,
                                               unsigned long sector2,
                                               unsigned short offset,
                                               unsigned short length,
                                               unsigned char* data);
extern unsigned char DMA_MMC_Transaction_Status(void);

#endif
