#include "../include/marklin_controller.h"
#include "../../include/syscall.h"
#include "../include/name_server.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"

#include "../../rpi.h"
#include "../include/uassert.h"
#include "../marklin/sensor.h"

namespace MARKLIN_NS
{
#define RECEIVE_BYTE_COUNT 8

    static void print_status(bool success)
    {
        return;
        const char *msg = success ? "MarklinController:: Command Successfully Sent to MarklinIOServer\r\n" : "MarklinController:: Invalid Command\r\n";
        const char *colour = success ? COLOR_GREEN : COLOR_RED;
        IO_NS::PrintTerminal("%s %s", colour, msg);
    }
    MarklinController::MarklinController()
    {
        MARKLIN_IO_SERVER_TID = -1;
        CLOCK_SERVER_TID = -1;

        REGISTERAS("MarklinController");
        int mytid = WHOIS("MarklinController");
        uassert(mytid > 0 && "MarklinController:: Failed to register as MarklinController");

        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        // CLOCK_SERVER_TID = WHOIS("ClockServer");

        trains.setIOServerTid(MARKLIN_IO_SERVER_TID);
        // for (;;)
        // {
        //     int time = 6000;
        //     uassert(trains.AccelerateTrain(55, 5) && "MarklinController:: Failed to Accelerate Train 54");
        //     Clock clock;
        //     clock.Delay(time);
        //     uassert(trains.AccelerateTrain(55, 14) && "MarklinController:: Failed to Accelerate Train 54");
        //     clock.Delay(time);
        //     uassert(trains.AccelerateTrain(55, 0) && "MarklinController:: Failed to Accelerate Train 54");
        // }

        // uassert(trains.AccelerateTrain(55, 8) && "MarklinController:: Failed to Accelerate Train 54");

        switches.setIOServerTid(MARKLIN_IO_SERVER_TID);
        switches.Init();

        // switches.s
        run();
    }

    MarklinController::~MarklinController()
    {
    }

    void MarklinController::run()
    {
        int tid;
        MarklinRequest request;
        int ret;

        while (true)
        {
            ret = -1;
            RECEIVE(&tid, (char *)&request, sizeof(request));

            switch (request.type)
            {
            case COMMAND::SET_SWITCH:
            {
                IO_NS::PrintTerminal("MarklinController:: SET_SWITCH: %d %c\r\n", request.id, request.data);
                int switch_addr = request.id;
                Switch_NS::SWITCH_STATE state = (request.data == 'S') ? Switch_NS::SWITCH_STATE::STRAIGHT : Switch_NS::SWITCH_STATE::CURVED;
                bool success = switches.SetSwitch(switch_addr, state);
                IO_NS::PrintTerminal("set switch result: %d\r\n", success);
                ret = success ? 1 : -1;
                print_status(success);
            }
            break;

            case COMMAND::REVERSE_TRAIN:
            {
                IO_NS::PrintTerminal("MarklinController:: REVERSE_TRAIN: %d\r\n", request.id);
                int train_num = request.id;
                bool success = trains.ReverseTrain(train_num);
                ret = success ? 1 : -1;
                print_status(success);
            }
            break;

            case COMMAND::ACCELERATE_TRAIN:
            {
                IO_NS::PrintTerminal("MarklinController:: ACCELERATE_TRAIN: %d %d\r\n", request.id, request.data);
                int train_num = request.id;
                int speed = request.data;
                bool success = trains.AccelerateTrain(train_num, speed);
                ret = success ? 1 : -1;
                print_status(success);
            }
            break;

            default:
                print_status(false);
                break;
            }

            REPLY(tid, (char *)(ret), sizeof(ret));
        }
    }

    void start_marklin_controller()
    {
        int SensorTaskTid = CREATE(PRIORITY::P2, SensorTask);
        uassert(SensorTaskTid >= 0 && "Error starting Sensor Task");
        MarklinController controller;
    }

    void SensorTask()
    {
        REGISTERAS("SensorTask");
        // uassert(4 == 5);
        Sensors_NS::SensorManager sensors;
        sensors.setMarklinIOtid(WHOIS("MarklinIOServer"));
        Clock clock;
        // while (true)
        {
            // sensors.ReadAll(NUM_BANKS); // this will put it to sleep until data is ready
            // sensors.Display();
            // clock.Delay(1000);
            // YIELD();
        }
        EXIT();
    }
}
