#include "../include/marklin_io.h"
#include "../include/name_server.h"
#include "../include/clock_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"

#include "../include/io.h"
#include "../../rpi.h"
#include "../../clock.h"
#include "../marklin/train.h"

extern Clock clock;
namespace MARKLIN_IO_SERVER
{

    char COMMANDS[] = {'s', 't', 'r', 'D'};
    static int IO_SERVER_TID;
    static int CLOCK_SERVER_TID;

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
        // spawnNotifiers();
        run();
    }

    MarklinIOServer::~MarklinIOServer()
    {
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
                // reply.type = REPLY_TYPE::FAILURE; // incase it's full
                // if (!transmit_buffer.IsFull())    // this will ignore characters if "overwhelmed"
                // {
                //     transmit_buffer.Push((uint8_t)(req.data));
                //     reply.type = REPLY_TYPE::SUCCESS;
                // }
                // handle_transmission();

                // ReplyWithMessage(sender_tid, reply); // sends UNIMPLEMENTED
            }
            break;
            case IO_REQUEST_TYPE::SEND_CMD:
            {
                // IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD\r\n", CMD_LOCATION + 2, 1);
                MarklinRequest m_req = *(req.request);
                switch (m_req.type)
                {
                case MARKLIN_REQUEST_TYPE::SET_SWITCH:
                {
                    uint8_t switch_addr = m_req.id;
                    unsigned char state = m_req.data;

                    cmd_buffer.Push(COMMANDS[0]);
                    transmit_buffer.Push((state == 'S') ? STRAIGHT_CMD : CURVED_CMD);
                    transmit_buffer.Push(switch_addr);

                    transmit_buffer.Push(OFF);
                    transmit_buffer.Push(switch_addr);

                    IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Setting switch %d to state %c\r\n", CMD_LOCATION + 3, 1, switch_addr, state);
                    reply.type = REPLY_TYPE::SUCCESS;
                    REPLY(sender_tid, (char *)&reply, sizeof(reply)); // wakeup marklin-controller
                }
                break;
                case MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN:
                {
                    uint8_t train_addr = m_req.id;
                    uint8_t speed = m_req.data;
                    cmd_buffer.Push(COMMANDS[1]);
                    transmit_buffer.Push(speed);
                    transmit_buffer.Push(train_addr);
                    IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Accelerating train %d to speed %d\r\n", CMD_LOCATION + 3, 1, train_addr, speed);
                    // wake up notifier
                    reply.type = REPLY_TYPE::SUCCESS;
                    REPLY(sender_tid, (char *)&reply, sizeof(reply)); // wakeup marklin-controller
                }
                break;
                case MARKLIN_REQUEST_TYPE::REVERSE_TRAIN:
                {
                    uint8_t train_addr = m_req.id;
                    // first need to check if train is moving
                    int cur_speed = Trains_NS::getSpeed(train_addr);

                    if (cur_speed > 0)
                    {
                        // need to stop train first
                        MarklinRequest stop_request = {MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN, train_addr, -1, 0};
                        SendCmd(IO_SERVER_TID, &stop_request);
                        transmit_buffer.Push((unsigned char)100);
                        cmd_buffer.Push(COMMANDS[3]); // DELAY
                    }
                    cmd_buffer.Push(COMMANDS[2]);
                    transmit_buffer.Push(REVERSE_CMD);
                    transmit_buffer.Push(train_addr);
                    IO_NS::Print(COLOR_MAGENTA MOVE_CURSOR CLEAR_TO_END_LINE "MarklinIO_server::SEND_CMD: Reversing train %d\r\n", CMD_LOCATION + 3, 1, train_addr);
                    // wake up notifier
                    reply.type = REPLY_TYPE::SUCCESS;

                    transmit_buffer.Push(cur_speed);                  // send the speed back
                    transmit_buffer.Push(train_addr);                 // send the train address back
                    REPLY(sender_tid, (char *)&reply, sizeof(reply)); // wakeup marklin-controller
                }
                break;
                default:
                    break;
                } // end switch (m_req.type)

                handle_transmission();

            } // end case IO_REQUEST_TYPE::SEND_CMD

            default:
                break;
            }
        }
    }

    void MarklinIOServer::handle_transmission()
    {
        if (transmit_buffer.IsEmpty())
        {
            return;
        }
        unsigned char cmd_type;
        cmd_buffer.Pop(&cmd_type);
        switch (cmd_type)
        {
            // use clock server to delay
        case 's':
            // switch
            write_to_uart();
            write_to_uart();
            DELAY(CLOCK_SERVER_TID, 10);
            // need to send OFF command after 10 ticks
            write_to_uart();
            write_to_uart();
            break;
        case 't':
        case 'r':
            write_to_uart();
            write_to_uart();
            DELAY(CLOCK_SERVER_TID, 3);
            break;
        case 'D':
            // delay
            DELAY(CLOCK_SERVER_TID, 10);
            break;
        default:
            break;
        }

        handle_transmission();
    }

    // can only send byte
    void MarklinIOServer::write_to_uart()
    {
        // // uart_printf(CONSOLE, "MarklinIO_server::write_to_uart\r\n");
        unsigned char ch = UNDEFINED_CHAR;
        transmit_buffer.Pop(&ch);
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

    void startMarklinIOServer()
    {
        MARKLIN_IO_SERVER::MarklinIOServer marklin_io_server;
    }
}