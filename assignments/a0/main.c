#include "rpi.h"
#include "clock.h"
#include <stdint.h>

int kmain()
{
	// set up GPIO pins for both console and marklin uarts
	gpio_init();
	// not strictly necessary, since console is configured during boot
	uart_config_and_enable(CONSOLE);
	// welcome message
	uart_puts(CONSOLE, "\r\nHello world, this is version: " __DATE__ " / " __TIME__ "\r\n\r\nPress 'q' to reboot\r\n");

	// clock init
	uint32_t last_time, elapsed_tenths, minutes, seconds, tenths = 0;

	unsigned int counter = 1;
	for (;;)
	{
		update_clock(&last_time, &elapsed_tenths, &minutes, &seconds, &tenths);
		uart_printf(CONSOLE, "Clock: %02u:%02u.%u\r\n", minutes, seconds, tenths);
		uart_printf(CONSOLE, "PI[%u]> ", counter++);
		for (;;)
		{
			char c = uart_getc(CONSOLE);
			uart_putc(CONSOLE, c);
			if (c == '\r')
			{
				uart_putc(CONSOLE, '\n');
				break;
			}
			else if (c == 'q' || c == 'Q')
			{
				uart_puts(CONSOLE, "\r\n");
				return 0;
			}
		}
	}
}
