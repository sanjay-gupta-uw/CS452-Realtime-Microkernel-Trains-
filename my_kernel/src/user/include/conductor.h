#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../include/marklin_structs.h"
namespace Conductor_NS
{
#define LOOP_START_NODE "B5"

    // CALIBRATION STAGES
    typedef enum
    {
        NONE,
        CALIBRATE_NAV_TO_LOOP,
        CALIBRATE_LOOP,
        CALIBRATE_NAV_TO_STOP
    } CALIBRATION_STAGE;

    class Conductor
    {
    public:
        Conductor();
        ~Conductor();

    private:
        void DispatchCommand();
        SensorStruct LOOP_START_SENSOR_DATA;
        int SWITCH_SERVER_TID;
        int SENSOR_SERVER_TID;
        // Switch_NS::Switches switches;

        int get_train_index(int train_num);
        void SetSwitches(Queue<PathNode, NUM_SWITCHES> *switch_nodes);
        void ProcessRequest(CMDRequest *req);
        void UpdateTrainDisplay();

        void ConductorLoop();
        void ConductorTest();
        int GetSegmentLength(int train_num);

        // Queue<TrainResponse, 8> train_messages;
        Queue<PathNode, NUM_SWITCHES> loop_switch_config;
        struct train_task_mapping
        {
            /* data */
            CALIBRATION_STAGE calibration_stage;
            int train_num;         // train number
            int path_messenger_id; // TID
            bool sent_reply_to_path_messenger;
            int cmd_messenger_id; // TID
            bool sent_reply_to_cmd_messenger;

            int train_task_tid;
            char target_sensor_name[4];

            int speed_level;
            int actual_speed_x10;
            char recent_sensor_bank;
            int recent_sensor_num;
            char next_predicted_bank;
            int next_predicted_num;
            char destination[5];
            int offset;

            Queue<TrainCommandNotification, 3> train_commands;
            Stack<PathNode, TRACK_MAX> path;
            bool start;
            track_node *last_node;
        };

        void SendSegmentToMessenger(int messenger_tid, train_task_mapping *train);
        void CalibrateTrain(train_task_mapping *train);
        void UpdateCalibrationStage(train_task_mapping *train);

        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
    };

    void start_conductor();
}

#endif // _CONDUCTOR_H_