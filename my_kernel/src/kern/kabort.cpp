#include "kabort.h"
#include "../rpi.h"
#include "../clock.h"
#include "constants.h"

extern Clock clock;

// kernel abort
void kabort(const char *condition, int line, const char *file, int EL)
{
    // disable interrupts
    clock.DisarmTimer();

    UART_IMSC_DISABLE(CONSOLE, (TX_INTERRUPT_MASK | RX_INTERRUPT_MASK | RTM_INTERRUPT_MASK));
    UART_CLEAR_INTERRUPT(CONSOLE, (TX_INTERRUPT_MASK | RX_INTERRUPT_MASK | RTM_INTERRUPT_MASK));

    UART_IMSC_DISABLE(MARKLIN, (TX_INTERRUPT_MASK | RX_INTERRUPT_MASK));
    UART_CLEAR_INTERRUPT(MARKLIN, (TX_INTERRUPT_MASK | RX_INTERRUPT_MASK | CTS_INTERRUPT_MASK));

    // print condition
    // IO_NS::PrintTerminal(COLOR_GREEN "ASSERTION FAILURE {%s} (in EL%d): line %d, file %s\r\n", condition, EL, line, file);
    // IO_NS::PrintTerminal(COLOR_RED "KERNEL PANIC: HALTING SYSTEM\r\n");
    uart_printf(CONSOLE, RESTORE_CURSOR "ASSERTION FAILURE {%s} (in EL%d): line %d, file %s\r\n" SAVE_CURSOR, condition, EL, line, file);
    uart_printf(CONSOLE, RESTORE_CURSOR "KERNEL PANIC: HALTING SYSTEM\r\n" SAVE_CURSOR);
    for (;;)
    {
        asm volatile("wfi"); // low power mode
    }
}