#ifndef _switch_h_
#define _switch_h_

#include <stdbool.h>

#include "rpi.h"
#include "clock.h"
#include "track_node.h"

typedef enum
{
   STRAIGHT, // green
   CURVED,   // red
} SWITCH_STATE;

typedef struct
{
   int address;
   bool straight;
} Switch;

void switch_straight(int switch_number);
void switch_branch(int switch_number);
void set_all_switches(bool isStraight);
void init_switch_display(int LOCATION);
void switch_display(int LOCATION);

bool is_valid_switch(int switch_number);
int switch_num_to_address(int track_node_num);
void set_switches(const SwitchSetting* switches, int num_switches);

#endif