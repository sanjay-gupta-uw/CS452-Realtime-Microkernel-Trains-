#include "syscall.h"
#include "rpi.h"

extern "C" void _syscallHandler()
{
   // Get the system call number
   uart_printf(CONSOLE, "SyscallHandler\n");

   for (;;)
   {
   }
}