#ifndef _sensor_h_
#define _sensor_h_

#include <stdint.h>
#include <stdbool.h>
#include "rpi.h"
#include "clock.h"
#include "util.h"
#include "ringbuffer.h"

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

typedef struct {
    char bank;
    uint8_t id;
    SensorState status;
} Sensor;

void sensor_init(bool resetMode);
void sensor_read_all(int num_banks, RingBuffer *recent_sensors);
void sensor_reset(bool reset_on);
void init_sensor_display(int location);
void sensor_display(int location, RingBuffer *recent_sensors);

#endif