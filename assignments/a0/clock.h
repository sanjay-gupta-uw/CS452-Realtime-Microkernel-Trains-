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

void clock_init(Clock *clock);
void clock_update(Clock *clock);
void clock_display(const Clock *clock);
#endif
