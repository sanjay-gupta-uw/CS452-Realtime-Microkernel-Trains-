/*
   ___                       _                     _
  / __|    ___    _ _     __| |   _  _     __     | |_     ___      _ _
 | (__    / _ \  | ' \   / _` |  | +| |   / _|    |  _|   / _ \    | '_|
  \___|   \___/  |_||_|  \__,_|   \_,_|   \__|_   _\__|   \___/   _|_|_
_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|
"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'
*/

#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../marklin/sensor.h"
#include "../marklin/switch.h"
#include "../marklin/train.h"
#include "../../include/syscall.h"

typedef struct FindPathRequest
{
    unsigned char *node_name;
};

namespace Conductor_NS
{
    class Conductor
    {
    private:
        // Sensors_NS::SensorManager sensors[NUM_BANKS * SENSORS_PER_BANK];
        Switch_NS::Switch switches[SWITCH_COUNT];
        Trains_NS::Train trains[NUM_TRAINS];

    public:
        Conductor();
        Conductor(char track_id);
        ~Conductor();
        Track track;
    };

    void start_conductor();
}

#endif // _CONDUCTOR_H_