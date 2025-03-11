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

typedef enum {
    STOP_IDLE,
    WAITING_FOR_SENSOR,
    WAITING_FOR_DELAY
} StopState;

typedef struct {
    StopState state;
    int train_num;
    track_node* dest_node;
    int offset;
    int speed_setting;
    uint32_t trigger_time;  // In microseconds
    uint32_t delay_micros;
} PendingAccurateStop;

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

	create_loop();
    
	bool pending_stop = false;
	int pending_stop_train = 0;
	bool pending_change_speed = false;
	int pending_change_speed_train = 0;
	int pending_change_speed_speed = 0;

	PendingAccurateStop accurate_stop_pending_stop = {STOP_IDLE};

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
				
				//uart_printf(CONSOLE, "1. Jack Ma testing train num: %d. offset: %d.\r\n", cmd.train_num, cmd.offset);
				if (accurate_stop_pending_stop.state != STOP_IDLE) {
					uart_printf(CONSOLE, "Another accurate stop is processing.\r\n");
					continue;
				} 
				if (is_valid_train(cmd.train_num)) {
					//uart_printf(CONSOLE, "2. Jack Ma testing train nun: %d...\r\n", cmd.train_num);
					accurate_stop_pending_stop.state = WAITING_FOR_SENSOR;
					accurate_stop_pending_stop.train_num = cmd.train_num;
					accurate_stop_pending_stop.dest_node = &track[cmd.dest];
					accurate_stop_pending_stop.offset = cmd.offset;
					accurate_stop_pending_stop.speed_setting = cmd.speed;
					uart_printf(CONSOLE, "Awaiting sensor trigger for train %d...\r\n", cmd.train_num);
				} else {
					uart_printf(CONSOLE, "Invalid train number.\r\n");
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

			// Process new sensor events
			if (sensor_buffer.size > prev_sensor_count) {
				SensorEvent event;
				remove_from_buffer(&sensor_buffer, &event);

				if (accurate_stop_pending_stop.state == WAITING_FOR_SENSOR) {
					// Get sensor's node
					track_node* start_node = find_node_by_sensor_index(track, event.sensor_idx);
					if (!start_node) continue;
		
					// Find path to destination
					SwitchSetting switches_set[MAX_SWITCHES];
					int num_switches, total_distance;
					bool found = find_path_BFS(track, start_node, accurate_stop_pending_stop.dest_node, 
										 switches_set, &num_switches, &total_distance);
					if (!found) {
						uart_printf(CONSOLE, "No path found.\r\n");
						accurate_stop_pending_stop.state = STOP_IDLE;
						continue;
					}
					uart_printf(CONSOLE, "Path found with a total distance of %d mm.\r\n", total_distance);
		
					// Set switches
					set_switches(switches_set, num_switches);
		
					// Get calibration data
					SpeedCalibration cal;
					bool found_cal = false;
					for (size_t i = 0; i < sizeof(calibration)/sizeof(calibration[0]); i++) {
						if (calibration[i].speed_setting == accurate_stop_pending_stop.speed_setting) {
							cal = calibration[i];
							found_cal = true;
							continue;
						}
					}
					if (!found_cal) {
						uart_printf(CONSOLE, "No calibration for speed %d.\r\n", accurate_stop_pending_stop.speed_setting);
						accurate_stop_pending_stop.state = STOP_IDLE;
						continue;
					}
					// Calculate adjusted distance and delay
					int adjusted_distance = total_distance*1000 + (accurate_stop_pending_stop.offset)*1000 - cal.stopping_distance_mm;
					// uart_printf(CONSOLE, "total_distance: %d, offset: %d, stopping_distance: %d.\r\n", 
					//	total_distance*1000, (accurate_stop_pending_stop.offset)*1000, cal.stopping_distance_mm);
					if (adjusted_distance <= 0) {
						uart_printf(CONSOLE, "Stopping distance too large.\r\n");
						accurate_stop_pending_stop.state = WAITING_FOR_SENSOR;
						continue;
					}
					uart_printf(CONSOLE, "Distance to stop: %d mm.\r\n", 
						adjusted_distance / 1000);
		
					accurate_stop_pending_stop.delay_micros = (adjusted_distance / cal.actual_speed_mmps) * 10;
					clock_delay(accurate_stop_pending_stop.delay_micros);
					//uart_printf(CONSOLE, "Adjusted_distance: %d, Actual_speed: %d, delay_micros: %d.\r\n", 
					//	adjusted_distance, cal.actual_speed_mmps, accurate_stop_pending_stop.delay_micros);
					accurate_stop_pending_stop.trigger_time = get_current_time();
					accurate_stop_pending_stop.state = WAITING_FOR_DELAY;
					
					uart_printf(CONSOLE, "Stopping train %d in %d ms.\r\n", 
						accurate_stop_pending_stop.train_num, accurate_stop_pending_stop.delay_micros);
				} 
				if (pending_stop) {
					actual_stop(pending_stop_train);
					pending_stop = false;
				}
				if (pending_change_speed) {
					actual_change_speed(pending_change_speed_train, pending_change_speed_speed);
					pending_change_speed = false;
				}
			}
			else if (accurate_stop_pending_stop.state == WAITING_FOR_DELAY) 
			{
				uint32_t current_time = get_current_time();
				if (current_time - accurate_stop_pending_stop.trigger_time >= accurate_stop_pending_stop.delay_micros) {
					stop_train(accurate_stop_pending_stop.train_num);
					uart_printf(CONSOLE, "Train %d stopped at target.\r\n", accurate_stop_pending_stop.train_num);
					accurate_stop_pending_stop.state = STOP_IDLE;
					continue;
				}
			}
		}
	}
}
