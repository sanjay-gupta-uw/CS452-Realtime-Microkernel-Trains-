#ifndef SENSOR_H
#define SENSOR_H

#include <cstdint>
#include "../include/io.h"
#include "../include/marklin_io.h"
#include "../../containers/ringbuffer.h"
// #include "clock.h"
#include "../include/name_server.h"
#include "../../shared_constants.h" 

#define NUM_BANKS 5
#define SENSORS_PER_BANK 16
#define MAX_RECENT_SENSORS 10

#define READ_ONE_SENSOR_BASE 192
#define READ_ALL_SENSOR_BASE 128
#define RESET_MODE_ON 192
#define RESET_MODE_OFF 128

typedef enum {
    SEN_OFF,
    SEN_ON,
} SensorState;

class SensorManager {
public:
    SensorManager();
    void Init(bool resetMode);
    void ReadAll(int num_banks, RingBuffer<int> *recent_sensors);
    void Reset(bool reset_on);
    void InitDisplay(IO *io, int location);
    void Display(IO *io, int location, RingBuffer<int> *recent_sensors);

private:
    struct Sensor {
        char bank;
        uint8_t id;
        int status;
    };

    Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK];
    static const char BANK_LABELS[NUM_BANKS];
    bool UPDATE_DISPLAY;
    char last_triggered_bank;
    uint8_t last_triggered_id;
    int marklin_io_tid;
    int console_io_tid;

    void processSensorData(int bank, uint8_t byte1, uint8_t byte2, RingBuffer<int> *recent_sensors);
};

#endif // SENSOR_H