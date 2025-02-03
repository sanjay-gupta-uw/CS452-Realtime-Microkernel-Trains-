#include "usertask.h"
#include "../rpi.h"
#include "../shared_constants.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "rps_server.h"
#include "rps_client.h"
#include <cstring>

// Task function prototypes
void TaskRegister();
void TaskQuery();
void TaskStressTest();

// Main task to start system services and tests
void Task1()
{
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

    // test Name Server registration
    int TR_TID = CREATE(MEDIUM, TaskRegister);

    // test Name Server queries
    int TQ_TID = CREATE(LOW, TaskQuery);

    uart_printf(CONSOLE, "FUT Created TaskRegister {%d} and TaskQuery {%d}\r\n", TR_TID, TQ_TID);

    // test the Name Server
    // CREATE(LOW, TaskStressTest);

    uart_printf(CONSOLE, "FirstUserTask: exiting.\r\n");
    EXIT(); // End Task1
}

// Task to register names with the Name Server
void TaskRegister()
{
    int my_tid = MYTID();

    const char *names[] = {"ServiceA", "ServiceB", "ServiceC", "ServiceD"};

    for (int i = 0; i < 4; i++)
    {
        uart_printf(CONSOLE, "Attempting to regiter self (%d) as '%s'\r\n", my_tid, names[i]);
        int ret = REGISTERAS(names[i]);
        uart_printf(CONSOLE, "Registration for '%s' returned: %d\r\n", names[i], ret);
    }

    EXIT();
}

// Task to query registered names from the Name Server
void TaskQuery()
{
    const char *names[] = {"ServiceA", "ServiceB", "ServiceC", "ServiceD"};
    int tid;

    for (int i = 0; i < 4; i++)
    {
        tid = WHOIS(names[i]);
        uart_printf(CONSOLE, "WHOIS '%s': TID=%d\r\n", names[i], tid);
    }

    EXIT();
}

// Stress test for the Name Server, registers many names
void TaskStressTest()
{
    char name[20];

    for (int i = 0; i < 100; i++)
    {
        uart_printf(CONSOLE, "Attempting to register Service%d\r\n", i);
        int ret = REGISTERAS(name);
        // RECEIVE(0, reply, sizeof(reply)); // Expecting reply from Name Server
        if (ret < 0)
        {
            uart_printf(CONSOLE, "Failed to register '%s' due to overflow or error\r\n", name);
            break;
        }
    }

    EXIT();
}

/*
// Task1 starts the servers and clients
void Task1() {
    uart_printf(CONSOLE, "FirstUserTask: Starting system services.\r\n");

    // Start the Name Server
    int ns_tid = CREATE(HIGH, NameServer);
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
*/