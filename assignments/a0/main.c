// #include <stdint.h>

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"
#include "train.h"
#include "switch.h"
#include "sensor.h"
#include "ui.h"
#include "command_buffer.h"
#include "command.h"

#define ENABLE_RESET_MODE true
#define DISABLE_RESET_MODE false
#define BRANCH false
#define STRAIGHT true

#define DEBUG 2

int kmain()
{
	CommandRingBuffer command_buffer;
	RingBuffer sensor_buffer;

	init_command_ring_buffer(&command_buffer);
	init_ring_buffer(&sensor_buffer);

	// Set up GPIO pins for both console and Marklin UARTs
	gpio_init();
	// Not strictly necessary, since console is configured during boot
	uart_config_and_enable(CONSOLE);
	uart_config_and_enable(MARKLIN);

	// move_cursor(CONSOLE, DEBUG, 1);
	// clear_to_end_line(CONSOLE);
	// uart_printf(CONSOLE, "CB Head:{%u}, T{%u} \r\n", command_buffer.head, command_buffer.tail);

	CommandType tmp_cmd = {INVALID_CMD, -1, -1};

	sensor_init(ENABLE_RESET_MODE); // enables reset mode
	set_all_switches(BRANCH);
	// Clock init
	clock_init();
	create_ui();

	// main polling loop
	for (;;)
	{
		// update the clock
		clock_update();
		update_ui(&sensor_buffer);

		int status = handle_command(&tmp_cmd);
		if (status == SUCCESS_CMD)
		{
			// create function for this!
			if (!is_command_buffer_full(&command_buffer))
			{
				add_command_to_buffer(&command_buffer, &tmp_cmd);
			}
		}

		if (!is_command_buffer_empty(&command_buffer)) // THIS LINE
		{
			CommandType cmd;
			int val = remove_command_from_buffer(&command_buffer, &cmd);
			if (cmd.cmd_type == QUIT_CMD)
			{
				return 0;
			}

			if (val)
			{
				parse_command(&cmd);
			}
		}
		else
		{
			// uint32_t time_start = get_current_time();

			sensor_read_all(NUM_BANKS, &sensor_buffer); // keep this

			/*
			uint32_t end_time = get_current_time();
			move_cursor(CONSOLE, 70, 1);
			clear_to_end_line(CONSOLE);
			uart_printf(CONSOLE, "Latency: {%d} ", (end_time - time_start));
			*/
		}
	}
}
