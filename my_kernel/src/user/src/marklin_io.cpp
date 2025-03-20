#include "../../register.h"
#include "../include/marklin_io.h"
#include "../include/name_server.h"
#include "../include/clock_server.h"
#include "../../include/syscall.h"
#include "../../shared_constants.h"

#include "../include/io.h"
#include "../../rpi.h"
#include "../../clock.h"
#include "../marklin/train.h"
#include "../include/uassert.h"
extern Clock clock;
namespace MARKLIN_IO_SERVER
{
    static void GETC_SUCCESS_REPLY(int tid, char ch)
    {
        IO_REPLY reply;
        reply.type = REPLY_TYPE::SUCCESS;
        reply.ch = ch;
        REPLY(tid, (char *)&reply, sizeof(reply));
    }

    MarklinIOServer::MarklinIOServer()
    {
        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "Transmitted Bytes: ", TRANSMITTED_BYTES_LOCATION, 1);

        total_bytes_transmitted = 0;
        REGISTERAS("MarklinIOServer");
        spawnNotifiers();

        sequence_length = 0;
        count = 0;

        run();
    }

    MarklinIOServer::~MarklinIOServer()
    {
    }

    void MarklinIOServer::spawnNotifiers()
    {
        // ensure this runs before tx_notifier
        rx_notifier_tid = CREATE(PRIORITY::P1, notifier_rx);
        uassert(rx_notifier_tid >= 0 && "Error starting RX Notifier");
        IO_NS::PrintTerminal("RX Notifier started with TID %d\r\n", rx_notifier_tid);

        tx_notifier_tid = CREATE(PRIORITY::P1, notifier_tx);
        uassert(tx_notifier_tid >= 0 && "Error starting TX Notifier");
        IO_NS::PrintTerminal("TX Notifier started with TID %d\r\n", tx_notifier_tid);
    }

    void notifier_rx()
    {
        int MARKLIN_IO_TID = WHOIS("MarklinIOServer");
        REGISTERAS("Marklin_RXNotifier");
        // (CONSOLE, "M-RX Notifier started\r\n");
        while (true)
        {
            // (CONSOLE, "ENABLING RX INTERRUPT\r\n");
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_RX);
            uassert(retval >= 0 && "RX NOTIFIER AWAITEVENT returned error");
            // (CONSOLE, "RX NOTIFIER AWOKEN\r\n");

            IO_REQUEST req{IO_REQUEST_TYPE::RX_NOTIFIER, 0};
            IO_REPLY reply;

            SEND(MARKLIN_IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "RX NOTIFIER REPLY returned error");
        }
    }

    void notifier_tx()
    {
        REGISTERAS("Marklin_TXNotifier");
        int MARKLIN_IO_TID = WHOIS("MarklinIOServer");

        int ret = -1;
        while (true)
        {
            if (TX_STATUS(MARKLIN) != 1)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_TX);
                uassert(ret >= 0 && "TX NOTIFIER::TX_AWAITEVENT returned error");
            }

            if (CTS_STATUS(MARKLIN) != 1)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_HIGH_AWAITEVENT returned error");
            }

            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0};
            IO_REPLY reply;

            // Notify the server that we can transmit
            SEND(MARKLIN_IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply)); // Server needs to reply when it writes to the UART
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "TX NOTIFIER REPLY returned error");

            // check if CTS is low
            if (CTS_STATUS(MARKLIN) != 0)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_LOW);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_LOW_AWAITEVENT returned error");
            }

            // check if CTS is high
            // IO_NS::PrintTerminal("MK-TX::Waiting for CTS HIGH\r\n");
            // while (CTS_STATUS(MARKLIN) != 1)
            if (CTS_STATUS(MARKLIN) != 1)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_HIGH_AWAITEVENT returned error");
            }
        }
    }

    void MarklinIOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        canTransmit = false;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            // IO_NS::PrintTerminal("MarklinIO_server::run: Received request from tid{%d} of size{%d}\r\n", sender_tid, retval);

            IO_REPLY reply;
            reply.type = REPLY_TYPE::FAILURE;
            reply.ch = UNDEFINED_CHAR;
            bool send_reply = false;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
                unsigned char ch = (unsigned char)uart_getc_non_blocking(MARKLIN);
                receive_buffer.Push(ch);

                if (!rx_waiting_tasks.IsEmpty())
                {
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    int waiting_tid;
                    rx_waiting_tasks.Pop(&waiting_tid);

                    GETC_SUCCESS_REPLY(waiting_tid, ch);
                }
                reply.type = REPLY_TYPE::SUCCESS;
                send_reply = true;
                // REPLY(sender_tid, (char *)&reply, sizeof(reply));
                break;
            }
            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                reply.type = REPLY_TYPE::SUCCESS;
                canTransmit = true;
                break;
            }
            case IO_REQUEST_TYPE::GETC:
            {
                if (!receive_buffer.IsEmpty()) // need to reply with CH
                {
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    reply.type = REPLY_TYPE::SUCCESS;
                    reply.ch = ch;
                    send_reply = true;
                }
                else
                {
                    rx_waiting_tasks.Push(sender_tid);
                }
                break;
            }
            case IO_REQUEST_TYPE::SEND_CMD:
            {
                // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD\r\n");
                MarklinRequest m_req = *(req.request);
                transmit_buffer.Push(m_req.byte1);
                int len = 1;
                if (!m_req.isSingleByteCommand)
                {
                    transmit_buffer.Push(m_req.byte2);
                    len = 4;
                }
                sequence_length_buffer.Push(len);
                reply.type = REPLY_TYPE::SUCCESS;
                send_reply = true;
            }

            default:
                break;
            }

            if (canTransmit)
            {
                handle_transmission();
            }

            if (send_reply)
            {
                // IO_NS::PrintTerminal("MarklinIO_server::run: Sending reply to tid{%d}\r\n", sender_tid);
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
            }
        }
    }

    void MarklinIOServer::handle_transmission()
    {
        uassert(canTransmit && "MarklinIOServer::handle_transmission: PANIC, cannot transmit");
        if (canTransmit && !transmit_buffer.IsEmpty())
        {
            // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: Transmitting\r\n");
            IO_REPLY reply = {REPLY_TYPE::SUCCESS, UNDEFINED_CHAR};
            REPLY(tx_notifier_tid, (char *)&reply, sizeof(reply));

            if (count == 0)
            {
                sequence_length_buffer.Pop(&sequence_length);
                start_time = clock.Time();
            }

            unsigned char ch;
            transmit_buffer.Pop(&ch);
            uart_putc_non_blocking(MARKLIN, ch);

            canTransmit = false;
            total_bytes_transmitted++;
            count++;
            if (count == sequence_length)
            {
                end_time = clock.Time();
                IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: Time to send 4-byte sequence: %d ms\r\n", (end_time - start_time) / 1000);
                count = 0;
                // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: Finished transmitting sequence\r\n");
            }
            IO_NS::Print(MOVE_CURSOR COLOR_GREEN "%d", TRANSMITTED_BYTES_LOCATION, TRANSMITTED_BYTES_COL, total_bytes_transmitted);
            // uassert(false && "FORCED PANIC -- FINISHED TRANSMITTING BYTE");
        }
    }

    int Getc(int tid)
    {
        int IO_TID = WHOIS("MarklinIOServer");

        IO_REQUEST req{IO_REQUEST_TYPE::GETC, 0, nullptr};
        IO_REPLY reply;
        SEND(IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Getc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? reply.ch : -1;
    }

    int Putc(int tid, unsigned char ch)
    {
        int IO_TID = WHOIS("MarklinIOServer");

        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, ch, nullptr};
        IO_REPLY reply;
        SEND(IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Putc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    int SendCmd(int tid, MarklinRequest *request)
    {
        int IO_TID = WHOIS("MarklinIOServer");

        IO_REQUEST req{IO_REQUEST_TYPE::SEND_CMD, 0, request};
        IO_REPLY reply;
        SEND(IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // IO_NS::PrintTerminal("MarklinIO::SendCmd: Successfully sent request to MarklinIOServer\r\n");

        return reply.type == REPLY_TYPE::FAILURE ? -1 : 0;
    }

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }
}
