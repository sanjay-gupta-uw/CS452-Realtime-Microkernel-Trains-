#ifndef SENSOR_H
#define SENSOR_H

#include <cstdint>
#include "../include/io.h"
#include "../include/marklin_io.h"
#include "../../containers/ringbuffer.h"
#include "../../clock.h"
#include "../include/name_server.h"
#include "../../shared_constants.h"
#include "train.h"

namespace Sensors_NS
{
#define NUM_TRAINS 5
#define VALIDATE_BANK(bank) (int)(bank > 0 && bank <= NUM_BANKS ? bank : NUM_BANKS)
#define SENSOR_TRIGGERED(byte, sensor) ((byte >> (8 - ((sensor > 8) ? (sensor - 8) : sensor))) & 0b00000001)

#define NUM_BANKS 5
#define SENSORS_PER_BANK 16
#define MAX_RECENT_SENSORS 10

#define READ_ONE_SENSOR_BASE 192
#define READ_ALL_SENSOR_BASE 128

#define RESET_MODE_ON READ_ONE_SENSOR_BASE
#define RESET_MODE_OFF READ_ALL_SENSOR_BASE

    enum class SENSOR_COMMAND
    {
        TRAIN_SENSOR,
        MESSENGER_READY,
        RESET,
        TICK,
    };

    // train query to conductor
    struct SensorQuery
    {
        SENSOR_COMMAND command;
        SensorStruct sensor;
        int train_tid;
        int train_num;
    };

    struct SensorResponse
    {
        bool success;
        int trigger_tick;
        int train_tid;
        SensorStruct sensor;
    };

#define SEN_OFF 0
#define SEN_ON 1

    class SensorManager
    {
    public:
        SensorManager();
        ~SensorManager();
        void ReadAll(int num_bank);
        void Reset(bool reset_on);
        void Display();

    private:
        struct WaitingTrain
        {
            int train_tid;
            int train_num;
            int idx; // idx of sensor_data it is waiting for
            int messenger_tid;
        };

        void SensorLoop();

        void processSensorData(int bank, uint8_t byte1, uint8_t byte2);
        Queue<int, MAX_RECENT_SENSORS> recent_sensors;

        struct Sensor
        {
            char bank;
            uint8_t id;
            int status;
        };
        Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK];

        void HandleWaitingTrains();
        void push_waiting_train(int idx, int train_tid, int train_num);
        WaitingTrain waiting_trains[NUM_TRAINS];

        bool UPDATE_DISPLAY;
        char last_triggered_bank;
        uint8_t last_triggered_id;
        int MARKLIN_IO_SERVER_TID;
        int SENSOR_TICKER_TID;
        bool isTickerRunning;

        int cur_tick;
    };

    void SensorServer();
    void SensorTicker();
    void SensorMessenger();

    void InitializeSensorManager();
} // namespace Sensors_NS

#endif // SENSOR_H
