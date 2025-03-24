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

        // create sensor server
        SENSOR_SERVER_TID = CREATE(PRIORITY::DEVICE, Sensors_NS::SensorServer);
        uassert(SENSOR_SERVER_TID > 0 && "Conductor::Error creating sensor server");
        IO_NS::PrintTerminal("Sensor server created with TID %d\r\n", SENSOR_SERVER_TID);
        // create switch server
        SWITCH_SERVER_TID = CREATE(PRIORITY::DEVICE_SERVER, Switch_NS::SwitchServer);
        uassert(SWITCH_SERVER_TID > 0 && "Conductor::Error creating switch server");
        IO_NS::PrintTerminal("Switch server created with TID %d\r\n", SWITCH_SERVER_TID);

        // initialize train_arr
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            train_arr[i].messenger_id = -1;
            train_arr[i].train_num = -1;
        }

        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        track.init(track_id);
        REPLY(sender_tid, nullptr, 0);

        LOOP_START_SENSOR_DATA = {BANKS::B, 5};

        InitializeTrainDisplay();
        ConductorLoop();
    }
    Conductor::~Conductor()
    {
    }

    static void get_switch_queue(Stack<PathNode, TRACK_MAX> *path, Queue<PathNode, NUM_SWITCHES> *switch_nodes)
    {
        while (!path->IsEmpty())
        {
            PathNode node;
            path->Pop(&node);
            if (node.node->type == NODE_BRANCH)
            {
                switch_nodes->Push(node);
            }
        }
    }

    static SensorStruct name_to_sensor_struct(const char *sensor_name)
    {
        SensorStruct sensor = {};
        sensor.bank = BANKS(sensor_name[0] - 'A');
        sensor.id = a2ui((char **)&sensor_name[1], 3);
        return sensor;
    }

    void Conductor::ProcessRequest(CMDRequest *req)
    {
        IO_NS::PrintTerminal("Conductor processing request\r\n");
        switch (req->command)
        {
        case COMMAND::SET_SWITCH:
        {
            IO_NS::PrintTerminal("Conductor received SET_SWITCH request for switch index %d\r\n", req->id);
            int switch_index = req->id;
            Switch_NS::SWITCH_STATE state = (req->data == 'S') ? Switch_NS::SWITCH_STATE::STRAIGHT : Switch_NS::SWITCH_STATE::CURVED;
            Switch_NS::SwitchRequest switch_req = {false, switch_index, state};
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

            TrainResponse response = {TrainResponseType::TRAIN_MESSENGER, TRAIN_COMMAND::ACCELERATE, speed};

            train_arr[train_index]
                .train_response_queue.Push(response);
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

            TrainResponse response = {TrainResponseType::TRAIN_MESSENGER, TRAIN_COMMAND::REVERSE};
            train_arr[train_index].train_response_queue.Push(response);
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

            int spawned_train_tid = CREATE(PRIORITY::DEVICE_SERVER, Trains_NS::spawn_train);
            uassert(spawned_train_tid > 0);
            IO_NS::PrintTerminal("Train %d spawned with TID %d\r\n", req->id, spawned_train_tid);
            bool trainSpawnedSuccess = false;

            Trains_NS::TrainParams train_params = {req->id, 0};
            // train task should reply with true
            SEND(spawned_train_tid, (char *)&train_params, sizeof(Trains_NS::TrainParams), (char *)&trainSpawnedSuccess, sizeof(bool));
            uassert(trainSpawnedSuccess && "Error spawning train");

            // sleeping_trains.Push({spawned_train_tid, req->id});
            // trains.SpawnTrain(req->id);
            for (int i = 0; i < NUM_TRAINS; i++)
            {
                if (train_arr[i].train_num == -1)
                {
                    train_arr[i].target_sensor_name = req->src;
                    train_arr[i].train_task_tid = spawned_train_tid;
                    train_arr[i].messenger_id = -1;
                    train_arr[i].calibration_stage = CALIBRATION_STAGE::NONE;

                    train_arr[i].train_num = req->id;
                    train_arr[i].actual_speed_x10 = -1;
                    train_arr[i].speed_level = -1;
                    train_arr[i].next_predicted_bank = sensor_bank;
                    train_arr[i].next_predicted_num = sensor_number;
                    train_arr[i].offset = -1;
                    train_arr[i].recent_sensor_bank = '\0';
                    train_arr[i].recent_sensor_num = -1;
                    memset(train_arr[i].destination, 0, 5);

                    break;
                }
            }

            Conductor::UpdateTrainDisplay();

            IO_NS::PrintTerminal("Train %d spawned successfully\r\n", req->id);
            break;
        }
        case COMMAND::CALIBRATE:
        {
            int train_num = req->id;
            int train_index = get_train_index(train_num);
            if (train_index == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }
            else
            {
                IO_NS::PrintTerminal("Train %d found, sending CALIBRATE command\r\n", train_num);
            }

            // get path to calibration sensor
            const char *start_node_name = train_arr[train_index].target_sensor_name;
            track.find_path(start_node_name, LOOP_START_NODE, &train_arr[train_index].path);
            Queue<PathNode, NUM_SWITCHES> switch_nodes;
            get_switch_queue(&train_arr[train_index].path, &switch_nodes);
            SetSwitches(&switch_nodes);
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
            IO_NS::PrintTerminal("CONDUCTOR::SettingSwitches -- NODE: %s\r\n", node.node->name);

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
            IO_NS::PrintTerminal("Train %d has reached destination\r\n", train->train_num);
            CalibrateTrain(train);
            return;
        }
        SensorStruct sensor = name_to_sensor_struct(train->target_sensor_name);
        COMMAND cmd = COMMAND::ACCELERATE_TRAIN;
        int speed = train->speed_level;
        TrainResponse response = {TrainResponseType::TRAIN_MESSENGER, TRAIN_COMMAND::ACCELERATE, speed, sensor, segment_length};

        // reply to messenger task with segment data
        REPLY(messenger_tid, (char *)&response, sizeof(TrainResponse));
    }

    void Conductor::CalibrateTrain(train_task_mapping *train)
    {
        switch (train->calibration_stage)
        {
        case CALIBRATE_NAV_TO_LOOP:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::NAV TO LOOP\r\n");
            // find path to loop start
            const char *start_node_name = train->target_sensor_name;
            track.find_path(start_node_name, LOOP_START_NODE, &train->path);
            Queue<PathNode, NUM_SWITCHES> switch_nodes;
            get_switch_queue(&train->path, &switch_nodes);
            SetSwitches(&switch_nodes);
            train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_LOOP;
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
            train->calibration_stage = CALIBRATION_STAGE::NONE;
            break;
        }
        case CALIBRATE_LOOP:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::CALIBRATE_LOOP\r\n");
            // find path to loop start
            const char *start_node_name = train->target_sensor_name;
            // update this so it finds shortest path back to start (loop)
            track.find_path(start_node_name, LOOP_START_NODE, &train->path, false);
            Queue<PathNode, NUM_SWITCHES> switch_nodes;
            get_switch_queue(&train->path, &switch_nodes);
            SetSwitches(&switch_nodes);
            // ONLU ALLOW
            train->calibration_stage = CALIBRATION_STAGE::CALIBRATE_NAV_TO_STOP;
            break;
        }

        case CALIBRATION_STAGE::NONE:
        {
            IO_NS::PrintTerminal("Conductor::CalibrateTrain -- CALIBRATION_STAGE::NONE\r\n");
            break;
        }
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
            bool sendReply = false;
            if (req.requestType == RequestType::CMD)
            {
                IO_NS::PrintTerminal("Conductor received cmd request\r\n");
                ProcessRequest(&(req.data.cmdRequest));
                sendReply = true;
            }
            else if (req.requestType == RequestType::GET_SEGMENT)
            {
                // extract messenger TID
                int messenger_tid = sender_tid;
                IO_NS::PrintTerminal("Conductor received non-cmd request\r\n");
                for (int i = 0; i < NUM_TRAINS; i++)
                {
                    if (train_arr[i].train_task_tid == req.data.spawned_train_tid)
                    {
                        // THESE WOULD CORRESEPOND TO USER INPUTTED TR/RV COMMANDS
                        if (!train_arr[i].train_response_queue.IsEmpty())
                        {
                            TrainResponse response;
                            train_arr[i].train_response_queue.Pop(&response);
                            REPLY(messenger_tid, (char *)&response, sizeof(TrainResponse));
                            break;
                        }
                        else
                        {
                            SendSegmentToMessenger(messenger_tid, &train_arr[i]);
                        }
                        break;
                    }
                }
            }

            if (sendReply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
        }
        EXIT();
    }

    // returns segment length, and
    int Conductor::GetSegmentLength(int train_num)
    {
        int train_index = get_train_index(train_num);
        if (train_index == -1)
        {
            IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
            return 0;
        }
        else
        {
            IO_NS::PrintTerminal("Train %d found, getting next segment\r\n", train_num);
        }

        Stack<PathNode, TRACK_MAX> *path = &train_arr[train_index].path;
        if (path->IsEmpty())
        {
            IO_NS::PrintTerminal("Train %d has reached destination\r\n", train_num);
            return 0;
        }

        PathNode next_node;
        path->Pop(&next_node);
        uassert(next_node.node->type == NODE_SENSOR && "GET-NEXT-SEGMENT::Next is not a sensor");
        // update recent sensor
        train_arr[train_index].recent_sensor_bank = next_node.node->name[0];
        train_arr[train_index].recent_sensor_num = a2ui((char **)&next_node.node->name[1], 3);

        int segment_length = next_node.node->edge[DIR_AHEAD].dist;

        // process untill next sensor
        while (!path->IsEmpty())
        {
            PathNode node;
            path->Pop(&node);
            if (node.node->type == NODE_SENSOR)
            {
                break;
            }
            if (node.node->type == NODE_BRANCH)
            {
                uassert(node.switch_state == Switch_NS::SWITCH_STATE::STRAIGHT || node.switch_state == Switch_NS::SWITCH_STATE::CURVED);
                segment_length += node.node->edge[node.switch_state].dist;
            }
            else
            {
                segment_length += node.node->edge[DIR_AHEAD].dist;
            }
        }

        // update current sensor name
        if (next_node.node->type == NODE_SENSOR)
        {
            train_arr[train_index].target_sensor_name = next_node.node->name;
        }

        return segment_length;
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