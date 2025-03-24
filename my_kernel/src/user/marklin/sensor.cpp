#include "sensor.h"
#include "../include/uassert.h"
#include "../include/clock_server.h"
#include "../marklin/train.h"

#define USE_CTS 1

#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif

namespace Sensors_NS
{
    static const char BANK_LABELS[NUM_BANKS] = {'A', 'B', 'C', 'D', 'E'};

    static void test_sensor_masks()
    {
        // test SENSOR_TRIGGERED
        {
            IO_NS::PrintTerminal("SensorServer: Testing SENSOR_TRIGGERED\r\n");

            uint8_t sensor_bytes[16] = {
                0b10000000,
                0b01000000,
                0b00100000,
                0b00010000,
                0b00001000,
                0b00000100,
                0b00000010,
                0b00000001,
                0b10000000,
                0b01000000,
                0b00100000,
                0b00010000,
                0b00001000,
                0b00000100,
                0b00000010,
                0b00000001};

            for (int i = 1; i <= 16; ++i)
            {
                IO_NS::PrintTerminal("SensorServer: Testing sensor %d\r\n", i);
                uassert(SENSOR_TRIGGERED(sensor_bytes[i - 1], i) && "SensorServer: Error testing SENSOR_TRIGGERED");
            }

            IO_NS::PrintTerminal("SensorServer: Finished testing SENSOR_TRIGGERED\r\n");
        }
    }

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
                sensor_data[idx].status = SEN_OFF;
            }
        }

        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        SENSOR_TICKER_TID = -1;
        isTickerRunning = false;
        Reset(true); // send reset command to the marklin
        SensorLoop();

        int train_nums[NUM_TRAINS] = {1, 54, 55, 58, 77};

        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            waiting_trains[i] = {-1, train_nums[i], -1, -1};
            waiting_trains[i].messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, SensorMessenger);
            uassert(waiting_trains[i].messenger_tid > 0 && "SensorManager::SensorManager: Error creating SensorMessenger");
            SEND(waiting_trains[i].messenger_tid, (char *)&waiting_trains[i].train_tid, sizeof(int), nullptr, 0);
        }
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

    void SensorManager::ReadAll(int num_banks)
    {
        num_banks = VALIDATE_BANK(num_banks);

        MARKLIN_IO_SERVER::MarklinRequest request = {true, READ_ALL_SENSOR_BASE + num_banks};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret >= 0 && "SensorManager::ReadAll: command sent to MarklinIOServer failed");
        IO_NS::PrintTerminal("SensorManager::ReadAll: Sent command to MarklinIOServer\r\n");

        for (int bank = 1; bank <= num_banks; ++bank)
        {
            uint8_t byte1 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(byte1 >= 0 && "SensorManager::ReadAll: Getc failed");
            uint8_t byte2 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(byte2 >= 0 && "SensorManager::ReadAll: Getc failed");

            if (byte1 == 0 && byte2 == 0)
            {
                continue;
            }
            processSensorData(bank, byte1, byte2);
        }
    }

    void SensorManager::processSensorData(int bank, uint8_t byte1, uint8_t byte2)
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

                    UPDATE_DISPLAY = true;
                }
                sensor_data[idx].status = SEN_ON;
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
        if (IRQ_ENABLED)
        {
            SENSOR_TICKER_TID = CREATE(PRIORITY::DEVICE_NOTIFIER, SensorTicker);
            uassert(SENSOR_TICKER_TID > 0 && "SensorManager::SensorLoop: Error creating SensorTicker");
        }

        int sender_tid;
        SensorQuery query;

        test_sensor_masks();
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "SensorServer: Error finding ClockServer");

        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&query, sizeof(query));
            uassert(retval >= 0 && "SensorServer: Error receiving SensorQuery");
            IO_NS::PrintTerminal("SensorServer: Received SensorQuery from %d\r\n", sender_tid);
            cur_tick = TIME(CLOCK_SERVER_TID);

            switch (query.command)
            {
            case SENSOR_COMMAND::TICK:
            {
                IO_NS::PrintTerminal("SensorServer: Received TICK request -- READING ALL BANKS\r\n");
                break;
            }
            case SENSOR_COMMAND::RESET:
            {
                // IO_NS::PrintTerminal("SensorServer: Received RESET request\r\n");
                Reset(true);
                break;
            }
            case SENSOR_COMMAND::TRAIN_SENSOR:
            {
                // train expects to hit a sensor in the near future
                IO_NS::PrintTerminal("SensorServer: Received TRAIN_SENSOR request\r\n");
                SensorStruct sensor = query.sensor;
                int idx = (int(sensor.bank) * SENSORS_PER_BANK) + (sensor.id - 1); // convert sensor to bank and id to compute index
                for (int i = 0; i < NUM_TRAINS; ++i)
                {
                    if (waiting_trains[i].train_num == query.train_num)
                    {
                        waiting_trains[i] = {query.train_tid, query.train_num, idx};
                        break;
                    }
                }
            }
            default:
                break;
            }

            if (IRQ_ENABLED)
            {
                ReadAll(NUM_BANKS);
            }
            HandleWaitingTrains();

            Display(); // this can block the server until IO is ready
            REPLY(sender_tid, nullptr, 0);
        }
    }

    void SensorManager::HandleWaitingTrains()
    {
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            WaitingTrain *train = &waiting_trains[i];
            if (train->idx >= 0 && sensor_data[train->idx].status == SEN_ON)
            {
                train->idx = -1;
                SensorResponse response = {true, cur_tick};
                int retval = REPLY(train->messenger_tid, (char *)&response, sizeof(SensorResponse));
                uassert(retval >= 0 && "SensorManager::HandleWaitingTrains: Error replying to train");
            }
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

    void SensorMessenger()
    {
        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "SensorMessenger: Error finding SensorServer");

        int train_tid;
        int sender_tid;
        int retval = RECEIVE(&sender_tid, (char *)&train_tid, sizeof(train_tid));
        uassert(retval >= 0 && "SensorMessenger: Error receiving train_tid");
        uassert(sender_tid == sensor_server_tid && "SensorMessenger: Received params from invalid src");

        SensorQuery sensor_query = {SENSOR_COMMAND::MESSENGER_READY};
        while (true)
        {
            SensorResponse response;
            int ret = SEND(sensor_server_tid, (char *)&sensor_query, sizeof(sensor_query), (char *)&response, sizeof(response));
            uassert(ret >= 0 && "SensorMessenger: Error sending to SensorServer");

            // send reply to train task
            ret = SEND(train_tid, (char *)&response, sizeof(SensorResponse), nullptr, 0);
            uassert(ret >= 0 && "SensorMessenger: Error replying to train task");
        }
    }
} // namespace Sensors_NS
