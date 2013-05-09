#ifndef __DMA_SPI_H
#define __DMA_SPI_H

#define DUMMY_BYTES_LENGTH 1024
#define MMC_DUMMY_DATA      0xFF

extern void init_DMA_SPI(unsigned char db);
extern unsigned char DMA_SPI_Write(unsigned char* buf, unsigned long length, void (*fp)(void));
extern unsigned char DMA_SPI_Read(unsigned char* buf, unsigned long length, void (*fp)(void));
extern void setDMA0IFG_CB(void (*func)(void));
extern void setDMA1IFG_CB(void (*func)(void));
extern void N_DMA_WriteByteSPI(unsigned char aByte);
extern void N_DMA_ReadByteSPI(unsigned char* ptr);
extern unsigned char DMA_SPI_IsBeingUsed(void);

#endif
