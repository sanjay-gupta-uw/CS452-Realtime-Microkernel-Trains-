#ifndef _ui_h_
#define _ui_h_

#include "../../clock.h"
#include "../marklin/switch.h"
#include "command.hpp"
// #include "../marklin/train.h"
#include "../include/io.h"

// extern Clock clock;

namespace UI_NS
{

// assume terminal is 80x24
#define NUM_ROWS 24
#define NUM_COLS 80

    // #define COMMAND_STATUS_LOCATION 61

    class UI
    {
        IO io;
        Switches switches;
        // CommandPrompt commandPrompt;

    public:
        UI();
        void Update();
    };

    void start_ui();
}

#endif
