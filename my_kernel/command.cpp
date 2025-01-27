#include "command.h"
#include "ui.h"
#include "train.h"
#include "switch.h"

// ********** CommandPrompt Class **********
CommandPrompt::CommandPrompt()
{
   uart_printf(CONSOLE, "CommandPrompt Constructor\r\n");
   BUFFER_INDEX = 0;
   UPDATE_DISPLAY = true;
   input_buffer[0] = '\0';
   current_cmd = {INVALID_CMD, -1, -1};
}

void CommandPrompt::ResetBuffer()
{
   BUFFER_INDEX = 0;
   input_buffer[0] = '\0';
   UPDATE_DISPLAY = true;
}

CommandStatus CommandPrompt::ExtractCommand(Command *cmd)
{
   move_cursor(CONSOLE, CMD_LOCATION, CMD_PREFIX_LENGTH + BUFFER_INDEX);
   clear_to_end_line(CONSOLE);

   CommandStatus status = AWAITING_CMD;

   if (uart_available(CONSOLE))
   {
      char c = uart_getc(CONSOLE); // Read a single character

      if (c == '\r' || c == '\n') // Enter key
      {
         input_buffer[BUFFER_INDEX] = '\0'; // Null-terminate the string

         if (BUFFER_INDEX > 0)
         {
            CreateCommand(input_buffer); // Process the command
            status = (current_cmd.user_command != INVALID_CMD) ? SUCCESS_CMD : ERROR_CMD;
            *cmd = current_cmd;
         }

         // Reset the buffer
         ResetBuffer();
      }
      else if (c == '\b' || c == 127)
      {
         if (BUFFER_INDEX > 0)
         {
            input_buffer[BUFFER_INDEX] = '\0';
            move_cursor(CONSOLE, CMD_LOCATION, CMD_PREFIX_LENGTH + --BUFFER_INDEX);
            uart_putc(CONSOLE, ' ');
            move_cursor(CONSOLE, CMD_LOCATION, CMD_PREFIX_LENGTH + BUFFER_INDEX);
         }
      }
      else if (BUFFER_INDEX < INPUT_BUFFER_SIZE - 1) // Only add if there's space
      {
         // Add character to buffer and echo it to the console
         input_buffer[BUFFER_INDEX++] = c;
         uart_putc(CONSOLE, c);
      }
      else
      {
         uart_putc(CONSOLE, '\n');
         clear_to_end_line(CONSOLE);
         // buffeer index is {}, input buffer is {}
         uart_printf(CONSOLE, "BUFFER FULL: %d, %s  :", BUFFER_INDEX, input_buffer);
         uart_puts(CONSOLE, "Discarding input.\n");
         ResetBuffer();
      }
   }

   return status; // Return status if needed
}

void CommandPrompt::CreateCommand(const char *command)
{
   Command cmd = {INVALID_CMD, -1, -1};

   if (command[0] == 'q')
   {
      cmd.user_command = QUIT_CMD;
   }
   else if (command[0] == 't' && command[1] == 'r') // Handle train movement command
   {
      const char *p = command + 2; // Skip "tr"
      int train_num = 0, speed = 0;

      while (*p == ' ')
         p++; // Skip spaces
      while (*p >= '0' && *p <= '9')
      { // Parse train number
         train_num = train_num * 10 + (*p - '0');
         p++;
      }
      while (*p == ' ')
         p++; // Skip spaces
      while (*p >= '0' && *p <= '9')
      { // Parse speed
         speed = speed * 10 + (*p - '0');
         p++;
      }

      if (train_num > 0 && speed >= 0)
      {
         cmd.user_command = TRAIN_MOVE_CMD;
         cmd.addr = train_num;
         cmd.param = speed;
      }
   }
   else if (command[0] == 'r' && command[1] == 'v')
   {
      const char *p = command + 2; // Skip "rv"
      int train_num = 0;

      while (*p == ' ')
         p++; // Skip spaces
      while (*p >= '0' && *p <= '9')
      { // Parse train number
         train_num = train_num * 10 + (*p - '0');
         p++;
      }

      if (train_num > 0)
      {
         cmd.user_command = TRAIN_REV_CMD;
         cmd.addr = train_num;
      }
   }
   else if (command[0] == 's' && command[1] == 'w')
   {
      const char *p = command + 2; // Skip "sw"
      int switch_num = 0;
      char direction = '\0';

      while (*p == ' ')
         p++; // Skip spaces
      while (*p >= '0' && *p <= '9')
      { // Parse switch number
         switch_num = switch_num * 10 + (*p - '0');
         p++;
      }
      while (*p == ' ')
         p++;         // Skip spaces
      direction = *p; // Get direction

      if (switch_num > 0 && (direction == 'S' || direction == 's' || direction == 'C' || direction == 'c'))
      {
         cmd.user_command = SWITCH_CMD;
         cmd.addr = switch_num;
         cmd.param = (direction == 'S' || direction == 's') ? STRAIGHT : CURVED;
      }
   }

   current_cmd = cmd;
}

void CommandPrompt::Execute(Command *cmd)
{
   move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
   clear_to_end_line(CONSOLE);

   switch (cmd->user_command)
   {
   case TRAIN_MOVE_CMD:
      if (cmd->param <= 30 && cmd->param >= 0)
      {
         int speed = cmd->param;
         bool headlight = false;
         if (speed >= 16)
         {
            speed -= 16;
            headlight = true;
         }
         uart_printf(CONSOLE, "Accelerating train {%d} with data set to {%d}\r\n", cmd->addr, cmd->param);
         if (trains.Accelerate(cmd->addr, speed, headlight) < 0)
         {
            color_red();
            uart_printf(CONSOLE, "Error: Cannot Accelerate, Invalid Train Number {%d}\r\n", cmd->addr);
         }
      }
      else
      {
         color_red();
         uart_puts(CONSOLE, "Error: Invalid Move Train Command");
      }
      break;
   case TRAIN_REV_CMD:
      if (trains.Reverse(cmd->addr) < 0)
      {
         color_red();
         uart_printf(CONSOLE, "Error: Cannot Reverse, Invalid Train Number {%d}\r\n", cmd->addr);
      }
      break;
   case SWITCH_CMD:
      if (cmd->param == STRAIGHT || cmd->param == CURVED)
      {
         switches.SetSwitch(cmd->addr, (SWITCH_STATE)cmd->param);
      }
      else
      {
         color_red();
         uart_printf(CONSOLE, "INVALID SWITCH SYNTAX", cmd->addr, cmd->param);
      }
      break;
   case QUIT_CMD:
      // code
      uart_printf(CONSOLE, "QUITTING IN MAIN LOOP \r\n");
      break;
   default:
      break;
      color_red();
      uart_printf(CONSOLE, "Invalid command \r\n");
   }
}

void CommandPrompt::Display(int LOCATION)
{
   if (!UPDATE_DISPLAY)
   {
      return;
   }

   move_cursor(CONSOLE, LOCATION, 1);
   clear_to_end_line(CONSOLE);
   color_green();
   uart_puts(CONSOLE, "$: ");
   UPDATE_DISPLAY = false;
}
// ********** CommandPrompt Class End **********