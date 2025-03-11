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
    int FUT = CREATE(PRIORITY::P6, FirstUserTask);
    uassert(FUT > 0 && "Error starting First User Task");

    /*
    // REGISTERAS("IdleTask");
    // extern Clock clock;
    */
    int IdleCompute = CREATE(PRIORITY::P6, ComputeIdleTask);
    uassert(IdleCompute > 0 && "Error starting Compute Idle Task");
    for (;;)
    {
        // IO_NS::PrintTerminal("IdleTask -- \r\n");
        // uassert(false && "IdleTask -- PANIC: Idle Task");
        // IO_NS::PrintTerminal("IdleTask -- updating clock\r\n");
        // clock.Update();
        asm volatile("wfi");
    }
    EXIT();
}

extern "C" void _reboot(void); // Declare the reboot function implemented in assembly
// tid 2
void FirstUserTask()
{
    // tid 3
    uart_printf(CONSOLE, RESTORE_CURSOR "First User Task Started\r\n" SAVE_CURSOR);
    int nameServerTid = CREATE(PRIORITY::P0, NameServer); // Start the Name Server
    uassert(nameServerTid > 0 && "Error starting Name Server");

    // tid 4
    int ioServerTid = CREATE(PRIORITY::P4, IO_SERVER::startIOServer); // Start the IO Server
    uassert(ioServerTid > 0 && "Error starting IO Server");
    // IO_NS::Print(CLEAR_SCREEN RESET_FORMATTING COLUMN_132 SCROLL_REGION MOVE_CURSOR SAVE_CURSOR, SCROLL_ROW_START, SCROLL_ROW_END, 1, 1);
    IO_NS::PrintTerminal("IO Server started\r\n");
    // uart_printf(CONSOLE, RESET_FORMATTING CLEAR_SCREEN COLUMN_132 SCROLL_REGION MOVE_CURSOR SMOOTH_SCROLL "Terminal Output Kernel:\r\n" SAVE_CURSOR, SCROLL_ROW_START, SCROLL_ROW_END, SCROLL_ROW_START, 1);

    int ConductorTid = CREATE(PRIORITY::P3, Conductor_NS::start_conductor);
    uassert(ConductorTid > 0 && "Error starting Conductor");
    IO_NS::PrintTerminal("Sending track id to Conductor\r\n");
    SEND(ConductorTid, "A", 1, nullptr, 0);
    IO_NS::PrintTerminal("First User Task -- Conductor replied\r\n");

    int clockServerTid = CREATE(PRIORITY::P0, ClockServer); // Start the Clock Server
    uassert(clockServerTid > 0 && "Error starting Clock Server");

    // /*
    // // START MARKLIN SERVER + CONTROLLER
    int marklinIoServerTid = CREATE(PRIORITY::P0, MARKLIN_IO_SERVER::startMarklinIOServer); // Start the Marklin IO Server
    uassert(marklinIoServerTid > 0 && "Error starting Marklin IO Server");

    int SensorTaskTid = CREATE(PRIORITY::P1, SensorTask);
    uassert(SensorTaskTid >= 0 && "Error starting Sensor Task");

    int marklinControllerTid = CREATE(PRIORITY::P3, MARKLIN_NS::start_marklin_controller); // Start the Marklin Controller
    uassert(marklinControllerTid > 0 && "Error starting Marklin Controller");
    //*/
    int uiTaskTid = CREATE(PRIORITY::P5, UI_NS::start_ui); // Start the UI Task
    uassert(uiTaskTid > 0 && "Error starting UI Task");

    int cmdTid = CREATE(PRIORITY::P5, UI_CMD_NS::start_command_prompt); // Start the Command Task
    uassert(cmdTid > 0 && "Error starting Command Task");

    EXIT();
}

// this could send to the controller, but faster to send to the IO SERVER directly
void SensorTask()
{
    REGISTERAS("SensorTask");
    // uassert(4 == 5);
    Sensors_NS::SensorManager sensors;
    // while (true)
    // {
    // sensors.ReadAll(NUM_BANKS); // this will put it to sleep until data is ready
    // sensors.Display();
    // }
    EXIT();
}

void ComputeIdleTask()
{
    int CLOCK_SERVER_TID = WHOIS("ClockServer");
    while (true)
    {
        int idle = GETIDLEPERCENT();
        IO_NS::Print(MOVE_CURSOR COLOR_CYAN "%d", IDLE_LOCATION, 1 + 6, idle);
        DELAY(CLOCK_SERVER_TID, 10);
    }
    EXIT();
}
