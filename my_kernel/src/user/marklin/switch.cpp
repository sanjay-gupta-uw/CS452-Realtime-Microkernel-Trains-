#include "switch.h"
// #include "../clock.h"
// #include "../include/io.h"
// #include "../include/name_server.h"
// #include "../include/marklin_io.h"
#include "../include/clock_server.h"
#include "../../shared_constants.h"
#include "../include/uassert.h"

namespace Switch_NS
{
    static int MARKLIN_IO_SERVER_TID;
    // ********* Switch Class *********
    Switch::Switch()
    {
        address = -1;
        ALIGNMENT = SWITCH_STATE::STRAIGHT; // should not matter at init
    }

    Switch::~Switch()
    {
    }

    void Switch::SetSwitch(SWITCH_STATE ALIGNMENT)
    {

        this->ALIGNMENT = ALIGNMENT;
        MarklinRequest request;
        request.type = COMMAND::SET_SWITCH;
        request.id = address;
        request.data = (ALIGNMENT == SWITCH_STATE::STRAIGHT) ? STRAIGHT_CMD : CURVED_CMD;
        // update display

        bool is_straight = (ALIGNMENT == SWITCH_STATE::STRAIGHT);
        if (ALIGNMENT == SWITCH_STATE::STRAIGHT)
        {
            IO_NS::PrintTerminal(COLOR_GREEN "Switch-%d set to %c\r\n", address, 'S');
        }
        else
        {
            IO_NS::PrintTerminal(COLOR_RED "Switch-%d set to %c\r\n", address, 'C');
        }
    }

    // ********* Switches Class *********

    Switches::Switches()
    {
        MARKLIN_IO_SERVER_TID = -1;
    }

    Switches::~Switches()
    {
    }

    void Switches::Init()
    {
        const int SWITCH_ADDR[SWITCH_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0x9A, 0x9B, 0x9C, 0x99};

        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            switches[i] = Switch();
            switches[i].address = SWITCH_ADDR[i];
            switches[i].SetSwitch(SWITCH_STATE::CURVED);
        }
    }

    bool Switches::SetSwitch(int switch_addr, SWITCH_STATE state)
    {
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            if (switches[i].address == switch_addr)
            {
                switches[i].SetSwitch(state);
                return true;
            }
        }

        return false;
    }

    void Switches::setIOServerTid(int tid)
    {
        MARKLIN_IO_SERVER_TID = tid;
    }
}