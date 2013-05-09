#ifndef __FAT32_DIRECTORY_ENTRY
#define __FAT32_DIRECTORY_ENTRY

typedef struct _FAT32_DirectoryEntry{
    unsigned char DirName[8];
    unsigned char DirExtention[3];
    unsigned char Attributes;
    unsigned char Reserved;
    unsigned char CreateTime10ms;
    unsigned short CreateTime;
    unsigned short CreateDate;
    unsigned short LastAccessDate;
    unsigned short WriteTime;
    unsigned short WriteDate;
    unsigned long FirstClusterNum;
    unsigned long FileSize;
} FAT32_DirectoryEntry_t;

#endif
