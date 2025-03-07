#include <stdbool.h>

#include "clock.h"
// #include "command.h" // for command prompt
#include "dummy.h"
#include "rpi.h"
#include "user/include/usertask.h"
#include "kern/kassert.h"
// #include "marklin/switch.h"
// #include "marklin/train.h"

#include "containers/ringbuffer.h"

// #include "sensor.h"

// instantiate the template class
#include "kern/kernel.h"
#include "kern/memory.h"

// extern void _reboot(void); // Declare the reboot function implemented in assembly

#if PERF == 1
#define PERF_VALUE 1
#else
#define PERF_VALUE 0
#endif

typedef void (*funcvoid_t)();
extern funcvoid_t __init_array_start;
extern funcvoid_t __init_array_end;

static uint32_t TOTAL_IDLE_TIME = 0;
static uint32_t TOTAL_TIME = 0;

// ENSURE THESE GET INITIALIZED
Clock clock;
// Switches switches;
// Trains trains;
// CommandPrompt cmd_prompt;

void call_global_constructors()
{
    for (funcvoid_t *ctr = &__init_array_start; ctr < &__init_array_end; ++ctr)
    {
        (*ctr)();
    }
}

extern "C" int __cxa_atexit(void (*)(void *), void *, void *)
{
    return 0; // Do nothing
}

void *__dso_handle = 0;

extern "C" size_t fetch_sp();
extern "C" void setup_mmu();
extern "C" int _get_el_();

extern "C" void _start(); // expose this to LLDB

extern "C" void printASM(int x0)
{
    // uart_printf(CONSOLE, "X0: 0x%x \n", x0);
}

extern "C" int kmain()
{
    if (false)
    {
        _start();
    }
    call_global_constructors();
    // Set up GPIO pins for both console and Marklin UARTs
    gpio_init();

    int delay = 100;
    clock.Delay(delay);
    // Not strictly necessary, since console is configured during boot
    uart_config_and_enable(CONSOLE);
    uart_config_and_enable(MARKLIN);

    // size_t sp = fetch_sp();
    clear_screen(CONSOLE);
    reset_formatting(CONSOLE);
    move_cursor(CONSOLE, IDLE_LOCATION, 1);
    // uart_printf(CONSOLE, "Welcome to the Train Controller\r\n");

#if defined(MMU)
    setup_mmu();
#endif

    Context kernel_context;                    // Initialize kernel context
    funcvoid_t bootstrap_task = FirstUserTask; // bootstrap task with first user task in user/usertask.h

    Kernel kernel(bootstrap_task); // bootstrap
    TaskDescriptor *current_task = nullptr;

    uint32_t start_time, end_time = 0;

    UART_IMSC_ENABLE(MARKLIN, (CTS_INTERRUPT_MASK)); // enable CTS interrupt
    int EL = _get_el_();
    uart_printf(CONSOLE, "Kernel has started (EL%d)\r\n", EL);
    // scheduler pops the highest priority task into td
    while ((current_task = kernel.Scheduler()) || kernel.areTasksWaiting())
    {
        // uart_printf(CONSOLE, "Kernel loop \r\n");
        start_time = clock.Time();
        int esr_el1 = kernel.DispatchTask(&kernel_context, current_task);
        end_time = clock.Time();

        // Get the time the task was running for (including transfer times)
        uint32_t task_time = end_time - start_time;
        TOTAL_TIME += task_time;
        if (current_task->getPriority() == PRIORITY::IDLE)
        {
            TOTAL_IDLE_TIME += task_time;
        }

        // apply mask to ESR to get SVC number
        int N = esr_el1 & 0xFFFF;
        // // uart_printf(CONSOLE, "ACTIVE: {%d}, ESR: {%d}, N: {%d}\r\n", current_task->getTid(), esr_el1, N);
        kassert(N >= 0 && "UNEXPECTED ERROR DECODING SVC NUMBER");
        kernel.Handler(N, (uint32_t)((TOTAL_IDLE_TIME * 100) / TOTAL_TIME));
    }

    kassert(false && "Kernel loop");
    return 0;
}
