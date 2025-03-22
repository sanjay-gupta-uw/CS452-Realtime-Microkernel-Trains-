#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
#include "../include/name_server.h"
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"
#include "../marklin/train.h"
#include "../../include/syscall.h"
#include "../../containers/stack.h"

typedef struct IO_REQUEST
{
    unsigned char ch;
};

namespace Conductor_NS
{
    Conductor::Conductor()
    {
        IO_NS::PrintTerminal("Starting Conductor\r\n");
        // Track track;
        REGISTERAS("Conductor");
        IO_NS::PrintTerminal("Conductor started\r\n");

        // create sensor server
        SENSOR_SERVER_TID = CREATE(PRIORITY::DEVICE_SERVER, Sensors_NS::SensorServer);
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
                    break;
                }
            }

            IO_NS::PrintTerminal("Train %d spawned successfully\r\n", req->id);
            break;
        }
        case COMMAND::GOTO:
        {
            IO_NS::PrintTerminal("UNIMPLEMENTED: Conductor received GOTO request\r\n");

            // track.find_path((char *)req->data);
            break;
        }
        case COMMAND::NAVIGATE_LOOP:
        {
            IO_NS::PrintTerminal("UNIMPLEMENTED: Conductor received NAVIGATE_LOOP request for train %d\r\n", req->id);
            // Sensor current_sensor = req->data;
            break;
        }
        default:
            IO_NS::PrintTerminal("Conductor received INVALID request\r\n");
            break;
        }
    }

    void Conductor::CalibrateTrain(int train_num)
    {
        int train_index = get_train_index(train_num);
        train_task_mapping *train = &train_arr[train_index];

        if (train_index == -1)
        {
            IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
            return;
        }
        IO_NS::PrintTerminal("Beginning calibration for train %d\r\n", train_num);

        // Find Path to loop
        Stack<PathNode, TRACK_MAX> path;

        track.find_path(train->last_hit_sensor.name, "B5", &path); // B5 is a loop node
        // check if path is empty
        uassert(!path.IsEmpty() && "Error finding path to loop");

        PathNode start_node;
        path.Pop(&start_node);

        int count = 0;
        while (!path.IsEmpty())
        {
            PathNode node;
            path.Pop(&node);
            if (count == 0)
            {
                TrainResponse response = {TRAIN_COMMAND::ACCELERATE, MEDIUM_SPEED};
                if (start_node.node == node.node->reverse) // send reverse command to train
                {
                    response = {TRAIN_COMMAND::REVERSE};
                }
                train->train_response_queue.Push(response);
            }
            if (node.node->type == NODE_BRANCH)
            {
                // switch
                Switch_NS::SwitchRequest switch_req = {false, node.node->num, Switch_NS::SWITCH_STATE::STRAIGHT};
                int retval = SEND(SWITCH_SERVER_TID, (char *)&switch_req, sizeof(switch_req), nullptr, 0);
                uassert(retval >= 0 && "Error sending switch request");
            }

            count++;
        }

        // once B5 is tripped, set train to loop
        // after sending last train command, we will receive the next when the train hits B5

        // should then turn off train, and set to loop
        // call another function to get destination once constant speed is reached
    }

    void start_conductor()
    {
        Conductor conductor;
        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        conductor.track.init(track_id);

        REPLY(sender_tid, nullptr, 0);
        // test
        {
            // test path finding
            Stack<PathNode, TRACK_MAX> path;
            conductor.track.find_path("E1", "E14", &path);
            conductor.track.find_path("E9", "D8", &path);
            conductor.track.find_path("A1", "A2", &path);
            conductor.track.find_path("A1", "E7", &path);
            conductor.track.find_path("A2", "E7", &path);
        }

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
                conductor.ProcessRequest(&(req.data.cmdRequest));
                sendReply = true;
            }
            else
            {
                IO_NS::PrintTerminal("Conductor received non-cmd request\r\n");
                // find train from TID and set isWaitingForCommand to true
                for (int i = 0; i < NUM_TRAINS; i++)
                {
                    if (conductor.train_arr[i].task_id == sender_tid)
                    {
                        IO_NS::PrintTerminal("Train %d is waiting for command\r\n", conductor.train_arr[i].train_num);
                        conductor.train_arr[i].isWaitingForCommand = true;
                        break;
                    }
                }
            }

            conductor.DispatchTrainCommand();

            // find path to node
            // conductor.track.find_path((char *)find_node_name.node_name);

            if (sendReply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
        }
        EXIT();
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

} // namespace Conductor_NS
