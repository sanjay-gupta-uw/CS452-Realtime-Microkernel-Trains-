#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
#include "../include/clock_server.h"
#include "../include/name_server.h"
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"
#include "../marklin/train.h"
#include "../../include/syscall.h"
#include "../../shared_constants.h"
#include <cstring>
#include <cctype>

#define TEST_ENABLED 0
typedef struct IO_REQUEST
{
    unsigned char ch;
};

namespace Conductor_NS
{

    static void InitializeTrainDisplay()
    {
        // Header
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+-----------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 0, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|                                Active Trains:                               |\r\n", TRAIN_TABLE_Y + 1, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-------------------------------------------------------------------------------\r\n", TRAIN_TABLE_Y + 2, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "| ID | Speed Level | Actual Speed | Last Sensor | Next Sensor | Dest | Offset |\r\n", TRAIN_TABLE_Y + 3, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-------------------------------------------------------------------------------\r\n", TRAIN_TABLE_Y + 4, TRAIN_TABLE_X);
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            int location_y = TRAIN_TABLE_Y + 5 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|    |             |              |             |             |      |        |", location_y, TRAIN_TABLE_X);
        }
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+-----------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 5 + NUM_TRAINS, TRAIN_TABLE_X);
    }

    Conductor::Conductor()
    {
        IO_NS::PrintTerminal("Starting Conductor\r\n");
        // Track track;
        REGISTERAS("Conductor");
        IO_NS::PrintTerminal("Conductor started\r\n");

        // initialize train_arr
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            train_arr[i].path_messenger_id = -1;
            train_arr[i].cmd_messenger_id = -1;
            train_arr[i].train_num = -1;
        }

        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        track.init(track_id);
        REPLY(sender_tid, nullptr, 0);

        // create sensor server
        SENSOR_SERVER_TID = CREATE(PRIORITY::DEVICE, Sensors_NS::SensorServer);
        uassert(SENSOR_SERVER_TID > 0 && "Conductor::Error creating sensor server");
        IO_NS::PrintTerminal("Sensor server created with TID %d\r\n", SENSOR_SERVER_TID);
        // create switch server
        SWITCH_SERVER_TID = CREATE(PRIORITY::DEVICE, Switch_NS::StartSwitchServer);
        uassert(SWITCH_SERVER_TID > 0 && "Conductor::Error creating switch server");
        IO_NS::PrintTerminal("Switch server created with TID %d\r\n", SWITCH_SERVER_TID);
        SEND(SWITCH_SERVER_TID, (char *)&track_id, sizeof(track_id), nullptr, 0); // send track id to switch server

        LOOP_START_SENSOR_DATA = {'B', 5};

        InitializeTrainDisplay();
        ConductorLoop();
    }
    Conductor::~Conductor()
    {
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
            int switch_addr = req->id;
            Switch_NS::SWITCH_STATE state = (req->data == 'S') ? Switch_NS::SWITCH_STATE::STRAIGHT : Switch_NS::SWITCH_STATE::CURVED;
            Switch_NS::SwitchRequest switch_req = {switch_addr, state};
            int retval = SEND(SWITCH_SERVER_TID, (char *)&switch_req, sizeof(Switch_NS::SwitchRequest), nullptr, 0);
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
                IO_NS::PrintTerminal("Train %d found at index %d, sending ACCELERATE command\r\n", train_num, train_index);
            }

            train_arr[train_index].train_commands.Push({TRAIN_COMMAND::ACCELERATE, speed});
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

            // Extract sensor ID from request
            char *sensor_id = req->src;
            char sensor_bank = sensor_id[0];
            char *sensor_num_ptr = sensor_id + 1;
            int sensor_number = a2ui(&sensor_num_ptr, 10);
            IO_NS::PrintTerminal("Parsed sensor: bank=%c, number=%d (from '%s')\r\n",
                                 sensor_bank, sensor_number, sensor_id);

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
                if (train_arr[i].train_num == -1)
                {
                    train = &train_arr[i];
                    memset(train->target_sensor_name, 0, 4);
                    strncpy(train->target_sensor_name, sensor_id, 4);
                    train->train_task_tid = spawned_train_tid;
                    train->path_messenger_id = -1;
                    train->cmd_messenger_id = -1;
                    train->calibration_stage = CALIBRATION_STAGE::NONE;

                    train->train_num = req->id;
                    train->actual_speed_x10 = -1;
                    train->speed_level = 0;
                    train->next_predicted_bank = sensor_bank;
                    train->next_predicted_num = sensor_number;
                    train->offset = -1;
                    train->recent_sensor_bank = '\0';
                    train->recent_sensor_num = -1;
                    memset(train->destination, 0, 5);

                    IO_NS::PrintTerminal("Index: %d, Train %d spawned with Sensor %s\r\n", i, train->train_num, train->target_sensor_name);
                    break;
                }
            }

            Conductor::UpdateTrainDisplay();

            IO_NS::PrintTerminal("Train %d spawned successfully, beginning calibration!\r\n", req->id);

            train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_NAV_TO_LOOP;
            IO_NS::PrintTerminal("Train %d, TID: %d calibrating -- start: %s", train->train_num, train->train_task_tid, train->target_sensor_name);

            CalibrateTrain(train);

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

            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 2, train_arr[i].train_num);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 7, train_arr[i].speed_level);
            IO_NS::Print(MOVE_CURSOR "%d.%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 21, train_arr[i].actual_speed_x10, train_arr[i].actual_speed_x10 % 10);
            IO_NS::Print(MOVE_CURSOR "%c",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 36, train_arr[i].recent_sensor_bank);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 37, train_arr[i].recent_sensor_num);
            IO_NS::Print(MOVE_CURSOR "%c",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 50, train_arr[i].next_predicted_bank);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 51, train_arr[i].next_predicted_num);
            IO_NS::Print(MOVE_CURSOR "%s",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 64, train_arr[i].destination);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 71, train_arr[i].offset);
            display_row++;
        }
    }

    void Conductor::SetSwitches(Queue<PathNode, NUM_SWITCHES> *switch_nodes)
    {
        while (!switch_nodes->IsEmpty())
        {
            PathNode node;
            switch_nodes->Pop(&node);
            // IO_NS::PrintTerminal("CONDUCTOR::SettingSwitches -- NODE: %s\r\n", node.node->name);

            if (node.node->type == NODE_BRANCH)
            {
                Switch_NS::SwitchRequest switch_req = {node.node->num, node.switch_state};

                int retval = SEND(SWITCH_SERVER_TID, (char *)&switch_req, sizeof(switch_req), nullptr, 0);
                uassert(retval >= 0 && "Error sending switch request");

                IO_NS::PrintTerminal("CONDUCTOR::Switch %s set to %c\r\n", node.node->name, node.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT ? 'S' : 'C');
                // break;
            }
        }
        // uassert(false && "FORCED ERROR");
    }

    void Conductor::ConductorTest()
    {
        if (!TEST_ENABLED)
        {
            return;
        }
        // TEST LOOP SWITCHES
        // IO_NS::PrintTerminal("Conductor testing loop switches\r\n");
        // Queue<PathNode, NUM_SWITCHES> switch_config;
        // int distance;
        // track.getLoop(&switch_config, &distance);
        // SetSwitches(&switch_config);
        // IO_NS::PrintTerminal("Conductor finished testing loop switches\r\n");

        // test path finding
        IO_NS::PrintTerminal("Conductor testing path finding\r\n");
        Stack<PathNode, TRACK_MAX> path;
        track.find_path("E1", "E14", &path);
        track.find_path("E9", "D8", &path);
        track.find_path("A1", "A2", &path);
        track.find_path("A1", "E7", &path);
        track.find_path("A2", "E7", &path);
        track.find_path(LOOP_START_NODE, LOOP_START_NODE, &path, false);
        // track.find_path(LOOP_START_NODE, "C11", &path);
        IO_NS::PrintTerminal("Conductor finished testing path finding\r\n");
    }

    void Conductor::SendSegmentToMessenger(int messenger_tid, train_task_mapping *train)
    {
        char *destination = train->destination;

        int segment_length = GetSegmentLength(train->train_num);
        if (segment_length == 0)
        {
            IO_NS::PrintTerminal(COLOR_RED "Train %d has reached destination\r\n", train->train_num);
            UpdateCalibrationStage(train);
            CalibrateTrain(train);
            if (train->calibration_stage == CALIBRATION_STAGE::NONE)
            {
                return;
            }
            segment_length = GetSegmentLength(train->train_num);
        }
        if (segment_length == -1)
        {
            IO_NS::PrintTerminal(COLOR_RED "Conductor::SendSegmentToMessenger -- NEGATIVE SEGMENT LENGTH -- PATH INIT!\r\n");
            return;
        }

        IO_NS::PrintTerminal("Conductor::SendSegmentToMessenger -- Segment length: %d\r\n", segment_length);
        SensorStruct sensor = name_to_sensor_struct(train->target_sensor_name);

        int speed = train->speed_level;
        TrainMessage message(sensor.bank, sensor.id, segment_length);
        IO_NS::PrintTerminal(COLOR_CYAN "Conductor::SendSegmentToMessenger -- Sending segment {with target sensor %c%d}to messenger %d\r\n", message.data.segment.sensor.bank, message.data.segment.sensor.id, messenger_tid);
        // reply to messenger task with segment data
        train->sent_reply_to_path_messenger = true;
        REPLY(messenger_tid, (char *)&message, sizeof(message));
    }

    void Conductor::CalibrateTrain(train_task_mapping *train)
    {
        train->speed_level = MEDIUM_SPEED;
        switch (train->calibration_stage)
        {
        case CALIBRATE_NAV_TO_LOOP:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::NAV TO LOOP\r\n");
            // find path to loop start
            const char *start_node_name = train->target_sensor_name;
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- Finding path from %s to %s\r\n", start_node_name, LOOP_START_NODE);
            track.find_path(start_node_name, LOOP_START_NODE, &train->path);
            train->start = true;

            uassert(!train->path.IsEmpty() && "CalibrateTrain: Path is empty");
            Queue<PathNode, NUM_SWITCHES> switch_nodes;
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- Found path from %s to %s\r\n", start_node_name, LOOP_START_NODE);
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- Setting switches\r\n");
            get_switch_queue(&train->path, &switch_nodes);
            SetSwitches(&switch_nodes);
            // this should be set when path is empty
            // train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_LOOP;
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- Set all switches -- can safely navigate from %s to %s\r\n", start_node_name, LOOP_START_NODE);
            break;
        }
        case CALIBRATE_LOOP:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::CALIBRATE_LOOP\r\n");
            // find path to loop start
            const char *start_node_name = train->target_sensor_name;
            // update this so it finds shortest path back to start (loop)
            uassert(strcmp(start_node_name, LOOP_START_NODE) == 0 && "CalibrateTrain: Start node is not loop start");
            track.find_path(start_node_name, LOOP_START_NODE, &train->path, false);
            Queue<PathNode, NUM_SWITCHES> switch_nodes;
            get_switch_queue(&train->path, &switch_nodes);
            SetSwitches(&switch_nodes);
            // ONLU ALLOW
            // train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_NAV_TO_STOP;
            break;
        }
        case CALIBRATE_NAV_TO_STOP:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::NAV TO STOP\r\n");
            // find path to loop start
            const char *start_node_name = train->target_sensor_name;
            track.find_path(start_node_name, train->destination, &train->path);
            Queue<PathNode, NUM_SWITCHES> switch_nodes;
            get_switch_queue(&train->path, &switch_nodes);
            SetSwitches(&switch_nodes);
            // train->calibration_stage = CALIBRATION_STAGE::NONE;
            break;
        }
        case CALIBRATION_STAGE::NONE:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::NONE\r\n");
            break;
        }
        default:
            break;
        }
    }

    void Conductor::ConductorLoop()
    {
        ConductorTest();
        int retval;
        int sender_tid;
        ConductorRequest req;
        while (true)
        {
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&req, sizeof(ConductorRequest));
            uassert(retval >= 0 && "Error receiving request from terminal");
            IO_NS::PrintTerminal("Conductor received %s from %d\r\n", req.requestType == RequestType::CMD ? "CMD" : "GET_SEGMENT", sender_tid);
            bool sendReply = false;
            if (req.requestType == RequestType::CMD)
            {
                ProcessRequest(&(req.data.cmdRequest));
                sendReply = true;
            }
            else if (req.requestType == RequestType::GET_SEGMENT)
            {
                // extract messenger TID
                int messenger_tid = sender_tid;
                for (int i = 0; i < NUM_TRAINS; i++)
                {
                    train_task_mapping *train = train_arr + i;
                    if (train->train_task_tid == req.data.spawned_train_tid)
                    {
                        train->sent_reply_to_path_messenger = false;
                        train->path_messenger_id = messenger_tid;
                        break;
                    }
                }
            }
            else if (req.requestType == RequestType::GET_CMD)
            {
                // extract messenger TID
                int messenger_tid = sender_tid;
                for (int i = 0; i < NUM_TRAINS; i++)
                {
                    train_task_mapping *train = train_arr + i;
                    if (train->train_task_tid == req.data.spawned_train_tid)
                    {
                        train->sent_reply_to_cmd_messenger = false;
                        train->cmd_messenger_id = messenger_tid;
                        break;
                    }
                }
            }

            DispatchCommand();

            if (sendReply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
        }
        EXIT();
    }

    void Conductor::DispatchCommand()
    {
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            train_task_mapping *train = train_arr + i;

            if (train->train_task_tid > 0)
            {
                if (train->path_messenger_id > 0 && !train->sent_reply_to_path_messenger)
                {
                    IO_NS::PrintTerminal(COLOR_CYAN "Conductor::DispatchCommand -- Extracting segment for train %d\r\n", train->train_num);
                    SendSegmentToMessenger(train->path_messenger_id, train); // this replies to the path messenger if successful
                }
                if (train->cmd_messenger_id > 0 && !train->sent_reply_to_cmd_messenger && !train->train_commands.IsEmpty())
                {
                    IO_NS::PrintTerminal(COLOR_CYAN "Conductor::DispatchCommand -- Sending Train Command to messenger %d (train %d)\r\n", train->cmd_messenger_id, train->train_num);
                    TrainCommandNotification command;
                    train->train_commands.Pop(&command);
                    train->sent_reply_to_cmd_messenger = true;
                    REPLY(train->cmd_messenger_id, (char *)&command, sizeof(TrainCommandNotification));
                }
            }
        }
    }

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
        IO_NS::PrintTerminal("CONDUCTOR-GetSegmentLength::cur sensor: %s, Processing segment until next sensor\r\n", train->target_sensor_name);

        PathNode cur_node;
        path->Pop(&cur_node);
        uassert(cur_node.node->type == NODE_SENSOR && "GET-NEXT-SEGMENT::Next is not a sensor");
        IO_NS::PrintTerminal("Setting recent sensor data to %s\r\n", cur_node.node->name);
        // update recent sensor
        train->recent_sensor_bank = cur_node.node->name[0];
        const char *num_ascii = cur_node.node->name + 1;
        train->recent_sensor_num = a2ui((char **)&num_ascii, 10);
        IO_NS::PrintTerminal("Set recent sensor data to %c%d\r\n", train->recent_sensor_bank, train->recent_sensor_num);
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
                train->train_commands.Push({TRAIN_COMMAND::ACCELERATE, LOW_SPEED});
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
                    train->train_commands.Push({TRAIN_COMMAND::REVERSE, LOW_SPEED});
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

    void Conductor::UpdateCalibrationStage(train_task_mapping *train)
    {
        switch (train->calibration_stage)
        {
        case CALIBRATE_NAV_TO_LOOP:
            IO_NS::PrintTerminal("Conductor::UpdateCalibrationStage -- CALIBRATION_STAGE::CALIBRATE_LOOP\r\n");
            train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_LOOP;
            break;
        case CALIBRATE_LOOP:
            IO_NS::PrintTerminal("Conductor::UpdateCalibrationStage -- CALIBRATION_STAGE::CALIBRATE_NAV_TO_STOP\r\n");
            train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_NAV_TO_STOP;
            break;
        case CALIBRATE_NAV_TO_STOP:
            IO_NS::PrintTerminal("Conductor::UpdateCalibrationStage -- CALIBRATION_STAGE::NONE\r\n");
            train->calibration_stage = CALIBRATION_STAGE::NONE;
            break;
        default:
            break;
        }
    }

    /*
    void conductor_notifier()
    {
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID >= 0 && "ConductorNotifier: Error finding ClockServer");

        int conductor_tid = WHOIS("Conductor");
        uassert(conductor_tid >= 0 && "ConductorNotifier: Error finding Conductor");
        int retval = 0;
        // ConductorRequest ticker_request({RequestType::TICKER});
        while (true)
        {
            retval = DELAY(CLOCK_SERVER_TID, 20);
            uassert(retval >= 0 && "ConductorNotifier: Error delaying");
            retval = SEND(conductor_tid, nullptr, 0, nullptr, 0);
            uassert(retval >= 0 && "ConductorNotifier: Error sending notification to Conductor");
        }
    }
    */

    void start_conductor()
    {
        Conductor conductor;
    }

} // namespace Conductor_NS