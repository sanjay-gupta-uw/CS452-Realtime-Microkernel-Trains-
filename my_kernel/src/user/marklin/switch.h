#ifndef _switch_h_
#define _switch_h_

#include <stdbool.h>
// #include "../../rpi.h"
#include "../include/io.h"
#include "../include/marklin_structs.h"
// #include "../include/util.h" // this includes rpi.h

namespace Switch_NS
{
#define SWITCH_COUNT 22
    typedef enum
    {
        STRAIGHT, // green
        CURVED,   // red
    } SWITCH_STATE;

    class Switch
    {
    private:
        int address;
        // void SendCommand(int data);

    public:
        Switch();
        ~Switch();
        void SetAddr(int addr);
        int GetAddr();
        void SetSwitch(SWITCH_STATE ALIGNMENT);
        SWITCH_STATE ALIGNMENT;
        bool updated;
    };

    // Switch switches[22]; // Zero-initialize the entire array

    bool isSwitchCommandValid(MarklinRequest *request); // returns true if switch was found/request updated

    void InitDisplay();
    void Display();
    MarklinRequest CreateSwitchRequest(int switch_num, SWITCH_STATE state);
};

#endif // _switch_h_