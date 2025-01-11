#ifndef _train_h
#define _train_h
#include <stdbool.h>
#include <stdint.h>

// toggle train direction
void accelerate_train(uint8_t train_num, uint8_t speed, bool headlightOn);
void reverse_train(uint8_t train_num, bool headlightOn);
void stop_train(uint8_t train_num);
void turn_on_headlight(uint8_t train_num);

#endif