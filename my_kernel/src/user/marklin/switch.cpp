#include "switch.h"
// #include "../clock.h"
// #include "../include/io.h"
// #include "../include/name_server.h"
#include "../include/marklin_io.h"
#include "../include/clock_server.h"
#include "../../shared_constants.h"
#include "../include/uassert.h"

namespace Switch_NS
{
    static int MARKLIN_IO_SERVER_TID;

    static void PrintSwitchStatus(int switch_addr, SWITCH_STATE state)
    {
        if (state == SWITCH_STATE::STRAIGHT)
        {
            IO_NS::Print(MOVE_CURSOR COLOR_GREEN "S", SWITCH_LOCATION + 3 + switch_addr, SWITCH_STATUS_COL);
        }
        else
        {
            IO_NS::Print(MOVE_CURSOR COLOR_RED "C", SWITCH_LOCATION + 3 + switch_addr, SWITCH_STATUS_COL);
        }
    }
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

        bool is_straight = (ALIGNMENT == SWITCH_STATE::STRAIGHT);
        if (ALIGNMENT == SWITCH_STATE::STRAIGHT)
        {
            // IO_NS::PrintTerminal(COLOR_GREEN "Setting Switch-%d to %c\r\n", address, 'S');
        }
        else
        {
            // IO_NS::PrintTerminal(COLOR_RED "Setting Switch-%d to %c\r\n", address, 'C');
        }
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    void Switch::SetSwitchIsolated(SWITCH_STATE ALIGNMENT)
    {
        this->ALIGNMENT = ALIGNMENT;
        SetSwitch(ALIGNMENT);
        // send off command to marklin
        MarklinRequest request = {COMMAND::SOLENOID_OFF, address, 0, 0};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        PrintSwitchStatus(address, ALIGNMENT);
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
        // const int COUNT = 6;
        // const int SWITCH_ADDR[COUNT] = {16, 17, 0x9A, 0x9B, 0x9C, 0x99};
        // const SWITCH_STATE state[SWITCH_COUNT] = {SWITCH_STATE::STRAIGHT,
        //   SWITCH_STATE::CURVED};

        const int SWITCH_ADDR[SWITCH_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 0x9A, 0x9B, 0x9C, 0x99};

        // for (int i = 0; i < COUNT; ++i)
        for (int i = 0; i < SWITCH_COUNT - 1; ++i)
        {
            IO_NS::PrintTerminal("Switch::Init %d: %d\r\n", i, SWITCH_ADDR[i]);
            switches[i] = Switch();
            switches[i].address = SWITCH_ADDR[i];
            switches[i].SetSwitch(SWITCH_STATE::CURVED);
            PrintSwitchStatus(i, SWITCH_STATE::CURVED);
            IO_NS::PrintTerminal("Switch::Init SUCCESS FOR %d: %d\r\n", i, SWITCH_ADDR[i]);
        }
        switches[SWITCH_COUNT - 1] = Switch();
        switches[SWITCH_COUNT - 1].address = SWITCH_ADDR[SWITCH_COUNT - 1];
        switches[SWITCH_COUNT - 1].SetSwitchIsolated(SWITCH_STATE::CURVED);
        PrintSwitchStatus(SWITCH_COUNT - 1, SWITCH_STATE::CURVED);
        // }
    }

    bool Switches::SetSwitch(int switch_addr, SWITCH_STATE state)
    {
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            if (switches[i].address == switch_addr)
            {
                switches[i].SetSwitch(state);
                PrintSwitchStatus(i, state);
                return true;
            }
        }
        IO_NS::PrintTerminal("Switches::SetSwitch: Switch-%d not found\r\n", switch_addr);

        return false;
    }

    void Switches::setIOServerTid(int tid)
    {
        MARKLIN_IO_SERVER_TID = tid;
    }
}