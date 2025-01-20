#ifndef _ui_h_
#define _ui_h_

#include "command.h"
#include "sensor.h"
#include "rpi.h"
#include "clock.h"
#include "switch.h"
#include "util.h"
#include "ringbuffer.h"

// assume terminal is 80x24
#define NUM_ROWS 24
#define NUM_COLS 80

#define CLOCK_LENGTH 8

#define SWITCH_LOCATION 3
#define SENSOR_LOCATION 35
#define CMD_LOCATION 60

void create_ui();
void update_ui(RingBuffer *rb);

#endif
