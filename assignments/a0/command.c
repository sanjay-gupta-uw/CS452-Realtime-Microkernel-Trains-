#include "command.h"
#include "rpi.h"
#include "switch.h"
#include "track_data.h"
#include <string.h>

#define DEBUG 70
static int BUFFER_INDEX;
static char input_buffer[INPUT_BUFFER_SIZE]; // Buffer for user input
static bool UPDATE_DISPLAY = true;
static int COMMAND_STATUS_LOCATION;

void init_commands()
{
   BUFFER_INDEX = 0;
}

static CommandType process_command(const char *command)
{
   CommandType cmd = {INVALID_CMD, -1, -1};

   if (command[0] == 'q')
   {
      cmd.cmd_type = QUIT_CMD;
   }
   // need to ensure train #'s/speed are valid
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
         cmd.cmd_type = TRAIN_MOVE_CMD;
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
         cmd.cmd_type = TRAIN_REV_CMD;
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
         cmd.cmd_type = SWITCH_CMD;
         cmd.addr = switch_num;
         cmd.param = (direction == 'S' || direction == 's') ? STRAIGHT : CURVED;
      }
   }
   else if (strncmp(command, "stop", 4) == 0) {
      const char *p = command + 4;
      int train_num = 0;
      while (*p == ' ') p++;
      while (*p >= '0' && *p <= '9') {
         train_num = train_num * 10 + (*p - '0');
         p++;
      }
      if (train_num > 0) {
         cmd.cmd_type = STOP_CMD;
         cmd.addr = train_num;
      }
   }
   else if (strncmp(command, "change", 6) == 0) {
      const char *p = command + 6; // Skip "change"
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
         cmd.cmd_type = CHANGE_SPEED_CMD;
         cmd.addr = train_num;
         cmd.param = speed;
      }
   }
   else if (strncmp(command, "acstop", 6) == 0) {
      const char *p = command + 6;
      int train_num = 0, offset = 0, start = 0, dest = 0, speed = 0;
      char start_str[10] = {0}, dest_str[10] = {0}, speed_str[10] = {0};

      // Parse train number
      while (*p == ' ') p++;
      while (*p >= '0' && *p <= '9') {
         train_num = train_num * 10 + (*p - '0');
         p++;
      }

      // Parse start node
      while (*p == ' ') p++;
      int i = 0;
      while (*p != ' ' && *p != '\0' && i < 9) {
         start_str[i++] = *p++;
      }
      start_str[i] = '\0';

      start = get_node_num_by_name_b(start_str);


      // Parse destination node
      while (*p == ' ') p++;
      i = 0;
      while (*p != ' ' && *p != '\0' && i < 9) {
         dest_str[i++] = *p++;
      }
      dest_str[i] = '\0';

      dest = get_node_num_by_name_b(dest_str);

      // Parse offset
      while (*p == ' ') p++;
      while (*p >= '0' && *p <= '9') {
          offset = offset * 10 + (*p - '0');
          p++;
      }

      // Parse speed
      while (*p == ' ') p++;
      i = 0;
      while (*p != ' ' && *p != '\0' && i < 9) {
          speed_str[i++] = *p++;
      }
      speed_str[i] = '\0';

      // Convert speed string to value
      if (strcmp(speed_str, "low") == 0) speed = 6;
      else if (strcmp(speed_str, "mid") == 0) speed = 9;
      else if (strcmp(speed_str, "high") == 0) speed = 12;
      else {
          cmd.cmd_type = INVALID_CMD;
          return cmd;
      }

      cmd.cmd_type = ACCURATE_STOP_CMD;
      cmd.train_num = train_num;
      cmd.start = start;
      cmd.dest = dest;
      cmd.offset = offset;
      cmd.speed = speed;
   }

   return cmd;
}

int handle_command(CommandType *active_cmd)
{
   move_cursor(CONSOLE, CMD_LOCATION, CMD_PREFIX_LENGTH + BUFFER_INDEX);
   clear_to_end_line(CONSOLE);

   int status = AWAITING_CMD;

   if (uart_available(CONSOLE))
   {
      char c = uart_getc(CONSOLE); // Read a single character

      if (c == '\r' || c == '\n') // Enter key
      {
         input_buffer[BUFFER_INDEX] = '\0'; // Null-terminate the string

         if (BUFFER_INDEX > 0)
         {
            CommandType cmd = process_command(input_buffer); // Process the command
            *active_cmd = cmd;

            int cmd_status = active_cmd->cmd_type;
            status = (cmd_status != INVALID_CMD) ? SUCCESS_CMD : ERROR_CMD;
         }

         // Reset the buffer
         BUFFER_INDEX = 0;
         input_buffer[0] = '\0';
         UPDATE_DISPLAY = true;
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
         // Handle overflow gracefully (optional)
         // move_cursor(CONSOLE, CMD_DEBUG_LOCATION, 1);
         // clear_to_end_line(CONSOLE);
         // uart_puts(CONSOLE, "Input buffer full. Discarding input.\n");
      }

      // Debugging output
      // move_cursor(CMD_DEBUG_LOCATION + 1, 1);
      // clear_to_end_line(CONSOLE);
      // uart_printf(CONSOLE, "Buffer index: %d \r\n", BUFFER_INDEX);
      // clear_to_end_line(CONSOLE);
      // input_buffer[BUFFER_INDEX] = '\0';
      // uart_printf(CONSOLE, "CMD: %s", input_buffer);
   }

   return status; // Return status if needed
}

static void color_red()
{
   uart_puts(CONSOLE, "\033[31m"); // red
}

void actual_stop(int pending_stop_train)

{
   move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
   uart_puts(CONSOLE, "\033[32m");
   clear_to_end_line(CONSOLE);
   stop_train(pending_stop_train);
	uart_printf(CONSOLE, "Train %d stopped by sensor.\r\n", pending_stop_train);
}

void actual_change_speed(int pending_change_speed_train, int pending_change_speed_speed)

{
   move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
   uart_puts(CONSOLE, "\033[32m");
   clear_to_end_line(CONSOLE);
   int idx = is_valid_train(pending_change_speed_train);
   accelerate_train(pending_change_speed_train, pending_change_speed_speed, false, idx);
	uart_printf(CONSOLE, "Changing train %d's spped to to %d by sensor.\r\n", pending_change_speed_train, pending_change_speed_speed);
         
}

void parse_command(CommandType *cmd)

{
   move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
   uart_puts(CONSOLE, "\033[32m");
   clear_to_end_line(CONSOLE);
   int idx;
   switch (cmd->cmd_type)
   {
   case TRAIN_MOVE_CMD:
      idx = is_valid_train(cmd->addr);
      if (idx > 0 && cmd->param <= 30 && cmd->param >= 0)
      {
         uart_printf(CONSOLE, "Accelerating train {%d} with data set to {%d}\r\n", cmd->addr, cmd->param);
         accelerate_train(cmd->addr, cmd->param, false, idx);
      }
      else
      {
         color_red();
         uart_printf(CONSOLE, "INVALID TRAIN SYNTAX", cmd->addr, cmd->param);
      }
      break;
   case TRAIN_REV_CMD:
      idx = is_valid_train(cmd->addr);
      if (idx == -1)
      {
         color_red();
         uart_printf(CONSOLE, "INVALID TRAIN REV SYNTAX", cmd->addr, cmd->param);
         break;
      }
      // check if moving
      if (is_moving(idx))
      {
         uart_printf(CONSOLE, "Stopping train {%d} for 4 seconds \r\n", cmd->addr);
         stop_train(cmd->addr);
         clock_delay(4000);
      }
      move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
      clear_to_end_line(CONSOLE);
      uart_printf(CONSOLE, "Reversing train {%d}\r\n", cmd->addr);
      reverse_train(cmd->addr);
      accelerate_train(cmd->addr, -1, true, idx);
      break;
   case SWITCH_CMD:
      if (is_valid_switch(cmd->addr))
      {
         if (cmd->param == STRAIGHT)
         {
            uart_printf(CONSOLE, "Setting switch {%d} to STRAIGHT \r\n", cmd->addr);
            switch_straight(cmd->addr);
            break;
         }
         else if (cmd->param == CURVED)
         {
            uart_printf(CONSOLE, "Setting switch {%d} to CURVED \r\n", cmd->addr);
            switch_branch(cmd->addr);
            break;
         }
      }
      color_red();
      uart_printf(CONSOLE, "INVALID SWITCH SYNTAX", cmd->addr, cmd->param);
      break;
   case QUIT_CMD:
      uart_printf(CONSOLE, "QUIT not implemented yet \r\n");
      break;
   case STOP_CMD:
      uart_printf(CONSOLE, "Train %d will stop at next sensor.\r\n", cmd->addr);
      break;
   case CHANGE_SPEED_CMD:
      uart_printf(CONSOLE, "Train %d will change spped to %d at next sensor.\r\n", cmd->addr, cmd->param);
      break;
   case ACCURATE_STOP_CMD:
      uart_printf(CONSOLE, "Finding Path.\r\n");
      break;
   case INVALID_CMD:
   default:
      break;
      uart_puts(CONSOLE, "\033[31m"); // red
      uart_printf(CONSOLE, "Invalid command \r\n");
   }
}

void command_display(int LOCATION)
{
   COMMAND_STATUS_LOCATION = LOCATION + 1;
   if (!UPDATE_DISPLAY)
   {
      return;
   }

   move_cursor(CONSOLE, LOCATION, 1);
   clear_to_end_line(CONSOLE);
   uart_puts(CONSOLE, "\033[32m"); // set colour to green
   uart_puts(CONSOLE, "$: ");
   UPDATE_DISPLAY = false;
}