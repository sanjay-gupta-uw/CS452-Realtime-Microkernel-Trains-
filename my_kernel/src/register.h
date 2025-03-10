#ifndef _register_h_
#define _register_h_

#include "kern/constants.h"

#define CTS_STATUS(line) (UART_REG(line, UART_FR) & UART_FR_CTS_MASK)
#define IS_TX_FIFO_FULL(line) !!(UART_REG(line, UART_FR) & UART_FR_TXFF_MASK)
#define TX_STATUS(line) !!(UART_REG(line, UART_FR) & UART_FR_TXFE_MASK)

#endif