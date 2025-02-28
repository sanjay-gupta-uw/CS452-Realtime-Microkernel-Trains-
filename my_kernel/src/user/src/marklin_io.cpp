#include "../include/marklin_io.h"
#include <cstdint>
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"

#include "../include/io.h"
#include "../../rpi.h"
#include "../../clock.h"

extern Clock clock;
namespace MARKLIN_IO_SERVER
{
    static int CTS_COUNT;

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
        REGISTERAS("MarklinIOServer");
        IO_SERVER_TID = MYTID();
        spawnNotifiers();
        run();
        CTS_COUNT = 0;
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
            uart_printf(CONSOLE, "Error starting TX Notifier\n");
        }
        // pending_tramissions.Push(tx_notifier_tid); // initial transmission

        cts_notifier_tid = CREATE(PRIORITY::P2, notifier_cts);
        if (cts_notifier_tid < 0)
        {
            uart_printf(CONSOLE, "Error starting CTS Notifier\n");
        }
    }

    void MarklinIOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            // uart_printf(CONSOLE, "MarklinIO_server::Waiting for request\r\n");
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
            if (retval < 0)
            {
                // uart_printf(CONSOLE, "PANIC, IO_SERVER RECEIVE returned error %d\n", retval);
            }
            // uart_printf(CONSOLE, "MarklinIO_server::Received request from %d\r\n", sender_tid);

            IO_REPLY reply;
            reply.type = REPLY_TYPE::FAILURE;
            reply.ch = UNDEFINED_CHAR;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::GETC:
            {
                // uart_printf(CONSOLE, "MarklinIO_server::GETC\r\n");
                reply.type = REPLY_TYPE::UNIMPLEMENTED;
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
                handle_transmission();

                ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::SEND_CMD:
            {
                IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD\r\n", CMD_LOCATION + 2, 1);
                MarklinRequest m_req = *(req.request);
                switch (m_req.type)
                {
                case MARKLIN_REQUEST_TYPE::SET_SWITCH:
                {
                    // uint8_t byte = m_req.data[0];
                    // cmd_buffer.Push('s');

                    // transmit_buffer.Push()
                }
                break;
                case MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN:
                {
                    uint8_t train_addr = m_req.id;
                    uint8_t speed = m_req.data[0];
                    cmd_buffer.Push('t');
                    transmit_buffer.Push(speed);
                    transmit_buffer.Push(train_addr);

                    handle_transmission();
                }
                break;
                case MARKLIN_REQUEST_TYPE::REVERSE_TRAIN:
                {
                    // uint8_t train_addr = m_req.id;
                    // cmd_buffer.Push('r');
                    // transmit_buffer.Push(train_addr);
                }
                break;
                default:
                    break;
                }

                handle_transmission();

                // wake up notifier
                IO_REPLY reply;
                reply.type = REPLY_TYPE::SUCCESS;
                REPLY(sender_tid, (char *)&reply, sizeof(reply));
            }

            // this should fire when byte received since no FIFO
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
            }
            break;

            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                tx_state_machine.Transition(STATES::TX_HIGH);
                handle_transmission();

                reply.type = REPLY_TYPE::SUCCESS;
                ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            case IO_REQUEST_TYPE::CTS_NOTIFIER:
            {
                // uart_printf(CONSOLE, "MarklinIO_server::CTS_NOTIFIER\r\n");
                tx_state_machine.Transition(req.data ? STATES::CTS_HIGH : STATES::CTS_LOW);
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
        // try to start a transaction
        if (tx_state_machine.BeginTransaction() || !tx_state_machine.isByteChosen())
        {
            if (!transmit_buffer.IsEmpty())
            {
                // can check the TYPE of command by checking cmd_buffer
                write_to_uart(); // write single byte from transmit buffer
                // use count to pop from cmd_buffer
            }
        }
    }

    // can only send byte
    void MarklinIOServer::write_to_uart()
    {
        // // uart_printf(CONSOLE, "MarklinIO_server::write_to_uart\r\n");
        unsigned char ch = UNDEFINED_CHAR;
        transmit_buffer.Pop(&ch);
        tx_state_machine.Transition(STATES::BYTE_CHOSEN);
        // uart_putc(MARKLIN, (uint8_t)data);
        uart_putc(MARKLIN, ch);
        // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::write_to_uart: Transmitting %c\r\n", CMD_LOCATION + 3, 1, ch);
        // spin_debug();
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
            // IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "TX NOTIFIER AWOKEN", CMD_LOCATION + 3, 1);
            // spin_debug();

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
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "CTS NOTIFIER AWOKEN, CTS: %d", CMD_LOCATION + 9 + CTS_COUNT, 1, CTS_SIGNAL);
            CTS_COUNT++;
            clock.Delay(5000);
            // spin_debug();

            if (CTS_SIGNAL < 0)
            {
                // uart_printf(CONSOLE, "PANIC, CTS_NOTIFIER AWAITEVENT returned error %d\n", CTS_SIGNAL);
                spin_debug();
            }

            IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER, CTS_SIGNAL, nullptr};
            int reply;

            SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            // uart_printf(CONSOLE, "CTS NOTIFIER UNBLOCKED\r\n");
        }
    }

    void STATE_MACHINE::Reset()
    {
        for (int i = 0; i < 4; i++)
        {
            STATE_ARR[i] = false;
        }

        status = STATUS::INACTIVE;
    }

    STATE_MACHINE::STATE_MACHINE()
    {
        for (int i = 0; i < NUM_STATES; i++)
        {
            STATE_ARR[i] = false;
        }
        // Reset();
    }

    STATE_MACHINE::~STATE_MACHINE()
    {
    }

    bool STATE_MACHINE::BeginTransaction()
    {
        if (status == STATUS::ACTIVE)
        {
            return false;
        }

        Reset();
        STATE_ARR[0] = true;
        status = STATUS::ACTIVE;
        return true;
    }

    void STATE_MACHINE::Transition(STATES state)
    {
        int idx = (int)state;
        if (STATE_ARR[idx])
        {
            if (state == STATES::TX_HIGH)
            {
                idx = (int)STATES::TX_HIGH2;
                STATE_ARR[idx] = true;
                IO_NS::Print(MOVE_CURSOR COLOR_CYAN CLEAR_TO_END_LINE "TX REASSERTED", CMD_LOCATION + 8, 1);
            }
            else if (state == STATES::CTS_HIGH)
            {
                idx = (int)STATES::CTS_HIGH2;
                STATE_ARR[idx] = true;
                IO_NS::Print(MOVE_CURSOR COLOR_CYAN CLEAR_TO_END_LINE "CTS REASSERTED", CMD_LOCATION + 8, 1);
            }
        }
        else
        {
            STATE_ARR[idx] = true;
        }
        // IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::Transition {%d}\r\n", CMD_LOCATION + 6 + idx, 1, idx);
        // if (idx > 3)
        // {
        //     spin_debug();
        // }
    }

    bool STATE_MACHINE::isTransactionComplete()
    {
        for (int i = 0; i < 4; i++)
        {
            if (!STATE_ARR[i])
            {
                return false;
            }
        }
        status = STATUS::INACTIVE;
        return true;
    }

    bool STATE_MACHINE::isByteChosen()
    {
        return STATE_ARR[(int)STATES::BYTE_CHOSEN];
    }

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }

}