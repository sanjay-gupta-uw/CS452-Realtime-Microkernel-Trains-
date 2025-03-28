#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../include/marklin_structs.h"

namespace Conductor_NS
{
    class Conductor
    {
    public:
        Conductor();
        ~Conductor();

    private:
        int SWITCH_SERVER_TID;
        int SENSOR_SERVER_TID;
        int sensor_poller_tid;
        // Switch_NS::Switches switches;

        int get_train_index(int train_num);
        void CalibrateTrain(int train_num);
        void SetSwitches(Queue<PathNode, NUM_SWITCHES> *switch_nodes);
        void ProcessRequest(CMDRequest *req);
        void DispatchTrainCommand();
        void UpdateTrainDisplay();

        void ConductorLoop();
        void ConductorTest();

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
            char recent_sensor_id[5];
            char next_predicted_bank;
            int next_predicted_num;
            char next_predicted_id[5];
            char destination[5];
            int offset;
        };

        Switch_NS::SWITCH_STATE switch_states[157] = {};
        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
    };

    void start_conductor();
    //void SensorPoller();
}

#endif // _CONDUCTOR_H_