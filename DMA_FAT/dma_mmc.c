#include "dma_spi.h"
#include "dma_mmc.h"
#include "dma_mmc_transaction.h"
#include "fat32_pbr.h"
#include "fat32_directory_entry.h"
#include "fat32_fsinfo.h"
#include "define.h"

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////
#define DWORD           unsigned long
#define BYTE            unsigned char
#define WORD            unsigned short
#define BOOL            unsigned char
#define TRUE            1
#define FALSE           0

#include "msp430x54x.h"

//#define MMC_DUMMY_DATA  0xFF

#define DMA_MMC_CardRes_Count_MAX 255

#define MMC_STATE_end   0xFF

#ifdef __MMC_INFO_GEN_ENABLED__
	#define MAX_FAT_ENTRIES_CACHE 128
	#define MAX_DIR_ENTIRIES_CACHE 16
#endif

////////////////////////////////////////////////////////////////////////////////
// Local variables
////////////////////////////////////////////////////////////////////////////////

#ifdef __DMA_MMC_ENABLED__
typedef enum{
    READ_SECTOR_ERROR = 0,
    READ_PARTIAL_SECTOR_ERROR = 1,
    READ_PARTIAL_MULTI_SECTOR_ERROR = 2,
    WRITE_SECTOR_ERROR = 8,
    WRITE_PARTIAL_SECTOR_ERROR = 9,
    WRITE_PARTIAL_MULTI_SECTOR_ERROR = 10,
} DMA_MMC_error_t;

//states
unsigned char main_current_state;
unsigned char read_current_state;
unsigned char write_current_state;

unsigned char DMA_MMC_WRITE_buffer[SECTOR_SIZE];
unsigned char* DMA_MMC_temp_WRITE_buffer;

unsigned char DMA_MMC_READ_buffer[SECTOR_SIZE];
unsigned char* DMA_MMC_ReadBufPtr;

volatile unsigned long DMA_MMC_sector1;
volatile unsigned long DMA_MMC_sector2;
volatile unsigned int DMA_MMC_byteOffset;
unsigned int DMA_MMC_bytesToWrite;
volatile unsigned int DMA_MMC_bytesToRead;
//volatile unsigned char* DMA_MMC_buf;

volatile unsigned char DMA_MMC_CardRes_State = 0;
volatile int DMA_MMC_CardRes_Count = 0x0100;
//volatile unsigned short DMA_MMC_count;

unsigned char DMA_MMC_WriteState = 0;
unsigned int DMA_MMC_WriteStateCount = 0;
unsigned char DMA_MMC_ReadResult;

volatile unsigned int DMA_MMC_WriteNumBytes;
unsigned int DMA_MMC_ReadNumBytes;

volatile unsigned char DMA_MMC_Write_Processing = 0;
volatile unsigned char DMA_MMC_Read_Processing = 0;
volatile unsigned char DMA_MMC_forceWrite = 0;
volatile unsigned char DMA_MMC_appendMode = 0;

DMA_MMC_CardType mSDType;

void (*DMA_MMC_current_procedure)(void) = 0;
void (*DMA_MMC_WriteAfterReadProc)(void) = 0;
void (*DMA_MMC_CallbackAfterWriteCheck)(void) = 0;

#ifdef __MMC_INFO_GEN_ENABLED__
static unsigned long MMC_sectorZero;
static FAT32_PBR_t mFAT32PBRInfo;
static unsigned long mFATEntryCache[MAX_FAT_ENTRIES_CACHE];
static FAT32_DirectoryEntry_t mDirEntryCache[MAX_DIR_ENTIRIES_CACHE];
static FAT32_DirectoryEntry_t mDirEntryCache2[MAX_DIR_ENTIRIES_CACHE];
static FAT32_FSInfo_t mFSInfo;
static unsigned long mStartSectorNumOfDataSection;
#endif

#endif

MMC_CardSpecInfo_t mCSD;
MMC_MasterBootRecord_t mPartitionInfo[MAX_NUM_PARTITIONS];
unsigned long numMaxSectors;
//extern unsigned long sectorZero;



////////////////////////////////////////////////////////////////////////////////
// Local protorypes
////////////////////////////////////////////////////////////////////////////////
#ifdef __DMA_MMC_ENABLED__
#ifdef __DMA_MMC_ESSENTIAL_ONLY__
//static unsigned char DMA_MMC_CardResponse(unsigned char expRes, void (*cb_fp)(void));
static void DMA_MMC_error(DMA_MMC_error_t rw);
//static void MMC_GetPartitionInfo_PROC(MMC_MasterBootRecord_t* mMBR);
static void DMA_MMC_ReadSector_proc(void);
//static void DMA_MMC_CheckWriteState_proc(void);
static void DMA_MMC_WriteSector_proc(void);
#else	//__DMA_MMC_ESSENTIAL_ONLY__
static void DMA_MMC_ReadPartialSector_proc(void);
static void DMA_MMC_ReadPartialMultiSector_proc(void);
static void DMA_MMC_WritePartialSector_proc(void);
static void DMA_MMC_WritePartialMultiSector_proc(void);
#endif 	//__DMA_MMC_ESSENTIAL_ONLY__
#endif	//__DMA_MMC_ENABLED__

#ifdef __MMC_INFO_GEN_ENABLED__
static unsigned int FAT32_ReadPartitionBootRecord(FAT32_PBR_t* aPBR);
static void MMC_GetPartitionInfo_PROC(MMC_MasterBootRecord_t* mMBR);
static unsigned int FAT32_ReadFSInfo(unsigned long addrFSInfoSector, FAT32_FSInfo_t* fsInfo);
static unsigned int FAT32_ReadDirEntries(unsigned long AddrDirEntryCluster, FAT32_DirectoryEntry_t* pDirEntry);

static inline unsigned char FAT32_GetUChar(unsigned char* buf, unsigned int* count)
{
    return buf[(*count)++];
}

static inline unsigned short FAT32_GetUShort(unsigned char* buf, unsigned int* count)
{
    unsigned short temp;
    
    temp = buf[(*count)++];
    temp |= ((unsigned short) buf[(*count)++]) << 8;
    return temp;
}

static inline unsigned long FAT32_GetULong(unsigned char* buf, unsigned int* count)
{
    unsigned long temp;
    
    temp = buf[(*count)++];
    temp |= ((unsigned long) buf[(*count)++]) << 8;
    temp |= ((unsigned long) buf[(*count)++]) << 16;
    temp |= ((unsigned long) buf[(*count)++]) << 24;
    return temp;
}

static inline void FAT32_GetArray(unsigned char* arr, unsigned char* buf, unsigned int length, unsigned int* count)
{
    unsigned int i;
    
    for (i = 0; i < length; ++i)
    {
        arr[i] = buf[i + *count];
    }
    *count += length;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Function Definitions
////////////////////////////////////////////////////////////////////////////////

void MMC_EnableCS(void)
{
    MMC_CS_PxOUT &= ~MMC_CS_BIT;    
}

void MMC_DisableCS(void)
{
    MMC_CS_PxOUT |= MMC_CS_BIT;
}

void MMC_SendCommand(unsigned char command,
                     unsigned char argA,
                     unsigned char argB,
                     unsigned char argC,
                     unsigned char argD,
                     unsigned char CRC)
{
    N_DMA_WriteByteSPI(command);
    N_DMA_WriteByteSPI(argA);
    N_DMA_WriteByteSPI(argB);
    N_DMA_WriteByteSPI(argC);
    N_DMA_WriteByteSPI(argD);
    N_DMA_WriteByteSPI(CRC);
}                       

/**
 *	waits for the memory card to response
 *
 *	@param expRes		expected response
 *
 *	@return TRUE		card responded within time limit
 *	@return FALSE		card did not response
 **/
BOOL MMC_CardResponse(unsigned char expRes)
{
	unsigned char actRes;
	unsigned int count = 2000;
	actRes = 0xFF;
	while ((actRes != expRes) && (count > 0))
	{
    	    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            N_DMA_ReadByteSPI(&actRes);		
	    --count;
	}
	if (count == 0)
	{
            return FALSE; 
        }
	return TRUE;
}

BOOL MMC_CardResponseRaw(unsigned char* res)
{
    unsigned int count = 492;
    *res = 0xFF;
    while ((*res & 0x80) && (count > 0))
    {
        N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
        N_DMA_ReadByteSPI(res);		
	--count;
    }
    if (count == 0)
    {
        return FALSE;
    }
    return TRUE;
}

/*
BOOL DMA_MMC_start(unsigned char* data, unsigned short length, DMA_MMC_COMM_t dmc)
{
    memcpy(data, DMA_MMC_buffer, length);
}

void DMA_MMC_proc(void)
{
    switch (state)
    {
	case DMA_MMC_COMPLETE:
	    break;
	case DMA_MMC_
    }
}        
*/


int MMC_MemCardInit(void)
{
	MMC_CardSpecInfo_t* csd_pt;
#ifdef __MMC_INFO_GEN_ENABLED__
    MMC_MasterBootRecord_t* mParInfo;
    unsigned long i;
    unsigned long j;
#endif
	
    //FAT_LibInit();
    switch (MMC_MemCardType())
    {
    case UNSUPPORTED_CARD:
        return 4;
    case SD:
        if (MMC_SetBLockLength())
        {
#ifdef __MMC_INFO_GEN_ENABLED__
            MMC_sectorZero = MMC_GetPartitionOffset();
#endif //__MMC_INFO_GEN_ENABLED__
        }
        else
        {
            return 3;
        }
        break;
    case SDHC:
        //MMC_GetMasterBootRecord();
        csd_pt = MMC_GetCardSpecInfo();
        if (csd_pt == 0)
        {
            return 4;
        }
        if (csd_pt->READ_BL_LEN == 9)
        {
            switch (csd_pt->CSD_STRUCTURE)
            {
            case 0x01:  //SDHC
#ifdef __MMC_INFO_GEN_ENABLED__
                MMC_GetMasterBootRecord();
                mParInfo = MMC_GetPartitionInfo();
                MMC_sectorZero = mParInfo->offset;
                FAT32_ReadPartitionBootRecord(&mFAT32PBRInfo);
                MMC_ReadSector(mFAT32PBRInfo.NumReservedSectors, DMA_MMC_READ_buffer);
                for (i = 0; i < 128; ++i)
                {
                    j = i << 2;
                    mFATEntryCache[i] = DMA_MMC_READ_buffer[j];
                    mFATEntryCache[i] |= ((unsigned long)DMA_MMC_READ_buffer[++j]) << 8;
                    mFATEntryCache[i] |= ((unsigned long)DMA_MMC_READ_buffer[++j]) << 16;
                    mFATEntryCache[i] |= (((unsigned long)DMA_MMC_READ_buffer[++j]) << 24) & 0x0FFFFFFF;
                }
                mStartSectorNumOfDataSection = mFAT32PBRInfo.NumCopiesOfFAT * mFAT32PBRInfo.NumSectorsPerFAT;
                FAT32_ReadDirEntries(mStartSectorNumOfDataSection + mFAT32PBRInfo.NumReservedSectors, mDirEntryCache);
                FAT32_ReadDirEntries(mStartSectorNumOfDataSection + mFAT32PBRInfo.NumReservedSectors + 1, mDirEntryCache2);
                FAT32_ReadFSInfo(mFAT32PBRInfo.SectorNumOfFileSysInfo ,&mFSInfo);
                __no_operation();
#endif
                break;
            case 0x00:  //SD
                break;
            default:
                return 4;
            }
        }
        break;
    default:
        return 4;
    }
    //FAT_IdentifyFileSystem(g_FATBuf);
    //FAT_ReadFileSystemInfo(g_FATBuf);
    return 0;
}

DMA_MMC_CardType MMC_MemCardType(void)
{
    unsigned int count;
    unsigned char temp;
    unsigned char buf[4];
    
    DMA_MMC_Transaction_Init();
    
	MMC_DisableCS();
	
#ifndef DMA_SPI_INITIALIZED
#define DMA_SPI_INITIALIZED
    init_DMA_SPI(MMC_DUMMY_DATA);
#endif
    
    MMC_DisableCS();
    for (count = 0;count < 10; count++)
    {
		N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    }

    MMC_EnableCS();

    for (count = 0; count < 100; ++count)
    {
        MMC_SendCommand(CMD0,0,0,0,0,0x95);	// initializes the MMC
        if (MMC_CardResponse(0x01))
        {
            break;
        }
    }
    if (count == 100)
    {
        MMC_DisableCS();
        mSDType = UNSUPPORTED_CARD;
        return mSDType;
    }
    else
    {				
        N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	/* check for version of SD card specification */
        MMC_SendCommand(CMD8, 0x00, 0x00, 0x01, 0xAA, 0x87);
        MMC_CardResponseRaw(&temp);
        if((temp & (1 << R1_ILL_COMMAND)) == 0)
        {
            for (count = 0; count < 4; ++count)
            {
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
                N_DMA_ReadByteSPI(&buf[count]);
            }
        
            if((buf[2] & 0x01) == 0)
            {
                mSDType = UNSUPPORTED_CARD; /* card operation voltage range doesn't match */
                return mSDType;
            }
            if(buf[3] != 0xaa)
            {
                mSDType = UNSUPPORTED_CARD; /* wrong test pattern */
                return mSDType;
            }
        
            /* card conforms to SD 2 card specification */
            //sd_raw_card_type |= (1 << SD_RAW_SPEC_2);
            
            //N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
//            MMC_SendCommand(CMD58, 0x00, 0x00, 0x00, 0x00, 0x95);
//            MMC_CardResponseRaw(&temp);
//            for (count = 0; count < 4; ++count)
//            {
//                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
//                N_DMA_ReadByteSPI(&buf[count]);
//            }
//            __no_operation();
			// the count is supposed to be way bigger than it was (1024 -> 4096) 
			// usually needs around 3000
            for (count = 4096; count > 0; --count)
            {
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
                MMC_SendCommand(CMD55, 0x00, 0x00, 0x00, 0x00, 0x95);
                MMC_CardResponseRaw(&temp);
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
                MMC_SendCommand(CMD41, 0x40, 0x00, 0x00, 0x00, 0x95);
                MMC_CardResponseRaw(&temp);
                if ((temp & IN_IDLE_STATE) == 0)
                {
                    break;
                }
				N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
				N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
				N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
				N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            }
            if (count == 0)
            {
                MMC_DisableCS();
                mSDType = UNSUPPORTED_CARD;
                return mSDType;
            }
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            MMC_SendCommand(CMD58, 0x00, 0x00, 0x00, 0x00, 0x95);
            MMC_CardResponseRaw(&temp);
            for (count = 0; count < 4; ++count)
            {
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
                N_DMA_ReadByteSPI(&buf[count]);
            }
            if (buf[0] & 0x40)
            {
                //high_cap
                __no_operation();
            }
            MMC_DisableCS();
            
	    	N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            mSDType = SDHC;
            return mSDType;
        }
        else
        {
            count = 0x1FF;
	    	do
	    	{
	        	MMC_SendCommand(CMD1,0,0,0,0,0xFF);	// brings card out of idle state
	        	count--;
	    	} while ((!MMC_CardResponse(0x00)) && (count > 0));

            MMC_DisableCS();
            
            if (count == 0)
            {
                mSDType = UNSUPPORTED_CARD;
                return mSDType;
            }
            
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            mSDType = SD;
                
            return mSDType;
        }
    }
}


/**
 *	Initialize the block length in the memory card to be 512 bytes
 *	
 *	return TRUE			block length sucessfully set
 *	return FALSE		block length not sucessfully set
 **/
unsigned char MMC_SetBLockLength(void)
{
    MMC_EnableCS();

    MMC_SendCommand(CMD16,0,0,2,0,0xFF);

    if (MMC_CardResponse(0x00))
    {
	N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
        MMC_DisableCS();
	return TRUE;
    }
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);

    MMC_DisableCS();

    return FALSE;
}

MMC_CardSpecInfo_t* MMC_GetCardSpecInfo(void)
{
    unsigned int count;
    unsigned char* buf;
    unsigned char temp;
    
    buf = (unsigned char*)(&mCSD);
    
    MMC_EnableCS();
    
    MMC_SendCommand(CMD9, 0, 0, 0, 0, 0xFF);
    
    if (MMC_CardResponse(0xFE))
    {
        for (count = 0; count < 15; ++count)
        {
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            N_DMA_ReadByteSPI(&buf[count]);
        }
        // convert to little endian from big endian
        temp = buf[4];
        buf[4] = buf[5];
        buf[5] = temp;
        temp = buf[6];
        buf[6] = buf[9];
        buf[9] = temp;
        temp = buf[7];
        buf[7] = buf[8];
        buf[8] = temp;
        temp = buf[10];
        buf[10] = buf[11];
        buf[11] = temp;
        temp = buf[12];
        buf[12] = buf[13];
        buf[13] = temp;
    }
    else
    {
        MMC_DisableCS();
        return 0;
    }
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    
    MMC_DisableCS();
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    
    numMaxSectors = 1024 * (mCSD.C_SIZE + 1);
    
    return &mCSD;
}

#ifdef __MMC_INFO_GEN_ENABLED__
unsigned int MMC_GetMasterBootRecord(void)
{
    unsigned int count;
    unsigned char charBuf;
    unsigned char ret = 0;
    
    MMC_EnableCS();
    MMC_SendCommand(CMD17,0,0,0,0,0xFF);
    if (MMC_CardResponse(0x00))
    {
		if (MMC_CardResponse(0xFE))
		{
			for (count = 0; count < 446; ++count)
			{
				N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
				N_DMA_ReadByteSPI(&charBuf);
			}
			for (count = 0; count < 4; ++count)
			{
				MMC_GetPartitionInfo_PROC(&mPartitionInfo[count]);
			}
			N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
			N_DMA_ReadByteSPI(&charBuf);
			N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
			N_DMA_ReadByteSPI(&charBuf);
			N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
			N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
			ret = 1;
		}
    }
    MMC_DisableCS();

    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    //N_DMA_ReadByteSPI(&charBuf);
    return ret;
}

unsigned int FAT32_ReadDirEntries(unsigned long AddrDirEntryCluster, FAT32_DirectoryEntry_t* pDirEntry)
{
    unsigned int ret = 0;
    unsigned int i;
    unsigned int numEntriesRetrieved = 0;
    
    if (MMC_ReadSector(MMC_sectorZero + AddrDirEntryCluster, DMA_MMC_READ_buffer))
    {
        for (i = 0; i < MAX_DIR_ENTIRIES_CACHE; ++i)
        {
            FAT32_GetArray(pDirEntry[i].DirName, DMA_MMC_READ_buffer, 8, &ret);
            FAT32_GetArray(pDirEntry[i].DirExtention, DMA_MMC_READ_buffer, 3, &ret);
            pDirEntry[i].Attributes = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].Reserved = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].CreateTime10ms = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].CreateTime = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].CreateDate = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].LastAccessDate = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].FirstClusterNum = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].FirstClusterNum <<= 16;
            pDirEntry[i].WriteTime = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].WriteDate = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].FirstClusterNum |= FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);
            pDirEntry[i].FileSize = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);
            if (pDirEntry[i].DirName[0] != 0)
            {
                ++numEntriesRetrieved;
            }
        }
    }
    return numEntriesRetrieved;
}

unsigned int FAT32_ReadFSInfo(unsigned long addrFSInfoSector, FAT32_FSInfo_t* fsInfo)
{
    unsigned int ret = 0;
    
    MMC_ReadSector(MMC_sectorZero + 0, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 0, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 1, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 2, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 3, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 4, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 5, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 6, DMA_MMC_READ_buffer);
    MMC_ReadSector(MMC_sectorZero + addrFSInfoSector + 7, DMA_MMC_READ_buffer);
    if (MMC_ReadSector(MMC_sectorZero + addrFSInfoSector, DMA_MMC_READ_buffer))
    {
        FAT32_GetArray(fsInfo->FirstSignature, DMA_MMC_READ_buffer, 4, &ret);
        ret = 0x01E4;
        FAT32_GetArray(fsInfo->SigFSInfoSector, DMA_MMC_READ_buffer, 4, &ret);
        fsInfo->NumFreeClusters = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);
        fsInfo->RecentlyAccessedClusterNum = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);
        FAT32_GetArray(fsInfo->Reserved, DMA_MMC_READ_buffer, 12, &ret);
        FAT32_GetArray(fsInfo->ExecutableCode, DMA_MMC_READ_buffer, 2, &ret);
        FAT32_GetArray(fsInfo->BootRecordSignature, DMA_MMC_READ_buffer, 2, &ret);
    }
    return ret;
}

unsigned int FAT32_ReadPartitionBootRecord(FAT32_PBR_t* aPBR)
{  
    unsigned int ret = 0;
    
    if (MMC_ReadSector(MMC_sectorZero + 0L, DMA_MMC_READ_buffer))
    {
        FAT32_GetArray(aPBR->JumpCode, DMA_MMC_READ_buffer, 3, &ret);                    //a 0
        FAT32_GetArray(aPBR->OEM_Name, DMA_MMC_READ_buffer, 8, &ret);                    //a 1
        aPBR->BytesPerSector = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);             //s 2
        aPBR->SectorsPerCluster = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);           //c 3
        aPBR->NumReservedSectors = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);            //s 4
        aPBR->NumCopiesOfFAT = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);              //c 5
        aPBR->MaxRootDirectoryEntries = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);    //s 6
        aPBR->NumSectorsInSmallPartition = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret); //s 7
        aPBR->MediaDescriptor = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);             //c 8
        aPBR->SectorsPerOldFAT = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);           //s 9
        aPBR->SectorsPerTrack = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);            //s 10
        aPBR->NumHeads = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);                   //s 11
        aPBR->NumHiddenSectorsInPartition = FAT32_GetULong(DMA_MMC_READ_buffer, &ret); //l 12
        aPBR->NumSectorsInPartition = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);       //l 13
        aPBR->NumSectorsPerFAT = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);         //l 14
        aPBR->Flags = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);                      //s 15
        aPBR->Version = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);                    //s 16
        aPBR->ClusterNumOfStartRootDir = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);    //l 17
        aPBR->SectorNumOfFileSysInfo = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);     //s 18
        aPBR->SectorNumOfBackupBootSector = FAT32_GetUShort(DMA_MMC_READ_buffer, &ret);//s 19
        FAT32_GetArray(aPBR->reserved, DMA_MMC_READ_buffer, 12, &ret);                   //a 20
        aPBR->LogicalDriveNumOfPartition = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);  //c 21
        aPBR->Unused = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);                      //c 22
        aPBR->ExtendedSignature = FAT32_GetUChar(DMA_MMC_READ_buffer, &ret);           //c 23
        aPBR->SerialNumOfPartition = FAT32_GetULong(DMA_MMC_READ_buffer, &ret);        //l 24
        FAT32_GetArray(aPBR->VolumeNameOfPartition, DMA_MMC_READ_buffer, 11, &ret);      //a 25
        FAT32_GetArray(aPBR->FATName, DMA_MMC_READ_buffer, 8, &ret);                     //a 26
        FAT32_GetArray(aPBR->ExecutableCode, DMA_MMC_READ_buffer, 420, &ret);            //a 27
        FAT32_GetArray(aPBR->BootRecordSignature, DMA_MMC_READ_buffer, 2, &ret);         //a 28
    }    
    return ret;
}

void MMC_GetPartitionInfo_PROC(MMC_MasterBootRecord_t* mMBR)
{
    unsigned char charBuf;
    unsigned char temp[4];
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&mMBR->current_state_of_partition);
            
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&mMBR->B_Head);
          
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&charBuf);
    mMBR->B_Sector = (unsigned short)charBuf;
    mMBR->B_Sector <<= 8;
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&charBuf);
    mMBR->B_Sector |= (unsigned short)charBuf;
            
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&mMBR->PartitionType);
           
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&mMBR->E_Head);
           
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&charBuf);
    mMBR->E_Sector = (unsigned short)charBuf;
    mMBR->E_Sector <<= 8;
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&charBuf);
    mMBR->E_Sector |= (unsigned short)charBuf;
            
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[0]);
    mMBR->offset = temp[0];
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[1]);
    mMBR->offset |= ((unsigned long)temp[1]) << 8;
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[2]);
    mMBR->offset |= ((unsigned long)temp[2]) << 16;
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[3]);
    mMBR->offset |= ((unsigned long)temp[3]) << 24;
          
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[0]);
    mMBR->numSectorsInPartition = temp[0];
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[1]);
    mMBR->numSectorsInPartition |= ((unsigned long)temp[1]) << 8;
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[2]);
    mMBR->numSectorsInPartition |= ((unsigned long)temp[2]) << 16;
    
    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&temp[3]);
    mMBR->numSectorsInPartition |= ((unsigned long)temp[3]) << 24;
}

MMC_MasterBootRecord_t* MMC_GetPartitionInfo(void)
{
    return mPartitionInfo;
}
#endif		//__MMC_INFO_GEN_ENABLED__


/**
 *	locates the offset of the partition table from the absolute zero sector
 *
 *	return ...			the offset of the partition table
 **/
unsigned long MMC_GetPartitionOffset(void)
{
    unsigned int count = 0;
    unsigned long offset = 0;
    unsigned char charBuf;

    MMC_EnableCS();

    MMC_SendCommand(CMD17,0,0,0,0,0xFF);

    if (MMC_CardResponse(0x00))
    {
	if (MMC_CardResponse(0xFE))
	{
            for (count = 0; count < 454; count++) 
            {
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
		N_DMA_ReadByteSPI(&charBuf);
            }
            
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&charBuf);
	    offset = charBuf;

            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&charBuf);
	    offset |= ((unsigned long int)charBuf) << 8;

            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&charBuf);
	    offset |= ((unsigned long int)charBuf) << 16;

            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&charBuf);
	    offset += ((unsigned long int)charBuf) << 24;
	    for (count = 458; count < SECTOR_SIZE; count++)
            {  
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
		N_DMA_ReadByteSPI(&charBuf);
            }
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&charBuf);

            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&charBuf);
	}
    }
    MMC_DisableCS();

    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&charBuf);

    return offset;
}

/**
 *	read content of the sector indicated by the input and place it in the buffer
 *
 *	@param	sector		sector to be read
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes read
 **/
unsigned int MMC_ReadSector(unsigned long sector, unsigned char *buf)
{
    unsigned int count = 0;
    unsigned char tmp;
    
    //sector += sectorZero;
    //sector *= 2;	

    MMC_EnableCS();
    MMC_SendCommand(CMD17,(sector>>24)&0xFF,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0xFF);

    if (MMC_CardResponse(0x00))
    {
        if (MMC_CardResponse(0xFE))
	{
	    for (count = 0; count < SECTOR_SIZE; count++)
	    {
                N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
		N_DMA_ReadByteSPI(&buf[count]);
	    }
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
	    N_DMA_ReadByteSPI(&tmp);

            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            N_DMA_ReadByteSPI(&tmp);
	}
    }
    MMC_DisableCS();

    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&tmp);

    return count;
}

/**
 *	checks to see if previous write operation is sucessful
 *	
 *	@return TRUE		if data is successfully written to disk
 *	@return FALSE		if data is not successfully written to disk
 **/
unsigned char MMC_CheckWriteState(void)
{
    unsigned int count = 0;
    BYTE inRes;
    while (count <= 64)
    {
        N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
        N_DMA_ReadByteSPI(&inRes);
        if ((inRes & 0x1F) == 0x05)
	    break;
        else if ((inRes & 0x1F) == 0x0B)
	    return FALSE;
        else if ((inRes & 0x1F) == 0x0D)
	    return FALSE;
        count++;
    }
    if (count > 64)
    {
        __no_operation();
    }
    count = 0;

    while (count < 64) 
    {
        N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
        N_DMA_ReadByteSPI(&inRes);
        if (inRes != 0x00) break;
    }
    
    if(count < 64)
	return TRUE;
    else
	return FALSE;
}

/**
 *	writes content of the buffer to the sector indicated by the input
 *
 *	@param	sector		sector to be written
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes written
 **/
unsigned int MMC_WriteSector(unsigned long sector, unsigned char *buf)
{
    unsigned int count = 0;
    unsigned char tmp;

    //sector += sectorZero;
    //sector *= 2;	

    MMC_EnableCS();

    MMC_SendCommand(CMD24,(sector>>24)&0xFF,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0xFF);

    if (MMC_CardResponse(0x00))
    {
	//N_DMA_WriteByteSPI(0xFF);
	N_DMA_WriteByteSPI(0xFE);
        {
            for (count = 0; count < SECTOR_SIZE; count++)
            {
                N_DMA_WriteByteSPI(buf[count]);
            }
    
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            N_DMA_ReadByteSPI(&tmp);
    
            N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
            N_DMA_ReadByteSPI(&tmp);
    
            MMC_CheckWriteState();
        }
    }

    MMC_DisableCS();

    N_DMA_WriteByteSPI(MMC_DUMMY_DATA);
    N_DMA_ReadByteSPI(&tmp);

    return count;
}

int DMA_MMC_IsIdle(void)
{
	if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
	{
		return 1;
	}
	return 0;
}

int DMA_MMC_ReadSector(unsigned long sector, unsigned char* buf, unsigned char override)
{
    unsigned char temp = 0;

	if (DMA_SPI_IsBeingUsed())
	{
		return -1;
	}
	
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return -1;
    }

    if (override == 0)
    {
        if (DMA_MMC_Write_Processing)
        {
            temp = 1;
        }
    }
            
    if ((read_current_state == 0) && (temp == 0))
    {
        //register DMA_MMC_ReadSector_Proc
        DMA_MMC_Read_Processing = 1;
        read_current_state = 1;
    
        //sector += sectorZero;
        //sector *= 2;
             
        DMA_MMC_ReadBufPtr = buf;
               
        DMA_MMC_WRITE_buffer[0] = CMD17;
        DMA_MMC_WRITE_buffer[1] = (sector>>24)&0xFF;
        DMA_MMC_WRITE_buffer[2] = (sector>>16)&0xFF;
        DMA_MMC_WRITE_buffer[3] = (sector>>8)&0xFF;
        DMA_MMC_WRITE_buffer[4] = sector&0xFF;
        DMA_MMC_WRITE_buffer[5] = 0xFF;
                
		DMA_MMC_CardRes_Count = (unsigned int)CARD_RESPONSE_MAX_COUNT;
		DMA_MMC_ReadResult = 0xFF;	//read
		//probe_high();	//to measure the time to read a block 
        MMC_EnableCS();
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_ReadSector_proc);
    }
    else
    {
		switch (read_current_state)
		{
        case 6:
			//probe_low();	////to measure the time to read a block 
            DMA_MMC_Read_Processing = 0;
            read_current_state = 0;
            return SECTOR_SIZE;
		case 1:
		case 2:
			if (!DMA_SPI_IsBeingUsed())
			{
				//DMA_MMC_CardResponse(0x00, DMA_MMC_WriteSector_proc);
        		DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadSector_proc);
			}
			break;
		case MMC_STATE_end:
            DMA_MMC_Read_Processing = 0;
            read_current_state = 0;
            return 0;
		}
    }
    return -1;
}

void DMA_MMC_ReadSector_proc(void)
{
    switch (read_current_state)
    {
    case 1:
        if ((DMA_MMC_ReadResult != 0x00) && (DMA_MMC_CardRes_Count > 0))
        {
            --DMA_MMC_CardRes_Count;
			if (!DMA_SPI_IsBeingUsed())
			{
				//DMA_MMC_CardResponse(0x00, DMA_MMC_WriteSector_proc);
        		DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadSector_proc);
			}
			return;
		}
		if (DMA_MMC_CardRes_Count == 0)
        {
			read_current_state = 0;
			MMC_DisableCS();
			DMA_MMC_error(READ_SECTOR_ERROR);
			return;
		}
		read_current_state = 2;
		return;
    case 2:
        if ((DMA_MMC_ReadResult != 0xFE) && (DMA_MMC_CardRes_Count > 0))
        {
            --DMA_MMC_CardRes_Count;
			if (!DMA_SPI_IsBeingUsed())
			{
				//DMA_MMC_CardResponse(0x00, DMA_MMC_WriteSector_proc);
        		DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadSector_proc);
			}
			return;
		}
		if (DMA_MMC_CardRes_Count == 0)
        {
			read_current_state = 0;
			MMC_DisableCS();
			DMA_MMC_error(READ_SECTOR_ERROR);
			return;
		}
        read_current_state = 3;
        DMA_SPI_Read(DMA_MMC_ReadBufPtr, SECTOR_SIZE, &DMA_MMC_ReadSector_proc);
		return;
	case 3:
		read_current_state = 4;
		DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
        DMA_MMC_WRITE_buffer[1] = MMC_DUMMY_DATA;
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_ReadSector_proc);
        break;
    case 4:
        read_current_state = 5;
        MMC_DisableCS();
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadSector_proc);
        break;
    case 5:
        if (DMA_MMC_WriteAfterReadProc)
        {
            read_current_state = 6;
            DMA_MMC_WriteAfterReadProc();
            DMA_MMC_WriteAfterReadProc = 0;
        }
        else
        {
            if (DMA_MMC_Transaction_Status() == 1)
            {
                read_current_state = 0;
                DMA_MMC_Read_Processing = 0;
                DMA_MMC_Transaction_CB(1);
            }
            else
            {
                read_current_state = 6;
            }
        }
        break;
    }
}

#ifdef __DMA_MMC_ENABLED__
#ifndef __DMA_MMC_ESSENTIAL_ONLY__
unsigned int DMA_MMC_ReadPartialSector(unsigned long sector,
                                        unsigned int byteOffset,
                                        unsigned int bytesToRead,
                                        unsigned char* buf)
{   
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return 0;
    }
    if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
    {
        if (main_current_state == 0)
        {
            DMA_MMC_Read_Processing = 1;
            main_current_state = 1;
            //register DMA_MMC_WritePartialMultiSector_Proc
                
            DMA_MMC_ReadNumBytes = 0;
            DMA_MMC_byteOffset = byteOffset;
            DMA_MMC_bytesToRead = bytesToRead;
            DMA_MMC_ReadBufPtr = buf;
            
            //sector += sectorZero;
            //sector <<= 1;
            
            DMA_MMC_WRITE_buffer[0] = CMD17;
            DMA_MMC_WRITE_buffer[1] = (sector>>24)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (sector>>16)&0xFF;
            DMA_MMC_WRITE_buffer[3] = (sector>>8)&0xFF;
            DMA_MMC_WRITE_buffer[4] = sector&0xFF;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
                 
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_ReadPartialSector_proc);
        }
    }
    else
    {
        if (main_current_state == 5)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            return DMA_MMC_ReadNumBytes;
        }
        else if (main_current_state == MMC_STATE_end)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            return (unsigned int)-1;
        }
    }
    return 0;
}
 
void DMA_MMC_ReadPartialSector_proc(void)
{
    unsigned char temp;
    int count;
     
    switch (main_current_state)
    {
    case 0:
        break;
    case 1:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_ReadPartialSector_proc);
        if (temp == 1)
        {
            main_current_state = 2;
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(READ_PARTIAL_SECTOR_ERROR);
            }
            break;
        }
    case 2:
        //temp = DMA_MMC_CardResponse(0xFE, &DMA_MMC_ReadPartialSector_proc);
        if (temp == 1)
        {
            main_current_state = 3;
            DMA_SPI_Read(DMA_MMC_READ_buffer, SECTOR_SIZE, &DMA_MMC_ReadPartialSector_proc);
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(READ_PARTIAL_SECTOR_ERROR);
            }
        }
        break;
    case 3:
        main_current_state = 4;
        for (count = 0; count < DMA_MMC_bytesToRead; ++count)
        {
            DMA_MMC_ReadBufPtr[count] = DMA_MMC_READ_buffer[count + DMA_MMC_byteOffset];
        }
        for (; count < SECTOR_SIZE; ++count)
        {
            DMA_MMC_ReadBufPtr[count] = 0;
        }
        DMA_MMC_ReadNumBytes = count;
        DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
        DMA_MMC_WRITE_buffer[1] = MMC_DUMMY_DATA;
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_ReadPartialSector_proc);
        break;
    case 4:
        main_current_state = 5;
        MMC_DisableCS();
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadPartialSector_proc);
        break;
    case 5:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            read_current_state = 6;
        }
        break;
    }
}


unsigned int DMA_MMC_ReadPartialMultiSector(unsigned long sector1,
                                            unsigned long sector2,
                                            unsigned int byteOffset,
                                            unsigned int bytesToRead,
                                            unsigned char* buf)
{
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return 0;
    }
    
    if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
    {
        if (main_current_state == 0)
        {
            //register DMA_MMC_WritePartialMultiSector_Proc
            main_current_state = 1;
            DMA_MMC_Read_Processing = 1;
                
            DMA_MMC_ReadBufPtr = buf;
            DMA_MMC_sector1 = sector1;// + sectorZero;
            //DMA_MMC_sector1 *= 2;
            DMA_MMC_sector2 = sector2;// + sectorZero;
            //DMA_MMC_sector2 *= 2;
            DMA_MMC_byteOffset = byteOffset;
            DMA_MMC_bytesToRead = bytesToRead;
            DMA_MMC_ReadNumBytes = 0;
               
            DMA_MMC_WRITE_buffer[0] = CMD17;
            DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector1>>24)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector1>>16)&0xFF;
            DMA_MMC_WRITE_buffer[3] = (DMA_MMC_sector1>>8)&0xFF;
            DMA_MMC_WRITE_buffer[4] = DMA_MMC_sector1&0xFF;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
                 
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_ReadPartialMultiSector_proc);
        }
    }
    else
    {
        if (main_current_state == 13)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            return DMA_MMC_ReadNumBytes;
        }
        else if (main_current_state == MMC_STATE_end)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            return (unsigned int)-1;
        }
    }
    return 0;
}

void DMA_MMC_ReadPartialMultiSector_proc(void)
{
    unsigned char temp;
    unsigned int count;
    
    switch (main_current_state)
    {
    case 0:
        break;
    case 1:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_ReadPartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 2;
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(READ_PARTIAL_MULTI_SECTOR_ERROR);
            }
            break;
        }
    case 2:
        temp = DMA_MMC_CardResponse(0xFE, &DMA_MMC_ReadPartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 3;
            DMA_SPI_Read(DMA_MMC_READ_buffer, SECTOR_SIZE, &DMA_MMC_ReadPartialMultiSector_proc);
        }
        else
        {
            if (temp == 0)
            {
               DMA_MMC_error(READ_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        break;
    case 3:
        main_current_state = 4;
        for (count = DMA_MMC_byteOffset; count < SECTOR_SIZE; ++DMA_MMC_ReadNumBytes, ++count)
        {
            DMA_MMC_ReadBufPtr[DMA_MMC_ReadNumBytes] = DMA_MMC_READ_buffer[count];
        }
        DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
        DMA_MMC_WRITE_buffer[1] = MMC_DUMMY_DATA;
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 4:
        main_current_state = 5;
        MMC_DisableCS();
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 5:
        main_current_state = 6;
        MMC_DisableCS();
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 6:
        main_current_state = 7;
        DMA_MMC_WRITE_buffer[0] = CMD17;
        DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector2>>24)&0xFF;
        DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector2>>16)&0xFF;
        DMA_MMC_WRITE_buffer[3] = (DMA_MMC_sector2>>8)&0xFF;
        DMA_MMC_WRITE_buffer[4] = DMA_MMC_sector2&0xFF;
        DMA_MMC_WRITE_buffer[5] = 0xFF;
             
        MMC_EnableCS();
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 7:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_ReadPartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 8;
            //DMA_MMC_CardResponse(0xFE);
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(READ_PARTIAL_MULTI_SECTOR_ERROR);
            }
            break;
        }
    case 8:
        temp = DMA_MMC_CardResponse(0xFE, &DMA_MMC_ReadPartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 9;
            DMA_SPI_Read(DMA_MMC_READ_buffer, SECTOR_SIZE, &DMA_MMC_ReadPartialMultiSector_proc);
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(READ_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        break;
    case 9:
        main_current_state = 10;
        for (count = 0; DMA_MMC_ReadNumBytes < DMA_MMC_bytesToRead; ++count, ++DMA_MMC_ReadNumBytes)
        {
            DMA_MMC_ReadBufPtr[DMA_MMC_ReadNumBytes] = DMA_MMC_READ_buffer[count];
        }
        for (count = DMA_MMC_ReadNumBytes; count < SECTOR_SIZE; ++count)
        {
            DMA_MMC_ReadBufPtr[count] = 0;
        }
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 10:
        main_current_state = 11;
        DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
        DMA_MMC_WRITE_buffer[1] = MMC_DUMMY_DATA;
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 11:
        main_current_state = 12;
        MMC_DisableCS();
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_ReadPartialMultiSector_proc);
        break;
    case 12:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            read_current_state = 13;
        }
        break;
    }
}
#endif 	//__DMA_MMC_ESSENTIAL_ONLY__
#endif 	//__DMA_MMC_ENABLED__


int DMA_MMC_WriteSector(unsigned long sector, unsigned char* buf)
{
	if (DMA_SPI_IsBeingUsed())
	{
		return -1;
	}
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return -1;
    }
    if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
    {
        if (main_current_state == 0)
        {
            DMA_MMC_Write_Processing = 1;
            main_current_state = 1;
            //register DMA_MMC_WriteSector_proc();
            
            DMA_MMC_temp_WRITE_buffer = buf;
            //DMA_MMC_sector1 = sector + sectorZero;
            //DMA_MMC_sector1 *= 2;
            DMA_MMC_sector1 = sector;
            DMA_MMC_WriteNumBytes = 0;
            
            DMA_MMC_WRITE_buffer[0] = CMD24;
            DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector1>>24)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector1>>16)&0xFF;
            DMA_MMC_WRITE_buffer[3] = (DMA_MMC_sector1>>8)&0xFF;
            DMA_MMC_WRITE_buffer[4] = (DMA_MMC_sector1)&0xFF;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
            
			DMA_MMC_CardRes_Count = (unsigned int)CARD_RESPONSE_MAX_COUNT;
			
			DMA_MMC_ReadResult = 0xFF;
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, 0);
        }
    }
    else
    {
		switch (main_current_state)
		{
        case 10:
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return DMA_MMC_WriteNumBytes;
        case 0xFF:
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return 0;
        case 1:
			if (!DMA_SPI_IsBeingUsed())
			{
				//DMA_MMC_CardResponse(0x00, DMA_MMC_WriteSector_proc);
        		DMA_SPI_Read(&DMA_MMC_ReadResult, 1, DMA_MMC_WriteSector_proc);
			}
			break;
		case 8:
			if (!DMA_SPI_IsBeingUsed())
			{
				DMA_SPI_Read(&DMA_MMC_ReadResult, 1, DMA_MMC_WriteSector_proc);
			}
		}
    }
    return -1;
}

/*
unsigned char DMA_MMC_CardResponse(unsigned char expRex, void (*cb_fp)(void))
{
    if (DMA_MMC_CardRes_State == 0)
    {
        DMA_MMC_CardRes_State = 1;
        DMA_MMC_CardRes_Count = (unsigned int)CARD_RESPONSE_MAX_COUNT;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, cb_fp);
        return 0xFF;
    }
    else
    {
        if ((DMA_MMC_ReadResult != expRex) && (DMA_MMC_CardRes_Count > 0))
        {
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, cb_fp);
            --DMA_MMC_CardRes_Count;
            return 0xFF;
        }
        if (DMA_MMC_CardRes_Count == 0)
        {
            DMA_MMC_CardRes_State = 0;
            return 0;
        }
    }
    DMA_MMC_CardRes_State = 0;
    return 1;
}
*/

void DMA_MMC_WriteSector_proc(void)
{
    //unsigned char temp;
    //unsigned int count = 0;
    switch (main_current_state)
    {
    case 0:
        break;
    case 1:
        //temp = MMC_CardResponse(0x00);
        //temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_WriteSector_proc);
		if ((DMA_MMC_ReadResult != 0x00) && (DMA_MMC_CardRes_Count > 0))
        {
            --DMA_MMC_CardRes_Count;
			if (!DMA_SPI_IsBeingUsed())
			{
				//DMA_MMC_CardResponse(0x00, DMA_MMC_WriteSector_proc);
        		DMA_SPI_Read(&DMA_MMC_ReadResult, 1, DMA_MMC_WriteSector_proc);
			}
			return;
		}
		if (DMA_MMC_CardRes_Count == 0)
        {
			main_current_state = 0;
			MMC_DisableCS();
			DMA_MMC_error(WRITE_SECTOR_ERROR);
			return;
		}	
        main_current_state = 2;
        DMA_MMC_WRITE_buffer[0] = 0xFF;
        DMA_MMC_WRITE_buffer[1] = 0xFE;
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_WriteSector_proc);
		break;
    case 2:
        main_current_state = 3;
        DMA_MMC_WriteNumBytes = SECTOR_SIZE;
        DMA_SPI_Write(DMA_MMC_temp_WRITE_buffer, DMA_MMC_WriteNumBytes, &DMA_MMC_WriteSector_proc);
        break;
    case 3:
        main_current_state = 7;
        DMA_SPI_Read(DMA_MMC_READ_buffer, 4, &DMA_MMC_WriteSector_proc);
        break;
    case 4:
        //main_current_state = 5;
        //DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WriteSector_proc);
        break;
    case 5:
        //main_current_state = 6;
        //DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WriteSector_proc);
        break;
    case 6:
        //DMA_MMC_CallbackAfterWriteCheck = &DMA_MMC_WriteSector_proc;
        //temp = DMA_MMC_CheckWriteState();
        //main_current_state = 7;
        //DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WriteSector_proc);
        break;
    case 7:
        if ((DMA_MMC_READ_buffer[3] & 0x1F) == 0x05)
        {
            main_current_state = 8;
            DMA_MMC_WriteStateCount = 0;
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WriteSector_proc);    
        }
        else
        {
            //write error
			main_current_state = 0;
			MMC_DisableCS();
            DMA_MMC_error(WRITE_SECTOR_ERROR);
        }
        break;
    case 8:
        if (DMA_MMC_ReadResult == 0)
        {
            if (DMA_MMC_WriteStateCount < DMA_MMC_MAX_WRITESTATE_COUNT)
            {
                ++DMA_MMC_WriteStateCount;
            }
            else
            {
                //write error
				main_current_state = 0;
				MMC_DisableCS();
                DMA_MMC_error(WRITE_SECTOR_ERROR);
            }
        }
        else
        {
            main_current_state = 9;
            MMC_DisableCS();
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WriteSector_proc);
        }
        break;
    case 9:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            main_current_state = 10;
        }
        break; 
    }
}

#ifdef __DMA_MMC_ENABLED__
#ifndef __DMA_MMC_ESSENTIAL_ONLY__
unsigned int DMA_MMC_WritePartialSector(unsigned long sector,
                                        unsigned int byteOffset,
                                        unsigned int bytesToWrite,
                                        unsigned char* buf)
{
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return 0;
    }
    
    if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
    {
        if (main_current_state == 0)
        {
            DMA_MMC_Write_Processing = 1;
            main_current_state = 1;
            //register DMA_MMC_WritePartialSector_proc();
            
            DMA_MMC_temp_WRITE_buffer = buf;
            DMA_MMC_sector1 = sector;// + sectorZero;
            //DMA_MMC_sector1 *= 2;
            DMA_MMC_byteOffset = byteOffset;
            DMA_MMC_bytesToWrite = bytesToWrite;
            DMA_MMC_WriteNumBytes = 0;
              
            DMA_MMC_WriteAfterReadProc = &DMA_MMC_WritePartialSector_proc;
            DMA_MMC_ReadSector(sector, DMA_MMC_READ_buffer, 1);
            //DMA_MMC_WritePartialSector_proc();
        }
    }
    else
    {
        if (main_current_state == 11)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return DMA_MMC_WriteNumBytes;
        }
        else if (main_current_state == MMC_STATE_end)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return (unsigned int) -1;
        }
    }
    return 0;
}

void DMA_MMC_WritePartialSector_proc(void)
{
    unsigned char temp;
    unsigned int count;
    
    switch (main_current_state)
    {
    case 1:
        if (DMA_MMC_ReadSector(DMA_MMC_sector1, DMA_MMC_READ_buffer, 1) == SECTOR_SIZE)
        {
            main_current_state = 2;
            
            //DMA_MMC_sector1 += sectorZero;
            //DMA_MMC_sector1 *= 2;
        
            DMA_MMC_WRITE_buffer[0] = CMD24;
            DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector1>>16)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector1>>8)&0xFF;
            DMA_MMC_WRITE_buffer[3] = DMA_MMC_sector1&0xFF;
            DMA_MMC_WRITE_buffer[4] = 0;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
            
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_WritePartialSector_proc);
        }
        break;
    case 2:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_WritePartialSector_proc);
        if (temp == 1)
        {
            main_current_state = 3;
            DMA_MMC_WRITE_buffer[0] = 0xFF;
            DMA_MMC_WRITE_buffer[1] = 0xFE;
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_WritePartialSector_proc);
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(WRITE_PARTIAL_SECTOR_ERROR);
            }
        }
        break;
    case 3:
        main_current_state = 4;
        //use memcpy later
        for (count = 0; count < DMA_MMC_byteOffset; ++count)		
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_READ_buffer[count];
        }
            
        for (DMA_MMC_WriteNumBytes = 0; DMA_MMC_WriteNumBytes < DMA_MMC_bytesToWrite; ++DMA_MMC_WriteNumBytes, ++count)
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_temp_WRITE_buffer[DMA_MMC_WriteNumBytes];
        }
        
        for (; count < SECTOR_SIZE; ++count)
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_READ_buffer[count];
        }
        
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, count, &DMA_MMC_WritePartialSector_proc);
        break;
    case 4:
        main_current_state = 5; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
        break;
    case 5:
        main_current_state = 6;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
        break;
    case 6:
        main_current_state = 7;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
        break;
    case 7:
        main_current_state = 8;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
        break;
    case 8:
        if ((DMA_MMC_ReadResult & 0x1F) == 0x05)
        {
            main_current_state = 9;
            DMA_MMC_WriteStateCount = 0;
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
        }
        else
        {
            DMA_MMC_error(WRITE_PARTIAL_SECTOR_ERROR);
        }
        break;
    case 9:
        if ((DMA_MMC_ReadResult & 0x1F) == 0x00)
        {
            if (DMA_MMC_WriteStateCount < DMA_MMC_MAX_WRITESTATE_COUNT)
            {
                ++DMA_MMC_WriteStateCount;
                DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
            }
            else
            {
                DMA_MMC_error(WRITE_PARTIAL_SECTOR_ERROR);
            }
        }
        else
        {
            main_current_state = 10;
            MMC_DisableCS();
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialSector_proc);
        }
        break;
    case 10:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            main_current_state = 11;
        }
        break;
    }
}

unsigned int DMA_MMC_WritePartialMultiSector(unsigned long sector1,
                                             unsigned long sector2,
                                             unsigned int byteOffset,
                                             unsigned int bytesToWrite,
                                             unsigned char *buf)
{
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return 0;
    }
    
    if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
    {
        if (main_current_state == 0)
        {
            //register DMA_MMC_WritePartialMultiSector_Proc
            main_current_state = 1;
            DMA_MMC_Write_Processing = 1;
            
            DMA_MMC_temp_WRITE_buffer = buf;
            DMA_MMC_sector1 = sector1;
            DMA_MMC_sector2 = sector2;
            DMA_MMC_byteOffset = byteOffset;
            DMA_MMC_bytesToWrite = bytesToWrite;
            DMA_MMC_WriteNumBytes = 0;
             
            DMA_MMC_WriteAfterReadProc = &DMA_MMC_WritePartialMultiSector_proc;
            DMA_MMC_ReadSector(DMA_MMC_sector1, DMA_MMC_READ_buffer, 1);
        }
    }
    else 
    {
        if (main_current_state == 21)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return DMA_MMC_WriteNumBytes;
        }
        else if (main_current_state == MMC_STATE_end)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return (unsigned int) -1;
        }
    }
    return 0;
}

void DMA_MMC_WritePartialMultiSector_proc(void)
{
    unsigned char temp;
    unsigned int count;
    unsigned int writeSector2;
    
    switch (main_current_state)
    {
    case 1:
        if (DMA_MMC_ReadSector(DMA_MMC_sector1, DMA_MMC_READ_buffer, 1) == SECTOR_SIZE)
        {
            main_current_state = 2;
            
            //DMA_MMC_sector1 += sectorZero;
            //DMA_MMC_sector1 *= 2;
        
            DMA_MMC_WRITE_buffer[0] = CMD24;
            DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector1>>24)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector1>>16)&0xFF;
            DMA_MMC_WRITE_buffer[3] = (DMA_MMC_sector1>>8)&0xFF;
            DMA_MMC_WRITE_buffer[4] = DMA_MMC_sector1&0xFF;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
            
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 2:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_WritePartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 3;
            
            DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
            DMA_MMC_WRITE_buffer[1] = 0xFE;
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            if (temp == 0)  // cancel
            {
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        break;
    case 3:
        main_current_state = 4;
        for (count = 0; count < DMA_MMC_byteOffset; ++count)		
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_READ_buffer[count];
        }
        for (count = DMA_MMC_byteOffset; count < SECTOR_SIZE; ++DMA_MMC_WriteNumBytes, ++count)
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_temp_WRITE_buffer[DMA_MMC_WriteNumBytes];
        }
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, count, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 4:
        main_current_state = 5; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 5:
        main_current_state = 6;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 6:
        main_current_state = 7; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 7:
        main_current_state = 8;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 8:
        if ((DMA_MMC_ReadResult & 0x1F) == 0x05)
        {
            main_current_state = 9;
            DMA_MMC_WriteStateCount = 0;
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
        }
        break;
    case 9:
        if (DMA_MMC_ReadResult == 0)
        {
            if (DMA_MMC_WriteStateCount < DMA_MMC_MAX_WRITESTATE_COUNT)
            {
                ++DMA_MMC_WriteStateCount;
                DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
            }
            else
            { 
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        else
        {
            main_current_state = 10;
            MMC_DisableCS();
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 10:
        main_current_state = 11;
        DMA_MMC_WriteAfterReadProc = &DMA_MMC_WritePartialMultiSector_proc;
        DMA_MMC_ReadSector(DMA_MMC_sector2, DMA_MMC_READ_buffer, 1);
        break;
    case 11:
        if (DMA_MMC_ReadSector(DMA_MMC_sector2, DMA_MMC_READ_buffer, 1))
        {
            main_current_state = 12;
            
            //DMA_MMC_sector2 += sectorZero;
            //DMA_MMC_sector2 *= 2;
        
            DMA_MMC_WRITE_buffer[0] = CMD24;
            DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector2>>24)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector2>>16)&0xFF;
            DMA_MMC_WRITE_buffer[3] = (DMA_MMC_sector2>>8)&0xFF;
            DMA_MMC_WRITE_buffer[4] = DMA_MMC_sector2&0xFF;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
            
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 12:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_WritePartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 13;   
            DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
            DMA_MMC_WRITE_buffer[1] = 0xFE;
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        break;
    case 13:
        main_current_state = 14;
        writeSector2 = DMA_MMC_bytesToWrite;
        writeSector2 += DMA_MMC_byteOffset;   
        writeSector2 -= SECTOR_SIZE;
        //use memcpy later
        for (count = 0; count < writeSector2; ++DMA_MMC_WriteNumBytes, ++count)		
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_temp_WRITE_buffer[DMA_MMC_WriteNumBytes];
        }
        for (count = writeSector2; count < SECTOR_SIZE; ++count)
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_READ_buffer[count];
        }
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, count, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 14:
        main_current_state = 15; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 15:
        main_current_state = 16;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 16:
        main_current_state = 17; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 17:
        main_current_state = 18;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 18:
        if ((DMA_MMC_ReadResult & 0x1F) == 0x05)
        {
            main_current_state = 19;
            DMA_MMC_WriteStateCount = 0;
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
        }
        break;
    case 19:
        if (DMA_MMC_ReadResult == 0)
        {
            if (DMA_MMC_WriteStateCount < DMA_MMC_MAX_WRITESTATE_COUNT)
            {
                ++DMA_MMC_WriteStateCount;
                DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
            }
            else
            { 
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        else
        {
            main_current_state = 20;
            MMC_DisableCS();
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 20:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            main_current_state = 21;
        }
        break;
    }
}

#endif 	//__DMA_MMC_ESSENTIAL_ONLY__
#endif 	//__DMA_MMC_ENABLED__


/*
//sector list must end with 0xFFFFFFFF
void DMA_Start_Write( unsigned long* sectorList,
                      unsigned int byteOffset,
                      unsigned int bytesToWrite,
                      unsigned char *buf,
                      unsigned char forceWrite )
{
    unsigned long count;
    unsigned long* temp;
    
    if ((DMA_MMC_Transaction_Status() == 1) && (DMA_MMC_get_Current_Transaction() == 0))
    {
        return;
    }
    
    if ((DMA_MMC_Write_Processing == 0) && (DMA_MMC_Read_Processing == 0))
    {
        if (main_current_state == 0)
        {
            count = 0;
            temp = sectorList;
            while (*temp != 0xFFFFFFFF)
            {
                ++count;
            }
            if ((bytesToWrite >> 9) <= count)
            {
                //register DMA_MMC_WritePartialMultiSector_Proc
                main_current_state = 1;
                DMA_MMC_Write_Processing = 1;
                
                DMA_MMC_temp_WRITE_buffer = buf;
                DMA_MMC_sectorList = sectorList;
                DMA_MMC_byteOffset = byteOffset;
                DMA_MMC_bytesToWrite = bytesToWrite;
                DMA_MMC_WriteNumBytes = 0;
                DMA_MMC_forceWrite = forceWrite;
                
                if (forceWrite || ((offset == 0) && (bytesToWrite >= 512)))
                {
                    DMA_MMC_WriteAfterReadProc = 0;
                    
                    main_current_state = 2;
                    
                    DMA_MMC_sector1 += sectorZero;
                    DMA_MMC_sector1 *= 2;
                
                    DMA_MMC_WRITE_buffer[0] = CMD24;
                    DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector1>>16)&0xFF;
                    DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector1>>8)&0xFF;
                    DMA_MMC_WRITE_buffer[3] = DMA_MMC_sector1&0xFF;
                    DMA_MMC_WRITE_buffer[4] = 0;
                    DMA_MMC_WRITE_buffer[5] = 0xFF;
                    
                    MMC_EnableCS();
                    DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_Write_Start_proc);
                }
            }
            else
            {
                DMA_MMC_WriteAfterReadProc = &DMA_Write_Start_proc;
                DMA_MMC_ReadSector(DMA_MMC_sector1, DMA_MMC_READ_buffer, 1);
            }
        }
    }
    else 
    {
        if (main_current_state == 4)
        {
            main_current_state = 0;
            //DMA_MMC_Write_Processing = 0;
            return DMA_MMC_WriteNumBytes;
        }
        else if (main_current_state == MMC_STATE_end)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            return (unsigned int) -1;
        }
    }
    return 0;
}
                      
void DMA_Write_Start_Proc(void)
{
    switch (main_current_state)
    {
    case 1:
        if (DMA_MMC_WriteAfterReadProc)
        {
            if (DMA_MMC_ReadSector(DMA_MMC_sector1, DMA_MMC_READ_buffer, 1) == SECTOR_SIZE)
            {
                main_current_state = 2;
                
                DMA_MMC_sector1 += sectorZero;
                DMA_MMC_sector1 *= 2;
            
                DMA_MMC_WRITE_buffer[0] = CMD24;
                DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector1>>16)&0xFF;
                DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector1>>8)&0xFF;
                DMA_MMC_WRITE_buffer[3] = DMA_MMC_sector1&0xFF;
                DMA_MMC_WRITE_buffer[4] = 0;
                DMA_MMC_WRITE_buffer[5] = 0xFF;
                
                MMC_EnableCS();
                DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_Write_Start_Proc);
            }
        } 
        break;
    case 2:
        temp = DMA_MMC_CardResponse(0x00, &DMA_Write_Start_Proc);
        if (temp == 1)
        {
            main_current_state = 3;
            
            DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
            DMA_MMC_WRITE_buffer[1] = 0xFE;
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_Write_Start_Proc);
        }
        else
        {
            if (temp == 0)  // cancel
            {
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        break;
    case 3:
        if (DMA_MMC_forceWrite == 0)
        {
            for (count = 0; count < DMA_MMC_byteOffset; ++count)		
            {
                DMA_MMC_WRITE_buffer[count] = DMA_MMC_READ_buffer[count];
            }
        }
        else
        {
            for (count = 0; count < DMA_MMC_byteOffset; ++count)		
            {
                DMA_MMC_WRITE_buffer[count] = 0;
            }
        }
        if (DMA_MMC_bytesToWrite < 512)
        {
            for (; count < DMA_MMC_bytesToWrite; ++count)
        for (; count < SECTOR_SIZE; ++DMA_MMC_WriteNumBytes, ++count)
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_temp_WRITE_buffer[DMA_MMC_WriteNumBytes];
        }
        DMA_MMC_WriteNumBytes = count;
        if (count < 512)
        {
            main_current_state = 11;
        }
        else
        {
            main_current_state = 4;
        }
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, count, &DMA_Write_Start_Proc);
        break;
    case 4:
        main_current_state = 5;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 5:
        main_current_state = 6;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 6:
        main_current_state = 7; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 7:
        main_current_state = 8;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 8:
        if ((DMA_MMC_ReadResult & 0x1F) == 0x05)
        {
            main_current_state = 9;
            DMA_MMC_WriteStateCount = 0;
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
        }
        break;
    case 9:
        if (DMA_MMC_ReadResult == 0)
        {
            if (DMA_MMC_WriteStateCount < DMA_MMC_MAX_WRITESTATE_COUNT)
            {
                ++DMA_MMC_WriteStateCount;
                DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
            }
            else
            { 
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        else
        {
            main_current_state = 10;
            MMC_DisableCS();
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 10:
        main_current_state = 11;
        DMA_MMC_WriteAfterReadProc = &DMA_MMC_WritePartialMultiSector_proc;
        DMA_MMC_ReadSector(DMA_MMC_sector2, DMA_MMC_READ_buffer, 1);
        break;     
    case 11:
        DMA_MMC_appendMode = 1;
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            //DMA_MMC_Write_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            main_current_state = 12;
        }
        break;
    }
}

unsigned char DMA_append(unsigned char* buf, unsigned char length)
{
    if (DMA_MMC_appendMode == 1)
    {
        if ((DMA_MMC_WriteNumBytes & 512) == 0)
        {
            
    }
}
    case 11:
        if (DMA_MMC_ReadSector(DMA_MMC_sector2, DMA_MMC_READ_buffer, 1))
        {
            main_current_state = 12;
            
            DMA_MMC_sector2 += sectorZero;
            DMA_MMC_sector2 *= 2;
        
            DMA_MMC_WRITE_buffer[0] = CMD24;
            DMA_MMC_WRITE_buffer[1] = (DMA_MMC_sector2>>16)&0xFF;
            DMA_MMC_WRITE_buffer[2] = (DMA_MMC_sector2>>8)&0xFF;
            DMA_MMC_WRITE_buffer[3] = DMA_MMC_sector2&0xFF;
            DMA_MMC_WRITE_buffer[4] = 0;
            DMA_MMC_WRITE_buffer[5] = 0xFF;
            
            MMC_EnableCS();
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 6, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 12:
        temp = DMA_MMC_CardResponse(0x00, &DMA_MMC_WritePartialMultiSector_proc);
        if (temp == 1)
        {
            main_current_state = 13;   
            DMA_MMC_WRITE_buffer[0] = MMC_DUMMY_DATA;
            DMA_MMC_WRITE_buffer[1] = 0xFE;
            DMA_SPI_Write(DMA_MMC_WRITE_buffer, 2, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            if (temp == 0)
            {
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        break;
    case 13:
        main_current_state = 14;
        writeSector2 = DMA_MMC_bytesToWrite;
        writeSector2 += DMA_MMC_byteOffset;   
        writeSector2 -= SECTOR_SIZE;
        //use memcpy later
        for (count = 0; count < writeSector2; ++DMA_MMC_WriteNumBytes, ++count)		
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_temp_WRITE_buffer[DMA_MMC_WriteNumBytes];
        }
        for (count = writeSector2; count < SECTOR_SIZE; ++count)
        {
            DMA_MMC_WRITE_buffer[count] = DMA_MMC_READ_buffer[count];
        }
        DMA_SPI_Write(DMA_MMC_WRITE_buffer, count, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 14:
        main_current_state = 15; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 15:
        main_current_state = 16;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 16:
        main_current_state = 17; 
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 17:
        main_current_state = 18;
        DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        break;
    case 18:
        if ((DMA_MMC_ReadResult & 0x1F) == 0x05)
        {
            main_current_state = 19;
            DMA_MMC_WriteStateCount = 0;
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        else
        {
            DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
        }
        break;
    case 19:
        if (DMA_MMC_ReadResult == 0)
        {
            if (DMA_MMC_WriteStateCount < DMA_MMC_MAX_WRITESTATE_COUNT)
            {
                ++DMA_MMC_WriteStateCount;
                DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
            }
            else
            { 
                DMA_MMC_error(WRITE_PARTIAL_MULTI_SECTOR_ERROR);
            }
        }
        else
        {
            main_current_state = 20;
            MMC_DisableCS();
            DMA_SPI_Read(&DMA_MMC_ReadResult, 1, &DMA_MMC_WritePartialMultiSector_proc);
        }
        break;
    case 20:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            DMA_MMC_Transaction_CB(1);
        }
        else
        {
            main_current_state = 21;
        }
        break;
    }
}
*/

void DMA_MMC_error(DMA_MMC_error_t rw)
{
    MMC_DisableCS();
    switch (rw)
    {
    case READ_SECTOR_ERROR:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            if (DMA_MMC_WriteAfterReadProc)
            {
                main_current_state = 0;
                DMA_MMC_Write_Processing = 0;
            }
            read_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            DMA_MMC_Transaction_CB(0);
        }
        else
        {
            read_current_state = MMC_STATE_end;
            if (DMA_MMC_WriteAfterReadProc)
            {
                DMA_MMC_WriteAfterReadProc();
                DMA_MMC_WriteAfterReadProc = 0;
            }
        }
        break;
    case READ_PARTIAL_SECTOR_ERROR:
    case READ_PARTIAL_MULTI_SECTOR_ERROR:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Read_Processing = 0;
            DMA_MMC_Transaction_CB(0);
        }
        else
        {
            main_current_state = MMC_STATE_end;
        }
        break;
    case WRITE_SECTOR_ERROR:
    case WRITE_PARTIAL_SECTOR_ERROR:
    case WRITE_PARTIAL_MULTI_SECTOR_ERROR:
        if (DMA_MMC_Transaction_Status() == 1)
        {
            main_current_state = 0;
            DMA_MMC_Write_Processing = 0;
            DMA_MMC_Transaction_CB(0);
        }
        else
        {
            main_current_state = MMC_STATE_end;
        }
        break;
    }
}
