#ifndef _command_h_
#define _command_h_

#include "ui.h"
#include "train.h"
#include "switch.h"

#define CMD_PREFIX_LENGTH 4
#define INPUT_BUFFER_SIZE 32

// NOTE THAT FOR COMPILATION, MOVED COMMAND TYPES TO circbuff.h

typedef enum
{
   AWAITING_CMD,
   SUCCESS_CMD,
   ERROR_CMD,
} StatusCommand;

typedef enum
{
   TRAIN_MOVE_CMD,
   TRAIN_REV_CMD,
   SWITCH_CMD,
   QUIT_CMD,
   INVALID_CMD,
   STOP_CMD,
   CHANGE_SPEED_CMD,
   ACCURATE_STOP_CMD,
} UserCommand;

typedef struct
{
   UserCommand cmd_type;
   int addr;
   int param;
   int train_num;
   int start;
   int dest;
   int offset;
   int speed;
} CommandType;

void init_commands();
int handle_command(CommandType *active_cmd); 
void actual_stop(int pending_stop_train);
void actual_change_speed(int pending_change_speed_train, int pending_change_speed_speed);
void parse_command(CommandType *cmd);
void command_display(int LOCATION);

#endif