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

        initialize_distance_stacks();

        // CREATE MESSENGERS
        path_messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::path_messenger);
        uassert(path_messenger_tid > 0 && "Error creating train messenger");
        int retval = SEND(path_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);

        sensor_messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::sensor_messenger);
        uassert(sensor_messenger_tid > 0 && "Error creating train messenger");
        retval = SEND(sensor_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);

        train_ticker_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::train_ticker);
        uassert(train_ticker_tid > 0 && "Error creating train messenger");
        retval = SEND(train_ticker_tid, (char *)&train_num, sizeof(int), nullptr, 0);

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

    // need to account for elapsed ticks since the sensor was hit and where we are now
    void Train::CompleteSegment()
    {
        // update distance travelled in segment
        int dist_travelled_in_segment;
        int total_dist;
        dist_travelled.Pop(&dist_travelled_in_segment);
        dist_travelled.Pop(&total_dist);

        dist_travelled.Push(total_dist + segment_length);
        dist_travelled.Push(0);

        // update stopping target
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

    void Train::process_train_command(TrainMessage *message)
    {
        // process train command
        switch (message->data.command)
        {
        case TRAIN_COMMAND::ACCELERATE:
            IO_NS::PrintTerminal("Train %d: Accelerating to speed %d\r\n", train_num, message->data.speed);
            Accelerate(message->data.speed);
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
        // SENSOR_TRIGGERED
        // update train position (first stack element)
        default:
            break;
        }
    }

    bool Train::ReleaseSensorMessenger()
    {
        has_read_target_sensor = true;
        REPLY(sensor_messenger_tid, (char *)&target_sensor, sizeof(SensorStruct));
    }

    void Train::TrainLoop()
    {
        int my_tid = MYTID();

        has_read_target_sensor = false;
        is_sensor_messenger_ready = false;

        while (true)
        {
            IO_NS::PrintTerminal(COLOR_MAGENTA "TRAIN %d: MYTID: %d\r\n", train_num, my_tid);
            cur_tick = TIME(CLOCK_SERVER_TID);
            IO_NS::PrintTerminal("TICK: %d\r\n", cur_tick);
            TrainMessage message;

            int sender_tid;
            bool send_reply = false;
            int retval = RECEIVE(&sender_tid, (char *)&message, sizeof(TrainMessage));
            uassert(retval >= 0 && "Error receiving TrainResponse");
            IO_NS::PrintTerminal("TrainLoop::Sender: %d, Command: %d, Speed: %d, Segment length %d, sensor hit: %d\r\n", sender_tid, message.data.command, message.data.speed, message.data.segment.segment_length, message.data.triggered_sensor.id);

            switch (message.type)
            {
            case TrainMessageType::PATH_MESSENGER:
            {
                SensorStruct sensor = message.data.segment.sensor;
                // check if it's a valid sensor
                if (sensor.id > 0)
                {
                    // valid sensor
                    target_sensor.bank = sensor.bank;
                    target_sensor.id = sensor.id;

                    has_read_target_sensor = false;

                    segment_length = message.data.segment.segment_length;
                }

                process_train_command(&message);
                break;
            }
            case TrainMessageType::SENSOR_MESSENGER:
            {
                SensorStruct triggered_sensor = message.data.triggered_sensor;
                // check if request is for target sensor, not target hit
                if (triggered_sensor.id < 0)
                {
                    is_sensor_messenger_ready = true;
                }
                else // this must be sensor hit
                {
                    CompleteSegment();
                    // need to free path messenger (or if window exceeded)
                    REPLY(path_messenger_tid, nullptr, 0);
                    send_reply = true; // free the sensor messenger so it can query the next sensor
                }
                break;
            }
            case TrainMessageType::TRAIN_TICKER:
                // update train position
                send_reply = true;
                break;

            default:
                break;
            }

            update_position();

            if (is_sensor_messenger_ready && !has_read_target_sensor)
            {
                // REPLY TO SENSOR MESSENGER
                ReleaseSensorMessenger();
            }

            // process sensor
            prev_tick = cur_tick;

            if (send_reply)
            {
                REPLY(sender_tid, nullptr, 0);
            }
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

        TrainMessage message();
        while (true)
        {
            int retval = SEND(train_tid, (char *)&message, sizeof(TrainMessage), nullptr, 0);
            DELAY(CLOCK_SERVER_TID, 20);
        }
    }

    // SENSOR/CONDUCTOR MESSENGER
    void path_messenger()
    {
        int my_tid = MYTID();
        // Train task should spawn its own messenger
        int train_num;
        int train_task_tid;
        int param_init_retval = RECEIVE(&train_task_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && train_task_tid >= 0 && "PATH MESSENGER: Error receiving train_num from parent task");
        REPLY(train_task_tid, nullptr, 0);

        IO_NS::PrintTerminal("PATH %d messenger spawned with TID %d\r\n", train_num, my_tid);

        int conductor_tid = WHOIS("Conductor");
        uassert(conductor_tid > 0 && "PATH MESSENGER: Error finding Conductor");

        TrainMessage conductor_response({}, -1, TRAIN_COMMAND::TICK);
        ConductorRequest conductor_request(train_task_tid);
        while (true)
        {
            // SEND SEGMENT REQUEST TO CONDUCTOR
            IO_NS::PrintTerminal("PATH MESSENGER:: requesting segment from Conductor for Train %d\r\n", train_num);
            int retval = SEND(conductor_tid, (char *)&conductor_request, sizeof(ConductorRequest), (char *)&conductor_response, sizeof(TrainMessage));
            uassert(retval >= 0 && "PATH MESSENGER: Error sending SegmentRequest to Conductor");

            IO_NS::PrintTerminal("PATH MESSENGER:: received segment from Conductor for Train %d\r\n", train_num);
            retval = SEND(train_task_tid, (char *)&conductor_response, sizeof(TrainMessage), nullptr, 0);
            uassert(retval >= 0 && "PATH MESSENGER: Error sending SegmentReply to Train task");
        }
    }

    void sensor_messenger()
    {
        // send to train task for current target sensor
        int my_tid = MYTID();
        int train_num;
        int train_task_tid;
        int param_init_retval = RECEIVE(&train_task_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && train_task_tid >= 0 && "SENSOR MESSENGER: Error receiving train_num from parent task");
        REPLY(train_task_tid, nullptr, 0);

        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "SENSOR MESSENGER: Error finding SensorServer");

        SensorStruct request_sensor = {BANKS::A, -1};
        TrainMessage sensor_request(-1, request_sensor);
        uassert(sensor_request.type == TrainMessageType::SENSOR_MESSENGER && "SENSOR MESSENGER: Error initializing TrainMessage");

        while (true)
        {
            // SEND MESSAGE TO PARENT FOR WHICH SENSOR TO WAIT FOR
            // TRAIN TASK SHOULD NOT ALLOW THIS TO SPAM THE SAME SENSOR
            int ret = SEND(train_task_tid, (char *)&sensor_request, sizeof(TrainMessage), nullptr, 0);
            uassert(ret >= 0 && "SENSOR MESSENGER: Error retrieving target sensor from train task");

            // SEND TO SENSOR SERVER TO WAIT FOR SENSOR HIT
            Sensors_NS::SensorQuery sensor_query = {Sensors_NS::SENSOR_COMMAND::TRAIN_SENSOR, request_sensor, train_task_tid, train_num};
            int retval = SEND(sensor_server_tid, (char *)&sensor_query, sizeof(Sensors_NS::SensorQuery), nullptr, 0);
            uassert(retval >= 0 && "SENSOR MESSENGER: Error sending SensorQuery to SensorServer");
        }
    }

} // namespace Trains_NS