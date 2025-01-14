#include "sensor.h"
#include "rpi.h"

#define SENSOR_DEBUG 8

Sensor sensor_data[NUM_BANKS * SENSORS_PER_BANK];
static const char BANK_LABELS[NUM_BANKS] = {'A', 'B', 'C', 'D', 'E'};
static bool UPDATE_DISPLAY = true;

static void add_color()
{
   uart_puts(CONSOLE, "\033[33m"); // Set color to yellow
}

// static void send_command(int sensor_data, int address)
// {
//    uart_putc(MARKLIN, (uint8_t)sensor_data);
//    uart_putc(MARKLIN, (uint8_t)address);
//    clock_delay(200);
// }

void sensor_init(bool resetMode)
{
   // Initialize sensor metadata (bank and id) once
   for (int bank_idx = 0; bank_idx < NUM_BANKS; ++bank_idx)
   {
      for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor)
      {
         int idx = bank_idx * SENSORS_PER_BANK + sensor;

         sensor_data[idx].bank = BANK_LABELS[bank_idx];
         sensor_data[idx].id = sensor + 1;
         sensor_data[idx].status = SEN_OFF; // Default status to inactive
      }
   }
   sensor_reset(resetMode);
}

void sensor_read_all(int num_banks, RingBuffer *recent_sensors)
{
   if (num_banks > NUM_BANKS || num_banks < 0)
   {
      num_banks = NUM_BANKS;
   }

   uart_putc(MARKLIN, READ_ALL_SENSOR_BASE + num_banks);
   clock_delay(20);

   if (!uart_available(MARKLIN))
   {
      return;
   }

   for (int bank = 0; bank < num_banks; ++bank)
   {
      uint8_t byte1 = uart_getc(MARKLIN);
      uint8_t byte2 = uart_getc(MARKLIN);
      // move_cursor(CONSOLE, SENSOR_DEBUG + 1, 1);
      // clear_to_end_line(CONSOLE);
      // uart_printf(CONSOLE, "byte1: {%d}, byte2:{%d}\r\n", byte1, byte2);

      for (int sensor = 0; sensor < SENSORS_PER_BANK; ++sensor)
      {
         int status = (sensor < 8) ? ((byte1 & (1 << (7 - sensor))) ? SEN_ON : SEN_OFF) : ((byte2 & (1 << (7 - sensor))) ? SEN_ON : SEN_OFF);
         int idx = (bank * SENSORS_PER_BANK) + sensor;

         if (status == SEN_ON && sensor_data[idx].status == SEN_OFF)
         {
            if (is_buffer_full(recent_sensors)) // change to while since async?
            {
               remove_from_buffer(recent_sensors, NULL);
            }
            add_to_buffer(recent_sensors, &idx);
            UPDATE_DISPLAY = true;
         }
      }
   }
}

void sensor_reset(bool reset_on)
{
   uart_putc(MARKLIN, reset_on ? RESET_MODE_ON : RESET_MODE_OFF);
   clock_delay(20);
}

void init_sensor_display(int LOCATION)
{
   // Sensors
   move_cursor(CONSOLE, LOCATION, 1);
   clear_to_end_line(CONSOLE);
   add_color();

   uart_printf(CONSOLE, "Recent Sensor Changes:\r\n");
   uart_printf(CONSOLE, "Bank | Sensor | Status\r\n");
   uart_printf(CONSOLE, "----------------------\r\n");
}

void sensor_display(int LOCATION, RingBuffer *recent_sensors)
{
   if (!UPDATE_DISPLAY)
      return;

   if (is_buffer_empty(recent_sensors))
   {
      return;
   }
   int current_idx = (recent_sensors->head == 0) ? RING_BUFFER_MAX_SIZE - 1 : recent_sensors->head - 1;

   // Track the number of elements displayed
   int count = 0;

   move_cursor(CONSOLE, LOCATION, 1);
   add_color();
   // Traverse the buffer, moving towards the tail
   while (count < 10 && current_idx != recent_sensors->tail)
   {
      clear_to_end_line(CONSOLE);

      // Retrieve the data at the current index
      int index = recent_sensors->buffer[current_idx];

      // Print the sensor data
      uart_printf(CONSOLE, "   %c%2d   \r\n",
                  sensor_data[index].bank,
                  sensor_data[index].id);

      // Move to the previous element in the circular buffer
      current_idx = (current_idx == 0) ? RING_BUFFER_MAX_SIZE - 1 : current_idx - 1;

      // Increment the count of displayed elements
      count++;
   }
   UPDATE_DISPLAY = false;
}
