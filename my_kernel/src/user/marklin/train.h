#ifndef _train_h
#define _train_h
#define MARKLIN 2
#include <stdbool.h>
#include "../include/marklin_structs.h"
#include "../marklin/sensor.h"
#include "../../containers/stack.h"
// #include "rpi.h"

// train query to conductor

namespace Trains_NS
{
#define NUM_TRAINS 5
    // #define MAX_SPEED 14
    // #define MIN_SPEED 0

#define NUM_SUPPORTED_SPEED_LEVELS 3
#define LOW_SPEED 7
#define MEDIUM_SPEED 10
#define HIGH_SPEED 12

    // this will change based on the train speed
    class Train
    {
    private:
        int MARKLIN_IO_SERVER_TID;
        int CLOCK_SERVER_TID;
        int train_speed; // between 0 and 14
        bool isReversed;

        // USE THESE TO CALCULATE DISTANCE TRAVELLED AS FOLLOWS
        // first push is defined length (when we finish a segment we add the length of the segment)
        // second push represents the distance travelled in the segment
        Stack<int, 2> dist_travelled;
        Stack<int, 2> stopping_target; // distance to destination (not stopping distance)
        void initialize_distance_stacks();

        int segment_length;
        int prev_tick, cur_tick;
        int personal_train_messenger_tid;
        bool free_train_messenger = true;

        int stopping_distance[NUM_SUPPORTED_SPEED_LEVELS] = {-1, -1, -1};
        int approximate_speed[NUM_SUPPORTED_SPEED_LEVELS] = {-1, -1, -1};

        void process_train_command(TrainResponse *response);
        void update_position();
        void sensor_pos_update(int trigger_tick, int segment_length);

        void Reverse();   // sends reverse train command to marklin
        void TrainLoop(); // train loop task

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

    void spawn_train(); // individual train tasks
    void train_ticker();

    void train_messenger();

};

#endif // _train_h