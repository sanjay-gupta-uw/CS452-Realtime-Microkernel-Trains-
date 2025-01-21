#include "clock.h"
// #include "rpi.h"

#define TENTH_OF_SECOND_MICRO_SECONDS 100000

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

Clock::Clock()
{
   last_time = SYSTIMER_REG(SYSTIMER_CLO);
   minutes = 0;
   seconds = 0;
   tenths = 0;
   UPDATE_DISPLAY = true;
}

Clock::~Clock() {}

uint32_t Clock::Time() { return last_time; }

void Clock::Update()
{
   // Get current time from system timer
   uint32_t current_time = SYSTIMER_REG(SYSTIMER_CLO);

   // elapsed time since last update (in microseconds)
   uint32_t elapsed = current_time - last_time;

   // Convert elapsed time to tenths of a second and accumulate
   uint32_t increment = elapsed / TENTH_OF_SECOND_MICRO_SECONDS;
   if (increment > 0)
   {
      tenths += increment;

      // Update seconds if tenths exceed 10
      seconds += tenths / 10;
      tenths %= 10;

      // Update minutes if seconds exceed 60
      minutes += seconds / 60;
      seconds %= 60;

      // uart_printf(CONSOLE, "FILLD: minutes: %u, seconds: %u, tenths: %u\r\n", *minutes, *seconds, *tenths);
      last_time = current_time;

      UPDATE_DISPLAY = true;
   }
   // wraparound after 71 minutes?
}

void Clock::Delay(uint32_t delay_ms)
{
   uint32_t delay_microseconds = delay_ms * 1000;    // Convert ms to microseconds
   uint32_t start_time = SYSTIMER_REG(SYSTIMER_CLO); // Record the start time
   uint32_t elapsed = 0;

   while (elapsed < delay_microseconds)
   {
      // Calculate elapsed time
      uint32_t current_time = SYSTIMER_REG(SYSTIMER_CLO);
      elapsed = current_time - start_time;

      // Update the clock to ensure it doesn't lose ticks during the delay
      Update();
   }
}

void Clock::Display(int LOCATION)
{
   if (!UPDATE_DISPLAY)
      return;

   move_cursor(CONSOLE, 1, LOCATION);
   clear_to_end_line(CONSOLE);

   color_magenta();
   if (minutes < 10)
      uart_putc(CONSOLE, '0'); // Add leading zero for minutes
   uart_printf(CONSOLE, "%u:", minutes);

   if (seconds < 10)
      uart_putc(CONSOLE, '0'); // Add leading zero for seconds
   uart_printf(CONSOLE, "%u.", seconds);

   uart_printf(CONSOLE, "%u", tenths); // Tenths don't need padding

   UPDATE_DISPLAY = false;
}
