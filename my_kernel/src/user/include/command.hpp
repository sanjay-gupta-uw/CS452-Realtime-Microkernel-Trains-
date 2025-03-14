#ifndef _command_h_
#define _command_h_

#include "io.h"
#include "io_server.h"
#include "marklin_structs.h"

namespace UI_CMD_NS
{

#define CMD_PREFIX_LENGTH 5
#define INPUT_BUFFER_SIZE 32

    class CommandPrompt
    {
        int BUFFER_INDEX;
        char inputBuffer[INPUT_BUFFER_SIZE];

        void clearInputBuffer();
        void Commandify(const char *str);
        void InitDisplay();

    public:
        CommandPrompt();
        void getInput();

        int IO_SERVER_TID;
        int CONDUCTOR_TID;
    };

    void start_command_prompt();

}

#endif