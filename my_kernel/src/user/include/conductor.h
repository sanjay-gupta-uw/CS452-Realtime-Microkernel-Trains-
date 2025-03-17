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
        Sensors_NS::SensorManager sensors;
        Switch_NS::Switches switches;
        Trains_NS::Trains trains;

    public:
        Conductor();
        ~Conductor();

        void ProcessRequest(MarklinRequest *req);

        Track track;
    };

    void start_conductor();
    void spawn_train();
}

#endif // _CONDUCTOR_H_