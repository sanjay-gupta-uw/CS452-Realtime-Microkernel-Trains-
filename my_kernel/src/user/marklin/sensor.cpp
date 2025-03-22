#include "sensor.h"
#include "../include/uassert.h"
#include "../include/clock_server.h"
#include "../marklin/train.h"

#define USE_CTS 1

#define MAX_POLL_RETRIES 50

namespace Sensors_NS
{

    static const char BANK_LABELS[NUM_BANKS] = {'A', 'B', 'C', 'D', 'E'};

    SensorManager::SensorManager()
    {
        IO_NS::PrintTerminal("SensorManager::SensorManager: Initializing SensorManager\r\n");
        REGISTERAS("SensorServer");
        last_triggered_bank = '\0';
        last_triggered_id = 0;
        UPDATE_DISPLAY = false;

        for (int bank = 0; bank < NUM_BANKS; ++bank)
        {
            for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor)
            {
                int idx = (bank * SENSORS_PER_BANK) + sensor;
                sensor_data[idx].bank = BANK_LABELS[bank];
                sensor_data[idx].id = sensor + 1;
                sensor_data[idx].status = SensorState::SEN_OFF;
            }
        }

        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        SENSOR_TICKER_TID = -1;
        isTickerRunning = false;
        Reset(true); // send reset command to the marklin
        SensorLoop();
    }

    SensorManager::~SensorManager()
    {
    }

    void SensorManager::Reset(bool reset_on)
    {
        MARKLIN_IO_SERVER::MarklinRequest request = {true, reset_on ? RESET_MODE_ON : RESET_MODE_OFF};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret >= 0 && "SensorManager::Reset: command sent to MarklinIOServer failed");
    }

    void SensorManager::ReadBank(int bank_num, BANK_MASK *bank_mask)
    {
        // invalidate bank_mask
        for (int i = 0; i < SENSORS_PER_BANK; ++i)
        {
            bank_mask->sensor[i] = false;
        }
        bank_num = VALIDATE_BANK(bank_num);

        // send command to marklin
        if (USE_CTS)
        {
            MARKLIN_IO_SERVER::MarklinRequest request = {true, READ_ONE_SENSOR_BASE + bank_num};
            int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
            uassert(ret >= 0 && "SensorManager::ReadBank: FAILED to command sent to MarklinIOServer");
        }
        else
        {
            uart_putc(MARKLIN, READ_ONE_SENSOR_BASE + bank_num);
            // uassert(false && "SensorManager::ReadBank: Sent command to Marklin");
        }

        // get response from marklin -- 2 bytes per bank
        char bytes[2];
        for (int i = 0; i < 2; ++i)
        {
            // get byte from marklin io
            // bytes[i] = uart_getc(MARKLIN);
            if (USE_CTS)
            {
                bytes[i] = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            }
            else
            {
                bytes[i] = uart_getc(MARKLIN);
            }
            // uassert(false && "SensorManager::ReadBank: Successfully Read Byte");
            uassert(bytes[i] >= 0 && "SensorManager::ReadBank: Getc failed");
        }
        processSensorData(bank_num, bytes[0], bytes[1], bank_mask);
    }

    int SensorManager::ReadAll(int num_banks, BANK_MASK *bank_mask)
    {
        num_banks = VALIDATE_BANK(num_banks);

        MARKLIN_IO_SERVER::MarklinRequest request = {true, READ_ALL_SENSOR_BASE + num_banks};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret >= 0 && "SensorManager::ReadAll: command sent to MarklinIOServer failed");
        IO_NS::PrintTerminal("SensorManager::ReadAll: Sent command to MarklinIOServer\r\n");
        int triggered_bank = -1;

        for (int bank = 1; bank <= num_banks; ++bank)
        {
            uint8_t byte1 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            // uassert(false && "Successfully Read Byte1");
            uassert(byte1 >= 0 && "SensorManager::ReadAll: Getc failed");
            uint8_t byte2 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            // uassert(false && "Successfully Read Byte2");
            uassert(byte2 >= 0 && "SensorManager::ReadAll: Getc failed");

            if (byte1 == 0 && byte2 == 0)
            {
                continue;
            }
            processSensorData(bank, byte1, byte2, bank_mask);
            triggered_bank = bank;

            // uassert(false && "Successfully Read Bank");
        }
        return triggered_bank;
        // uassert(false && "Successfully Read All Banks");
    }

    void SensorManager::processSensorData(int bank, uint8_t byte1, uint8_t byte2, BANK_MASK *bank_mask)
    {
        if (byte1 == 0 && byte2 == 0)
        {
            return;
        }
        IO_NS::PrintTerminal("SensorManager::processSensorData: byte1: %d, byte2: %d\r\n", byte1, byte2);
        for (int sensor = 1; sensor <= SENSORS_PER_BANK; ++sensor)
        {
            int idx = ((bank - 1) * SENSORS_PER_BANK) + (sensor - 1);
            int new_state = (sensor < 9) ? SENSOR_TRIGGERED(byte1, sensor) : SENSOR_TRIGGERED(byte2, sensor);
            IO_NS::PrintTerminal("NEW STATE: %d\r\n", new_state);

            int old_state = sensor_data[idx].status;
            sensor_data[idx].status = new_state;

            if (new_state == SEN_ON && old_state == SEN_OFF)
            {
                uassert(bank_mask != nullptr && "SensorManager::processSensorData: bank_mask is null");

                if (sensor_data[idx].bank != last_triggered_bank ||
                    sensor_data[idx].id != last_triggered_id)
                {

                    if (recent_sensors.IsFull())
                    {
                        int dummy;
                        recent_sensors.Pop(&dummy);
                        IO_NS::PrintTerminal("SensorManager::processSensorData: recent sensors is full -- making space\r\n");
                    }
                    recent_sensors.Push(idx);

                    last_triggered_bank = sensor_data[idx].bank;
                    last_triggered_id = sensor_data[idx].id;

                    bank_mask->sensor[sensor - 1] = true;
                    UPDATE_DISPLAY = true;
                }
            }
        }
    }

    void SensorManager::Display()
    {
        if (!UPDATE_DISPLAY)
            return;

        Queue<int, MAX_MAX_RECENT_SENSORS> temp_queue;
        int count = 0;
        while (!recent_sensors.IsEmpty())
        {
            int idx;
            recent_sensors.Pop(&idx);
            temp_queue.Push(idx);

            IO_NS::Print(COLOR_YELLOW MOVE_CURSOR "%c%d tripped  ",
                         SENSOR_LOCATION + 3 + count++, BOX_WIDTH + 5,
                         sensor_data[idx].bank, sensor_data[idx].id);
        }

        while (!temp_queue.IsEmpty())
        {
            int idx;
            temp_queue.Pop(&idx);
            recent_sensors.Push(idx);
        }

        UPDATE_DISPLAY = false;
    }

    void SensorManager::SensorLoop()
    {
        // create sensor ticker
        isTickerRunning = true;
        SENSOR_TICKER_TID = CREATE(PRIORITY::DEVICE_NOTIFIER, SensorTicker);

        int sender_tid;
        SensorQuery query;

        // test SENSOR_TRIGGERED
        // {
        //     IO_NS::PrintTerminal("SensorServer: Testing SENSOR_TRIGGERED\r\n");

        //     uint8_t sensor_bytes[16] = {
        //         0b10000000,
        //         0b01000000,
        //         0b00100000,
        //         0b00010000,
        //         0b00001000,
        //         0b00000100,
        //         0b00000010,
        //         0b00000001,
        //         0b10000000,
        //         0b01000000,
        //         0b00100000,
        //         0b00010000,
        //         0b00001000,
        //         0b00000100,
        //         0b00000010,
        //         0b00000001};

        //     for (int i = 1; i <= 16; ++i)
        //     {
        //         IO_NS::PrintTerminal("SensorServer: Testing sensor %d\r\n", i);
        //         uassert(SENSOR_TRIGGERED(sensor_bytes[i - 1], i) && "SensorServer: Error testing SENSOR_TRIGGERED");
        //     }

        //     IO_NS::PrintTerminal("SensorServer: Finished testing SENSOR_TRIGGERED\r\n");
        // }

        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&query, sizeof(query));
            uassert(retval >= 0 && "SensorServer: Error receiving SensorQuery");

            switch (query.command)
            {
            case SENSOR_COMMAND::READ_ALL:
            case SENSOR_COMMAND::READ_BANK:
            {
                // TRAIN must have sent this for initialization
                // IO_NS::PrintTerminal("SensorServer: Received READ_ALL request -- READING ALL BANKS\r\n");
                train_requests.Push({sender_tid, query, 0});
            }

            case SENSOR_COMMAND::TICK:
            {
                IO_NS::PrintTerminal("SensorServer: Received TICK request -- READING ALL BANKS\r\n");
                isTickerRunning = false;
                break;
            }
            default:
                break;
            }

            HandleSensorQuery();

            Display(); // this can block the server until IO is ready
        }
    }

    // this returns true if the train is freed
    bool SensorManager::HandleReadAll(WaitingTrain *train, BANK_MASK *bank_mask)
    {
        // need to make a bank mask for this call
        int triggered_bank = ReadAll(NUM_BANKS, bank_mask);

        // check if bank mask has any sensors
        if (triggered_bank < 0)
        {
            return false;
        }

        for (int i = 0; i < SENSORS_PER_BANK; ++i)
        {
            if (bank_mask->sensor[i])
            {
                // send reply to train
                SensorResponse response = {{BANKS(triggered_bank), i + 1}, true};
                REPLY(train->train_tid, (char *)&response, sizeof(SensorResponse));
                break;
            }
        }
        return true;
    }

    bool SensorManager::HandleReadBank(WaitingTrain *train, BANK_MASK *bank_mask)
    {
        ReadBank(int(train->query.sensor.bank), bank_mask);
        // check if the train is waiting for the sensor
        if (bank_mask->sensor[train->query.sensor.id - 1])
        {
            // send reply to train
            SensorResponse response = {train->query.sensor, true};
            REPLY(train->train_tid, (char *)&response, sizeof(SensorResponse));
            return true;
        }
        return false;
    }

    void SensorManager::HandleSensorQuery()
    {
        Queue<WaitingTrain, NUM_TRAINS> temp_queue;
        while (!train_requests.IsEmpty())
        {
            WaitingTrain train;
            train_requests.Pop(&train);
            if (train.poll_count >= MAX_POLL_RETRIES)
            {
                IO_NS::PrintTerminal("SensorManager::HandleSensorQuery: MAX_POLL_RETRIES reached\r\n");
                // send reply to train
                SensorResponse response = {train.query.sensor, false};
                REPLY(train.train_tid, (char *)&response, sizeof(SensorResponse));
                continue;
            }

            bool freed_train = false;

            BANK_MASK bank_mask;
            if (train.query.command == SENSOR_COMMAND::READ_ALL)
            {
                // need to make a bank mask for this call
                freed_train = HandleReadAll(&train, &bank_mask);
            }
            else
            {
                freed_train = HandleReadBank(&train, &bank_mask);
            }

            if (!freed_train)
            {
                train.poll_count++;
                temp_queue.Push(train);
            }
        }
        // put back the trains that are still waiting
        while (!temp_queue.IsEmpty())
        {
            WaitingTrain train;
            temp_queue.Pop(&train);
            train_requests.Push(train);
        }

        if (!isTickerRunning && !train_requests.IsEmpty())
        {
            IO_NS::PrintTerminal("SensorServer: Sending reply to sensor ticker\r\n");
            isTickerRunning = true;
            REPLY(SENSOR_TICKER_TID, nullptr, 0);
        }
    }

    void SensorServer()
    {
        Sensors_NS::SensorManager sensors;
    }

    // sensor notifier -- sends a tick to the sensor server every 200ms
    void SensorTicker()
    {
        REGISTERAS("SensorTicker");
        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "SensorTicker: Error finding SensorServer");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "SensorTicker: Error finding ClockServer");

        SensorQuery query = {SENSOR_COMMAND::TICK};
        while (true)
        {
            int retval = SEND(sensor_server_tid, (char *)&query, sizeof(query), nullptr, 0);
            uassert(retval >= 0 && "SensorTicker: Error sending SensorQuery to SensorServer");
            IO_NS::PrintTerminal("SensorTicker: DELAYING\r\n");
            DELAY(CLOCK_SERVER_TID, 20);
        }
    }
} // namespace Sensors_NS