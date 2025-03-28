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
    // returns -1 if not a switch command, otherwise, returns the index of the switch
    int MarklinIOServer::isSwitchCommand(int addr)
    {
        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            if (SWITCH_ADDRS[i] == addr)
            {
                return i;
            }
        }
        return -1;
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
        CLOCK_SERVER_TID = WHOIS("ClockServer");

        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "Transmitted Bytes: ", TRANSMITTED_BYTES_LOCATION, 1);
        const int switch_addrs[NUM_SWITCHES] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0x99, 0x9A, 0x9B, 0x9C};

        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            SWITCH_ADDRS[i] = switch_addrs[i];
            // CREATE SWITCH NOTIFIER
            switch_notifier_tid[i] = CREATE(PRIORITY::MARKLIN_NOTIFIER, switchNotifier);
            RECEIVE(nullptr, nullptr, 0);
        }

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
        rx_notifier_tid = CREATE(PRIORITY::MARKLIN_NOTIFIER, notifier_rx);
        uassert(rx_notifier_tid >= 0 && "Error starting RX Notifier");
        IO_NS::PrintTerminal("RX Notifier started with TID %d\r\n", rx_notifier_tid);

        tx_notifier_tid = CREATE(PRIORITY::MARKLIN_NOTIFIER, notifier_tx);
        uassert(tx_notifier_tid >= 0 && "Error starting TX Notifier");
        IO_NS::PrintTerminal("TX Notifier started with TID %d\r\n", tx_notifier_tid);
    }

    void notifier_rx()
    {
        REGISTERAS("Marklin_RXNotifier");
        int my_tid = MYTID();
        int MARKLIN_IO_TID = WHOIS("MarklinIOServer");
        IO_NS::PrintTerminal("RX Notifier started with TID %d\r\n", my_tid);

        int count = 0;
        RECEIVED_BYTES_STRUCT receive_buffer;

        IO_REPLY reply;
        while (true)
        {
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_RX);
            uassert(retval >= 0 && "RX NOTIFIER AWAITEVENT returned error");

            unsigned char ch = uart_getc_non_blocking(MARKLIN);
            receive_buffer.bytes[count++] = ch;
            receive_buffer.count = count;

            if (count == 10)
            {
                IO_REQUEST req(&receive_buffer);
                SEND(MARKLIN_IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                // uassert(false && "SERVER REPLIED TO RX NOTIFIER");
                count = 0;
                // IO_NS::PrintTerminal("WAITING FOR RX");
            }
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
                // IO_NS::PrintTerminal("TX NOTIFIER::Waiting for TX\r\n");
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_TX);
                uassert(ret >= 0 && "TX NOTIFIER::TX_AWAITEVENT returned error");
            }

            if (CTS_STATUS(MARKLIN) != 1)
            {
                // IO_NS::PrintTerminal("TX NOTIFIER::Waiting for CTS HIGH\r\n");
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_HIGH_AWAITEVENT returned error");
            }

            IO_REQUEST req(IO_REQUEST_TYPE::TX_NOTIFIER);
            IO_REPLY reply;

            // IO_NS::PrintTerminal("TX NOTIFIER::Available to transmit\r\n");

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
        isReceiving = false;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            int tick = TIME(CLOCK_SERVER_TID);
            // IO_NS::PrintTerminal("MarklinIO_server::run: Received request from tid{%d} at tick %d\r\n", sender_tid, tick);

            IO_REPLY reply;
            reply.type = REPLY_TYPE::FAILURE;
            reply.ch = UNDEFINED_CHAR;
            bool send_reply = false;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
                RECEIVED_BYTES_STRUCT *rec_buf = req.data.received_bytes;
                for (int i = 0; i < rec_buf->count; ++i)
                {
                    unsigned char ch;
                    ch = rec_buf->bytes[i];
                    receive_buffer.Push(ch);
                    BYTES_RECEIVED++;
                    // IO_NS::PrintTerminal(COLOR_CYAN "MarklinIO_server::RX_NOTIFIER: {%d} Received %d\r\n", i, ch);
                }
                // uassert(false && "MarklinIO_server::RX_NOTIFIER: -- Received RX NOTIFIER");
                // IO_NS::PrintTerminal(COLOR_CYAN "MarklinIO_server::RX_NOTIFIER: Received %d bytes, was waiting for %d bytes\r\n", count, TOTAL_BYTES_TO_RECEIVE);
                if (BYTES_RECEIVED == TOTAL_BYTES_TO_RECEIVE)
                {
                    isReceiving = false;
                    BYTES_RECEIVED = 0;
                    TOTAL_BYTES_TO_RECEIVE = 0;
                    // IO_NS::PrintTerminal(COLOR_RED "MarklinIO_server::RX_NOTIFIER: Finished receiving %d bytes\r\n", TOTAL_BYTES_TO_RECEIVE);
                }

                while (!rx_waiting_tasks.IsEmpty() && !receive_buffer.IsEmpty())
                {
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    int waiting_tid;
                    rx_waiting_tasks.Pop(&waiting_tid);

                    // uassert(false && "MarklinIO_server::RX_NOTIFIER: -- Sending reply to waiting task");

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
                    // uassert(false && "MarklinIO_server::GETC: -- Sending reply to waiting task");
                }
                else
                {
                    rx_waiting_tasks.Push(sender_tid);
                }
                break;
            }
            case IO_REQUEST_TYPE::SEND_CMD:
            {
                IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD:: Received command from tid{%d}\r\n", sender_tid);
                MarklinRequest *m_req = req.data.request;
                if (m_req->isSingleByteCommand)
                {
                    // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD: Byte1: %d\r\n", m_req->byte1);
                }
                else
                {
                    // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD: Byte1: %d, Byte2: %d\r\n", m_req->byte1, m_req->byte2);
                }
                transmit_buffer.Push(m_req->byte1);
                int len = 1;
                if (!m_req->isSingleByteCommand)
                {
                    len = 2;
                    int addr = m_req->byte2;

                    transmit_buffer.Push(addr);
                }
                else
                {
                    // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD: Single byte command\r\n");
                }

                sequence_length_buffer.Push(len);
                reply.type = REPLY_TYPE::SUCCESS;
                send_reply = true;
                break;
            }

            case IO_REQUEST_TYPE::PUTC:
            {
                // ASSUME ALL PUTC COMMANDS ARE SOLENOID OFF COMMANDS, SWITCH ADDR STORED IN REQ.DATA
                if (sequence_length > 0 && count > 0) // these will never be equal otherwise (handle trasmission sets them both to zero when count == length)
                {
                    // IO_NS::PrintTerminal("MARKLIN_IO_SERVER::PUTC: sequence_length: %d, count: %d\r\n", sequence_length, count);
                    // uassert(sequence_length == count && "UNEXPECTED ERROR: TRASMISSION DIDNT UPDATE SEQUENCE LENGTH AND COUNT?");
                    int bytes_to_extract = sequence_length - count;

                    // two byte command at most
                    uint8_t command_buff[2];

                    for (int i = 0; i < bytes_to_extract; ++i)
                    {
                        transmit_buffer.Pop(&command_buff[i]);
                    }

                    PushSolenoidOffCommand();

                    for (int i = 0; i < bytes_to_extract; ++i)
                    {
                        transmit_buffer.PushFront(command_buff[(i + 1) % bytes_to_extract]);
                    }
                }
                else
                {
                    PushSolenoidOffCommand();
                }
                break;
            }

            default:
                break;
            }

            // IO_NS::PrintTerminal("canTransmit: %d, isReceiving: %d\r\n", canTransmit, isReceiving);
            if (canTransmit && !isReceiving)
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

    void MarklinIOServer::PushSolenoidOffCommand()
    {
        sequence_length_buffer.PushFront(1);
        transmit_buffer.PushFront(SOLENOID_OFF_CMD);
        // IO_NS::PrintTerminal("MarklinIO_server::PushSolenoidOffCommand: Pushed solenoid off command\r\n");
    }

    void MarklinIOServer::handle_transmission()
    {
        uassert(canTransmit && !isReceiving && "MarklinIOServer::handle_transmission: PANIC, cannot transmit");
        if (!transmit_buffer.IsEmpty())
        {
            // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: Transmitting\r\n");
            IO_REPLY reply = {REPLY_TYPE::SUCCESS, UNDEFINED_CHAR};
            REPLY(tx_notifier_tid, (char *)&reply, sizeof(reply));

            if (count == 0)
            {
                sequence_length_buffer.Pop(&sequence_length);
            }
            else if (count == 2 && sequence_length == 3)
            {
                end_time = clock.Time();
                // uassert(false && "FORCED PANIC");
                // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: sending solenoid off within: %d ms\r\n", (end_time - start_time) / 1000);
                // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: Finished transmitting sequence\r\n");
            }

            unsigned char ch;
            transmit_buffer.Pop(&ch);
            uart_putc_non_blocking(MARKLIN, ch);

            canTransmit = false;
            total_bytes_transmitted++;
            count++;

            int DELAY_TIME = 25;

            if (sequence_length == 1)
            {
                int num_banks_to_read = int(ch) - READ_ALL_SENSOR_BASE;
                // check if command is a sensor read -- need to make sure we dont transmit until we have the sensor data
                if (num_banks_to_read > 0 && num_banks_to_read <= NUM_BANKS)
                {
                    BYTES_RECEIVED = 0;
                    TOTAL_BYTES_TO_RECEIVE = num_banks_to_read * 2; // 2 bytes per bank
                    isReceiving = true;
                    // IO_NS::PrintTerminal(COLOR_RED "MarklinIO_server::handle_transmission: Sensor read command for %d banks\r\n", num_banks_to_read);
                    // uassert(false && "MarklinIO_server::SEND_CMD: VERIFYING READ BYTES IS CORRECT");
                }
            }

            if (count == 2)
            {
                int idx = isSwitchCommand(ch);
                if (idx != -1)
                {
                    // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: REQUESTING SOLENOID OFF AFTER DELAY\r\n");
                    REPLY(switch_notifier_tid[idx], nullptr, 0);
                }
            }
            if (count == sequence_length)
            {
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

        IO_REQUEST req(IO_REQUEST_TYPE::GETC);
        IO_REPLY reply;
        SEND(IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Getc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? reply.ch : -1;
    }

    int Putc(int tid, unsigned char ch)
    {
        IO_NS::PrintTerminal("MarklinIO::Putc: THIS FUNCTION IS DEPRECATED\r\n");
        return -1;
        // int IO_TID = WHOIS("MarklinIOServer");

        // IO_REQUEST req(IO_REQUEST_TYPE::PUTC, ch);
        // IO_REPLY reply;
        // SEND(IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Putc: PANIC, Unimplemented");

        // return reply.type == REPLY_TYPE::SUCCESS ? 0 : -1;
    }

    int SendCmd(int tid, MarklinRequest *request)
    {
        int IO_TID = WHOIS("MarklinIOServer");

        IO_REQUEST req(request);
        IO_REPLY reply;
        SEND(IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        // IO_NS::PrintTerminal("MarklinIO::SendCmd: Successfully sent request to MarklinIOServer\r\n");

        return reply.type == REPLY_TYPE::FAILURE ? -1 : 0;
    }

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }

    // this is used to send a solenoid off command to the switch
    void switchNotifier()
    {
        int my_tid = MYTID();
        int MARKLIN_IO_TID = MYPARENTTID();
        int CLOCK_SERVER_TID = WHOIS("ClockServer");

        // BYPASS PUTC LOOKUP
        IO_REQUEST req(IO_REQUEST_TYPE::SWITCH_NOTIFIER);
        while (true)
        {
            // IO_NS::PrintTerminal("SWNOTIFIER{%d}: Waiting for switch notifier\r\n", my_tid);
            SEND(MARKLIN_IO_TID, (char *)&req, sizeof(req), nullptr, 0);
            // IO_NS::PrintTerminal("SWNOTIFIER{%d}: DELAYING\r\n", my_tid);
            // IO_NS::PrintTerminal(COLOR_YELLOW "SWITCH NOTIFIER{%d}: DELAYING\r\n", my_tid);
            DELAY(CLOCK_SERVER_TID, 25);
        }
    }
}
