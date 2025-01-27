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
#include "kernel.h"
#include "syscall.h"
// #include "bwio.h"

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
   // Set up GPIO pins for both console and Marklin UARTs
   gpio_init();

   int delay = 100;
   clock.Delay(delay);
   // Not strictly necessary, since console is configured during boot
   uart_config_and_enable(CONSOLE);
   uart_config_and_enable(MARKLIN);
   Kernel kernel;

   // move_cursor(CONSOLE, 1, 1);

   // clock.Delay(time);

   size_t sp = fetch_sp();
   uart_printf(CONSOLE, "Welcome to the Train Controller !\n");
   uart_printf(CONSOLE, "WELSOME SP: %x\n", sp);

   uart_printf(CONSOLE, "Creating first task!\n");
   int taskID = kernel.Create(LOW, Task1);
   uart_printf(CONSOLE, "Task ID: %d\n", taskID);

   int EL = _get_el_debug();
   uart_printf(CONSOLE, "EL: %d\n", EL);
   // clock.Delay(delay);

   for (;;)
   {
      kernel.Scheduler();
      break;
   }
   for (;;)
   {
   }

   // UI ui;
   // RingBuffer<Command> command_buffer;
   // RingBuffer sensor_buffer;

   // sensor_init(ENABLE_RESET_MODE); // enables reset mode
   // switches.SetAll(CURVED);

   // main polling loop
   /*
   for (;;)
   {
      // update the clock
      clock.Update();
      ui.Update(); // update_ui(&sensor_buffer);

      Command tmp_cmd = {INVALID_CMD, -1, -1};
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
   */
   return 0;
}

// VBAR TABLE:
//