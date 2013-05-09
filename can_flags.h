#ifndef __CAN_FLAGS_H__
#define __CAN_FALGS_H__

typedef enum{
  CAN_ACK = 0,
  CAN_NORMAL_PACKET,
  CAN_SPECIAL_PACKET,
  CAN_INBINDER_STATUS_REQUEST,
  CAN_OUTBINDER_STATUS_REQUEST,
  CAN_CLEAR_ALARM,
} canFlag_t;

#define CAN_SUB_NONE 0
#define CAN_SUB_OK  1
#define CAN_SUB_CHKSUM_ERROR 2
#define CAN_SUB_TEST 3

#endif