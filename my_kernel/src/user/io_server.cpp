#include "io_server.h"
#include "name_server.h"
#include "../kern/syscall.h"
#include "../shared_constants.h"

IOServer::IOServer()
{
    // potentially create separate IO Servers for marklin/console
    REGISTERAS("IOServer");
    run();
}

IOServer::~IOServer()
{
}

void IOServer::run()
{
    int sender_tid;
    int channel;
    unsigned char ch;
    while (true)
    {
        RECEIVE(&sender_tid, (char *)&channel, sizeof(channel));
        // switch (channel)
        // {
        // case COM1:
        //     ch = uart_getc(COM1);
        //     REPLY(sender_tid, (char *)&ch, sizeof(ch));
        //     break;
        // case COM2:
        //     ch = uart_getc(COM2);
        //     REPLY(sender_tid, (char *)&ch, sizeof(ch));
        //     break;
        // case COM3:
        //     ch = uart_getc(COM3);
        //     REPLY(sender_tid, (char *)&ch, sizeof(ch));
        //     break;
        // case COM4:
        //     ch = uart_getc(COM4);
        //     REPLY(sender_tid, (char *)&ch, sizeof(ch));
        //     break;
        // default:
        //     uart_printf(COM2, "Invalid channel %d\n", channel);
        //     break;
        // }
    }
}

void IOServer::notifyRX()
{
    // check for status
    // action or
    // wait for interrupt
    AWAITEVENT(InterruptEvents::UART_RX);
    // repeat
}

void IOServer::notifyTX()
{
    // check for status
    // action or
    // wait for interrupt
    AWAITEVENT(InterruptEvents::UART_TX);
    // repeat
}