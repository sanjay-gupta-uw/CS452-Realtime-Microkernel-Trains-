#include "usertask.h"
#include "rpi.h"
#include "shared_constants.h"
#include "kern/syscall.h"
#include "user/name_server.h" 
#include "user/rps_server.h" 
#include "user/rps_client.h"

void Task1() {
    uart_printf(CONSOLE, "FirstUserTask: Starting RPS test...\r\n");

    int ns_tid = CREATE(HIGH, NameServer);
    uart_printf(CONSOLE, "FirstUserTask: Created NameServer TID = <%d>\r\n", ns_tid);

    int rps_tid = CREATE(MEDIUM, RPSServer);
    uart_printf(CONSOLE, "FirstUserTask: Created RPS Server TID = <%d>\r\n", rps_tid);

    int c1_tid = CREATE(LOW, RPSClient);
    int c2_tid = CREATE(LOW, RPSClient);
    uart_printf(CONSOLE, "FirstUserTask: Created RPS Clients c1 = <%d>, c2 = <%d>\r\n", c1_tid, c2_tid);

    uart_printf(CONSOLE, "FirstUserTask: Exiting...\r\n");
    EXIT();
}