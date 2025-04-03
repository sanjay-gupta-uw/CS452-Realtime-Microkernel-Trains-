#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../include/marklin_structs.h"
namespace Conductor_NS
{
#define LOOP_START_NODE "B5p"

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
        int CLOCK_SERVER_TID;
        // Switch_NS::Switches switches;

        int get_train_index(int train_num);
        void SetSwitches(Queue<PathNode, NUM_SWITCHES> *switch_nodes);
        void ProcessRequest(CMDRequest *req);
        void UpdateTrainDisplay();

        void ConductorLoop();
        void ConductorTest();
        int GetSegmentLength(int train_num); // deprecate this

        // Queue<TrainResponse, 8> train_messages;
        Queue<PathNode, NUM_SWITCHES> loop_switch_config;
        struct MessengerUnit
        {
            int messenger_id;
            bool sent_reply;
        };

        struct train_task_mapping
        {
            /* data */
            int train_num; // train number

            MessengerUnit command_messenger;
            MessengerUnit path_messenger;
            MessengerUnit sensor_messenger;

            track_node *last_sensor;
            track_node *next_predicted_sensor;

            int speed_level;
            int actual_speed_x10;
            int offset;
            char destination[5];

            Queue<TrainCommandNotification, 3> train_commands;
            Stack<PathNode, TRACK_MAX> path;
            Queue<PathNode, 20> reserved_nodes;

            int current_segment_length = 0;
            Stack<int, 2> total_dist_travelled;

            track_node *last_sent_sensor;
            uint32_t last_sensor_trigger_tick = 0;
        };
        void update_position(train_task_mapping *train);

        void SwitchNextSegment(Stack<PathNode, TRACK_MAX> *path);

        void SendSegmentToMessenger(int messenger_tid, train_task_mapping *train);
        void CalibrateTrain(train_task_mapping *train);
        void UpdateCalibrationStage(train_task_mapping *train);
        void setSwitch(int addr, Switch_NS::SWITCH_STATE state);
        void PopSegment(train_task_mapping *train);
        bool ReserveSegment(train_task_mapping *train);
        void releaseSegment(train_task_mapping *train);

        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
    };

    void start_sensor_messenger();
    void ticker();
    void start_conductor();
}

#endif // _CONDUCTOR_H_
