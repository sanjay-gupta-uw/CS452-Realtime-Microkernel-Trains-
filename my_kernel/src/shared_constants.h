#ifndef _shared_constants_h_
#define _shared_constants_h_

typedef enum
{
    TIMER_TICK,
    UART_RX_TIMEOUT,
    UART_TX,
    UART_MARKLIN_RX,
    UART_MARKLIN_TX,
    UART_MARKLIN_CTS_HIGH,
    UART_MARKLIN_CTS_LOW,
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
    SVC_GETIDLE,
    SVC_ABORT
} Syscall;

#define SWITCH_COUNT 22
// assume terminal is 80x24
#define NUM_ROWS 24
#define NUM_COLS 132

#define SCROLL_ROW_START 1
#define SCROLL_ROW_END 30

#define CLOCK_LENGTH 8
#define CLOCK_LOCATION_X (NUM_COLS - CLOCK_LENGTH) / 2
#define CLOCK_LOCATION_Y SCROLL_ROW_END + 1

#define IDLE_LOCATION CLOCK_LOCATION_Y

#define SWITCH_LOCATION IDLE_LOCATION + 1

#define CMD_LOCATION SWITCH_LOCATION + 26
#define CMD_STATUS_LOCATION CMD_LOCATION + 1

#define SENSOR_LOCATION SWITCH_LOCATION + 25

// need to deprecate
#define TRANSMIT_LOCATION SENSOR_LOCATION + 20
#define START_COLUMN 88

#endif
