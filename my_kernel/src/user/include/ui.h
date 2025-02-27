#ifndef _ui_h_
#define _ui_h_

#include "../../clock.h"
#include "../marklin/switch.h"
#include "command.hpp"
// #include "../marklin/train.h"

extern Clock clock;

namespace UI_NS
{

    class UI
    {
        // Switches switches;
        // CommandPrompt commandPrompt;

    public:
        UI();
        void Update();
    };

    void start_ui();
}

#endif
