#include "../include/command.h"
#include "../include/name_server.h"
#include "../../include/syscall.h"
#include "../../shared_constants.h"
#include "../../rpi.h"
#include "../include/uassert.h"
#include "../include/conductor.h"
#include "../include/graph_data.h"
#include <cstring>
#include <cctype>

extern "C" void _reboot(void); // Declare the reboot function implemented in assembly

namespace UI_CMD_NS
{
    void display_track(const TrackConfig *config)
    {
        int y = TRACK_LAYOUT_LOCATION_Y;
        int x = TRACK_LAYOUT_LOCATION_X;

        // Draw diagram
        for (size_t i = 0; i < config->diagram_lines; i++)
        {
            IO_NS::Print(MOVE_CURSOR "%s", y + i, x + 1, config->diagram[i]);
        }

        // Initialize switches
        for (size_t i = 0; i < config->switches_count; i++)
        {
            IO_NS::Print(MOVE_CURSOR COLOR_GREEN "S",
                         y + config->switches[i].line,
                         x + config->switches[i].col);
        }
    }

    static int getSwitchIndex(int switch_num)
    {
        if (switch_num > 0 && switch_num < 19)
        {
            return switch_num - 1;
        }
        int middle_switches[4] = {0x99, 0x9A, 0x9B, 0x9C};
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
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Train Spawn Command(offset must be positive): SPAWN <train_num> <sensor_id> <offset>", CMD_INFO_LOCATION + 4, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Train Accelerate Command: TR <train_num> <speed>", CMD_INFO_LOCATION + 2, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Train Reverse Command: RV <train_num>", CMD_INFO_LOCATION + 3, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Go Command(support speed 7-14): GO <train_num> <speed> <node_name> <offset>", CMD_INFO_LOCATION + 5, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Stop All Command: STOPALL", CMD_INFO_LOCATION + 6, 1);
        IO_NS::Print(MOVE_CURSOR COLOR_WHITE "Quit Command: q", CMD_INFO_LOCATION + 7, 1);
    }

    CommandPrompt::CommandPrompt()
    {
        IO_NS::PrintTerminal("Starting Command Prompt\r\n");
        REGISTERAS("CommandPrompt");
        IO_SERVER_TID = WHOIS("IOServer");
        CONDUCTOR_TID = WHOIS("Conductor");
        BUFFER_INDEX = 0;
        isTrackIDKnown = false;
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
            ConductorRequest request(COMMAND::SET_SWITCH, switch_num, switch_state);

            SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), nullptr, 0);
            // update switch display
            // need to get status of switch

            const char *color = (switch_state == 'S') ? COLOR_GREEN : COLOR_RED;
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
                char sensor_id[5] = {0};
                bool sensor_set = false;
                int offset = 0;
                bool offset_set = false;

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
                if (train_num < 0 || train_num > 80)
                {
                    IO_NS::PrintTerminal("Invalid Train Number\r\n");
                    return;
                }

                while (*ptr == ' ')
                {
                    ptr++;
                }
                // Parse sensor ID (up to 4 characters)
                int sidx = 0;
                while (*ptr != '\0' && *ptr != ' ' && sidx < 4)
                {
                    sensor_id[sidx++] = *ptr++;
                    sensor_set = true;
                }
                sensor_id[sidx] = '\0';

                bool is_valid_sensor_id = false;

                sensor_id[0] = std::toupper(sensor_id[0]);
                char sensor_bank = sensor_id[0];

                if (sensor_bank >= 'A' && sensor_bank <= 'E')
                {
                    char *sensor_num_ptr = sensor_id + 1;
                    uint32_t sensor_num = a2ui(&sensor_num_ptr, 10);
                    // IO_NS::PrintTerminal("Parsed sensor: bank=%c, number=%d (from '%s')\r\n",  sensor_bank, sensor_num, sensor_id);
                    if (sensor_num >= 1 && sensor_num <= 16)
                    {
                        is_valid_sensor_id = true;
                    }
                }

                while (*ptr == ' ')
                {
                    ptr++;
                }

                // Parse offset (optional)
                int sign = 1;
                if (*ptr == '-')
                {
                    sign = -1;
                    ptr++;
                }

                while (*ptr >= '0' && *ptr <= '9')
                {
                    offset = offset * 10 + (*ptr - '0');
                    ptr++;
                    offset_set = true;
                }

                offset *= sign;

                /*
                // extract offset num
                while (*ptr >= '0' && *ptr <= '9')
                {
                    start_offset = start_offset * 10 + (*ptr - '0');
                    ptr++;
                    offset_set = true;
                }

                if (!num_set || !sensor_set || !is_valid_sensor_id || !offset_set)
                {
                    IO_NS::PrintTerminal("Invalid Train SPAWN command\r\n");
                    // IO_NS::PrintTerminal("Input: %s\r\n", str);
                    // IO_NS::PrintTerminal("Train Number: %d, Sensor ID: %s\r\n", train_num, sensor_id);
                    return;
                }
                */
                if (!num_set || !sensor_set || !is_valid_sensor_id)
                {
                    IO_NS::PrintTerminal("Invalid Train SPAWN command\r\n");
                    // IO_NS::PrintTerminal("Input: %s\r\n", str);
                    // IO_NS::PrintTerminal("Train Number: %d, Sensor ID: %s\r\n", train_num, sensor_id);
                    return;
                }

                IO_NS::PrintTerminal("Attempting to spawn Train %d in front of sensor %s with offset %d\r\n", train_num, sensor_id, offset);

                // Create request with sensor ID
                ConductorRequest request(COMMAND::SPAWN_TRAIN, train_num, 0, sensor_id, nullptr, offset);
                request.data.cmdRequest.offset = offset;

                SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), (char *)command_received, sizeof(int));
            }
        }

        // Jack's code for parsing the GO command
        else if ((first == 'G' || first == 'g') &&
                 (second == 'O' || second == 'o'))
        {
            const char *ptr = str + 2;

            // Parse train num
            int train_num = 0;
            while (*ptr == ' ')
            {
                ptr++;
            }
            while (*ptr >= '0' && *ptr <= '9')
            {
                train_num = train_num * 10 + (*ptr - '0');
                ptr++;
            }

            // Parse speed num
            int speed = 0;
            while (*ptr == ' ')
            {
                ptr++;
            }
            while (*ptr >= '0' && *ptr <= '9')
            {
                speed = speed * 10 + (*ptr - '0');
                ptr++;
            }

            // capitalize the first letter
            char node_name[5] = {0};

            while (*ptr == ' ')
            {
                ptr++;
            }

            int sidx = 0;
            while (*ptr != '\0' && *ptr != ' ' && sidx < 4)
            {
                node_name[sidx++] = *ptr++;
            }
            node_name[sidx] = '\0';
            node_name[0] = std::toupper(node_name[0]);

            // Parse offset
            int offset = 0;
            while (*ptr == ' ')
            {
                ptr++;
            }
            int sign = 1;
            if (*ptr == '-')
            {
                sign = -1;
                ptr++;
            }
            while (*ptr >= '0' && *ptr <= '9')
            {
                offset = offset * 10 + (*ptr - '0');
                ptr++;
            }
            offset *= sign;

            ConductorRequest request(COMMAND::GOTO, train_num, speed, node_name, node_name, offset);
            // send node name to conductor
            IO_NS::PrintTerminal("Attempting to find path for Train %d to go to %s %d with speed %d, sending to Conductor tid: %d\r\n", train_num, node_name, offset, speed, CONDUCTOR_TID);
            SEND(CONDUCTOR_TID, (char *)&request, sizeof(request), nullptr, 0);
        }

        else if ((first == 'C' || first == 'c') &&
                 (second == 'A' || second == 'a'))
        {
            char third = str[2];
            if (third == 'L' || third == 'l')
            {
                int train_num = 0;
                bool num_set = false;

                const char *ptr = str + 3;
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
                    IO_NS::PrintTerminal("Invalid Train Calibrate Command\r\n");
                    return;
                }

                // create conductor request
                // IO_NS::PrintTerminal(CLEAR_SCREEN);
                IO_NS::PrintTerminal("Attempting to calibrate Train %d\r\n", train_num);
                ConductorRequest request(COMMAND::CALIBRATE, train_num, 0);
                int retval = SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), (char *)command_received, sizeof(int));
                uassert(retval >= 0 && "Error sending calibrate request to Conductor");
            }
        }
        else if ((first == 'S' || first == 's') &&
                 (second == 'T' || second == 't'))
        {
            // read next two characters
            char third = str[2];
            char fourth = str[3];
            char fifth = str[4];
            char sixth = str[5];
            char seventh = str[6];

            if ((third == 'O' || third == 'o') &&
                (fourth == 'P' || fourth == 'p') &&
                (fifth == 'A' || fifth == 'a') &&
                (sixth == 'L' || sixth == 'l') &&
                (seventh == 'L' || seventh == 'l'))
            {
                IO_NS::PrintTerminal("Attempting to stop all trains");
                ConductorRequest request(COMMAND::STOP_ALL, 0, 0);
                int retval = SEND(CONDUCTOR_TID, (char *)&request, sizeof(request), nullptr, 0);
                uassert(retval >= 0 && "Error sending stopall to Conductor");
            }
        }
        else if ((first == 'A' || first == 'a') &&
                 (second == 'U' || second == 'u'))
        {
            // read next two characters
            char third = str[2];
            char fourth = str[3];

            if ((third == 'T' || third == 't') &&
                (fourth == 'O' || fourth == 'o'))
            {
                IO_NS::PrintTerminal("Attempting to enable auto mode for all trains\r\n");
                ConductorRequest request(COMMAND::AUTO, 0, 0);
                SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), nullptr, 0);
            }
        }
        else if ((first == 'D' || first == 'd') &&
                 (second == 'E' || second == 'e'))
        {
            // read next two characters
            char third = str[2];
            char fourth = str[3];

            if ((third == 'M' || third == 'm') &&
                (fourth == 'O' || fourth == 'o'))
            {
                IO_NS::PrintTerminal("Attempting to perform a 3 minutes Demo\r\n");
                ConductorRequest request(COMMAND::DEMO, 0, 0);
                SEND(CONDUCTOR_TID, (char *)&request, sizeof(ConductorRequest), nullptr, 0);
            }
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
        if (!isTrackIDKnown)
        {
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "cmd> Please enter the Track ID: ", CMD_LOCATION, 1);
            return;
        }
        else
        {
            IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE COLOR_GREEN "cmd> ", CMD_LOCATION, 1);
        }
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
                IO_NS::PrintTerminal("Command: %s\r\n", inputBuffer);

                Commandify(inputBuffer);
                clearInputBuffer();
                // spin_debug();
            }
        }
        break;
        case '\b':
        case 127:
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
            uart_printf(CONSOLE, "Received Track ID: %c\r\n", track_id);
            if (track_id == 'A' || track_id == 'a' || track_id == 'B' || track_id == 'b')
            {
                // display the track graph
                if (track_id == 'A' || track_id == 'a')
                {
                    commandPrompt.current_track = &TRACK_A;
                }
                else
                {
                    commandPrompt.current_track = &TRACK_B;
                }
                display_track(commandPrompt.current_track);

                // create conductor
                IO_NS::PrintTerminal("Attempting to start Conductor with track ID: %c, CONDUCTOR_TID: %d\r\n", track_id, commandPrompt.CONDUCTOR_TID);
                SEND(commandPrompt.CONDUCTOR_TID, (char *)&track_id, sizeof(char), nullptr, 0);
                commandPrompt.isTrackIDKnown = true;
                commandPrompt.clearInputBuffer();

                // send message to conductor
                break;
            }
            else
            {
                IO_NS::PrintTerminal("Invalid Track ID. Please enter A or B (case ignored)\r\n");
            }
        }

        IO_NS::PrintTerminal("Command Prompt started\r\n");

        // track A
        //char *initial_commands_list[] = {
            //"SPAWN 77 B7 0",
            //"SPAWN 58 B11 0",
            //"SPAWN 55 B9 0",
            //"GO 77 7 A2 0",
            //"GO 55 8 A15 0",
            //"GO 58 8 A14 0",
            // "AUTO",
        //};
        // char *initial_commands_list[] = {
        //     "SPAWN 77 A16 0",
        //     "SPAWN 58 A13 0",
        //     "GO 77 10 A2 0",
        //     "GO 58 10 B13 0",
        //     // "AUTO",
        // };

        IO_NS::PrintTerminal("Parsing Initial commands:\r\n");
        /*for (int i = 0; i < sizeof(initial_commands_list) / sizeof(initial_commands_list[0]); i++)
        {
            IO_NS::PrintTerminal("%s\r\n", initial_commands_list[i]);
            IO_NS::PrintTerminal("Parsing command: %s -- press any key to proceed\r\n", initial_commands_list[i]);
            unsigned char ch = uart_getc(CONSOLE);
            commandPrompt.Commandify(initial_commands_list[i]);
        }*/
        // uassert(false && "FORCED ERROR -- this is for testing only");

        while (true)
        {
            commandPrompt.getInput();
        }
    }
}