#include "usertask.h"
#include "usercall.h"
#include "rpi.h"
#include "shared_constants.h"

// ********** USER TASKS **********
void Task1()
{
   uart_puts(CONSOLE, "THIS IS <TASK 1>\n");
   for (int i = 0; i < 4; ++i)
   {
      int PRIORITY = i < 2 ? LOW : HIGH;
      CreateTask(PRIORITY, Task2);
   }
   uart_printf(CONSOLE, "FirstUserTask: exiting");
   Exit();
}

void Task2()
{
   int myid = MyTid();
   int parentid = MyParentTid();
   uart_printf(CONSOLE, "THIS IS <TASK 2> MY ID: %d, PARENT ID: %d\n", myid, parentid);
   Yield();
   uart_printf(CONSOLE, "THIS IS <TASK 2> MY ID: %d, PARENT ID: %d\n", myid, parentid);
   Exit();
}