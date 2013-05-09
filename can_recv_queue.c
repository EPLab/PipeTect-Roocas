#include "can_recv_queue.h"

can_recv_queue_item_t mCan_recv_queue[CAN_RECV_QUEUE_SIZE];
BYTE mCan_recv_queue_in_count = 0;
BYTE mCan_recv_queue_out_count = 0;
BYTE mCan_recv_queue_item_count = 0;

can_recv_queue_item_t* can_recv_enqueue(void)
{
    can_recv_queue_item_t* temp;
    
    if (mCan_recv_queue_item_count < CAN_RECV_QUEUE_SIZE)
    {
        temp = &mCan_recv_queue[mCan_recv_queue_in_count];
        mCan_recv_queue_in_count = (mCan_recv_queue_in_count + 1) % CAN_RECV_QUEUE_SIZE;
        ++mCan_recv_queue_item_count;
        return temp;
    }
    return 0;
}

can_recv_queue_item_t* can_recv_dequeue(void)
{
    can_recv_queue_item_t* temp;
    
    if (mCan_recv_queue_item_count > 0)
    {
        temp = &mCan_recv_queue[mCan_recv_queue_out_count];
        mCan_recv_queue_out_count = (mCan_recv_queue_out_count + 1) % CAN_RECV_QUEUE_SIZE;
        --mCan_recv_queue_item_count;
        return temp;
    }
    return 0;
}
