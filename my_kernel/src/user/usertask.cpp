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
#define REPEATS 10000

// Task function prototypes
/*
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
*/

// CT1 only throws rock
void clientTask1()
{
    GameMove moves[] = {ROCK, ROCK, ROCK, QUIT};
    RpsClient(moves, 4);
}

void clientTask2()
{
    GameMove moves[] = {ROCK, PAPER, SCISSORS, PAPER, QUIT};
    RpsClient(moves, 5);
}

void clientTask3()
{
    GameMove moves[] = {PAPER, SCISSORS, SCISSORS, QUIT, QUIT, QUIT};
    RpsClient(moves, 6);
}

void clientTask4()
{
    GameMove moves[] = {PAPER, SCISSORS, PAPER, PAPER, QUIT, QUIT, QUIT};
    RpsClient(moves, 7);
}

void Task1()
{
    uart_printf(CONSOLE, "FirstUserTask: Starting system services.\r\n");

    int ns_tid = CREATE(HIGH, NameServer);
    if (ns_tid < 0)
    {
        uart_printf(CONSOLE, "Failed to start Name Server. Exiting...\r\n");
        EXIT();
    }
    setNameServerTid(ns_tid);
    uart_printf(CONSOLE, "Name Server started with TID: %d\r\n", ns_tid);

    int rps_server_tid = CREATE(HIGH, RpsServer);
    uart_printf(CONSOLE, "RPS Server started with TID: %d\r\n", rps_server_tid);

    YIELD();
    YIELD(); // Allow servers to initialize

    int client_tid1 = CREATE(MEDIUM, clientTask1);
    int client_tid2 = CREATE(MEDIUM, clientTask2);

    int client_tid3 = CREATE(MEDIUM, clientTask3);
    int client_tid4 = CREATE(MEDIUM, clientTask4);

    // for (int i = 0; i < 20; ++i)
    // {
    //     YIELD();
    // }

    // uart_printf(CONSOLE, "FirstUserTask: Shutting down system.\r\n");
    EXIT();
}

bool SendTask(int r_tid, int msglen_opt, char *reply, int replylen)
{
    char msg_4[4] = "123";
    char msg_64[64] = "123456789012345678901234567890123456789012345678901234567890123";
    char msg_256[256] = "123456789012345678901234567890123456789012345678901234567890123412345678901234567890123456789012345678901234567890123456789012341234567890123456789012345678901234567890123456789012345678901234123456789012345678901234567890123456789012345678901234567890123";
    char getTime[2] = "t";
    char quit[2] = "q";

    int msglen = 0;

    char *msg = nullptr;
    switch (msglen_opt)
    {
    case 0:
        msg = msg_4;
        msglen = 4;
        break;

    case 1:
        msg = msg_64;
        msglen = 64;
        break;

    case 2:
        msg = msg_256;
        msglen = 256;
        break;
    case 3:
        msg = getTime;
        msglen = 2;
        break;
    case 4:
        msg = quit;
        msglen = 2;
        break;
    default:
        return false;
    }

    // uart_printf(CONSOLE, "ABOUT TO SEND TO RECEIVER %d\r\n", r_tid);
    int retval = SEND(r_tid, msg, msglen, reply, replylen);
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

    char reply_4[4];
    char reply_64[64];
    char reply_256[256];

    char *replies[3] = {reply_4, reply_64, reply_256};
    int message_sizes[3] = {4, 64, 256};

#if OPT == 1
#define OPT_VALUE 1
#else
#define OPT_VALUE 0
#endif

    // Performance test
    int tid = MYTID();
    uart_printf(CONSOLE, "PerformanceTask: TID=%d\r\n", tid);

    // Performance test
    // int COMPILER_OPTS = 2; // perform this outside of the loop

    uint32_t times[CACHE_OPTS][SSR_EXCHANGE_OPTS][MSG_SIZE_OPTS];
    int TEST_TO_CACHE = 2;

    for (int cache_opt = 0; cache_opt < CACHE_OPTS; ++cache_opt)
    {
        if (cache_opt > TEST_TO_CACHE)
        {
            break;
        }
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
                    bool success = SendTask(receiver_tid, msg_opt, replies[msg_opt], message_sizes[msg_opt]);
                    if (!success)
                    {
                        uart_printf(CONSOLE, "PerformanceTask: SendTask failed.\r\n");
                        break;
                    }
                }
                end_time = clock.Time();
                SendTask(receiver_tid, 3, replies[1], message_sizes[1]); // get the start time in 64 byte buffer
                // uart_printf(CONSOLE, "PERF: Start Time: %s\r\n", replies[1]);
                // convert replies[1] to uint32_t
                unsigned int receiver_start_time = a2ui(&replies[1], 10);
                // uart_printf(CONSOLE, "PERF: Start Time (uint): %d\r\n", receiver_start_time); // uart_printf(CONSOLE, "PERF: Receiver Start Time: %d\r\n", receiver_start_time);

                start_time = start_time > receiver_start_time ? receiver_start_time : start_time;

                uart_printf(CONSOLE, "Start Time: %d, End Time: %d, Duration: %d\r\n", start_time, end_time, end_time - start_time);
                times[cache_opt][isReceiveFirst][msg_opt] = (end_time - start_time) / REPEATS; // update times array
            }
            SendTask(receiver_tid, 4, replies[0], message_sizes[0]); // quit -> 4 bytes sufficient
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
        if (cache_opt > TEST_TO_CACHE)
        {
            break;
        }
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
    uint32_t start_time = clock.Time();
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

        switch (msg[0])
        {
        case 't':
        {
            // if t, send the start time
            ui2a(start_time, 10, reply);
            msglen = REPLY(sender_tid, reply, 12);
            break;
        }
        case 'q':
        {
            // if q, exit
            msglen = REPLY(sender_tid, "Q", 2);
            EXIT();
            break; // this should never be reached
        }
        default:
            for (int i = 0; i < msglen - 1; i++)
            {
                reply[i] = '1';
            }
            reply[msglen] = '\0';
            // reply
            REPLY(sender_tid, reply, msglen);
            break;
        }
    }

    // EXIT();
}

// **************************************** K3 ****************************************

#define TIMER_C1 97
#define TIMER_C3 99
void FirstUserTask_K3()
{
    int tid = MYTID();
    uart_printf(CONSOLE, "K3 FirstUserTask: Starting system services, TID=%d.\r\n", tid);

    CREATE(IDLE, idle_task);
    uart_printf(CONSOLE, "FUT EXit.\r\n");
    EXIT();
}

void idle_task()
{
    int tid = MYTID();
    uart_printf(CONSOLE, "IDLE TASK: Starting with TID=%d.\r\n", tid);

    CREATE(HIGH, AwaitTestTask_K3);
    while (1)
    {
        // uart_printf(CONSOLE, "IDLE TASK\r\n");
        asm volatile("wfi"); // Wait for interrupt (ARMv8 low-power mode)
        // YIELD();
    }
}

void AwaitTestTask_K3()
{
    int tid = MYTID();
    uart_printf(CONSOLE, "K3 AwaitTestTask: Starting with TID=%d.\r\n", tid);
    int ret = AWAITEVENT(TIMER_TICK);
    uart_printf(CONSOLE, "K3 AwaitTestTask: AWAITEVENT returned %d.\r\n", ret);

    int invalid = AWAITEVENT(10);
    uart_printf(CONSOLE, "INVALID EVENT should be -1: %d\r\n", invalid);

    uart_printf(CONSOLE, "EXITING AWAIT TASK");
    EXIT();
}
