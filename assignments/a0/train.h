#ifndef _train_h
#define _train_h

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"

typedef struct
{
   int train_num;
   int train_speed; 
   bool isActive;
} TrainComponent;

typedef struct {
   int speed_setting;
   uint32_t actual_speed_mmps;  // Millimeters per second
   uint32_t stopping_distance_mm;
} SpeedCalibration;

// calibration data for train 77
/*
static SpeedCalibration calibration[] = {
   {6, 165469, 156800},   
   {9, 339192, 450200},   
   {12, 565448, 1002400}, 
};
*/

// calibration data for train 55
static SpeedCalibration calibration[] = {
   {6, 111300, 51000}, 
   {9, 286000, 227000}, 
   {12, 450000, 584000},
};

// toggle train direction
void accelerate_train(int train_num, int speed, bool headlightOn, int idx);
void reverse_train(int train_num);
void stop_train(int train_num);
void turn_on_headlight(int train_num);

int is_valid_train(int train_num);
bool is_moving(int idx);

#endif