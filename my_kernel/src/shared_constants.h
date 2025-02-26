#ifndef _shared_constants_h_
#define _shared_constants_h_

typedef enum
{
    UART_RX_TIMEOUT,
    UART_RX,
    UART_TX,
    UART_CTS,
    TIMER_TICK,
    NUM_EVENTS
} InterruptEvents;

// priority levels
typedef enum
{
    P0,
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    IDLE
} PRIORITY;

typedef enum
{
    SVC_CREATE,
    SVC_MYTID,
    SVC_MYPARENTID,
    SVC_YIELD,
    SVC_EXIT,
    SVC_SEND,
    SVC_RECEIVE,
    SVC_REPLY,
    SVC_ICACHE,
    SVC_DCACHE,
    SVC_BCACHE,
    SVC_AWAITEVENT,
    IRQ,
} Syscall;

#endif