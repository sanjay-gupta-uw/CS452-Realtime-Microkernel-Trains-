#include "switch.h"
// #include "../clock.h"
// #include "../include/io.h"
// #include "../include/name_server.h"
// #include "../include/marklin_io.h"
#include "../include/clock_server.h"
#include "../../shared_constants.h"

namespace Switch_NS
{

    Switch switches[SWITCH_COUNT];

    static const int SWITCH_ADDR[SWITCH_COUNT] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0x9A, 0x9B, 0x9C, 0x99};

    static bool UPDATE_DISPLAY = true;

    // ********* Switch Class *********
    Switch::Switch()
    {
        address = -1;
        ALIGNMENT = SWITCH_STATE::STRAIGHT;
        updated = true;
    }

    Switch::~Switch()
    {
    }

    void Switch::SetAddr(int idx)
    {
        address = SWITCH_ADDR[idx];
    }

    void Switch::SetSwitch(SWITCH_STATE ALIGNMENT)
    {

        if (this->ALIGNMENT != ALIGNMENT)
        {
            this->ALIGNMENT = ALIGNMENT;
            int s_num = address;
            int idx = (s_num < 18) ? s_num : 18 + s_num - 0x9A;
            updated = true;
        }
    }

    // ensures valid address passed in
    bool isSwitchCommandValid(MarklinRequest *request)
    {
        int addr = request->id;
        // unsigned char state = *((char *)request->data);
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            if (SWITCH_ADDR[i] == addr)
            {
                request->idx = i;
                return true;
            }
        }
        return false;
    }

    MarklinRequest CreateSwitchRequest(int switch_num, SWITCH_STATE state)
    {
        unsigned char alignment = (state == SWITCH_STATE::STRAIGHT) ? 'S' : 'C';
        MarklinRequest req = {MARKLIN_REQUEST_TYPE::SET_SWITCH, SWITCH_ADDR[switch_num], switch_num, (char *)&alignment};
        return req;
    }

    void InitDisplay()
    {
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 0, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|   Table of Switches:   |\r\n", SWITCH_LOCATION + 1, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 2, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|  Switch  |   Status    |\r\n", SWITCH_LOCATION + 3, 1);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 4, 1);
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            // print borders and switch address so display only needs to update the status
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "|     %d    |           |", SWITCH_LOCATION + 5 + i, 1, SWITCH_ADDR[i]);
        }
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR CLEAR_TO_END_LINE "--------------------------\r\n", SWITCH_LOCATION + 5 + SWITCH_COUNT, 1);
    }

    void Display()
    {
        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            if (switches[i].updated)
            {
                // print the char to exact location
                SWITCH_STATE state = switches[i].ALIGNMENT;
                if (state == SWITCH_STATE::STRAIGHT)
                {
                    // colour green
                    IO_NS::Print(MOVE_CURSOR COLOR_GREEN "S", SWITCH_LOCATION + 5 + i, 15);
                }
                else
                {
                    // colour red
                    IO_NS::Print(MOVE_CURSOR COLOR_RED "C", SWITCH_LOCATION + 5 + i, 15);
                }
                switches[i].updated = false;
            }
        }
    }

}