#include "../include/io_server.h"
#include "../include/name_server.h"
#include "../../include/syscall.h"
#include "../../shared_constants.h"
#include <cstdint>
#include "../../rpi.h"
#include "../include/uassert.h"

#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif

namespace IO_SERVER
{
#define MMIO_BASE 0xFE000000
    static char *const UART0_BASE = (char *)(MMIO_BASE + 0x201000);
    static char *const UART3_BASE = (char *)(MMIO_BASE + 0x201600);

#define UART_FR_TXFE_MASK 0x80
#define UART_FR_RXTM_MASK 0x40
#define UART_FR 0x18
#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))
    static char *const line_uarts[] = {NULL, UART0_BASE, UART3_BASE};

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
        tx_state = TX_DEASSERTED;
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
        uassert(rtm_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting RTM Notifier");

        tx_notifier_tid = CREATE(PRIORITY::P2, notifier_tx);
        uassert(tx_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting TX Notifier");
    }

    void IOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            uassert(retval >= 0 && "IO_SERVER::run: PANIC, Error receiving message");

            IO_REPLY reply;
            reply.type = REPLY_TYPE::UNIMPLEMENTED;
            reply.ch = UNDEFINED_CHAR;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::GETC:
            {
                if (!receive_buffer.IsEmpty())
                {
                    // need to reply with CH
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    GETC_SUCCESS_REPLY(sender_tid, ch);
                }
                else
                {
                    rx_waiting_tasks.Push(sender_tid);
                }
            }
            break;
            case IO_REQUEST_TYPE::PUTC:
            {
                // reply.type = REPLY_TYPE::FAILURE;  // incase it's full
                // if (!transmit_buffer_str.IsFull()) // this will ignore characters if "overwhelmed"
                // {
                //     transmit_buffer_str.Push((unsigned char *)(req.ch));
                //     tx_waiting_tasks.Push(sender_tid);
                //     reply.type = REPLY_TYPE::SUCCESS;
                //     // count++;
                // }

                // if (tx_state == TX_ASSERTED)
                // {
                //     write_to_uart();
                // }
                // // ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::PUTS:
            {
                unsigned char *str = req.str;
                reply.type = REPLY_TYPE::FAILURE; // incase it's full
                if (!transmit_buffer_str.IsFull())
                {
                    transmit_buffer_str.Push(str);
                    tx_waiting_tasks.Push(sender_tid);
                    reply.type = REPLY_TYPE::SUCCESS;
                }

                if (tx_state == TX_ASSERTED)
                {
                    write_to_uart();
                }
                // ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::RTM_NOTIFIER:
            {
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
                    receive_buffer.Pop(&ch);
                    GETC_SUCCESS_REPLY(waiting_task, ch);
                }

                ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                write_to_uart();
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
            while (!transmit_buffer_str.IsEmpty())
            {
                unsigned char *str = nullptr;
                transmit_buffer_str.Pop(&str);

                while (*str)
                {
                    uart_putc(CONSOLE, *(str++));
                }
                int sender_tid;
                tx_waiting_tasks.Pop(&sender_tid);
                ReplyWithMessage(sender_tid, (IO_REPLY){REPLY_TYPE::SUCCESS, 0});
            }
            tx_state = TX_DEASSERTED;
            // wake up tx_notifier
            IO_REPLY reply;
            reply.type = REPLY_TYPE::SUCCESS;
            REPLY(tx_notifier_tid, (char *)&reply, sizeof(reply));
        }
    }

    int Getc(int tid)
    {
        uassert(tid == IO_SERVER_TID && "IO_SERVER::Getc: PANIC, Invalid tid");

#if IRQ_ENABLED == 0
        char ch = uart_getc(CONSOLE);
        return ch;
#endif

        IO_REQUEST req{IO_REQUEST_TYPE::GETC, 0, nullptr};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Getc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? reply.ch : -1;
    }

    int Putc(int tid, unsigned char ch)
    {
        uassert(tid == IO_SERVER_TID && "IO_SERVER::Putc: PANIC, Invalid tid");

        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, ch, nullptr};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Putc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    int Puts(int tid, unsigned char *str)
    {
        uassert(tid == IO_SERVER_TID && "IO_SERVER::Puts: PANIC, Invalid tid");

        IO_REQUEST req{IO_REQUEST_TYPE::PUTS, '\0', str};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Puts: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    // notify that user has entered a character
    // this works because flag is set based on receive buffer, which triggers interrupt
    void notifier_rxto() // receive timeout
    {
        while (true)
        {
            if (UART_REG(CONSOLE, UART_FR) & UART_FR_RXTM_MASK)
            {
                IO_REQUEST req{IO_REQUEST_TYPE::RTM_NOTIFIER, 0, nullptr};
                IO_REPLY reply;

                SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::notifier_rxto: PANIC, Unimplemented");
            }
            AWAITEVENT(InterruptEvents::UART_RX_TIMEOUT);
        }
    }

    void notifier_tx()
    {

        while (true)
        {
            if (UART_REG(CONSOLE, UART_FR) & UART_FR_TXFE_MASK)
            {
                tx_state = TX_ASSERTED;
                IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0, nullptr};
                IO_REPLY reply;

                SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::notifier_tx: PANIC, Unimplemented");
            }

            // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
            AWAITEVENT(InterruptEvents::UART_TX);
        }
    }

    void startIOServer()
    {
        IO_SERVER::IOServer ioServer;
    }
}