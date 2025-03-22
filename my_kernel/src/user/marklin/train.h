#ifndef _train_h
#define _train_h
#define MARKLIN 2
#include <stdbool.h>
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"
// #include "rpi.h"

// train query to conductor

namespace Trains_NS
{
#define LOW_SPEED 7
#define MEDIUM_SPEED 10
#define HIGH_SPEED 12

#define NUM_TRAINS 5
#define MAX_SPEED 14
#define MIN_SPEED 0
    class Train
    {
    private:
        int MARKLIN_IO_SERVER_TID;
        int CLOCK_SERVER_TID;
        int CONDUCTOR_TID;
        int SENSOR_SERVER_TID;
        int train_speed; // between 0 and 14
        bool isReversed;
        SensorStruct last_hit_sensor;

        void Reverse(); // sends reverse train command to marklin

        void TrainLoop();
        SensorStruct GetLocation();

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

    struct TrainParams
    {
        int train_num;
        int speed;
    };

    void
    spawn_train(); // individual train tasks

};

#endif // _train_h