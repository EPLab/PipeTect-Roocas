#ifndef __LOCAL_STORE_H
#define __LOCAL_STORE_H

#define LOS_WRITE_BUFFER_SIZE 4096
#define MAX_FILE_SIZE 1048576UL * 2
#define MAX_NUM_FILES 3

typedef enum{
    SD_OK = 0,
    SD_ERR_INIT,
    SD_ERR_ATTACH
} SD_status_t;

extern void LOS_init(void);

#ifndef __LOCAL_STORE_ACCEL_MODE__
extern int LOS_write(unsigned char* data, int length);
#else
#include "acc.h"
#include "rtc.h"

extern int LOS_accel_write(RTC_TIME* time, ACC_SENSOR* s_data);
#endif

#endif
