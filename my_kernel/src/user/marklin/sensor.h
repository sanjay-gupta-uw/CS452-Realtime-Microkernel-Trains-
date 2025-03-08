#ifndef SENSOR_H
#define SENSOR_H

#include <cstdint>
#include "../include/io.h"
#include "../include/marklin_io.h"
#include "../../containers/ringbuffer.h"
#include "../../clock.h"
#include "../include/name_server.h"
#include "../../shared_constants.h"

namespace Sensors_NS
{
#define VALIDATE_BANK(bank) (int)(bank >= 0 && bank < NUM_BANKS ? bank : NUM_BANKS)
#define SENSOR_TRIGGERED(byte, sensor) ((byte >> (7 - sensor)) & 1)

#define NUM_BANKS 5
#define SENSORS_PER_BANK 16
#define MAX_RECENT_SENSORS 10

#define READ_ONE_SENSOR_BASE 192
#define READ_ALL_SENSOR_BASE 128

#define RESET_MODE_ON READ_ONE_SENSOR_BASE
#define RESET_MODE_OFF READ_ALL_SENSOR_BASE

    typedef enum
    {
        SEN_OFF,
        SEN_ON,
    } SensorState;

    class SensorManager
    {
    public:
        SensorManager();
        ~SensorManager();
        void setMarklinIOtid(int tid);
        void ReadBank(int num_bank);
        void ReadAll(int num_banks);
        void Reset(bool reset_on);
        void Display();

    private:
        Queue<int, MAX_RECENT_SENSORS> recent_sensors;
        struct Sensor
        {
            char bank;
            uint8_t id;
            int status;
        };

        Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK];
        bool UPDATE_DISPLAY;
        char last_triggered_bank;
        uint8_t last_triggered_id;
        int MARKLIN_IO_SERVER_TID;

        void processSensorData(int bank, uint8_t byte1, uint8_t byte2);
    };
} // namespace Sensors_NS

#endif // SENSOR_H
