#ifndef _switch_h_
#define _switch_h_

#include <stdbool.h>
// #include "../../rpi.h"
#include "../include/io.h"
#include "../include/marklin_structs.h"
// #include "../include/util.h" // this includes rpi.h
#include "../../shared_constants.h"

namespace Switch_NS
{
    typedef enum
    {
        STRAIGHT, // green
        CURVED,   // red
    } SWITCH_STATE;

    class Switches;
    class Switch
    {
    private:
        SWITCH_STATE ALIGNMENT;

    public:
        int address;
        Switch();
        ~Switch();
        void SetSwitch(SWITCH_STATE ALIGNMENT);

        friend class Switches;
    };

    class Switches
    {
    private:
        Switch switches[SWITCH_COUNT];

    public:
        Switches();
        ~Switches();
        void Init();
        bool SetSwitch(int switch_addr, SWITCH_STATE state);
        void setIOServerTid(int tid);
    };
    // ui.h for switch display
};

#endif // _switch_h_