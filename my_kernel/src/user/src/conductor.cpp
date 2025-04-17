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

static constexpr char *FORBIDDEN_SENSORS[] = {
    "A3", "B16", "D6", "D7"};

static constexpr char *SENSOR_IDS_0[] = {
    "A15", "E5", "E2", "B16", "D12", "A3", "E9", "B15"};

static constexpr int OFFSETS_0[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static constexpr int SPEEDS_0[] = {
    7, 7, 7, 7, 7, 7, 7, 7};

static constexpr char *SENSOR_IDS_1[] = {
    "A14", "C14", "A3", "E5", "E2", "B16", "D12", "A3"};

static constexpr int OFFSETS_1[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static constexpr int SPEEDS_1[] = {
    7, 7, 7, 7, 7, 7, 7, 7};

namespace Conductor_NS
{

    static void STOP(int train_num, int MARKLIN_IO_SERVER_TID)
    {
        int train_speed = 0;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, train_speed + 16, train_num};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    static void releaseNode(track_node *node)
    {
        node->who_reserved_me = -1;
        node->reverse->who_reserved_me = -1;
    }

    static void reserveNode(track_node *node, int train_num)
    {
        node->who_reserved_me = train_num;
        node->reverse->who_reserved_me = train_num;
    }

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
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "| ID | Speed Level | Actual Speed | Current Loc | Next Sensor | Dest | Offset | Total Dist | Dist Travelled|\r\n", TRAIN_TABLE_Y + 3, TRAIN_TABLE_X);
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
            train->last_sent_sensor_safety = nullptr;

            train->total_dist_travelled = 0;
        }

        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        track.init(track_id);
        command_index = 0;

        if (track_id == 'A' || track_id == 'a')
        {
            speed_data.InitializeTrackA();
        }
        else
        {
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

        int train_nums[NUM_TRAINS] = {1, 54, 55, 58, 77};
        // stop all trains
        int MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            IO_NS::PrintTerminal("Conductor::Conductor init -- stopping train %d\r\n", train_nums[i]);
            STOP(train_nums[i], MARKLIN_IO_SERVER_TID);
        }

        InitializeTrainDisplay();

        StopAllTrains();
        ConductorLoop();
    }
    Conductor::~Conductor()
    {
    }

    // extracts the top segment from the path and sets the switch
    // need to make sure that the top of the stack is the next sensor to be hit!
    void Conductor::SwitchNextSegment(Stack<PathNode, TRACK_MAX> *path)
    {
        // NEED TO ONLY SET SWITCHES IF NODES RESERVED
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

    void Conductor::setSwitch(int addr, SwitchState state)
    {

        char state_char = (state == SwitchState::STRAIGHT) ? 'S' : 'C';
        IO_NS::PrintTerminal("CONDUCTOR SET SWITCH:: %d->%c\r\n", addr, state_char);

        // upadte track-switch state
        track.set_switch_state(addr, state_char);
        Switch_NS::SwitchRequest switch_req = {addr, (Switch_NS::SWITCH_STATE)state};
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
            setSwitch(req->id, (req->data == 'S') ? SwitchState::STRAIGHT : SwitchState::CURVED);
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
                IO_NS::PrintTerminal("Train %d found at index %d, pushing ACCELERATE command to train command queue\r\n", train_num, train_index);
                train_arr[train_index].speed_level = speed;

                if (speed >= 7 && speed <= 14)
                {
                    // Get calibrated speed
                    int calibrated_speed_x100 = speed_data.GetSpeed(train_num, speed);
                    train_arr[train_index].actual_speed_x100 = calibrated_speed_x100;

                    // Get stopping distance
                    int stopping_dist = speed_data.GetStoppingDistance(train_num, speed);
                    train_arr[train_index].stopping_distance = stopping_dist;
                }
                Conductor::UpdateTrainDisplay();
                train_arr[train_index].train_commands.Push({TRAIN_COMMAND::ACCELERATE, speed});
                train_arr[train_index].isMoving = true;
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

            train_task_mapping *train = nullptr;
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

                    train->speed_level = 0;
                    train->actual_speed_x100 = 0;
                    // train->start_offset = req->data;
                    train->stopping_distance = 0;
                    train->offset = req->offset;
                    IO_NS::PrintTerminal(COLOR_RED "OFFSET: %d\r\n", req->offset);
                    memset(train->destination, '-', 5);

                    int total_path_distance = 0;
                    int remaining_distance = 0;
                    int middle_distance = 0;

                    train->isTrainBlocked = false;

                    train->isMoving = false;
                    train->reach_first_sensor = false;

                    train->train_commands.Clear();
                    train->path.Clear();
                    train->reserved_nodes.Clear();

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
            UpdateTrainDisplay();

            // PUT TRACK IN LOOP
            // track.find_path("C10", "C10", &train->path, false, 0, &train->total_path_distance, false, train->train_num);
            // uassert(false && "FORCED ERROR");
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

            train->path.Clear();
            while (!train->reserved_nodes.IsEmpty())
            {
                PathNode node;
                train->reserved_nodes.Pop(&node);
                releaseNode(node.node);
            }
            // train->reserved_nodes.Clear();
            train->reserved_sensors_count = 0;

            memcpy(train->destination, dest, 4);
            train->destination[4] = '\0';
            train->destination_node = track.get_node_by_name(dest);

            // train->offset = offset;

            IO_NS::PrintTerminal("Conductor::GOTO -- Train %d going to %s from %s\r\n", train_num, dest, train->last_sensor->name);
            track_node *start_sensor = train->last_sensor;

            // extract length of the path
            IO_NS::PrintTerminal("Conductor::GOTO -- Finding conflict free path from %s to %s for train %d\r\n", start_sensor->name, dest, train_num);
            int total_path_length = 0;
            track.find_path(start_sensor->name, dest, &train->path, false, offset, &total_path_length, true, train->train_num);
            if (total_path_length == -1)
            {
                IO_NS::PrintTerminal("Conductor::GOTO -- No unreserved path found from %s to %s for train %d\r\n", start_sensor->name, dest, train_num);
                IO_NS::PrintTerminal("Conductor::GOTO -- searching for alternative path (won't ignore reserved segments)\r\n");
                track.find_path(start_sensor->name, dest, &train->path, false, offset, &total_path_length);
                if (total_path_length == -1)
                {
                    IO_NS::PrintTerminal("Conductor::GOTO -- No path found from %s to %s for train %d\r\n", start_sensor->name, dest, train_num);
                    return;
                }
            }

            if (train->path.IsEmpty() || total_path_length == -1)
            {
                IO_NS::PrintTerminal("Conductor::GOTO -- No path found from %s to %s\r\n", start_sensor->name, dest);
                return;
            }
            train->isTrainBlocked = !ReservePath(train);
            IO_NS::PrintTerminal(COLOR_GREEN "Conductor::GOTO -- Reserved path for train %d\r\n", train_num);
            train->check_if_blocked = train->isTrainBlocked;

            // print reserved nodes
            if (train->isTrainBlocked)
            {
                int dist_to_conflict;
                track_node conflict_node;
                // train->stopping_targets.Peek(&conflict_node, &dist_to_conflict);
                // IO_NS::PrintTerminal("Conductor::GOTO -- PATH NOT FULLY RESERVED for TRAIN %d -- STOP NEEDED IN %d mm AT %s\r\n", train_num, dist_to_conflict, conflict_node.name);
                // uassert(false && "Conductor::GOTO -- PATH NOT FULLY RESERVED -- STOP NEEDED");
            }
            // uassert(false && "Conductor::GOTO -- FORCED ERROR ");

            SwitchNextSegment(&train->path);
            IO_NS::PrintTerminal("POPPING FIRST SEGMENT SINCE WE ALREADY PASSED OVER IT\r\n");
            PopSegment(train);
            IO_NS::PrintTerminal(COLOR_GREEN "Conductor::GOTO -- Reserved path after popping:\r\n");
            train->reserved_nodes.Print();

            IO_NS::PrintTerminal("VERIFYING PATH: ");
            train->path.Print();

            PathNode start_node;
            train->path.Pop(&start_node);
            train->path.Push(start_node);

            if (train->last_sensor == start_node.node->reverse)
            {
                // IO_NS::PrintTerminal("Last sensor: %s, start node: %s\r\n", train->last_sensor->name, start_node.node->name);
                IO_NS::PrintTerminal("Conductor::GOTO -- Reversing train %d\r\n", train_num);
                train->train_commands.Push({TRAIN_COMMAND::REVERSE, speed});
            }

            // this should be done after the train is moving
            train_arr[train_index].speed_level = speed;
            SwitchNextSegment(&train->path);
            IO_NS::PrintTerminal(COLOR_GREEN "Conductor::GOTO -- Reserved segment after:\r\n");
            train->reserved_nodes.Print();

            // Get calibrated speed
            int calibrated_speed_x100 = speed_data.GetSpeed(train_num, speed);
            train_arr[train_index].actual_speed_x100 = calibrated_speed_x100;

            // Get stopping distance
            int stopping_dist = speed_data.GetStoppingDistance(train_num, speed);
            train_arr[train_index].stopping_distance = stopping_dist;

            IO_NS::PrintTerminal(COLOR_RED "Jack------- Stopping distance for %d is %d\r\n", speed, train->stopping_distance);

            // Calculate total path distance
            train->total_path_distance = total_path_length;
            track_node *destination_node = track.get_node_by_name(dest);

            train->stopping_targets.Clear();
            train->stopping_targets.Push(*destination_node, total_path_length - train->stopping_distance - train->offset);
            int stopping_target;
            train->stopping_targets.Peek(destination_node, &stopping_target);
            IO_NS::PrintTerminal(COLOR_GREEN "Conductor::GOTO -- Stopping target: %s in %d mm\r\n", destination_node->name, stopping_target);
            train->middle_distance = 0;
            train->reach_first_sensor = false;

            int cur_tick = TIME(CLOCK_SERVER_TID);

            train->last_sensor_trigger_tick = cur_tick;

            Conductor::UpdateTrainDisplay();

            break;
        }
        case COMMAND::STOP_ALL:
        {
            StopAllTrains();
        }
        case COMMAND::AUTO:
        {
            for (int i = 0; i < NUM_TRAINS; ++i)
            {
                if (train_arr[i].train_num != -1)
                {
                    train_arr[i].auto_mode = true;

                    // Generate random parameters
                    GenerateAndSendNewCommand(&train_arr[i]);
                }
            }
            IO_NS::PrintTerminal("Auto mode enabled for all trains\r\n");
            break;
        }
        case COMMAND::GO_AGAIN:
        {
            int train_num = req->id;
            IO_NS::PrintTerminal(COLOR_GREEN "CONDUCTOR::PROCESSREQUEST -- GO AGAIN -- train num: %d\r\n", train_num);
            int train_index = get_train_index(train_num);
            GenerateAndSendNewCommand(&train_arr[train_index]);
            IO_NS::PrintTerminal(COLOR_GREEN "CONDUCTOR::PROCESSREQUEST -- GO AGAIN EXECUTED FOR train num: %d\r\n", train_num);
            break;
        }
        default:
            IO_NS::PrintTerminal("Conductor received INVALID request\r\n");
            break;
        }
    }

    void Conductor::StopAllTrains()
    {
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            if (train_arr[i].train_num != -1)
            {
                IO_NS::PrintTerminal(COLOR_RED "Sending STOP command to Train %d\r\n", train_arr[i].train_num);
                train_arr[i].speed_level = 0;
                train_arr[i].actual_speed_x100 = 0;
                train_arr[i].train_commands.Push({TRAIN_COMMAND::STOP, 0});
                train_arr[i].isMoving = false;
            }
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

            train_task_mapping *train = &train_arr[i];
            const char *next_sensor_name = (train->last_sent_sensor && train->last_sensor != train->last_sent_sensor) ? train_arr[i].last_sent_sensor->name : "N/A";
            // IO_NS::PrintTerminal("Conductor::UpdateTrainDisplay -- Train %d: next sensor -> {%s}\r\n", train_arr[i].train_num, next_sensor_name);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 2, train_arr[i].train_num);
            IO_NS::Print(MOVE_CURSOR "%d ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 7, train_arr[i].speed_level);
            IO_NS::Print(MOVE_CURSOR "%d.%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 21, train_arr[i].actual_speed_x100 / 100, train_arr[i].actual_speed_x100 % 100);
            IO_NS::Print(MOVE_CURSOR "%s ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 36, train_arr[i].last_sensor->name);
            if (train_arr[i].middle_distance != 0)
            {
                IO_NS::Print(MOVE_CURSOR "%d ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 40, train_arr[i].middle_distance);
            }
            IO_NS::Print(MOVE_CURSOR "%s ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 50, next_sensor_name);
            if (train_arr[i].destination[0] == '-')
            {
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
            if (train_arr[i].total_path_distance != 0)
            {
                IO_NS::Print(MOVE_CURSOR "%d ",
                             TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 80, train_arr[i].total_path_distance);
            }

            IO_NS::Print(MOVE_CURSOR "%d ",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 93, train_arr[i].middle_distance + train_arr[i].middle_distance);
            // int approx_dist_travelled_in_segment = 0;
            // int known_dist_travelled = 0;
            // train_arr[i].total_dist_travelled.Pop(&approx_dist_travelled_in_segment);
            // train_arr[i].total_dist_travelled.Pop(&known_dist_travelled);
            //  IO_NS::PrintTerminal("Conductor::UpdateTrainDisplay -- Train %d: approx_dist_travelled_in_segment: %d, known_dist_travelled: %d\r\n", train_arr[i].train_num, approx_dist_travelled_in_segment, known_dist_travelled);

            // IO_NS::Print(MOVE_CURSOR "%d", TRAIN_TABLE_Y + 5 + 0, TRAIN_TABLE_X + 80, known_dist_travelled + approx_dist_travelled_in_segment);

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
                Switch_NS::SwitchRequest switch_req = {node.node->num, (Switch_NS::SWITCH_STATE)node.switch_state};
                int retval = SEND(SWITCH_SERVER_TID, (char *)&switch_req, sizeof(switch_req), nullptr, 0);
                uassert(retval >= 0 && "Error sending switch request");

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
            // track_node *BR2 = track.get_node_by_name("BR2");
            // IO_NS::PrintTerminal(COLOR_CYAN "Conductor::ConductorLoop -- BR2: reserved by %d\r\n", BR2->who_reserved_me);
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&req, sizeof(ConductorRequest));
            uassert(retval >= 0 && "Error receiving request from terminal");
            IO_NS::PrintTerminal("Conductor received %s from %d\r\n", req.requestType == RequestType::CMD ? "CMD" : (req.requestType == RequestType::GET_SEGMENT ? "GET_SEGMENT" : (req.requestType == RequestType::GET_CMD ? "GET_CMD" : (req.requestType == RequestType::GET_SENSOR ? "GET_SENSOR" : (req.requestType == RequestType::SENSOR_TRIGGER ? "SENSOR_TRIGGER" : "TICK")))), sender_tid);

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
            // REPLACE WITH SENSOR_TRIGGER FUNCTION
            else if (req.requestType == RequestType::SENSOR_TRIGGER)
            {
                IO_NS::PrintTerminal(COLOR_BLUE "TYPE CHARACTER TO CONTINUE RUNNING\r\n");
                unsigned char ch = uart_getc(CONSOLE);

                ProcessSensorTrigger(&req.data.sensor_trigger_response);
                sendReply = true;
            }
            else if (req.requestType == RequestType::TICK)
            {
                // IO_NS::PrintTerminal(COLOR_YELLOW "Conductor::ConductorLoop -- Received TICK request from %d\r\n", sender_tid);
                for (int i = 0; i < NUM_TRAINS; ++i)
                {
                    if (train_arr[i].train_num == -1 || train_arr[i].speed_level <= 0)
                    {
                        continue;
                    }
                    update_position(train_arr + i);
                }
                sendReply = true;
            }

            DispatchCommand();

            UpdateTrainDisplay();

            if (sendReply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
        }
        EXIT();
    }

    int Conductor::custom_rand()
    {
        int custom_rand_seed = TIME(CLOCK_SERVER_TID);
        // Simple Linear Congruential Generator
        custom_rand_seed = (214013 * custom_rand_seed + 2531011);
        return (custom_rand_seed >> 16) & 0x7FFF;
    }

    void Conductor::GenerateAndSendNewCommand(train_task_mapping *train)
    {
        char dest_sensor[5] = {0};
        int offset;
        int speed;

        bool is_forbidden;
        const int num_forbidden = sizeof(FORBIDDEN_SENSORS) / sizeof(FORBIDDEN_SENSORS[0]);

        /*
                // Get values from lists for testing
                int list_size;
                if (train->train_num == 54)
                {
                    dest_sensor = SENSOR_IDS_0[command_index];
                    offset = OFFSETS_0[command_index];
                    speed = SPEEDS_0[command_index];
                    list_size = sizeof(SENSOR_IDS_0)/sizeof(SENSOR_IDS_0[0]);
                }
                else if(train->train_num == 77)
                {
                    dest_sensor = SENSOR_IDS_1[command_index];
                    offset = OFFSETS_1[command_index];
                    speed = SPEEDS_1[command_index];
                    list_size = sizeof(SENSOR_IDS_1)/sizeof(SENSOR_IDS_1[0]);
                }
                else {
                    dest_sensor = SENSOR_IDS_1[command_index];
                    offset = OFFSETS_1[command_index];
                    speed = SPEEDS_1[command_index];
                    list_size = sizeof(SENSOR_IDS_1)/sizeof(SENSOR_IDS_1[0]);
                }
        */

        /*
        // random test
        for (int i = 0; i <= 10; i++)
        {
            // Generate random sensor components
            dest_sensor[5] = {0};
            char bank = 'A' + (custom_rand() % 5);     // A-E
            int sensor_num = (custom_rand() % 14) + 1; // 1-14
            IO_NS::PrintTerminal(COLOR_YELLOW "Random generated bank: %c, Random generated sensor num: %d%\r\n", bank, sensor_num);

            // Manually construct sensor name
            dest_sensor[0] = bank;
            if (sensor_num >= 10)
            {
                dest_sensor[1] = '0' + (sensor_num / 10); // Tens digit
                dest_sensor[2] = '0' + (sensor_num % 10); // Units digit
                dest_sensor[3] = '\0';
            }
            else
            {
                dest_sensor[1] = '0' + sensor_num; // Single digit
                dest_sensor[2] = '\0';
            }
            IO_NS::PrintTerminal(COLOR_YELLOW "Random generated: %s\r\n", dest_sensor);
        }
        */

        // Generate valid random sensor
        do
        {
            dest_sensor[5] = {0};

            // Generate random sensor components
            char bank = 'A' + (custom_rand() % 5);     // A-E
            int sensor_num = (custom_rand() % 14) + 1; // 1-14

            // Manually construct sensor name
            dest_sensor[0] = bank;
            if (sensor_num >= 10)
            {
                dest_sensor[1] = '0' + (sensor_num / 10); // Tens digit
                dest_sensor[2] = '0' + (sensor_num % 10); // Units digit
                dest_sensor[3] = '\0';
            }
            else
            {
                dest_sensor[1] = '0' + sensor_num; // Single digit
                dest_sensor[2] = '\0';
            }

            IO_NS::PrintTerminal(COLOR_YELLOW "Random generated: %s\r\n", dest_sensor);

            // Check against forbidden list
            is_forbidden = false;
            for (int i = 0; i < num_forbidden; ++i)
            {
                if (strcmp(dest_sensor, FORBIDDEN_SENSORS[i]) == 0)
                {
                    is_forbidden = true;
                    break;
                }
            }

            IO_NS::PrintTerminal(COLOR_YELLOW "Is this sensor forbiden: %d\r\n", is_forbidden);
        } while (is_forbidden);

        offset = 0;
        speed = 7;

        // Create command
        ConductorRequest req(COMMAND::GOTO,
                             train->train_num,
                             speed,
                             dest_sensor,
                             dest_sensor,
                             offset);

        ProcessRequest(&req.data.cmdRequest);

        IO_NS::PrintTerminal(COLOR_YELLOW "Automated GOTO: train %d go to %s %d with speed %d\r\n", train->train_num, dest_sensor, offset, speed);

        /*
        // Move to next index
        command_index++;

        // Optional: Reset index if needed
        if (command_index >= list_size) {
            command_index = 0;
        }
        */
    }

    void Conductor::update_position(train_task_mapping *train)
    {
    }

    void Conductor::DispatchCommand()
    {
        bool all_trains_blocked = true;
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            if (train_arr[i].train_num == -1)
            {
                continue;
            }
            train_task_mapping *train = train_arr + i;
            MessengerUnit *command_messenger = &train->command_messenger;
            MessengerUnit *sensor_messenger = &train->sensor_messenger;

            if (train->isTrainBlocked)
            {
                train->isTrainBlocked = !ReservePath(train);
            }

            if (sensor_messenger->messenger_id > 0 && !sensor_messenger->sent_reply && !train->path.IsEmpty())
            {
                // check if current top node is still what was last sent
                PathNode node;
                train->path.Pop(&node);
                train->path.Push(node);

                track_node *first_sensor;
                track_node *second_sensor;
                get_sensors_to_listen_to(train, first_sensor, second_sensor);

                // NEED TO CHECK THE FOLLOWING:
                // 1. If the first sensor is the same as the last sent sensor, dont send again
                // 2. Only reply with a single sensor if the sensor is the destination

                if (first_sensor &&
                    (first_sensor != train->last_sent_sensor) &&
                    (second_sensor || ((second_sensor == nullptr) && (first_sensor == train->destination_node))))
                {
                    IO_NS::PrintTerminal(COLOR_MAGENTA "Conductor::DispatchCommand -- train %d: first: %s, safety: %s\r\n", train->train_num, (first_sensor != nullptr) ? first_sensor->name : "NULLPTR", (second_sensor != nullptr) ? second_sensor->name : "NULLPTR");
                    train->last_sent_sensor = first_sensor;
                    train->last_sent_sensor_safety = second_sensor;

                    // send sensor to messenger
                    ListenToSensors sensors = {train->train_num, first_sensor, second_sensor};
                    REPLY(sensor_messenger->messenger_id, (char *)&sensors, sizeof(ListenToSensors));
                    sensor_messenger->sent_reply = true;

                    if (!train->isMoving)
                    {
                        // get reserved path distance
                        int reserved_path_distance = GetReservedPathLength(train);
                        // SEND ACCELERATE COMMAND
                        IO_NS::PrintTerminal(COLOR_GREEN "Conductor::DispatchCommand -- Making Train %d move with reserved path distance: %d\r\n", train->train_num, reserved_path_distance);
                        TrainCommandNotification command = {TRAIN_COMMAND::ACCELERATE, 7, reserved_path_distance};
                        train->train_commands.Push(command);
                        train->isMoving = true;
                    }

                    if (second_sensor && second_sensor == train->destination_node)
                    {
                        // SLOW DOWN TRAIN
                        IO_NS::PrintTerminal(COLOR_GREEN "Conductor::DispatchCommand -- Train %d is approaching destination, sending SLOW command\r\n", train->train_num);
                        TrainCommandNotification command = {TRAIN_COMMAND::ACCELERATE, 4, 0};
                    }
                }
                else if (!second_sensor && (first_sensor != train->destination_node) && train->isMoving)
                {
                    // SEND STOP COMMAND
                    IO_NS::PrintTerminal(COLOR_RED "Conductor::DispatchCommand -- Train %d is moving, sending STOP command\r\n", train->train_num);
                    train->train_commands.Push({TRAIN_COMMAND::STOP, 0});
                    train->isMoving = false;
                }
            }

            if (command_messenger->messenger_id > 0 && !command_messenger->sent_reply && !train->train_commands.IsEmpty())
            {
                IO_NS::PrintTerminal(COLOR_CYAN "Conductor::DispatchCommand -- Sending Train Command to messenger %d (train %d)\r\n", command_messenger->messenger_id, train->train_num);
                TrainCommandNotification command;
                train->train_commands.Pop(&command);
                // check for accelerate command
                train->last_sensor_trigger_tick = TIME(CLOCK_SERVER_TID);

                REPLY(command_messenger->messenger_id, (char *)&command, sizeof(TrainCommandNotification));
                command_messenger->sent_reply = true;
            }

            if (!train->isTrainBlocked)
            {
                all_trains_blocked = false;
            }
        }
        if (all_trains_blocked)
        {
            // IO_NS::PrintTerminal(COLOR_RED "Conductor::DispatchCommand -- All trains are blocked\r\n");
            // uassert(false && "Conductor::DispatchCommand -- All trains are blocked");
        }
        else
        {
            // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::DispatchCommand -- Not all trains are blocked\r\n");
        }
    }
    void Conductor::get_sensors_to_listen_to(train_task_mapping *train, track_node *&first_sensor, track_node *&second_sensor)
    {
        Queue<PathNode, TRACK_MAX> *reserved_nodes = &train->reserved_nodes;
        Stack<PathNode, 20> temp_queue;
        // extract the first two nodes on the path
        first_sensor = nullptr;
        second_sensor = nullptr;
        if (reserved_nodes->IsEmpty())
        {
            return;
        }

        PathNode last_hit_sensor;
        reserved_nodes->Pop(&last_hit_sensor);
        temp_queue.Push(last_hit_sensor);

        // uassert(last_hit_sensor.node->type == NODE_SENSOR && "Conductor::get_sensors_to_listen_to -- first node is not a sensor");

        while (!reserved_nodes->IsEmpty())
        {

            PathNode pnode;
            reserved_nodes->Pop(&pnode);
            temp_queue.Push(pnode);
            track_node *node = pnode.node;

            // IO_NS::PrintTerminal("Conductor::get_sensors_to_listen_to -- checking node: %s\r\n", node->name);

            if (node->type == NODE_SENSOR)
            {
                // IO_NS::PrintTerminal(COLOR_BLUE "Conductor::get_sensors_to_listen_to -- found sensor: %s\r\n", node.node->name);
                if (first_sensor == nullptr)
                {
                    // IO_NS::PrintTerminal(COLOR_RED "Setting first sensor to %s\r\n", node->name);
                    first_sensor = this->track.get_node_by_name(node->name);
                    // IO_NS::PrintTerminal(COLOR_RED "Conductor::get_sensors_to_listen_to -- first sensor set to %s\r\n", first_sensor->name);
                }
                else if (second_sensor == nullptr)
                {
                    // IO_NS::PrintTerminal(COLOR_RED "Setting second sensor to %s\r\n", node->name);
                    // IO_NS::PrintTerminal(COLOR_RED " FIRST SENSOR CHECKED: %s\r\n", first_sensor->name);
                    second_sensor = this->track.get_node_by_name(node->name);
                    break;
                }
                else
                {
                    // IO_NS::PrintTerminal("Conductor::get_sensors_to_listen_to -- both not null\r\n");
                    break;
                }
            }
        }
        // IO_NS::PrintTerminal(COLOR_MAGENTA "Conductor::get_sensors_to_listen_to -- train %d: first: %s, safety: %s\r\n", train->train_num, (first_sensor != nullptr) ? first_sensor->name : "NULLPTR", (second_sensor != nullptr) ? second_sensor->name : "NULLPTR");

        // push the nodes back to the path
        while (!temp_queue.IsEmpty())
        {
            PathNode node;
            temp_queue.Pop(&node);
            reserved_nodes->PushFront(node);
        }
        // IO_NS::PrintTerminal(COLOR_BLUE "VERIFYING RESERVED NODES: ");
        // reserved_nodes->Print();
        // if (first_sensor != nullptr)
        // {
        //     IO_NS::PrintTerminal(COLOR_RED "Conductor::get_sensors_to_listen_to -- first sensor: %s\r\n", first_sensor->name);
        // }
        // if (second_sensor != nullptr)
        // {
        //     IO_NS::PrintTerminal(COLOR_RED "Conductor::get_sensors_to_listen_to -- second sensor: %s\r\n", second_sensor->name);
        // }
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

        ListenToSensors target_pair;
        ConductorRequest conductor_request(train_num, RequestType::GET_SENSOR);
        int marklin_io_tid = WHOIS("MarklinIOServer");
        uassert(marklin_io_tid > 0 && "CONDUCTOR_SENSOR MESSENGER: Error finding MarklinIOServer");
        while (true)
        {
            // IO_NS::PrintTerminal(COLOR_RED "ENTER KEY TO CONTINUE");
            // char c = uart_getc(CONSOLE);
            // SEND SEGMENT REQUEST TO CONDUCTOR
            IO_NS::PrintTerminal(COLOR_YELLOW "CONDUCTOR_SENSOR MESSENGER{%d/%d}:: requesting next sensor from Conductor\r\n", my_tid, train_num);
            int retval = SEND(conductor_tid, (char *)&conductor_request, sizeof(ConductorRequest), (char *)&target_pair, sizeof(target_pair));
            uassert(retval >= 0 && "PATH MESSENGER: Error sending SegmentRequest to Conductor");

            IO_NS::PrintTerminal(COLOR_YELLOW "CONDUCTOR_SENSOR MESSENGER{%d/%d}:: received next sensor from Conductor: %s, %s, train: %d\r\n", my_tid, train_num, target_pair.first_sensor->name, (target_pair.second_sensor != nullptr) ? target_pair.second_sensor->name : "NULLPTR", target_pair.train_num);

            MARKLIN_IO_SERVER::IO_REQUEST io_request(&target_pair);
            IO_NS::PrintTerminal(COLOR_YELLOW "CONDUCTOR_SENSOR MESSENGER{%d/%d}:: Sending request to IO server: %d\r\n", my_tid, train_num, marklin_io_tid);
            SEND(marklin_io_tid, (char *)&io_request, sizeof(MARKLIN_IO_SERVER::IO_REQUEST), nullptr, 0);
            IO_NS::PrintTerminal(COLOR_YELLOW "CONDUCTOR_SENSOR MESSENGER{%d/%d}:: RECEIVED REPLY FROM IO SERVER\r\n", my_tid, train_num);
        }
    }

    // returns true if fully reserved (no conflicts)
    bool Conductor::ReservePath(train_task_mapping *train)
    {
        Stack<PathNode, TRACK_MAX> *path = &train->path;
        Stack<PathNode, TRACK_MAX> temp_stack; // used to restore the path
        temp_stack.Clear();
        Queue<PathNode, TRACK_MAX> temp_queue; // used to push to the train queue for reserved nodes
        temp_queue.Clear();

        bool reservation_conflict = false;

        Queue<PathNode, 6> temp_segment_queue; // use this to check if the segment is already reserved
        PathNode last_reserved_node;
        while (!path->IsEmpty())
        {
            PathNode node;
            path->Pop(&node);
            temp_stack.Push(node);
            if (node.node->who_reserved_me != -1 && node.node->who_reserved_me != train->train_num)
            {
                IO_NS::PrintTerminal("Conductor::ReservePath -- reservation conflict on %s -- reserved by %d\r\n", node.node->name, node.node->who_reserved_me);
                reservation_conflict = true;
                break;
            }
            // temp_queue.Push(node);
            temp_segment_queue.Push(node);
            last_reserved_node = node;
            if (node.node->type == NODE_SENSOR)
            {
                while (!temp_segment_queue.IsEmpty())
                {
                    PathNode segment_node;
                    temp_segment_queue.Pop(&segment_node);
                    if (segment_node.node->who_reserved_me == train->train_num)
                    {
                        continue; // already reserved
                    }
                    temp_queue.Push(segment_node);
                }
            }
        }
        // restore the path
        while (!temp_stack.IsEmpty())
        {
            PathNode node;
            temp_stack.Pop(&node);
            path->Push(node);
        }

        train->conflict_exists = reservation_conflict;
        int distance_to_conflict = 0;
        while (!temp_queue.IsEmpty())
        {
            PathNode node;
            temp_queue.Pop(&node);

            if (reservation_conflict)
            {
                int DIR = 0;
                if (node.node->type == NODE_BRANCH)
                {
                    DIR = (node.switch_state == SwitchState::STRAIGHT) ? 0 : 1;
                }
                // add length of segment to distance to conflict

                if (node.node != train->last_sensor)
                {
                    distance_to_conflict += node.node->edge[DIR].dist;
                }
            }

            if (node.node->type == NODE_SENSOR)
            {
                train->reserved_sensors_count++;
            }
            reserveNode(node.node, train->train_num);
            train->reserved_nodes.Push(node);
        }

        temp_queue.Clear();
        temp_stack.Clear();

        IO_NS::PrintTerminal(COLOR_CYAN "Conductor::RESERVE_PATH -- Reserved nodes for train %d:\r\n", train->train_num);
        train->reserved_nodes.Print();

        IO_NS::PrintTerminal(COLOR_CYAN "Conductor::RESERVE_PATH -- PATH nodes for train %d:\r\n", train->train_num);
        path->Print();

        if (reservation_conflict) // if we have a reservation conflict, PUSH NEW STOPPING TARGET
        {
            IO_NS::PrintTerminal("CONFLICT ENCOUNTERED -- LAST RESERVED NODE: %s\r\n", last_reserved_node.node->name);
            // PEEK INTO STOPPING_TARGETS TO SEE IF THIS NODE IS ALREADY REGISTERED
            // track_node *next_proposed_target;
            // train->stopping_targets.Peek(next_proposed_target, nullptr);
            // if (next_proposed_target && next_proposed_target == last_reserved_node.node)
            // {
            //     IO_NS::PrintTerminal("Conductor::ReservePath -- already registered as stopping target\r\n");
            //     uassert(false && "FORCED ERROR -- CONFLICT ENCOUNTERED (THIS IS GOOD!)");
            // }
            // else
            // {
            //     IO_NS::PrintTerminal("Train %d OFFSET: %d\r\n", train->train_num, train->offset);
            //     int stopping_target = train->total_dist_travelled + distance_to_conflict - train->stopping_distance - train->offset;
            //     // add to stopping targets
            //     train->stopping_targets.Push(*last_reserved_node.node, stopping_target);
            //     IO_NS::PrintTerminal(COLOR_RED "Conductor::ReservePath -- added %s to stopping targets in %d mm\r\n", last_reserved_node.node->name, stopping_target);
            //     IO_NS::PrintTerminal(COLOR_RED "Conductor::ReservePath -- WITHOUT OFFSET: %s to stopping targets in %d mm\r\n", last_reserved_node.node->name, distance_to_conflict - train->stopping_distance);
            //     // uassert(false && "FORCED ERROR -- CONFLICT ENCOUNTERED");
            //     train->offset = 0;
            // }
            IO_NS::PrintTerminal(COLOR_RED "Conductor::ReservePath -- CONFLICT ENCOUNTERED -- Train %d -- distance to conflict: %d\r\n", train->train_num, distance_to_conflict);
        }
        IO_NS::PrintTerminal("RETURNING FROM RESERVE PATH -- CONFLICT: %d\r\n", reservation_conflict);

        return (reservation_conflict ? false : true);
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

        while (!path->IsEmpty())
        {
            train->current_segment_length += curnode.node->edge[DIR].dist;
            IO_NS::PrintTerminal(COLOR_YELLOW "Conductor::PopSegment -- popped node: %s -- length: %d\r\n", curnode.node->name, train->current_segment_length);

            path->Pop(&curnode);
            // IO_NS::PrintTerminal("Conductor::PopSegment -- popped node: %s\r\n", curnode.node->name);
            bool isSensor = false;
            switch (curnode.node->type)
            {
            case NODE_BRANCH:
                DIR = curnode.switch_state == SwitchState::STRAIGHT ? 0 : 1;
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
        IO_NS::PrintTerminal(COLOR_YELLOW "Conductor::PopSegment -- current segment went to: %s with length %d\r\n", curnode.node->name, train->current_segment_length);
    }

    void Conductor::ReleasePath(train_task_mapping *train)
    {
        track_node *dest_node = train->destination_node;

        while (!train->reserved_nodes.IsEmpty())
        {
            PathNode pnode;
            train->reserved_nodes.Pop(&pnode);
            // IO_NS::PrintTerminal("Conductor::ReleasePath -- releasing %s for train %d\r\n", node.name, train->train_num);
            if (pnode.node->type == NODE_SENSOR)
            {
                train->reserved_sensors_count--;
            }
            releaseNode(pnode.node);
        }

        reserveNode(dest_node, train->train_num);
        train->reserved_nodes.Push({dest_node, SwitchState::UNINITIALIZED});
    }

    static bool isReverseInPath(track_node *node, Queue<PathNode, TRACK_MAX> *reserved_path)
    {
        Queue<PathNode, TRACK_MAX> temp_queue;
        bool found_reverse_node = false;

        while (!reserved_path->IsEmpty())
        {
            PathNode pnode;
            reserved_path->Pop(&pnode);
            temp_queue.Push(pnode);
            if (pnode.node == node->reverse)
            {
                found_reverse_node = true;
                // uassert(false && "Conductor::isReverseInPath -- found reverse node in path");
            }
        }

        // restore the path
        while (!temp_queue.IsEmpty())
        {
            PathNode pnode;
            temp_queue.Pop(&pnode);
            reserved_path->Push(pnode);
        }

        return found_reverse_node;
    }

    void Conductor::ReleaseSegment(train_task_mapping *train)
    {
        IO_NS::PrintTerminal(COLOR_RED "Conductor::ReleaseSegment -- releasing segment for train %d -- last sensor: %s\r\n", train->train_num, train->last_sensor->name);
        // use last hit sensor to compare the name

        // print reserved nodes
        IO_NS::PrintTerminal(COLOR_CYAN "Conductor::ReleaseSegment -- PATH:\r\n");
        train->path.Print();
        IO_NS::PrintTerminal(COLOR_CYAN "Conductor::ReleaseSegment -- reserved nodes:\r\n");
        train->reserved_nodes.Print();

        while (!train->reserved_nodes.IsEmpty())
        {
            PathNode pnode;
            int ret = train->reserved_nodes.Peek(&pnode);
            uassert(ret == 0 && "Conductor::ReleaseSegment -- Error peeking reserved node");

            if (pnode.node->type == NODE_SENSOR && pnode.node->num == train->last_sensor->num)
            {
                break;
            }

            IO_NS::PrintTerminal("Conductor::ReleaseSegment -- releasing %s for train %d\r\n", pnode.node->name, train->train_num);
            train->reserved_nodes.Pop(&pnode);
            // CHECK IF REVERSE OF THE PNODE IS IN THE RESERVED NODES
            bool is_reverse_saved = isReverseInPath(pnode.node, &train->reserved_nodes);
            if (!is_reverse_saved)
            {
                releaseNode(pnode.node);
            }

            if (pnode.node->type == NODE_SENSOR)
            {
                train->reserved_sensors_count--;
            }

            IO_NS::PrintTerminal(COLOR_RED "Conductor::ReleaseSegment -- RELEASED REVERSE NODE: %s = {%d}\r\n", pnode.node->reverse->name, pnode.node->reverse->who_reserved_me);
            // get key to continue
            // IO_NS::PrintTerminal(COLOR_RED "Conductor::ReleaseSegment -- Press any key to continue\r\n");
            // track_node *BR2 = track.get_node_by_name("BR2");
            // IO_NS::PrintTerminal(COLOR_RED "Conductor::ReleaseSegment -- BR2: reserved by %d\r\n", BR2->who_reserved_me);
            // char c = uart_getc(CONSOLE);
        }
        // uassert(false && "Conductor::ReleaseSegment -- Error getting last sensor address");
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
            DELAY(CLOCK_SERVER_TID, 50);
        }
    }

    void start_conductor()
    {
        Conductor conductor;
    }

    void Conductor::ProcessSensorTrigger(SensorTriggerResponse *trigger_response)
    {
        int triggered_idx = trigger_response->sensor_idx;
        int triggered_tick = trigger_response->trigger_tick;
        int train_num = trigger_response->train_num;

        int train_idx = get_train_index(trigger_response->train_num);
        uassert(train_idx >= 0 && "Conductor::ProcessSensorTrigger -- Error getting train index");
        train_task_mapping *train = train_arr + train_idx;

        int expected_idx = get_index_from_name((const char *)train->last_sent_sensor->name);
        int safety_idx = (train->last_sent_sensor_safety != nullptr) ? get_index_from_name((const char *)train->last_sent_sensor_safety->name) : -1;

        // set this to true if safety or expected sensor is triggered
        bool segment_verified = (expected_idx == triggered_idx) ? true : false;
        int missed_segment_length = 0;
        // IO_NS::PrintTerminal("Conductor::ProcessSensorTrigger -- expected sensor: %s, triggered sensor: %s\r\n", train->last_sent_sensor->name, triggered_idx);
        if (safety_idx > -1 && safety_idx == triggered_idx)
        {
            IO_NS::PrintTerminal(COLOR_RED "Conductor::ProcessSensorTrigger -- SAFETY SENSOR %s was triggered at tick %d for train %d\r\n", train->last_sent_sensor_safety->name, triggered_tick, train_num);
            // extract the length of the missed segment into temp var
            IO_NS::PrintTerminal("CURRENT SEGMENT LENGTH: %d\r\n", train->current_segment_length);
            PopSegment(train); // pop the segment, now current_segment_length is the length of the segment from the missed to triggered sensor
            SwitchNextSegment(&train->path);
            missed_segment_length = train->current_segment_length;
            segment_verified = true;
        }

        if (!segment_verified)
        {
            IO_NS::PrintTerminal(COLOR_RED "Conductor::ProcessSensorTrigger -- FATAL ERROR -- TRAIN{%d} was lost (expected/safety not triggered)");
            return;
        }

        IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ProcessSensorTrigger -- Sensor %s was triggered at tick %d for train %d\r\n", (triggered_idx == expected_idx) ? train->last_sent_sensor->name : train->last_sent_sensor_safety->name, triggered_tick, train_num);
        train->reach_first_sensor = true;
        // UPDATE TRAIN POSITION
        if (train->last_sensor_trigger_tick > 0)
        {
            int delta_tick = triggered_tick - train->last_sensor_trigger_tick;
            // IO_NS::PrintTerminal("Conductor::ProcessSensorTrigger -- delta tick: %d\r\n", delta_tick);
            train->actual_speed_x100 = (train->current_segment_length * 100) / delta_tick;
            // IO_NS::PrintTerminal("Conductor::ProcessSensorTrigger -- actual speed: %d\r\n", train->actual_speed_x100);
        }
        else
        {
            train->actual_speed_x100 = 0;
        }
        // UPDATE STACK SO NEXT SENSOR IS THE TOP
        PathNode next_sensor;
        train->path.Pop(&next_sensor);
        train->path.Push(next_sensor);

        train->last_sensor = next_sensor.node;

        // new distance calculation logic
        train->total_dist_travelled += train->current_segment_length + missed_segment_length;
        train->middle_distance = 0; // Reset between-sensors distance

        train->last_sensor_trigger_tick = triggered_tick;
        // UpdateTrainDisplay();

        PopSegment(train);
        SwitchNextSegment(&train->path);
        // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ProcessSensorTrigger -- Reserved QUEUE:");
        // train->reserved_nodes.Print();
        // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ProcessSensorTrigger -- RELEASING SEGMENT:");
        ReleaseSegment(train);

        IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ProcessSensorTrigger -- Train %d -- Popped segment from path:\r\n", train->train_num);
        train->path.Print();
        train->reserved_nodes.Print();

        // get key to continue
        // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ProcessSensorTrigger -- Press any key to continue\r\n");
        // unsigned char ch = uart_getc(CONSOLE);

        if (train->isMoving && train->last_sensor == train->destination_node)
        {
            IO_NS::PrintTerminal(COLOR_GREEN "Conductor::DispatchCommand -- Train %d reached destination %s -- STOPPING TRAIN\r\n", train->train_num, train->last_sensor->name);
            ReleasePath(train);
            train->path.Clear();

            train->train_commands.Clear();
            IO_NS::PrintTerminal(COLOR_RED "Conductor::ProcessSensorTrigger -- Train %d reached destination %s -- STOPPING TRAIN\r\n", train->train_num, train->last_sensor->name);
            train->train_commands.Push({TRAIN_COMMAND::STOP, 5}); // NON ZERO SINCE DESTINATION STOP
            train->isMoving = false;
        }

        // IO_NS::PrintTerminal(COLOR_GREEN "POPPED SEGMENT FROM PATH; REMAINING PATH:");
        // train->path.Print();
        // IO_NS::PrintTerminal(COLOR_GREEN "Conductor::ProcessSensorTrigger -- HANDLED %s TRIGGER\r\n", (triggered_idx == expected_idx) ? train->last_sent_sensor->name : train->last_sent_sensor_safety->name);
    }

    void Conductor::StopTrainConflict(train_task_mapping *train)
    {
    }

    int Conductor::GetReservedPathLength(train_task_mapping *train)
    {
        int length = 0;
        Queue<PathNode, TRACK_MAX> *reserved_nodes = &train->reserved_nodes;
        Queue<PathNode, TRACK_MAX> temp_stack;
        IO_NS::PrintTerminal(COLOR_YELLOW "Conductor::GetReservedPathLength -- reserved nodes:\r\n");
        while (!reserved_nodes->IsEmpty())
        {
            PathNode pnode;
            reserved_nodes->Pop(&pnode);
            IO_NS::PrintTerminal(COLOR_YELLOW "%s ", pnode.node->name);
            temp_stack.Push(pnode);
            int DIR = 0;
            if (pnode.node->type == NODE_BRANCH)
            {
                DIR = (pnode.switch_state == SwitchState::STRAIGHT) ? 0 : 1;
            }
            // IO_NS::PrintTerminal(" -- %d -- ", pnode.node->edge[DIR].dist);
            length += pnode.node->edge[DIR].dist;
        }
        IO_NS::PrintTerminal("\r\n");
        while (!temp_stack.IsEmpty())
        {
            PathNode node;
            temp_stack.Pop(&node);
            reserved_nodes->Push(node);
        }

        // IO_NS::PrintTerminal("LENGTH: %d\r\n", length);
        // uassert(false && "FORCED ERROR");
        return length;
    }
}

// ADD IS_MOVING UPDATE