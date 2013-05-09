#ifndef __CAN_RECV_QUEUE_H
#define __CAN_RECV_QUEUE_H

#define CAN_RECV_QUEUE_SIZE 10

#include "define.h"

typedef struct can_recv_queue_item
{
    BYTE std_id;
    BYTE cmd;
    WORD ext_id;
    BYTE len;
    BYTE data[8];
} can_recv_queue_item_t;

extern can_recv_queue_item_t* can_recv_enqueue(void);
extern can_recv_queue_item_t* can_recv_dequeue(void);

#endif
