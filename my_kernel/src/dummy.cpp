#include "dummy.h"
#include "rpi.h"
#include "kern/kernel.h"
#include "shared_constants.h"
#include "kern/kassert.h"

extern "C" size_t fetch_sp();

static const char *const group_names[] = {
    "Exception from the current EL while using SP_EL0",
    "Exception from the current EL while using SP_ELx",
    "Exception from a lower EL and at least one lower EL is AArch64",
    "Exception from a lower EL and all lower ELs are AArch32",
};

extern "C" void __SError__(int group)
{
    uart_printf(CONSOLE, "SError: %s\r\n", group_names[group]);
    kassert(false && group_names[group]);
}
extern "C" void __IRQ__(int group)
{
    uart_printf(CONSOLE, "IRQ: %s\r\n", group_names[group]);
    kassert(false && group_names[group]);
}
extern "C" void __FIQ__(int group)
{
    uart_printf(CONSOLE, "FIQ: %s\r\n", group_names[group]);
    kassert(false && group_names[group]);
}
extern "C" void __Sync__(int group)
{
    uart_printf(CONSOLE, "Sync: %s\r\n", group_names[group]);
    kassert(false && group_names[group]);
}
