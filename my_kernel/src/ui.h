#ifndef _ui_h_
#define _ui_h_

#include "util.h"
// #include "rpi.h"

// assume terminal is 80x24
#define NUM_ROWS 24
#define NUM_COLS 80

#define CLOCK_LENGTH 8

#define SWITCH_LOCATION 3
#define SENSOR_LOCATION 35
#define CMD_LOCATION 60
#define COMMAND_STATUS_LOCATION 61

class Clock;
class Switches;
class CommandPrompt;

extern Clock clock;
extern Switches switches;
extern CommandPrompt cmd_prompt;

class UI
{
public:
   UI();
   void Update();
};

#endif
