#ifndef _constants_h_
#define _constants_h_

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

#endif