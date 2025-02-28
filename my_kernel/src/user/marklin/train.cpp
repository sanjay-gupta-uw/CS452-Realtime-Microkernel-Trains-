#include "train.h"
// #include "../clock.h" // this includes util which includes rpi.h
#include "../include/marklin_io.h"
static int MARKLIN_IO_SERVER_TID;

namespace Trains_NS
{
    static Train trains[5];

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
    }

    void Train::Reverse()
    {
        // int data = 15;
        // SendCommand(data);
        isReversed = !isReversed;
    }

    void Train::Stop()
    {
        train_speed = 0;
        // SendCommand(0);
    }

    bool Train::isMoving()
    {
        return (train_speed > 0);
    }

    // ********* End Train Class *********
    void init_trains()
    {
        const int TRAIN_NUMS[5] = {1, 54, 55, 58, 77};
        for (int i = 0; i < 5; ++i)
        {
            trains[i].train_num = TRAIN_NUMS[i];
        }
    }

    bool isValidRequest(MarklinRequest *req)
    {
        int train_num = req->id;

        for (int i = 0; i < 5; ++i)
        {
            if (trains[i].train_num == train_num)
            {

                switch (req->type)
                {
                case MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN:
                {
                    // ensure input speed is valid
                    int speed = int(req->data);
                    if (req->data != '\0' && speed >= MIN_SPEED && speed <= MAX_SPEED)
                    {
                        return true;
                    }
                    break;
                }
                case MARKLIN_REQUEST_TYPE::REVERSE_TRAIN:
                    return true;

                default:
                    return false;
                }
            }
        }
        return false;
    }
}