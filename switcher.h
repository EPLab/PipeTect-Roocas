#ifndef __SWITCHER_H
#define __SWITCHER_H

#define MAX_NUM_TASKS 8
#define MAX_NUM_MSGS 4

typedef struct swMsg{
	unsigned char id;
	unsigned char* data;
	unsigned char length;
} swMsg_t;

extern void initSwitcher(unsigned int tick);
extern unsigned char registerTask(void (*fp)(unsigned char tid, void* msg), unsigned char tuid, unsigned short period, char ttl);
extern unsigned char unregisterTask(unsigned char taskNum);
extern unsigned char sendTaskMsg(unsigned char tuid, void* msg);
extern void swPutAsleep(unsigned char taskNum, unsigned short count);
extern unsigned char swIsProcSleeping(unsigned char taskNum);
extern void swWakeUp(unsigned char taskNum);
extern void Switcher(void);
extern void pauseTask(unsigned char taskNum);
extern void resumeTask(unsigned char taskNum);

#endif
