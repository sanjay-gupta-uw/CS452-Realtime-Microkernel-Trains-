#include "../include/command.hpp"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"
#include "../../rpi.h"

using namespace UI_CMD_NS;
CommandPrompt::CommandPrompt()
{
    IO_SERVER_TID = WHOIS("IOServer");

    BUFFER_INDEX = 0;
    clearInputBuffer();
}

void CommandPrompt::clearInputBuffer()
{
    for (int i = 0; i < INPUT_BUFFER_SIZE; i++)
    {
        inputBuffer[i] = '\0';
    }
    IO io;
    InitDisplay(&io, CMD_LOCATION);
}

void CommandPrompt::getInput(IO *io, int LOCATION_Y)
{

    // const int CONSOLE = 1;

    char c = IO_SERVER::Getc(IO_SERVER_TID);

    io->Print(MOVE_CURSOR, LOCATION_Y, BUFFER_INDEX + CMD_PREFIX_LENGTH);
    switch (c)
    {
    case '\r':
    case '\n':
    {
        inputBuffer[BUFFER_INDEX] = '\0';
        if (BUFFER_INDEX > 0)
        {
            // PROCESS COMMAND
            clearInputBuffer();
        }
    }
    break;
    case '\b':
    {
        if (BUFFER_INDEX > 0)
        {
            inputBuffer[BUFFER_INDEX] = '\0';
            io->Print(MOVE_CURSOR " ", LOCATION_Y, BUFFER_INDEX-- + CMD_PREFIX_LENGTH);
        }
    }
    break;
    default:
    {
        if (BUFFER_INDEX < INPUT_BUFFER_SIZE - 1)
        {
            inputBuffer[BUFFER_INDEX] = c;
            BUFFER_INDEX++;
            // io->Print(MOVE_CURSOR COLOR_GREEN "%s", LOCATION_Y, CMD_PREFIX_LENGTH, inputBuffer);
            io->Print(MOVE_CURSOR COLOR_GREEN "%c", LOCATION_Y, BUFFER_INDEX + CMD_PREFIX_LENGTH, c);
        }
    }
    break;
    }
}

void CommandPrompt::InitDisplay(IO *io, int LOCATION_Y)
{
    io->Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "cmd> ", LOCATION_Y, 1);
}

void UI_CMD_NS::start_command_prompt()
{
    CommandPrompt commandPrompt;
    IO io;
    // uart_printf(CONSOLE, "Starting Command Prompt, location %d\r\n", CMD_LOCATION);

    commandPrompt.InitDisplay(&io, CMD_LOCATION);
    while (true)
    {
        commandPrompt.getInput(&io, CMD_LOCATION);
    }
}