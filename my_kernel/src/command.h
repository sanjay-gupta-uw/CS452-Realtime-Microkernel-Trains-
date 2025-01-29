#ifndef _command_h_
#define _command_h_

#define CMD_PREFIX_LENGTH 4
#define INPUT_BUFFER_SIZE 32

typedef enum
{
   AWAITING_CMD,
   SUCCESS_CMD,
   ERROR_CMD,
} CommandStatus;

typedef enum
{
   TRAIN_MOVE_CMD,
   TRAIN_REV_CMD,
   SWITCH_CMD,
   QUIT_CMD,
   INVALID_CMD,
} UserCommand;

typedef struct
{
   UserCommand user_command;
   int addr;
   int param;
} Command;

class Switches;
class Trains;

extern Switches switches;
extern Trains trains;

// can easily extend to store recent commands
class CommandPrompt
{
private:
   int BUFFER_INDEX;
   char input_buffer[INPUT_BUFFER_SIZE]; // Buffer for user input
   bool UPDATE_DISPLAY;
   Command current_cmd;

   void CreateCommand(const char *command);
   void ResetBuffer();

public:
   CommandPrompt();
   CommandStatus ExtractCommand(Command *cmd);
   void Execute(Command *cmd);
   void Display(int LOCATION);
};

#endif