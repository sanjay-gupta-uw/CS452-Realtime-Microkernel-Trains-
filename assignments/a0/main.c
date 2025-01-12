// #include <stdint.h>
#include <stdbool.h>

#include "rpi.h"
#include "clock.h"
#include "train.h"
#include "switch.h"

int kmain()
{
	// Clock init
	Clock sys_clock;
	clock_init(&sys_clock);

	// Set up GPIO pins for both console and Marklin UARTs
	gpio_init();
	// Not strictly necessary, since console is configured during boot
	uart_config_and_enable(CONSOLE);
	uart_config_and_enable(MARKLIN);

	// Welcome message (displayed below the existing boot log)
	uart_puts(CONSOLE, "\r\nHello world, this is version: " __DATE__ " / " __TIME__ "\r\n");
	uart_puts(CONSOLE, "Press 'q' to reboot\r\n");

	uart_puts(CONSOLE, "\033[s"); // Save the current cursor position for the sys_clock

	bool enableClock = true;
	bool trainEnabled = false;
	bool switchTestEnabled = true;

	// main polling loop
	for (;;)
	{
		// Update the sys_clock
		if (enableClock)
		{
			clock_update(&sys_clock);
			uart_puts(CONSOLE, "\033[u");
			uart_puts(CONSOLE, "\033[K"); // clear the line
			clock_display(&sys_clock);
			uart_puts(CONSOLE, "\r");
		}

		if (!trainEnabled)
		{
			accelerate_train(55, 10, true);
			// clock_delay(&sys_clock, 30);
			trainEnabled = true;
		}

		if (switchTestEnabled)
		{
			// switch_straight(4);
			bool isStraight = false;
			set_all_switches(isStraight);
			switchTestEnabled = false;
		}
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
