#include "../include/usertask.h"
#include "../../kern/syscall.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include "../include/marklin_io.h"
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
            uart_printf(CONSOLE, "\r\nError printing character %c to IO Server\r\n", str[i]);
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
    int idleTid = CREATE(PRIORITY::IDLE, IdleTask);

    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    if (nameServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Name Server\r\n");
        EXIT();
    }

    // int ioServerTid = CREATE(PRIORITY::P0, IO_SERVER::startIOServer); // Start the IO Server
    // if (ioServerTid < 0)
    // {
    //     uart_printf(CONSOLE, "Error starting IO Server\r\n");
    //     EXIT();
    // }

    int marklinIoServerTid = CREATE(PRIORITY::P0, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    if (marklinIoServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Marklin IO Server\r\n");
        EXIT();
    }

    // create sample clients
    // uart_printf(CONSOLE, "First User Task: Spawning Client Task.\r\n");
    // int clientTid = CREATE(PRIORITY::P4, ClientTask);
    int marklinTID = CREATE(PRIORITY::P4, MarklinTask);
    if (marklinTID < 0)
    {
        uart_printf(CONSOLE, "Error starting Marklin Client Task\r\n");
        EXIT();
    }
    int clientTid = CREATE(PRIORITY::P4, ClientTask);
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
    int ret = -1;
    int ch = -1;
    // test print
    Print("Client Task: Starting IO Test.\r\n");

    while (true)
    {
        ch = MARKLIN_IO_SERVER::Getc(ioServerTid, CONSOLE);
        ret = MARKLIN_IO_SERVER::Putc(ioServerTid, CONSOLE, ch);
    }

    // using this instead of idle for testing
    for (;;)
    {
        YIELD();
    }

    EXIT();
}

void MarklinTask()
{
    int myTid = MYTID();
    // uart_printf(CONSOLE, "Client Task: My Tid is %d.\r\n", myTid);

    int ioServerTid = WHOIS("MarklinIOServer");
    if (ioServerTid < 0)
    {
        uart_printf(CONSOLE, "Error finding IO Server\r\n");
        EXIT();
    }
    int ret = -1;

    unsigned char ch = 'A';

    for (int i = 0; i < 1; ++i)
    {
        // uart_printf(CONSOLE, "Marklin Client Task: Sending character {%c} to IO SERVER.\r\n", ch);
        // ch = MARKLIN_IO_SERVER::Getc(ioServerTid, CONSOLE);
        // ret = MARKLIN_IO_SERVER::Putc(ioServerTid, CONSOLE, ch);

        uart_printf(CONSOLE, "Marklin Client Task: Sending character {%c} to marklin.\r\n", ch);
        uart_putc(MARKLIN, ch);
    }

    ch = 'B';
    for (int i = 0; i < 10; ++i)
    {
        ret = MARKLIN_IO_SERVER::Putc(ioServerTid, CONSOLE, ch);
    }

    // using this instead of idle for testing
    for (;;)
    {
        YIELD();
    }

    uart_printf(CONSOLE, "Marklin Client Task: Exiting.\r\n");
    EXIT();
}

void IdleTask()
{
    for (;;)
    {
        // asm volatile("wfi");
        YIELD();
    }
    EXIT();
}