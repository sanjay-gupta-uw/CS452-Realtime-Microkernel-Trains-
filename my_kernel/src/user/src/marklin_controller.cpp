#include "../include/marklin_controller.h"
#include "../../kern/syscall.h"
#include "../include/name_server.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"

namespace MARKLIN_NS
{

    MarklinController::MarklinController()
    {
        MARKLIN_IO_SERVER_TID = -1;
        CLOCK_SERVER_TID = -1;

        REGISTERAS("MarklinController");
        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        // trains.SetIOServerTid(marklin_io_tid);
        // switches.setMarklinIoServerTid(marklin_io_tid);
        CLOCK_SERVER_TID = WHOIS("ClockServer");
        // switches.setClockServerTid(clock_tid);

        Trains_NS::init_trains();

        for (int i = 0; i < SWITCH_COUNT; ++i)
        {
            MarklinRequest req = Switch_NS::CreateSwitchRequest(i, Switch_NS::SWITCH_STATE::STRAIGHT);
            int ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &req);
            if (ret < 0)
            {
                IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinController:: Failed to send command to MarklinIOServer to init switch %d\r\n", CMD_LOCATION + 2, 0, i);
                // spin_debug();
            }
        }

        run();
    }

    MarklinController::~MarklinController()
    {
    }

    void MarklinController::run()
    {
        int tid;
        MarklinRequest request;
        while (true)
        {
            int ret = -1;
            RECEIVE(&tid, (char *)&request, sizeof(request));

            switch (request.type)
            {
            case MARKLIN_REQUEST_TYPE::SET_SWITCH:
            {
                bool success = Switch_NS::isSwitchCommandValid(&request);
                if (success)
                {
                    ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
                    IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinController:: Sent command to MarklinIOServer\r\n", CMD_LOCATION + 2, 1);
                }

                break;
            }
            case MARKLIN_REQUEST_TYPE::REVERSE_TRAIN:
            case MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN:
            {
                bool success = Trains_NS::isValidRequest(&request);
                if (success)
                {
                    ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
                    IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinController:: Sent command to MarklinIOServer\r\n", CMD_LOCATION + 2, 1);
                }
                break;
            }

            default:
                break;
            }

            if (ret < 0)
            {
                IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "MarklinController:: Failed to send command to MarklinIOServer\r\n", CMD_LOCATION + 2, 0);
                // spin_debug();
            }
            REPLY(tid, (char *)&ret, sizeof(ret));
        }
    }

    void start_marklin_controller()
    {
        MarklinController controller;
    }
}
