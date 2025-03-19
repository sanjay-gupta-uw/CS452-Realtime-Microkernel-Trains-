#include "switch.h"
// #include "../clock.h"
// #include "../include/io.h"
// #include "../include/name_server.h"
#include "../include/marklin_io.h"
#include "../include/clock_server.h"
#include "../../shared_constants.h"
#include "../include/uassert.h"
#include "../include/name_server.h"
namespace Switch_NS
{
    // ********* Switch Class *********
    Switch::Switch()
    {
        address = -1;
        MARKLIN_IO_SERVER_TID = -1;
    }

    Switch::Switch(int address, int MARKLIN_IO_SERVER_TID)
        : address(address), MARKLIN_IO_SERVER_TID(MARKLIN_IO_SERVER_TID)
    {
    }

    Switch::~Switch()
    {
    }

    void Switch::SetSwitch(SWITCH_STATE ALIGNMENT)
    {
        uint32_t switch_cmd = (ALIGNMENT == SWITCH_STATE::STRAIGHT) ? STRAIGHT_CMD : CURVED_CMD;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, switch_cmd, address};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
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
        REGISTERAS("SwitchServer");
        int MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID >= 0 && "SwitchServer: WHOIS failed");

        const int switch_addrs[NUM_SWITCHES] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                11, 12, 13, 14, 15, 16, 17, 18,
                                                0x9A, 0x9B, 0x9C, 0x99};

        Switch switches[NUM_SWITCHES];
        for (int i = 0; i < NUM_SWITCHES; ++i)
        {
            switches[i] = Switch(switch_addrs[i], MARKLIN_IO_SERVER_TID);
        }

        int timer_tid = CREATE(PRIORITY::P0, switch_timer);
        uassert(timer_tid >= 0 && "SwitchServer: failed to CREATE timer");
        bool isTimerActive = false;

        int sender_tid;
        int last_switch_index = -1;
        SwitchRequest request;
        while (true)
        {
            int ret = RECEIVE(&sender_tid, (char *)&request, sizeof(SwitchRequest));
            uassert(ret >= 0 && "SwitchServer: RECEIVE failed");
            if (request.isTimer && last_switch_index >= 0)
            {
                isTimerActive = false;
                switches[last_switch_index].SendOffCommand();
                // verify this is sending off command
            }
            else
            {
                // ASSUME SWITCH COMMANDS ARE VALIDATED ALREADY
                last_switch_index = request.switch_index;
                switches[request.switch_index].SetSwitch(request.switch_state == 'S' ? SWITCH_STATE::STRAIGHT : SWITCH_STATE::CURVED);
                if (!isTimerActive)
                {
                    isTimerActive = true;
                    REPLY(timer_tid, nullptr, 0);
                }
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