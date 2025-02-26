#ifndef _shared_constants_h_
#define _shared_constants_h_

typedef enum
{
    TIMER_TICK,
    UART_RX_TIMEOUT,
    UART_RX,
    UART_TX,
    UART_CTS,
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

// void color_black() { uart_puts(CONSOLE, "\033[30m"); }
// void color_red() { uart_puts(CONSOLE, "\033[31m"); }
// void color_green() { uart_puts(CONSOLE, "\033[32m"); }
// void color_yellow() { uart_puts(CONSOLE, "\033[33m"); }
// void color_blue() { uart_puts(CONSOLE, "\033[34m"); }
// void color_magenta() { uart_puts(CONSOLE, "\033[35m"); }
// void color_cyan() { uart_puts(CONSOLE, "\033[36m"); }
// void color_white() { uart_puts(CONSOLE, "\033[37m"); }
// save colours as strings

#endif
