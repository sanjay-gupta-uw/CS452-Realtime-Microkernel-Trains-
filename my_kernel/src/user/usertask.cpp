#include "usertask.h"
#include "../kern/syscall.h"
#include "name_server.h"
#include "clock_server.h"
#include "../shared_constants.h"
#include "../rpi.h"

void FirstUserTask()
{
    uart_printf(CONSOLE, "First User Task: Starting System Services.\r\n");
    // CREATE IDLE TASK
    int idleTid = CREATE(PRIORITY::IDLE, IdleTask);

    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    if (nameServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Name Server\r\n");
        EXIT();
    }

    int clockServerTid = CREATE(PRIORITY::P0, ClockServer); // Start the Clock Server
    if (clockServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Clock Server\r\n");
        EXIT();
    }

    // Create Clock Client Tasks with varying priorities and delays
    int delay_intervals[] = {10, 23, 33, 71};
    int delays[] = {20, 9, 6, 3};
    int priorities[] = {3, 4, 5, 6};
    int clientTids[] = {-1, -1, -1, -1};

    for (int i = 0; i < 4; i++)
    {
        clientTids[i] = CREATE(priorities[i], ClientTask);
        if (clientTids[i] < 0)
        {
            uart_printf(CONSOLE, "Error starting Client Task %d\r\n", i);
            EXIT();
        }
        // uart_printf(CONSOLE, "Client Task %d created with priority %d\r\n", clientTids[i], priorities[i]);
    }

    // uart_printf(CONSOLE, "First User Task: ALL CLIENTS CREATED.\r\n");
    // Wait for initialization messages from clients
    for (int i = 0; i < 4; i++)
    {
        int senderTid;
        char msg[64];
        RECEIVE(&senderTid, msg, sizeof(msg));
        // uart_printf(CONSOLE, "Initialization message from client %d: %s\r\n", senderTid, msg);

        bool found = false;
        for (int j = 0; j < 4; j++)
        {
            if (clientTids[j] == senderTid)
            {
                // uart_printf(CONSOLE, "Sending params to client %d\r\n", senderTid);
                ParamRequest request = {delay_intervals[j], delays[j]};
                REPLY(senderTid, (char *)&request, sizeof(request));
                found = true;
                break;
            }
        }
        if (!found)
        {
            uart_printf(CONSOLE, "Error: Initialization message from unknown client %d\r\n", senderTid);
        }
    }
    EXIT();
}

void ClientTask()
{
    // uart_printf(CONSOLE, "Client Task: Initializing.\r\n");
    // Client task wants to request params from FUT
    int parentTid = MYPARENTTID();

    ParamRequest request = {0, 0}; // initialize with random values
    int retval = SEND(parentTid, "Client task initializing", 25, (char *)&request, sizeof(request));
    if (retval < 0)
    {
        uart_printf(CONSOLE, "Error sending initialization message to parent\r\n");
    }

    int delay_interval = request.delay_interval;
    int num_delays = request.num_delays;

    // uart_printf(CONSOLE, "Client Task: RECEIVED delays with delay interval {%d} and num delays {%d}.\r\n", delay_interval, num_delays);

    int clockServerTid = WHOIS("ClockServer");
    if (clockServerTid < 0)
    {
        uart_printf(CONSOLE, "Error finding Clock Server\r\n");
        EXIT();
    }

    for (int i = 1; i <= num_delays; i++)
    {
        // uart_printf(CONSOLE, "Client Task: Delaying for {%d} ticks.\r\n", delay_interval);
        // Delay for delay_interval
        DELAY(clockServerTid, delay_interval); // delay is wrapper for send to clock server, so blocked until clock server replies
        uart_printf(CONSOLE, "tid{%d}, delay interval{%d}, delays completed{%d}\r\n", MYTID(), delay_interval, i);
    }

    EXIT();
}

void IdleTask()
{
    for (;;)
    {
        asm volatile("wfi");
    }
    EXIT();
}