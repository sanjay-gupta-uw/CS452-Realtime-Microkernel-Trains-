#ifndef _CONDUCTOR_H_
#define _CONDUCTOR_H_

#include "track_data.h"
#include "../../include/syscall.h"
namespace Conductor_NS
{

    class Conductor
    {
    public:
        Conductor();
        Conductor(char track_id);
        ~Conductor();

        Track track;
    };

    void start_conductor();
}

#endif // _CONDUCTOR_H_