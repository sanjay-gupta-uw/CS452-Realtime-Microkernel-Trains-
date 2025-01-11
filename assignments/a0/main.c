#include "rpi.h"
#include "clock.h"
#include <stdint.h>

int kmain()
{
	// Set up GPIO pins for both console and Marklin UARTs
	gpio_init();
	// Not strictly necessary, since console is configured during boot
	uart_config_and_enable(CONSOLE);

	// Welcome message (displayed below the existing boot log)
	uart_puts(CONSOLE, "\r\nHello world, this is version: " __DATE__ " / " __TIME__ "\r\n");
	uart_puts(CONSOLE, "Press 'q' to reboot\r\n");

	uart_puts(CONSOLE, "\033[s"); // Save the current cursor position for the clock

	// Clock init
	Clock clock;
	clock_init(&clock);

	// main polling loop
	for (;;)
	{
		// Update the clock
		clock_update(&clock);

		uart_puts(CONSOLE, "\033[u");
		uart_puts(CONSOLE, "\033[K"); // clear the line
		clock_display(&clock);
		uart_puts(CONSOLE, "\r");
	}

	// from IOtest
	for (;;)
	{
		if (uart_getc(CONSOLE) == 'q')
		{
			uart_puts(CONSOLE, "\r\nExiting...\r\n");
			return 0;
		}
	}
}
