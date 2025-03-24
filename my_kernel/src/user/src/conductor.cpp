#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
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
        // // create switch server
        SWITCH_SERVER_TID = CREATE(PRIORITY::DEVICE_SERVER, Switch_NS::SwitchServer);
        uassert(SWITCH_SERVER_TID > 0 && "Conductor::Error creating switch server");
        IO_NS::PrintTerminal("Switch server created with TID %d\r\n", SWITCH_SERVER_TID);

        // initialize train_arr
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            train_arr[i].task_id = -1;
            train_arr[i].train_num = -1;
            train_arr[i].isWaitingForCommand = false;
        }

        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        track.init(track_id);
        REPLY(sender_tid, nullptr, 0);

        InitializeTrainDisplay();
        ConductorLoop();
    }
    Conductor::~Conductor()
    {
    }

    void Conductor::DispatchTrainCommand()
    {
        for (int i = 0; i < NUM_TRAINS; i++)
        {
            if (train_arr[i].isWaitingForCommand && !train_arr[i].train_response_queue.IsEmpty())
            {
                // IO_NS::PrintTerminal("Conductor dispatching command to train %d\r\n", train_arr[i].train_num);
                TrainResponse response;
                int retval = train_arr[i].train_response_queue.Pop(&response);
                uassert(retval >= 0 && "Error popping train response");
                train_arr[i].isWaitingForCommand = false;
                REPLY(train_arr[i].task_id, (char *)&response, sizeof(TrainResponse));
            }
        }
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

            train_arr[train_index].speed_level = speed;
            //train_arr[train_index].actual_speed_x10 = ;
            Conductor::UpdateTrainDisplay();

            TrainResponse response = {TRAIN_COMMAND::ACCELERATE, speed};
            train_arr[train_index].train_response_queue.Push(response);
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

            TrainResponse response = {TRAIN_COMMAND::REVERSE, 0};
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

            bool spawned = false;

            for (int i = 0; i < NUM_TRAINS; i++)
            {
                if (train_arr[i].train_num == req->id)
                {
                    IO_NS::PrintTerminal("Train %d spawned already\r\n", req->id);
                    spawned = true;
                    break;
                }
            }

            if(spawned) break;

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
                    train_arr[i].train_num = req->id;
                    train_arr[i].task_id = spawned_train_tid;
                    train_arr[i].isWaitingForCommand = true;
                    train_arr[i].speed_level = 0;                  
                    train_arr[i].actual_speed_x10 = 0;
                    train_arr[i].recent_sensor_bank = '\0';
                    train_arr[i].recent_sensor_num = -1;
                    train_arr[i].next_predicted_bank = sensor_bank;
                    train_arr[i].next_predicted_num = sensor_number;
                    memset(train_arr[i].destination, '-', 4);
                    train_arr[i].destination[4] = '\0'; 
                    train_arr[i].offset = 0;
                    break;
                }
            }

            Conductor::UpdateTrainDisplay();

            IO_NS::PrintTerminal("Train %d spawned successfully\r\n", req->id);
            break;
        }
        case COMMAND::GOTO:
        {
            IO_NS::PrintTerminal("Conductor received SPAWN_TRAIN request for train %d\r\n", req->id);

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
            else
            {
                IO_NS::PrintTerminal("Train %d found, sending it to %s %d\r\n",
                    train_num, dest, offset);
            }

            memcpy(train_arr[train_index].destination, dest, 4);
            train_arr[train_index].destination[4] = '\0'; 
            train_arr[train_index].offset = offset;

            Conductor::UpdateTrainDisplay();
                                 
            // track.find_path((char *)req->data);
            break;
        }
        case COMMAND::NAVIGATE_LOOP:
        {
            IO_NS::PrintTerminal("UNIMPLEMENTED: Conductor received NAVIGATE_LOOP request for train %d\r\n", req->id);
            // Sensor current_sensor = req->data;
            break;
        }
        /*case COMMAND::SENSOR_TRIGGER:
        {
            // Extract sensor ID from request
            int sensor_number = req->id;
            char sensor_bank = req->src[0];

            IO_NS::PrintTerminal("Triggered sensor: bank=%c, number=%d\r\n", sensor_bank, sensor_number);
            track_node* sensor_node = track.find_sensor(bank, number);

            for (int i = 0; i < NUM_TRAINS; i++) {
                if (train_arr[i].next_predicted_id == sensor_node) {
                    // Update train's sensor information
                    train_arr[i].recent_sensor_bank = bank;
                    train_arr[i].recent_sensor_num = number;
                    train_arr[i].recent_sensor_id = sensor_node;

                    // Predict next sensor
                    int dist;
                    track_node* next_sensor = track.get_node_by_name(sensor_id);
                    train_arr[i].predicted_node = next_sensor;

                    // Update display
                    if (next_sensor) {
                        train_arr[i].next_predicted_bank = next_sensor->name[0];
                        train_arr[i].next_predicted_num =
                            std::stoi(next_sensor->name + 1);
                    }
                    UpdateTrainDisplay();
                    break;
                }
                }
            break;
        }*/
        case COMMAND::STOP_ALL:
        {
            IO_NS::PrintTerminal("JACK2----------------------Conductor received STOP_ALL request");
            for (int i = 0; i < NUM_TRAINS; i++)
            {
                if (train_arr[i].train_num != -1)
                {
                    IO_NS::PrintTerminal("Sending STOP command to Train %d\r\n", train_arr[i].train_num);
                    TrainResponse response = {TRAIN_COMMAND::ACCELERATE, 0};
                    train_arr[i].speed_level = 0;
                    train_arr[i].actual_speed_x10 = 0;;
                    train_arr[i].train_response_queue.Push(response);
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

            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 2, train_arr[i].train_num);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 7, train_arr[i].speed_level);
            IO_NS::Print(MOVE_CURSOR "%d.%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 21, train_arr[i].actual_speed_x10, train_arr[i].actual_speed_x10 % 10);
            if(train_arr[i].recent_sensor_bank == '\0') {
                IO_NS::Print(MOVE_CURSOR "-",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 36);
            }         
            else
            {
                IO_NS::Print(MOVE_CURSOR "%c",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 36, train_arr[i].recent_sensor_bank);
                IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 37, train_arr[i].recent_sensor_num);
            }
            IO_NS::Print(MOVE_CURSOR "%c",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 50, train_arr[i].next_predicted_bank);
            IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 51, train_arr[i].next_predicted_num);
            if(train_arr[i].destination[0] == '-') {
                IO_NS::Print(MOVE_CURSOR "-",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 64);
                IO_NS::Print(MOVE_CURSOR "-",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 71);
            }         
            else
            {
                IO_NS::Print(MOVE_CURSOR "%s",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 64, train_arr[i].destination);
                IO_NS::Print(MOVE_CURSOR "%d",
                         TRAIN_TABLE_Y + 5 + display_row, TRAIN_TABLE_X + 71, train_arr[i].offset);
            }            
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
        IO_NS::PrintTerminal("Conductor testing loop switches\r\n");
        Queue<PathNode, NUM_SWITCHES> switch_config;
        int distance;
        track.getLoop(&switch_config, &distance);
        SetSwitches(&switch_config);
        IO_NS::PrintTerminal("Conductor finished testing loop switches\r\n");

        // test path finding
        IO_NS::PrintTerminal("Conductor testing path finding\r\n");
        Stack<PathNode, TRACK_MAX> path;
        track.find_path("E1", "E14", &path);
        track.find_path("E9", "D8", &path);
        track.find_path("A1", "A2", &path);
        track.find_path("A1", "E7", &path);
        track.find_path("A2", "E7", &path);
        IO_NS::PrintTerminal("Conductor finished testing path finding\r\n");
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
            else
            {
                IO_NS::PrintTerminal("Conductor received non-cmd request\r\n");
                // find train from TID and set isWaitingForCommand to true
                for (int i = 0; i < NUM_TRAINS; i++)
                {
                    if (train_arr[i].task_id == sender_tid)
                    {
                        IO_NS::PrintTerminal("Train %d is waiting for command\r\n", train_arr[i].train_num);
                        train_arr[i].isWaitingForCommand = true;
                        break;
                    }
                }
            }

            DispatchTrainCommand();

            // find path to node
            // conductor.track.find_path((char *)find_node_name.node_name);

            if (sendReply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
        }
        EXIT();
    }

    void start_conductor()
    {
        Conductor conductor;
    }

} // namespace Conductor_NS