#ifndef __BYTE_QUEUE_H
#define __BYTE_QUEUE_H

typedef struct _ByteQueue{
	int inCount;
	int outCount;
	int numBytes;
	int maxBytes;
	unsigned char* buffer;
} ByteQueue_t;


extern unsigned char BQ_init(ByteQueue_t* q, unsigned char* buf, int size);
extern unsigned char BQ_enqueue(ByteQueue_t* q, unsigned char byte);
extern int BQ_enqueue_msg(ByteQueue_t* q, unsigned char* data, int length);
extern unsigned char BQ_dequeue(ByteQueue_t* q, unsigned char* byte);
extern int BQ_spaceLeft(ByteQueue_t* q);

#endif
