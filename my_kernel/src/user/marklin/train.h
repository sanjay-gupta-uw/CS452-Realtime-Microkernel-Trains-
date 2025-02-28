#ifndef _train_h
#define _train_h
#define MARKLIN 2
#include <stdbool.h>
#include "../include/marklin_structs.h"
// #include "rpi.h"

// forward declaration
namespace Trains_NS
{
#define MAX_SPEED 14
#define MIN_SPEED 0
    class Train
    {
    private:
        int train_speed; // between 0 and 14
        bool headlight_on;
        bool isReversed;

    public:
        int train_num;
        void HeadlightOn();
        void Accelerate(int speed);
        void Reverse();
        void Stop();
        bool isMoving();
        int getSpeed();

        Train();
        Train(int train_num);
        ~Train();
    };

    // Train trains[5];
    int getSpeed(int addr);
    void init_trains();
    bool isValidRequest(MarklinRequest *req);
};

#endif // _train_h