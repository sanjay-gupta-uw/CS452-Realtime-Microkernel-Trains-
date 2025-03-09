#include "../../register.h"
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
        // rtm_notifier_tid = CREATE(PRIORITY::P0, notifier_rxto);
        // uassert(rtm_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting RTM Notifier");

        tx_notifier_tid = CREATE(PRIORITY::P0, notifier_tx);
        uassert(tx_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting TX Notifier");

        // // ensure it runs after tx-notifiers
        int init_stream_tid = CREATE(PRIORITY::P1, init_stream);
        uassert(init_stream_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting TX Notifier");

        cts_notifier_tid = CREATE(PRIORITY::P0, notifier_cts);
        uassert(cts_notifier_tid >= 0 && "IO_SERVER::spawnNotifiers: PANIC, Error starting CTS Notifier");
    }

    void IOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::run: Received message from %d, type: %d\r\n" SAVE_CURSOR, sender_tid, req.type);
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
                    REPLY(sender_tid, (char *)&reply, sizeof(reply));
                }
                else
                {
                    rx_waiting_tasks.Push(sender_tid);
                }
            }
            break;
            case IO_REQUEST_TYPE::PUTS:
            {
                // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::PUTS: %s\r\n" SAVE_CURSOR, req.str);
                unsigned char *str = req.str;
                reply.type = REPLY_TYPE::FAILURE; // incase it's full
                if (!transmit_buffer.IsFull())
                {
                    while (*str != '\0')
                    {
                        int ret = transmit_buffer.Push(*str);
                        uassert(ret != -1 && "IO_SERVER::run: PANIC, Error pushing to transmit buffer");
                        str++;
                    }
                    reply.type = REPLY_TYPE::SUCCESS;
                }
                else
                {
                    reply.type = REPLY_TYPE::FAILURE;
                }
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
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
                    REPLY(waiting_task, (char *)&rx_reply, sizeof(rx_reply));
                }

                REPLY(sender_tid, (char *)&reply, sizeof(reply));
            }
            break;
            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                // uassert(false && "IO_SERVER::run: PANIC, TX_NOTIFIER CALLED");
            }
            break;

            default:
                break;
            }

            // check status
            if (tx_state == TX_ASSERTED)
            {
                // uassert(false && "WAKING UP PUTS CALLED");
                write_to_uart();
            }
        }
    }

    void IOServer::transmit_char(unsigned char ch)
    {
        tx_state = TX_DEASSERTED;
        uart_putc(CONSOLE, ch);
        REPLY(tx_notifier_tid, nullptr, 0);
    }

    void IOServer::write_to_uart()
    {
        if (!transmit_buffer.IsEmpty())
        {
            // unsigned char ch = 'A';
            unsigned char ch;
            transmit_buffer.Pop(&ch);
            transmit_char(ch);
            // uassert(false && "IO_SERVER::write_to_uart: PANIC, Unimplemented");
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
        bool awoken = false;
        REGISTERAS("CONSOLE_TXNotifier");
        while (true)
        {
            int status = TX_STATUS(CONSOLE);
            // uart_printf(CONSOLE, RESTORE_CURSOR "TX NOTIFIER STATUS: %d\r\n" SAVE_CURSOR, status);
            if (status == 1)
            {
                // uassert(false && "TX NOTIFIER ASSERTED");
                tx_state = TX_ASSERTED;
                IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0, nullptr};
                IO_REPLY reply;
                // uart_printf(CONSOLE, RESTORE_CURSOR "TX NOTIFIER ASSERTED -- sending to IO [%d]\r\n" SAVE_CURSOR, IO_SERVER_TID);

                SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                // uassert(false && "IO_SERVER::notifier_tx: FORCED PANIC");
            }
            else if (awoken)
            {
                uassert(false && "TX NOTIFIER NOT ASSERTED AFTER AWAIT");
            }
            // else
            // {
            //     uassert(false && "TX NOTI INIT CONDITION")
            // }
            int ret = AWAITEVENT(InterruptEvents::UART_TX);
            status = TX_STATUS(CONSOLE);

            awoken = true;
            // uassert(false && "IO_SERVER::notifier_tx: FORCED PANIC");
            // uart_printf(CONSOLE, RESTORE_CURSOR "TX NOTIFIER AWOKEN\r\n" SAVE_CURSOR);
        }
        // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
    }

    void notifier_cts()
    {
        int IO_SERVER_TID = WHOIS("IOServer");
        REGISTERAS("CONSOLE_CTSNotifier");
        while (true)
        {
            AWAITEVENT(InterruptEvents::UART_CTS);
            // if (UART_REG(CONSOLE, UART_FR) & UART_FR_TXFE_MASK)
            {
                // uart_printf(CONSOLE, RESTORE_CURSOR "CTS NOTIFIER ASSERTED\r\n" SAVE_CURSOR);
                // uassert(false && "CTS NOTIFIER ASSERTED");
                IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER, 0, nullptr};
                IO_REPLY reply;

                SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::notifier_cts: PANIC, Unimplemented");
            }
        }
    }

    void startIOServer()
    {
        IO_SERVER::IOServer ioServer;
    }
}