#include "../include/usertask.h"
#include "../../include/syscall.h"
#include "../include/name_server.h"
#include "../include/io_server.h"
#include "../include/marklin_io.h"
#include "../../shared_constants.h"
// #include "../../rpi.h"
#include "../include/ui.h"
#include "../include/clock_server.h"
#include "../include/io.h"
#include "../include/command.hpp"
#include "../include/uassert.h"
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
    int FUT = CREATE(PRIORITY::P4, FirstUserTask);
    uassert(FUT > 0 && "Error starting First User Task");
    /*
    // REGISTERAS("IdleTask");
    // extern Clock clock;
    */
    int IdleCompute = CREATE(PRIORITY::P6, ComputeIdleTask);
    uassert(IdleCompute > 0 && "Error starting Compute Idle Task");
    for (;;)
    {
        // IO_NS::PrintTerminal("Idle Task\r\n");
        asm volatile("wfi");
    }
    EXIT();
}

extern "C" void _reboot(void); // Declare the reboot function implemented in assembly

static void CreateIOServers()
{
    int ioServerTid = CREATE(PRIORITY::P3, IO_SERVER::startIOServer); // Start the IO Server
    uassert(ioServerTid > 0 && "Error starting IO Server");
    IO_NS::PrintTerminal("IO Server started with TID %d\r\n", ioServerTid);

    int marklinIoServerTid = CREATE(PRIORITY::P0, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    uassert(marklinIoServerTid > 0 && "Error starting Marklin IO Server");
    IO_NS::PrintTerminal("Marklin IO Server started with TID %d\r\n", marklinIoServerTid);
}
// tid 2
void FirstUserTask()
{
    // uart_printf(CONSOLE, RESTORE_CURSOR "First User Task Started\r\n" SAVE_CURSOR);

    // create the name server
    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    uassert(nameServerTid > 0 && "Error starting Name Server");
    // create the console/marlin IO servers
    CreateIOServers();

    IO_NS::PrintTerminal("Name Server started with TID %d\r\n", nameServerTid);

    // create the clock server
    int clockServerTid = CREATE(PRIORITY::P1, ClockServer); // Start the Clock Server
    uassert(clockServerTid > 0 && "Error starting Clock Server");
    IO_NS::PrintTerminal("Clock Server started with TID %d\r\n", clockServerTid);

    // create conductor for communicating between trains/sensors/switches
    int ConductorTid = CREATE(PRIORITY::P4, Conductor_NS::start_conductor);
    uassert(ConductorTid > 0 && "Error starting Conductor");
    IO_NS::PrintTerminal("Conductor started with TID %d\r\n", ConductorTid);

    // create command prompt
    int cmdTid = CREATE(PRIORITY::P6, UI_CMD_NS::start_command_prompt); // Start the Command Task
    uassert(cmdTid > 0 && "Error starting Command Task");
    IO_NS::PrintTerminal("Command Task started with TID %d\r\n", cmdTid);

    // create the UI task
    int uiTaskTid = CREATE(PRIORITY::P6, UI_NS::start_ui); // Start the UI Task
    uassert(uiTaskTid > 0 && "Error starting UI Task");
    IO_NS::PrintTerminal("UI Task started with TID %d\r\n", uiTaskTid);

    EXIT();
}

// this is deprecated -- see sensor.cpp for server implementation
void SensorTask()
{
    REGISTERAS("SensorTask");
    // uassert(4 == 5);
    Sensors_NS::SensorManager sensors;
    while (true)
    {
        sensors.ReadAll(NUM_BANKS); // this will put it to sleep until data is ready
        sensors.Display();
    }
    EXIT();
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
