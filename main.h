#ifndef __MAIN_H__
#define __MAIN_H__

#include "mbed.h"
#include "rtos.h"


extern "C" {
#include "CANopen.h"
}

#define TMR_TASK_INTERVAL   (1000)          /* Interval of tmrTask thread in microseconds */
#define INCREMENT_1MS(var)  (var++)         /* Increment 1ms variable in tmrTask */

#define BUS_BITRATE     (500)
#define NODEID          (0x04)

void tmrTask_thread();

void appTask_thread();

void programAysnc(uint16_t timer1msDiff);

void _CANInterruptHandler(void);

#endif /* MAIN_H_ */
