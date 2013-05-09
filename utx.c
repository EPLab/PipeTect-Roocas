#include "utx.h"
#include "sys_isr.h"
#include "msp430x54x.h"

ByteQueue_t utxQueue;
unsigned char utxBuffer[UTX_BUFFER_SIZE];


void UTX_init(void)
{
	BQ_init(&utxQueue, utxBuffer, sizeof(utxBuffer));
	ISR_SetUTXQueue(&utxQueue);
	UTX_ClearOnTransmissionFlag();
}

int UTX_enqueue(unsigned char* data, int size)
{
	int i;
	unsigned char temp;
	
	if (UTX_GetOnTransmissionFlag() == 0)
	{
		i = BQ_enqueue_msg(&utxQueue, data + 1, size - 1);
		if (i)
		{
			UTX_SetOnTransmissionFlag();
			UCA2TXBUF = data[0];
			return 1;
		}
		return 0;
	}
	i = BQ_enqueue_msg(&utxQueue, data, size);
	// in case, the transmission terminated during enqueueing
	if (UTX_GetOnTransmissionFlag() == 0)
	{
		BQ_dequeue(&utxQueue, &temp);
		UCA2TXBUF = temp;
	}
	return i;
}
