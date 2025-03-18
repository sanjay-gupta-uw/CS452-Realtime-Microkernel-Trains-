#ifndef _train_h
#define _train_h
#define MARKLIN 2
#include <stdbool.h>
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"
// #include "rpi.h"

enum class TRAIN_COMMAND
{
    ACCELERATE,
    REVERSE,
    STOP,
};

enum class DIRECTION
{
    FORWARD,
    REVERSE,
};

// train query to conductor
struct TrainQuery // FIX THIS -- DOESNT INCLUDE COMAND
// CONDUCTOR TAKES MARKLIN REQUEST?
{
    SensorStruct sensor;
    DIRECTION direction; // direction of the train
};

struct TrainResponse
{
    TRAIN_COMMAND command; // command to be executed, if any
    int speed;             // assume stop is only issued when next sensor is the destination
    SensorStruct sensor;
};

namespace Trains_NS
{
#define NUM_TRAINS 5
#define MAX_SPEED 14
#define MIN_SPEED 0
    class Train
    {
    private:
        int MARKLIN_IO_SERVER_TID;
        int CLOCK_SERVER_TID;
        int train_speed; // between 0 and 14
        bool isReversed;

        void Reverse(); // sends reverse train command to marklin

    public:
        int train_num;
        Train();
        Train(int train_num, int MARKLIN_IO_SERVER_TID, int CLOCK_SERVER_TID);
        ~Train();

        void Accelerate(int speed); // sends accelerate train command to marklin
        void ReverseTrain();        // stops train, reverses, then accelerates
        void Stop();                // sends stop train command to marklin

        bool isMoving();
        int getSpeed();
    };

    void spawn_train(); // individual train tasks

};

#endif // _train_h