#ifndef _train_h
#define _train_h

#include <stdbool.h>
// #include "rpi.h"

class Clock;

extern Clock clock;

// forward declaration
class Trains;
class Train
{
private:
   int train_num;
   int train_speed; // between 0 and 30 (ignore 15 + 16)

   void SendCommand(int data);
   void ActivateHeadlight();
   void Accelerate(int speed, bool headlightOn);
   void Reverse();
   void Stop();

public:
   Train();
   Train(int train_num);
   ~Train();

   friend class Trains;
};

class Trains
{
private:
   Train trains[5]; // Zero-initialize the entire array
public:
   Trains();
   ~Trains();

   int ActivateHeadlight(int train_num);
   int Accelerate(int train_num, int speed, bool headlightOn);
   int Reverse(int train_num);
   int Stop(int train_num);
};
#endif // _train_h