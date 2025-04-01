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
    Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK];
    Queue<int, 20> sensor_change_buffer;
    uint32_t cur_tick;

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

    static void processSensorData(char bank, uint8_t byte1, uint8_t byte2)
    {
        if (byte1 == 0 && byte2 == 0)
        {
            return;
        }
        // uassert(false && "PROCESSING SENSOR DATA -- THIS IS GETTING HIT");
        // IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager::processSensorData: byte1: %d, byte2: %d\r\n", byte1, byte2);
        for (int sensor = 1; sensor <= SENSORS_PER_BANK; ++sensor)
        {
            int idx = ((int)(bank - 'A') * SENSORS_PER_BANK) + (sensor - 1);
            int new_state = (sensor < 9) ? SENSOR_TRIGGERED(byte1, sensor) : SENSOR_TRIGGERED(byte2, sensor);

            int old_state = sensor_data[idx].isTriggered;
            if (old_state != new_state)
            {
                sensor_change_buffer.Push(idx);
                sensor_data[idx].isTriggered = new_state;

                // update tick that sensor was triggered
                if (new_state == SEN_ON)
                {
                    sensor_data[idx].trigger_tick = cur_tick;
                }
            }
        }
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

        IO_NS::PrintTerminal("MarklinIOServer started with TID %d, initializing with reset on!\r\n", MYTID());
        sequence_length_buffer.Push(1);
        transmit_buffer.Push(RESET_MODE_ON);

        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            sensor_messenger[i].messenger_id = -1;
            sensor_messenger[i].train_num = -1;
            sensor_messenger[i].sensor_idx = -1;
            sensor_messenger[i].sent_reply = false;
        }
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

        for (int i = 0; i < NUM_BANKS * SENSORS_PER_BANK; ++i)
        {
            sensor_data[i].isTriggered = false;
            sensor_data[i].bank = 'A' + (i / SENSORS_PER_BANK);
            sensor_data[i].id = (i % SENSORS_PER_BANK) + 1;
            sensor_data[i].trigger_tick = 0;
        }

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
                for (int i = 0; i < 5; ++i)
                {
                    processSensorData(char('A' + i), receive_buffer.bytes[i * 2], receive_buffer.bytes[i * 2 + 1]);
                }

                IO_REQUEST req(&receive_buffer);
                SEND(MARKLIN_IO_TID, (char *)&req, sizeof(req), (char *)&reply, sizeof(reply));
                // // uassert(false && "SERVER REPLIED TO RX NOTIFIER");
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

    int get_messenger_index(int train_num)
    {
        const int train_nums[NUM_TRAINS] = {1, 54, 55, 58, 77};
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            if (train_nums[i] == train_num)
            {
                return i;
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
            cur_tick = TIME(CLOCK_SERVER_TID);
            IO_NS::PrintTerminal(COLOR_CYAN "MarklinIO_server::run: Received request from tid{%d} at tick %d\r\n", sender_tid, cur_tick);

            IO_REPLY reply;
            reply.type = REPLY_TYPE::FAILURE;
            reply.ch = UNDEFINED_CHAR;
            bool send_reply = false;

            switch (req.type)
            {
            case IO_REQUEST_TYPE::RX_NOTIFIER:
            {
                isReceiving = false;
                send_reply = true;

                // check the global receive buffer to see if any messenger should be notified
                for (int i = 0; i < NUM_TRAINS; ++i)
                {
                    MessengerUnit *messenger = &sensor_messenger[i];
                    if (messenger->messenger_id > 0 && messenger->sent_reply == false)
                    {
                        IO_NS::PrintTerminal("CHECKING IF SENSOR IDX %d is TRIGGERED\r\n", messenger->sensor_idx);

                        // check if the sensor is triggered
                        Sensor *sensor = &sensor_data[messenger->sensor_idx];
                        if (sensor->isTriggered == SEN_ON)
                        {
                            // send reply to the messenger
                            IO_NS::PrintTerminal("MarklinIO_server::run: Sending reply to sensor messenger %d\r\n", messenger->messenger_id);
                            REPLY(messenger->messenger_id, (char *)&reply, sizeof(reply));
                            messenger->sent_reply = true;
                        }
                    }
                }
            }

            case IO_REQUEST_TYPE::SENSOR_LISTENER:
            {

                // ADD TO SENSOR LISTENER QUEUE
                // message must have train_num and sensor name
                // int idx = get_messenger_index(req.data.sensor_listener
                SensorListener *sensor_listener = req.data.sensor_listener;
                int train_num = sensor_listener->train_num;
                int messenger_idx = get_messenger_index(train_num);
                uassert(messenger_idx >= 0 && "MarklinIO_server::run: -- Invalid train number");

                // IO_NS::PrintTerminal("Getting sensor data from idx %d\r\n", train_index);
                // set sensor_messenger
                MessengerUnit *messenger = &sensor_messenger[messenger_idx];
                if (messenger->messenger_id < 0)
                {
                    messenger->messenger_id = CREATE(PRIORITY::MARKLIN_NOTIFIER, start_sensor_messenger);
                    SEND(messenger->messenger_id, (char *)&train_num, sizeof(int), nullptr, 0);
                }

                IO_NS::PrintTerminal("MarklinIO_server::run: Received sensor listener request from tid{%d} for train %d, sensor %s\r\n", sender_tid, train_num, sensor_listener->sensor_name);
                send_reply = true;
                break;
            }
            case IO_REQUEST_TYPE::SENSOR_MESSENGER:
            {

                IO_NS::PrintTerminal(COLOR_RED "MarklinIO_server::run: Received sensor messenger request from tid{%d} \r\n", sender_tid);
                for (int i = 0; i < NUM_TRAINS; ++i)
                {
                    MessengerUnit *messenger = &sensor_messenger[i];

                    if (messenger->messenger_id == sender_tid)
                    {
                        IO_NS::PrintTerminal(COLOR_RED "MarklinIO_server::run: -- Sensor messenger found\r\n");
                        messenger->sent_reply = false;
                        break;
                    }
                }
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
                // IO_NS::PrintTerminal("MarklinIO_server::SEND_CMD:: Received command from tid{%d}\r\n", sender_tid);
                MarklinRequest *m_req = req.data.request;
                transmit_buffer.Push(m_req->byte1);
                int len = 2;
                if (!m_req->isSingleByteCommand)
                {
                    int addr = m_req->byte2;

                    transmit_buffer.Push(addr);
                }
                else
                {
                    uassert(false && "MarklinIO_server::SEND_CMD: -- Single byte command not implemented");
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

            while (!sensor_change_buffer.IsEmpty())
            {
                int idx;
                sensor_change_buffer.Pop(&idx);
                Sensor *sensor = &sensor_data[idx];
                int column = 0;
                switch (sensor->bank)
                {
                case 'A':
                    column = BANK_A_COL;
                    break;
                case 'B':

                    column = BANK_B_COL;
                    break;
                case 'C':
                    column = BANK_C_COL;
                    break;
                case 'D':
                    column = BANK_D_COL;
                    break;
                case 'E':
                    column = BANK_E_COL;
                    break;
                default:
                    IO_NS::PrintTerminal("Invalid bank: %c\r\n", sensor->bank);
                    return;
                }

                if (sensor->isTriggered == SEN_ON)
                {
                    // IO_NS::PrintTerminal(COLOR_GREEN "Sensor %c%d triggered\r\n", sensor->bank, sensor->id);
                    IO_NS::Print(COLOR_YELLOW MOVE_CURSOR "%d", SENSOR_START_ROW + sensor->id - 1, column, sensor->isTriggered);
                }
                else
                {
                    // IO_NS::PrintTerminal(COLOR_RED "Sensor %c%d untriggered\r\n", sensor->bank, sensor->id);
                    IO_NS::Print(COLOR_RED MOVE_CURSOR "%d", SENSOR_START_ROW + sensor->id - 1, column, sensor->isTriggered);
                }
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
        if (transmit_buffer.IsEmpty())
        {
            // IO_NS::PrintTerminal(COLOR_RED "MarklinIOServer::handle_transmission: transmit buffer is empty -- Polling Sensors\r\n");
            sequence_length_buffer.Push(1);
            transmit_buffer.Push(READ_ALL_SENSOR_BASE + NUM_BANKS);
            BYTES_RECEIVED = 0;
            TOTAL_BYTES_TO_RECEIVE = NUM_BANKS * 2; // 2 bytes per bank
            isReceiving = true;
        }

        // IO_NS::PrintTerminal("MarklinIOServer::handle_transmission: Transmitting\r\n");
        IO_REPLY reply = {REPLY_TYPE::SUCCESS, UNDEFINED_CHAR};
        REPLY(tx_notifier_tid, (char *)&reply, sizeof(reply));

        if (count == 0)
        {
            sequence_length_buffer.Pop(&sequence_length);
        }

        unsigned char ch;
        transmit_buffer.Pop(&ch);
        uart_putc_non_blocking(MARKLIN, ch);

        canTransmit = false;
        total_bytes_transmitted++;
        count++;

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
        }

        IO_NS::Print(MOVE_CURSOR COLOR_GREEN "%d", TRANSMITTED_BYTES_LOCATION, TRANSMITTED_BYTES_COL, total_bytes_transmitted);
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

    // SENSOR/CONDUCTOR MESSENGER
    void start_sensor_messenger()
    {
        int my_tid = MYTID();
        int marklin_io_tid;
        int train_num;

        int param_init_retval = RECEIVE(&marklin_io_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && marklin_io_tid > 0 && train_num > 0 && "PATH MESSENGER: Error receiving train_num from parent task");
        REPLY(marklin_io_tid, nullptr, 0);

        int conductor_tid = WHOIS("Conductor");
        uassert(conductor_tid > 0 && "SENSOR MESSENGER: Error finding Conductor");

        IO_NS::PrintTerminal(COLOR_YELLOW "MARKLIN_IO::SENSOR_MESSENGER{%d/%d} spawned with TID %d\r\n", my_tid, train_num, my_tid);

        IO_REQUEST req(IO_REQUEST_TYPE::SENSOR_MESSENGER);
        SensorTriggerResponse sensor_triggered_struct;
        while (true)
        {
            IO_NS::PrintTerminal(COLOR_YELLOW "MARKLIN_IO::SENSOR_MESSENGER{%d/%d}:: waiting to be released by marklin server\r\n", my_tid, train_num);
            int retval = SEND(marklin_io_tid, (char *)&req, sizeof(req), (char *)&sensor_triggered_struct, sizeof(SensorTriggerResponse));
            uassert(retval >= 0 && "PATH MESSENGER: Error sending SegmentRequest to Conductor");

            IO_NS::PrintTerminal("SENSOR MESSENGER{%d/%d}:: sensor triggered: %d, at tick %d\r\n", my_tid, train_num, sensor_triggered_struct.is_triggered, sensor_triggered_struct.trigger_tick);
            // SEND TO CONDUCTOR
            ConductorRequest conductor_request(&sensor_triggered_struct);
            IO_NS::PrintTerminal(COLOR_YELLOW "SENSOR MESSENGER{%d/%d}:: notifying Conductor\r\n", my_tid, train_num);
            retval = SEND(conductor_tid, (char *)&conductor_request, sizeof(ConductorRequest), nullptr, 0);
            uassert(retval >= 0 && "SENSOR MESSENGER: Error sending SensorTriggerResponse to Conductor");
            IO_NS::PrintTerminal(COLOR_YELLOW "SENSOR MESSENGER{%d/%d}:: notified Conductor\r\n", my_tid, train_num);
        }
    }
}
