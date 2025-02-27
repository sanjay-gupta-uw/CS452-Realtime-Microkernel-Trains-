#ifndef _switch_h_
#define _switch_h_

#include <stdbool.h>
// #include "../../rpi.h"
#include "../include/io.h"
#include "../include/marklin_structs.h"
// #include "../include/util.h" // this includes rpi.h

// class Clock;
// extern Clock clock;

typedef enum
{
    STRAIGHT, // green
    CURVED,   // red
} SWITCH_STATE;

// forward declaration
class Switches;

class Switch
{
private:
    SWITCH_STATE ALIGNMENT;
    int address;

    void SendCommand(int data);
    // void Print(IO *io) const;

public:
    Switch();
    ~Switch();
    void SetAddr(int addr);
    void SetSwitch(SWITCH_STATE ALIGNMENT);

    friend class Switches;
};

class Switches
{
private:
    Switch switches[22]; // Zero-initialize the entire array

public:
    Switches();
    ~Switches();

    bool SetSwitch(MarklinRequest *request); // returns true if switch was found/request updated
    void SetAll(SWITCH_STATE ALIGNMENT);

    void InitDisplay(int LOCATION);
    void Display(int LOCATION);
    void setClockServerTid(int tid);
    void setMarklinIoServerTid(int tid);
};

#endif