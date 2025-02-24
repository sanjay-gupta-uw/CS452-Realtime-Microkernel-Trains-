#ifndef _constants_h_
#define _constants_h_
#include <cstddef>
// *************************** INTERRUPT OFFSETS ***************************
// GICD
#define GICD_CTLR 0x0000
#define GICD_ISENABLER 0x0100
#define GICD_ITARGETSR 0x0800

// GICC
#define GICC_CTLR 0x0000
#define GICC_IAR 0x000C
#define GICC_EOIR 0x0010

// *************************** REGISTER CONSTANTS ***************************
#define MMIO_BASE 0xFE000000

#define GIC_BASE (MMIO_BASE + 0x1840000)
#define GICD_BASE (GIC_BASE + 0x1000)
#define GICC_BASE (GIC_BASE + 0x2000)

#define GICD_ISENABLERn(id) (*(volatile uint32_t *)(GICD_BASE + GICD_ISENABLER + (4 * (id >> 5))))
#define GICD_ITARGETSRn(id) (*(volatile uint32_t *)(GICD_BASE + GICD_ITARGETSR + (4 * (id >> 2))))

#define D_REG(base, offset) (*(volatile uint32_t *)((base) + (offset)))

// *************************** TIMER CONSTANTS ***************************
#define TEN_MS 10000
#define SYS_TMR_BASE (MMIO_BASE + 0x3000)
#define TIMER_C1 97

// *************************** UART CONSTANTS ***************************
#define UART_IRQ 153

#define UART_IMSC 0x38
#define UART_MIS 0x40
#define UART_ICR 0x44

#define RX_INTERRUPT_MASK 0x10
#define TX_INTERRUPT_MASK 0x20
#define RTM_INTERRUPT_MASK 0x40 // receive timeout
#define CTS_INTERRUPT_MASK 0x02

static char *const UART0_BASE = (char *)(MMIO_BASE + 0x201000);
static char *const UART3_BASE = (char *)(MMIO_BASE + 0x201600);

static char *const line_uarts[] = {NULL, UART0_BASE, UART3_BASE};
#define UART_REG(line, offset) (*(volatile uint32_t *)(line_uarts[line] + offset))

// mask is 1 at specific bit, 0 at other bits
#define UART_IMSC_ENABLE(line, mask) (UART_REG(line, UART_IMSC) &= ~mask)   // enable should set specific bit to 0
#define UART_IMSC_DISABLE(line, mask) (UART_REG(line, UART_IMSC) |= mask)   // disable should set specific bit to 1
#define UART_CLEAR_INTERRUPT(line, mask) (UART_REG(line, UART_ICR) |= mask) // clear should set specific bit to 1

// SPURIOS INTERRUPT
#define SPURIOUS_INTERRUPT 1023

#endif