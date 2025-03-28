#ifndef _switch_h_
#define _switch_h_

#include <stdbool.h>
// #include "../../rpi.h"
#include "../include/io.h"
#include "../include/marklin_structs.h"
// #include "../include/util.h" // this includes rpi.h
#include "../../shared_constants.h"
#include "../include/graph_data.h"
extern const SwitchPos track_a_switches[];
extern const SwitchPos track_b_switches[];
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
        int switch_num;
        SWITCH_STATE alignment;
    };
    struct Switches_Alignment
    {
        SwitchRequest switches[NUM_SWITCHES];
    };
    class SwitchServer
    {
    private:
        Switches_Alignment switches;
        SwitchPos switches_locations[NUM_SWITCHES]; // location info for printing to diagram

        int CLOCK_SERVER_TID;
        int MARKLIN_IO_SERVER_TID;
        bool SetSwitch(int addr, SWITCH_STATE ALIGNMENT);
        void ServerLoop();

    public:
        SwitchServer();
        ~SwitchServer();

        SWITCH_STATE state;
    };

    void StartSwitchServer();
    // ui.h for switch display
};

#endif // _switch_h_