#ifndef _train_h
#define _train_h

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"

// toggle train direction
void accelerate_train(int train_num, int speed, bool headlightOn);
void reverse_train(int train_num);
void stop_train(int train_num);
void turn_on_headlight(int train_num);

#endif