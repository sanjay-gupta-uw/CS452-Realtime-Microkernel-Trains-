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
        active_command_size = 0;
        bytes_transmitted = 0;
        last_switch_addr = -1;
        // initialized = false;
        REGISTERAS("MarklinIOServer");
        IO_SERVER_TID = MYTID();
        spawnNotifiers();
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

        cts_notifier_high_tid = CREATE(PRIORITY::P0, notifier_cts_high);
        uassert(cts_notifier_high_tid >= 0 && "Error starting CTS Notifier");

        cts_notifier_low_tid = CREATE(PRIORITY::P0, notifier_cts_low);
        uassert(cts_notifier_low_tid >= 0 && "Error starting CTS Notifier");
    }

    void notifier_rx()
    {
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
    void notifier_tx()
    {
        // (CONSOLE, "M-TX Notifier started\r\n");
        while (true)
        {
            // (CONSOLE, "ENABLING TX INTERRUPT\r\n");
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_TX);
            uassert(retval >= 0 && "TX NOTIFIER AWAITEVENT returned error");
            // (CONSOLE, "TX NOTIFIER AWOKEN\r\n");

            IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0};
            IO_REPLY reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "TX NOTIFIER REPLY returned error");
        }
    }
    void notifier_cts_high()
    {
        // (CONSOLE, "M-CTS-HIGH Notifier started\r\n");
        while (true)
        {
            // (CONSOLE, "WAITING FOR CTS HIGH \r\n");
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_HIGH);
            uassert(retval >= 0 && "CTS NOTIFIER AWAITEVENT returned error");
            // (CONSOLE, "CTS-HIGH NOTIFIER AWOKEN: %d\r\n", retval);

            IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER_HIGH};
            IO_REPLY reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "CTS NOTIFIER REPLY returned error");
        }
    }
    void notifier_cts_low()
    {
        // (CONSOLE, "M-CTS-LOW Notifier started\r\n");
        while (true)
        {
            // (CONSOLE, "WAITING FOR CTS LOW\r\n");
            int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS_LOW);
            uassert(retval >= 0 && "CTS NOTIFIER AWAITEVENT returned error");
            // (CONSOLE, "CTS-LOW NOTIFIER AWOKEN: %d\r\n", retval);

            IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER_LOW};
            IO_REPLY reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "CTS NOTIFIER REPLY returned error");
        }
    }

    void MarklinIOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            // bool isReady = tx_state.isReady();
            // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::Ready to Transmit: {%s}\r\n", TRANSMIT_LOCATION + 2, 1, isReady ? "YES" : "NO");
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));

            IO_REPLY reply;
            reply.type = REPLY_TYPE::FAILURE;
            reply.ch = UNDEFINED_CHAR;
            bool sentReply = false;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
                unsigned char ch = (unsigned char)uart_getc(MARKLIN);
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
                // REPLY(sender_tid, (char *)&reply, sizeof(reply));
            }
            break;
            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                // tx_state.update_state(STATES::TX_HIGH);
                reply.type = REPLY_TYPE::SUCCESS;
            }
            break;
            case IO_REQUEST_TYPE::CTS_NOTIFIER_HIGH:
            {
                tx_state.update_state(STATES::CTS_HIGH);
                reply.type = REPLY_TYPE::SUCCESS;
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
                sentReply = true;
            }
            break;

            case IO_REQUEST_TYPE::CTS_NOTIFIER_LOW:
            {
                tx_state.update_state(STATES::CTS_LOW);
                reply.type = REPLY_TYPE::SUCCESS;
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
                sentReply = true;
            }
            break;

            case IO_REQUEST_TYPE::GETC:
            {
                if (!receive_buffer.IsEmpty()) // need to reply with CH
                {
                    unsigned char ch;
                    receive_buffer.Pop(&ch);
                    GETC_SUCCESS_REPLY(sender_tid, ch);
                    sentReply = true;
                }
                else
                {
                    rx_waiting_tasks.Push(sender_tid);
                }
            }
            break;
            case IO_REQUEST_TYPE::PUTC:
            {
                // reply.type = REPLY_TYPE::FAILURE; // incase it's full
                // if (!transmit_buffer.IsFull())    // this will ignore characters if "overwhelmed"
                // {
                //     transmit_buffer.Push((uint8_t)(req.data));
                //     reply.type = REPLY_TYPE::SUCCESS;
                // }

                // ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::SEND_CMD:
            {
                // IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD\r\n", CMD_LOCATION + 2, 1);
                MarklinRequest m_req = *(req.request);
                if (m_req.type != COMMAND::SET_SWITCH && last_switch_addr != -1)
                {
                    // send off command
                    cmd_buffer.Push(COMMAND::SOLENOID_OFF);
                    transmit_buffer.Push(OFF);
                    transmit_buffer.Push(last_switch_addr);

                    last_switch_addr = -1;
                }

                switch (m_req.type)
                {
                case COMMAND::READ_SENSOR:
                {
                    cmd_buffer.Push(COMMAND::READ_SENSOR);
                    transmit_buffer.Push((unsigned char)m_req.data);
                    reply.type = REPLY_TYPE::SUCCESS;
                }
                break;
                case COMMAND::SET_SWITCH:
                {
                    uint8_t switch_addr = m_req.id;
                    unsigned char state = m_req.data;

                    cmd_buffer.Push(COMMAND::SET_SWITCH);
                    transmit_buffer.Push(state);
                    transmit_buffer.Push(switch_addr);

                    IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Setting switch %d to state %c\r\n", CMD_STATUS_LOCATION, 1, switch_addr, state);
                    last_switch_addr = switch_addr;

                    reply.type = REPLY_TYPE::SUCCESS;
                }
                break;
                case COMMAND::ACCELERATE_TRAIN:
                {
                    // (CONSOLE, "#DEBUG# ~ MarklinIO_server::SEND_CMD: Accelerating train %d to speed %d\r\n", m_req.id, m_req.data);
                    uint8_t train_addr = m_req.id;
                    uint8_t speed = m_req.data;
                    cmd_buffer.Push(COMMAND::ACCELERATE_TRAIN);
                    transmit_buffer.Push(speed);
                    transmit_buffer.Push(train_addr);
                    // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Accelerating train %d to speed %d\r\n", CMD_STATUS_LOCATION, 1, train_addr, speed);

                    reply.type = REPLY_TYPE::SUCCESS;
                }
                break;
                case COMMAND::REVERSE_TRAIN:
                {
                    uint8_t train_addr = m_req.id;

                    transmit_buffer.Push(REVERSE_CMD); // send the speed back
                    transmit_buffer.Push(train_addr);  // send the train address back
                    IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Reversing train %d\r\n", CMD_STATUS_LOCATION, 1, train_addr);
                    reply.type = REPLY_TYPE::SUCCESS;
                }
                break;
                default:
                    break;
                } // end switch (m_req.type)

            } // end case IO_REQUEST_TYPE::SEND_CMD

            default:
                break;
            }

            handle_transmission();

            if (!sentReply)
            {
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
            }
        }
    }

    void MarklinIOServer::handle_transmission()
    {
        bool isReady = tx_state.isReady();
        // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::Ready to Transmit: {%s}\r\n", TRANSMIT_LOCATION + 4, 1, isReady ? "YES" : "NO");
        if (active_command_size == bytes_transmitted)
        {
            // We have processed the entire command, pop the next command
            if (cmd_buffer.IsEmpty())
            {
                return;
            }
            COMMAND cmd_type;
            cmd_buffer.Pop(&cmd_type);
            // (CONSOLE, "Processing next command: %d\r\n", cmd_type);

            active_command_size = (cmd_type == COMMAND::READ_SENSOR) ? 1 : 2;
            bytes_transmitted = 0;
        }
        // check if we can transmit (CTS STATE MACHINE)
        if (!transmit_buffer.IsEmpty() && tx_state.isReady())
        {
            tx_state.begin_transmission();
            write_to_uart();
        }
    }

    // can only send byte
    void MarklinIOServer::write_to_uart()
    {
        unsigned char ch = UNDEFINED_CHAR;
        transmit_buffer.Pop(&ch);
        // (CONSOLE, "MarklinIO_server::write_to_uart: {%d} transmitting: %c\r\n", ch);
        uart_putc(MARKLIN, ch);
        IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::write_to_uart: {%d} successfully transmitted.\r\n", TRANSMIT_LOCATION + bytes_transmitted, 1, ch);
        // (CONSOLE, "MarklinIO_server::write_to_uart: {%d} successfully transmitted.\r\n", ch);
        bytes_transmitted++;
    }

    int Getc(int tid)
    {
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::GETC, 0, nullptr};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Getc: PANIC, Unimplemented");

        return reply.type == REPLY_TYPE::SUCCESS ? reply.ch : -1;
    }

    int Putc(int tid, unsigned char ch)
    {
        if (tid != IO_SERVER_TID) // check if tid if valid uart server
        {
            return -1;
        }

        IO_REQUEST req{IO_REQUEST_TYPE::PUTC, ch, nullptr};
        IO_REPLY reply;
        SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        uassert(reply.type != REPLY_TYPE::UNIMPLEMENTED && "IO_SERVER::Putc: PANIC, Unimplemented");

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

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }
}
