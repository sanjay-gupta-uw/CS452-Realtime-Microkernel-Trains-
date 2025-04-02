#ifndef _command_h_
#define _command_h_

#include "io.h"
#include "io_server.h"
#include "marklin_structs.h"
#include "graph_data.h"

namespace UI_CMD_NS
{

#define CMD_PREFIX_LENGTH 5
#define INPUT_BUFFER_SIZE 32

    class CommandPrompt
    {
        int BUFFER_INDEX;
        char inputBuffer[INPUT_BUFFER_SIZE];

        void InitDisplay();

    public:
        bool isTrackIDKnown;
        void Commandify(const char *str);

        CommandPrompt();
        void getInput();
        void clearInputBuffer();

        int IO_SERVER_TID;
        int CONDUCTOR_TID;
        const TrackConfig *current_track;
    };

    void start_command_prompt();

}

#endif