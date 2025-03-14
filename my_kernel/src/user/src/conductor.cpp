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
        IO_NS::PrintTerminal("Starting Conductor\r\n");
        // Track track;
        REGISTERAS("Conductor");
        IO_NS::PrintTerminal("Conductor started\r\n");
    }
    Conductor::~Conductor()
    {
    }

    void start_conductor()
    {
        Conductor conductor;
        int sender_tid;
        unsigned char track_id;
        int retval = RECEIVE(&sender_tid, (char *)&track_id, sizeof(track_id));
        IO_NS::PrintTerminal("Conductor received request for track %c\r\n", track_id);
        uassert(track_id == 'A' || track_id == 'B' || track_id == 'a' || track_id == 'b');
        conductor.track.init(track_id);

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
