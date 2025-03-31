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
    static int getSwitchIndex(int switch_num)
    {
        if (switch_num > 0 && switch_num < 19)
        {
            return switch_num - 1;
        }
        int middle_switches[4] = {0x99, 0x9A, 0x9B, 0x9C};
        for (int i = 0; i < 4; i++)
        {
            if (switch_num == middle_switches[i])
            {
                return 18 + i;
            }
        }
        return -1;
    }
    // ********* Switch Class *********
    SwitchServer::SwitchServer()
    {
        // receive track id from conductor
        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        uassert(retval >= 0 && "SwitchServer: RECEIVE failed");
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        REPLY(sender_tid, nullptr, 0);
        // Use the correct pointer assignment
        const SwitchPos *source_switches = (track_id == 'A' || track_id == 'a') ? track_a_switches : track_b_switches;

        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            int index = getSwitchIndex(source_switches[i].num);
            // IO_NS::PrintTerminal("SwitchServer: Switch %d at index %d\r\n", source_switches[i].num, index);

            switches_locations[index].col = source_switches[i].col;
            switches_locations[index].line = source_switches[i].line;
            switches_locations[index].num = source_switches[i].num;
        }
        // while (1)
        // {
        // }

        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID >= 0 && "SwitchServer: WHOIS failed");
        CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID >= 0 && "SwitchServer: WHOIS failed");

        const int switch_addrs[NUM_SWITCHES] = {1, 2, 3, 4, 5, 6,
                                                7, 8, 9, 10, 11, 12,
                                                13, 14, 15, 16, 17, 18,
                                                0x99, 0x9A, 0x9B, 0x9C};

        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            switches.switches[i].alignment = SWITCH_STATE::UNINITIALIZED;
            switches.switches[i].switch_num = switch_addrs[i];
        }

        ServerLoop();
    }

    SwitchServer::~SwitchServer()
    {
    }

    bool SwitchServer::SetSwitch(int addr, SWITCH_STATE ALIGNMENT)
    {
        int index = getSwitchIndex(addr);
        // IO_NS::PrintTerminal("State: %d, Alignment: %d\r\n", state, ALIGNMENT);
        if (switches.switches[index].alignment == ALIGNMENT)
        {
            // IO_NS::PrintTerminal("Switch %d already in alignment %d\r\n", address, ALIGNMENT);
            return false;
        }

        uint8_t switch_cmd = (ALIGNMENT == SWITCH_STATE::STRAIGHT) ? STRAIGHT_CMD : CURVED_CMD;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, switch_cmd, addr};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
        // MARKLIN SERVER GENERATES OFF COMMAND

        char switch_state = (ALIGNMENT == SWITCH_STATE::STRAIGHT) ? 'S' : 'C';
        const char *color = (ALIGNMENT == SWITCH_STATE::STRAIGHT) ? COLOR_GREEN : COLOR_RED;
        // Update switches table
        IO_NS::Print(MOVE_CURSOR "%s%c", SWITCH_LOCATION + 3 + index, SWITCH_STATUS_COL, color, switch_state);
        // update track diagram
        // IO_NS::PrintTerminal("Switch.cpp: %d line %d col %d\r\n", addr, switches_locations[index].line, switches_locations[index].col);
        if (switches_locations[index].num == addr)
        {
            // Use current_track->switches[i].line/col
            IO_NS::Print(MOVE_CURSOR "%s%c",
                         TRACK_LAYOUT_LOCATION_Y + switches_locations[index].line,
                         TRACK_LAYOUT_LOCATION_X + switches_locations[index].col,
                         color,
                         switch_state);
        }

        // IO_NS::PrintTerminal("Switch.cpp: %d set to %c GRAPH UPATED", addr, switch_state);

        switches.switches[index].alignment = ALIGNMENT;

        return true;
    }

    // ********* End Switch Class *********

    // Switch server
    void SwitchServer::ServerLoop()
    {
        int my_tid = MYTID();
        REGISTERAS("SwitchServer");
        IO_NS::PrintTerminal("SwitchServer: Started with tid{%d}\r\n", my_tid);

        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            bool success = SetSwitch(switches.switches[i].switch_num, SWITCH_STATE::STRAIGHT);
            uassert(success && "SwitchServer: SetSwitch failed");
        }

        int sender_tid;
        SwitchRequest request;
        while (true)
        {
            int ret = RECEIVE(&sender_tid, (char *)&request, sizeof(SwitchRequest));
            uassert(ret >= 0 && "SwitchServer: RECEIVE failed");

            // ASSUME SWITCH COMMANDS ARE VALIDATED ALREADY
            SWITCH_STATE alignment = request.alignment;
            int switch_num = request.switch_num;
            IO_NS::PrintTerminal("SwitchServer: Setting switch %d to %c\r\n", request.switch_num, alignment == SWITCH_STATE::STRAIGHT ? 'S' : 'C');

            bool success = SetSwitch(switch_num, alignment);
            if (!success)
            {
                // IO_NS::PrintTerminal("SwitchServer: Switch index %d already in alignment %c\r\n", request.switch_num, request.alignment);
            }
            REPLY(sender_tid, nullptr, 0);
        }
    }

    void StartSwitchServer()
    {
        SwitchServer switch_server;
    }
}