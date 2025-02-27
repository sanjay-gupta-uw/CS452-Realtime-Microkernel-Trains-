#include "../include/marklin_controller.h"
#include "../../kern/syscall.h"
#include "../include/name_server.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"

namespace MARKLIN_NS
{

    MarklinController::MarklinController()
    {
        REGISTERAS("MarklinController");
        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        // trains.SetIOServerTid(marklin_io_tid);
        // switches.setMarklinIoServerTid(marklin_io_tid);
        CLOCK_SERVER_TID = WHOIS("ClockServer");
        // switches.setClockServerTid(clock_tid);

        Trains_NS::init_trains();

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
                break;
            }
            case MARKLIN_REQUEST_TYPE::REVERSE_TRAIN:
            case MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN:
            {
                bool success = Trains_NS::isValidRequest(&request);
                if (success)
                {
                    ret = MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
                    IO_NS::Print(MOVE_CURSOR "MarklinController:: Sent command to MarklinIOServer\r\n", CMD_LOCATION + 2, 1);
                }
                break;
            }

            default:
                break;
            }

            if (ret < 0)
            {
                IO_NS::Print(MOVE_CURSOR "MarklinController::run: Failed to send command to MarklinIOServer\r\n", 55, 0);
                // spin_debug();
            }
        }
    }

    void start_marklin_controller()
    {
        MarklinController controller;
    }
}
