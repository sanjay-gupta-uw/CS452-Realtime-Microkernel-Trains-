#include "sensor.h"
#include "../include/uassert.h"
namespace Sensors_NS
{

    const char SensorManager::BANK_LABELS[NUM_BANKS] = {'A', 'B', 'C', 'D', 'E'};

    SensorManager::SensorManager()
        : UPDATE_DISPLAY(false), last_triggered_bank('\0'), last_triggered_id(0)
    {
        MARKLIN_IO_SERVER_TID = -1;
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
        IO_NS::PrintTerminal("Sending Initial Reset to Marklin\r\n");
        uart_putc(MARKLIN, RESET_MODE_ON); // this will allow the marklin to send cts/tx interrupts
    }

    SensorManager::~SensorManager()
    {
    }

    void SensorManager::Reset(bool reset_on)
    {
        // first thing sent to marklin
        // int ret = MARKLIN_IO_SERVER::Putc(MARKLIN_IO_SERVER_TID, MARKLIN, reset_on ? RESET_MODE_ON : RESET_MODE_OFF);
        MarklinRequest request = {COMMAND::READ_SENSOR, -1, -1, reset_on ? RESET_MODE_ON : RESET_MODE_OFF};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret > 0 && "SensorManager::Reset: command sent to MarklinIOServer failed");
    }

    void SensorManager::setMarklinIOtid(int tid)
    {
        MARKLIN_IO_SERVER_TID = tid;
    }

    void SensorManager::ReadBank(int num_bank)
    {
        num_bank = VALIDATE_BANK(num_bank);

        // send command to marklin
        MarklinRequest request = {COMMAND::READ_SENSOR, -1, -1, READ_ONE_SENSOR_BASE + num_bank};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret > 0 && "SensorManager::ReadBank: command sent to MarklinIOServer failed");

        // get response from marklin -- 2 bytes per bank
        char bytes[2];
        for (int i = 0; i < 2; ++i)
        {
            // get byte from marklin io
            bytes[i] = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(bytes[i] >= 0 && "SensorManager::ReadBank: Getc failed");
        }
        processSensorData(num_bank, bytes[0], bytes[1]);
    }

    void SensorManager::ReadAll(int num_banks)
    {
        num_banks = VALIDATE_BANK(num_banks);

        MarklinRequest request = {COMMAND::READ_SENSOR, -1, -1, READ_ALL_SENSOR_BASE + num_banks};
        int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        uassert(ret > 0 && "SensorManager::ReadAll: command sent to MarklinIOServer failed");

        for (int bank = 0; bank < num_banks; ++bank)
        {
            uint8_t byte1 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(byte1 >= 0 && "SensorManager::ReadAll: Getc failed");
            uint8_t byte2 = MARKLIN_IO_SERVER::Getc(MARKLIN_IO_SERVER_TID);
            uassert(byte2 >= 0 && "SensorManager::ReadAll: Getc failed");
            processSensorData(bank, byte1, byte2);
        }
    }

    void SensorManager::processSensorData(int bank, uint8_t byte1, uint8_t byte2)
    {
        for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor)
        {
            int idx = bank * SENSORS_PER_BANK + sensor;
            int new_state = (sensor < 8) ? SENSOR_TRIGGERED(byte1, sensor) : SENSOR_TRIGGERED(byte2, sensor - 8);

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
                    }
                    recent_sensors.Push(idx);

                    last_triggered_bank = sensor_data[idx].bank;
                    last_triggered_id = sensor_data[idx].id;
                    UPDATE_DISPLAY = true;
                }
            }
        }
    }

    // void SensorManager::InitDisplay(IO *io, int location)
    // {
    //     io->move_cursor(location, 1);
    //     io->color_yellow();
    //     io->Print("Recent Sensor Changes (Last 10):\r\n");
    //     io->Print("--------------------------------\r\n");
    // }

    // void SensorManager::Display(IO *io, int location, RingBuffer<int> *recent_sensors)
    // {
    //     if (!UPDATE_DISPLAY || recent_sensors->IsEmpty())
    //         return;

    //     int total_items = recent_sensors->Size() < MAX_RECENT_SENSORS ? recent_sensors->Size() : MAX_RECENT_SENSORS;

    //     for (int i = 0; i < MAX_RECENT_SENSORS; i++)
    //     {
    //         io->move_cursor(location + 2 + i, 1);
    //         io->clear_to_end_line();
    //         io->color_yellow();

    //         if (i < total_items)
    //         {
    //             int logical_idx = recent_sensors->Size() - 1 - i;
    //             int sensor_idx = recent_sensors->Get(logical_idx, &sensor_idx);
    //             io->Print("%2d. %c%02d\r\n", i + 1, sensor_data[sensor_idx].bank, sensor_data[sensor_idx].id);
    //         }
    //     }

    //     UPDATE_DISPLAY = false;
    // }
} // namespace Sensors_NS