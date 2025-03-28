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
            IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager: Testing SENSOR_TRIGGERED\r\n");

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
                IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager: Testing sensor %d\r\n", i);
                uassert(SENSOR_TRIGGERED(sensor_bytes[i - 1], i) && "SensorManager: Error testing SENSOR_TRIGGERED");
            }

            IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager: Finished testing SENSOR_TRIGGERED\r\n");
        }
    }

    SensorManager::SensorManager()
    {
        IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager::SensorManager: Initializing SensorManager\r\n");
        REGISTERAS("SensorManager");
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
        CONDUCTOR_TID = WHOIS("Conductor");
        uassert(CONDUCTOR_TID > 0 && "Jack----------Sensor: Conductor not found");
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

    void SensorManager::ReadAll(int num_banks)
    {
        num_banks = VALIDATE_BANK(num_banks);

        IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager::ReadAll: Sending command to MarklinIOServer\r\n");
        MARKLIN_IO_SERVER::MarklinRequest request = {true, READ_ALL_SENSOR_BASE + num_banks};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret >= 0 && "SensorManager::ReadAll: command sent to MarklinIOServer failed");
        IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager::ReadAll: Sent command to MarklinIOServer\r\n");

        for (int bank = 1; bank <= num_banks; ++bank)
        {
            uint8_t byte1 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(byte1 >= 0 && "SensorManager::ReadAll: Getc failed");
            IO_NS::PrintTerminal("SensorManager::ReadAll: Read byte1: %d\r\n", byte1);
            uint8_t byte2 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(byte2 >= 0 && "SensorManager::ReadAll: Getc failed");
            IO_NS::PrintTerminal("SensorManager::ReadAll: Read byte2: %d\r\n", byte2);

            if (byte1 == 0 && byte2 == 0)
            {
                continue;
            }
            processSensorData(bank, byte1, byte2);
        }
        IO_NS::PrintTerminal(COLOR_RED "SensorManager::ReadAll: Finished reading all banks\r\n");
    }

    void SensorManager::processSensorData(int bank, uint8_t byte1, uint8_t byte2)
    {
        if (byte1 == 0 && byte2 == 0)
        {
            return;
        }
        // uassert(false && "PROCESSING SENSOR DATA -- THIS IS GETTING HIT");
        IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager::processSensorData: byte1: %d, byte2: %d\r\n", byte1, byte2);
        for (int sensor = 1; sensor <= SENSORS_PER_BANK; ++sensor)
        {
            int idx = ((bank - 1) * SENSORS_PER_BANK) + (sensor - 1);
            int new_state = (sensor < 9) ? SENSOR_TRIGGERED(byte1, sensor) : SENSOR_TRIGGERED(byte2, sensor);
            // IO_NS::PrintTerminal(COLOR_YELLOW "NEW STATE: %d\r\n", new_state);

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
                        IO_NS::PrintTerminal(COLOR_YELLOW "SensorManager::processSensorData: recent sensors is full -- making space\r\n");
                    }
                    recent_sensors.Push(idx);

                    last_triggered_bank = sensor_data[idx].bank;
                    last_triggered_id = sensor_data[idx].id;

                    IO_NS::PrintTerminal("Jack::Sensor------Sensor %c%d triggered\r\n", last_triggered_bank, last_triggered_id);
                    int command_received = -3;
                    char sensor_bank[2] = {0};
                    sensor_bank[0] = last_triggered_bank;
                    ConductorRequest request(COMMAND::SENSOR_TRIGGER, last_triggered_id, 0, sensor_bank);
                    SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), (char *)command_received, sizeof(int));

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
            IO_NS::PrintTerminal("SensorManager::SensorLoop: Created SensorTicker with TID %d\r\n", SENSOR_TICKER_TID);
            uassert(SENSOR_TICKER_TID > 0 && "SensorManager::SensorLoop: Error creating SensorTicker");
        }

        int my_tid = MYTID();
        int sender_tid;
        SensorQuery query;

        test_sensor_masks();
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "SensorManager: Error finding ClockServer");

        while (true)
        {
            bool send_reply = false;
            int retval = RECEIVE(&sender_tid, (char *)&query, sizeof(query));
            uassert(retval >= 0 && "SensorManager: Error receiving SensorQuery");
            IO_NS::PrintTerminal(COLOR_RED "SensorManager: Received SensorQuery from %d\r\n", sender_tid);
            cur_tick = TIME(CLOCK_SERVER_TID);

            switch (query.command)
            {
            case SENSOR_COMMAND::TICK:
            {
                IO_NS::PrintTerminal(COLOR_RED "SensorManager: Received TICK request -- READING ALL BANKS\r\n");
                send_reply = true;
                break;
            }
            case SENSOR_COMMAND::RESET:
            {
                // IO_NS::PrintTerminal(COLOR_YELLOW"SensorManager: Received RESET request\r\n");
                Reset(true);
                send_reply = true;
                break;
            }
            default:
                break;
            }

            if (IRQ_ENABLED)
            {
                IO_NS::PrintTerminal(COLOR_RED "SensorManager: Reading all banks\r\n");
                ReadAll(NUM_BANKS);
            }

            Display(); // this can block the server until IO is ready
            if (send_reply)
            {
                IO_NS::PrintTerminal(COLOR_RED "SensorManager: Sending reply to %d\r\n", sender_tid);
                REPLY(sender_tid, nullptr, 0);
            }
            // uassert(false && "SensorManager::SensorLoop: THIS MUST BE HIT");

            // THIS IS NOT REPLYING TO TRAIN MESSENGER
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
        int sensor_server_tid = WHOIS("SensorManager");
        uassert(sensor_server_tid > 0 && "SensorTicker: Error finding SensorServer");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "SensorTicker: Error finding ClockServer");

        SensorQuery query = {SENSOR_COMMAND::TICK};
        while (true)
        {
            int retval = SEND(sensor_server_tid, (char *)&query, sizeof(query), nullptr, 0);
            uassert(retval >= 0 && "SensorTicker: Error sending SensorQuery to SensorServer");
            IO_NS::PrintTerminal(COLOR_YELLOW "SensorTicker: DELAYING\r\n");
            DELAY(CLOCK_SERVER_TID, 20);
        }
    }
} // namespace Sensors_NS
