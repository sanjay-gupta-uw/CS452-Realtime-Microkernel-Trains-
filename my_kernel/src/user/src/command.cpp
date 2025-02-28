#include "../include/command.hpp"
#include "../include/name_server.h"
#include "../../kern/syscall.h"
#include "../../shared_constants.h"
#include "../../rpi.h"

namespace UI_CMD_NS
{
    static int IO_SERVER_TID;
    static int MARKLIN_CONTROLLER_TID;

    CommandPrompt::CommandPrompt()
    {
        IO_SERVER_TID = WHOIS("IOServer");
        MARKLIN_CONTROLLER_TID = WHOIS("MarklinController");

        BUFFER_INDEX = 0;
        clearInputBuffer();
    }

    static void Commandify(const char *str)
    {
        // clear status
        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE, CMD_LOCATION + 1, 1);
        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE, CMD_LOCATION + 2, 1);

        int command_received = -3;
        // if (str == nullptr)
        // {
        //     return;
        // }

        char first = str[0];
        if (first == 'q')
        {
            // quit command
            // UNIMPLEMENTED
            return;
        }

        char second = str[1];
        if ((first == 'S' || first == 's') &&
            (second == 'W' || second == 'w')) // switch command
        {
            int switch_num = 0;
            bool set_switch_num = false;
            char switch_state = ' ';

            const char *ptr = str + 2;
            while (*ptr == ' ')
            {
                ptr++;
            }
            // extract switch num
            while (*ptr >= '0' && *ptr <= '9')
            {
                switch_num = switch_num * 10 + (*ptr - '0');
                ptr++;
                set_switch_num = true;
            }

            while (*ptr == ' ')
            {
                ptr++;
            }

            if (*ptr == 'S' || *ptr == 's')
            {
                switch_state = 'S';
            }
            else if (*ptr == 'C' || *ptr == 'c')
            {
                switch_state = 'C';
            }

            if (!set_switch_num || switch_state == ' ')
            {
                IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Invalid Switch Command", CMD_LOCATION + 1, 1);
                return;
            }

            // create marklin request
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Switch %d set to %c", CMD_LOCATION + 1, 1, switch_num, switch_state);
            MarklinRequest request = {MARKLIN_REQUEST_TYPE::SET_SWITCH, switch_num, -1, &switch_state};
            SEND(MARKLIN_CONTROLLER_TID, (char *)&request, sizeof(MarklinRequest), nullptr, 0);
        }
        else if ((first == 'T' || first == 't') &&
                 (second == 'R' || second == 'r'))
        {
            // train command
            int train_num = 0;
            bool num_set = false;
            int train_speed = 0;
            bool speed_set = false;

            const char *ptr = str + 2;
            while (*ptr == ' ')
            {
                ptr++;
            }
            // extract train num
            while (*ptr >= '0' && *ptr <= '9')
            {
                train_num = train_num * 10 + (*ptr - '0');
                ptr++;
                num_set = true;
            }
            while (*ptr == ' ')
            {
                ptr++;
            }
            while (*ptr >= '0' && *ptr <= '9')
            {
                train_speed = train_speed * 10 + (*ptr - '0');
                ptr++;
                speed_set = true;
            }

            if (!num_set || !speed_set)
            {
                IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Invalid Train Command", CMD_LOCATION + 1, 1);
                return;
            }

            // create marklin request
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Train %d accelerated to %d", CMD_LOCATION + 1, 1, train_num, train_speed);
            MarklinRequest request = {MARKLIN_REQUEST_TYPE::ACCELERATE_TRAIN, train_num, -1, (char *)&train_speed};
            SEND(MARKLIN_CONTROLLER_TID, (char *)&request, sizeof(MarklinRequest), (char *)command_received, sizeof(int));
        }
        else if ((first == 'R' || first == 'r') &&
                 (second == 'V' || second == 'v'))
        {
            // train command
            int train_num = 0;
            bool num_set = false;

            const char *ptr = str + 2;
            while (*ptr == ' ')
            {
                ptr++;
            }
            // extract train num
            while (*ptr >= '0' && *ptr <= '9')
            {
                train_num = train_num * 10 + (*ptr - '0');
                ptr++;
                num_set = true;
            }

            if (!num_set)
            {
                IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Invalid Train Reverse Command", CMD_LOCATION + 1, 1);
                return;
            }

            // create marklin request
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Train %d reversed", CMD_LOCATION + 1, 1, train_num);
            MarklinRequest request = {MARKLIN_REQUEST_TYPE::REVERSE_TRAIN, train_num, -1, nullptr};
            SEND(MARKLIN_CONTROLLER_TID, (char *)&request, sizeof(MarklinRequest), (char *)command_received, sizeof(int));
        }
        else
        {
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "Invalid Command", CMD_LOCATION + 1, 1);
            // invalid command
            // UNIMPLEMENTED
        }
    }

    void CommandPrompt::clearInputBuffer()
    {
        for (int i = 0; i < INPUT_BUFFER_SIZE; i++)
        {
            inputBuffer[i] = '\0';
        }
        BUFFER_INDEX = 0;
        InitDisplay();
    }

    void CommandPrompt::InitDisplay()
    {
        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "cmd> ", CMD_LOCATION, 1);
    }

    void CommandPrompt::getInput()
    {

        // const int CONSOLE = 1;

        char c = IO_SERVER::Getc(IO_SERVER_TID);

        IO_NS::Print(MOVE_CURSOR, CMD_LOCATION, BUFFER_INDEX + CMD_PREFIX_LENGTH);
        switch (c)
        {
        case '\r':
        case '\n':
        {
            inputBuffer[BUFFER_INDEX] = '\0';
            if (BUFFER_INDEX > 0)
            {
                // PROCESS COMMAND
                Commandify(inputBuffer);
                clearInputBuffer();
                // spin_debug();
            }
        }
        break;
        case '\b':
        {
            if (BUFFER_INDEX > 0)
            {
                inputBuffer[BUFFER_INDEX] = '\0';
                IO_NS::Print(MOVE_CURSOR " ", CMD_LOCATION, BUFFER_INDEX-- + CMD_PREFIX_LENGTH);
            }
        }
        break;
        default:
        {
            if (BUFFER_INDEX < INPUT_BUFFER_SIZE - 1)
            {
                inputBuffer[BUFFER_INDEX] = c;
                BUFFER_INDEX++;
                IO_NS::Print(MOVE_CURSOR COLOR_GREEN "%c", CMD_LOCATION, BUFFER_INDEX + CMD_PREFIX_LENGTH, c);
            }
        }
        break;
        }
    }

    void start_command_prompt()
    {
        CommandPrompt commandPrompt;
        // uart_printf(CONSOLE, "Starting Command Prompt, location %d\r\n", CMD_LOCATION);

        commandPrompt.InitDisplay();
        while (true)
        {
            commandPrompt.getInput();
        }
    }
}