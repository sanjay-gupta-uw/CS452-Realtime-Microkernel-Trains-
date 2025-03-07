/*
#include "idle_task.h"
#include "../../include/syscall.h"
#include "../shared_constants.h" // Ensure you have a global access to system time and other constants
#include <cstdint>

// Global variables to track the CPU usage
volatile uint32_t idleTime = 0;
volatile uint32_t lastTimeStamp = 0;
volatile uint32_t totalTime = 0;

void updateCpuUsage()
{
    uint32_t currentTime = SYSTIMER_REG(SYSTIMER_CLO); // Fetch current system time
    uint32_t elapsedTime = currentTime - lastTimeStamp;
    totalTime += elapsedTime;
    lastTimeStamp = currentTime;

    // Calculate CPU idle time percentage
    if (totalTime > 0)
    {
        double idlePercentage = ((double)idleTime / totalTime) * 100.0;
        // uart_printf(CONSOLE, "CPU Idle Time: %.2f%%\n", idlePercentage);
    }

    // Reset the counters every so often to avoid overflow and keep figures recent
    if (totalTime > 10000000)
    { // Example threshold
        totalTime = 0;
        idleTime = 0;
    }
}

void IdleTask()
{
    lastTimeStamp = SYSTIMER_REG(SYSTIMER_CLO); // Initialize the last time stamp

    while (true)
    {
        uint32_t startIdle = SYSTIMER_REG(SYSTIMER_CLO);
        YIELD(); // Yield to the scheduler, indicating idle status

        // Perform low-power halt until the next interrupt
        __asm volatile("wfi"); // ARMv8 Wait For Interrupt instruction

        uint32_t endIdle = SYSTIMER_REG(SYSTIMER_CLO);
        idleTime += (endIdle - startIdle); // Update idle time spent

        updateCpuUsage(); // Update and print CPU usage
    }
}
*/