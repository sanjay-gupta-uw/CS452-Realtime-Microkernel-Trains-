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

#define READ_ONE_SENSOR_BASE 192 // pass this + {unit #} to read it
#define READ_ALL_SENSOR_BASE 128

#define RESET_MODE_ON 192
#define RESET_MODE_OFF 128

typedef enum
{
   SEN_OFF,
   SEN_ON,
} SensorState;

typedef struct
{
   char bank;          // 'A', 'B', 'C', 'D', 'E'
   uint8_t id;         // Sensor ID (1 to MAX_SENSORS)
   SensorState status; // Sensor status: 1 = active, 0 = inactive
} Sensor;

void sensor_init(bool resetMode);                                // Initialize the sensors
void sensor_read_all(int num_banks, RingBuffer *recent_sensors); // Read all sensors and store their states
void sensor_read_bank(char bank);                                // Read the status of a specific sensor
void sensor_reset(bool reset_on);                                // Reset all sensor data
void init_sensor_display(int LOCATION);                          // Debug function to print sensor data
void sensor_display(int LOCATION, RingBuffer * recent_sensors);                               // Debug function to print sensor data

#endif
