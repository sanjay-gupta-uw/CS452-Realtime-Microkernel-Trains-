#ifndef _command_prompt_h_
#define _command_prompt_h_

namespace UI
{
#define INPUT_BUFFER_SIZE 32

#define NUM_ROWS 24
#define NUM_COLS 80

#define IDLE_LOCATION 0
#define CLOCK_LOCATION 2
#define SWITCH_LOCATION 4

#define SENSOR_LOCATION 30
#define CMD_LOCATION 60
    // #define COMMAND_STATUS_LOCATION 61
    class CommandPrompt
    {
    private:
        int BUFFER_INDEX;
        char input_buffer[INPUT_BUFFER_SIZE]; // Buffer for user input
        bool UPDATE_DISPLAY;

        // void ResetBuffer();

    public:
        CommandPrompt();
        ~CommandPrompt();
        // CommandStatus ExtractCommand(Command *cmd);
        // void Execute(Command *cmd);
        void Display();
    };

    void start_command_prompt();
}
#endif // _command_prompt_h_