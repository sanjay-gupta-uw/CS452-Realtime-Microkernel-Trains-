#include "../include/usertask.h"
#include "../../kern/syscall.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include "../../shared_constants.h"
#include "../../rpi.h"

extern "C" void Print(char *str)
{
    int ioServerTid = WHOIS("IOServer");
    for (int i = 0; str[i] != '\0'; i++)
    {
        int retval = IO_SERVER::Putc(ioServerTid, CONSOLE, str[i]);
        if (retval < 0)
        {
            uart_printf(CONSOLE, "Error printing character %c to IO Server\r\n", str[i]);
            uart_printf(CONSOLE, "SPINNING FROM PRINT\r\n");
            for (;;)
                ;
            EXIT();
        }
    }
}

void FirstUserTask()
{
    uart_printf(CONSOLE, "First User Task: Starting System Services.\r\n");
    // CREATE IDLE TASK
    // int idleTid = CREATE(PRIORITY::IDLE, IdleTask);

    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    if (nameServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Name Server\r\n");
        EXIT();
    }

    int ioServerTid = CREATE(PRIORITY::P0, startIOServer); // Start the IO Server
    if (ioServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting IO Server\r\n");
        EXIT();
    }

    // create sample clients
    // uart_printf(CONSOLE, "First User Task: Spawning Client Task.\r\n");
    int clientTid = CREATE(PRIORITY::P1, ClientTask);
    if (clientTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Client Task\r\n");
        EXIT();
    }

    EXIT();
}

// CONSOLE CLIENT
void ClientTask()
{
    int myTid = MYTID();
    // uart_printf(CONSOLE, "Client Task: My Tid is %d.\r\n", myTid);

    int ioServerTid = WHOIS("IOServer");
    if (ioServerTid < 0)
    {
        uart_printf(CONSOLE, "Error finding IO Server\r\n");
        EXIT();
    }

    // using this instead of idle for testing
    // uart_printf(CONSOLE, "Client Task: requesting character from IO Server.\r\n");

    // test putc
    // char *str = "Hello from Client Task!\r\n";
    // Print(str);

    int ch = IO_SERVER::Getc(ioServerTid, CONSOLE);
    if (ch < 0)
    {
        uart_printf(CONSOLE, "Error getting character from IO Server\r\n");
        EXIT();
    }
    else
    {
        uart_printf(CONSOLE, "Client Task: Received character %c from IO Server.\r\n", ch);
    }
    uart_printf(CONSOLE, "Client Task: Exiting.\r\n");
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