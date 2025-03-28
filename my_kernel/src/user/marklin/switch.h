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
    typedef enum
    {
        STRAIGHT, // green
        CURVED,   // red
        UNINITIALIZED,
    } SWITCH_STATE;

    struct SwitchRequest
    {
        int switch_index;
        SWITCH_STATE switch_state;
    };

    class Switch
    {
    private:
        int CLOCK_SERVER_TID;
        int MARKLIN_IO_SERVER_TID;
        int address;
        void SendOffCommand();

    public:
        Switch();
        Switch(int address, int MARKLIN_IO_SERVER_TID, int CLOCK_SERVER_TID);
        ~Switch();
        bool SetSwitch(SWITCH_STATE ALIGNMENT);

        SWITCH_STATE state;
    };

    void SwitchServer();
    // ui.h for switch display
};

#endif // _switch_h_