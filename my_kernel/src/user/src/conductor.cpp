#include "../include/conductor.h"
#include "../include/io.h"
#include "../include/uassert.h"
#include "../include/name_server.h"

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
        REGISTERAS("Conductor");
        IO_NS::PrintTerminal("Conductor started\r\n");
        int sender_tid;
        IO_NS::PrintTerminal("Conductor waiting for request\r\n");
        IO_REQUEST req;
        int retval = RECEIVE(&sender_tid, (char *)&req, sizeof(req));
        IO_NS::PrintTerminal("Conductor received request\r\n");
        Conductor conductor(req.ch);
        REPLY(sender_tid, nullptr, 0);
        // test
        FindPathRequest find_node_name;
        while (true)
        {
            // receive request from terminal
            retval = RECEIVE(&sender_tid, (char *)&find_node_name, sizeof(find_node_name));
            IO_NS::PrintTerminal("Conductor received request for %s\r\n", find_node_name);
            // find path to node
            conductor.track.find_path((char *)find_node_name.node_name);
            REPLY(sender_tid, nullptr, 0);
        }
        EXIT();
    }
} // namespace Conductor_NS
