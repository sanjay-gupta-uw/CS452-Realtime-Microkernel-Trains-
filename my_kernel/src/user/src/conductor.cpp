#include "../include/conductor.h"
#include "../include/io.h"

typedef struct IO_REQUEST
{
    unsigned char ch;
};

namespace Conductor_NS
{

    Conductor::Conductor()
    {
        // Track track;
    }
    Conductor::Conductor(char track_id) : Conductor()
    {
        track.init(track_id);
    }
    Conductor::~Conductor()
    {
    }

    void start_conductor()
    {
        IO_NS::PrintTerminal("Conductor started\r\n");
        int sender_tid;
        IO_NS::PrintTerminal("Conductor waiting for request\r\n");
        IO_REQUEST req;
        int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
        IO_NS::PrintTerminal("Conductor received request\r\n");
        Conductor conductor(req.ch);
        REPLY(sender_tid, nullptr, 0);

        // test
        conductor.track.find_path("C7");
        EXIT();
    }
} // namespace Conductor_NS
