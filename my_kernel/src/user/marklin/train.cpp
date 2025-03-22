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
        TrainParams train_params;
        int retval = RECEIVE(&CONDUCTOR_TID, (char *)&train_params, sizeof(TrainParams));
        uassert(retval >= 0 && "Error receiving train_num from Conductor");

        train_num = train_params.train_num;

        if (train_num == 1)
        {
            CURRENT_SPEED_LOCATION = 3;
        }
        else if (train_num == 54)
        {
            CURRENT_SPEED_LOCATION = 4;
        }
        else if (train_num == 55)
        {
            CURRENT_SPEED_LOCATION = 5;
        }
        else if (train_num == 58)
        {
            CURRENT_SPEED_LOCATION = 6;
        }
        else if (train_num == 77)
        {
            CURRENT_SPEED_LOCATION = 7;
        }
        else
        {
            IO_NS::PrintTerminal("Error: Invalid train number %d\r\n", train_num);
            EXIT();
        }

        // FIND SERVER TIDS
        MARKLIN_IO_SERVER_TID = WHOIS("MarklinIOServer");
        uassert(MARKLIN_IO_SERVER_TID > 0 && "Error finding MarklinIOServer");
        CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "Error finding ClockServer");
        SENSOR_SERVER_TID = WHOIS("SensorServer");
        uassert(SENSOR_SERVER_TID > 0 && "Error finding SensorServer");

        isReversed = false;
        // SET SPEED TO 0
        train_speed = 0;
        Accelerate(0);
        REPLY(CONDUCTOR_TID, (char *)true, sizeof(bool));

        // RUN TRAIN LOOP
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
            Accelerate(LOW_SPEED);
        }

        isReversed = !isReversed;

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

    static DIRECTION getDirection(bool isReversed)
    {
        return isReversed ? DIRECTION::REVERSE : DIRECTION::FORWARD;
    }

    SensorStruct Train::GetLocation()
    {
        // SEND ACCELERATE COMMAND TO MARKLIN
        Accelerate(LOW_SPEED);

        // SEND SENSOR QUERY TO SENSOR SERVER
        Sensors_NS::SensorQuery sensor_query = {Sensors_NS::SENSOR_COMMAND::READ_ALL};
        Sensors_NS::SensorResponse sensor_response;
        int retval = SEND(SENSOR_SERVER_TID, (char *)&sensor_query, sizeof(Sensors_NS::SensorQuery), (char *)&sensor_response, sizeof(Sensors_NS::SensorResponse));
        if (!sensor_response.success)
        {
            return {BANKS::A, -1};
        }
        uassert(retval >= 0 && "Error sending SensorQuery to SensorServer");
        // this runs after a sensor is hit

        // reverse train and request sensor again
        DELAY(CLOCK_SERVER_TID, 25); // let train keep moving for a little bit

        // determine the reverse sensor
        int sensor_num = sensor_response.triggered_sensor.id;
        int reverse_sensor_num = (sensor_num % 2 == 0) ? sensor_num - 1 : sensor_num + 1;

        ReverseTrain();
        SensorStruct reverse_sensor = {sensor_response.triggered_sensor.bank, reverse_sensor_num};
        Sensors_NS::SensorResponse reverse_sensor_response;
        retval = SEND(SENSOR_SERVER_TID, (char *)&sensor_query, sizeof(Sensors_NS::SensorQuery), (char *)&reverse_sensor_response, sizeof(Sensors_NS::SensorResponse));
        uassert(retval >= 0 && "Error sending SensorQuery to SensorServer");
        if (!reverse_sensor_response.success)
        {
            // IO_NS::PrintTerminal("Error: Sensor %c%d not triggered\r\n", reverse_sensor.bank, reverse_sensor_num);
            return {BANKS::A, -1};
        }
        // this runs after the sensor is hit -- acts as a confirmation

        // reverse train back to original direction
        ReverseTrain();

        Accelerate(0); // stop train

        return sensor_response.triggered_sensor;
    }

    void Train::TrainLoop()
    {
        IO_NS::PrintTerminal("Train %d spawned\r\n", train_num);

        TICKER_TID = CREATE(PRIORITY::DEVICE_NOTIFIER, Trains_NS::train_ticker);

        // initialize train: get location of train
        // DETERMINE PATH TO NAVIGATE LOOP
        last_hit_sensor = GetLocation();
        if (last_hit_sensor.id == -1)
        {
            IO_NS::PrintTerminal("Error: Could not spawn train %d, please try again\r\n", train_num);
            EXIT();
        }

        ConductorRequest train_query(last_hit_sensor, getDirection(isReversed));
        int PREV_TICK = TIME(CLOCK_SERVER_TID);

        // train loop
        while (true)
        {
            int CUR_TICK = TIME(CLOCK_SERVER_TID);

            // Send to the conductor with the current position and direction of travel
            TrainResponse response;
            int retval = SEND(CONDUCTOR_TID, (char *)&train_query, sizeof(train_query), (char *)&response, sizeof(TrainResponse));
            uassert(retval >= 0 && "Error sending TrainQuery to Conductor");
            switch (response.command)
            {
            case TRAIN_COMMAND::TICK:
            {
                // TICKER
                break;
            }
            case TRAIN_COMMAND::ACCELERATE:
                IO_NS::PrintTerminal("Accelerating train %d to speed %d\r\n", train_num, response.speed);
                uassert(response.speed >= 0 && response.speed <= 14);
                Accelerate(response.speed);
                break;
            case TRAIN_COMMAND::REVERSE:
                IO_NS::PrintTerminal("Reversing train %d\r\n", train_num);
                ReverseTrain();
                break;
            case TRAIN_COMMAND::STOP:
                // USE BIJECTION METHOD TO HAVE FINAL STOP AS CLOSE TO SENSOR AS POSSIBLE
                // SHOULD COMPUTE MEAN STOPPING DISTANCE/VELOCITY
                Stop();
                break;

            default:
                break;
            }

            // PASS BACK DISTANCE TO NEXT SENSOR

            PREV_TICK = CUR_TICK;
        }
    }

    // add train ticker to determine pos/time

    void spawn_train()
    {
        Train train;
    }

    void train_ticker()
    {
        int TRAIN_SERVER_TID = WHOIS("TrainServer");
        uassert(TRAIN_SERVER_TID > 0 && "Error finding TrainServer");
        int CLOCK_SERVER_TID = WHOIS("ClockServer");
        uassert(CLOCK_SERVER_TID > 0 && "Error finding ClockServer");

        TrainResponse response = {TRAIN_COMMAND::TICK};

        while (true)
        {
            int retval = SEND(TRAIN_SERVER_TID, (char *)&response, sizeof(TrainResponse), nullptr, 0);
            DELAY(CLOCK_SERVER_TID, 5);
        }
    }
} // namespace Trains_NS