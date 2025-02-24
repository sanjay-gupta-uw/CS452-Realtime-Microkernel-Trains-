#include "../include/io_server.h"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"

#include "../../rpi.h"

namespace IO_SERVER
{
    int IO_SERVER_TID = -1;

    static void ReplyWithMessage(int tid, REPLY_TYPE reply)
    {
        REPLY(tid, (char *)&reply, sizeof(reply));
    }

    IOServer::IOServer()
    {
        // potentially create separate IO Servers for marklin/console
        REGISTERAS("IOServer");
        IO_SERVER_TID = MYTID();
        spawnNotifiers();
        run();
    }

    IOServer::~IOServer()
    {
    }

    void IOServer::spawnNotifiers()
    {
        rx_notifier_tid = CREATE(PRIORITY::P1, notifierRX);
        if (rx_notifier_tid < 0)
        {
            uart_printf(CONSOLE, "Error starting RX Notifier\n");
        }

        // tx_notifier_tid = CREATE(PRIORITY::P1, notifierTX);
        // if (tx_notifier_tid < 0)
        // {
        //     uart_printf(CONSOLE, "Error starting TX Notifier\n");
        // }
    }

    void IOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            if (retval < 0)
            {
                uart_printf(CONSOLE, "PANIC, IO_SERVER RECEIVE returned error %d\n", retval);
            }
            uart_printf(CONSOLE, "IO_SERVER::Received request from %d\r\n", sender_tid);

            switch (req.type)
            {
            case IO_REQUEST_TYPE::GETC:
            {
                uart_printf(CONSOLE, "IO_SERVER::Getc\r\n");
                ReplyWithMessage(sender_tid, REPLY_TYPE::UNIMPLEMENTED);
                // unsigned char ch = uart_getc(CONSOLE);
                // uart_printf(CONSOLE, "Received char %c\n", ch);

                // need to get char from fifo

                break;
            }
            case IO_REQUEST_TYPE::PUTC:
            {

                uart_printf(CONSOLE, "IO_SERVER::Putc\r\n");
                ReplyWithMessage(sender_tid, REPLY_TYPE::UNIMPLEMENTED);
                // need to check CLS flag and state machine
                uart_putc(CONSOLE, req.ch);
                uart_printf(CONSOLE, "\r\n");
                // REPLY(sender_tid, (char *)&retval, sizeof(retval));
                break;
            }

            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
                uart_printf(CONSOLE, "IO_SERVER::RX_NOTIFIER\r\n");
                //
                break;
            }

            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                uart_printf(CONSOLE, "IO_SERVER::TX_NOTIFIER\r\n");
                //
                break;
            }

            case IO_REQUEST_TYPE::CTS_NOTIFIER:
            {
                uart_printf(CONSOLE, "IO_SERVER::CTS_NOTIFIER\r\n");
                //
                break;
            }

            default:
                break;
            }
        }
    }

    int Getc(int tid, int channel)
    {
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::GETC, channel, 0};
        int ch;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&ch, sizeof(ch));
        uart_printf(CONSOLE, "IO_SERVER::Getc returned %s\r\n", REPLY_TYPE_STR((REPLY_TYPE)ch));

        return ch;
    }

    int Putc(int tid, int channel, unsigned char ch)
    {
        uart_printf(CONSOLE, "IO_SERVER::Putc\r\n");
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, channel, ch};
        int reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uart_printf(CONSOLE, "IO_SERVER::Putc returned %s\r\n", REPLY_TYPE_STR((REPLY_TYPE)reply));

        return reply;
    }

    // notify that user has entered a character
    // this works because flag is set based on receive buffer, which triggers interrupt
    void notifierRX()
    {
        uart_printf(CONSOLE, "RX Notifier started\r\n");
        // check for status
        // action or
        // wait for interrupt
        // repeat
        while (true)
        {
            int ch = uart_getc_non_blocking(CONSOLE);
            if (ch != NO_CHAR)
            {
                uart_printf(CONSOLE, "Received char %c\n", ch);
            }
            // this will be triggered when user types in command line since it will add to the receive buffer
            AWAITEVENT(InterruptEvents::UART_RX_TIMEOUT);
            IO_REQUEST req{IO_REQUEST_TYPE::RX_NOTIFIER, CONSOLE, 0};

            int reply;
            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }

    void notifierTX()
    {
        // check for status
        // action or
        // wait for interrupt
        // repeat
        while (true)
        {
            AWAITEVENT(InterruptEvents::UART_TX);
            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, CONSOLE, 0};
            int reply;
            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));

            if (reply < 0)
            {
                uart_printf(CONSOLE, "PANIC, TX_NOTIFIER returned error %d\n", reply);
            }
        }
    }
}

void startIOServer()
{
    IO_SERVER::IOServer ioServer;
}