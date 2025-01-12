#ifndef _switch_h_
#define _switch_h_

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"

typedef struct
{
   int address;
   bool straight;
} Switch;


void switch_straight(int switch_number);
void switch_branch(int switch_number);
void set_all_switches(bool isStraight);

#endif