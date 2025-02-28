#include "../include/marklin_io.h"

#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"

#include "../include/io.h"
#include "../../rpi.h"
#include "../../clock.h"

extern Clock clock;
namespace MARKLIN_IO_SERVER
{
    static char TRAIN_COMMANDS[3] = {'t', 'r', 's'};
#define MMIO_BASE 0xFE000000
    static char *const UART0_BASE = (char *)(MMIO_BASE + 0x201000);
    static char *const UART3_BASE = (char *)(MMIO_BASE + 0x201600);

#define UART_FR_TXFE_MASK 0x80
#define UART_FR_RXFE_MASK 0x10
#define UART_FR_CTS_MASK 0x01
#define UART_FR 0x18
#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))
    static char *const line_uarts[] = {NULL, UART0_BASE, UART3_BASE};

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

        cts_notifier_tid = CREATE(PRIORITY::P0, notifier_cts);
        if (cts_notifier_tid < 0)
        {
            uart_printf(CONSOLE, "Error starting CTS Notifier\n");
        }

        tx_notifier_tid = CREATE(PRIORITY::P1, notifier_tx);
        if (tx_notifier_tid < 0)
        {
            uart_printf(CONSOLE, "Error starting TX Notifier\n");
        }
        // pending_tramissions.Push(tx_notifier_tid); // initial transmission
    }

    void MarklinIOServer::run()
    {
        int sender_tid;
        IO_REQUEST req;
        while (true)
        {
            // uart_printf(CONSOLE, "MarklinIO_server::Waiting for request\r\n");
            int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));

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
                    transmit_buffer.Push((uint8_t)(req.data));
                    reply.type = REPLY_TYPE::SUCCESS;
                    count++;
                }
                handle_transmission();

                ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::SEND_CMD:
            {
                // uart_printf(CONSOLE, "MarklinIO_server::SEND_CMD\r\n");
                // spin_debug();
                // IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD\r\n", CMD_LOCATION + 2, 1);
                MarklinRequest m_req = *(req.request);
                switch (m_req.type)
                {
                case MARKLIN_REQUEST_TYPE::SET_SWITCH:
                {
                    // uint8_t byte = m_req.data[0];
                    // cmd_buffer.Push('s');

                    // transmit_buffer.Push()
                    break;
                }
                case MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN:
                {
                    // uart_printf(CONSOLE, "0-MarklinIO_server::SEND_CMD: Accelerating train \r\n");
                    uint8_t train_addr = m_req.id;
                    unsigned char speed = m_req.data;
                    // cmd_buffer.Push(TRAIN_COMMANDS[0]);
                    if (!transmit_buffer.IsFull())
                    {
                        transmit_buffer.Push((unsigned char)(speed));
                    }
                    if (!transmit_buffer.IsFull())
                    {
                        transmit_buffer.Push((unsigned char)(train_addr));
                    }
                    // uart_printf(CONSOLE, COLOR_MAGENTA "MarklinIO_server::SEND_CMD: Accelerating train %d to speed %d\r\n", train_addr, speed);
                    // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Accelerating train %d to speed %d\r\n", CMD_LOCATION + 3, 1, train_addr, speed);
                    // wake up notifier
                    IO_REPLY reply;
                    reply.type = REPLY_TYPE::SUCCESS;
                    REPLY(sender_tid, (char *)&reply, sizeof(reply)); // wakeup marklin-controller
                    break;
                }
                case MARKLIN_REQUEST_TYPE::REVERSE_TRAIN:
                {
                    // uint8_t train_addr = m_req.id;
                    // cmd_buffer.Push('r');
                    // transmit_buffer.Push(train_addr);
                    break;
                }
                default:
                    break;
                }

                handle_transmission();
            }

            // this should fire when byte received since no FIFO
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
            }
            break;

            case IO_REQUEST_TYPE::TX_NOTIFIER:
            {
                uart_printf(CONSOLE, "MarklinIO_server::TX_NOTIFIER FIRED\r\n");
                if (initialized)
                {
                    if (!tx_state_machine.isTransactionInProgress())
                    {
                        tx_state_machine.BeginTransaction(STATES::TX_HIGH);
                    }
                    else
                    {
                        tx_state_machine.Transition(STATES::TX_HIGH);
                    }
                }
                else
                {
                    tx_state_machine.BeginTransaction(STATES::TX_HIGH);
                    initialized = true;
                }
                handle_transmission();
                // uart_printf(CONSOLE, CLEAR_TO_END_LINE "MarklinIO_server::TX_NOTIFIER FIRED\r\n");

                reply.type = REPLY_TYPE::SUCCESS;
                ReplyWithMessage(sender_tid, reply); // wake up notifier
            }
            break;

            case IO_REQUEST_TYPE::CTS_NOTIFIER:
            {
                if (initialized)
                {
                    tx_state_machine.Transition(req.data ? STATES::CTS_HIGH : STATES::CTS_LOW);
                }
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
        if (tx_state_machine.isTransactionInProgress() &&
            !tx_state_machine.isByteChosen() &&
            tx_state_machine.isCTS1_HIGH() &&
            !transmit_buffer.IsEmpty())
        {
            // can check the TYPE of command by checking cmd_buffer
            write_to_uart(); // write single byte from transmit buffer
            // use count to pop from cmd_buffer
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
        uart_printf(CONSOLE, COLOR_MAGENTA "MarklinIO_server::write_to_uart: Transmitting: {%d}\r\n", int(ch));
        uart_putc(MARKLIN, ch);
        // IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::write_to_uart: Transmitting: {%d}\r\n", CMD_LOCATION + 3, 1, int(ch));
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
            // int retval = AWAITEVENT(InterruptEvents::UART_MARKLIN_RX);
            // if (retval < 0)
            // {
            //     // uart_printf(CONSOLE, "PANIC, MARKLIN RX_NOTIFIER AWAITEVENT returned error %d\n", retval);
            //     spin_debug();
            // }
            // // // uart_printf(CONSOLE, "RTM NOTIFIER AWOKEN\r\n");

            // // don't need to pass marklin in these
            // IO_REQUEST req{IO_REQUEST_TYPE::RX_NOTIFIER, 0};
            // int reply;

            // SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
        }
    }

    void notifier_tx()
    {

        // uart_printf(CONSOLE, "M-TX Notifier started\r\n");
        while (true)
        {
            // // POLL and check if TX FIFO is empty
            // if (UART_REG(MARKLIN, UART_FR) & UART_FR_TXFE_MASK)
            // {
            //     // uart_printf(CONSOLE, "TX FIFO EMPTY\r\n");
            //     // perform action (notify)
            //     IO_REQUEST req{IO_REQUEST_TYPE::TX_NOTIFIER, 0};
            //     int reply;

            //     SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
            // }

            // this will be awoken when there is space in the transmit buffer (if fifo is enabled)
            AWAITEVENT(InterruptEvents::UART_MARKLIN_TX);
            uart_printf(CONSOLE, "TX ASSERTED\r\n");
        }
    }

    void notifier_cts()
    {
        int TID = MYTID();
        uart_printf(CONSOLE, "CTS Notifier started with TID: %d\r\n", TID);
        // THIS WILL FIRE WHEN ASSERTED/DEASSERTED
        // uart_printf(CONSOLE, "M-CTS Notifier started\r\n");

        // check CTS
        int CTS_SIGNAL = !(UART_REG(MARKLIN, UART_FR) & UART_FR_CTS_MASK);
        // uart_printf(CONSOLE, "CTS SIGNAL at notifier start: %d\r\n", CTS_SIGNAL);
        // uart_putc(MARKLIN, 'A');
        while (true)
        {
            // if (CTS_SIGNAL)
            // {
            //     // uart_printf(CONSOLE, "CTS SIGNAL HIGH\r\n");
            //     // spin_debug();
            // }
            // IO_REQUEST req{IO_REQUEST_TYPE::CTS_NOTIFIER, CTS_SIGNAL, nullptr};
            // int reply;

            // SEND(IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));

            // uart_printf(CONSOLE, "CTS NOTIFIER GOING TO SLEEP\r\n");
            CTS_SIGNAL = AWAITEVENT(InterruptEvents::UART_MARKLIN_CTS);
            uart_printf(CONSOLE, "CTS NOTIFIER AWOKEN with RETVAL: %d\r\n", CTS_SIGNAL);
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

    bool STATE_MACHINE::isTransactionInProgress()
    {
        return status == STATUS::ACTIVE;
    }

    bool STATE_MACHINE::BeginTransaction(STATES state)
    {
        if (status == STATUS::ACTIVE)
        {
            return false;
        }

        Reset();
        STATE_ARR[(int)state] = true;
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
                // IO_NS::Print(MOVE_CURSOR COLOR_MAGENTA CLEAR_TO_END_LINE "TX REASSERTED", CMD_LOCATION + 8, 1);
                // move_cursor(CONSOLE, CMD_LOCATION + 8, 1);
                uart_printf(CONSOLE, COLOR_MAGENTA "TX REASSERTED\r\n");
            }
            else if (state == STATES::CTS_HIGH)
            {
                idx = (int)STATES::CTS_HIGH2;
                STATE_ARR[idx] = true;
                // IO_NS::Print(MOVE_CURSOR COLOR_RED CLEAR_TO_END_LINE "CTS REASSERTED", CMD_LOCATION + 8, 1);
                // move_cursor(CONSOLE, CMD_LOCATION + 8, 1);
                uart_printf(CONSOLE, COLOR_YELLOW "CTS REASSERTED\r\n");
            }
            isTransactionComplete();
        }
        else
        {
            STATE_ARR[idx] = true;
        }
    }

    bool STATE_MACHINE::isTransactionComplete()
    {
        for (int i = 0; i < NUM_STATES; i++)
        {
            if (!STATE_ARR[i])
            {
                return false;
            }
        }
        Reset();
        uart_printf(CONSOLE, COLOR_RED "Transaction Complete\r\n");
        return true;
    }

    bool STATE_MACHINE::isByteChosen()
    {
        return STATE_ARR[(int)STATES::BYTE_CHOSEN];
    }

    bool STATE_MACHINE::isCTS1_HIGH()
    {
        return STATE_ARR[(int)STATES::CTS_HIGH];
    }

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }
}