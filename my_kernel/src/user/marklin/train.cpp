#include "train.h"
// #include "../clock.h" // this includes util which includes rpi.h
#include "../include/marklin_io.h"

namespace Trains_NS
{
    static int MARKLIN_IO_SERVER_TID;
    // ********* Train Class *********
    // initialize with invaid train number
    Train::Train()
    {
        train_num = -1;
        train_speed = 0;
        headlight_on = false;
        MARKLIN_IO_SERVER_TID = -1;
        isReversed = false;
    }

    Train::~Train()
    {
    }

    // use this constructor to set the train number
    Train::Train(int train_num) : train_num(train_num)
    {
        train_speed = 0;
        headlight_on = false;
    }

    void Train::HeadlightOn()
    {
        headlight_on = true;
    }

    void Train::Accelerate(int speed)
    {
        train_speed = speed;
        MarklinRequest request;
        request.type = COMMAND::ACCELERATE_TRAIN;
        request.id = train_num;
        request.data = speed + 16; // turn on the lights
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    void Train::Reverse()
    {
        // int data = 15;
        // SendCommand(data);
        isReversed = !isReversed;
        MarklinRequest request;
        request.type = COMMAND::REVERSE_TRAIN;
        request.id = train_num;
        // request.data = REVERSE_CMD;
        MARKLIN_IO_SERVER::SendCmd(MARKLIN_IO_SERVER_TID, &request);
    }

    void Train::Stop()
    {
        // SendCommand(0);
        train_speed = 0;
        Accelerate(0);
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
    Trains::Trains()
    {
        const int TRAIN_NUMS[5] = {1, 54, 55, 58, 77};

        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            trains[i] = Train(TRAIN_NUMS[i]);
        }
    }

    Trains::~Trains()
    {
    }

    // check if the supplied train number is supported (returns -1 if not found)
    Train *Trains::isTrainValid(int train_num)
    {
        for (int i = 0; i < NUM_TRAINS; ++i)
        {
            if (trains[i].train_num == train_num)
            {
                return &trains[i];
            }
        }
        return nullptr;
    }

    // check if the supplied speed is valid (range 0-14)
    bool Trains::isSpeedValid(int speed)
    {
        return (speed >= MIN_SPEED && speed <= MAX_SPEED);
    }

    bool Trains::AccelerateTrain(int train_num, int speed)
    {
        Train *train = isTrainValid(train_num);
        if (train == nullptr || !isSpeedValid(speed))
        {
            return false;
        }

        train->Accelerate(speed);
        return true;
    }

    bool Trains::ReverseTrain(int train_num)
    {
        Train *train = isTrainValid(train_num);
        if (train == nullptr)
        {
            return false;
        }

        // current speed:
        int initial_speed = train->getSpeed();
        if (initial_speed > 0)
        {
            train->Stop();
        }

        train->Reverse();

        // if the train was moving, start it again
        if (initial_speed > 0)
        {
            train->Accelerate(initial_speed);
        }
        return true;
    }

    void Trains::setIOServerTid(int tid)
    {
        MARKLIN_IO_SERVER_TID = tid;
    }
} // namespace Trains_NS