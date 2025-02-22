#include "interrupts.h"
#include <stdint.h>
#include "../rpi.h"
#include "../shared_constants.h"
#include "constants.h"

static void enable_gic_distributor()
{
    D_REG(GICD_BASE, GICD_CTLR) = 0x3; // write to GICD_CTLR, enable group 0 and 1 interrupts
}

static void enable_gic_cpu_interface()
{
    D_REG(GICC_BASE, GICC_CTLR) = 0x3;
}

static void enable_peripheral_interrupt(int m)
{
    GICD_ISENABLERn(m) |= 1 << (m & 0x1F); // m & 0x1F -> m % 32
}

static void set_peripheral_target(int m)
{
    GICD_ITARGETSRn(m) = 0x1 << ((m & 0x3) << 3); //  -> (m % 4) * 8
}

void enable_irq()
{
    asm volatile("msr daifclr, #2");
    // uart_printf(CONSOLE, "IRQs enabled\r\n");
}

void disable_irq()
{
    asm volatile("msr daifset, #2");
}

void InitGIC()
{
    enable_gic_distributor();   // Enable the GIC distributor
    enable_gic_cpu_interface(); // Enable the GIC CPU interface

    set_peripheral_target(TIMER_C1);
    enable_peripheral_interrupt(TIMER_C1);

    set_peripheral_target(UART_IRQ);
    enable_peripheral_interrupt(UART_IRQ);

    enable_irq();
}
