#ifndef __UTX_H
#define __UTX_H

#define UTX_BUFFER_SIZE 512

extern void UTX_init(void);
extern int UTX_enqueue(unsigned char* data, int size);
extern void UTX_clearOnTransmissionFlag(void);

#endif
