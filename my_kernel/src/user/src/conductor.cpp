#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
#include "../include/clock_server.h"
#include "../include/name_server.h"
#include "../include/marklin_structs.h"
#include "../marklin/train.h"
#include "../../include/syscall.h"
#include "../../shared_constants.h"
#include <cstring>
#include <cctype>
#include "../include/marklin_io.h"

#define TEST_ENABLED 0
typedef struct IO_REQUEST
{
    unsigned char ch;
};

namespace Conductor_NS
{

    static void StackTest()
    {
        IO_NS::PrintTerminal(CLEAR_SCREEN "StackTest -- testing stack\r\n");
        Stack<int, 10> test_stack;
        for (int i = 0; i < 10; i++)
        {
            test_stack.Push(i);
            IO_NS::PrintTerminal("StackTest -- pushed %d -- size: %d\r\n", i, test_stack.size);
        }
        for (int i = 0; i < 10; i++)
        {
            int popped;
            test_stack.Pop(&popped);
            IO_NS::PrintTerminal("StackTest -- popped %d -- size: %d\r\n", popped, test_stack.size);
        }
        uassert(false && "Conductor::PopSegment -- end of test");
    }

    static void VerifyPath(Stack<PathNode, TRACK_MAX> *path)
    {
        // IO_NS::PrintTerminal(CLEAR_SCREEN MOVE_CURSOR "Conductor::VerifyPath -- Verifying path\r\n", 1, 1);
        PathNode node;
        while (!path->IsEmpty())
        {
            path->Pop(&node);
            IO_NS::PrintTerminal("Conductor::VerifyPath -- %s\r\n", node.node->name);
        }
        uassert(false && "Conductor::VerifyPath -- Test finished -- forced error");
    }

    static void InitializeTrainDisplay()
    {
        // Header
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+----------------------------------------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 0, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|                                Active Trains:                                                            |\r\n", TRAIN_TABLE_Y + 1, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-----------------------------------------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 2, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "| ID | Speed Level | Actual Speed | Current Loc | Next Sensor | Dest | Offset | Total Dist | Remaining Dist|\r\n", TRAIN_TABLE_Y + 3, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-----------------------------------------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 4, TRAIN_TABLE_X);
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            int location_y = TRAIN_TABLE_Y + 5 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|    |             |              |             |             |      |        |            |               |", location_y, TRAIN_TABLE_X);
        }
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+----------------------------------------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 5 + NUM_TRAINS, TRAIN_TABLE_X);
    }

    Conductor::Conductor()
    {
        IO_NS::PrintTerminal("Starting Conductor\r\n");
        // Track track;
        REGISTERAS("Conductor");
        IO_NS::PrintTerminal("Conductor started\r\n");

        CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "Conductor::Error finding ClockServer");

        // initialize train_arr
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            train_task_mapping *train = &train_arr[i];

            train->train_num = -1;
            train->command_messenger = {-1, false};
            train->path_messenger = {-1, false};
            train->sensor_messenger = {-1, false};
            train->last_sent_sensor = nullptr;

            train->total_dist_travelled.Push(0);
            train->total_dist_travelled.Push(0);
        }

        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        track.init(track_id);

        if(track_id == 'A' || track_id == 'a') {
            speed_data.InitializeTrackA();
        } else {
            speed_data.InitializeTrackB();
        }
        speed_data.SetActiveTrack(toupper(track_id));

        REPLY(sender_tid, nullptr, 0);

        // create switch server
        SWITCH_SERVER_TID = CREATE(PRIORITY::DEVICE, Switch_NS::StartSwitchServer);
        uassert(SWITCH_SERVER_TID > 0 && "Conductor::Error creating switch server");
        IO_NS::PrintTerminal("Switch server created with TID %d\r\n", SWITCH_SERVER_TID);
        SEND(SWITCH_SERVER_TID, (char *)&track_id, sizeof(track_id), nullptr, 0); // send track id to switch server

        int ticker_tid = CREATE(PRIORITY::DEVICE, ticker);
        uassert(ticker_tid > 0 && "Conductor::Error creating ticker");
        IO_NS::PrintTerminal("Conductor ticker created with TID %d\r\n", ticker_tid);

        LOOP_START_SENSOR_DATA = {'B', 5};

        InitializeTrainDisplay();
        ConductorLoop();
    }
    Conductor::~Conductor()
    {
    }

    // extracts the top segment from the path and sets the switch
    // need to make sure that the top of the stack is the next sensor to be hit!
    void Conductor::SwitchNextSegment(Stack<PathNode, TRACK_MAX> *path)
    {
        if (path->IsEmpty())
        {
            return;
        }
        // IO_NS::PrintTerminal(CLEAR_SCREEN MOVE_CURSOR "Conductor::SwitchNextSegment -- Processing next segment\r\n", 1, 1);
        Stack<PathNode, 10> temp_stack;
        temp_stack.Clear();
        PathNode node;
        path->Pop(&node);
        // IO_NS::PrintTerminal("Conductor::SwitchNextSegment -- cur_node: %s\r\n", node.node->name);
        temp_stack.Push(node);
        // IO_NS::PrintTerminal("Conductor::SwitchNextSegment -- NODE AFTER: %s\r\n", cur_node.node->name);
        while (!path->IsEmpty())
        {
            PathNode cur_node;
            path->Pop(&cur_node);
            if (cur_node.node->type == NODE_SENSOR)
            {
                // IO_NS::PrintTerminal("Conductor::SwitchNextSegment -- cur_node: %s\r\n", cur_node.node->name);
                temp_stack.Push(cur_node);
                break;
            }

            // IO_NS::PrintTerminal("Conductor::SwitchNextSegment -- %s\r\n", cur_node.node->name);
            if (cur_node.node->type == NODE_BRANCH)
            {
                IO_NS::PrintTerminal("Conductor::SwitchNextSegment -- SETTING Switch %s\r\n", cur_node.node->name);
                setSwitch(cur_node.node->num, cur_node.switch_state);
                // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::SwitchNextSegment -- Switch %s set to %c\r\n", cur_node.node->name, cur_node.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT ? 'S' : 'C');
            }
            temp_stack.Push(cur_node);
        }

        while (!temp_stack.IsEmpty())
        {
            PathNode cur_node;
            temp_stack.Pop(&cur_node);
            path->Push(cur_node);
        }
    }

    void Conductor::setSwitch(int addr, Switch_NS::SWITCH_STATE state)
    {

        char state_char = (state == Switch_NS::SWITCH_STATE::STRAIGHT) ? 'S' : 'C';
        IO_NS::PrintTerminal("CONDUCTOR SET SWITCH:: %d->%c\r\n", addr, state_char);

        // upadte track-switch state
        track.set_switch_state(addr, state_char);
        Switch_NS::SwitchRequest switch_req = {addr, state};
        int retval = SEND(SWITCH_SERVER_TID, (char *)&switch_req, sizeof(Switch_NS::SwitchRequest), nullptr, 0);
        uassert(retval >= 0 && "Conductor::setSwitch -- Error sending switch request to switch server");
    }

    static void get_switch_queue(Stack<PathNode, TRACK_MAX> *path, Queue<PathNode, NUM_SWITCHES> *switch_nodes)
    {
        Stack<PathNode, TRACK_MAX> temp;
        while (!path->IsEmpty())
        {
            PathNode node;
            path->Pop(&node);
            if (node.node->type == NODE_BRANCH)
            {
                switch_nodes->Push(node);
            }
            temp.Push(node);
        }

        while (!temp.IsEmpty())
        {
            PathNode node;
            temp.Pop(&node);
            path->Push(node);
        }
    }

    static SensorStruct name_to_sensor_struct(char sensor_name[4])
    {
        SensorStruct sensor = {};

        sensor.bank = sensor_name[0];
        const char *sensor_name_ptr = sensor_name + 1;
        sensor.id = a2ui((char **)&sensor_name_ptr, 10);

        IO_NS::PrintTerminal("name_to_sensor_struct:: for %c%d\r\n", sensor.bank, sensor.id);
        return sensor;
    }

    void Conductor::ProcessRequest(CMDRequest *req)
    {
        IO_NS::PrintTerminal("Conductor processing request\r\n");
        switch (req->command)
        {
        case COMMAND::SET_SWITCH:
        {
            IO_NS::PrintTerminal("Conductor received SET_SWITCH request for switch addr %d\r\n", req->id);
            setSwitch(req->id, (req->data == 'S') ? Switch_NS::SWITCH_STATE::STRAIGHT : Switch_NS::SWITCH_STATE::CURVED);
            // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::SwitchNextSegment -- Switch %s set to %c\r\n", cur_node.node->name, cur_node.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT ? 'S' : 'C');
            break;
        }
        case COMMAND::ACCELERATE_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received ACCELERATE_TRAIN request for train %d\r\n", req->id);
            int train_num = req->id;
            int speed = req->data;
            IO_NS::PrintTerminal("CONDUCTOR SEARCHING FOR TRAIN %d\r\n", train_num);
            int train_index = get_train_index(train_num);
            if (train_index == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }
            else
            {
                IO_NS::PrintTerminal("Train %d found at index %d, pushing ACCELERATE command to train command queue\r\n", train_num, train_index); train_arr[train_index].speed_level = speed;

                if(speed >= 7 && speed <= 14) {
                    // Get calibrated speed
                    int calibrated_speed_x100 = speed_data.GetSpeed(train_num, speed);
                    train_arr[train_index].actual_speed_x100 = calibrated_speed_x100;
        
                    // Get stopping distance
                    int stopping_dist = speed_data.GetStoppingDistance(train_num, speed);
                    train_arr[train_index].stopping_distance = stopping_dist;
                }
                Conductor::UpdateTrainDisplay();
                train_arr[train_index].train_commands.Push({TRAIN_COMMAND::ACCELERATE, speed});
            }
            break;
        }
        case COMMAND::REVERSE_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received REVERSE_TRAIN request for train %d\r\n", req->id);
            int train_num = req->id;

            int train_index = get_train_index(train_num);
            if (train_index == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }
            else
            {
                IO_NS::PrintTerminal("Train %d found, sending REVERSE command\r\n", train_num);
            }

            train_arr[train_index].train_commands.Push({TRAIN_COMMAND::REVERSE, -1});
            break;
        }
        case COMMAND::SPAWN_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received SPAWN_TRAIN request for train %d\r\n", req->id);

            int spawned_train_tid = CREATE(PRIORITY::DEVICE, Trains_NS::spawn_train);
            uassert(spawned_train_tid > 0);
            IO_NS::PrintTerminal("Train %d spawned with TID %d\r\n", req->id, spawned_train_tid);
            int trainSpawnedSuccess;

            Trains_NS::TrainParams train_params = {req->id, 0};
            // train task should reply with true
            int ret = SEND(spawned_train_tid, (char *)&train_params, sizeof(Trains_NS::TrainParams), (char *)&trainSpawnedSuccess, sizeof(int));
            uassert(ret >= 0 && "Error sending train params to train task");

            // sleeping_trains.Push({spawned_train_tid, req->id});
            train_task_mapping *train = nullptr;
            // trains.SpawnTrain(req->id);
            for (int i = 0; i < NUM_TRAINS; i++)
            {
                train = &train_arr[i];
                if (train->train_num == -1)
                {

                    train->train_num = req->id;

                    train->path_messenger = {-1, false};
                    train->command_messenger = {-1, false};

                    const char *sensor_name = req->src;
                    train->last_sensor = track.get_node_by_name(sensor_name);
                    // train->next_predicted_sensor = track.predict_next_sensor(train->last_sensor);
                    train->next_predicted_sensor = nullptr;

                    train->speed_level = 0;
                    train->actual_speed_x100 = 0;
                    train->stopping_distance = 0;
                    train->offset = 0;
                    memset(train->destination, '-', 5);

                    int total_path_distance = 0;
                    int remaining_distance = 0;
                    int middle_distance = 0;

                    train->go = false;

                    train->train_commands.Clear();
                    train->path.Clear();

                    IO_NS::PrintTerminal("Index: %d, Train %d spawned with last Sensor %s\r\n", i, train->train_num, train->last_sensor->name);
                    break;
                }
            }

            IO_NS::PrintTerminal("Spawning sensor messenger for train %d\r\n", req->id);
            int sensor_messenger_tid = CREATE(PRIORITY::CORE_NOTIFIER, start_sensor_messenger);
            uassert(sensor_messenger_tid > 0 && "Error creating sensor messenger");
            // send train number to sensor messenger
            int retval = SEND(sensor_messenger_tid, (char *)&req->id, sizeof(int), nullptr, 0);
            uassert(retval >= 0 && "Error sending train number to sensor messenger");
            train->sensor_messenger = {sensor_messenger_tid, false};

            IO_NS::PrintTerminal("Train %d spawned successfully!\r\n", req->id);

            // IO_NS::PrintTerminal("Train %d spawned successfully, beginning calibration!\r\n", req->id);

            // train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_NAV_TO_LOOP;
            // IO_NS::PrintTerminal("Train %d, TID: %d calibrating -- start: %s", train->train_num, train->train_task_tid, train->target_sensor_name);

            // CalibrateTrain(train);

            break;
        }
        // based off of Jack's code
        case COMMAND::GOTO:
        {
            // Extract destination from request
            int train_num = req->id;
            int speed = req->data;
            char *dest = req->dest;
            int offset = req->offset;

            int train_index = get_train_index(train_num);
            if (train_index == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }
            if (speed < 7 || speed > 14)
            {
                IO_NS::PrintTerminal("Go To command only go with speed 7 - 14.\r\n");
                return;
            }
            train_task_mapping *train = &train_arr[train_index];
            // IO_NS::PrintTerminal(MOVE_CURSOR "CONDUCTOR::GO_COMMAND:Train %d found, sending it from %s to %s +{%d}\r\n", 1, 1,
            //                      train_num, train->next_predicted_sensor->name, dest, offset);

            memcpy(train->destination, dest, 4);
            train->destination[4] = '\0';
            train->offset = offset;

            track_node *start_sensor = train->last_sensor;
            bool isInitialPath = false;

            if (train->next_predicted_sensor == nullptr)
            {
                train->next_predicted_sensor = track.predict_next_sensor(start_sensor);
                start_sensor = train->next_predicted_sensor;
                isInitialPath = true;
            }

            // extract length of the path
            int total_path = 0;
            track.find_path(start_sensor->name, dest, &train->path, false, offset, &total_path);
            if (train->path.IsEmpty() || total_path == -1)
            {
                IO_NS::PrintTerminal("Conductor::GOTO -- No path found from %s to %s\r\n", start_sensor->name, dest);
                return;
            }

            if (!isInitialPath)
            {
                IO_NS::PrintTerminal("POPPING FIRST SEGMENT SINCE WE ALREADY PASSED OVER IT\r\n");
                // need to decrement the dist to travel by the length of popped segment
                PopSegment(train);
            }

            IO_NS::PrintTerminal("VERIFYING PATH: ");
            train->path.Print();

            PathNode start_node;
            train->path.Pop(&start_node);
            train->path.Push(start_node);

            train->next_predicted_sensor = start_node.node;

            if (train->last_sensor == start_node.node->reverse)
            {
                // IO_NS::PrintTerminal("Last sensor: %s, start node: %s\r\n", train->last_sensor->name, start_node.node->name);
                IO_NS::PrintTerminal("Conductor::GOTO -- Reversing train %d\r\n", train_num);
                train->train_commands.Push({TRAIN_COMMAND::REVERSE, HIGH_SPEED});
            }

            train_arr[train_index].speed_level = speed;

            // Get calibrated speed
            int calibrated_speed_x100 = speed_data.GetSpeed(train_num, speed);
            train_arr[train_index].actual_speed_x100 = calibrated_speed_x100;
    
            // Get stopping distance
            int stopping_dist = speed_data.GetStoppingDistance(train_num, speed);
            train_arr[train_index].stopping_distance = stopping_dist;

            IO_NS::PrintTerminal(COLOR_RED "Jack------- Stopping distance for %d is %d\r\n", speed, train->stopping_distance);

            // Calculate total path distance
            train->total_path_distance = total_path;

            train->remaining_distance = train->total_path_distance - train->stopping_distance;
           
            train->middle_distance = 0;

            train->go = true;

            IO_NS::PrintTerminal(COLOR_RED "Jack------- Total Path: %d, stopping_distance: %d, remaining_distance %d\r\n", train->total_path_distance, train->stopping_distance, train->remaining_distance);

            SwitchNextSegment(&train->path);

            // Accelerates the train
            train->train_commands.Push({TRAIN_COMMAND::ACCELERATE, speed});

            int cur_tick = TIME(CLOCK_SERVER_TID);
            train->last_sensor_trigger_tick = cur_tick;

            Conductor::UpdateTrainDisplay();

            break;
        }
        case COMMAND::STOP_ALL:
        {
            IO_NS::PrintTerminal("Conductor received STOP_ALL request");
            for (int i = 0; i < NUM_TRAINS; i++)
            {
                if (train_arr[i].train_num != -1)
                {
                    IO_NS::PrintTerminal("Sending STOP command to Train %d\r\n", train_arr[i].train_num);
                    train_arr[i].speed_level = 0;
                    train_arr[i].actual_speed_x100 = 0;;
                    train_arr[i].train_commands.Push({TRAIN_COMMAND::ACCELERATE, 0});        
                    train_arr[i].go = false;
                }
                Conductor::UpdateTrainDisplay();
            }
            break;
        }  
        default:
            IO_NS::PrintTerminal("Conductor received INVALID request\r\n");
            break;
        }
    }

    int Conductor::get_train_index(int train_num)
    {
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            if (train_arr[i].train_num == train_num)
            {
                return i;
            }
        }
        return -1;
    }

    void Conductor::UpdateTrainDisplay()
    {
        int display_row = 0;
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            if (train_arr[i].train_num == -1)
                continue;

            const char *next_sensor_name = train_arr[i].next_predicted_sensor ? train_arr[i].next_predicted_sensor->name : "-  ";
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 2, train_arr[i].train_num);
            IO_NS::Print(MOVE_CURSOR "%d ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 7, train_arr[i].speed_level);
            IO_NS::Print(MOVE_CURSOR "%d.%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 21, train_arr[i].actual_speed_x100 / 100, train_arr[i].actual_speed_x100 % 100);
            IO_NS::Print(MOVE_CURSOR "%s ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 36, train_arr[i].last_sensor->name);
            if(train_arr[i].middle_distance != 0) {
                IO_NS::Print(MOVE_CURSOR "%d ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 40, train_arr[i].middle_distance);
            }
            IO_NS::Print(MOVE_CURSOR "%s ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 50, next_sensor_name);
            if(train_arr[i].destination[0] == '-') {
                IO_NS::Print(MOVE_CURSOR "-  ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 64);
                IO_NS::Print(MOVE_CURSOR "-    ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 71);
            }         
            else
            {
                IO_NS::Print(MOVE_CURSOR "%s",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 64, train_arr[i].destination);
                IO_NS::Print(MOVE_CURSOR "%d",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 71, train_arr[i].offset);
            } 
            if(train_arr[i].total_path_distance != 0) {
                IO_NS::Print(MOVE_CURSOR "%d ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 80, train_arr[i].total_path_distance);
            } 
            if(train_arr[i].remaining_distance != 0) {
                IO_NS::Print(MOVE_CURSOR "%d ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 93, train_arr[i].remaining_distance - train_arr[i].middle_distance);
            }     
            //int approx_dist_travelled_in_segment = 0;
            //int known_dist_travelled = 0;
            //train_arr[i].total_dist_travelled.Pop(&approx_dist_travelled_in_segment);
            //train_arr[i].total_dist_travelled.Pop(&known_dist_travelled);
            // IO_NS::PrintTerminal("Conductor::UpdateTrainDisplay -- Train %d: approx_dist_travelled_in_segment: %d, known_dist_travelled: %d\r\n", train_arr[i].train_num, approx_dist_travelled_in_segment, known_dist_travelled);

            //IO_NS::Print(MOVE_CURSOR "%d", TRAIN_TABLE_Y + 5 + 0, TRAIN_TABLE_X + 80, known_dist_travelled + approx_dist_travelled_in_segment);

            display_row++;
        }
    }

    // deprecated
    void Conductor::SetSwitches(Queue<PathNode, NUM_SWITCHES> *switch_nodes)
    {
        uassert(false && "this is deprecated -- use on demand switching");
        while (!switch_nodes->IsEmpty())
        {
            PathNode node;
            switch_nodes->Pop(&node);
            // IO_NS::PrintTerminal("CONDUCTOR::SettingSwitches -- NODE: %s\r\n", node.node->name);

            if (node.node->type == NODE_BRANCH)
            {
                track.set_switch_state(node.node->num, node.switch_state);
                Switch_NS::SwitchRequest switch_req = {node.node->num, node.switch_state};
                int retval = SEND(SWITCH_SERVER_TID, (char *)&switch_req, sizeof(switch_req), nullptr, 0);
                uassert(retval >= 0 && "Error sending switch request");

                // IO_NS::PrintTerminal("CONDUCTOR::Switch %s set to %c\r\n", node.node->name, node.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT ? 'S' : 'C');
                // break;
            }
        }
        // uassert(false && "FORCED ERROR");
    }

    static void clear_path(Stack<PathNode, TRACK_MAX> *path)
    {
        while (!path->IsEmpty())
        {
            PathNode node;
            path->Pop(&node);
        }
    }

    void Conductor::ConductorTest()
    {
        if (!TEST_ENABLED)
        {
            return;
        }

        // test path finding
        IO_NS::PrintTerminal(CLEAR_SCREEN "Conductor testing path finding\r\n");
        Stack<PathNode, TRACK_MAX> path;
        track.find_path("E1", "E14", &path);
        clear_path(&path);
        track.find_path("E9", "D8", &path);
        clear_path(&path);
        track.find_path("A1", "A2", &path);
        clear_path(&path);
        track.find_path("A1", "E7", &path);
        clear_path(&path);
        track.find_path("A2", "E7", &path);
        clear_path(&path);
        track.find_path(LOOP_START_NODE, LOOP_START_NODE, &path, false);
        clear_path(&path);
        // track.find_path(LOOP_START_NODE, "C11", &path);
        IO_NS::PrintTerminal("Conductor finished testing path finding\r\n");
        uassert(false && "Conductor::ConductorLoop: Test finsihed -- forced error");
    }

    void Conductor::ConductorLoop()
    {
        ConductorTest();
        int retval;
        int sender_tid;
        ConductorRequest req;
        int sensor_trigger_count = 0;
        while (true)
        {
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&req, sizeof(ConductorRequest));
            uassert(retval >= 0 && "Error receiving request from terminal");
            // IO_NS::PrintTerminal("Conductor received %s from %d\r\n", req.requestType == RequestType::CMD ? "CMD" : (req.requestType == RequestType::GET_SEGMENT ? "GET_SEGMENT" : (req.requestType == RequestType::GET_CMD ? "GET_CMD" : (req.requestType == RequestType::GET_SENSOR ? "GET_SENSOR" : (req.requestType == RequestType::SENSOR_TRIGGER ? "SENSOR_TRIGGER" : "TICK")))), sender_tid);

            bool sendReply = false;
            if (req.requestType == RequestType::CMD)
            {
                ProcessRequest(&(req.data.cmdRequest));
                sendReply = true;
            }
            else if (req.requestType == RequestType::GET_CMD)
            {
                IO_NS::PrintTerminal("Conductor::ConductorLoop -- Received GET_CMD request from %d -- hanging on until a command is ready\r\n", sender_tid);

                // extract messenger TID
                int train_idx = get_train_index(req.data.train_num);
                uassert(train_idx >= 0 && "Conductor::ConductorLoop -- Error getting train index");

                train_task_mapping *train = train_arr + train_idx;
                MessengerUnit *command_messenger = &train->command_messenger;

                command_messenger->messenger_id = sender_tid; // this only happens once
                command_messenger->sent_reply = false;
                sendReply = false;
            }
            else if (req.requestType == RequestType::GET_SENSOR)
            {
                int train_num = req.data.train_num;
                IO_NS::PrintTerminal("Conductor::ConductorLoop -- Received GET_SENSOR request from %d, for train %d\r\n", sender_tid, train_num);

                int train_index = get_train_index(train_num);
                uassert(train_index >= 0 && "Conductor::ConductorLoop -- Error getting train index");

                train_task_mapping *train = train_arr + train_index;
                MessengerUnit *messenger = &train->sensor_messenger;

                messenger->sent_reply = false;
                messenger->messenger_id = sender_tid;
                sendReply = false;
            }
            else if (req.requestType == RequestType::SENSOR_TRIGGER)
            {
                // IO_NS::PrintTerminal("Conductor::ConductorLoop -- Received SENSOR_TRIGGER request from %d\r\n", sender_tid);
                int triggered_idx = req.data.sensor_trigger_response.sensor_idx;
                int triggered_tick = req.data.sensor_trigger_response.trigger_tick;
                int train_num = req.data.sensor_trigger_response.train_num;
                // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ConductorLoop -- Sensor %d was triggered at tick %d for train %d\r\n", triggered_idx, triggered_tick, train_num);
                // uassert(triggered_idx != 54 && "Conductor::ConductorLoop -- Error: triggered sensor 54 (D7)");

                int train_idx = get_train_index(req.data.sensor_trigger_response.train_num);
                uassert(train_idx >= 0 && "Conductor::ConductorLoop -- Error getting train index");
                train_task_mapping *train = train_arr + train_idx;

                int expected_idx = get_index_from_name((const char *)train->next_predicted_sensor->name);

                IO_NS::PrintTerminal("Expected Sensor Name: %s, expected index: %d, triggered index: %d\r\n", train->next_predicted_sensor->name, expected_idx, triggered_idx);
                // IO_NS::PrintTerminal("EXPECTED NAME: %s, expected index: %d, triggered index: %d\r\n", train->next_predicted_sensor->name, expected_idx, triggered_idx);
                if (expected_idx == triggered_idx)
                {
                    IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ConductorLoop -- Sensor %s was triggered at tick %d for train %d\r\n", train->next_predicted_sensor->name, triggered_tick, train_num);
                    // UPDATE TRAIN POSITION
                    if (train->last_sensor_trigger_tick > 0)
                    {
                        int delta_tick = triggered_tick - train->last_sensor_trigger_tick;
                        // IO_NS::PrintTerminal("Conductor::ConductorLoop -- delta tick: %d\r\n", delta_tick);
                        train->actual_speed_x100 = (train->current_segment_length * 100) / delta_tick;
                        // IO_NS::PrintTerminal("Conductor::ConductorLoop -- actual speed: %d\r\n", train->actual_speed_x100);
                    }
                    else
                    {
                        train->actual_speed_x100 = 0;
                    }
                    // UPDATE STACK SO NEXT SENSOR IS THE TOP
                    train->last_sensor = train->next_predicted_sensor;

                    //int total_distance;
                    //train->total_dist_travelled.Pop(nullptr);                                         // approximate
                    //train->total_dist_travelled.Pop(&total_distance);                                 // known
                    //train->total_dist_travelled.Push(total_distance + train->current_segment_length); // known
                    //train->total_dist_travelled.Push(0);                                              // approximate
                    // IO_NS::PrintTerminal("HERE1\r\n");

                    //new distance calculation logic
                    train->remaining_distance -= train->current_segment_length;
                    train->middle_distance = 0;  // Reset between-sensors distance

                    // Prevent negative remaining distance
                    if (train->remaining_distance < 0) {
                        train->remaining_distance = 0;
                    }

                    IO_NS::PrintTerminal(COLOR_CYAN "CURRENT PATH\r\n");
                    train->path.Print();
                    IO_NS::PrintTerminal(COLOR_CYAN "POPPING SEGMENT\r\n");
                    PopSegment(train);
                    IO_NS::PrintTerminal(COLOR_CYAN "REMAINING PATH\r\n");
                    train->path.Print();

                    // IO_NS::PrintTerminal("HERE2\r\n");
                    train->last_sensor_trigger_tick = triggered_tick;
                    if (!train->path.IsEmpty())
                    {
                        IO_NS::PrintTerminal(COLOR_RED "Conductor::ConductorLoop -- Path is not empty after popping segment\r\n");

                        train->next_predicted_sensor = track.predict_next_sensor(train->last_sensor);
                        IO_NS::PrintTerminal("LATEST SENSOR: %s, NEXT SENSOR: %s\r\n", train->last_sensor->name, train->next_predicted_sensor->name);
                        SwitchNextSegment(&train->path);
                    }
                    Conductor::UpdateTrainDisplay();
                }
                else
                {
                    IO_NS::PrintTerminal(COLOR_RED "Conductor::ConductorLoop -- Expected sensor index: %d, received sensor index: %d\r\n", expected_idx, triggered_idx);
                    uassert(false && "Conductor::ConductorLoop -- Error: triggered sensor is not the expected sensor");
                }
                sendReply = true;
                // while
            }
            else if (req.requestType == RequestType::TICK)
            {
                // IO_NS::PrintTerminal(COLOR_YELLOW "Conductor::ConductorLoop -- Received TICK request from %d\r\n", sender_tid);
                for (int i = 0; i < NUM_TRAINS; ++i)
                {
                    if (train_arr[i].train_num == -1)
                    {
                        continue;
                    }
                    update_position(train_arr + i);
                }
                sendReply = true;
            }

            DispatchCommand();
            Conductor::UpdateTrainDisplay();

            if (sendReply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
        }
        EXIT();
    }

    void Conductor::update_position(train_task_mapping *train)
    {
        if(!train->go) return;
        // IO_NS::PrintTerminal(COLOR_YELLOW "Conductor::ConductorLoop -- Updating position for train %d\r\n", train->train_num);
        int cur_tick = TIME(CLOCK_SERVER_TID);

        int delta_ticks = cur_tick - train->last_sensor_trigger_tick;

        // Update middle distance between sensors
        train->middle_distance = (train->actual_speed_x100 * delta_ticks) / 100;

        IO_NS::PrintTerminal(COLOR_RED "Jack:----------------- train->last_sensor_trigger_tick %d, actual_speedx100 %d.\n", cur_tick, train->last_sensor_trigger_tick, train->actual_speed_x100);

        // Check stopping condition
        int current_remaining = train->remaining_distance - train->middle_distance;

        IO_NS::PrintTerminal("Train %d: remaining_distance %d, middle_distance %d, current_remaining %d,  total_path_distance%d\n",
            train->train_num,
            train->remaining_distance,
            train->middle_distance,
            current_remaining,
            train->total_path_distance);

        if (current_remaining <= 0) {
            IO_NS::PrintTerminal(COLOR_RED "STOPPING Train %d: Reached stopping point!\n", 
                               train->train_num);
            train->train_commands.Push({TRAIN_COMMAND::ACCELERATE, 0});
            train->go = false;
        }
    
        Conductor::UpdateTrainDisplay();
        //train->total_dist_travelled.Pop(nullptr);
        //train->total_dist_travelled.Push(approximate_distance);
    }

    void Conductor::DispatchCommand()
    {
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            if (train_arr[i].train_num == -1)
            {
                continue;
            }
            train_task_mapping *train = train_arr + i;
            MessengerUnit *command_messenger = &train->command_messenger;
            MessengerUnit *sensor_messenger = &train->sensor_messenger;

            if (sensor_messenger->messenger_id > 0 && !sensor_messenger->sent_reply && !train->path.IsEmpty())
            {
                // check if current top node is still what was last sent
                PathNode node;
                train->path.Pop(&node);
                train->path.Push(node);

                // IO_NS::PrintTerminal("NODE NAME: %s\r\n", node.node->name);
                // IO_NS::PrintTerminal("NEXT SENSOR NAME: %s\r\n", train->next_predicted_sensor->name);
                if (train->last_sent_sensor != nullptr && train->last_sent_sensor == node.node)
                {
                    // IO_NS::PrintTerminal(COLOR_CYAN "Conductor::DispatchCommand -- Sensor %s is the same\r\n", node.node->name);
                }
                else
                {
                    IO_NS::PrintTerminal(COLOR_MAGENTA "Conductor::DispatchCommand -- REPLYING sensor %s\r\n", node.node->name);
                    train->last_sent_sensor = node.node;

                    // send sensor to messenger
                    REPLY(sensor_messenger->messenger_id, (char *)&node.node, sizeof(SensorStruct));
                    sensor_messenger->sent_reply = true;
                }
            }

            if (command_messenger->messenger_id > 0 && !command_messenger->sent_reply && !train->train_commands.IsEmpty())
            {
                IO_NS::PrintTerminal(COLOR_CYAN "Conductor::DispatchCommand -- Sending Train Command to messenger %d (train %d)\r\n", command_messenger->messenger_id, train->train_num);
                TrainCommandNotification command;
                train->train_commands.Pop(&command);
                REPLY(command_messenger->messenger_id, (char *)&command, sizeof(TrainCommandNotification));
                command_messenger->sent_reply = true;
            }
        }
    }
    void Conductor::PopSegment(train_task_mapping *train)
    {
        // StackTest();

        Stack<PathNode, TRACK_MAX> *path = &train->path;
        // IO_NS::PrintTerminal("PATH NODES LEFT: %d\r\n", path->size);
        // uassert(train->path.size != 12 && "HERE");
        // path->Print();
        train->current_segment_length = 0;

        PathNode curnode;
        path->Pop(&curnode);

        // IO_NS::PrintTerminal("FIRST NODE POPPED: %s -- path empty? %s -- size: %d\r\n", curnode.node->name, path->IsEmpty() ? "true" : "false", path->size);
        // path->Print();
        // uassert(false && "FORCED ERROR");
        uassert(curnode.node->type == NODE_SENSOR && "Conductor::PopSegment -- popped node is not a sensor");

        int DIR = 0;

        if (strncmp(curnode.node->name, train->destination, 4) == 0)
        {
            IO_NS::PrintTerminal(COLOR_MAGENTA "Conductor::PopSegment -- popped destination node: %s\r\n", curnode.node->name);
            train->current_segment_length = 0;
            train->train_commands.Push({TRAIN_COMMAND::STOP, 0});
            return;
        }

        while (!path->IsEmpty())
        {
            train->current_segment_length += curnode.node->edge[DIR].dist;

            path->Pop(&curnode);
            // IO_NS::PrintTerminal("Conductor::PopSegment -- popped node: %s\r\n", curnode.node->name);
            bool isSensor = false;
            switch (curnode.node->type)
            {
            case NODE_BRANCH:
                DIR = curnode.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT ? 0 : 1;
                // get switch state from curnode
                break;
            case NODE_SENSOR:
                isSensor = true;
                break;
            default:
                DIR = 0;
                break;
            }

            if (isSensor)
            {
                break;
            }
        }
        path->Push(curnode);
        IO_NS::PrintTerminal("Conductor::PopSegment -- current segment length is now: %d\r\n", train->current_segment_length);
    }

    // SENSOR/CONDUCTOR MESSENGER
    void start_sensor_messenger()
    {
        int my_tid = MYTID();
        // Train task should spawn its own messenger
        int train_num;
        int conductor_tid;
        int param_init_retval = RECEIVE(&conductor_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && conductor_tid > 0 && train_num > 0 && "PATH MESSENGER: Error receiving train_num from parent task");
        REPLY(conductor_tid, nullptr, 0);

        IO_NS::PrintTerminal(COLOR_YELLOW "CONDUCTOR_SENSOR MESSENGER{%d/%d}  spawned with TID %d\r\n", my_tid, train_num, my_tid);

        track_node *target_sensor;
        ConductorRequest conductor_request(train_num, RequestType::GET_SENSOR);
        int marklin_io_tid = WHOIS("MarklinIOServer");
        uassert(marklin_io_tid > 0 && "CONDUCTOR_SENSOR MESSENGER: Error finding MarklinIOServer");
        while (true)
        {
            // IO_NS::PrintTerminal(COLOR_RED "ENTER KEY TO CONTINUE");
            // char c = uart_getc(CONSOLE);
            // SEND SEGMENT REQUEST TO CONDUCTOR
            IO_NS::PrintTerminal(COLOR_YELLOW "CONDUCTOR_SENSOR MESSENGER{%d/%d}:: requesting next sensor from Conductor\r\n", my_tid, train_num);
            int retval = SEND(conductor_tid, (char *)&conductor_request, sizeof(ConductorRequest), (char *)&target_sensor, sizeof(track_node));
            uassert(retval >= 0 && "PATH MESSENGER: Error sending SegmentRequest to Conductor");

            IO_NS::PrintTerminal("CONDUCTOR_SENSOR MESSENGER{%d/%d}:: received next sensor from Conductor: %s\r\n", my_tid, train_num, target_sensor->name);

            const char *sensor_name = target_sensor->name;
            MARKLIN_IO_SERVER::SensorListener listener;
            listener.train_num = train_num;
            strncpy((char *)listener.sensor_name, sensor_name, 4);
            MARKLIN_IO_SERVER::IO_REQUEST io_request(&listener);
            IO_NS::PrintTerminal("CONDUCTOR_SENSOR MESSENGER:: Sending request to IO server: %d\r\n", marklin_io_tid);
            SEND(marklin_io_tid, (char *)&io_request, sizeof(MARKLIN_IO_SERVER::IO_REQUEST), nullptr, 0);
            IO_NS::PrintTerminal(COLOR_RED "CONDUCTOR_SENSOR MESSENGER:: RECEIVED REPLY FROM IO SERVER\r\n");
        }
    }

    void ticker()
    {
        int my_tid = MYTID();
        int conductor_tid = MYPARENTTID();
        uassert(conductor_tid > 0 && "CONDUCTOR TICKER:Error finding parent task");

        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "TRAIN TICKER:Error finding ClockServer");

        ConductorRequest tick_message(RequestType::TICK);
        uassert(tick_message.requestType == RequestType::TICK && "Conductor::ticker -- Error creating tick message");
        // IO_NS::PrintTerminal(COLOR_YELLOW "TRAIN TICKER{%d}:: Sending TICK to Train task -- type: %d\r\n", my_tid, tick_message.requestType);
        while (true)
        {
            // IO_NS::PrintTerminal(COLOR_YELLOW "TRAIN TICKER{%d}:: Sending TICK to Train task\r\n", my_tid);
            int retval = SEND(conductor_tid, (char *)&tick_message, sizeof(tick_message), nullptr, 0);
            DELAY(CLOCK_SERVER_TID, 5);
        }
    }

    void start_conductor()
    {
        Conductor conductor;
    }

} // namespace Conductor_NS