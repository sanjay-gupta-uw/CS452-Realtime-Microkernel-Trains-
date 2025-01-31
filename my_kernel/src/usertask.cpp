#include "usertask.h"
#include "rpi.h"
#include "shared_constants.h"
#include "kern/syscall.h"

#define RPLEN 6
#define MSGLEN 6

// ********** USER TASKS **********
// this runs with medium priority
void Task1()
{
   uart_printf(CONSOLE, "FirstUserTask running\r\n");
   // int nameserver = CREATE(MEDIUM, NameServer);
   // int rps_server = CREATE(MEDIUM, RPS);
   char reply[RPLEN];
   char msg[MSGLEN] = "Hello";

   // can test S/R and R/S using priority
   int test_send_id = CREATE(LOW, Task2);
   int myid = MYTID();

   uart_printf(CONSOLE, "FirstUserTask: TID: <%d>, TEST_SEND_ID: <%d>\r\n", myid, test_send_id);
   int ret_val = SEND(test_send_id, msg, MSGLEN, reply, RPLEN);
   uart_printf(CONSOLE, "FirstUserTask: SEND returned: <%d>\r\n", ret_val);

   uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
   EXIT();
}

void Task2()
{
   uart_printf(CONSOLE, "SecondUserTask running\r\n");
   int send_tid = -1;
   char msg[MSGLEN]; // test with differing lengths

   int ret_val = RECEIVE(&send_tid, msg, MSGLEN);
   uart_printf(CONSOLE, "UT2: RECEIVE returned: <%d>\r\n", ret_val);
   uart_printf(CONSOLE, "UT2: TID: <%d>, MSG: <%s>\r\n", send_tid, msg);

   EXIT();
}