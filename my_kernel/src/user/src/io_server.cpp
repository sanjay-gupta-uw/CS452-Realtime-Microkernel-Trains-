#include "../include/io_server.h"
#include "../include/io.h"

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
    static int tx_state;

    static void init_stream()
    {
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::init_stream: Starting\r\n" SAVE_CURSOR);
        uart_putc(CONSOLE, ' ');
        // uassert(false && "IO_SERVER::init_stream: CALLED");
        EXIT();
    }

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
        int my_tid = MYTID();
        // potentially create separate IO Servers for marklin/console
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER(): Registering IOServer: %d\r\n" SAVE_CURSOR, my_tid);
        int retval = REGISTERAS("IOServer");

        tx_state = TX_DEASSERTED;
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER(): Spawning Notifiers\r\n" SAVE_CURSOR);
        spawnNotifiers();
        run();
    }

    IOServer::~IOServer()
    {
    }

    void IOServer::spawnNotifiers()
    {
        rtm_notifier_tid = CREATE(PRIORITY::P1, notifier_rxto);
        uassert(rtm_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting RTM Notifier");

        tx_notifier_tid = CREATE(PRIORITY::P1, notifier_tx);
        uassert(tx_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting TX Notifier");

        // ensure it runs after notifiers
        int init_stream_tid = CREATE(PRIORITY::P2, init_stream);
        uassert(init_stream_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting TX Notifier");
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
                    reply.ch = ch;
                    reply.type = REPLY_TYPE::SUCCESS;
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
                // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::PUTS: %s\r\n" SAVE_CURSOR, req.str);
                unsigned char *str = req.str;
                reply.type = REPLY_TYPE::FAILURE; // incase it's full
                if (!transmit_buffer_str.IsFull())
                {
                    transmit_buffer_str.Push(str);
                    tx_waiting_tasks.Push(sender_tid);
                    reply.type = REPLY_TYPE::SUCCESS;
                }
                else
                {
                    reply.type = REPLY_TYPE::FAILURE;
                }

                if (tx_state == TX_ASSERTED)
                {
                    write_to_uart();
                }
                // uassert(false && "WAKING UP PUTS CALLED");
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
                    IO_REPLY rx_reply = {REPLY_TYPE::SUCCESS, ch};
                    ReplyWithMessage(waiting_task, rx_reply);
                }
            }
            break;
            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                reply.type = REPLY_TYPE::SUCCESS;
                write_to_uart();
            }
            break;

            default:
                break;
            }

            ReplyWithMessage(sender_tid, reply);
        }
    }

    void IOServer::write_to_uart()
    {
        if (!transmit_buffer_str.IsEmpty())
        {
            int sender_tid;
            while (!transmit_buffer_str.IsEmpty())
            {
                unsigned char *str = nullptr;
                transmit_buffer_str.Pop(&str);

                while (*str)
                {
                    uart_putc(CONSOLE, *(str++));
                }
                sender_tid = -1;
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

#if IRQ_ENABLED == 0
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::blocking until char received\r\n" SAVE_CURSOR);
        unsigned char ch = uart_getc(CONSOLE);
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::received char: %c\r\n" SAVE_CURSOR, ch);
        return ch;
#endif

        IO_REQUEST req{IO_REQUEST_TYPE::GETC, 0, nullptr};
        IO_REPLY reply;
        SEND(tid, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Getc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? reply.ch : -1;
    }

    int Putc(int tid, unsigned char ch)
    {
        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, ch, nullptr};
        IO_REPLY reply;
        SEND(tid, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Putc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    int Puts(int tid, unsigned char *str)
    {
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::Puts: CALLED\r\n" SAVE_CURSOR, str);
        int io_tid = WHOIS("IOServer");
        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::Puts Sending string to %d\r\n" SAVE_CURSOR, io_tid);
        uassert(io_tid > 0 && "IO_SERVER::Puts: PANIC, IOServer not found");

        IO_REQUEST req{IO_REQUEST_TYPE::PUTS, '\0', str};
        IO_REPLY reply;
        SEND(io_tid, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));

        // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::Puts SENT string to %d\r\n" SAVE_CURSOR, io_tid);
        uassert(reply.type != REPLY_TYPE::FAILURE && "IO_SERVER::Puts: PANIC, Failure");

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    // notify that user has entered a character
    // this works because flag is set based on receive buffer, which triggers interrupt
    void notifier_rxto() // receive timeout
    {
        // uart_printf(CONSOLE, RESTORE_CURSOR "RXTM NOTIFIER STARTED\r\n" SAVE_CURSOR);
        int IO_SERVER_TID = WHOIS("IOServer");
        REGISTERAS("CONSOLE_RTMNotifier");
        while (true)
        {
            if (UART_REG(CONSOLE, UART_FR) & UART_FR_RXTM_MASK)
            {
                // uart_printf(CONSOLE, RESTORE_CURSOR "RXTM NOTIFIER ASSERTED\r\n" SAVE_CURSOR);
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
        // uart_printf(CONSOLE, RESTORE_CURSOR "TX NOTIFIER STARTED\r\n" SAVE_CURSOR);
        int IO_SERVER_TID = WHOIS("IOServer");
        REGISTERAS("CONSOLE_TXNotifier");
        while (true)
        {
            AWAITEVENT(InterruptEvents::UART_TX);
            // if (UART_REG(CONSOLE, UART_FR) & UART_FR_TXFE_MASK)
            {
                uart_printf(CONSOLE, RESTORE_CURSOR "TX NOTIFIER ASSERTED\r\n" SAVE_CURSOR);
                // uassert(false && "TX NOTIFIER ASSERTED");
                tx_state = TX_ASSERTED;
                IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0, nullptr};
                IO_REPLY reply;

                SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::notifier_tx: PANIC, Unimplemented");
            }

            // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
        }
    }

    void startIOServer()
    {
        IO_SERVER::IOServer ioServer;
    }
}