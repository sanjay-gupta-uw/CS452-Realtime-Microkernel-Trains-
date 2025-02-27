#include "../include/marklin_io.h"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"

#include "../../rpi.h"

namespace MARKLIN_IO_SERVER
{
    static int IO_SERVER_TID;
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

    MarklinIOServer::MarklinIOServer()
    {
        cts_state = PIN_STATE::HIGH;
        // tx_state = PIN_STATE::HIGH;
        tx_state_machine = STATE_MACHINE::MARKLIN_READY;
        // potentially create separate IO Servers for marklin/console
        REGISTERAS("MarklinIOServer");
        IO_SERVER_TID = MYTID();
        // tx_state = TX_LOW;
        count = 0;
        spawnNotifiers();
        run();
    }

    MarklinIOServer::~MarklinIOServer()
    {
    }

    void MarklinIOServer::spawnNotifiers()
    {
        // ensure this runs before tx_notifier
        // rx_notifier_tid = CREATE(PRIORITY::P2, notifier_rx);
        // if (rx_notifier_tid < 0)
        // {
        //     // uart_printf(CONSOLE, "Error starting RTM Notifier\n");
        // }

        tx_notifier_tid = CREATE(PRIORITY::P2, notifier_tx);
        if (tx_notifier_tid < 0)
        {
            // uart_printf(CONSOLE, "Error starting TX Notifier\n");
        }
        // pending_tramissions.Push(tx_notifier_tid); // initial transmission

        cts_notifier_tid = CREATE(PRIORITY::P2, notifier_cts);
        if (cts_notifier_tid < 0)
        {
            // uart_printf(CONSOLE, "Error starting CTS Notifier\n");
        }
    }

    void MarklinIOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            // // uart_printf(CONSOLE, "MarklinIO_server::Waiting for request\r\n");
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            // // uart_printf(CONSOLE, "MarklinIO_server::Received request from %d\r\n", sender_tid);
            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, IO_SERVER RECEIVE returned error %d\n", retval);
            }
            // // uart_printf(CONSOLE, "MarklinIO_server::Received request from %d\r\n", sender_tid);

            IO_REPLY reply;
            reply.type = REPLY_TYPE::UNIMPLEMENTED;
            reply.ch = UNDEFINED_CHAR;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::GETC:
            {
                // // uart_printf(CONSOLE, "MarklinIO_server::GETC\r\n");
                if (!receive_buffer.IsEmpty())
                {
                    // need to reply with CH
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    GETC_SUCCESS_REPLY(sender_tid, ch);
                }
                else
                {
                    // // uart_printf(CONSOLE, "MarklinIO_server::GETC: {tid: %d} waiting for character\r\n", sender_tid);
                    rx_waiting_tasks.Push(sender_tid);
                }
            }
            break;
            case IO_REQUEST_TYPE::PUTC:
            {
                reply.type = REPLY_TYPE::FAILURE; // incase it's full
                if (!transmit_buffer.IsFull())    // this will ignore characters if "overwhelmed"
                {
                    transmit_buffer.Push((unsigned char)(req.data));
                    reply.type = REPLY_TYPE::SUCCESS;
                    count++;
                }

                // // uart_printf(CONSOLE, "MarklinIO_server::PUTC: %c\r\n", req.data);
                // // uart_printf(CONSOLE, "CURRENT STATE: %d\r\n", tx_state_machine);
                if (tx_state_machine == STATE_MACHINE::MARKLIN_READY)
                {
                    handle_transmission();
                }
                // else
                // {
                //     // // uart_printf(CONSOLE, "MarklinIO_server::PUTC: PANIC, Transmit buffer is full\r\n");
                // }

                ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            // this should fire when byte received since no FIFO
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
            }
            break;

            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                // // uart_printf(CONSOLE, "MarklinIO_server::TX_NOTIFIER\r\n");
                pending_tramissions.Push(sender_tid);
                handle_transmission();

                reply.type = REPLY_TYPE::SUCCESS;
                ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            case IO_REQUEST_TYPE::CTS_NOTIFIER:
            {
                // uart_printf(CONSOLE, "MarklinIO_server::CTS_NOTIFIER\r\n");
                cts_state = (PIN_STATE)(req.data);
                handle_transmission();
                reply.type = REPLY_TYPE::SUCCESS;
                ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            default:
                break;
            }
        }
    }

    void MarklinIOServer::handle_transmission()
    {
        // // uart_printf(CONSOLE, "HANDLE_TRANSMISSION\r\n");
        if (cts_state == PIN_STATE::HIGH)
        {
            // // uart_printf(CONSOLE, "CTS HIGH\r\n");
            if (tx_state_machine == STATE_MACHINE::MARKLIN_PROCESSING)
            {
                tx_state_machine = STATE_MACHINE::MARKLIN_READY;
            }

            if (!transmit_buffer.IsEmpty() && !pending_tramissions.IsEmpty())
            {
                int tid;
                pending_tramissions.Pop(&tid);
                tx_state_machine = STATE_MACHINE::TRANSMITTING;
                write_to_uart(); // write single byte from transmit buffer
            }
            else
            {
                // // uart_printf(CONSOLE, "CTS HIGH, NO TRANSMISSION\r\n");
                tx_state_machine = STATE_MACHINE::MARKLIN_READY;
            }
        }
        // cts must be low
        else if (tx_state_machine == STATE_MACHINE::TRANSMITTING)
        {
            // // uart_printf(CONSOLE, "CTS LOW, IS CURRENTLY TRANSMITTING?\r\n");
            tx_state_machine = STATE_MACHINE::MARKLIN_PROCESSING;
        }
        else
        {
            // // uart_printf(CONSOLE, "CTS LOW, NOT CURRENTLY TRANSMITTING\r\n");
        }
    }

    // can only send byte
    void MarklinIOServer::write_to_uart()
    {
        // // uart_printf(CONSOLE, "MarklinIO_server::write_to_uart\r\n");
        unsigned char ch = UNDEFINED_CHAR;
        transmit_buffer.Pop(&ch);
        // uart_putc(MARKLIN, (uint8_t)data);
        uart_putc(MARKLIN, ch);
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
        // // uart_printf(CONSOLE, "MarklinIO_server::Putc\r\n");
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, ch, nullptr};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // // uart_printf(CONSOLE, "MarklinIO_server::Putc returned %s\r\n", REPLY_TYPE_STR((REPLY_TYPE)reply));

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    int SendCmd(int tid, MarklinRequest *request)
    {
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::SEND_CMD, 0, request};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    void notifier_rx() // receive
    {
        // uart_printf(CONSOLE, "M-RX Notifier started\r\n");
        while (true)
        {
            // // uart_printf(CONSOLE, "ENABLING RTM INTERRUPT\r\n");
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_RX);
            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, MARKLIN RX_NOTIFIER AWAITEVENT returned error %d\n", retval);
                spin_debug();
            }
            // // uart_printf(CONSOLE, "RTM NOTIFIER AWOKEN\r\n");

            // don't need to pass marklin in these
            IO_REQUEST req{IO_REQUEST_TYPE::RX_NOTIFIER, 0};
            int reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }

    void notifier_tx()
    {
        // uart_printf(CONSOLE, "M-TX Notifier started\r\n");

        while (true)
        {
            // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_TX);

            // tx_state = TX_HIGH;

            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, TX_NOTIFIER AWAITEVENT returned error %d\n", retval);
                spin_debug();
            }
            // uart_printf(CONSOLE, "TX NOTIFIER AWOKEN\r\n");
            // spin_debug();

            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0};
            int reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }

    void notifier_cts()
    {
        // THIS WILL FIRE WHEN ASSERTED/DEASSERTED
        // uart_printf(CONSOLE, "M-CTS Notifier started\r\n");

        while (true)
        {
            // FOR THIS EVENT, CTS ASSERTED/DEASSERTED returned (TODO)
            // should be 0 if low, 1 if high (translated in kernel)
            int CTS_SIGNAL = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS);
            // uart_printf(CONSOLE, "CTS NOTIFIER AWOKEN, CTS: %d\r\n", CTS_SIGNAL);

            if (CTS_SIGNAL < 0)
            {
                // uart_printf(CONSOLE, "PANIC, CTS_NOTIFIER AWAITEVENT returned error %d\n", CTS_SIGNAL);
                spin_debug();
            }

            IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER, CTS_SIGNAL};
            int reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            // uart_printf(CONSOLE, "CTS NOTIFIER UNBLOCKED\r\n");
        }
    }

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }
}