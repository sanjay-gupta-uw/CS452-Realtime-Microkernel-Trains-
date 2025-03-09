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

void IdleTask()
{
    int FUT = CREATE(PRIORITY::P6, FirstUserTask);
    uassert(FUT > 0 && "Error starting First User Task");
    // REGISTERAS("IdleTask");
    // extern Clock clock;
    for (;;)
    {
        // IO_NS::PrintTerminal("IdleTask -- updating clock\r\n");
        // clock.Update();
        asm volatile("wfi");
    }
    EXIT();
}

// tid 2
void FirstUserTask()
{
    // tid 3
    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    uassert(nameServerTid > 0 && "Error starting Name Server");

    // tid 4
    int ioServerTid = CREATE(PRIORITY::P0, IO_SERVER::startIOServer); // Start the IO Server
    uassert(ioServerTid > 0 && "Error starting IO Server");

    IO_NS::PrintTerminal("Starting User Tasks!\r\n");
    IO_NS::PrintTerminal("This should be on the next line!\r\n");

    // IO_NS::PrintTerminal("Starting User Tasks!\r\n");
    // IO_NS::PrintTerminal("NEXT LINE!\r\n");

    // uassert(false && "FORCED PANIC -- FUT -- REMOVE THIS LINE");

    // uassert(false && "FORCED PANIC -- FUT -- REMOVE THIS LINE");
    // // uart_printf(CONSOLE, RESTORE_CURSOR "FUT RUNNING AGAIN -- issuing second print\r\n" SAVE_CURSOR);
    // IO_NS::PrintTerminal("SHOULD BE ON NEXT LINE!\r\n");
    // uassert(false && "FORCED PANIC -- FUT RUNNING AGAIN! -- REMOVE THIS LINE");

    /*
    int marklinIoServerTid = CREATE(PRIORITY::P0, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    uassert(marklinIoServerTid > 0 && "Error starting Marklin IO Server");

    #if IRQ_ENABLED == 1
    int clockServerTid = CREATE(PRIORITY::P0, ClockServer); // Start the Clock Server
    uassert(clockServerTid > 0 && "Error starting Clock Server");
    #endif
    */

    // create sample clients
    // int ui = CREATE(PRIORITY::P3, UI_NS::start_ui); // this initializes the sensors so must be after the marklin io server
    // IO_NS::PrintTerminal("UI Task Created!\r\n");
    // // uassert(false && "FORCED PANIC -- FUT -- REMOVE THIS LINE");
    // uassert(ui > 0 && "Error starting UI Task");
    // uassert(false && "FORCED PANIC -- FUT about to exit -- REMOVE THIS LINE");

    /*
    int marklinControllerTid = CREATE(PRIORITY::P3, MARKLIN_NS::start_marklin_controller); // must be called after UI since ui sets sensors
    uassert(marklinControllerTid > 0 && "Error starting Marklin Controller Task");

    int cmd_prompt = CREATE(PRIORITY::P3, UI_CMD_NS::start_command_prompt);
    uassert(cmd_prompt >= 0 && "Error starting Command Prompt Task");
    */

    EXIT();
}
