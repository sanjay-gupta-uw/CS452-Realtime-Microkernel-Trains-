#include "../include/usertask.h"
#include "../../include/syscall.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"
// #include "../../rpi.h"
#include "../include/ui.h"
#include "../include/marklin_controller.h"
#include "../include/clock_server.h"
#include "../include/io.h"
#include "../include/command.hpp"
#include "../include/uassert.h"

#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif

void FirstUserTask()
{

    int nameServerTid = CREATE(PRIORITY::P1, NameServer); // Start the Name Server
    uart_printf(CONSOLE, "Name Server TID: %d\r\n", nameServerTid);

    int ioServerTid = CREATE(PRIORITY::P1, IO_SERVER::startIOServer); // Start the IO Server
    uart_printf(CONSOLE, "IO Server TID: %d\r\n", ioServerTid);

    IO_NS::IO io; // initialize IO object for printing
    IO_NS::Print(CLEAR_SCREEN COLUMN_132 SCROLL_REGION MOVE_CURSOR SMOOTH_SCROLL "Terminal Output:\r\n" SAVE_CURSOR, SCROLL_ROW_START, SCROLL_ROW_END, SCROLL_ROW_START, 1);

    int idleTid = CREATE(PRIORITY::IDLE, IdleTask);
    uassert(idleTid >= 0 && "Error starting Idle Task");

    int marklinIoServerTid = CREATE(PRIORITY::P1, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    uassert(marklinIoServerTid >= 0 && "Error starting Marklin IO Server");

#if IRQ_ENABLED == 1
    int clockServerTid = CREATE(PRIORITY::P1, ClockServer); // Start the Clock Server
    uassert(clockServerTid >= 0 && "Error starting Clock Server");
#endif

    // create sample clients
    int marklinTID = CREATE(PRIORITY::P3, MarklinTask);
    uassert(marklinTID >= 0 && "Error starting Marklin Task");

    int clientTid = CREATE(PRIORITY::P4, ClientTask);
    uassert(clientTid >= 0 && "Error starting Client Task");

    EXIT();
}

// CONSOLE CLIENT
void ClientTask()
{
    int myTid = MYTID();

    int ui = CREATE(PRIORITY::P4, UI_NS::start_ui);
    uassert(ui >= 0 && "Error starting UI Task");

    int cmd_prompt = CREATE(PRIORITY::P3, UI_CMD_NS::start_command_prompt);
    uassert(cmd_prompt >= 0 && "Error starting Command Prompt Task");

    EXIT();
}

void MarklinTask()
{

    // start marklin task
    int marklinControllerTid = CREATE(PRIORITY::P2, MARKLIN_NS::start_marklin_controller);
    uassert(marklinControllerTid >= 0 && "Error starting Marklin Controller Task");

    EXIT();
}

void IdleTask()
{
    // extern Clock clock;
    for (;;)
    {
        // IO_NS::PrintTerminal("IdleTask -- updating clock\r\n");
        // clock.Update();
        asm volatile("wfi");
    }
    EXIT();
}
