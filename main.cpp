#include "main.h"

#include "leds.h"
#include "watchdog.h"
#include "debug.h"

extern "C" {
#include "CO_driver.h"
#include "CO_CAN.h"
}


/* Global variables and objects */

Watchdog wdog;

volatile uint16_t   CO_timer1ms = 0U;   /* variable increments each millisecond */

Timer t;

extern "C" void mbed_reset();


int main (void){
    HBled = 1;
    CANrunLed = 1;
    CANerrLed = 1;
    ERRled = 1;

    Thread tmr_thread;
    Thread app_thread;

    wdog.kick(10); // First watchdog kick to trigger it

    USBport.baud(115200);

    tmr_thread.start(tmrTask_thread);
    app_thread.start(appTask_thread);

    CO_NMT_reset_cmd_t reset = CO_RESET_NOT;

    while(reset != CO_RESET_APP){
        /* CANopen communication reset - initialize CANopen objects *******************/
        CO_ReturnError_t err;
        uint16_t timer1msPrevious;

        wdog.kick(); // Kick the watchdog

        /* initialize CANopen */
        err = CO_init(
                (int32_t)0,             /* CAN module address */
                (uint8_t)NODEID,        /* NodeID */
                (uint16_t)BUS_BITRATE); /* bit rate */

        if(err != CO_ERROR_NO){
            ERRled = 1;

            while(1);
            /* CO_errorReport(CO->em, CO_EM_MEMORY_ALLOCATION_ERROR, CO_EMC_SOFTWARE_INTERNAL, err); */
        }

        CO_CANsetNormalMode(CO->CANmodule[0]);

        /* Configure CAN transmit and receive interrupt */

        CANport->attach(&_CANInterruptHandler, CAN::RxIrq);
        CANport->attach(&_CANInterruptHandler, CAN::TxIrq);

        /* start CAN */

        HBled = 0;
        ERRled = 0;
        CANrunLed = 0;
        CANerrLed = 0;

        reset = CO_RESET_NOT;
        timer1msPrevious = CO_timer1ms;

        wdog.kick(5); // Change watchdog timeout

        while(reset == CO_RESET_NOT) {
            /* loop for normal program execution */

            wdog.kick(); // Kick the watchdog

            uint16_t timer1msCopy, timer1msDiff;
            timer1msCopy = CO_timer1ms;
            timer1msDiff = timer1msCopy - timer1msPrevious;
            timer1msPrevious = timer1msCopy;

            /* CANopen process */
            reset = CO_process(CO, timer1msDiff, NULL);

            /* Nonblocking application code may go here. */

            programAysnc(timer1msDiff);

            Thread::wait(1);
        }
    }


    /* program exit */
    /* stop threads */
    HBled = 1;
    CANrunLed = 1;
    CANerrLed = 1;
    ERRled = 1;

    tmr_thread.terminate();
    app_thread.terminate();

    /* delete objects from memory */
    CO_delete(0/* CAN module address */);

    /* reset */
    mbed_reset();
    return 1;
}


/* timer thread executes in constant intervals */
void tmrTask_thread(){
    while(true) {

        /* sleep for interval */
        Thread::wait(1);

        INCREMENT_1MS(CO_timer1ms);

        if(CO->CANmodule[0]->CANnormal) {
            bool_t syncWas;

            /* Process Sync and read inputs */
            syncWas = CO_process_SYNC_RPDO(CO, TMR_TASK_INTERVAL);

            /* Further I/O or nonblocking application code may go here. */


            /* Write outputs */
            CO_process_TPDO(CO, syncWas, TMR_TASK_INTERVAL);

            /* verify timer overflow */
            if(0) {
                CO_errorReport(CO->em, CO_EM_ISR_TIMER_OVERFLOW, CO_EMC_SOFTWARE_INTERNAL, 0U);
            }

            if(CO_timer1ms % 50 == 0) {
                HBled = !HBled;
            }
        }
    }
}

void appTask_thread(){
    while(true) {
        Thread::wait(500);
    }
}

/* Led update thread */
void programAysnc(uint16_t timer1msDiff) {
    CANrunLed = LED_GREEN_RUN(CO->NMT) ? true : false;
    CANerrLed = LED_RED_ERROR(CO->NMT) ? true : false;
}

/* CAN interrupt function */
void _CANInterruptHandler(void){
    CO_CANinterrupt(CO->CANmodule[0]);
}
