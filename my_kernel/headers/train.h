#ifndef _train_h
#define _train_h

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"

typedef struct
{
   int train_num;
   int train_speed; // between 0 and 30 (ignore 15 + 16)
   bool isActive;
} TrainComponent;

// toggle train direction
void accelerate_train(int train_num, int speed, bool headlightOn, int idx);
void reverse_train(int train_num);
void stop_train(int train_num);
void turn_on_headlight(int train_num);

int is_valid_train(int train_num);
bool is_moving(int idx);

#endif