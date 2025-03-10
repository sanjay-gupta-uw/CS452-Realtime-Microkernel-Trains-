#include "sensor.h"
#include "rpi.h"
#include "clock.h"

#define SENSOR_DISPLAY_LOCATION 13
#define MAX_DISPLAY_ITEMS 10

Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK];
static const char BANK_LABELS[NUM_BANKS] = {'A', 'B', 'C', 'D', 'E'};
static bool UPDATE_DISPLAY = false;

// Track last triggered sensor globally
static char last_triggered_bank = '\0';
static uint8_t last_triggered_id = 0;

static void set_display_color() {
    uart_puts(CONSOLE, "\033[33m");  // Yellow color
}

static void clear_display_line() {
    uart_puts(CONSOLE, "\033[K");  // Clear line
}

void sensor_init(bool resetMode) {
    last_triggered_bank = '\0';
    last_triggered_id = 0;
    
    // Initialize all sensors to OFF state
    for (int bank = 0; bank < NUM_BANKS; ++bank) {
        for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor) {
            int idx = bank * SENSORS_PER_BANK + sensor;
            sensor_data[idx] = (Sensor){
                .bank = BANK_LABELS[bank],
                .id = sensor + 1,
                .status = SEN_OFF
            };
        }
    }
    sensor_reset(resetMode);
}

void sensor_read_all(int num_banks, RingBuffer *recent_sensors) {
    if (num_banks < 1 || num_banks > NUM_BANKS) num_banks = NUM_BANKS;
    
    // Request sensor data from controller
    uart_putc(MARKLIN, READ_ALL_SENSOR_BASE + num_banks);
    clock_delay(20);  // Short delay for response

    if (!uart_available(MARKLIN)) return;

    // Process each bank's data
    for (int bank = 0; bank < num_banks; ++bank) {
        uint8_t byte1 = uart_getc(MARKLIN);
        uint8_t byte2 = uart_getc(MARKLIN);

        // Process individual sensors in this bank
        for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor) {
            const int idx = bank * SENSORS_PER_BANK + sensor;
            const SensorState new_state = (sensor < 8) ? 
                ((byte1 >> (7 - sensor)) & 1) : 
                ((byte2 >> (7 - (sensor - 8))) & 1);

            const SensorState old_state = sensor_data[idx].status;
            sensor_data[idx].status = new_state;

            // Detect activation (OFF -> ON transition)
            if (new_state == SEN_ON && old_state == SEN_OFF) {
                // Check against last triggered to avoid duplicates
                if (sensor_data[idx].bank != last_triggered_bank || 
                    sensor_data[idx].id != last_triggered_id) {
                    
                    // Create sensor event with timestamp
                    SensorEvent event;
                    event.sensor_idx = idx;
                    event.minutes = sys_clock.minutes;
                    event.seconds = sys_clock.seconds;
                    event.tenths = sys_clock.tenths;

                    // Manage buffer space
                    if (is_buffer_full(recent_sensors)) {
                        remove_from_buffer(recent_sensors, NULL);
                    }
                    add_to_buffer(recent_sensors, &event);
                    
                    // Update last triggered
                    last_triggered_bank = sensor_data[idx].bank;
                    last_triggered_id = sensor_data[idx].id;
                    UPDATE_DISPLAY = true;
                }
            }
        }
    }
}

void sensor_reset(bool reset_on) {
    uart_putc(MARKLIN, reset_on ? RESET_MODE_ON : RESET_MODE_OFF);
    clock_delay(20);  // Delay for command processing
}

void init_sensor_display(int location) {
    move_cursor(CONSOLE, location, 1);
    set_display_color();
    uart_puts(CONSOLE, 
        "Recent Sensor Triggers (Time | Sensor):\r\n"
        "---------------------------------------\r\n"
    );
}

void sensor_display(int location, RingBuffer *recent_sensors) {
    if (!UPDATE_DISPLAY || is_buffer_empty(recent_sensors)) return;

    const int total_items = recent_sensors->size < MAX_DISPLAY_ITEMS 
                          ? recent_sensors->size 
                          : MAX_DISPLAY_ITEMS;

    for (int i = 0; i < MAX_DISPLAY_ITEMS; i++) {
        move_cursor(CONSOLE, location + 2 + i, 1);
        clear_display_line();
        set_display_color();

        if (i < total_items) {
            int logical_idx = recent_sensors->size - 1 - i;
            SensorEvent event;
            
            if (get_from_buffer(recent_sensors, logical_idx, &event)) {
                // Format: [MM:SS.T] SensorID
                uart_printf(CONSOLE, "%2d. [%02u:%02u.%u] %c%02d\r\n", 
                          i + 1,
                          event.minutes,
                          event.seconds,
                          event.tenths,
                          sensor_data[event.sensor_idx].bank,
                          sensor_data[event.sensor_idx].id);
            }
        }
    }

    UPDATE_DISPLAY = false;
}
