#ifndef _command_h_
#define _command_h_

#include "io.h"
#include "io_server.h"

namespace UI_CMD_NS
{

#define CMD_PREFIX_LENGTH 6
#define INPUT_BUFFER_SIZE 32

    class CommandPrompt
    {
        int IO_SERVER_TID;
        int BUFFER_INDEX;
        char inputBuffer[INPUT_BUFFER_SIZE];

        void clearInputBuffer();

    public:
        CommandPrompt();
        void InitDisplay(IO *io, int LOCATION_Y);
        void getInput(IO *io, int LOCATION_Y);
    };

    void start_command_prompt();

}

#endif