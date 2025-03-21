#include "sensor.h"
#include "../include/uassert.h"
#include "../include/clock_server.h"
#include "../marklin/train.h"
namespace Sensors_NS
{

    static const char BANK_LABELS[NUM_BANKS] = {'A', 'B', 'C', 'D', 'E'};

    SensorManager::SensorManager()
        : UPDATE_DISPLAY(false), last_triggered_bank('\0'), last_triggered_id(0)
    {
        IO_NS::PrintTerminal("SensorManager::SensorManager: Initializing SensorManager\r\n");
        last_triggered_bank = '\0';
        last_triggered_id = 0;

        for (int bank = 0; bank < NUM_BANKS; ++bank)
        {
            for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor)
            {
                int idx = bank * SENSORS_PER_BANK + sensor;
                sensor_data[idx].bank = BANK_LABELS[bank];
                sensor_data[idx].id = sensor + 1;
                sensor_data[idx].status = SensorState::SEN_OFF;
            }
        }

        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        Reset(true); // send reset command to the marklin
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
        // MarklinRequest request = {COMMAND::READ_SENSOR, -1, -1, READ_ONE_SENSOR_BASE + bank_num};
        MARKLIN_IO_SERVER::MarklinRequest request = {true, READ_ONE_SENSOR_BASE + bank_num};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret >= 0 && "SensorManager::ReadBank: command sent to MarklinIOServer failed");

        // get response from marklin -- 2 bytes per bank
        char bytes[2];
        for (int i = 0; i < 2; ++i)
        {
            // get byte from marklin io
            bytes[i] = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(bytes[i] >= 0 && "SensorManager::ReadBank: Getc failed");
        }
        processSensorData(bank_num, bytes[0], bytes[1], bank_mask);
    }

    void SensorManager::ReadAll(int num_banks)
    {
        num_banks = VALIDATE_BANK(num_banks);

        MARKLIN_IO_SERVER::MarklinRequest request = {true, READ_ALL_SENSOR_BASE + num_banks};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret >= 0 && "SensorManager::ReadAll: command sent to MarklinIOServer failed");
        IO_NS::PrintTerminal("SensorManager::ReadAll: Sent command to MarklinIOServer\r\n");

        BANK_MASK bank_mask;
        for (int bank = 0; bank < num_banks; ++bank)
        {
            uint8_t byte1 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            // uassert(false && "Successfully Read Byte1");
            uassert(byte1 >= 0 && "SensorManager::ReadAll: Getc failed");
            uint8_t byte2 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            // uassert(false && "Successfully Read Byte2");
            uassert(byte2 >= 0 && "SensorManager::ReadAll: Getc failed");
            processSensorData(bank, byte1, byte2, &bank_mask);
            // uassert(false && "Successfully Read Bank");
        }
        // uassert(false && "Successfully Read All Banks");
    }

    void SensorManager::processSensorData(int bank, uint8_t byte1, uint8_t byte2, BANK_MASK *bank_mask)
    {
        if (byte1 > 0 || byte2 > 0)
        {
            IO_NS::PrintTerminal("SensorManager::processSensorData: BYTE1: %d, BYTE2: %d\r\n", byte1, byte2);
            uassert(false && "SensorManager::processSensorData -- forced panic");
        }
        for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor)
        {
            int idx = bank * SENSORS_PER_BANK + sensor;
            int new_state = (sensor < 8) ? SENSOR_TRIGGERED(byte1, sensor) : SENSOR_TRIGGERED(byte2, sensor - 8);

            int old_state = sensor_data[idx].status;
            sensor_data[idx].status = new_state;

            if (new_state == SEN_ON && old_state == SEN_OFF)
            {
                uassert(false && "SensorManager::processSensorData: Sensor triggered");
                uassert(bank_mask != nullptr && "SensorManager::processSensorData: bank_mask is null");
                bank_mask->sensor[sensor] = true;

                if (sensor_data[idx].bank != last_triggered_bank ||
                    sensor_data[idx].id != last_triggered_id)
                {

                    if (recent_sensors.IsFull())
                    {
                        int dummy;
                        recent_sensors.Pop(&dummy);
                    }
                    recent_sensors.Push(idx);

                    last_triggered_bank = sensor_data[idx].bank;
                    last_triggered_id = sensor_data[idx].id;
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
        for (int i = 0; i < MAX_RECENT_SENSORS; i++)
        {
            if (recent_sensors.IsEmpty())
            {
                return;
            }
            int idx;
            recent_sensors.Pop(&idx);
            temp_queue.Push(idx);

            int bank = idx / SENSORS_PER_BANK;
            int sensor = idx % SENSORS_PER_BANK;
            char bank_label = sensor_data[idx].bank;

            IO_NS::Print(COLOR_YELLOW MOVE_CURSOR "%c%d %s",
                         SENSOR_LOCATION + 3 + i, BOX_WIDTH + 5,
                         bank_label, sensor + 1,
                         sensor_data[idx].status == SEN_ON ? "ON" : "OFF");
        }
        while (!temp_queue.IsEmpty())
        {
            int idx;
            temp_queue.Pop(&idx);
            recent_sensors.Push(idx);
        }

        UPDATE_DISPLAY = false;
    }

    void SensorServer()
    {
        REGISTERAS("SensorServer");
        SensorManager sensors;

        struct SensorTaskPair
        {
            int tid;
            int sensor;
        };

        // create sensor ticker
        int sensor_ticker_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, SensorTicker);
        Queue<SensorTaskPair, NUM_TRAINS> awaiting_for_sensor[NUM_TRAINS];

        bool is_ticker_available = false;
        int sender_tid;
        SensorQuery query;
        while (true)
        {
            int retval = RECEIVE(&sender_tid, (char *)&query, sizeof(query));
            uassert(retval >= 0 && "SensorServer: Error receiving SensorQuery");

            switch (query.command)
            {
            case SENSOR_COMMAND::TICK:
            {
                IO_NS::PrintTerminal("SensorServer: Received TICK request -- READING ALL BANKS\r\n");
                is_ticker_available = true;
                sensors.ReadAll(NUM_BANKS);
                IO_NS::PrintTerminal("SensorServer: Finished reading all banks\r\n");
                break;
            }
            default:
                break;
            }

            sensors.Display(); // this can block the server until IO is ready

            if (is_ticker_available)
            {
                // uassert(false && "THIS SHOULD BE REPLYING!");
                IO_NS::PrintTerminal("SensorServer: Sending reply to sensor ticker\r\n");
                is_ticker_available = false;
                REPLY(sensor_ticker_tid, nullptr, 0);
            }
        }
    }

    // sensor notifier
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
            IO_NS::PrintTerminal("SensorTicker: Sent TICK request to SensorServer\r\n");
            DELAY(CLOCK_SERVER_TID, 10);
        }
    }
} // namespace Sensors_NS