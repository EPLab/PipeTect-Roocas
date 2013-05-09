#include "switcher.h"
#include "sys_timer.h"
//#include <p18cxxx.h>
//#include "vector_mapper.h"
#include "sys_isr.h"

//#define NULL 0

typedef struct task{
	unsigned char taskUID;
	unsigned char next;
	unsigned short period;
	unsigned short count;
	char ttl;
	unsigned short sleepCount;
	unsigned char flag;
	void (*pf_event)(unsigned char tid, void* msg);
} task_t;

typedef struct msgItem{
	unsigned char tid;
	void* msg;
} msgItem_t;

typedef struct msgQueue{
	msgItem_t msgs[MAX_NUM_TASKS];
	unsigned char numItems;
	unsigned char checkIn;
	unsigned char checkOut;
} msgQueue_t;

volatile unsigned char numRegisteredTasks;
volatile unsigned char headTask;
unsigned char currentTask;
task_t mTask[MAX_NUM_TASKS] = {0, NULL};
volatile unsigned short mTick;
msgQueue_t mMsgQueue;
volatile unsigned char swTick;
volatile unsigned long timeStamp = 0;

void timer0_tick(void);
static msgItem_t* getTaskMsg(unsigned char tid);
static unsigned char getTid(unsigned char tuid);

void initSwitcher(unsigned int tick)
{
	headTask = 0xFF;
	numRegisteredTasks = 0;
	currentTask = 0;
	//INTCON = 0;
	//mTick = PORTB;
	mTick = tick;
	mMsgQueue.numItems = 0;
	mMsgQueue.checkIn = 0;
	mMsgQueue.checkOut = 0;
	if (mTick)
	{
		ISR_SetTimer_A1_WB(&timer0_tick);
		//OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_8);
		//WriteTimer0(mTick);
	        //SYS_SetTimer(TA1_CONF1, tick);
		//INTCON2bits.TMR0IP = 1;		//Set timer0 priority high
		//INTCONbits.TMR0IE = 1;		//Enable timer0 interrupt
		swTick = 0;
	}
	//RCONbits.IPEN = 1;			//Enable priority on interrupts
	//INTCONbits.PEIE = 1;		//Enable high priority
	//INTCONbits.GIE = 1;			//Enable low priority
}

void timer0_tick(void)
{
	swTick = 1;
	//INTCONbits.TMR0IF = 0; //Reset Timer1 interrupt flag
	//WriteTimer0(mTick);
	//++timeStamp;
}

void pauseTask(unsigned char taskNum)
{
	mTask[taskNum].flag &= 0xFE;
}

void resumeTask(unsigned char taskNum)
{
	mTask[taskNum].flag |= 1;
}		

static unsigned char findSlot(void)
{
	unsigned char i;
	
	if (numRegisteredTasks < MAX_NUM_TASKS)
	{
		for (i = 0; i < MAX_NUM_TASKS; ++i)
		{
			if (mTask[i].pf_event == NULL)
			{
				return i;
			}
		}			
	}
	return 0xFF;
}		

// 
unsigned char registerTask(void (*fp)(unsigned char tid, void* msg), 
						   unsigned char tuid, unsigned short period, char ttl)
{
	unsigned char temp;
	
	temp = findSlot();
	if (temp != 0xFF)
	{
		if (temp == 0)
		{
			if (numRegisteredTasks == 0)
			{
				mTask[0].next = 0xFF;
			}
			else
			{
				mTask[0].next = headTask;
			}	
			headTask = 0;
		}
		else
		{
			if (numRegisteredTasks == (MAX_NUM_TASKS - 1))
			{
				mTask[temp].next = 0xFF;
			}
			else
			{	
				mTask[temp].next = mTask[temp - 1].next;
				mTask[temp - 1].next = temp;
			}				
		}
		mTask[temp].pf_event = fp;
		mTask[temp].taskUID = tuid;
		mTask[temp].period = period;
		mTask[temp].count = period;
		mTask[temp].ttl = ttl;
		mTask[temp].sleepCount = 0;
		mTask[temp].flag = 1;
		++numRegisteredTasks;
		return temp;
	}
	return 0xFF;
}

unsigned char unregisterTask(unsigned char taskNum)
{
	signed short i;

	if (numRegisteredTasks > 0)
	{
		if (numRegisteredTasks == 1)
		{
			headTask = 0xFF;
		}
		else
		{
			if (taskNum == 0)
			{
				headTask = mTask[taskNum].next;
			}
			else
			{
				for (i = (signed short)(taskNum - 1); i >= 0; --i)
				{
					if (mTask[i].pf_event)
					{
						mTask[i].next = mTask[taskNum].next;
						break;
					}
				}
			}
		}
		if (mTask[taskNum].next == 0xFF)
		{
			currentTask = 0;
		}
		mTask[taskNum].pf_event = NULL;
		--numRegisteredTasks;
		return 1;		
	}
	return 0;
}

void swPutAsleep(unsigned char taskNum, unsigned short count)
{
	mTask[taskNum].sleepCount = count;
}

void swWakeUp(unsigned char taskNum)
{
	mTask[taskNum].sleepCount = 0;
}

unsigned char swIsProcSleeping(unsigned char taskNum)
{
	if (mTask[taskNum].sleepCount)
	{
		return 1;
	}
	return 0;
}
		
void Switcher(void)
{
	msgItem_t* temp;
	void* msg = NULL;
	
	for(;;)
	{
	  	WDTCTL = WDTPW + WDTSSEL__ACLK +WDTIS__512K;
		if (numRegisteredTasks == 0)	//no registered task
		{
			continue;
		}

		if ((mTask[currentTask].ttl) && 
		    ((mTask[currentTask].count == 0) && 
		    (mTask[currentTask].sleepCount == 0)))
		{
			temp = getTaskMsg(currentTask);
			if (temp)
			{
				if (currentTask == temp->tid)
				{
					msg = temp->msg;
				}
			}
					
			if (mTask[currentTask].flag & 0x01)
			{
				mTask[currentTask].pf_event(currentTask, msg);
			}
				
			if (mTask[currentTask].ttl == 0)
			{
				unregisterTask(currentTask);
			}
			else
			{	
				if (mTask[currentTask].ttl != 0xFF)
				{
					mTask[currentTask].ttl -= 1;
				}
			}
			mTask[currentTask].count = mTask[currentTask].period;			
		}
		else
		{
			if (swTick)
			{
                if (mTask[currentTask].sleepCount > 0)
                {
					mTask[currentTask].sleepCount -= 1;
                }
                if (mTask[currentTask].count > 0)
                {
                    mTask[currentTask].count -= 1;
                }
			    swTick = 0;
			}
		}	
		if (mTask[currentTask].next == 0xFF)
		{
			currentTask = headTask;
		}
		else
		{
			currentTask = mTask[currentTask].next;
		}
		//ClrWdt();			
		WDTCTL = WDTPW + WDTSSEL__ACLK +WDTIS__512K + WDTCNTCL;
	}
}

msgItem_t* getTaskMsg(unsigned char tid)
{
	msgItem_t* tempMsgItem = NULL;
	
	if (mMsgQueue.numItems)
	{
		tempMsgItem = &mMsgQueue.msgs[mMsgQueue.checkOut];
		if (tempMsgItem->tid == tid)
		{
			--mMsgQueue.numItems;
			++mMsgQueue.checkOut;
		}	
		return tempMsgItem;
	}
	return NULL;		
}

unsigned char sendTaskMsg(unsigned char tuid, void* msg)
{
	msgItem_t* tempMsgItem;
	unsigned char tempId = getTid(tuid);
	
	if (tempId != 0xFF)
	{
		if (mMsgQueue.numItems < MAX_NUM_MSGS)
		{
			tempMsgItem = &mMsgQueue.msgs[mMsgQueue.checkIn];
			tempMsgItem->tid = tempId;
			tempMsgItem->msg = msg;
			++mMsgQueue.numItems;
			++mMsgQueue.checkIn;
			if (mMsgQueue.checkIn == MAX_NUM_MSGS)
			{
				mMsgQueue.checkIn = 0;
			}	
			return 1;
		}
	}	
	return 0;
}
		
unsigned char getTid(unsigned char tuid)
{
	unsigned char i;
	
	if (tuid)
	{
		for (i = 0; i < MAX_NUM_TASKS; ++i)
		{
			if (mTask[i].taskUID == tuid)
			{
				return i;
			}
		}
	}	
	return 0xFF;
}

unsigned long getTimeStamp(void)
{
	return timeStamp;
}
	