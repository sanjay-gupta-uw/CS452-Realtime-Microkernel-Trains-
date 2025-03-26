#include "train.h"
// #include "../clock.h" // this includes util which includes rpi.h
#include "../include/marklin_io.h"
#include "../marklin/sensor.h"
#include "../include/name_server.h"
#include "../include/clock_server.h"
#include "../include/uassert.h"
#include "../../include/syscall.h"
namespace Trains_NS
{
    static int MARKLIN_IO_SERVER_TID;
    // ********* Train Class *********
    // initialize with invaid train number
    Train::Train()
    {
        train_num = -1;
        train_speed = 0;
        MARKLIN_IO_SERVER_TID = -1;
        isReversed = false;
    }

    Train::~Train()
    {
    }

    // use this constructor to set the train number
    Train::Train(int train_num, int MARKLIN_IO_SERVER_TID, int CLOCK_SERVER_TID)
        : train_num(train_num), MARKLIN_IO_SERVER_TID(MARKLIN_IO_SERVER_TID), CLOCK_SERVER_TID(CLOCK_SERVER_TID)
    {
        train_speed = 0;
        isReversed = false;
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

    void spawn_train()
    {
        // only conductor should be interacting with this
        int conductor_tid;

        TrainParams train_params;
        int retval = RECEIVE(&conductor_tid, (char *)&train_params, sizeof(TrainParams));

        int train_num = train_params.train_num;
        // SET SPEED TO 0
        uassert(retval >= 0 && "Error receiving train_num from Conductor");
        IO_NS::PrintTerminal("Train %d spawned\r\n", train_num);
        REPLY(conductor_tid, (char *)true, sizeof(bool));

        int MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID > 0 && "Error finding MarklinIOServer");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "Error finding ClockServer");
        // initialize train: get location of train
        // DETERMINE PATH TO NAVIGATE LOOP
        Trains_NS::Train train(train_num, MARKLIN_IO_SERVER_TID, CLOCK_SERVER_TID);
        ConductorRequest train_query({BANKS::A, 1}, DIRECTION::FORWARD);

        int sensor_server_tid = WHOIS("SensorServer");
        uassert(sensor_server_tid > 0 && "Error finding SensorServer");
        // train loop
        while (true)
        {
            // Send to the conductor with the current position and direction of travel
            TrainResponse response;
            int retval = SEND(conductor_tid, (char *)&train_query, sizeof(train_query), (char *)&response, sizeof(TrainResponse));
            uassert(retval >= 0 && "Error sending TrainQuery to Conductor");
            switch (response.command)
            
            {
            case TRAIN_COMMAND::ACCELERATE:
                IO_NS::PrintTerminal("Accelerating train %d to speed %d\r\n", train_num, response.speed);
                uassert(response.speed >= 0 && response.speed <= 14);
                train.Accelerate(response.speed);
                break;
            case TRAIN_COMMAND::REVERSE:
                IO_NS::PrintTerminal("Reversing train %d\r\n", train_num);
                train.ReverseTrain();
                break;
            case TRAIN_COMMAND::STOP:
                // USE BIJECTION METHOD TO HAVE FINAL STOP AS CLOSE TO SENSOR AS POSSIBLE
                // SHOULD COMPUTE MEAN STOPPING DISTANCE/VELOCITY
                train.Stop();
                break;
            default:
                break;
            }

            // Reply contains the next sensor node to query for
            // Poll from next expected sensor bank
            // SensorStruct next_sensor;
            // next_sensor.bank = BANKS::A;
            // next_sensor.id = 3; // A4

            // Sensors_NS::SensorQuery sensor_query = {Sensors_NS::SENSOR_COMMAND::READ_BANK, next_sensor};
            // retval = SEND(sensor_server_tid, (char *)&sensor_query, sizeof(Sensors_NS::SensorQuery), nullptr, 0);
            // uassert(retval >= 0 && "Error sending SensorQuery to SensorServer");

            // // send waits for the sensor to be hit (not robust incase sensor is broken)
            // train_query.data.trainQuery.sensor = response.sensor;
        }
    }

} // namespace Trains_NS