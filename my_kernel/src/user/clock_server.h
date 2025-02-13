#ifndef CLOCK_SERVER_H
#define CLOCK_SERVER_H

#include "../util.h"
#include <stdint.h>
#include <stdbool.h>

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

// Clock request types and structures
enum class ClockRequestType { TIME, DELAY, DELAY_UNTIL , TICK };

void ClockServer();

#endif
