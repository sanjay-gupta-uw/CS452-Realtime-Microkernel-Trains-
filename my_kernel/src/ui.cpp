#include "ui.h"

#include "clock.h"
#include "command.h"
#include "marklin/switch.h"
// #include "sensor.h"
// #include "util.h"

UI::UI()
{
   clear_screen(CONSOLE);
   clock.Display((NUM_COLS - CLOCK_LENGTH) / 2);

   // Switches
   switches.InitDisplay(SWITCH_LOCATION);

   // Sensors
   // init_sensor_display(SENSOR_LOCATION);

   // uart_puts(CONSOLE, "Press 'q' to reboot\r\n");
}

// void update_ui(RingBuffer *rb)
void UI::Update()
{
   // move cursor to top middle for clock
   clock.Display((NUM_COLS - CLOCK_LENGTH) / 2);

   //    sensor_display(SENSOR_LOCATION + 3, rb);
   switches.Display(SWITCH_LOCATION + 5);
   cmd_prompt.Display(CMD_LOCATION);

   reset_formatting(CONSOLE);
   // move_cursor(CM)
}
