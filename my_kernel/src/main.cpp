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
#include "kern/interrupts.h"

#include "register.h"
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

#define MMIO_BASE 0xFE000000

#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))
#define UART_FR 0x18

// masks for flag register
#define UART_FR_TXFE (1 << 7)
#define UART_FR_CTS (1 << 0)

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

extern "C" int kmain()
{
    call_global_constructors();
    // Set up GPIO pins for both console and Marklin UARTs
    gpio_init();

    int delay = 100;
    clock.Delay(delay);
    // disable_irq(); // disable interrupts
    // uint64_t daif;
    // asm volatile("mrs %0, daif" : "=r"(daif));
    // uart_printf(CONSOLE, "DAIF Register: 0x%llx\r\n", daif);
    // Not strictly necessary, since console is configured during boot
    uart_config_and_enable(CONSOLE);
    uart_config_and_enable(MARKLIN);

    // size_t sp = fetch_sp();

#if defined(MMU)
    setup_mmu();
#endif

    Context kernel_context;               // Initialize kernel context
    funcvoid_t bootstrap_task = IdleTask; // bootstrap task with first user task in user/usertask.h

    Kernel kernel(bootstrap_task); // bootstrap
    TaskDescriptor *current_task = nullptr;

    // uart_printf(CONSOLE, CLEAR_SCREEN RESET_FORMATTING MOVE_CURSOR SAVE_CURSOR, 1, 1);
    // uart_printf(CONSOLE, RESTORE_CURSOR "Kernel Started -- enabling interrupts\r\n" SAVE_CURSOR);

    uint32_t start_time, end_time = 0;
    uart_printf(CONSOLE, CLEAR_SCREEN RESET_FORMATTING HIDE_CURSOR COLUMN_132 MOVE_CURSOR SAVE_CURSOR, 1, 1);
    // for (int i = 0; i < 14; ++i)
    // {
    //     uart_printf(CONSOLE, "123456789-");
    // }
    // uart_printf(CONSOLE, "\r\n");
    // for (int i = 0; i < 14; ++i)
    // {
    //     uart_printf(CONSOLE, "%d" CHANGE_COLUMN, i * 10, (i * 10) + 1);
    // }

    // uart_printf(CONSOLE, "\r\n" CHANGE_COLUMN "HELLO WORLD\r\n", 200);
    // // while (1)
    // {
    // }
    UART_IMSC_ENABLE(MARKLIN, (CTS_INTERRUPT_MASK)); // enable receive timeout interrupt
    // scheduler pops the highest priority task into td
    while ((current_task = kernel.Scheduler()) != nullptr)
    {
        // uart_printf(CONSOLE, MOVE_CURSOR CLEAR_TO_END_LINE "ACTIVE TASK: tid{%d} priority{%d}\r\n", TRANSMITTED_BYTES_LOCATION + 1, 1, current_task->getTid(), current_task->getPriority());
        // kassert(false && "PANIC: Kernel loop");
        // uart_printf(CONSOLE, RESTORE_CURSOR "ACTIVE TASK: tid{%d} priority{%d}\r\n" SAVE_CURSOR, current_task->getTid(), current_task->getPriority());
        start_time = clock.Time();

        // enable_irq(); // enable interrupts
        int esr_el1 = kernel.DispatchTask(&kernel_context, current_task);
        // disable_irq(); // disable interrupts
        end_time = clock.Time();

        // Get the time the task was running for (including transfer times)
        uint32_t task_time = end_time - start_time;
        TOTAL_TIME += task_time;
        if (current_task->getPriority() == PRIORITY::IDLE)
        {
            TOTAL_IDLE_TIME += task_time;
        }

        uint32_t total_idle = (uint32_t)(((uint64_t)(TOTAL_IDLE_TIME) * 100) / TOTAL_TIME);

        // apply mask to ESR to get SVC number
        int N = esr_el1 & 0xFFFF;
        kassert(N >= 0 && "UNEXPECTED ERROR DECODING SVC NUMBER");
        kernel.Handler(N, total_idle);
        // IO_NS::Print(MOVE_CURSOR CLEAR_TO_END_LINE, IDLE_LOCATION, 1);
        // IO_NS::Print(MOVE_CURSOR COLOR_CYAN "IDLE: %d%%", IDLE_LOCATION, 1, (uint32_t)((TOTAL_IDLE_TIME * 100) / TOTAL_TIME));
    }

    kassert(false && "Kernel loop exited unexpectedly");
    return 0;
}
