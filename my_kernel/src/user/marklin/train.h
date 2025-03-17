#ifndef _train_h
#define _train_h
#define MARKLIN 2
#include <stdbool.h>
#include "../include/marklin_structs.h"
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
struct TrainQuery
{
    char *name;          // sensor node name
    DIRECTION direction; // direction of the train
};

struct TrainResponse
{
    char *name;            // next expected sensor node name
    TRAIN_COMMAND command; // command to be executed, if any
    int speed;             // assume stop is only issued when next sensor is the destination
};

namespace Trains_NS
{
#define NUM_TRAINS 5
#define MAX_SPEED 14
#define MIN_SPEED 0

    class Trains;
    class Train
    {
    private:
        int train_num;
        int train_speed; // between 0 and 14
        bool headlight_on;
        bool isReversed;
        void HeadlightOn();

    public:
        Train();
        Train(int train_num);
        ~Train();

        void Accelerate(int speed);
        void Reverse();
        void Stop();
        bool isMoving();
        int getSpeed();

        friend class Trains;
    };

    class Trains
    {
    private:
        Train trains[NUM_TRAINS];
        void init_trains();
        Train *isTrainValid(int train_num);
        bool isSpeedValid(int speed);

    public:
        Trains();
        ~Trains();
        bool AccelerateTrain(int train_num, int speed);
        bool ReverseTrain(int train_num);
        void setIOServerTid(int tid);
    };

};

#endif // _train_h