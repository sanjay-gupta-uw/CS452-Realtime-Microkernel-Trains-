// #include <stdint.h>
#include <stdbool.h>

#include "rpi.h"
#include "clock.h"
#include "train.h"
#include "switch.h"
#include "ui.h"

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

	// main polling loop
	for (;;)
	{
		// update the clock
		clock_update(&sys_clock);

		draw_ui();
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
