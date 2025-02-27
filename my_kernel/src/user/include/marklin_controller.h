#ifndef _marklin_controller_h_
#define _marklin_controller_h_
#include "../marklin/train.h"
#include "../marklin/switch.h"
#include "marklin_structs.h"

namespace MARKLIN_NS
{
    class MarklinController
    {
        void run();
        // Switches switches;

        int MARKLIN_IO_SERVER_TID;
        int CLOCK_SERVER_TID;

    public:
        MarklinController();
        ~MarklinController();
    };

    void start_marklin_controller();
}

#endif