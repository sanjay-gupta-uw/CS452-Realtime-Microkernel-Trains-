#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
#include "../include/name_server.h"
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"

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
    }
    Conductor::~Conductor()
    {
    }

    void Conductor::ProcessRequest(MarklinRequest *req)
    {
        switch (req->type)
        {
        case COMMAND::SPAWN_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received SPAWN_TRAIN request for train %d\r\n", req->id);
            int spawned_train = CREATE(PRIORITY::P0, spawn_train);
            uassert(spawned_train > 0);
            bool reply = false;
            SEND(spawned_train, (char *)req->data, sizeof(req->data), (char *)&reply, sizeof(reply));
            uassert(reply && "Error spawning train");
            // trains.SpawnTrain(req->id);
            break;
        }
        case COMMAND::GOTO:
        {
            IO_NS::PrintTerminal("Conductor received GOTO request for train %d to node %s\r\n", req->id, req->data);
            // track.find_path((char *)req->data);
            break;
        }
        case COMMAND::READ_SENSOR:
        {
            IO_NS::PrintTerminal("Conductor received READ_SENSOR request for sensor %d\r\n", req->id);
            break;
        }
        case COMMAND::SET_SWITCH:
        {
            IO_NS::PrintTerminal("Conductor received SET_SWITCH request for switch %d\r\n", req->id);
            int switch_addr = req->id;
            Switch_NS::SWITCH_STATE state = (req->data == 'S') ? Switch_NS::SWITCH_STATE::STRAIGHT : Switch_NS::SWITCH_STATE::CURVED;
            bool success = switches.SetSwitch(switch_addr, state);
            // IO_NS::PrintTerminal("set switch result: %d\r\n", success);
            break;
        }
        case COMMAND::ACCELERATE_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received ACCELERATE_TRAIN request for train %d\r\n", req->id);
            int train_num = req->id;
            int speed = req->data;
            bool success = trains.AccelerateTrain(train_num, speed);
            break;
        }
        case COMMAND::REVERSE_STOP_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received REVERSE_STOP_TRAIN request for train %d\r\n", req->id);
            break;
        }
        case COMMAND::REVERSE_TRAIN:
        {
            IO_NS::PrintTerminal("Conductor received REVERSE_TRAIN request for train %d\r\n", req->id);
            int train_num = req->id;
            bool success = trains.ReverseTrain(train_num);
            break;
        }
        case COMMAND::SOLENOID_OFF:
        {
            IO_NS::PrintTerminal("Conductor received SOLENOID_OFF request for switch %d\r\n", req->id);
            // MARKLIN_IO_SERVER::IO_REQUEST req{MARKLIN_IO_SERVER::IO_REQUEST_TYPE::SEND_CMD, 0, &request};
            // SEND(MARKLIN_IO_SERVER_TID, (char *)&req, sizeof(req), (char *)&ret, sizeof(ret));
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

        MarklinRequest req;
        while (true)
        {
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&req, sizeof(MarklinRequest));
            conductor.ProcessRequest(&req);

            // find path to node
            // conductor.track.find_path((char *)find_node_name.node_name);
            REPLY(sender_tid, nullptr, 0);
        }
        EXIT();
    }

    void spawn_train()
    {
        // only conductor should be interacting with this
        int conductor_tid;
        int train_num;
        int retval = RECEIVE(&conductor_tid, (char *)&train_num, sizeof(train_num));
        uassert(retval > 0);
        IO_NS::PrintTerminal("Train %d spawned\r\n", train_num);
        REPLY(conductor_tid, (char *)true, sizeof(bool));

        // initialize train
        Trains_NS::Train train(train_num);
        // get location of train

        // FOR NOW ASSUME TRAIN IS AT START (hit A1)
        TrainQuery query = {{BANKS::A, 1}, DIRECTION::FORWARD};

        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "Error finding SensorServer");
        // train loop
        while (true)
        {
            // Send to the conductor with the current position and direction of travel
            TrainResponse response;
            int retval = SEND(conductor_tid, (char *)&query, sizeof(TrainQuery), (char *)&response, sizeof(TrainResponse));
            uassert(retval > 0 && "Error sending TrainQuery to Conductor");
            switch (response.command)
            {
            case TRAIN_COMMAND::ACCELERATE:
                uassert(response.speed >= 0 && response.speed <= 14);
                train.Accelerate(response.speed);
                break;
            case TRAIN_COMMAND::REVERSE:
                train.Reverse();
                IO_NS::PrintTerminal("Train::Reverse called, please implement delay in train.cpp\r\n");
                break;
            case TRAIN_COMMAND::STOP:
                // USE BIJECTION METHOD TO HAVE FINAL STOP AS CLOSE TO SENSOR AS POSSIBLE
                train.Stop();
                IO_NS::PrintTerminal("Train::Stop called, please implement precision stop\r\n");
                break;
            default:
                break;
            }

            // Reply contains the next sensor node to query for
            // Poll from next expected sensor bank
            SensorQuery sensor_query = {SENSOR_COMMAND::READ_BANK, response.sensor_req};
            retval = SEND(sensor_server_tid, (char *)&sensor_query, sizeof(SensorQuery), nullptr, 0);
            uassert(retval > 0 && "Error sending SensorQuery to SensorServer");

            // send waits for the sensor to be hit (not robust incase sensor is broken)
            query.sensor = response.sensor_req;
        }
    }

} // namespace Conductor_NS
