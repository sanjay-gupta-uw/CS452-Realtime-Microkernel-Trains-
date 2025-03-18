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
#define NUM_SWITCHES 22

    struct SwitchRequest
    {
        bool isTimer;
        int switch_index;
        char switch_state;
    };

    typedef enum
    {
        STRAIGHT, // green
        CURVED,   // red
    } SWITCH_STATE;

    class Switch
    {
    private:
        int MARKLIN_IO_SERVER_TID;
        int address;

    public:
        Switch();
        Switch(int address, int MARKLIN_IO_SERVER_TID);
        ~Switch();
        void SetSwitch(SWITCH_STATE ALIGNMENT);
        void SendOffCommand();
    };

    void SwitchServer();
    void switch_timer();
    // ui.h for switch display
};

#endif // _switch_h_