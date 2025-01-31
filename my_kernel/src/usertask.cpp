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
   uart_printf(CONSOLE, "FirstUserTask: SEND returned: <%d>, REPLY: <%s>\r\n", ret_val, reply);

   uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
   EXIT();
}

void Task2()
{
   uart_printf(CONSOLE, "SecondUserTask running\r\n");
   int send_tid = -1;
   char msg[MSGLEN]; // test with differing lengths

   int ret_val = RECEIVE(&send_tid, msg, MSGLEN);
   uart_printf(CONSOLE, "UT2: BUF SIZE: <%d>, SENDER: <%d>, MSG: <%s>\r\n", ret_val, send_tid, msg);

   int reply_id = CREATE(HIGH, Task3);
   uart_printf(CONSOLE, "T2 CREATED HIGH PRIORITY TASK FOR REPLY: <%d>\r\n", reply_id);
   EXIT();
}

void Task3()
{
   uart_printf(CONSOLE, "ThirdUserTask running\r\n");
   int myid = MYTID();
   uart_printf(CONSOLE, "ThirdUserTask: TID: <%d>\r\n", myid);

   REPLY(0, "RPT3", 5);

   EXIT();
}