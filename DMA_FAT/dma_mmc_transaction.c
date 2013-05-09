#ifdef __DMA_MMC_ENABLED__

#include "dma_mmc_transaction.h"
#include "dma_mmc.h"

DMA_MMC_cBlock_t DMCB_pool[DMCB_BLOCK_NUM];
unsigned short DMCB_num_used;
unsigned short DMCB_num_in;
unsigned short DMCB_num_out;
DMA_MMC_cBlock_t* DMCB_current_transaction;
unsigned long DMCB_tID_count = 0;
volatile unsigned char DMA_MMC_Overall_Processing = 0;


static unsigned int DMA_MMC_Transaction_Select(DMA_MMC_cBlock_t* tr);


void DMA_MMC_Transaction_Init(void)
{
    // init DMCB array
    DMCB_num_used = 0;
    DMCB_num_in = 0;
    DMCB_num_out = 0;
}

//add a request to handle a MMC transaction
unsigned int DMA_MMC_RequestTransaction(DMA_MMC_COMM_t transactionType,
                                        unsigned long sector1,
                                        unsigned long sector2,
                                        unsigned short offset,
                                        unsigned short length,
                                        unsigned char* data)
{
    DMA_MMC_cBlock_t* tempBlock;
    
    if (DMCB_num_used < DMCB_BLOCK_NUM)
    {
        tempBlock = &DMCB_pool[DMCB_num_in];
        //fill up a cBlock;
        tempBlock->tID = DMCB_tID_count++;
        tempBlock->tType = transactionType;
        tempBlock->firstSector = sector1;
        tempBlock->secondSector = sector2;
        tempBlock->offset = offset;
        tempBlock->length = length;
        tempBlock->dataPT = data;
        
        DMCB_num_in = (DMCB_num_in + 1) % DMCB_BLOCK_NUM;
        ++DMCB_num_used;
        
        if (DMA_MMC_Overall_Processing == 0)
        {
            DMA_MMC_Overall_Processing = 1;
            DMCB_current_transaction = &DMCB_pool[DMCB_num_out];
            DMA_MMC_Transaction_Select(DMCB_current_transaction);
        }
        return 1; 
    }
    return 0;
}

//process the queued transactions
void DMA_MMC_Transaction_CB(unsigned char result)
{
    if (DMA_MMC_Overall_Processing == 1)
    {       
        if (result)
        {
            if (DMCB_current_transaction->callback)
            {
                DMCB_current_transaction->callback();
            }
            DMCB_num_out = (DMCB_num_out + 1) % DMCB_BLOCK_NUM;
            --DMCB_num_used;
            if (DMCB_num_used > 0)
            {
                DMCB_current_transaction = &DMCB_pool[DMCB_num_out];  
            }
            else
            {
                DMA_MMC_Overall_Processing = 0;
                DMCB_current_transaction = 0;
                return;
            }
        }
        DMA_MMC_Transaction_Select(DMCB_current_transaction);
    }
}

unsigned int DMA_MMC_Transaction_Select(DMA_MMC_cBlock_t* tr)
{
    switch (tr->tType)
    {
    case DMA_MMC_COMM_READ_SECTOR:
        return DMA_MMC_ReadSector(tr->firstSector, tr->dataPT, 0);
    case DMA_MMC_COMM_WRITE_SECTOR:
        return DMA_MMC_WriteSector(tr->firstSector, tr->dataPT);
#ifndef __DMA_MMC_ESSENTIAL_ONLY__
	case DMA_MMC_COMM_READ_PARTIAL_SECTOR:
        return DMA_MMC_ReadPartialSector(tr->firstSector, tr->offset, tr->length, tr->dataPT); 
    case DMA_MMC_COMM_READ_MULTI_PARTIAL_SECTOR:
        return DMA_MMC_ReadPartialMultiSector(tr->firstSector,
                                              tr->secondSector,
                                              tr->offset,
                                              tr->length,
                                              tr->dataPT);
    case DMA_MMC_COMM_WRITE_PARTIAL_SECTOR:
        return DMA_MMC_WritePartialSector(tr->firstSector, tr->offset, tr->length, tr->dataPT);
    case DMA_MMC_COMM_WRITE_MULTI_PARTIAL_SECTOR:
        return DMA_MMC_WritePartialMultiSector(tr->firstSector,
                                               tr->secondSector,
                                               tr->offset,
                                               tr->length,
                                               tr->dataPT);
#endif
    }
    return 0;
}

unsigned char DMA_MMC_Transaction_Status(void)
{
    return DMA_MMC_Overall_Processing;
}

DMA_MMC_cBlock_t* DMA_MMC_get_Current_Transaction(void)
{
    return DMCB_current_transaction;
}
#endif
