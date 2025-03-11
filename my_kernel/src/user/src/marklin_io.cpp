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
    static int IO_SERVER_TID;

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
        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "Transmitted Bytes: ", TRANSMITTED_BYTES_LOCATION, 1);

        time_of_last_switch_transmission = 0;
        total_bytes_transmitted = 0;
        active_command_size = 0;
        bytes_transmitted = 0;
        last_switch_addr = -1;
        // initialized = false;
        REGISTERAS("MarklinIOServer");
        IO_SERVER_TID = MYTID();
        spawnNotifiers();
        numSeenCommands = 0;
        run();
    }

    MarklinIOServer::~MarklinIOServer()
    {
    }

    void MarklinIOServer::spawnNotifiers()
    {
        // ensure this runs before tx_notifier
        rx_notifier_tid = CREATE(PRIORITY::P0, notifier_rx);
        uassert(rx_notifier_tid >= 0 && "Error starting RX Notifier");

        tx_notifier_tid = CREATE(PRIORITY::P0, notifier_tx);
        uassert(tx_notifier_tid >= 0 && "Error starting TX Notifier");

        // clock notifier to ensure commands are sent at specific intervals
        clock_notifier_tid = CREATE(PRIORITY::P0, notifier_clock);
        uassert(clock_notifier_tid >= 0 && "Error starting Clock Notifier");
    }

    void notifier_rx()
    {
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

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "RX NOTIFIER REPLY returned error");
        }
    }
    void notifier_cts_high()
    {
        REGISTERAS("Marklin_CTS_HIGH_Notifier");
        // int transmi
        // WHOIS()
        while (true)
        {
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
            uassert(retval >= 0 && "CTS_HIGH NOTIFIER AWAITEVENT returned error");
            IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER_HIGH, 0};
            IO_REPLY reply;
            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "CTS_HIGH NOTIFIER REPLY returned error");
        }
    }

    void notifier_clock()
    {
        int IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(IO_SERVER_TID >= 0 && "MARKLIN CLOCK NOTIFIER::WHOIS returned error");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID >= 0 && "MARKLIN CLOCK NOTIFIER::WHOIS returned error");
        REGISTERAS("Marklin_ClockNotifier");
        while (true)
        {
            IO_REQUEST req{IO_REQUEST_TYPE::CLOCK_NOTIFIER, 0, nullptr};
            IO_REPLY reply;
            IO_NS::PrintTerminal("MARKLIN CLOCK NOTIFIER::Sending to Server\r\n");
            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            IO_NS::PrintTerminal("MARKLIN CLOCK NOTIFIER::Waiting for delay\r\n");
            // uassert(false && "MARKLIN WAITING FOR DELAY TO FINISH");
            DELAY(CLOCK_SERVER_TID, 300);
        }
    }

    void notifier_tx()
    {
        int ret = -1;
        int IO_SERVER_TID = WHOIS("MarklinIOServer");

        REGISTERAS("Marklin_TXNotifier");
        bool initialized = false;

        while (true)
        {
            // IO_NS::PrintTerminal("MK-TX::Waiting for Marklin to be ready to transmit\r\n");
            // uassert(!initialized && "WORKING! -- REMOVE THIS LINE AND IT SHOULD WORK");
            if (TX_STATUS(MARKLIN) != 1)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_TX);
                uassert(ret >= 0 && "TX NOTIFIER::TX_AWAITEVENT returned error");
            }
            // IO_NS::PrintTerminal("MK-TX::Marklin TX Status: %d\r\n", TX_STATUS(MARKLIN));

            if (CTS_STATUS(MARKLIN) != 1)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_HIGH_AWAITEVENT returned error");
            }
            // uassert(false && "TX NOTIFIER::CTS_HIGH_AWAITEVENT FORCED PANIC");

            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0};
            IO_REPLY reply;

            // IO_NS::PrintTerminal("MK-TX::Sending ready to transmit to Marklin Server \r\n");
            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply)); // Server needs to reply when it writes to the UART
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "TX NOTIFIER REPLY returned error");

            // IO_NS::PrintTerminal("MK-TX::Waiting for CTS LOW\r\n");

            // check if CTS is low
            if (CTS_STATUS(MARKLIN) != 0)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_LOW);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_LOW_AWAITEVENT returned error");
            }
            // uassert(false && "TX NOTIFIER::CTS_LOW_AWAITEVENT FORCED PANIC");
            // RECEIVE(&ret, (char *)&req, sizeof(req)); // use receive to notify of CTS HIGH

            // check if CTS is high
            // IO_NS::PrintTerminal("MK-TX::Waiting for CTS HIGH\r\n");
            // while (CTS_STATUS(MARKLIN) != 1)
            if (CTS_STATUS(MARKLIN) != 1)
            {
                ret = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
                uassert(ret >= 0 && "TX NOTIFIER::CTS_HIGH_AWAITEVENT returned error");
            }
            // uassert(false && "TX NOTIFIER::CTS_HIGH_AWAITEVENT FORCED PANIC");

            // IO_NS::PrintTerminal("MK-TX::CTS HIGH -- SEQUENCE COMPLETE\r\n");
            // uassert(false && "TX NOTIFIER::CTS_HIGH_AWAITEVENT FORCED PANIC");
            initialized = true;
        }
        // SEND TO MARKLIN TO PROCESS COMMAND
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
                // IO_NS::PrintTerminal("Total bytes transmitted: %d\r\n", total_bytes_transmitted);
                IO_NS::Print(MOVE_CURSOR COLOR_GREEN "%d", TRANSMITTED_BYTES_LOCATION, TRANSMITTED_BYTES_COL, total_bytes_transmitted);
                // WE CAN TRANSMIT TO MARKLIN
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
            case IO_REQUEST_TYPE::CLOCK_NOTIFIER:
            {
                // this should only be woken up when  we receive stop commands
                // IO_NS::PrintTerminal("MarklinIO_server::CLOCK_NOTIFIER\r\n");
                clock_blocked = false;
                break;
            }

            case IO_REQUEST_TYPE::SEND_CMD:
            {
                // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD\r\n");
                MarklinRequest m_req = *(req.request);
                switch (m_req.type)
                {
                case COMMAND::READ_SENSOR:
                {
                    // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD: Reading sensor %d\r\n", m_req.data);
                    cmd_buffer.Push(COMMAND::READ_SENSOR);
                    transmit_buffer.Push((unsigned char)m_req.data);
                    reply.type = REPLY_TYPE::SUCCESS;
                    send_reply = true;
                    break;
                }
                case COMMAND::SET_SWITCH:
                {
                    COMMAND last_pushed_cmd;
                    cmd_buffer.Peek(&last_pushed_cmd);

                    uint8_t switch_addr = m_req.id;
                    unsigned char state = m_req.data;

                    cmd_buffer.Push(COMMAND::SET_SWITCH);
                    transmit_buffer.Push(state);
                    transmit_buffer.Push(switch_addr);
                    last_switch_addr = switch_addr;

                    reply.type = REPLY_TYPE::SUCCESS;
                    send_reply = true;
                    break;
                }
                case COMMAND::SOLENOID_OFF:
                {
                    cmd_buffer.Push(COMMAND::SOLENOID_OFF);
                    transmit_buffer.Push(OFF);
                    transmit_buffer.Push(m_req.id);

                    reply.type = REPLY_TYPE::SUCCESS;
                    send_reply = true;
                    break;
                }
                case COMMAND::ACCELERATE_TRAIN:
                {
                    uint8_t train_addr = m_req.id;
                    uint8_t speed = m_req.data;
                    cmd_buffer.Push(COMMAND::ACCELERATE_TRAIN);
                    transmit_buffer.Push(speed);
                    transmit_buffer.Push(train_addr);

                    reply.type = REPLY_TYPE::SUCCESS;
                    send_reply = true;
                    break;
                }
                case COMMAND::REVERSE_STOP_TRAIN:
                {
                    uint8_t train_addr = m_req.id;
                    cmd_buffer.Push(COMMAND::REVERSE_STOP_TRAIN);
                    transmit_buffer.Push(0);          // send the speed back
                    transmit_buffer.Push(train_addr); // send the train address back

                    reply.type = REPLY_TYPE::SUCCESS;
                    send_reply = true;
                    break;
                }
                case COMMAND::REVERSE_TRAIN:
                {
                    uint8_t train_addr = m_req.id;
                    cmd_buffer.Push(COMMAND::REVERSE_TRAIN);
                    transmit_buffer.Push(REVERSE_CMD); // send the speed back
                    transmit_buffer.Push(train_addr);  // send the train address back

                    reply.type = REPLY_TYPE::SUCCESS;
                    send_reply = true;
                    break;
                }
                default:
                    break;
                } // end switch (m_req.type)

            } // end case IO_REQUEST_TYPE::SEND_CMD

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
        if (active_command_size == bytes_transmitted)
        {
            if (active_command == COMMAND::REVERSE_STOP_TRAIN && clock_blocked)
            {
                // reply to clock notifier
                REPLY(clock_notifier_tid, nullptr, 0);
                return;
            }
            // We have processed the entire command, pop the next command
            if (cmd_buffer.IsEmpty())
            {
                return;
            }

            cmd_buffer.Pop(&active_command);

            // IO_NS::PrintTerminal("Processing next command: %d\r\n", active_command);
            numSeenCommands++;
            active_command_size = (active_command == COMMAND::READ_SENSOR) ? 1 : 2;
            bytes_transmitted = 0;
        }

        if (canTransmit && !transmit_buffer.IsEmpty())
        {
            IO_REPLY reply = {REPLY_TYPE::SUCCESS, UNDEFINED_CHAR};
            REPLY(tx_notifier_tid, (char *)&reply, sizeof(reply));

            unsigned char ch;
            transmit_buffer.Pop(&ch);
            uart_putc_non_blocking(MARKLIN, ch);

            canTransmit = false;
            bytes_transmitted++;
            total_bytes_transmitted++;

            if (active_command == COMMAND::SET_SWITCH && bytes_transmitted == active_command_size)
            {
                time_of_last_switch_transmission = clock.Time();
                // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: SET_SWITCH -- CLOCK_BLOCKED!\r\n");
            }

            // wake up notifier
            if (active_command == COMMAND::REVERSE_STOP_TRAIN && bytes_transmitted == active_command_size)
            {
                IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: REVERSE_STOP_TRAIN -- CLOCK_BLOCKED!\r\n");
                clock_blocked = true;
            }
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

        return reply.type == REPLY_TYPE::FAILURE ? -1 : 0;
    }

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }
}
