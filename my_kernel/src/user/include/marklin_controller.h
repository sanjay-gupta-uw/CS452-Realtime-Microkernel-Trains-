#ifndef _marklin_controller_h_
#define _marklin_controller_h_
#include "../marklin/train.h"
#include "../marklin/switch.h"
#include "../marklin/sensor.h"
#include "marklin_structs.h"

namespace MARKLIN_NS
{
    class MarklinController
    {
    private:
        Sensors_NS::SensorManager sensors; // this must be initialized first since it sends the reset command to marklin
        Trains_NS::Trains trains;
        Switch_NS::Switches switches;

        void run();

        int MARKLIN_IO_SERVER_TID;
        int CLOCK_SERVER_TID;

        void SensorReadAll();

    public:
        MarklinController();
        ~MarklinController();
    };

    void start_marklin_controller();
}

#endif