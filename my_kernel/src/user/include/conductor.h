#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../../containers/pqueue.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../include/marklin_structs.h"
#include "../include/speed_data.h"
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

            track_node *destination_node; // DESTINATION NODE

            track_node *last_sensor;               // LAST TRIGGERED SENSOR
            track_node *last_sent_sensor;          // LAST SENSOR SENT TO MARKLIN WATCHER
            track_node *last_sent_sensor_safety;   // LAST SAFETY SENSOR SENT TO MARKLIN WATCHER
            uint32_t last_sensor_trigger_tick = 0; // LAST TICK WHEN SENSOR WAS TRIGGERED

            int speed_level;
            int actual_speed_x100;
            int stopping_distance;

            int start_offset;
            char destination[5];
            int offset;

            int go_speed;
            int slow_down_speed;

            int total_path_distance; // Total distance of current path
            PQueue<track_node, 3> stopping_targets;
            bool conflict_exists;

            bool isMoving;
            bool reach_first_sensor;
            bool check_if_blocked;
            bool isTrainBlocked;

            bool auto_mode;
            bool demo_mode;

            Queue<TrainCommandNotification, 5> train_commands;
            Stack<PathNode, TRACK_MAX> path;
            Queue<PathNode, TRACK_MAX> reserved_nodes;
            int reserved_sensors_count;

            int current_segment_length = 0;
            int total_dist_travelled;
        };
        int custom_rand();

        void GenerateAndSendNewCommand(train_task_mapping *train);
        void GenerateAndSendDemoCommand(train_task_mapping *train);

        void SwitchNextSegment(Stack<PathNode, TRACK_MAX> *path);

        void SendSegmentToMessenger(int messenger_tid, train_task_mapping *train);
        void CalibrateTrain(train_task_mapping *train);
        void UpdateCalibrationStage(train_task_mapping *train);
        void setSwitch(int addr, SwitchState state);
        void PopSegment(train_task_mapping *train);

        // bool ReserveSegment(train_task_mapping *train);
        bool ReservePath(train_task_mapping *train);
        void ReleaseSegment(train_task_mapping *train);
        int GetReservedPathLength(train_task_mapping *train);

        void get_sensors_to_listen_to(train_task_mapping *train, track_node *&first_sensor, track_node *&second_sensor, track_node *&third_sensor);
        void ProcessSensorTrigger(SensorTriggerResponse *trigger_response);

        void StopTrainConflict(train_task_mapping *train);
        void StopAllTrains();
        void ReleasePath(train_task_mapping *train);

        train_task_mapping train_arr[NUM_TRAINS];
        Track track;
        SpeedData speed_data;

        int command_index_0;
        int command_index_1;
        int command_index_2;
    };

    void start_sensor_messenger();
    void start_conductor();

}

#endif // _CONDUCTOR_H_
