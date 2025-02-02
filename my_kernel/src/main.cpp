#include <stdbool.h>

#include "clock.h"
#include "command.h" // for command prompt
#include "dummy.h"
#include "rpi.h"
#include "ui.h"
#include "user/usertask.h"
#include "marklin/switch.h"
#include "marklin/train.h"

#include "containers/ringbuffer.h"

// #include "sensor.h"

// instantiate the template class
#include "kern/kernel.h"
#include "kern/memory.h"

typedef void (*funcvoid_t)();
extern funcvoid_t __init_array_start;
extern funcvoid_t __init_array_end;

// ENSURE THESE GET INITIALIZED
Clock clock;
Switches switches;
Trains trains;
CommandPrompt cmd_prompt;

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

extern "C" size_t fetch_sp();
extern "C" int _get_el_debug();
extern "C" void setup_mmu();

extern "C" int kmain()
{
#if defined(MMU)
   setup_mmu();
#endif

   call_global_constructors();
   // Set up GPIO pins for both console and Marklin UARTs
   gpio_init();

   int delay = 100;
   clock.Delay(delay);
   // Not strictly necessary, since console is configured during boot
   uart_config_and_enable(CONSOLE);
   uart_config_and_enable(MARKLIN);

   // size_t sp = fetch_sp();
   clear_screen(CONSOLE);
   uart_printf(CONSOLE, "Welcome to the Train Controller\r\n");

   // uart_printf(CONSOLE, "FUNCTION: 0x%x\r\n", function);
   uart_printf(CONSOLE, "TASK1: 0x%x\r\n", Task1);
   // uart_printf(CONSOLE, "TASK2: 0x%x\r\n", Task2);
   // uart_printf(CONSOLE, "Welcome to the Train Controller SP: %x\n", sp);

   // uart_printf(CONSOLE, "F1{0x%x}, F2{0x%x}\n", Task1, Task2);
   // Initialize kernel context
   Context kernel_context;
   Kernel kernel(Task1);

   // move_cursor(CONSOLE, 1, 1);

   // clock.Delay(time);

   // int taskID = kernel.Create(MEDIUM, Task1);
   // if (taskID < 0)
   // {
   //    uart_printf(CONSOLE, "ERROR CREATING FIRST USER TASK\r\n");
   // }
   // uart_printf(CONSOLE, "FIRST USER TASK CREATED {%d}(id)\n", taskID);

   // int EL = _get_el_debug();
   // uart_printf(CONSOLE, "EL: %d\n", EL);
   // clock.Delay(delay);

   TaskDescriptor *current_task = nullptr;

   // scheduler pops the highest priority task into td
   while ((current_task = kernel.Scheduler()))
   {
      int esr_el1 = kernel.DispatchTask(&kernel_context, current_task);
      // apply mask to ESR to get SVC number
      int N = esr_el1 & 0xFFFF;
      // uart_printf(CONSOLE, "ESR: {%d}, N: {%d}\n", esr_el1, N);
      if (esr_el1 < 0)
      {
         uart_printf(CONSOLE, "UNEXPECTED ERROR\r\n");
         for (;;)
         {
         }
      }
      kernel.Handler(N);
   }

   uart_printf(CONSOLE, "NO MORE READY TASKS\r\n");

   for (;;)
   {
      // uart_printf(CONSOLE, "Kernel Loop\n");
      // clock.Delay(1000);
   }

   return 0;
}
