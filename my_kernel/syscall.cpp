#include "syscall.h"
#include "rpi.h"
#include "kernel.h"
#include "shared_constants.h"

extern "C" size_t fetch_sp();

extern Kernel kernel;

extern "C" void _syscallHandler(int N, int PRIORITY, void (*function)())
{

   // Get the system call number
   uart_printf(CONSOLE, "PRIORITY: {%d}\n", PRIORITY);
   uart_printf(CONSOLE, "SyscallHandler: {%d}\n", N);
   int tid = -1;
   bool performERET = true;
   switch (N)
   {
   case SVC_CREATE:
      uart_printf(CONSOLE, "Creating Task with Priority: {%d}\n", PRIORITY);
      tid = kernel.Create(PRIORITY, function);
      break;

   case SVC_MYTID:
      uart_printf(CONSOLE, "Getting Task ID\n");
      tid = kernel.MyTid();
      break;

   case SVC_MYPARENTID:
      uart_printf(CONSOLE, "Getting Parent Task ID\n");
      tid = kernel.MyParentTid();
      break;

   case SVC_YIELD:
      uart_printf(CONSOLE, "Yielding Task\n");
      kernel.Yield();
      performERET = false;
      break;

   case SVC_EXIT:
      uart_printf(CONSOLE, "Exiting Task\n");
      kernel.Exit();
      performERET = false;
      break;

   default:
      break;
   }

   uart_printf(CONSOLE, "TID: {%d}\n", tid);
   if (!performERET)
   {
      asm volatile(
          "mov x0, %0\n"
          "ERET"
          :
          : "r"(tid)
          : "x0");
   }
}

extern "C" void _dummyHandler()
{
   // Get the system call number
   uart_printf(CONSOLE, "Dummy Handler\n");
   size_t sp = fetch_sp();
   uart_printf(CONSOLE, "SP: %x\n", sp);

   for (;;)
   {
   }
}