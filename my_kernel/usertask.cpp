#include "usertask.h"
#include "rpi.h"
#include "shared_constants.h"
#include "syscall.h"

// ********** USER TASKS **********
void Task1()
{
   // int myid = MyTid();
   // uart_printf(CONSOLE, "THIS IS <TASK 1> MY ID: %d\n", myid);
   // uart_puts(CONSOLE, "THIS IS <TASK 1>\n");
   // uart_printf(CONSOLE, "CREATING HP TASK\n");
   // int tid = CREATE(HIGH, Task2);
   // uart_printf(CONSOLE, "CREATED HP TASK: %d\n", tid);

   for (int i = 0; i < 4; ++i)
   {
      int PRIORITY = i < 2 ? LOW : HIGH;
      int tid = CREATE(PRIORITY, Task2);
      uart_printf(CONSOLE, "Created: <%d>\r\n", tid);
   }
   uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
   EXIT();
}

void Task2()
{
   int myid = MYTID();
   int parentid = MYPARENTTID();
   uart_printf(CONSOLE, "TID: <%d>, PARENT-TID: <%d>\r\n", myid, parentid);
   YIELD();
   uart_printf(CONSOLE, "TID: <%d>, PARENT-TID: <%d>\r\n", myid, parentid);
   EXIT();
}