#include "../include/io_server.h"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"

#include "../../rpi.h"

namespace IO_SERVER
{
    static int IO_SERVER_TID;
    static int tx_state;
    static int count;

    static void ReplyWithMessage(int tid, IO_REPLY reply)
    {
        REPLY(tid, (char *)&reply, sizeof(reply));
    }

    static void GETC_SUCCESS_REPLY(int tid, char ch)
    {
        IO_REPLY reply;
        reply.type = REPLY_TYPE::SUCCESS;
        reply.ch = ch;
        REPLY(tid, (char *)&reply, sizeof(reply));
    }

    IOServer::IOServer()
    {
        // potentially create separate IO Servers for marklin/console
        REGISTERAS("IOServer");
        // spin_debug();
        IO_SERVER_TID = MYTID();
        tx_state = TX_LOW;
        count = 0;
        spawnNotifiers();
        run();
    }

    IOServer::~IOServer()
    {
    }

    void IOServer::spawnNotifiers()
    {
        // ensure this runs before tx_notifier
        rtm_notifier_tid = CREATE(PRIORITY::P1, notifier_rxto);
        if (rtm_notifier_tid < 0)
        {
            // uart_printf(CONSOLE, "Error starting RTM Notifier\n");
        }

        tx_notifier_tid = CREATE(PRIORITY::P2, notifier_tx);
        if (tx_notifier_tid < 0)
        {
            // uart_printf(CONSOLE, "Error starting TX Notifier\n");
        }
    }

    void IOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            // // uart_printf(CONSOLE, "IO_SERVER::Waiting for request\r\n");
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            // // uart_printf(CONSOLE, "IO_SERVER::Received request from %d\r\n", sender_tid);
            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, IO_SERVER RECEIVE returned error %d\n", retval);
            }
            // // uart_printf(CONSOLE, "IO_SERVER::Received request from %d\r\n", sender_tid);

            IO_REPLY reply;
            reply.type = REPLY_TYPE::UNIMPLEMENTED;
            reply.ch = UNDEFINED_CHAR;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::GETC:
            {
                // // uart_printf(CONSOLE, "IO_SERVER::GETC\r\n");
                if (!receive_buffer.IsEmpty())
                {
                    // need to reply with CH
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    GETC_SUCCESS_REPLY(sender_tid, ch);
                }
                else
                {
                    // // uart_printf(CONSOLE, "IO_SERVER::GETC: {tid: %d} waiting for character\r\n", sender_tid);
                    rx_waiting_tasks.Push(sender_tid);
                }
            }
            break;
            case IO_REQUEST_TYPE::PUTC:
            {
                uart_printf(CONSOLE, "IO_SERVER::PUTC\r\n");
                reply.type = REPLY_TYPE::FAILURE;  // incase it's full
                if (!transmit_buffer_str.IsFull()) // this will ignore characters if "overwhelmed"
                {
                    transmit_buffer_str.Push((unsigned char *)(req.ch));
                    tx_waiting_tasks.Push(sender_tid);
                    reply.type = REPLY_TYPE::SUCCESS;
                    // count++;
                }

                if (tx_state == TX_HIGH)
                {
                    write_to_uart();
                }
                // ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::PUTS:
            {
                // uart_printf(CONSOLE, "IO_SERVER::PUTS\r\n");
                unsigned char *str = req.str;
                uart_puts(CONSOLE, reinterpret_cast<const char *>(str));
                // spin_debug();
                reply.type = REPLY_TYPE::FAILURE; // incase it's full
                if (!transmit_buffer_str.IsFull())
                {
                    transmit_buffer_str.Push(str);
                    tx_waiting_tasks.Push(sender_tid);
                    reply.type = REPLY_TYPE::SUCCESS;
                }

                if (tx_state == TX_HIGH)
                {
                    write_to_uart();
                }
                // ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::RTM_NOTIFIER:
            {
                // // uart_printf(CONSOLE, "IO_SERVER::RTM_NOTIFIER FIRED, RECEIVING CHAR\r\n");
                // need to get char from fifo
                unsigned char ch = uart_receive_c(CONSOLE);
                if (!receive_buffer.IsFull()) // this will ignore characters if "overwhelmed"
                {
                    receive_buffer.Push(ch);
                    reply.type = REPLY_TYPE::SUCCESS;
                }

                if (!rx_waiting_tasks.IsEmpty())
                {
                    int waiting_task;
                    int ret = rx_waiting_tasks.Pop(&waiting_task);
                    // // uart_printf(CONSOLE, "IO_SERVER::RTM_NOTIFIER: {tid: %d} received character %c\r\n", waiting_task, ch);
                    receive_buffer.Pop(&ch);
                    GETC_SUCCESS_REPLY(waiting_task, ch);
                }

                ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                // // uart_printf(CONSOLE, "IO_SERVER::TX_NOTIFIER\r\n");
                write_to_uart();
                // // uart_printf(CONSOLE, "IO_SERVER::TX_NOTIFIER: Transmit buffer is empty\r\n");
                // reply.type = REPLY_TYPE::SUCCESS;
                // ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            default:
                break;
            }
        }
    }

    void IOServer::write_to_uart()
    {
        if (!transmit_buffer_str.IsEmpty())
        {
            // uart_printf(CONSOLE, "IO_SERVER::write_to_uart\r\n");
            // spin_debug();
            while (!transmit_buffer_str.IsEmpty())
            {
                unsigned char *str = nullptr;
                transmit_buffer_str.Pop(&str);

                // uart_printf(CONSOLE, "IO_SERVER::write_to_uart: TRANSMITTING string %s\r\n", str);

                while (*str)
                {
                    uart_putc(CONSOLE, *(str++));
                }
                int sender_tid;
                tx_waiting_tasks.Pop(&sender_tid);
                ReplyWithMessage(sender_tid, (IO_REPLY){REPLY_TYPE::SUCCESS, 0});
            }
            tx_state = TX_LOW;
            // wake up tx_notifier
            IO_REPLY reply;
            reply.type = REPLY_TYPE::SUCCESS;
            REPLY(tx_notifier_tid, (char *)&reply, sizeof(reply));
        }
    }

    int Getc(int tid)
    {
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::GETC, 0, nullptr};
        IO_REPLY reply;
        // // uart_printf(CONSOLE, "GETC: Sending request to IOServer\r\n");
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // // uart_printf(CONSOLE, "GETC: IOServer finished processing request\r\n");

        return reply.type == REPLY_TYPE::SUCCESS ? reply.ch : -1;
    }

    int Putc(int tid, unsigned char ch)
    {
        // // uart_printf(CONSOLE, "IO_SERVER::Putc\r\n");
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            uart_printf(CONSOLE, "IO_SERVER::Putc: PANIC, Invalid tid %d\r\n", tid);
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, ch, nullptr};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // // uart_printf(CONSOLE, "IO_SERVER::Putc returned %s\r\n", REPLY_TYPE_STR((REPLY_TYPE)reply));

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    int Puts(int tid, unsigned char *str)
    {
        // // uart_printf(CONSOLE, "IO_SERVER::Puts\r\n");
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            uart_printf(CONSOLE, "IO_SERVER::Puts: PANIC, Invalid tid %d\r\n", tid);
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::PUTS, '\0', str};
        IO_REPLY reply;
        // uart_printf(CONSOLE, "IO_SERVER::Puts: Sending request to IOServer\r\n");
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // // uart_printf(CONSOLE, "IO_SERVER::Puts returned %s\r\n", REPLY_TYPE_STR((REPLY_TYPE)reply));

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    // notify that user has entered a character
    // this works because flag is set based on receive buffer, which triggers interrupt
    void notifier_rxto() // receive timeout
    {
        // uart_printf(CONSOLE, "RTM Notifier started\r\n");
        while (true)
        {
            // // uart_printf(CONSOLE, "ENABLING RTM INTERRUPT\r\n");
            int retval = AWAITEVENT(InterruptEvents::UART_RX_TIMEOUT);
            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, RTM_NOTIFIER AWAITEVENT returned error %d\n", retval);
            }
            // // uart_printf(CONSOLE, "RTM NOTIFIER AWOKEN\r\n");

            IO_REQUEST req{IO_REQUEST_TYPE::RTM_NOTIFIER, 0, nullptr};
            int reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }

    void notifier_tx()
    {
        // uart_printf(CONSOLE, "TX Notifier started\r\n");

        while (true)
        {
            // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
            int retval = AWAITEVENT(InterruptEvents::UART_TX);

            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, TX_NOTIFIER AWAITEVENT returned error %d\n", retval);
            }
            tx_state = TX_HIGH;
            // // uart_printf(CONSOLE, "TX NOTIFIER AWOKEN, STALLING TO DEBUG\r\n");
            // for (;;)
            //     ;

            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0, nullptr};
            int reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }

    void startIOServer()
    {
        IO_SERVER::IOServer ioServer;
    }
}