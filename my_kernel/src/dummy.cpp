#include "dummy.h"
#include "rpi.h"
#include "kern/kernel.h"
#include "shared_constants.h"

extern "C" size_t fetch_sp();

extern "C" void _dummyHandler1()
{
    // Get the system call number
    // uart_printf(CONSOLE, "Dummy Handler 1\r\n");
    size_t sp = fetch_sp();
    // uart_printf(CONSOLE, "SP: %x\r\n", sp);

    for (;;)
    {
    }
}

extern "C" void _dummyHandler2(int x)
{
    // Get the system call number
    // uart_printf(CONSOLE, "Dummy Handler 2 {%d}\r\n", x);
    size_t sp = fetch_sp();
    // uart_printf(CONSOLE, "SP: %x\r\n", sp);

    for (;;)
    {
    }
}

extern "C" void _dummyHandler3()
{
    // Get the system call number
    // uart_printf(CONSOLE, "Dummy Handler 3\r\n");
    size_t sp = fetch_sp();
    // uart_printf(CONSOLE, "SP: %x\r\n", sp);

    for (;;)
    {
    }
}

extern "C" void _dummyHandler4()
{
    // Get the system call number
    // uart_printf(CONSOLE, "Dummy Handler 4");
    size_t sp = fetch_sp();
    // uart_printf(CONSOLE, "SP: %x\r\n", sp);

    for (;;)
    {
    }
}