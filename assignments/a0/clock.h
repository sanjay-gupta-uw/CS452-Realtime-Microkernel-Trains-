#ifndef _clock_h_
#define _clock_h_

#include <stdint.h>

typedef struct
{
   uint32_t last_time; // Last time the clock was updated
   uint32_t minutes;   // Minutes part of the clock
   uint32_t seconds;   // Seconds part of the clock
   uint32_t tenths;    // Tenths of a second
} Clock;

extern Clock sys_clock;

void clock_init();
void clock_update();
void clock_delay(uint32_t delay_ms);
void clock_display();
#endif
