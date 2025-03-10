// #include <stdint.h>

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"
#include "train.h"
#include "switch.h"
#include "track_node.h"
#include "track_data.h"
#include "route.h"
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

	track_node track[TRACK_MAX];

	init_command_ring_buffer(&command_buffer);
	init_ring_buffer(&sensor_buffer);

	// Set up GPIO pins for both console and Marklin UARTs
	gpio_init();
	uart_config_and_enable(CONSOLE);
	uart_config_and_enable(MARKLIN);

	CommandType tmp_cmd = {INVALID_CMD, -1, -1};

	sensor_init(ENABLE_RESET_MODE); // enables reset mode
	set_all_switches(BRANCH);
	
	// Clock init
	clock_init();
	create_ui();

	// Initialize track and find nodes
    init_trackb(track);  // Initialize Track B
    
	bool pending_stop = false;
	int pending_stop_train = 0;
	bool pending_change_speed = false;
	int pending_change_speed_train = 0;
	int pending_change_speed_speed = 0;

	//track_node* start = find_node_by_name(track, "B15");
			
    //track_node* dest = find_node_by_name(track, "C11");
		
					/*
			//move_cursor(CONSOLE, PATH_FIND_DISPLAY_LOCATION + 1, 1);
			//uart_printf(CONSOLE, "Node found! Name: %d\n", start->num);
			//move_cursor(CONSOLE, PATH_FIND_DISPLAY_LOCATION + 2, 1);
			//uart_printf(CONSOLE, "Node found! Name: %d\n", dest->num);
            if(start && dest) {
                SwitchSetting switches_set[MAX_SWITCHES];
                int num_switches = 0;
                int total_distance = 0;
				

				switches_set[0].switch_num = 10;
				switches_set[1].switch_num = 11;
				switches_set[0].dir = SWITCH_STRAIGHT;
				switches_set[1].dir = SWITCH_STRAIGHT;
				num_switches = 2;
			

                set_switches(switches_set, num_switches);
                if(find_path(track, start, dest, switches_set, &num_switches, &total_distance)) {
					//move_cursor(CONSOLE, PATH_FIND_DISPLAY_LOCATION + 100, 1);
                    uart_printf(CONSOLE, "Initial path found! Distance: %dmm\r\n", total_distance);
                    set_switches(switches_set, num_switches);
                } else {
					//move_cursor(CONSOLE, PATH_FIND_DISPLAY_LOCATION + 100, 1);
                    uart_printf(CONSOLE, "Initial path not found\r\n");
                }
				
            }
					*/

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

		if (!is_command_buffer_empty(&command_buffer))
		{
			CommandType cmd;
			int val = remove_command_from_buffer(&command_buffer, &cmd);
			if (cmd.cmd_type == QUIT_CMD)
			{
				return 0;
			}
			else if (cmd.cmd_type == STOP_CMD)
            {
                pending_stop = true;
                pending_stop_train = cmd.addr;
                uart_printf(CONSOLE, "Train %d will stop at next sensor.\r\n", cmd.addr);
            }
			else if (cmd.cmd_type == CHANGE_SPEED_CMD)
            {
                pending_change_speed = true;
                pending_change_speed_train = cmd.addr;
				pending_change_speed_speed = cmd.param;

                uart_printf(CONSOLE, "Train %d will change speed to %d at next sensor.\r\n", cmd.addr, cmd.param);
            }
			else if (cmd.cmd_type == ACCURATE_STOP_CMD)
            {
      			track_node* start_node = &track[cmd.start];
      			track_node* dest_node = &track[cmd.dest];
      			SwitchSetting switches_set[MAX_SWITCHES];
      			int num_switches = 0;
      			int total_distance = 0;
      			if (find_path_BFS(track, start_node, dest_node, switches_set, &num_switches, &total_distance)) {
         			set_switches(switches_set, num_switches);
         			uart_printf(CONSOLE, "Initial path found! Distance: %dmm\r\n", total_distance);
      			}
				else
				{
      				uart_printf(CONSOLE, "Path not found.\r\n");
				}
            }
            else if (val)
            {
                parse_command(&cmd);
            }
		}
		else
		{
			int prev_sensor_count = sensor_buffer.size;
			sensor_read_all(NUM_BANKS, &sensor_buffer);
		
			if (pending_stop && sensor_buffer.size > prev_sensor_count) {
				actual_stop(pending_stop_train);
				pending_stop = false;
			}
			if (pending_change_speed && sensor_buffer.size > prev_sensor_count) {
				actual_change_speed(pending_change_speed_train, pending_change_speed_speed);
				pending_change_speed = false;
			}
		}
	}
}
