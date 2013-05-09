#define DMA_UART_OUTGOING_BUFFER_SIZE 1000

unsigned char DMA_UART_OutGoing_buffer[2][DMA_UART_OUTGOING_BUFFER_SIZE];
unsigned short DMA_UART_inPrt = 0;
unsigned char DMA_UART_current_buffer_ind = 0;

unsigned char DMA_UART_init(void)
{
    DMACTL
}

unsigned char DMA_UART_Enqueue(unsigned char* data, unsigned short length)
{
    unsigned short i;
    
    if ((DMA_UART_inPtr + length) < DMA_UART_OUTGOING_BUFFER_SIZE)
    {
        for (i = 0; i < length; ++DMA_UART_inPtr, ++i)
        {
            DMA_UART_Outgoing_buffer[DMA_UART_current_buffer_ind][DMA_UART_inPtr] = data[i];
        }
    }
    else
    {
        DMA_UART_send_buffer(DMA_UART_current_buffer[DMA_UART_current_ind], DMA_UART_inPtr);
        DMA_UART_current_buffer_ind ^= 1;
        for (DMA_UART_inPrt = 0; DMA_UART_inPrt < length; ++DMA_UART_inPrt)
        {
            DMA_UART_Outgoing_buffer[DMA_UART_current_buffer_ind][DMA_UART_inPtr] = data[DMA_UART_inPtr];
        }
    }
}

void DMA_UART_send_buffer(unsigned char* buf, unsigned char length)
{
    