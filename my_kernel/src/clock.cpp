#include "clock.h"
#include <assert.h>
#include "shared_constants.h"
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
static const uint32_t SYSTIMER_C1 = 0x10; // System Timer Compare 1
// static const uint32_t SYSTIMER_C2 = 0x14;	 // System Timer Compare 2
static const uint32_t SYSTIMER_C3 = 0x18; // System Timer Compare 3

// CS register masks
static const uint32_t SYSTIMER_CS_M0 = 0x01; // Match 0
static const uint32_t SYSTIMER_CS_M1 = 0x02; // Match 1
static const uint32_t SYSTIMER_CS_M2 = 0x03; // Match 2
static const uint32_t SYSTIMER_CS_M3 = 0x04; // Match 3

// CLO register masks

/*********** ************************************************ ************/

Clock::Clock()
{
    TICKS = 0;
    last_time = SYSTIMER_REG(SYSTIMER_CLO);
    minutes = 0;
    seconds = 0;
    tenths = 0;
    UPDATE_DISPLAY = true;
}

Clock::~Clock() {}

uint32_t Clock::Time() { return SYSTIMER_REG(SYSTIMER_CLO); }

void Clock::Update()
{
    // Get current time from system timer
    uint32_t current_time = SYSTIMER_REG(SYSTIMER_CLO);
    // IO_NS::PrintTerminal("Clock::Update() - current_time: %u, last_time: %u, diff: %u\r\n", current_time, last_time, current_time - last_time);

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

void Clock::Display()
{
    Update();

    if (!UPDATE_DISPLAY)
    {
        // IO_NS::PrintTerminal("Clock::Display() - No update needed\r\n");
        return;
    }

    char min[3] = {'0', '0', '\0'};
    if (minutes < 10)
    {
        min[1] = minutes + '0';
    }
    else
    {
        min[0] = minutes / 10 + '0';
        min[1] = minutes % 10 + '0';
    }

    char sec[3] = {'0', '0', '\0'};
    if (seconds < 10)
    {
        sec[1] = seconds + '0';
    }
    else
    {
        sec[0] = seconds / 10 + '0';
        sec[1] = seconds % 10 + '0';
    }

    // IO_NS::PrintTerminal("Clock::Display()\r\n");

    IO_NS::Print(MOVE_CURSOR COLOR_YELLOW "%s:%s:%u", CLOCK_LOCATION_Y, CLOCK_LOCATION_X, min, sec, tenths);
    UPDATE_DISPLAY = false;
}

// CMP_REG should be 1/3
void Clock::ReArmTimer(uint32_t delay_interval)
{
    uint32_t next_match = SYSTIMER_REG(SYSTIMER_CLO) + delay_interval;
    SYSTIMER_REG(SYSTIMER_C1) = next_match;
    // ++TICKS;
}

void Clock::DisarmTimer()
{
    SYSTIMER_REG(SYSTIMER_CS) = (1 << 1); // clear match bit for C1 (2nd bit)
}
