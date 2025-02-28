#ifndef _ui_h_
#define _ui_h_

#include "../../clock.h"
#include "../marklin/switch.h"
#include "command.hpp"
// #include "../marklin/train.h"

extern Clock clock;

namespace UI_NS
{

    class IdleTask
    {
        int idlePercent;

    public:
        IdleTask();
        ~IdleTask();
        void Display();
    };

    class UI
    {
        IdleTask idleTask;
        // Switches switches;
        // CommandPrompt commandPrompt;

        // SensorManager sensors;
        // RingBuffer<int> recent_sensors; 

    public:
        UI();
        void Update();
    };

    void start_ui();
}

#endif
