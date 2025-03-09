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
    static const uint32_t UART_DR = 0x00;
#define UART_FR_TXFE_MASK 0x80
#define UART_FR_RXTM_MASK 0x40
#define UART_FR 0x18
#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))
    static char *const line_uarts[] = {NULL, UART0_BASE, UART3_BASE};

    IOServer::IOServer()
    {
        int my_tid = MYTID();
        int retval = REGISTERAS("IOServer");
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
    }

    void IOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        int count_send = 0;
        int count_tx = 0;
        bool awaiting_tx = false;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            uart_putc(MARKLIN, 'R');
            // uart_printf(CONSOLE, RESTORE_CURSOR "IO_SERVER::run: Received message from %d, type: %d\r\n" SAVE_CURSOR, sender_tid, req.type);
            uassert(retval >= 0 && "IO_SERVER::run: PANIC, Error receiving message");

            IO_REPLY reply;
            reply.type = REPLY_TYPE::FAILURE;
            reply.ch = UNDEFINED_CHAR;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::PUTC:
            {
                uart_putc(MARKLIN, 'P');
                int ret = transmit_buffer.Push(req.ch);
                uassert(ret >= 0 && "IO_SERVER::run: PANIC, Error pushing to transmit buffer");
                reply.type = REPLY_TYPE::SUCCESS;
                break;
            }
            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                uart_putc(MARKLIN, 'T');
                awaiting_tx = false;
                count_tx++;
                // uassert(false && "IO_SERVER::run: PANIC, TX_NOTIFIER received");
                break;
            }
            case IO_REQUEST_TYPE::RTM_NOTIFIER:
            {
                uassert(false && "IO_SERVER::run: PANIC, RTM_NOTIFIER received");
                break;
            }

            default:
                break;
            }

            // while (!transmit_buffer.IsEmpty())
            // {
            while (!transmit_buffer.IsEmpty())
            {
                if (!IS_TX_FIFO_FULL(CONSOLE))
                {
                    // uassert(false && "IO_SERVER::run: PANIC, TRANSMITTED");
                    unsigned char ch;
                    transmit_buffer.Pop(&ch);
                    uart_putc_non_blocking(CONSOLE, ch);
                    // uassert(false && "DID DUMMY GET HIT?");
                }
                else if (!awaiting_tx)
                {
                    awaiting_tx = true;
                    uassert(false && "IO_SERVER::run: PANIC -- replying to TX to re-enable IRQ");
                    REPLY(tx_notifier_tid, nullptr, 0);
                }
            }
            // }

            // uassert(count_send < 2 && "IO_SERVER::run: PANIC, waking up non TX task");
            if (sender_tid != tx_notifier_tid)
            {
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
            }
        }
    }
    int Getc(int tid)
    {
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
    }

    void notifier_rxto()
    {
        int IO_SERVER_TID = WHOIS("IOServer");
        REGISTERAS("CONSOLE_RXTO_Notifier");
        while (true)
        {
            int ret = AWAITEVENT(InterruptEvents::UART_RX_TIMEOUT);
            uassert(ret >= 0 && "IO_SERVER::notifier_rxto: PANIC, Error awaiting RX Timeout event");

            IO_REQUEST req{IO_REQUEST_TYPE::RTM_NOTIFIER, 0, nullptr};
            IO_REPLY reply;
            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }
    void notifier_tx()
    {
        // uart_printf(CONSOLE, RESTORE_CURSOR "TX NOTIFIER STARTED\r\n" SAVE_CURSOR);
        int IO_SERVER_TID = WHOIS("IOServer");
        bool awoken = false;
        REGISTERAS("CONSOLE_TX_Notifier");
        while (true)
        {
            // uart_putc(MARKLIN, 'A');
            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0, nullptr};
            IO_REPLY reply;

            // uassert(!awoken && "IO_SERVER::notifier_tx: PANIC, FORCED -- AWOKEN -- sending TX to IO Server");

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            // uassert(false && "IO_SERVER::notifier_tx: FORCED PANIC -- REPLY SENT");

            int ret = AWAITEVENT(InterruptEvents::UART_TX);
            // uassert(false && "IO_SERVER::notifier_tx: PANIC, FORCED -- TX AWOKEN");
            awoken = true;
            uassert(ret >= 0 && "IO_SERVER::notifier_tx: PANIC, Error awaiting TX event");
        }
        // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
    }

    void startIOServer()
    {
        IO_SERVER::IOServer ioServer;
    }
}