#ifndef __FAT32_PBR_H
#define __FAT32_PBR_H

typedef struct _FAT32_PBR
{
    unsigned char JumpCode[3];                  //0x00  0
    unsigned char OEM_Name[8];                  //0x03  1
    unsigned short BytesPerSector;              //0x0B  2
    unsigned char SectorsPerCluster;            //0x0D  3
    unsigned short NumReservedSectors;          //0x0E  4
    unsigned char NumCopiesOfFAT;               //0x10  5
    unsigned short MaxRootDirectoryEntries;     //0x11  6
    unsigned short NumSectorsInSmallPartition;  //0x13  7
    unsigned char MediaDescriptor;              //0x15  8
    unsigned short SectorsPerOldFAT;            //0x16  9
    unsigned short SectorsPerTrack;             //0x18  10
    unsigned short NumHeads;                    //0x1A  11
    unsigned long NumHiddenSectorsInPartition;  //0x1C  12
    unsigned long NumSectorsInPartition;        //0x20  13
    unsigned long NumSectorsPerFAT;             //0x24  14
    unsigned short Flags;                       //0x28  15
    unsigned short Version;                     //0x2A  16
    unsigned long ClusterNumOfStartRootDir;     //0x2C  17
    unsigned short SectorNumOfFileSysInfo;      //0x30  18
    unsigned short SectorNumOfBackupBootSector; //0x32  19
    unsigned char reserved[12];                 //0x34  20
    unsigned char LogicalDriveNumOfPartition;   //0x40  21
    unsigned char Unused;                       //0x41  22
    unsigned char ExtendedSignature;            //0x42  (29h)
    unsigned long SerialNumOfPartition;         //0x43  24
    unsigned char VolumeNameOfPartition[11];    //0x47  25
    unsigned char FATName[8];                   //0x52  26
    unsigned char ExecutableCode[420];          //0x5A  27
    unsigned char BootRecordSignature[2];       //0x1FE 28
} FAT32_PBR_t;

#endif
