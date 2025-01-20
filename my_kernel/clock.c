#include "clock.h"
#include "rpi.h"

#define TENTH_OF_SECOND_MICRO_SECONDS 100000

static bool UPDATE_DISPLAY = true;

/*********** SYSTEM TIMER CONTROL ************************ ************/
static char *const SYSTIMER_BASE = (char *)(0xFE003000);
#define SYSTIMER_REG(offset) (*(volatile uint32_t *)(SYSTIMER_BASE + offset))

// System Timer register offsets
static const uint32_t SYSTIMER_CS = 0x00;  // System Timer Control/Status
static const uint32_t SYSTIMER_CLO = 0x04; // System Timer Counter Lower 32 bits
static const uint32_t SYSTIMER_CHI = 0x08; // System Timer Counter Higher 32 bits
// static const uint32_t SYSTIMER_C0 = 0x0C;	 // System Timer Compare 0
// static const uint32_t SYSTIMER_C1 = 0x10;	 // System Timer Compare 1
// static const uint32_t SYSTIMER_C2 = 0x14;	 // System Timer Compare 2
// static const uint32_t SYSTIMER_C3 = 0x18;	 // System Timer Compare 3

// CS register masks
static const uint32_t SYSTIMER_CS_M0 = 0x01; // Match 0
static const uint32_t SYSTIMER_CS_M1 = 0x02; // Match 1
static const uint32_t SYSTIMER_CS_M2 = 0x03; // Match 2
static const uint32_t SYSTIMER_CS_M3 = 0x04; // Match 3

// CLO register masks

/*********** ************************************************ ************/
Clock sys_clock; // Define the global variable

void clock_init()
{
   sys_clock.last_time = SYSTIMER_REG(SYSTIMER_CLO);
   sys_clock.minutes = 0;
   sys_clock.seconds = 0;
   sys_clock.tenths = 0;
}

uint32_t get_current_time()
{
   return SYSTIMER_REG(SYSTIMER_CLO);
}

void clock_update()
{

   // Get current time from system timer
   uint32_t current_time = SYSTIMER_REG(SYSTIMER_CLO);

   // elapsed time since last update (in microseconds)
   uint32_t elapsed = current_time - sys_clock.last_time;

   // Convert elapsed time to tenths of a second and accumulate
   uint32_t increment = elapsed / TENTH_OF_SECOND_MICRO_SECONDS;
   if (increment > 0)
   {
      sys_clock.tenths += increment;

      // Update seconds if tenths exceed 10
      sys_clock.seconds += sys_clock.tenths / 10;
      sys_clock.tenths %= 10;

      // Update minutes if seconds exceed 60
      sys_clock.minutes += sys_clock.seconds / 60;
      sys_clock.seconds %= 60;

      // uart_printf(CONSOLE, "FILLD: minutes: %u, seconds: %u, tenths: %u\r\n", *minutes, *seconds, *tenths);
      sys_clock.last_time = current_time;

      UPDATE_DISPLAY = true;
   }
   // wraparound after 71 minutes?
}

void clock_delay(uint32_t delay_ms)
{
   uint32_t delay_microseconds = delay_ms * 1000;    // Convert ms to microseconds
   uint32_t start_time = SYSTIMER_REG(SYSTIMER_CLO); // Record the start time
   uint32_t elapsed = 0;

   while (elapsed < delay_microseconds)
   {
      // Calculate elapsed time
      uint32_t current_time = SYSTIMER_REG(SYSTIMER_CLO);
      elapsed = current_time - start_time;

      // Update the sys_clock to ensure it doesn't lose ticks during the delay
      clock_update();
   }
}

void clock_display(int LOCATION)
{
   if (!UPDATE_DISPLAY)
      return;

   move_cursor(CONSOLE, 1, LOCATION);
   clear_to_end_line(CONSOLE);

   uart_puts(CONSOLE, "\033[36m"); // set colour to magenta
   // uart_printf(CONSOLE, "%02u:%02u.%u", sys_clock.minutes, sys_clock.seconds, sys_clock.tenths);
   // Format the clock display manually with zero-padding
   if (sys_clock.minutes < 10)
      uart_putc(CONSOLE, '0'); // Add leading zero for minutes
   uart_printf(CONSOLE, "%u:", sys_clock.minutes);

   if (sys_clock.seconds < 10)
      uart_putc(CONSOLE, '0'); // Add leading zero for seconds
   uart_printf(CONSOLE, "%u.", sys_clock.seconds);

   uart_printf(CONSOLE, "%u", sys_clock.tenths); // Tenths don't need padding

   UPDATE_DISPLAY = false;
}
