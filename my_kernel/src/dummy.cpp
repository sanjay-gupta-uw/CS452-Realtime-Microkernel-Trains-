#include "dummy.h"
#include "rpi.h"
#include "kern/kernel.h"
#include "shared_constants.h"
#include "kern/kassert.h"

extern "C" size_t fetch_sp();

extern "C" void _dummyHandler1()
{
    kassert(false && "Dummy Handler 1" && "IRQ TRIGGERED");
}

extern "C" void _dummyHandler2(int x)
{
    kassert(false && "Dummy Handler 2" && "IRQ TRIGGERED");
}

extern "C" void _dummyHandler3()
{
    kassert(false && "Dummy Handler 3" && "IRQ TRIGGERED");
}

extern "C" void _dummyHandler4()
{
    kassert(false && "Dummy Handler 4" && "IRQ TRIGGERED");
}