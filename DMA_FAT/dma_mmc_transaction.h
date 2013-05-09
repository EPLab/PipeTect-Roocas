#ifndef __DMA_MMC_TRANSACTION_H__
#define __DMA_MMC_TRANSACTION_H__

#ifdef __DMA_MMC_ENABLED__

typedef enum{
    DMA_MMC_COMM_READ_SECTOR = 0,
    DMA_MMC_COMM_WRITE_SECTOR = 8,
#ifndef __DMA_MMC_ESSENTIAL_ONLY
	DMA_MMC_COMM_READ_PARTIAL_SECTOR = 1,
    DMA_MMC_COMM_READ_MULTI_PARTIAL_SECTOR = 2,
    DMA_MMC_COMM_WRITE_PARTIAL_SECTOR = 9,
    DMA_MMC_COMM_WRITE_MULTI_PARTIAL_SECTOR = 10,
#endif
} DMA_MMC_COMM_t;

typedef struct DMA_MMC_cBlock
{
    unsigned long tID;
    DMA_MMC_COMM_t tType;
    unsigned long firstSector;
    unsigned long secondSector;
    unsigned short offset;
    unsigned short length;
    unsigned char* dataPT;
    void (*callback)(void);
} DMA_MMC_cBlock_t;

#define DMCB_BLOCK_NUM 2

extern void DMA_MMC_Transaction_Init(void);
extern unsigned int DMA_MMC_RequestTransaction(DMA_MMC_COMM_t transactionType,
                                               unsigned long sector1,
                                               unsigned long sector2,
                                               unsigned short offset,
                                               unsigned short length,
                                               unsigned char* data);
extern void DMA_MMC_Transaction_CB(unsigned char result);
extern unsigned char DMA_MMC_Transaction_Status(void);
extern DMA_MMC_cBlock_t* DMA_MMC_get_Current_Transaction(void);

#endif 	//__DMA_MMC_ENABLED__
#endif
