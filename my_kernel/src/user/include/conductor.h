#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../include/marklin_structs.h"
#include "../include/speed_data.h"
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
            int actual_speed_x100;
            int stopping_distance;
            char destination[5];
            int offset;

            int total_path_distance;    // Total distance of current path
            int remaining_distance;     // Remaining distance to destination
            int middle_distance;        // Distance traveled since last sensor

            Queue<TrainCommandNotification, 3> train_commands;
            Stack<PathNode, TRACK_MAX> path;

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

        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
        SpeedData speed_data;
    };

    void start_sensor_messenger();
    void ticker();
    void start_conductor();
}

#endif // _CONDUCTOR_H_

/*
    // returns segment length, and
    int Conductor::GetSegmentLength(int train_num)
    {
        // CHECK IF WE NEED TO REVERSE/ACCELERATE
        int train_index = get_train_index(train_num);
        if (train_index == -1)
        {
            IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
            return -1;
        }
        else
        {
            IO_NS::PrintTerminal("Train %d found, getting next segment\r\n", train_num);
        }
        train_task_mapping *train = &train_arr[train_index];

        Stack<PathNode, TRACK_MAX> *path = &train_arr[train_index].path;
        if (path->IsEmpty())
        {
            IO_NS::PrintTerminal("CONDUCTOR-GetSegmentLength::PATH IS EMPTY\r\n");
            return 0;
        }
        IO_NS::PrintTerminal("CONDUCTOR-GetSegmentLength::cur sensor: %s, Processing segment until next sensor\r\n", train->last_sensor->name);

        PathNode cur_node;
        path->Pop(&cur_node);
        uassert(cur_node.node->type == NODE_SENSOR && "GET-NEXT-SEGMENT::Next is not a sensor");
        IO_NS::PrintTerminal("Setting recent sensor data to %s\r\n", cur_node.node->name);
        // update recent sensor
        train->last_sensor = cur_node.node;
        int segment_length = 0;
        // cur_node.node->edge[DIR_AHEAD].dist;

        if (path->IsEmpty())
        {
            IO_NS::PrintTerminal("PATH IS EMPTY\r\n");
            return segment_length;
        }

        // process untill next sensor
        bool start = true;
        while (!path->IsEmpty())
        {
            IO_NS::PrintTerminal("Processing segment %s\r\n", cur_node.node->name);
            if (train->start)
            {
                train->train_commands.Push({TRAIN_COMMAND::ACCELERATE, HIGH_SPEED});
                IO_NS::PrintTerminal(COLOR_CYAN "Conductor::GetSegmentLength -- Sending ACCELERATE command to train %d\r\n", train_num);
                train->start = false;
                train->last_node = cur_node.node;
                segment_length = -1;
                path->Pop(&cur_node);
                break;
            }
            else
            {
                if (cur_node.node == train->last_node->reverse)
                {
                    train->train_commands.Push({TRAIN_COMMAND::REVERSE, HIGH_SPEED});
                    IO_NS::PrintTerminal(COLOR_CYAN "Conductor::GetSegmentLength -- Sending REVERSE command to train %d\r\n", train_num);
                }

                if (cur_node.node->type == NODE_SENSOR && !start)
                {
                    break;
                }
                else if (cur_node.node->type == NODE_BRANCH)
                {
                    uassert(cur_node.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT || cur_node.switch_state == Switch_NS::SWITCH_STATE::CURVED);
                    segment_length += cur_node.node->edge[cur_node.switch_state].dist;
                }
                else
                {
                    segment_length += cur_node.node->edge[DIR_AHEAD].dist;
                }
            }

            train->last_node = cur_node.node;
            path->Pop(&cur_node);
            start = false;
        }

        // update current sensor name
        if (cur_node.node->type == NODE_SENSOR)
        {
            path->Push(cur_node);
            IO_NS::PrintTerminal("Setting target sensor data to %s\r\n", cur_node.node->name);
            strncpy(train_arr[train_index].target_sensor_name, cur_node.node->name, 4);
            IO_NS::PrintTerminal("Set target sensor data to %s\r\n", train_arr[train_index].target_sensor_name);
        }

        IO_NS::PrintTerminal("Calculating segment length for train %d: %d\r\n", train_num, segment_length);
        return segment_length;
    }
    */