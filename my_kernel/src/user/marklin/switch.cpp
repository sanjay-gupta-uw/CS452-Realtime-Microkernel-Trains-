#include "switch.h"
// #include "../clock.h"
// #include "../include/io.h"
// #include "../include/name_server.h"
#include "../include/marklin_io.h"
#include "../include/clock_server.h"
#include "../../shared_constants.h"
#include "../include/uassert.h"
#include "../include/name_server.h"

#include "../../clock.h"
namespace Switch_NS
{
    // ********* Switch Class *********
    Switch::Switch()
    {
        address = -1;
        MARKLIN_IO_SERVER_TID = -1;
        state = SWITCH_STATE::UNINITIALIZED;
    }

    Switch::Switch(int address, int MARKLIN_IO_SERVER_TID, int CLOCK_SERVER_TID)
        : Switch()
    {
        this->address = address;
        this->MARKLIN_IO_SERVER_TID = MARKLIN_IO_SERVER_TID;
        this->CLOCK_SERVER_TID = CLOCK_SERVER_TID;
    }

    Switch::~Switch()
    {
    }

    bool Switch::SetSwitch(SWITCH_STATE ALIGNMENT)
    {
        // IO_NS::PrintTerminal("State: %d, Alignment: %d\r\n", state, ALIGNMENT);
        if (state == ALIGNMENT)
        {
            // IO_NS::PrintTerminal("Switch %d already in alignment %d\r\n", address, ALIGNMENT);
            return false;
        }
        Clock clock;
        uint32_t start_time = clock.Time();

        uint32_t switch_cmd = (ALIGNMENT == SWITCH_STATE::STRAIGHT) ? STRAIGHT_CMD : CURVED_CMD;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, switch_cmd, address};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        state = ALIGNMENT;
        DELAY(CLOCK_SERVER_TID, 25);

        uint32_t end_time = clock.Time();
        IO_NS::PrintTerminal("SetSwitch::time to send and return first switch command: %d\r\n", end_time - start_time);

        SendOffCommand();

        return true;
    }

    void Switch::SendOffCommand()
    {
        MARKLIN_IO_SERVER::MarklinRequest request = {false, SOLENOID_OFF_CMD, address};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    // ********* End Switch Class *********

    // Switch server
    void SwitchServer()
    {
        int my_tid = MYTID();
        REGISTERAS("SwitchServer");
        IO_NS::PrintTerminal("SwitchServer: Started with tid{%d}\r\n", my_tid);
        int MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID >= 0 && "SwitchServer: WHOIS failed");

        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID >= 0 && "SwitchServer: WHOIS failed");

        const int switch_addrs[NUM_SWITCHES] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                11, 12, 13, 14, 15, 16, 17, 18,
                                                0x99, 0x9A, 0x9B, 0x9C};

        Switch switches[NUM_SWITCHES];
        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            switches[i] = Switch(switch_addrs[i], MARKLIN_IO_SERVER_TID, CLOCK_SERVER_TID);
            // switches[i].SetSwitch(SWITCH_STATE::STRAIGHT);
            // IO_NS::Print(MOVE_CURSOR COLOR_GREEN "S", SWITCH_LOCATION + 3 + i, SWITCH_STATUS_COL);
        }

        IO_NS::PrintTerminal("SwitchServer: STARTING SWITCH TEST\r\n");
        for (int i = 0; i < 20; ++i)
        {
            switches[0].SetSwitch(i % 2 == 0 ? SWITCH_STATE::STRAIGHT : SWITCH_STATE::CURVED);
            IO_NS::Print(MOVE_CURSOR COLOR_GREEN "S", SWITCH_LOCATION + 3 + i, SWITCH_STATUS_COL);
            IO_NS::PrintTerminal("DELAYING FOR 200 TICKS\r\n");
            DELAY(CLOCK_SERVER_TID, 200);
        }
        uassert(false && "SwitchServer: Finished switch test");

        // switches[NUM_SWITCHES - 1].SendOffCommand();

        // int timer_tid = CREATE(PRIORITY::P0, switch_timer);
        // uassert(timer_tid >= 0 && "SwitchServer: failed to CREATE timer");
        // IO_NS::PrintTerminal("SwitchServer: Timer created with tid{%d}\r\n", timer_tid);
        // bool isTimerActive = true;

        int sender_tid;
        // int last_switch_index = -1;
        SwitchRequest request;
        while (true)
        {
            int ret = RECEIVE(&sender_tid, (char *)&request, sizeof(SwitchRequest));
            IO_NS::PrintTerminal("SwitchServer: Received message from tid{%d}\r\n", sender_tid);
            IO_NS::PrintTerminal("SwitchServer: IS TIME REQUEST: %d\r\n", request.isTimer);
            uassert(ret >= 0 && "SwitchServer: RECEIVE failed");

            // if (request.isTimer)
            // {
            //     IO_NS::PrintTerminal("SwitchServer: Timer expired\r\n");
            //     isTimerActive = false;
            //     if (last_switch_index >= 0)
            //     {
            //         IO_NS::PrintTerminal("SwitchServer: Sending off command to switch index %d\r\n", last_switch_index);
            //         switches[last_switch_index].SendOffCommand();
            //         last_switch_index = -1;
            //     }
            // }
            // else
            {
                // ASSUME SWITCH COMMANDS ARE VALIDATED ALREADY
                SWITCH_STATE alignment = request.switch_state;
                IO_NS::PrintTerminal("SwitchServer: Setting switch %d to %c\r\n", request.switch_index, alignment == SWITCH_STATE::STRAIGHT ? 'S' : 'C');
                // last_switch_index = request.switch_index;
                bool success = switches[request.switch_index].SetSwitch(alignment);
                if (!success)
                {
                    IO_NS::PrintTerminal("SwitchServer: Switch index %d already in alignment %c\r\n", request.switch_index, request.switch_state);
                }
                // if (success && !isTimerActive)
                // {
                //     IO_NS::PrintTerminal("WAKING SWITCH TIMER\r\n");
                //     isTimerActive = true;
                //     REPLY(timer_tid, nullptr, 0);
                // }
                REPLY(sender_tid, nullptr, 0);
            }
        }
    }

    void switch_timer()
    {
        REGISTERAS("SwitchTimer");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID >= 0 && "SwitchTimer: WHOIS failed");
        int switch_server_tid = WHOIS("SwitchServer");
        uassert(switch_server_tid >= 0 && "SwitchTimer: WHOIS failed");

        SwitchRequest request = {true};
        while (true)
        {
            int ret = SEND(switch_server_tid, (char *)&request, sizeof(SwitchRequest), nullptr, 0);

            // delay for 20 ticks when freed
            DELAY(CLOCK_SERVER_TID, 20);
        }
    }
}