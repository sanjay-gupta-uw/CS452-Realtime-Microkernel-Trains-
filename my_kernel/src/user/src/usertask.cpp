#include "../include/usertask.h"
#include "../../include/syscall.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include "../include/marklin_io.h"
// #include "../../rpi.h"
#include "../include/ui.h"
#include "../include/clock_server.h"
#include "../include/io.h"
#include "../include/command.h"
#include "../include/uassert.h"
#include "../../shared_constants.h"
#include "../include/conductor.h"
#include "../../register.h"
#include "../marklin/sensor.h"

#if IRQEn == 1
#define IRQ_ENABLED 1
#else
#define IRQ_ENABLED 0
#endif

void IdleTask()
{
    int FUT = CREATE(PRIORITY::LOWEST, FirstUserTask);
    uassert(FUT > 0 && "Error starting First User Task");

    int IdleCompute = CREATE(PRIORITY::LOWEST, ComputeIdleTask);
    uassert(IdleCompute > 0 && "Error starting Compute Idle Task");

    /*
    // REGISTERAS("IdleTask");
    // extern Clock clock;
    */
    for (;;)
    {
        // // IO_NS::PrintTerminal("Idle Task\r\n");
        asm volatile("wfi");
    }
    EXIT();
}

extern "C" void _reboot(void); // Declare the reboot function implemented in assembly

static void CreateCoreServers()
{

    int ioServerTid = CREATE(PRIORITY::CONSOLE_SERVER, IO_SERVER::startIOServer); // Start the IO Server
    uassert(ioServerTid > 0 && "Error starting IO Server");
    // IO_NS::PrintTerminal("IO Server started with TID %d\r\n", ioServerTid);

    // create the clock server
    int clockServerTid = CREATE(PRIORITY::CORE, ClockServer); // Start the Clock Server
    uassert(clockServerTid > 0 && "Error starting Clock Server");
    // IO_NS::PrintTerminal("Clock Server started with TID %d\r\n", clockServerTid);

    // create conductor for communicating between trains/sensors/switches
    // int ConductorTid = CREATE(PRIORITY::ORCHESTRATOR, Conductor_NS::start_conductor);
    int ConductorTid = CREATE(PRIORITY::DEVICE_NOTIFIER, Conductor_NS::start_conductor);
    uassert(ConductorTid > 0 && "Error starting Conductor");
    // IO_NS::PrintTerminal("Conductor started with TID %d\r\n", ConductorTid);

    int marklinIoServerTid = CREATE(PRIORITY::MARKLIN_SERVER, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    uassert(marklinIoServerTid > 0 && "Error starting Marklin IO Server");
    // IO_NS::PrintTerminal("Marklin IO Server started with TID %d\r\n", marklinIoServerTid);
}
// tid 2
void FirstUserTask()
{
    // uart_printf(CONSOLE, RESTORE_CURSOR "First User Task Started\r\n" SAVE_CURSOR);

    // create the name server
    int nameServerTid = CREATE(PRIORITY::CORE, NameServer); // Start the Name Server
    uassert(nameServerTid > 0 && "Error starting Name Server");
    // create the console/marlin IO servers
    CreateCoreServers();

    // IO_NS::PrintTerminal("Name Server started with TID %d\r\n", nameServerTid);

    int uiTaskTid = CREATE(PRIORITY::CORE, UI_NS::start_ui); // Start the UI Task
    uassert(uiTaskTid > 0 && "Error starting UI Task");
    // IO_NS::PrintTerminal("UI Task started with TID %d\r\n", uiTaskTid);

    // create command prompt
    int cmdTid = CREATE(PRIORITY::LOWEST, UI_CMD_NS::start_command_prompt); // Start the Command Task
    uassert(cmdTid > 0 && "Error starting Command Task");
    // IO_NS::PrintTerminal("Command Task started with TID %d\r\n", cmdTid);

    EXIT();
}

// this is deprecated -- see sensor.cpp for server implementation
void SensorTask()
{
}

void ComputeIdleTask()
{
    int CLOCK_SERVER_TID = WHOIS("ClockServer");
    while (true)
    {
        uint32_t idle = GETIDLEPERCENT();
        IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE "%d%%", IDLE_LOCATION, 1 + 6, idle);
        DELAY(CLOCK_SERVER_TID, 50);
    }
    EXIT();
}
