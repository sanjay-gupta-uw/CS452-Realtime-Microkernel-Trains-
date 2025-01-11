#include "clock.h"
#include "rpi.h"

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

void clock_init(Clock *clock)
{
   clock->last_time = SYSTIMER_REG(SYSTIMER_CLO);
   clock->minutes = 0;
   clock->seconds = 0;
   clock->tenths = 0;
}

void clock_update(Clock *clock)
{
   // uart_printf(CONSOLE, "INIT: minutes: %u, seconds: %u, tenths: %u\r\n", *minutes, *seconds, *tenths);
   // Get current time from system timer
   uint32_t current_time = SYSTIMER_REG(SYSTIMER_CLO);

   // elapsed time since last update (in microseconds)
   uint32_t elapsed = current_time - clock->last_time;
   // uart_printf(CONSOLE, "current time: %u\r\n", current_time);
   // uart_printf(CONSOLE, "elapsed time: %u\r\n", elapsed);
   // uart_printf(CONSOLE, "minutes: %u, seconds: %u", minutes, seconds);

   // Convert elapsed time to tenths of a second and accumulate
   uint32_t increment = elapsed / TENTH_OF_SECOND_MICRO_SECONDS;
   if (increment > 0)
   {
      clock->tenths += elapsed / TENTH_OF_SECOND_MICRO_SECONDS;

      // Update seconds if tenths exceed 10
      clock->seconds += clock->tenths / 10;
      clock->tenths %= 10;

      // Update minutes if seconds exceed 60
      clock->minutes += clock->seconds / 60;
      clock->seconds %= 60;

      // uart_printf(CONSOLE, "FILLD: minutes: %u, seconds: %u, tenths: %u\r\n", *minutes, *seconds, *tenths);
      clock->last_time = current_time;
   }
   // wraparound after 71 minutes?
}

void clock_display(const Clock *clock)
{
   uart_printf(CONSOLE, "%u:%u.%u", clock->minutes, clock->seconds, clock->tenths);
}