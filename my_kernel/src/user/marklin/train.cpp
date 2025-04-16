#include "train.h"
// #include "../clock.h" // this includes util which includes rpi.h
#include "../include/marklin_io.h"
#include "../marklin/sensor.h"
#include "../include/name_server.h"
#include "../include/clock_server.h"
#include "../include/uassert.h"
#include "../../include/syscall.h"
#include "../include/track_node.h"

#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif
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
        // prev_tick = 0;
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
        cmd_messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::command_messenger);
        uassert(cmd_messenger_tid > 0 && "Error creating train messenger");
        int retval = SEND(cmd_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);
        uassert(retval >= 0 && "Error sending train number to command messenger");

        // path_messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::path_messenger);
        // uassert(path_messenger_tid > 0 && "Error creating train messenger");
        // retval = SEND(path_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);

        // sensor_messenger_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::sensor_messenger);
        // uassert(sensor_messenger_tid > 0 && "Error creating train messenger");
        // retval = SEND(sensor_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);

        train_ticker_tid = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::train_ticker);
        uassert(train_ticker_tid > 0 && "Error creating train messenger");

        stop_messenger_tid = CREATE(PRIORITY::MARKLIN_NOTIFIER, Trains_NS::stop_messenger);
        uassert(stop_messenger_tid > 0 && "Error creating train messenger");
        int retval2 = SEND(stop_messenger_tid, (char *)&train_num, sizeof(int), nullptr, 0);
        uassert(retval2 >= 0 && "Error sending train number to stop messenger");

        TrainLoop();
    }

    void Train::Accelerate(int speed)
    {
        train_speed = speed;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, train_speed + 16, train_num};
        IO_NS::PrintTerminal("SENDING SPEED %d to IO_SERVER %d at tick %d\r\n", train_speed, MARKLIN_IO_SERVER_TID);
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    void Train::ReverseTrain()
    {
        int initial_speed = train_speed;
        if (initial_speed > 0)
        {
            Stop();
            DELAY(CLOCK_SERVER_TID, 200); // stop for 2 seconds
        }
        Reverse();

        if (initial_speed > 0)
        {
            Accelerate(initial_speed);
        }
    }

    // sends reverse command to the train
    void Train::Reverse()
    {
        isReversed = !isReversed;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, REVERSE_CMD + 16, train_num};
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    static void STOP(int train_num, int MARKLIN_IO_SERVER_TID)
    {
        int train_speed = 0;
        MARKLIN_IO_SERVER::MarklinRequest request = {false, train_speed + 16, train_num};
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
        // IO_NS::PrintTerminal("Train %d: Updating position -- tick: %d\r\n", train_num, cur_tick);
    }

    void Train::process_train_command(TrainMessage *message)
    {
        // process train command
        switch (message->data.train_command.command)
        {
        case TRAIN_COMMAND::ACCELERATE:
        {
            IO_NS::PrintTerminal(COLOR_GREEN "Train %d: Received command to accelerate to speed %d\r\n", train_num, message->data.train_command.speed);
            Accelerate(message->data.train_command.speed);
            break;
        }
        case TRAIN_COMMAND::REVERSE:
            IO_NS::PrintTerminal(COLOR_GREEN "Train %d: Reversing\r\n", train_num);
            ReverseTrain();
            // uassert(false && "Train::process_train_command: Reversing train -- forced error");

            break;
        case TRAIN_COMMAND::STOP:
            IO_NS::PrintTerminal(COLOR_GREEN "Received command from conductor to stop train %d -- sending command at tick %d\r\n", train_num, cur_tick);
            Stop();
            // uassert(false && "TRAIN STOPPED -- FORCED ERROR");
            break;
            // move this to the train ticker switch case
        case TRAIN_COMMAND::TICK:
        {
            // update train position
            update_position();
        }
        // SENSOR_TRIGGERED
        // update train position (first stack element)
        case TRAIN_COMMAND::UPDATE_RESERVED:
        {
            // THIS IS THE CONDUCTOR MESSAGE THAT TELLS US HOW FAR WE CAN TRAVEL
            Accelerate(message->data.train_command.speed); // could determine speed from speed table to find suitable speed
            // is_destination_within_reach = message->data.train_command.is_destination_within_reserved;

            StoppingStruct stopping_struct;
            stopping_struct.destination_within_reach = is_destination_within_reach;
            // stopping_struct.delay_until_time = 0; // DETERMINE WHEN THIS IS, USING CONSTANT VELOCITY OR SHORT STOP
            // use message->data.train_command.reserved_distance

            REPLY(stop_messenger_tid, (char *)&stopping_struct, sizeof(StoppingStruct));
        }
        default:
            break;
        }
        IO_NS::PrintTerminal(COLOR_GREEN "Train %d: Processed command %d\r\n", train_num, message->data.train_command.command);
    }

    void Train::ReleaseSensorMessenger()
    {
        IO_NS::PrintTerminal(COLOR_MAGENTA "TRAIN::RELEASE SENSOR MESSENGER:{%d}, Sensor: {%c%d}\r\n", sensor_messenger_tid, target_sensor.bank, target_sensor.id);
        uassert(target_sensor.id > 0 && "Train::ReleaseSensorMessenger: Invalid sensor");
        has_read_target_sensor = true;
        is_sensor_messenger_ready = false;

        // IO_NS::PrintTerminal("REPLYING TO SENSOR MESSENGER\r\n");
        SensorStruct response = {target_sensor.bank, target_sensor.id};
        REPLY(sensor_messenger_tid, (char *)&response, sizeof(response));
        // IO_NS::PrintTerminal("REPLIED TO SENSOR MESSENGER\r\n");
    }

    void Train::TrainLoop()
    {
        int my_tid = MYTID();

        has_read_target_sensor = true; // this makes it wait for path to be executed first
        is_sensor_messenger_ready = false;
        targer_sensor_idx = -1;

        // IO_NS::PrintTerminal(CLEAR_SCREEN);
        is_destination_within_reach = false;

        while (true)
        {
            // IO_NS::PrintTerminal(COLOR_MAGENTA "TRAIN %d: MYTID: %d\r\n", train_num, my_tid);
            // IO_NS::Print(MOVE_CURSOR "%d",
            //              TRAIN_TABLE_Y + 5 + 0, TRAIN_TABLE_X + 80, cur_tick);

            // IO_NS::PrintTerminal(COLOR_MAGENTA "TRAIN::TICK: %d\r\n", cur_tick);
            TrainMessage message;

            int sender_tid;
            bool send_reply = false;
            // IO_NS::PrintTerminal(COLOR_MAGENTA "TRAIN %d: Waiting for message\r\n", train_num);
            int retval = RECEIVE(&sender_tid, (char *)&message, sizeof(TrainMessage));
            cur_tick = TIME(CLOCK_SERVER_TID);

            // check if sensor was triggered

            uassert(retval >= 0 && "Error receiving TrainResponse");
            IO_NS::PrintTerminal(COLOR_MAGENTA "TrainLoop{%d}::Sender: %d, MESSAGE TYPE: %d\r\n", train_num, sender_tid, message.type);

            // uassert(segment_length <= 0 && "TrainLoop::FINALLY RECEIVED SEGMENT");
            // uassert(false && "THIS MUST BE HIT --1!");
            switch (message.type)
            {
            case TrainMessageType::PATH_MESSENGER:
            {
                SensorStruct sensor = message.data.segment.sensor;
                // valid sensor
                target_sensor.bank = sensor.bank;
                target_sensor.id = sensor.id;
                targer_sensor_idx = (sensor.bank - 'A') * SENSORS_PER_BANK + (sensor.id - 1);
                MARKLIN_IO_SERVER::sensor_data[targer_sensor_idx].trigger_tick = 0; // reset trigger tick

                IO_NS::PrintTerminal(COLOR_MAGENTA "\r\n TRAIN{%d}::NEW TARGET SENSOR: %c%d\r\n", train_num, target_sensor.bank, target_sensor.id);

                has_read_target_sensor = false;
                segment_length = message.data.segment.segment_length;
                // uassert(false && "SUCCESFULLY UPDATE TRAINS NEXT SEGMENT");
                break;
            }
            case TrainMessageType::TRAIN_COMMAND:
            {
                IO_NS::PrintTerminal(COLOR_MAGENTA "TRAIN %d::Received command from Conductor: %d\r\n", train_num, message.data.train_command.command);
                process_train_command(&message);
                IO_NS::PrintTerminal(COLOR_MAGENTA "\r\nTRAIN %d::Processed command from Conductor: %d\r\n", train_num, message.data.train_command.command);
                send_reply = true;
                break;
            }
            case TrainMessageType::TRAIN_TICKER:
            {
                // update train position
                send_reply = true;
                break;
            }
            case TrainMessageType::STOP_MESSENGER:
            {
                send_reply = false;

                // HANG ON TO THIS UNTIL THE TRAIN RECEIVES UPDATE_RESERVED COMMAND
                break;
            }
            default:
                break;
            }

            // update_position();

            // if (is_sensor_messenger_ready && !has_read_target_sensor)
            // {
            //     // REPLY TO SENSOR MESSENGER
            //     ReleaseSensorMessenger();
            // }

            // process sensor
            if (send_reply)
            {
                // IO_NS::PrintTerminal("TRAIN %d::Sending reply to sender {%d}\r\n", train_num, sender_tid);
                REPLY(sender_tid, nullptr, 0);
            }
            else
            {
                IO_NS::PrintTerminal("TRAIN %d::NOT SENDING reply to sender {%d}\r\n", train_num, sender_tid);
            }

            // IO_NS::PrintTerminal("\r\n");
            // IO_NS::PrintTerminal(COLOR_MAGENTA "\r\nTRAIN %d::Done processing message\r\n", train_num);
            // uassert(false && "THIS MUST BE HIT!");
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
        int my_tid = MYTID();
        int train_tid = MYPARENTTID();
        uassert(train_tid > 0 && "TRAIN TICKKER:Error finding parent task");

        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "TRAIN TICKER:Error finding ClockServer");

        TrainMessage message(TrainMessageType::TRAIN_TICKER);
        while (true)
        {
            // IO_NS::PrintTerminal(COLOR_YELLOW "TRAIN TICKER{%d}:: Sending TICK to Train task\r\n", my_tid);
            int retval = SEND(train_tid, (char *)&message, sizeof(TrainMessage), nullptr, 0);
            DELAY(CLOCK_SERVER_TID, 20);
        }
    }

    // messenger for train stopped
    void stop_messenger()
    {
        int my_tid = MYTID();
        IO_NS::PrintTerminal(COLOR_YELLOW "STOP MESSENGER spawned with TID %d\r\n", my_tid);
        int train_num;
        int train_task_tid;
        int param_init_retval = RECEIVE(&train_task_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && train_task_tid >= 0 && "STOP MESSENGER: Error receiving train_num from parent task");
        REPLY(train_task_tid, nullptr, 0);

        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "STOP MESSENGER: Error finding ClockServer");

        int MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID > 0 && "STOP MESSENGER: Error finding MarklinIOServer");

        int CONDUCTOR_TID = WHOIS("Conductor");
        uassert(CONDUCTOR_TID > 0 && "STOP MESSENGER: Error finding Conductor");

        TrainMessage message(TrainMessageType::STOP_MESSENGER);

        while (true)
        {
            StoppingStruct stopping_struct;
            // SEND TO TRAIN TASK TO WAIT FOR STOP REPLY
            int retval = SEND(train_task_tid, (char *)&message, sizeof(TrainMessage), (char *)&stopping_struct, sizeof(StoppingStruct));
            uassert(retval >= 0 && "STOP MESSENGER: Error sending TrainMessage to Train task");
            // RESPONSE NEEDS TO CONTAIN THE TICK TO DELAY UNTIL

            DELAY_UNTIL(CLOCK_SERVER_TID, stopping_struct.delay_until_time);

            // SEND STOP COMMAND
            STOP(train_num, MARKLIN_IO_SERVER_TID);
            // THIS RUNS WHEN MARKLIN ACCEPTS THE STOP COMMAND

            if (stopping_struct.destination_within_reach)
            {
                // SEND TWO MESSAGES TO CONDUCTOR::
                // 1. RELEASE PATH
                // 1.a) DELAY FOR 2 SECONDS TO ALLOW OTHER TRAINS TO RESERVE PATH/MAKE PROGRESS
                ConductorRequest release_request(train_num, RequestType::RELEASE_PATH);
                int retval = SEND(CONDUCTOR_TID, (char *)&release_request, sizeof(ConductorRequest), nullptr, 0);
                uassert(retval >= 0 && "STOP MESSENGER: Error sending TrainCommandNotification to Conductor");

                // PATH IS RELEASED -- ONLY RESERVED NODE SHOULD BE DESTINATION!

                DELAY(CLOCK_SERVER_TID, 200); // wait for 2 seconds
                // 2. SEND GO COMMAND TO CONDUCTOR
                // ConductorRequest request(COMMAND::GOTO, train_num, 7, node_name, node_name, offset);
                // send node name to conductor
                // IO_NS::PrintTerminal("Attempting to find path for Train %d to go to %s %d with speed %d, sending to Conductor tid: %d\r\n", train_num, node_name, offset, speed, CONDUCTOR_TID);
                // SEND(CONDUCTOR_TID, (char *)&request, sizeof(request), nullptr, 0);
            }
            else
            {
                // SEND RESERVE PATH COMMAND TO CONDUCTOR
                // THIS ENABLES THE update_reserved_path FLAG IN CONDUCTOR (for specified train)
                // CONDUCTOR THEN TRIES TO FIND A PATH ON EACH REFRESH TICK FROM CONDUCTOR TICKER
                ConductorRequest conductor_request(COMMAND::UPDATE_RESERVED_PATH, train_num);
                int retval = SEND(CONDUCTOR_TID, (char *)&conductor_request, sizeof(ConductorRequest), nullptr, 0);
                uassert(retval >= 0 && "STOP MESSENGER: Error sending TrainCommandNotification to Conductor");
            }
        }
    }

    void command_messenger()
    {
        int my_tid = MYTID();
        IO_NS::PrintTerminal(COLOR_YELLOW "COMMAND MESSENGER spawned with TID %d\r\n", my_tid);
        int train_num;
        int train_task_tid;
        int param_init_retval = RECEIVE(&train_task_tid, (char *)&train_num, sizeof(int));
        uassert(param_init_retval >= 0 && train_task_tid >= 0 && "COMMAND MESSENGER: Error receiving train_num from parent task");
        REPLY(train_task_tid, nullptr, 0);

        int CONDUCTOR_TID = WHOIS("Conductor");

        ConductorRequest conductor_request(train_num);
        TrainCommandNotification command_struct;
        while (true)
        {
            IO_NS::PrintTerminal(COLOR_YELLOW "COMMAND MESSENGER{%d}:: waiting for command from Conductor\r\n", my_tid);
            int retval = SEND(CONDUCTOR_TID, (char *)&conductor_request, sizeof(ConductorRequest), (char *)&command_struct, sizeof(TrainCommandNotification));
            uassert(retval >= 0 && "COMMAND MESSENGER: Error sending TrainCommandNotification to Conductor");

            IO_NS::PrintTerminal(COLOR_YELLOW "COMMAND MESSENGER{%d}:: received command from Conductor for Train {%d}, relaying command...\r\n", my_tid, train_num);
            TrainMessage message(command_struct.command, command_struct.speed, command_struct.reserved_distance, command_struct.is_destination_within_reserved);
            retval = SEND(train_task_tid, (char *)&message, sizeof(TrainMessage), nullptr, 0);
            uassert(retval >= 0 && "COMMAND MESSENGER: Error sending TrainCommandNotification to Train task");
            // uassert(false && "HERE");

            IO_NS::PrintTerminal(COLOR_YELLOW "COMMAND MESSENGER{%d}:: Train accepted command %d\r\n", my_tid, command_struct.command);
        }
    }

} // namespace Trains_NS
