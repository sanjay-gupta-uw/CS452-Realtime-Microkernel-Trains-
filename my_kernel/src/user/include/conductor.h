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
        void UpdateTrainDisplay();
        struct train_task_mapping
        {
            /* data */
            bool isWaitingForCommand;
            int task_id;   // TID
            int train_num; // train number
            Queue<TrainResponse, 8> train_response_queue;

            int speed_level;
            int actual_speed_x10;
            char recent_sensor_bank;
            int recent_sensor_num;
            char next_predicted_bank;
            int next_predicted_num;
            char destination[5];
            int offset;
        };

        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
    };

    void start_conductor();
    void InitializeTrainDisplay();
}

#endif // _CONDUCTOR_H_