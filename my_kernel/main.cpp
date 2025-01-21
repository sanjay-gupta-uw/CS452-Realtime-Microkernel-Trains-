#include <stdbool.h>

#include "TestClass.h"
#include "rpi.h"
#include "ui.h"
#include "clock.h"
// #include "train.h"
// #include "switch.h"
// #include "sensor.h"
// #include "command_buffer.h"
// #include "command.h"

#define DEBUG 2
#define ENABLE_RESET_MODE true
#define DISABLE_RESET_MODE false
#define BRANCH false
#define STRAIGHT true

Clock clock;

typedef void (*funcvoid_t)();
extern funcvoid_t __init_array_start;
extern funcvoid_t __init_array_end;

void call_global_constructors()
{
   for (funcvoid_t *ctr = &__init_array_start; ctr < &__init_array_end; ++ctr)
   {
      (*ctr)();
   }
}

extern "C" int __cxa_atexit(void (*)(void *), void *, void *)
{
   return 0; // Do nothing
}

void *__dso_handle = 0;

extern "C" int kmain()
{
   call_global_constructors();

   // TestClass test1;
   // TestClass test2;
   // test1.printMessage(); // this is throwing the error
   // test2.printMessage(); // this is throwing the error

   // CommandRingBuffer command_buffer;
   // RingBuffer sensor_buffer;

   // init_command_ring_buffer(&command_buffer);
   // init_ring_buffer(&sensor_buffer);

   // Set up GPIO pins for both console and Marklin UARTs
   gpio_init();
   // Not strictly necessary, since console is configured during boot
   uart_config_and_enable(CONSOLE);
   uart_config_and_enable(MARKLIN);

   // CommandType tmp_cmd = {INVALID_CMD, -1, -1};

   // sensor_init(ENABLE_RESET_MODE); // enables reset mode
   // set_all_switches(BRANCH);
   // // Clock init
   // clock_init();
   // create_ui();
   UI ui;

   // main polling loop
   for (;;)
   {
      // update the clock
      clock.Update();
      ui.Update(); // update_ui(&sensor_buffer);

      /*

      int status = handle_command(&tmp_cmd);
      if (status == SUCCESS_CMD)
      {
         // create function for this!
         if (!is_command_buffer_full(&command_buffer))
         {
            add_command_to_buffer(&command_buffer, &tmp_cmd);
         }
      }

      if (!is_command_buffer_empty(&command_buffer)) // THIS LINE
      {
         CommandType cmd;
         int val = remove_command_from_buffer(&command_buffer, &cmd);
         if (cmd.cmd_type == QUIT_CMD)
         {
            return 0;
         }

         if (val)
         {
            parse_command(&cmd);
         }
      }
      else
      {
         // uint32_t time_start = get_current_time();

         sensor_read_all(NUM_BANKS, &sensor_buffer); // keep this

         // uint32_t end_time = get_current_time();
         // move_cursor(CONSOLE, 70, 1);
         // clear_to_end_line(CONSOLE);
         // uart_printf(CONSOLE, "Latency: {%d} ", (end_time - time_start));
      }
      */
   }
}
