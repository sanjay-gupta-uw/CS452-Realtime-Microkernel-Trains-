#include "usertask.h"
#include "../rpi.h"
#include "../shared_constants.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "rps_server.h"
#include "rps_client.h"
#include <cstring>

#define CACHE_OPTS 4
#define SSR_EXCHANGE_OPTS 2
#define MSG_SIZE_OPTS 3
// #define REPEATS 1
#define REPEATS 1000

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

bool SendTask(int r_tid, int msglen)
{
    char reply_4[4] = "123";
    char reply_64[64] = "123456789012345678901234567890123456789012345678901234567890123";
    char reply_256[256] = "123456789012345678901234567890123456789012345678901234567890123412345678901234567890123456789012345678901234567890123456789012341234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123";
    char quit[2] = "q";

    char *s = nullptr;
    switch (msglen)
    {
    case 0:
        s = reply_4;
        msglen = 4;
        break;

    case 1:
        s = reply_64;
        msglen = 64;
        break;

    case 2:
        s = reply_256;
        msglen = 256;
        break;
    case 3:
        s = quit;
        msglen = 2;
        break;
    default:
        return false;
    }

    // uart_printf(CONSOLE, "ABOUT TO SEND TO RECEIVER %d\r\n", r_tid);
    int retval = SEND(r_tid, s, msglen, s, msglen);
    // uart_printf(CONSOLE, "Received {%d} bytes {%s} from R{%d}\r\n", retval, s, r_tid);
    if (retval < 0)
    {
        uart_printf(CONSOLE, "SendTask: SEND failed with retval %d\r\n", retval);
        return false;
    }
    return true;
}
// This task is used to test the performance of the SSR system under 48 different scenarios
void PerformanceTask()
{
    uart_printf(CONSOLE, "PerformanceTask: Starting.\r\n");
#if OPT == 1
#define OPT_VALUE 1
#else
#define OPT_VALUE 0
#endif

    int message_sizes[3] = {4, 64, 256};
    // Performance test
    int tid = MYTID();
    uart_printf(CONSOLE, "PerformanceTask: TID=%d\r\n", tid);

    // Performance test
    // int COMPILER_OPTS = 2; // perform this outside of the loop

    uint32_t times[CACHE_OPTS][SSR_EXCHANGE_OPTS][MSG_SIZE_OPTS];

    for (int cache_opt = 0; cache_opt < CACHE_OPTS; ++cache_opt)
    {
        // if (cache_opt > 0)
        // {
        //     break;
        // }
        switch (cache_opt)
        {
        case 0:
            // No cache
            break;
        case 1:
            // I-cache
            uart_printf(CONSOLE, "ENABLE_ICACHE\r\n");
            ENABLE_ICACHE();
            break;
        case 2:
            // D-cache
            uart_printf(CONSOLE, "ENABLE_DCACHE\r\n");
            ENABLE_DCACHE();
            break;
        case 3:
            // Both caches
            uart_printf(CONSOLE, "ENABLE_BCACHE\r\n");
            ENABLE_BCACHE();
            break;
        }

        uint32_t start_time, end_time;
        for (int isReceiveFirst = 0; isReceiveFirst < SSR_EXCHANGE_OPTS; ++isReceiveFirst)
        {
            int receiver_tid = isReceiveFirst ? CREATE(HIGH, ReceiveTask) : CREATE(LOW, ReceiveTask);
            // YIELD(); // dont need yield since create reschedules
            for (int msg_opt = 0; msg_opt < MSG_SIZE_OPTS; ++msg_opt)
            {
                start_time = clock.Time();

                // Performance test
                for (int m = 0; m < REPEATS; ++m)
                {
                    bool success = SendTask(receiver_tid, msg_opt);
                    if (!success)
                    {
                        uart_printf(CONSOLE, "PerformanceTask: SendTask failed.\r\n");
                        break;
                    }
                }
                end_time = clock.Time();

                uart_printf(CONSOLE, "Start Time: %d, End Time: %d\r\n", start_time, end_time);
                times[cache_opt][isReceiveFirst][msg_opt] = (end_time - start_time) / REPEATS; // update times array
            }
            SendTask(receiver_tid, 3); // need to restart task with different priority
        }
    }

    // clear_screen(CONSOLE);

    uart_printf(CONSOLE, "**********\r\n");
    uart_printf(CONSOLE, "Final Results (REPEAT = %d):\r\n", REPEATS);

    const char *opt = "opt";
    const char *noopt = "noopt";

    const char *opt_string;
    opt_string = OPT_VALUE ? opt : noopt;

    for (int cache_opt = 0; cache_opt < CACHE_OPTS; ++cache_opt)
    {
        // if (cache_opt > 0)
        // {
        //     break;
        // }
        const char *nocache = "nocache";
        const char *icache = "icache";
        const char *dcache = "dcache";
        const char *bcache = "bcache";

        const char *cache_string;
        switch (cache_opt)
        {
        case 0:
            cache_string = nocache;
            break;
        case 1:
            cache_string = icache;
            break;
        case 2:
            cache_string = dcache;
            break;
        case 3:
            cache_string = bcache;
            break;
        }

        for (int isReceiveFirst = 0; isReceiveFirst < SSR_EXCHANGE_OPTS; ++isReceiveFirst)
        {
            for (int msg_opt = 0; msg_opt < MSG_SIZE_OPTS; ++msg_opt)
            {
                uart_printf(CONSOLE, "%s,%s,%c,%d,%d\r\n", opt_string, cache_string, isReceiveFirst ? 'R' : 'S', message_sizes[msg_opt], times[cache_opt][isReceiveFirst][msg_opt]);
            }
        }
    }

    EXIT();
}

void ReceiveTask()
{
    // int tid = MYTID();
    // uart_printf(CONSOLE, "ReceiveTask: TID=%d\r\n", tid);
    int sender_tid;
    char msg[256];
    char reply[256];

    while (true)
    {
        int msglen = RECEIVE(&sender_tid, msg, 256);
        if (msglen < 0)
        {
            uart_printf(CONSOLE, "ReceiveTask: RECEIVE failed with retval %d\r\n", msglen);
            break;
        }
        // uart_printf(CONSOLE, "ReceiveTask: Received message ({%d} bytes) from %d: %s\r\n", msglen, sender_tid, msg);

        if (msg[0] == 'q')
        {
            // uart_printf(CONSOLE, "ReceiveTask: Exiting.\r\n");
            msglen = REPLY(sender_tid, "Q", 2);
            break;
        }
        for (int i = 0; i < msglen - 1; i++)
        {
            reply[i] = '1';
        }
        reply[msglen] = '\0';
        // reply
        REPLY(sender_tid, reply, msglen);

        // this should execute after the sender has received the reply if LOWER priority
        // uart_printf(CONSOLE, "ReceiveTask: Replied to %d: %s\r\n", sender_tid, "World");
    }

    EXIT();
}