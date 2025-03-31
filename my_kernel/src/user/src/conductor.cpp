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
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "+---------------------------------------------------------------------------------------+\r\n", TRAIN_TABLE_Y + 0, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|                                Active Trains:                                         |\r\n", TRAIN_TABLE_Y + 1, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-----------------------------------------------------------------------------------------\r\n", TRAIN_TABLE_Y + 2, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "| ID | Speed Level | Actual Speed | Last Sensor | Next Sensor | Dest | Offset | Tick    |\r\n", TRAIN_TABLE_Y + 3, TRAIN_TABLE_X);
        IO_NS::Print(COLOR_WHITE MOVE_CURSOR "-----------------------------------------------------------------------------------------\r\n", TRAIN_TABLE_Y + 4, TRAIN_TABLE_X);
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            int location_y = TRAIN_TABLE_Y + 5 + i;
            IO_NS::Print(COLOR_WHITE MOVE_CURSOR "|    |             |              |             |             |      |        |         |", location_y, TRAIN_TABLE_X);
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
            train_task_mapping *train = &train_arr[i];

            train->train_num = -1;
            train->command_messenger = {-1, false};
            train->path_messenger = {-1, false};
        }

        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        track.init(track_id);
        REPLY(sender_tid, nullptr, 0);

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
            // upadte track-switch state
            track.set_switch_state(switch_addr, req->data);
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
                    train->next_predicted_sensor = track.predict_next_sensor(train->last_sensor);

                    train->actual_speed_x10 = -1;
                    train->speed_level = 0;
                    train->offset = -1;
                    memset(train->destination, 0, 5);

                    train->train_commands.Clear();
                    train->path.Clear();

                    IO_NS::PrintTerminal("Index: %d, Train %d spawned with last Sensor %s\r\n", i, train->train_num, train->last_sensor->name);
                    break;
                }
            }

            Conductor::UpdateTrainDisplay();

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
            char *dest = req->src;
            int offset = req->data;

            int train_index = get_train_index(train_num);
            if (train_index == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }
            train_task_mapping *train = &train_arr[train_index];
            IO_NS::PrintTerminal("CONDUCTOR::GO_COMMAND:Train %d found, sending it from %s to %s +{%d}\r\n",
                                 train_num, train->next_predicted_sensor->name, dest, offset);

            memcpy(train->destination, dest, 4);
            train->destination[4] = '\0';
            train->offset = offset;

            track.find_path(train->next_predicted_sensor->name, dest, &train->path, false, offset);
            uassert(false && "Conductor::ProcessRequest: Test finished -- forced error");

            Conductor::UpdateTrainDisplay();

            // track.find_path((char *)req->data);
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
            IO_NS::Print(MOVE_CURSOR "%s",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 36, train_arr[i].last_sensor->name);
            IO_NS::Print(MOVE_CURSOR "%s",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 50, train_arr[i].next_predicted_sensor->name);
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
        // TEST LOOP SWITCHES
        // IO_NS::PrintTerminal("Conductor testing loop switches\r\n");
        // Queue<PathNode, NUM_SWITCHES> switch_config;
        // int distance;
        // track.getLoop(&switch_config, &distance);
        // SetSwitches(&switch_config);
        // IO_NS::PrintTerminal("Conductor finished testing loop switches\r\n");

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
        while (true)
        {
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&req, sizeof(ConductorRequest));
            uassert(retval >= 0 && "Error receiving request from terminal");
            IO_NS::PrintTerminal("Conductor received %s from %d\r\n", req.requestType == RequestType::CMD ? "CMD" : (req.requestType == RequestType::GET_SEGMENT ? "GET_SEGMENT" : "GET_CMD"), sender_tid);

            bool sendReply = false;
            if (req.requestType == RequestType::CMD)
            {
                ProcessRequest(&(req.data.cmdRequest));
                sendReply = true;
            }

            else if (req.requestType == RequestType::GET_CMD)
            {
                // extract messenger TID
                for (int i = 0; i < NUM_TRAINS; i++)
                {
                    train_task_mapping *train = train_arr + i;
                    MessengerUnit *command_messenger = &train->command_messenger;
                    if (command_messenger->messenger_id == -1)
                    {
                        command_messenger->messenger_id = sender_tid;
                        command_messenger->sent_reply = false;
                        IO_NS::PrintTerminal("Conductor::ConductorLoop -- Messenger %d registered\r\n", sender_tid);
                        break;
                    }
                    else if (command_messenger->messenger_id == sender_tid)
                    {
                        // check if the messenger is already registered
                        IO_NS::PrintTerminal("Conductor::ConductorLoop -- Messenger %d already registered\r\n", sender_tid);
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
            MessengerUnit *command_messenger = &train->command_messenger;

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

    void start_conductor()
    {
        Conductor conductor;
    }

} // namespace Conductor_NS