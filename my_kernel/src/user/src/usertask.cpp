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
#include "../../register.h"

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
    for (;;)
    {
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

    int clockServerTid = CREATE(PRIORITY::P1, ClockServer); // Start the Clock Server
    uassert(clockServerTid > 0 && "Error starting Clock Server");

    // tid 4
    int ioServerTid = CREATE(PRIORITY::P2, IO_SERVER::startIOServer); // Start the IO Server
    uassert(ioServerTid > 0 && "Error starting IO Server");
    IO_NS::PrintTerminal("IO Server started\r\n");
    for (int i = 0; i < 200; i++)
    {
        IO_NS::PrintTerminal("line %d\r\n", i);
    }
    /*
    IO_SERVER::Putc(ioServerTid, 'A');
    // IO_SERVER::Putc(ioServerTid, 'A');
    for (int i = 0; i < 100; i++)
    {
        IO_SERVER::Putc(ioServerTid, 'A');
        if (i % 10 == 0)
        {
            IO_SERVER::Putc(ioServerTid, '\r');
            IO_SERVER::Putc(ioServerTid, '\n');
        }
    }
    */

    EXIT();
}
