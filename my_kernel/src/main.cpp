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

extern "C" void printASM(int x0)
{
   uart_printf(CONSOLE, "X0: 0x%x \n", x0);
}

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

   Context kernel_context; // Initialize kernel context

   // Kernel kernel(Task1);
   Kernel kernel(PerformanceTask); // bootstrap with performance task
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
