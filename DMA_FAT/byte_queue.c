#include "byte_queue.h"
#include "string.h"

unsigned char BQ_init(ByteQueue_t* q, unsigned char* buf, int size)
{
	if (q && buf && size)
	{
		q->inCount = 0;
		q->outCount = 0;
		q->numBytes = 0;
		q->buffer = buf;
		q->maxBytes = size;
		return 1;
	}
	return 0;
}

int num_miss = 0;

int BQ_enqueue_msg(ByteQueue_t* q, unsigned char* data, int length)
{
	int temp = 0;
	
	if ((q->numBytes + length) <= q->maxBytes)
	{
		if ((q->inCount + length) > q->maxBytes)
		{
			temp = q->maxBytes - q->inCount;
			memcpy(&q->buffer[q->inCount], data, temp);
			q->inCount = 0;
			length -= temp;
			q->numBytes += temp;
			data = &data[temp];
		}
		memcpy(&q->buffer[q->inCount], data, length);
		q->inCount += length;
		q->inCount %= q->maxBytes;
		q->numBytes += length;
		return 1;
	}
	++num_miss;
	return 0;
}

unsigned char BQ_enqueue(ByteQueue_t* q, unsigned char byte)
{
	if (q->numBytes < q->maxBytes)
	{
		q->buffer[q->inCount] = byte;
		++q->inCount;
		q->inCount %= q->maxBytes;
		++q->numBytes;
		return 1;
	}
	return 0;
}

unsigned char BQ_dequeue(ByteQueue_t* q, unsigned char* byte)
{
	unsigned char temp;
	
	if (q->numBytes > 0)
	{
		temp = q->buffer[q->outCount];
		++q->outCount;
		q->outCount %= q->maxBytes;
		--q->numBytes;
		*byte = temp;
		return 1;
	}
	return 0;
}

int BQ_spaceLeft(ByteQueue_t* q)
{
	return q->numBytes;
}
