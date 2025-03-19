#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
#include "../include/name_server.h"
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"
#include "../marklin/train.h"
#include "../../include/syscall.h"

typedef struct IO_REQUEST
{
    unsigned char ch;
};

namespace Conductor_NS
{
    static int get_sleeping_task_id(Queue<Conductor::train_task_mapping, NUM_TRAINS> *sleeping_trains, int train_num)
    {
        // need a temp queue to store the popped elements
        Queue<Conductor::train_task_mapping, NUM_TRAINS> temp_queue;

        Conductor::train_task_mapping mapping;
        int train_tid = -1;
        while (sleeping_trains->Pop(&mapping))
        {
            if (mapping.train_num != train_num)
            {
                temp_queue.Push(mapping);
            }
            else
            {
                train_tid = mapping.data;
            }
        }

        // restore the elements back to the original queue
        while (!temp_queue.IsEmpty())
        {
            temp_queue.Pop(&mapping);
            sleeping_trains->Push(mapping);
        }

        return train_tid;
    }

    Conductor::Conductor()
    {
        IO_NS::PrintTerminal("Starting Conductor\r\n");
        // Track track;
        REGISTERAS("Conductor");
        IO_NS::PrintTerminal("Conductor started\r\n");

        // create sensor server
        SWITCH_SERVER_TID = CREATE(PRIORITY::P0, Switch_NS::SwitchServer);
        uassert(SWITCH_SERVER_TID > 0 && "Conductor::Error creating switch server");
        // create switch server
        SENSOR_SERVER_TID = CREATE(PRIORITY::P0, Sensors_NS::SensorServer);
        uassert(SENSOR_SERVER_TID > 0 && "Conductor::Error creating sensor server");
    }
    Conductor::~Conductor()
    {
    }

    void Conductor::ProcessRequest(CMDRequest *req)
    {
        switch (req->command)
        {
        case COMMAND::SET_SWITCH:
        {
            IO_NS::PrintTerminal("Conductor received SET_SWITCH request for switch %d\r\n", req->id);
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

            int train_tid = get_sleeping_task_id(&sleeping_trains, train_num);
            if (train_tid == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }
            IO_NS::PrintTerminal("Train %d found, sending ACCELERATE command with speed %d\r\n", train_num, speed);
            TrainResponse response = {TRAIN_COMMAND::ACCELERATE, speed};
            int retval = REPLY(train_tid, (char *)&response, sizeof(TrainResponse));
            uassert(retval > 0);
            break;
        }
        case COMMAND::REVERSE_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received REVERSE_TRAIN request for train %d\r\n", req->id);
            int train_num = req->id;

            int train_tid = get_sleeping_task_id(&sleeping_trains, train_num);
            if (train_tid == -1)
            {
                IO_NS::PrintTerminal("Train %d not found or initialized.\r\n", train_num);
                return;
            }

            TrainResponse response = {TRAIN_COMMAND::REVERSE, 0};
            int retval = REPLY(train_tid, (char *)&response, sizeof(TrainResponse));
            uassert(retval > 0);
            break;
        }
        case COMMAND::SPAWN_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received SPAWN_TRAIN request for train %d\r\n", req->id);
            int spawned_train = CREATE(PRIORITY::P0, Trains_NS::spawn_train);
            uassert(spawned_train > 0);
            bool reply = false;
            SEND(spawned_train, (char *)req->id, sizeof(req->data), (char *)&reply, sizeof(reply));
            uassert(reply && "Error spawning train");

            sleeping_trains.Push({spawned_train, req->id});
            // trains.SpawnTrain(req->id);
            break;
        }
        case COMMAND::GOTO:
        {
            IO_NS::PrintTerminal("Conductor received GOTO request for train %d to node %s\r\n", req->id, req->data);
            // track.find_path((char *)req->data);
            break;
        }
        case COMMAND::NAVIGATE_LOOP:
        {
            IO_NS::PrintTerminal("Conductor received NAVIGATE_LOOP request for train %d\r\n", req->id);
            // Sensor current_sensor = req->data;
            break;
        }
        default:
            IO_NS::PrintTerminal("Conductor received INVALID request\r\n");
            break;
        }
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

        // spawn sensor task
        int sensor_server_tid = CREATE(PRIORITY::P0, Sensors_NS::SensorServer);

        ConductorRequest req;
        while (true)
        {
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&req, sizeof(ConductorRequest));
            uassert(retval > 0 && "Error receiving request from terminal");

            if (req.requestType == RequestType::CMD)
            {
                IO_NS::PrintTerminal("Conductor received cmd request\r\n");
                conductor.ProcessRequest(&(req.data.cmdRequest));
            }
            else
            {
                IO_NS::PrintTerminal("Conductor received non-cmd request\r\n");
                // process train request
            }

            // find path to node
            // conductor.track.find_path((char *)find_node_name.node_name);
            REPLY(sender_tid, nullptr, 0);
        }
        EXIT();
    }

} // namespace Conductor_NS
