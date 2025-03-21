#ifndef SENSOR_H
#define SENSOR_H

#include <cstdint>
#include "../include/io.h"
#include "../include/marklin_io.h"
#include "../../containers/ringbuffer.h"
#include "../../clock.h"
#include "../include/name_server.h"
#include "../../shared_constants.h"

struct SensorResponse
{
    int id; // -1 if no sensor triggered
};
namespace Sensors_NS
{
#define VALIDATE_BANK(bank) (int)(bank > 0 && bank <= NUM_BANKS ? bank : NUM_BANKS)
#define SENSOR_TRIGGERED(byte, sensor) ((byte >> (8 - ((sensor > 8) ? (sensor - 8) : sensor))) & 0b00000001)

#define NUM_BANKS 5
#define SENSORS_PER_BANK 16
#define MAX_RECENT_SENSORS 10

#define READ_ONE_SENSOR_BASE 192
#define READ_ALL_SENSOR_BASE 128

#define RESET_MODE_ON READ_ONE_SENSOR_BASE
#define RESET_MODE_OFF READ_ALL_SENSOR_BASE

    struct BANK_MASK
    {
        bool sensor[16];
    };

    enum class SENSOR_COMMAND
    {
        READ_BANK,
        READ_ALL,
        RESET,
        TICK,
    };

    // train query to conductor
    struct SensorQuery
    {
        SENSOR_COMMAND command;
        SensorStruct sensor;
    };

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
        void ReadBank(int bank_num, BANK_MASK *bank_mask);
        void ReadAll(int num_banks);
        void Reset(bool reset_on);
        void Display();

    private:
        void processSensorData(int bank, uint8_t byte1, uint8_t byte2, BANK_MASK *bank_mask);
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
    };

    void SensorServer();
    void SensorTicker();
} // namespace Sensors_NS

#endif // SENSOR_H
