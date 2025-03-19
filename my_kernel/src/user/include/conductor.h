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

        int get_train_index(int train_num);

    public:
        Conductor();
        ~Conductor();

        void ProcessRequest(CMDRequest *req);
        void DispatchTrainCommand();
        struct train_task_mapping
        {
            /* data */
            bool isWaitingForCommand;
            int task_id;   // TID
            int train_num; // train number
            Queue<TrainResponse, 8> train_response_queue;
        };

        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
    };

    void start_conductor();
}

#endif // _CONDUCTOR_H_