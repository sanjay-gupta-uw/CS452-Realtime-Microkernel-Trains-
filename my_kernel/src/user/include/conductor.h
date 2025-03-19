#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../include/marklin_structs.h"

typedef struct FindPathRequest
{
    unsigned char *node_name;
};

namespace Conductor_NS
{
    class Conductor
    {
    private:
        int SWITCH_SERVER_TID;
        int SENSOR_SERVER_TID;
        // Switch_NS::Switches switches;

    public:
        Conductor();
        ~Conductor();

        void ProcessRequest(CMDRequest *req);
        struct train_task_mapping
        {
            /* data */
            int data;
            int train_num;
        };

        Queue<train_task_mapping, NUM_TRAINS> sleeping_trains;

        Track track;
    };

    void start_conductor();
}

#endif // _CONDUCTOR_H_