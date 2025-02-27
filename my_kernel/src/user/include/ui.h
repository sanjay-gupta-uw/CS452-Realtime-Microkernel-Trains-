#ifndef _ui_h_
#define _ui_h_

#include "../../clock.h"
#include "../marklin/switch.h"
#include "../marklin/sensor.h"
#include "../../container/ringbuffer.h"
#include "../include/io.h"

// extern Clock clock;
// extern CommandPrompt cmd_prompt;

namespace UI_NS
{

// assume terminal is 80x24
#define NUM_ROWS 24
#define NUM_COLS 80

#define CLOCK_LENGTH 8

#define IDLE_LOCATION 0
#define CLOCK_LOCATION 2
#define SWITCH_LOCATION 4

#define SENSOR_LOCATION 30
#define CMD_LOCATION 60
    // #define COMMAND_STATUS_LOCATION 61

    class UI
    {
        IO io;
        Switches switches;
        SensorManager sensors;
        RingBuffer<int> recent_sensors; 

    public:
        UI() : recent_sensors(MAX_RECENT_SENSORS) {};
        void Update();
    };

    void start_ui();
}

#endif
