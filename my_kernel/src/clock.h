#ifndef _clock_h_
#define _clock_h_

#include "util.h"
#include <stdint.h>
#include <stdbool.h>

class Clock
{
public:
   int TICKS;
   // reintegrate with Tid: https://student.cs.uwaterloo.ca/~cs452/W25/assignments/kernel.html
   Clock();
   ~Clock();

   uint32_t Time();
   void Update();
   void Delay(uint32_t delay_ms);
   void Display(int LOCATION);
   void ReArmTimer(uint32_t delay_interval);
   void DisarmTimer();

private:
   uint32_t last_time;
   uint32_t minutes;
   uint32_t seconds;
   uint32_t tenths;
   bool UPDATE_DISPLAY;
};

#endif
