#include "usertask.h"
#include "../rpi.h"
#include "../shared_constants.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "rps_server.h"
#include "rps_client.h"
#include <cstring>

// Task1 starts the servers and clients
void Task1() {
    uart_printf(CONSOLE, "FirstUserTask: Starting system services.\r\n");

    // Start the Name Server
    int ns_tid = CREATE(HIGH, NameServer);

    if (ns_tid < 0)
    {
        uart_printf(CONSOLE, "Failed to start Name Server. Exiting...\r\n");
        EXIT();
    }
    setNameServerTid(ns_tid);
    uart_printf(CONSOLE, "Name Server started with TID: %d\r\n", ns_tid);

    // Start the RPS Server
    int rps_server_tid = CREATE(HIGH, RpsServer);
    uart_printf(CONSOLE, "RPS Server started with TID: %d\r\n", rps_server_tid);

    // Delay to allow name registration to complete
    YIELD();
    YIELD();

    // Start RPS Clients
    int client_tid1 = CREATE(MEDIUM, RpsClient);
    int client_tid2 = CREATE(MEDIUM, RpsClient);
    uart_printf(CONSOLE, "RPS Clients started with TIDs: %d, %d\r\n", client_tid1, client_tid2);

    // Task1 now waits for some time then shuts down the system
    for (int i = 0; i < 20; ++i) {
        YIELD();  // Allow other tasks to run
    }

    // Shutdown message
    uart_printf(CONSOLE, "FirstUserTask: Shutting down system.\r\n");
    EXIT();  // End Task1
}