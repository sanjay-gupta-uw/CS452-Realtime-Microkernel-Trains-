#include "ui.h"

#include <stdio.h>

void create_ui()
{
   init_commands(); // sets buffer index to 0
   clear_screen(CONSOLE);
   clock_display((NUM_COLS - CLOCK_LENGTH) / 2);

   // Switches
   init_switch_display(SWITCH_LOCATION);

   // Sensors
   init_sensor_display(SENSOR_LOCATION);

   // command prompt for user input
   command_display(CMD_LOCATION);

   // uart_puts(CONSOLE, "Press 'q' to reboot\r\n");
}

void update_ui(RingBuffer *rb)
{

   // move cursor to top middle for clock
   clock_display((NUM_COLS - CLOCK_LENGTH) / 2);
   sensor_display(SENSOR_LOCATION + 3, rb);
   switch_display(SWITCH_LOCATION + 5);
   command_display(CMD_LOCATION);

   reset_formatting(CONSOLE);
   // move_cursor(CM)
}
