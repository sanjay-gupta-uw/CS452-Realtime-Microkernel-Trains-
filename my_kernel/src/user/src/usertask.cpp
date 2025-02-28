#include "../include/usertask.h"
#include "../../kern/syscall.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"
// #include "../../rpi.h"
#include "../include/ui.h"
#include "../include/marklin_controller.h"
#include "../include/clock_server.h"
#include "../include/io.h"

void FirstUserTask()
{
    clear_screen(CONSOLE);
    uart_printf(CONSOLE, "First User Task: Starting System Services.\r\n");
    // CREATE IDLE TASK
    int idleTid = CREATE(PRIORITY::IDLE, IdleTask);

    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    if (nameServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Name Server\r\n");
        EXIT();
    }
    // clock server
    int clockServerTid = CREATE(PRIORITY::P0, ClockServer); // Start the Clock Server
    if (clockServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Clock Server\r\n");
        EXIT();
    }

    int ioServerTid = CREATE(PRIORITY::P0, IO_SERVER::startIOServer); // Start the IO Server
    if (ioServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting IO Server\r\n");
        EXIT();
    }

    IO_NS::IO io; // initialize IO object for printing

    int marklinIoServerTid = CREATE(PRIORITY::P0, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    if (marklinIoServerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Marklin IO Server\r\n");
        EXIT();
    }

    // create sample clients
    int marklinTID = CREATE(PRIORITY::P3, MarklinTask);
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
    // uart_puts(CONSOLE, "Client Task: Starting IO Test.\r\n");

    int ioServerTid = WHOIS("IOServer");
    if (ioServerTid < 0)
    {
        uart_printf(CONSOLE, "Error finding IO Server\r\n");
        EXIT();
    }
    uart_printf(CONSOLE, "Client Task: Found IO Server {tid: %d}.\r\n", ioServerTid);
    int ret = -1;
    int ch = -1;
    // test print
    // uart_putc(CONSOLE, 'A'); // must initialize uart before using
    // uart_printf(CONSOLE, "\r\n");

    IO_NS::Print("Hello World!\r\n");
    IO_NS::Print(CLEAR_SCREEN);
    int ui = CREATE(PRIORITY::P4, UI_NS::start_ui);
    int cmd_prompt = CREATE(PRIORITY::P3, UI_CMD_NS::start_command_prompt);

    EXIT();
}

void MarklinTask()
{

    // start marklin task
    int marklinControllerTid = CREATE(PRIORITY::P2, MARKLIN_NS::start_marklin_controller);
    if (marklinControllerTid < 0)
    {
        uart_printf(CONSOLE, "Error starting Marklin Controller Task\r\n");
        EXIT();
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
