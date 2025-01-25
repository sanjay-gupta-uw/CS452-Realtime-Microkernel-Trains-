#include <stdbool.h>

#include "rpi.h"
#include "ui.h"
#include "clock.h"

#include "train.h"
#include "switch.h"
// #include "sensor.h"
#include "command.h"
#include "ringbuffer.h"

#include "task.h"

#define DEBUG 2
#define ENABLE_RESET_MODE true
#define DISABLE_RESET_MODE false

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

Clock clock;
Switches switches;
Trains trains;
CommandPrompt cmd_prompt;

extern "C" size_t fetch_sp();

extern "C" int kmain()
{
   call_global_constructors();

   RingBuffer<Command> command_buffer;
   // RingBuffer sensor_buffer;

   // Set up GPIO pins for both console and Marklin UARTs
   gpio_init();
   // Not strictly necessary, since console is configured during boot
   uart_config_and_enable(CONSOLE);
   uart_config_and_enable(MARKLIN);

   Command tmp_cmd = {INVALID_CMD, -1, -1};

   // sensor_init(ENABLE_RESET_MODE); // enables reset mode
   // switches.SetAll(CURVED);
   UI ui;

   size_t sp = fetch_sp();
   uart_printf(CONSOLE, "SP: %x\n", sp);

   for (;;)
   {
   }

   // main polling loop
   for (;;)
   {
      // update the clock
      clock.Update();
      ui.Update(); // update_ui(&sensor_buffer);

      int status = cmd_prompt.ExtractCommand(&tmp_cmd);

      if (status == SUCCESS_CMD)
      {
         // create function for this!
         if (!command_buffer.IsFull())
         {
            command_buffer.Push(&tmp_cmd);
         }
      }

      if (!command_buffer.IsEmpty())
      {
         Command cmd;
         int val = command_buffer.Pop(&cmd);
         if (val == 0)
         {
            cmd_prompt.Execute(&cmd);
            if (cmd.user_command == QUIT_CMD)
            {
               return 0;
            }
         }
      }
      else
      {
         // uint32_t time_start = get_current_time();

         // sensor_read_all(NUM_BANKS, &sensor_buffer); // keep this

         // uint32_t end_time = get_current_time();
         // move_cursor(CONSOLE, 70, 1);
         // clear_to_end_line(CONSOLE);
         // uart_printf(CONSOLE, "Latency: {%d} ", (end_time - time_start));
      }
      {
         // SOFTWARE EXCEPTION
         ContextSwitch();
      }
   }
}

// VBAR TABLE:
//