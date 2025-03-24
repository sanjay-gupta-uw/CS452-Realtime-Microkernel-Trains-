#include "train.h"
// #include "../clock.h" // this includes util which includes rpi.h
#include "../include/marklin_io.h"
#include "../marklin/sensor.h"
#include "../include/name_server.h"
#include "../include/clock_server.h"
#include "../include/uassert.h"
#include "../../include/syscall.h"
#include "../include/track_node.h"
namespace Trains_NS
{
    static int MARKLIN_IO_SERVER_TID;
    // ********* Train Class *********
    // initialize with invaid train number

    static int get_speed_level_index(int speed)
    {
        if (speed <= LOW_SPEED)
        {
            return 0;
        }
        else if (speed <= MEDIUM_SPEED)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }
    Train::Train()
    {
        train_num = -1;
        train_speed = 0;
        MARKLIN_IO_SERVER_TID = -1;
        isReversed = false;
        cur_tick = 0;
        prev_tick = 0;

        initialize_distance_stacks();
    }

    Train::~Train()
    {
    }

    // sets all stack contents to 0
    void Train::initialize_distance_stacks()
    {
        dist_travelled.Push(0);
        dist_travelled.Push(0);
        stopping_target.Push(0);
        stopping_target.Push(0);
    }

    // use this constructor to set the train number
    Train::Train(int train_num, int MARKLIN_IO_SERVER_TID, int CLOCK_SERVER_TID)
        : train_num(train_num), MARKLIN_IO_SERVER_TID(MARKLIN_IO_SERVER_TID), CLOCK_SERVER_TID(CLOCK_SERVER_TID)
    {
        train_speed = 0;
        isReversed = false;

        TrainLoop();
    }

    void Train::Accelerate(int speed)
    {
        train_speed = speed;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, train_speed + 16, train_num};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    void Train::ReverseTrain()
    {
        int initial_speed = train_speed;
        if (initial_speed == 0)
        {
            // cannot reverse a stopped train
            // IO_NS::PrintTerminal("Cannot reverse a stopped train\r\n");
            return;
        }

        Stop();
        DELAY(CLOCK_SERVER_TID, 200); // stop for 2 seconds
        Reverse();

        Accelerate(initial_speed);
    }

    void Train::Reverse()
    {
        isReversed = !isReversed;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, REVERSE_CMD + 16, train_num};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    void Train::Stop()
    {
        train_speed = 0;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, train_speed + 16, train_num};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    bool Train::isMoving()
    {
        return (train_speed > 0);
    }

    int Train::getSpeed()
    {
        return train_speed;
    }

    // ********* End Train Class *********
    static bool isTrainValid(int train_num)
    {
        int valid_trains[NUM_TRAINS] = {1, 54, 55, 58, 77};
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            if (valid_trains[i] == train_num)
            {
                return true;
            }
        }
        return false;
    }

    void Train::update_position()
    {
        // calculate distance travelled
        if (train_speed <= 0)
        {
            return;
        }

        int index = get_speed_level_index(train_speed);
        if (approximate_speed[index] == -1)
        {
            return;
        }

        int dist = (cur_tick - prev_tick) * approximate_speed[index];

        // update distance travelled in segment
        int dist_travelled_in_segment;
        dist_travelled.Pop(&dist_travelled_in_segment);
        dist_travelled_in_segment += dist;
        dist_travelled.Push(dist_travelled_in_segment);
        // update stopping target in case it's within this segment
        int stopping_target_dist;
        stopping_target.Pop(&stopping_target_dist);
        stopping_target.Push(stopping_target_dist - dist);

        // check if we need to stop
        stopping_target.Pop(&stopping_target_dist);
        if (stopping_target_dist <= stopping_distance[get_speed_level_index(train_speed)])
        {
            // stop train
            Stop();
        }

        // NEED TO HANDLE:
        // CHECK IF SENSOR TRIGGER IS WITHIN A SPECFIC TIME FRAME
        // int dist_to_next_sensor = segment_length - dist_travelled_in_segment;
        // IF WINDOW IS EXCEEDED, REPLY TO MESSENGER FOR NEXT SEGMENT
        // ON INITIALIZATION, SET EXPECTED TIME TO BE LONGER THAN THE TIME IT TAKES TO TRAVEL THE SEGMENT
    }

    void Train::sensor_pos_update(int trigger_tick, int segment_length)
    {
        int total_dist;
        dist_travelled.Pop(&total_dist);
        dist_travelled.Pop(&total_dist);
        dist_travelled.Push(total_dist + segment_length);
        // if approximate speed > 0, we can compare the speed with ticks now vs when the sensor was triggered
        dist_travelled.Push(0);

        int stopping_target;
        this->stopping_target.Pop(&stopping_target);
        this->stopping_target.Pop(&stopping_target);

        this->stopping_target.Push(stopping_target - segment_length);

        this->segment_length = segment_length;
    }

    void Train::process_train_command(TrainResponse *response)
    {
        // process train command
        switch (response->command)
        {
        case TRAIN_COMMAND::ACCELERATE:
            IO_NS::PrintTerminal("Train %d: Accelerating to speed %d\r\n", train_num, response->speed);
            Accelerate(response->speed);
            break;
        case TRAIN_COMMAND::REVERSE:
            IO_NS::PrintTerminal("Train %d: Reversing\r\n", train_num);
            Reverse();
            break;
        case TRAIN_COMMAND::STOP:
            IO_NS::PrintTerminal("Received command from conductor to stop train %d\r\n", train_num);
            Stop();
            break;
        case TRAIN_COMMAND::TICK:
        {
            // update train position
            update_position();
        }
        case TRAIN_COMMAND::SENSOR_TARGET:
        {
            if (response->type == TrainResponseType::TRAIN_MESSENGER)
            {
            }
            else if (response->type == TrainResponseType::SENSOR_MESSENGER)
            {
                sensor_pos_update(response->trigger_tick, response->segment_length);
            }
        }
        // SENSOR_TRIGGERED
        // update train position (first stack element)
        default:
            break;
        }
    }

    void Train::TrainLoop()
    {
        // ConductorRequest train_query({BANKS::A, 1}, DIRECTION::FORWARD);

        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "Error finding SensorServer");
        // train loop
        // distance to next node
        int sender_tid;
        while (true)
        {
            cur_tick = TIME(CLOCK_SERVER_TID);
            TrainResponse response;
            int retval = RECEIVE(&sender_tid, (char *)&response, sizeof(TrainResponse));
            uassert(retval >= 0 && "Error receiving TrainResponse");

            process_train_command(&response);

            // process sensor
            prev_tick = cur_tick;

            REPLY(sender_tid, nullptr, 0);
        }
    }

    void spawn_train()
    {
        // SET SPEED TO 0
        int conductor_tid;

        TrainParams train_params;
        int retval = RECEIVE(&conductor_tid, (char *)&train_params, sizeof(TrainParams));

        int train_num = train_params.train_num;
        uassert(retval >= 0 && "Error receiving train_num from Conductor");

        IO_NS::PrintTerminal("Train %d spawned\r\n", train_num);
        REPLY(conductor_tid, (char *)true, sizeof(bool));

        int MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID > 0 && "Error finding MarklinIOServer");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        IO_NS::PrintTerminal("CLOCK SERVER TID: %d\r\n", CLOCK_SERVER_TID);
        uassert(CLOCK_SERVER_TID > 0 && "Error finding ClockServer");

        // CREATE MESSENGER
        int train_messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::train_messenger);
        uassert(train_messenger_tid > 0 && "Error creating train messenger");
        retval = SEND(train_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);

        // initialize train: get location of train
        // DETERMINE PATH TO NAVIGATE LOOP
        Train train(train_num, MARKLIN_IO_SERVER_TID, CLOCK_SERVER_TID);
    }

    // DELAY and resends message to parent train task
    void train_ticker()
    {
        int train_tid = MYPARENTTID();
        uassert(train_tid > 0 && "TRAIN TICKKER:Error finding parent task");

        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "TRAIN TICKER:Error finding ClockServer");

        TrainResponse response = {TrainResponseType::TRAIN_TICKER, TRAIN_COMMAND::TICK};
        while (true)
        {
            int retval = SEND(train_tid, (char *)&response, sizeof(TrainResponse), nullptr, 0);
            DELAY(CLOCK_SERVER_TID, 20);
        }
    }

    static SensorStruct get_sensor_from_conductor_request(SegmentReply *segment)
    {
        const char *sensor_name = segment->sensor_node->name;
        SensorStruct sensor = {};
        sensor.bank = BANKS(sensor_name[0] - 'A');
        sensor.id = a2ui((char **)&sensor_name[1], 3);
        return sensor;
    }

    // SENSOR/CONDUCTOR MESSENGER
    void train_messenger()
    {
        int my_tid = MYTID();
        // Train task should spawn its own messenger
        int train_num;
        int train_task_tid;
        int param_init_retval = RECEIVE(&train_task_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && train_task_tid >= 0 && "TRAIN MESSENGER: Error receiving train_num from parent task");
        REPLY(train_task_tid, nullptr, 0);

        IO_NS::PrintTerminal("Train %d messenger spawned with TID %d\r\n", train_num, my_tid);

        int conductor_tid = WHOIS("Conductor");
        uassert(conductor_tid > 0 && "TRAIN MESSENGER: Error finding Conductor");

        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "TRAIN MESSENGER: Error finding SensorServer");

        SegmentReply segment_reply;
        ConductorRequest conductor_request(train_task_tid);
        while (true)
        {
            // SEND SEGMENT REQUEST TO CONDUCTOR
            IO_NS::PrintTerminal("Train %d requesting segment from Conductor\r\n", train_num);
            int retval = SEND(conductor_tid, (char *)&conductor_request, sizeof(ConductorRequest), (char *)&segment_reply, sizeof(SegmentReply));
            uassert(retval >= 0 && "TRAIN MESSENGER: Error sending SegmentRequest to Conductor");

            IO_NS::PrintTerminal("Train %d received segment from Conductor\r\n", train_num);

            // SEND SENSOR QUERY TO SENSOR SERVER
            // HAVE SENSOR SERVER REPLY TO SENSOR MESSENGER IMMEDIATELY
            // SEND MESSAGE TO TRAIN TASK IF SENSOR IS TRIGGERED
            // NEED TO PASS TRAIN TID TO SENSOR SERVER
            SensorStruct sensor = get_sensor_from_conductor_request(&segment_reply);

            Sensors_NS::SensorResponse sensor_response;
            Sensors_NS::SensorQuery sensor_query = {Sensors_NS::SENSOR_COMMAND::TRAIN_SENSOR, sensor, train_task_tid, train_num};
            retval = SEND(sensor_server_tid, (char *)&sensor_query, sizeof(Sensors_NS::SensorQuery), (char *)&sensor_response, sizeof(Sensors_NS::SensorResponse));
            uassert(retval >= 0 && "TRAIN MESSENGER: Error sending SensorQuery to SensorServer");

            // NOTIFY TRAIN TASK THAT SENSOR WAS QUERIED, and pass train command if necessary
            // TRAIN WILL REPLY TO THIS IF IT RECEIVES MESSAGE FROM SENSOR SERVER, or if the time is up
            // THIS SHOULDN"T BE FREED UNTIL THE WINDOW FOR THE SENSOR TRIGGER HAS PASSED
            TrainResponse train_response = {TrainResponseType::TRAIN_MESSENGER, TRAIN_COMMAND::SENSOR_TARGET, 0, sensor, segment_reply.segment_length};
            retval = SEND(train_task_tid, (char *)&train_response, sizeof(TrainResponse), nullptr, 0);
        }
    }

} // namespace Trains_NS