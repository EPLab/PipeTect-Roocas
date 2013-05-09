#ifndef __FAT32_FSINFO_SECTOR_H
#define __FAT32_FSINFO_SECTOR_H

typedef struct _FAT32_FSInfo{
    unsigned char FirstSignature[4];
    unsigned char SigFSInfoSector[4];
    unsigned long NumFreeClusters;
    unsigned long RecentlyAccessedClusterNum;
    unsigned char Reserved[12];
    unsigned char ExecutableCode[2];
    unsigned char BootRecordSignature[2];
} FAT32_FSInfo_t;

#endif
