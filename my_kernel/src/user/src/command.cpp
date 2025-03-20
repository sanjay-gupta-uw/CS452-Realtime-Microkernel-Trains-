#include "../include/command.hpp"
#include "../include/name_server.h"
#include "../../include/syscall.h"
#include "../../shared_constants.h"
#include "../../rpi.h"
#include "../include/uassert.h"
#include "../include/conductor.h"

extern "C" void _reboot(void); // Declare the reboot function implemented in assembly

namespace UI_CMD_NS
{

    static int getSwitchIndex(int switch_num)
    {
        if (switch_num > 0 && switch_num < 19)
        {
            return switch_num - 1;
        }
        int middle_switches[4] = {0x9A, 0x9B, 0x9C, 0x99};
        for (int i = 0; i < 4; i++)
        {
            if (switch_num == middle_switches[i])
            {
                return 18 + i;
            }
        }
        return -1;
    }

    static void PrintCommandHelp()
    {
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Command List:", CMD_INFO_LOCATION, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Switch Command: SW <switch_num> <S/C>", CMD_INFO_LOCATION + 1, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Train Accelerate Command: TR <train_num> <speed>", CMD_INFO_LOCATION + 2, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Train Reverse Command: RV <train_num>", CMD_INFO_LOCATION + 3, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Train Spawn Command: SPAWN <train_num>", CMD_INFO_LOCATION + 4, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Go Command: GO <node_name>", CMD_INFO_LOCATION + 5, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Quit Command: q", CMD_INFO_LOCATION + 6, 1);
    }

    CommandPrompt::CommandPrompt()
    {
        IO_NS::PrintTerminal("Starting Command Prompt\r\n");
        REGISTERAS("CommandPrompt");
        IO_SERVER_TID = WHOIS("IOServer");
        CONDUCTOR_TID = WHOIS("Conductor");
        BUFFER_INDEX = 0;
        clearInputBuffer();
        InitDisplay();
        PrintCommandHelp();
    }

    void CommandPrompt::Commandify(const char *str)
    {
        int command_received = -3;
        // if (str == nullptr)
        // {
        //     return;
        // }

        char first = str[0];
        if (first == 'q')
        {
            // quit command
            // IO_NS::PrintTerminal(RESET_FORMATTING CLEAR_SCREEN "Quitting...\r\n");
            _reboot();
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
            else
            {
                switch_state = ' ';
            }
            int switch_index = -1;
            if (set_switch_num)
            {
                // check if switch num is valid
                switch_index = getSwitchIndex(switch_num);
            }

            if (!set_switch_num || switch_state == ' ' || switch_index == -1)
            {
                IO_NS::PrintTerminal("Invalid Switch Command\r\n");
                return;
            }

            // create marklin request
            IO_NS::PrintTerminal("Attempting to set Switch %d to %c\r\n", switch_num, switch_state);
            ConductorRequest request(COMMAND::SET_SWITCH, switch_index, switch_state);
            SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), nullptr, 0);
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

            if (!num_set || !speed_set || train_speed > 15 || train_speed < 0)
            {
                IO_NS::PrintTerminal("Invalid Train Command\r\n");
                return;
            }

            // create marklin request
            IO_NS::PrintTerminal("Attempting to accelerate Train %d to %d\r\n", train_num, train_speed);
            ConductorRequest request(COMMAND::ACCELERATE_TRAIN, train_num, train_speed);
            SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), (char *)command_received, sizeof(int));
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
                IO_NS::PrintTerminal("Invalid Train Reverse Command\r\n");
                return;
            }

            // create marklin request
            IO_NS::PrintTerminal("Attempting to reverse Train %d\r\n", train_num);
            ConductorRequest request(COMMAND::REVERSE_TRAIN, train_num, 0);
            SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), (char *)command_received, sizeof(int));
        }
        else if ((first == 'S' || first == 's') &&
                 (second == 'P' || second == 'p'))
        {
            // read next two characters
            char third = str[2];
            char fourth = str[3];
            char fifth = str[4];

            if ((third == 'A' || third == 'a') &&
                (fourth == 'W' || fourth == 'w') &&
                (fifth == 'N' || fifth == 'n'))
            {
                // read the trian being spawned
                int train_num = 0;
                bool num_set = false;
                const char *ptr = str + 5;
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

                // try to spawn train
                if (!num_set || train_num < 0 || train_num > 80)
                {
                    IO_NS::PrintTerminal("Invalid Train Spawn Command\r\n");
                    return;
                }
                IO_NS::PrintTerminal("Attempting to spawn Train %d\r\n", train_num);
                ConductorRequest request(COMMAND::SPAWN_TRAIN, train_num, 0);
                SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), (char *)command_received, sizeof(int));
            }
        }

        else if ((first == 'G' || first == 'g') &&
                 (second == 'O' || second == 'o'))
        {
            // ignore spaces
            const char *ptr = str + 2;
            while (*ptr == ' ')
            {
                ptr++;
            }

            // extract train num

            // capitalize the first letter
            char node_name[5];
            for (int i = 0; i < 4; i++)
            {
                node_name[i] = *ptr;
                ptr++;
            }
            node_name[4] = '\0';
            if (node_name[0] >= 'a' && node_name[0] <= 'z')
            {
                node_name[0] = node_name[0] - 32;
            }
            ConductorRequest request(COMMAND::GOTO, 0, 0, node_name, node_name);
            // send node name to conductor
            IO_NS::PrintTerminal("Attempting to find path to %s, sending to Conductor tid: %d\r\n", node_name, CONDUCTOR_TID);
            SEND(CONDUCTOR_TID, (char *)&request, sizeof(request), nullptr, 0);
        }

        else
        {
            IO_NS::PrintTerminal("Invalid Command\r\n");
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
        // IO_NS::PrintTerminal("GETTING INPUT\r\n");
        // const int CONSOLE = 1;

        unsigned char c = (unsigned char)(IO_SERVER::Getc(IO_SERVER_TID));
        // IO_NS::PrintTerminal("GOT INPUT: %c\r\n", c);

        // IO_NS::Print(MOVE_CURSOR, CMD_LOCATION, BUFFER_INDEX + CMD_PREFIX_LENGTH);
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

        IO_NS::PrintTerminal("Please enter the Track ID: ");
        while (true)
        {
            unsigned char track_id = (unsigned char)(IO_SERVER::Getc(commandPrompt.IO_SERVER_TID));
            IO_NS::PrintTerminal("%c\r\n", track_id);
            if (track_id == 'A' || track_id == 'a' || track_id == 'B' || track_id == 'b')
            {
                // create conductor
                IO_NS::PrintTerminal("Attempting to start Conductor with track ID: %c, CONDUCTOR_TID: %d\r\n", track_id, commandPrompt.CONDUCTOR_TID);
                SEND(commandPrompt.CONDUCTOR_TID, (char *)&track_id, sizeof(char), nullptr, 0);
                // send message to conductor
                break;
            }
            else
            {
                IO_NS::PrintTerminal("Invalid Track ID. Please enter A or B (case ignored)\r\n");
            }
        }

        IO_NS::PrintTerminal("Command Prompt started\r\n");

        while (true)
        {
            commandPrompt.getInput();
        }
    }
}