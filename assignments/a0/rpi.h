#ifndef _rpi_h_
#define _rpi_h_ 1

#include <stddef.h>

#define CONSOLE 1
#define MARKLIN 2

void gpio_init();                                          // initialize GPIO pins
void uart_config_and_enable(size_t line);                  // configure and enable UART line
unsigned char uart_getc(size_t line);                      // read a character from UART line
void uart_putc(size_t line, char c);                       // write a character to UART line
void uart_putl(size_t line, const char *buf, size_t blen); // write a buffer to UART line
void uart_puts(size_t line, const char *buf);              // write a string to UART line
void uart_printf(size_t line, const char *fmt, ...);       // write a formatted string to UART line

#endif /* rpi.h */
